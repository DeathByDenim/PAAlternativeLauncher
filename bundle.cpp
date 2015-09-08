#include "bundle.h"
#include "patcher.h"
#include "version.h"
#include "information.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <zlib.h>

Bundle::Bundle(QString install_path, Patcher *patcher)
 : QObject(), mInstallPath(install_path), mPatcher(patcher), mNeedsDownloading(false), mZstreamBufferSize(1024*1024)
{
}

Bundle::~Bundle()
{
}

void Bundle::verify(QJsonObject* array)
{
	Q_ASSERT(array != NULL);

	mNumVerified = 0;
	mNumToVerify = 0;
	
	mChecksum = (*array)["checksum"].toString();
	mTotalSize = (*array)["size"].toString().toULong();
	QJsonArray entries = (*array)["entries"].toArray();

	for(QJsonArray::const_iterator e = entries.constBegin(); e != entries.constEnd(); ++e)
	{
		File file_entry;
		file_entry.filename = (*e).toObject()["filename"].toString();
		file_entry.checksum = (*e).toObject()["checksum"].toString();
		file_entry.offset = (*e).toObject()["offset"].toString().toULong();
		file_entry.size = (*e).toObject()["size"].toString().toULong();
		file_entry.sizeZ = (*e).toObject()["sizeZ"].toString().toULong();
		file_entry.executable = (*e).toObject()["executable"].toBool();
		file_entry.file = NULL;
		file_entry.compressed = (file_entry.sizeZ != 0);
		file_entry.zbuffer = NULL;

		if(file_entry.size == 0)
		{
			createEmptyFile(file_entry.filename);
			continue;
		}

		bool was_symlinked = false;
		for(QList<File>::const_iterator prevfile = mFiles.constBegin(); prevfile != mFiles.constEnd(); ++prevfile)
		{
			if(prevfile->offset == file_entry.offset && prevfile->size != 0 && file_entry.size != 0)
			{
				createSymbolicLink(prevfile->filename, file_entry.filename);
				was_symlinked = true;
				break;
			}
		}

		if(was_symlinked)
			continue;

		if(file_entry.size == 0)
		{
			// Zero byte files. Just create those.
			QFile file(mPatcher->getFile(mInstallPath + "/" +file_entry.filename));
			if(!file.open(QFile::WriteOnly))
			{
				error_occurred = true;
				emit error(tr("Couldn't create empty file \"%1\"").arg(file.fileName()));
			}
			file.close();
			continue;
		}

		if(mNeedsDownloading)
		{
			// No need to check files if we already know we need to download.
			mFiles.push_back(file_entry);
			continue;
		}

		mNumToVerify++;

		// Check the SHA1 checksum.
		file_entry.future = QtConcurrent::run(verifySHA1, file_entry, &mNeedsDownloading, mPatcher, mInstallPath);
		QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
		watcher->setFuture(file_entry.future);
		connect(watcher, SIGNAL(finished()), SLOT(verifyFinished()));

		mFiles.push_back(file_entry);
	}
}

bool Bundle::verifySHA1(Bundle::File file_entry, bool* downloading, Patcher *patcher, QString install_path)
{
	if(*downloading)
		// No need to verify the SHA1 if we are already downloading;
		return false;

	QByteArray data_on_disk(patcher->getFile(install_path + "/" +file_entry.filename));
	if(data_on_disk.isNull())
	{
		// File does not exist. Therefore this bundle needs to be downloaded.
		*downloading = true;
		return false;
	}

	QCryptographicHash hash(QCryptographicHash::Sha1);

	QBuffer buffer(&data_on_disk);
	if(buffer.open(QBuffer::ReadOnly))
	{
		while(!buffer.atEnd())
		{
			if(*downloading || error_occurred)
				return false;

			hash.addData(buffer.read(4096));
		}
		buffer.close();

		if(hash.result().toHex() == file_entry.checksum)
		{
			return true;
		}
		else
		{
			info.log("Verification failed", file_entry.filename, true);
			*downloading = true;
			return false;
		}
	}
	else
		return false;//FATAL
}

void Bundle::verifyFinished()
{
	mNumVerified++;
	if(mNumToVerify == mNumVerified)
	{
		emit verifyDone();

		if(mNeedsDownloading)
			emit downloadMe();
		else
			emit finished();
	}
}

void Bundle::downloadAndExtract(QNetworkAccessManager* network_access_manager, QString download_url)
{
	info.log("Download bundle", mChecksum);

	QNetworkRequest request(QUrl(download_url.arg(mChecksum)));
	request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
	request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());

	mBytesDownloaded = 0;
	mBytesProgress = 0;

	QNetworkReply *reply = network_access_manager->get(request);
	connect(reply, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(reply, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(downloadProgress(qint64,qint64)));
}

void Bundle::downloadReadyRead()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(error_occurred)
		{
			reply->abort();
			return;
		}

		if(reply->error() == QNetworkReply::NoError)
		{
			qint64 bytes_available = reply->bytesAvailable();
			if(bytes_available > 0)
			{
				processData(reply, bytes_available);
			}

			mBytesDownloaded += bytes_available;
		}
		else
		{
			if(reply->error() != QNetworkReply::OperationCanceledError)
			{
				emit error(tr("Error while getting Bundle (1).\n%1").arg(reply->errorString()));
				error_occurred = true;
			}
		}
	}
}

