#include "bundle.h"
#include "patcher.h"
#include "version.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QMessageBox>
#include <zlib.h>
#if defined(_WIN32)
#  include <WinBase.h>
#else
#  include <unistd.h>
#  include <cstring>
#  include <cerrno>
#endif

Bundle::Bundle(QString install_path, Patcher *patcher)
 : QObject(), mInstallPath(install_path), mPatcher(patcher), mNeedsDownloading(false), mBufferSize(10*1024)
{
	mBuffer = NULL;
	mCurrentFile = NULL;
}

Bundle::~Bundle()
{
	if(mBuffer)
		delete[] mBuffer;
	
	if(mCurrentFile)
		delete mCurrentFile;
}

void Bundle::verify(QJsonObject* array)
{
	Q_ASSERT(array != NULL);

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
		file_entry.data_on_disk = NULL;

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

		if(mNeedsDownloading)
		{
			// No need to check files if we already know we need to download.
			mFiles.push_back(file_entry);
			continue;
		}

		file_entry.data_on_disk = new QByteArray(mPatcher->getFile(mInstallPath + "/" +file_entry.filename));
		if(file_entry.data_on_disk->isNull())
		{
			// File does not exist. Therefore this bundle needs to be downloaded.
			if(file_entry.size == 0)
			{
				// Except for zero byte files. Just create those.
				QFile file(mPatcher->getFile(mInstallPath + "/" +file_entry.filename));
				file.open(QFile::WriteOnly);
				file.close();
			}
			else
				mNeedsDownloading = true;
		}
		else
		{
			// Check the SHA1 checksum.
			file_entry.future = QtConcurrent::run(verifySHA1, file_entry, &mNeedsDownloading);
		}

		mFiles.push_back(file_entry);
	}

	for(QList<File>::iterator f = mFiles.begin(); f != mFiles.end(); ++f)
		(*f).future.waitForFinished();

	if(mNeedsDownloading)
		emit downloadMe();
	
	emit verifyDone();
}

bool Bundle::verifySHA1(Bundle::File file_entry, bool* downloading)
{
	if(*downloading)
		// No need to verify the SHA1 if we are already downloading;
		return false;

	QCryptographicHash hash(QCryptographicHash::Sha1);

	QBuffer buffer(file_entry.data_on_disk);
	if(buffer.open(QBuffer::ReadOnly))
	{
		while(!buffer.atEnd())
		{
			if(*downloading)
				return false;

			hash.addData(buffer.read(4096));
		}
		if(hash.result().toHex() == file_entry.checksum)
		{
//			qDebug() << "Checksum matches for" + file_entry.filename;
			return true;
		}
		else
		{
			qDebug() << "Checksum FAILED for" + file_entry.filename;
			qDebug() << file_entry.checksum << "!=" << hash.result().toHex();
			*downloading = true;
			return false;
		}
	}
	else
		;//FATAL
}

void Bundle::downloadAndExtract(QNetworkAccessManager* network_access_manager, QString download_url)
{
	QNetworkRequest request(QUrl(download_url.arg(mChecksum)));
	request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());

	mBytesDownloaded = 0;
	mBytesProgress = 0;
	mFilesCurrentIndex = 0;
	prepareFile();

	if(mCurrentFileIsGzipped)
		prepareZLib();

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
		if(reply->error() == QNetworkReply::NoError)
		{
			qint64 bytes_available = reply->bytesAvailable();
			if(bytes_available > 0)
			{
				while(mFilesCurrentIndex < mFiles.count() - 1 && mBytesDownloaded + bytes_available > mFiles[mFilesCurrentIndex+1].offset && bytes_available > 0)
				{
					qint64 bytes_available_previous_file = mFiles[mFilesCurrentIndex+1].offset - mBytesDownloaded;
					if(bytes_available_previous_file == 0)
					{
						nextFile();
						continue;
					}

					processData(reply, bytes_available_previous_file);
					bytes_available -= bytes_available_previous_file;

					mBytesDownloaded += bytes_available_previous_file;
				}

				processData(reply, bytes_available);
			}

			mBytesDownloaded += bytes_available;
		}
		else
		{
			if(reply->error() != QNetworkReply::OperationCanceledError)
				QMessageBox::critical(NULL, tr("Bundle"), tr("Error while getting Bundle (1).\n%1").arg(reply->errorString()));
		}
	}
}

void Bundle::processData(QNetworkReply *reply, qint64 bytes_available)
{
	if(bytes_available == 0)
		return;

	QByteArray input = reply->read(bytes_available);

	if(mCurrentFileIsGzipped)
	{
		int res;

		mZstream.next_in = (Bytef *)input.constData();
		mZstream.avail_in = bytes_available;

		do
		{
			uInt old_avail_out = mZstream.avail_out;
			res = inflate(&mZstream, Z_SYNC_FLUSH);
			if(res != Z_OK && res != Z_STREAM_END)
			{
				reply->abort();
				QMessageBox::warning(NULL, "ZLib", QString("ReadyRead error (%1)\n%2").arg(res).arg(mZstream.msg));
				return;
			}

			mCurrentFile->write((const char *)mBuffer, old_avail_out - mZstream.avail_out);
			mZstream.avail_out = mBufferSize;
			mZstream.next_out = mBuffer;
		}
		while(mZstream.avail_in > 0);

		if(res == Z_STREAM_END)
			nextFile();
	}
	else
		mCurrentFile->write(input);
}

void Bundle::downloadFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			qDebug() << "Download successful";
		}
		else
		{
			if(reply->error() != QNetworkReply::OperationCanceledError)
				QMessageBox::critical(NULL, tr("Bundle"), tr("Error while getting Bundle (2).\n%1").arg(reply->errorString()));
			reply->deleteLater();
			return;
		}

		if(mCurrentFileIsGzipped)
		{
			int res = inflateEnd(&mZstream);
			if(res != Z_OK && res != Z_STREAM_END)
			{
				reply->abort();
				QMessageBox::warning(NULL, "ZLib", QString("downloadFinished error (%1)\n%2").arg(res).arg(mZstream.msg));
				return;
			}
		}
		mCurrentFile->close();
		delete mCurrentFile;
		mCurrentFile = NULL;

		reply->deleteLater();
	}
}

void Bundle::downloadProgress(qint64 downloaded, qint64)
{
	emit downloadProgress(downloaded - mBytesProgress);
	mBytesProgress = downloaded;
}


void Bundle::prepareZLib()
{
	if(mBuffer == NULL)
		mBuffer = new Bytef[mBufferSize];

	mZstream.next_out = mBuffer;
	mZstream.avail_out = mBufferSize;
	mZstream.next_in = Z_NULL;
	mZstream.avail_in = 0;
	mZstream.zalloc = Z_NULL;
	mZstream.zfree = Z_NULL;
	mZstream.opaque = Z_NULL;
	
	int res = inflateInit2(&mZstream, 16 + MAX_WBITS);
	if(res != Z_OK)
	{
		QMessageBox::critical(NULL, "ZLib", QString("prepareZLib error (%1)\n%2").arg(res).arg(mZstream.msg));
		return;
	}
}

void Bundle::prepareFile()
{
	// Create the necessary directories.
	QFileInfo file_info(mInstallPath + "/" + mFiles[mFilesCurrentIndex].filename);
	file_info.absoluteDir().mkpath(".");

	mCurrentFile = new QFile(file_info.absoluteFilePath());
	Q_ASSERT(mCurrentFile->open(QFile::WriteOnly));
	if(mFiles[mFilesCurrentIndex].executable)
		mCurrentFile->setPermissions(mCurrentFile->permissions() | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);

	mCurrentFileIsGzipped = (mFiles[mFilesCurrentIndex].sizeZ > 0);
}

void Bundle::nextFile()
{
	mFilesCurrentIndex++;

	if(mFilesCurrentIndex < mFiles.count())
	{
		mCurrentFile->close();
		delete mCurrentFile;
		mCurrentFile = NULL;
		if(mCurrentFileIsGzipped)
		{
			int res = inflateEnd(&mZstream);
			if(res != Z_OK && res != Z_STREAM_END)
			{
//				reply->abort();
				QMessageBox::warning(NULL, "ZLib", QString("downloadFinished error (%1)\n%2").arg(res).arg(mZstream.msg));
				return;
			}
		}

		prepareFile();
		if(mCurrentFileIsGzipped)
			prepareZLib();
	}
}

bool Bundle::createSymbolicLink(const QString& from, const QString& to)
{
	// This is the same file, so make a symbolic link, but first
	// find the relative path.
	QStringList path1 = from.split(QRegExp("[\\\\/]"));
	QStringList path2 = to.split(QRegExp("[\\\\/]"));
	int i;
	for(i = 0; i < path2.count() - 1; i++)
	{
		if(path1[i] == path2[i])
		{
			path1[i] = "";
			path2[i] = "";
		}
		else
			break;
	}
	for(; i < path2.count() - 1; i++)
	{
		path1.push_front("..");
	}

	for(int i = path1.count() - 1; i >= 0; i--)
	{
		if(path1[i].isEmpty())
			path1.removeAt(i);
	}

#if defined(_WIN32)
	BOOLEAN res = CreateSymbolicLink((mInstallPath + "/" + to).toStdString().c_str(), path1.join("/").toStdString().c_str(), 0);
	if(!res)
	{
		qDebug() << GetLastError();
		return false;
	}
#else
	int res = symlink(path1.join("/").toStdString().c_str(), (mInstallPath + "/" + to).toStdString().c_str());
	if(res != 0)
	{
		qDebug() << strerror(errno);
		return false;
	}
#endif

	return true;
}

bool Bundle::createEmptyFile(const QString& file_name)
{
	QFileInfo(mInstallPath + '/' + file_name).absoluteDir().mkpath(".");
	QFile file(file_name);
	if(file.open(QFile::WriteOnly))
	{
		file.close();
		return true;
	}
	else
		return false;
}

#include "bundle.moc"