void Bundle::processData(QNetworkReply *reply, qint64 bytes_available)
{
	if(!reply)
	{
		error_occurred = true;
		emit error("NULL reply in processData");
		return;
	}

	QByteArray data = reply->readAll();

	for(QList<File>::iterator f = mFiles.begin(); f != mFiles.end(); ++f)
	{
// Three cases to visualize the calculations
//
//    |.......*.....|.............|..*...........|
//    0       6     10           20  22          30
//
//  first block:  mBytesDownloaded =  0
//                bytes_available  = 10
//                f->offset        =  6
//                endoffset        = 22
//
//  second block: mBytesDownloaded = 10
//                bytes_available  = 10
//                f->offset        =  6
//                endoffset        = 22
//
//  third block:  mBytesDownloaded = 20
//                bytes_available  = 10
//                f->offset        =  6
//                endoffset        = 22

// Within block case
//
//    |..........*....*........|
//   10         18    22      30
//
//                mBytesDownloaded = 10
//                bytes_available  = 20
//                f->offset        = 18
//                endoffset        = 22

		ulong endoffset = (f->compressed ? f->offset + f->sizeZ : f->offset + f->size);
		if(mBytesDownloaded + bytes_available > f->offset && mBytesDownloaded <= endoffset)
		{
			// Our file has data in this block.

			if(f->file == NULL)
			{
				prepareFile(&(*f));
			}
			ulong start_in_data = 0;
			if(f->offset > mBytesDownloaded)
				start_in_data = f->offset - mBytesDownloaded;

			ulong length_in_data = bytes_available - start_in_data;
			if(mBytesDownloaded + bytes_available > endoffset)
			{
				Q_ASSERT(endoffset >= mBytesDownloaded + start_in_data);
				length_in_data = endoffset - mBytesDownloaded - start_in_data;
			}

			if(f->compressed)
			{
				int res;

				QByteArray snippet(data.mid(start_in_data, length_in_data).constData(), length_in_data);
				f->zstream.next_in = (Bytef *)snippet.constData();
				f->zstream.avail_in = length_in_data;

				do
				{
					uInt old_avail_out = f->zstream.avail_out;
					res = inflate(&(f->zstream), Z_SYNC_FLUSH);
					if(res != Z_OK && res != Z_STREAM_END)
					{
						error_occurred = true;
						reply->abort();
						emit error(QString("ZLib ReadyRead error (%1)\n%2").arg(mChecksum).arg(f->zstream.msg));
						return;
					}

					f->file->write((const char *)f->zbuffer, old_avail_out - f->zstream.avail_out);
					f->zstream.avail_out = mZstreamBufferSize;
					f->zstream.next_out = f->zbuffer;
				}
				while(f->zstream.avail_in > 0);
			}
			else
				f->file->write(data.mid(start_in_data, length_in_data));
		}
		else
		{
			// Our file has no business here. So if it's open, close it.
			closeFile(&(*f));
		}
	}
}

void Bundle::downloadFinished()
{
	if(error_occurred)
		return;

	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() != QNetworkReply::NoError)
		{
			if(reply->error() != QNetworkReply::OperationCanceledError)
			{
				error_occurred = true;
				emit error(tr("Error while getting Bundle (2).\n%1").arg(reply->errorString()));
			}
			reply->deleteLater();
			return;
		}

		for(QList<File>::iterator f = mFiles.begin(); f != mFiles.end(); ++f)
		{
			closeFile(&(*f));
		}


		reply->deleteLater();

		emit finished();
	}
}

void Bundle::downloadProgress(qint64 downloaded, qint64)
{
	emit downloadProgress(downloaded - mBytesProgress);
	mBytesProgress = downloaded;
}

void Bundle::prepareFile(File *file)
{
	// Create the necessary directories.
	QFileInfo file_info(mInstallPath + "/" + file->filename);
	file_info.absoluteDir().mkpath(".");

	file->file = new QFile(file_info.absoluteFilePath());
	if(!file->file->open(QFile::WriteOnly))
	{
		error_occurred = true;
		emit error(QString("Can't write to \"%1\"").arg(file->filename));
		return;
	}
	if(file->compressed)
	{
		file->zbuffer = new Bytef[mZstreamBufferSize];
		file->zstream.next_out = file->zbuffer;
		file->zstream.avail_out = mZstreamBufferSize;
		file->zstream.next_in = Z_NULL;
		file->zstream.avail_in = 0;
		file->zstream.zalloc = Z_NULL;
		file->zstream.zfree = Z_NULL;
		file->zstream.opaque = Z_NULL;

		int res = inflateInit2(&(file->zstream), 16 + MAX_WBITS);
		if(res != Z_OK)
		{
			error_occurred = true;
			emit error(QString("prepareZLib error (%1)\n%2").arg(res).arg(file->zstream.msg));
			return;
		}
	}
	if(file->executable)
		file->file->setPermissions(file->file->permissions() | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
}

void Bundle::closeFile(File* file)
{
	if(file->file != NULL)
	{
		file->file->close();
		delete file->file;
		file->file = NULL;

		if(file->zbuffer)
		{
			delete[] file->zbuffer;
			file->zbuffer = NULL;
			int res = inflateEnd(&file->zstream);
			if(res != Z_OK && res != Z_STREAM_END)
			{
				error_occurred = true;
				emit error(QString("ZLib closeFile error (%1)\n%2").arg(res).arg(file->zstream.msg));
				return;
			}
		}
	}
}

bool Bundle::createSymbolicLink(const QString& from, const QString& to)
{
	// Add to the list, which is retrieved by Patcher later to make the actual links.
	mSymLinkLater[mInstallPath + "/" + to] = mInstallPath + "/" + from;

	return true;
}

bool Bundle::createEmptyFile(const QString& file_name)
{
	QFileInfo(mInstallPath + '/' + file_name).absoluteDir().mkpath(".");
	QFile file(mInstallPath + '/' + file_name);
	if(file.open(QFile::WriteOnly))
	{
		file.close();
		return true;
	}
	else
		return false;
}

#include "bundle.moc"
