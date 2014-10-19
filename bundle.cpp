#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
#include <QFileInfo>
#include <QDir>
#include <climits>
#include <qjson/serializer.h>
#include "bundle.h"
#include "sha1.h"
#include "information.h"

Bundle::Bundle(QString installpath, QVariant bundlevariant, QString downloadurl, QString titlefolder, QString authsuffix, QNetworkAccessManager *networkaccessmanager, QWidget *parent)
 : m_parent(parent), m_verification_state(unknown), m_error_occured(false)
{
	m_network_access_manager = networkaccessmanager;

	QVariantMap bundle = bundlevariant.toMap();
	QVariantList entries = bundle["entries"].toList();
	for(QVariantList::const_iterator entryvariant = entries.constBegin(); entryvariant != entries.constEnd(); ++entryvariant)
	{
		QVariantMap entryvariantmap = entryvariant->toMap();
		entry_t entry;
		entry.sizeZ = entryvariantmap["sizeZ"].toString().toULong();
		entry.offset = entryvariantmap["offset"].toString().toULong();
		entry.size = entryvariantmap["size"].toString().toULong();
		entry.fullfilename << installpath + entryvariantmap["filename"].toString();
		entry.checksumZ = entryvariantmap["checksumZ"].toString();
		entry.checksum = entryvariantmap["checksum"].toString();
		entry.executable = entryvariantmap.contains("executable") ? entryvariantmap["executable"].toBool() : false;
		m_entries.push_back(entry);
	}
	m_size = bundle["size"].toString().toULong();
	m_checksum = bundle["checksum"].toString();
	

	m_url = QUrl(QString("%1/%2/hashed/%3%4").arg(downloadurl).arg(titlefolder).arg(m_checksum).arg(authsuffix));

	// Some files are just zero bytes. So be it, but get rid of them.
	for(int i = m_entries.count()-1; i >= 0; i--)
	{
		if(m_entries[i].size == 0)
		{
			QFileInfo fileinfo(m_entries[i].fullfilename[0]);
			fileinfo.absoluteDir().mkpath(".");
			info.log("Empty file creation", m_entries[i].fullfilename[0]);

			QFile file(m_entries[i].fullfilename[0]);
			m_entries.removeAt(i);
			if(!file.open(QIODevice::WriteOnly))
			{
				info.critical("File creation", "Could not create the file \"" + file.fileName() + "\"");
			}
			file.close();
		}
	}

	// Some files are just a copy and have the same offset. Sneaky!
	for(int i = m_entries.count()-1; i >= 0; i--)
	{
		for(int j = i-1; j >= 0; j--)
		{
			if(m_entries[i].offset == m_entries[j].offset && m_entries[i].size != 0 && m_entries[j].size != 0)
			{
				m_entries[j].fullfilename << m_entries[i].fullfilename;
				m_entries.removeAt(i);
				break;
			}
		}
	}

	int lastoffset = INT_MAX;
	for(int i = m_entries.count()-1; i >= 0; i--)
	{
		m_entries[i].next_offset = lastoffset;
		lastoffset = m_entries[i].offset;
	}
}

Bundle::~Bundle()
{
	for(int i = 0; i < m_entry_file.count(); i++)
	{
		m_entry_file[i]->close();
		delete m_entry_file[i];
	}

	m_cache_file.close();
}


// VERIFICATION
void Bundle::verifyAndMaybeDownload()
{
	m_verification_state = unknown;
	QFuture<bool> status = QtConcurrent::mapped(m_entries, verifyEntry);
	QFutureWatcher<bool> *statuswatcher = new QFutureWatcher<bool>(this);
	connect(statuswatcher, SIGNAL(finished()), SLOT(verifyFinished()));
	statuswatcher->setFuture(status);
}

bool Bundle::verifyEntry(Bundle::entry_t entry)
{
	char buffer[33];
	for(QStringList::const_iterator filename = entry.fullfilename.constBegin(); filename != entry.fullfilename.constEnd(); ++filename)
	{
		SHA1::calculateSHA1(filename->toStdString().c_str(), buffer);
		if(entry.checksum != buffer)
			return false;
	}

	return true;
}

void Bundle::verifyFinished()
{
	QFutureWatcher<bool> *watcher = dynamic_cast< QFutureWatcher<bool>* >(sender());
	if(watcher)
	{
		QFuture<bool> status = watcher->future();
		for(QFuture<bool>::const_iterator bundle_ok = status.constBegin(); bundle_ok != status.constEnd(); ++bundle_ok)
		{
			if(!*bundle_ok)
			{
				info.log("Download required for", m_checksum);
				m_verification_state = bad;
				download();
				emit verifyDone(m_size);
				return;
			}
		}
		m_verification_state = good;
		emit verifyDone(0);
	}
}


// DOWNLOADING
void Bundle::download()
{
	m_current_entry_index = 0;

	for(int i = 0; i < m_entry_file.count(); i++)
	{
		m_entry_file[i]->close();
		delete m_entry_file[i];
	}
	m_entry_file.clear();

	// Open the file or create it including the directories.
	for(int i = 0; i < m_entries[0].fullfilename.count(); i++)
	{
		QFileInfo fileinfo(m_entries[0].fullfilename[i]);
		fileinfo.absoluteDir().mkpath(".");

		QFile *entryfile = new QFile(m_entries[0].fullfilename[i]);
		m_entry_file << entryfile;
		if(!m_entry_file[i]->open(QIODevice::WriteOnly))
		{
			m_error_occured = true;
			info.critical(tr("I/O error"), tr("Could not open file \"%1\" for writing.").arg(m_entries[0].fullfilename[i]));
			emit errorOccurred();
			return;
		}
		info.log("File creation", m_entries[0].fullfilename[i]);

		if(m_entries[0].executable)
			m_entry_file[i]->setPermissions(m_entry_file[i]->permissions() | QFile::ExeGroup | QFile::ExeOther | QFile::ExeOwner | QFile::ExeUser);
	}

	// Set up Zlib for on-the-fly decompression;
	m_gzipstream.zalloc = Z_NULL;
	m_gzipstream.zfree = Z_NULL;
	m_gzipstream.opaque = Z_NULL;
	m_gzipstream.next_in = Z_NULL;
	m_gzipstream.avail_in = 0;
	m_gzipstream.next_out = Z_NULL;
	m_gzipstream.avail_out = 0;

	// 16+MAX_WBITS means read as gzip.
	if(inflateInit2(&m_gzipstream, 16 + MAX_WBITS) != Z_OK)
	{
		m_error_occured = true;
		emit errorOccurred();
		info.critical("ZLib", tr("Couldn't init zlibstream."));
		return;
	}

	m_alreadyread = 0;
	m_alreadydownloaded = 0;
	QNetworkRequest request(m_url);
	QNetworkReply *reply = m_network_access_manager->get(request);
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(downloadProgress(qint64,qint64)));
	connect(reply, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(reply, SIGNAL(readyRead()), SLOT(readyRead()));
}

void Bundle::nextFile(QNetworkReply* reply)
{
	for(int i = 0; i < m_entry_file.count(); i++)
	{
		m_entry_file[i]->close();
		delete m_entry_file[i];
	}
	m_entry_file.clear();

	info.log("Next file", QString("%1:%2").arg(m_checksum).arg(m_current_entry_index+1), true);

	
	m_current_entry_index++;
	if(m_current_entry_index == m_entries.count())
	{
		inflateEnd(&m_gzipstream);
		return;
	}

	if(m_current_entry_index >= m_entries.count())
	{
		m_error_occured = true;
		emit errorOccurred();
		reply->abort();
		info.critical("I/O error", "More files than expected!");
		return;
	}

	for(int i = 0; i < m_entries[m_current_entry_index].fullfilename.count(); i++)
	{
		QFileInfo fileinfo(m_entries[m_current_entry_index].fullfilename[i]);
		fileinfo.absoluteDir().mkpath(".");

		QFile *entryfile = new QFile(m_entries[m_current_entry_index].fullfilename[i]);
		m_entry_file << entryfile;
		if(!m_entry_file[i]->open(QIODevice::WriteOnly))
		{
			info.critical(tr("I/O error"), tr("Could not open file \"%1\" for writing.").arg(m_entries[m_current_entry_index].fullfilename[i]));
			exit(1);
		}
		info.log("File creation", m_entries[m_current_entry_index].fullfilename[i]);

		if(m_entries[m_current_entry_index].executable)
			m_entry_file[i]->setPermissions(m_entry_file[i]->permissions() | QFile::ExeGroup | QFile::ExeOther | QFile::ExeOwner | QFile::ExeUser);
	}

	inflateEnd(&m_gzipstream);
	m_gzipstream.zalloc = Z_NULL;
	m_gzipstream.zfree = Z_NULL;
	m_gzipstream.opaque = Z_NULL;
	m_gzipstream.next_in = Z_NULL;
	m_gzipstream.avail_in = 0;
	m_gzipstream.next_out = Z_NULL;
	m_gzipstream.avail_out = 0;

	// 16+MAX_WBITS means read as gzip.
	if(inflateInit2(&m_gzipstream, 16 + MAX_WBITS) != Z_OK)
	{
		m_error_occured = true;
		emit errorOccurred();
		reply->abort();
		info.critical("ZLib", tr("Couldn't init zlibstream."));
		return;
	}
}


void Bundle::downloadFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		inflateEnd(&m_gzipstream);

		if(reply->error())
		{
			QString error = reply->errorString();
			info.critical(tr("Download error"), error);
			exit(1);
		}
		else
		{
			QByteArray streamdata = reply->readAll();
			if(streamdata.count() != 0)
			{
				info.critical("Download error", "Still data left!");
				exit(1);
			}

			emit done();
		}

		reply->deleteLater();
	}
}

void Bundle::downloadProgress(qint64 value, qint64)
{
	emit downloadProgress(value - m_alreadydownloaded);
	m_alreadydownloaded = value;
}

void Bundle::readyRead()
{
	if(m_error_occured)
		return;

	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		do
		{
			if(m_current_entry_index >= m_entries.count())
				break;

			QByteArray streamdata = reply->read(m_entries[m_current_entry_index].next_offset - m_alreadyread);
			if(streamdata.count() == 0)
				break;

			QByteArray outputdata(8*1024, Qt::Uninitialized);

			// If there is no checksumZ, then it's not gzipped.
			if(m_entries[m_current_entry_index].checksumZ == "")
			{
				for(QList<QFile *>::iterator entryfile = m_entry_file.begin(); entryfile != m_entry_file.end(); ++entryfile)
				{
					qint64 really_written = (*entryfile)->write(streamdata);
					if(really_written == -1)
					{
						m_error_occured = true;
						emit errorOccurred();
						reply->abort();
						info.critical(tr("Write error"), tr("Error while writing to %1 (%2).").arg((*entryfile)->fileName()).arg((*entryfile)->errorString()));
						return;
					}
				}
				info.log(tr("Writing to file"), QString("(1) ") + tr("%1 bytes written to %2.").arg(streamdata.count()).arg(m_entry_file[0]->fileName()), true);
				m_alreadyread += streamdata.count();

				if(m_alreadyread == m_entries[m_current_entry_index].next_offset)
					nextFile(reply);
			}
			else
			{
				m_gzipstream.next_in = (Bytef *)streamdata.data();
				m_gzipstream.avail_in = streamdata.count();
				m_gzipstream.next_out = (Bytef *)outputdata.data();
				m_gzipstream.avail_out = outputdata.count();
				int inflate_status;
				do
				{
					inflate_status = inflate(&m_gzipstream, Z_SYNC_FLUSH);
					
					if(m_gzipstream.avail_out == 0)
					{
						for(QList<QFile *>::iterator entryfile = m_entry_file.begin(); entryfile != m_entry_file.end(); ++entryfile)
						{
							qint64 really_written = (*entryfile)->write(outputdata);
							if(really_written == -1)
							{
								m_error_occured = true;
								emit errorOccurred();
								reply->abort();
								info.critical(tr("Write error"), tr("Error while writing to %1 (%2).").arg((*entryfile)->fileName()).arg((*entryfile)->errorString()));
								return;
							}
						}
						info.log("Writing to file", QString("(2) %1 bytes written to %2.").arg(outputdata.count()).arg(m_entry_file[0]->fileName()), true);
						m_gzipstream.next_out = (Bytef *)outputdata.data();
						m_gzipstream.avail_out = outputdata.count();
					}
				}
				while(inflate_status == Z_OK && m_gzipstream.avail_in > 0);

				outputdata.truncate(outputdata.count()-m_gzipstream.avail_out);
				for(QList<QFile *>::iterator entryfile = m_entry_file.begin(); entryfile != m_entry_file.end(); ++entryfile)
				{
					qint64 really_written = (*entryfile)->write(outputdata);
					if(really_written == -1)
					{
						m_error_occured = true;
						emit errorOccurred();
						reply->abort();
						info.critical(tr("Write error"), tr("Error while writing to %1 (%2).").arg((*entryfile)->fileName()).arg((*entryfile)->errorString()));
						return;
					}
				}
				info.log("Writing to file", QString("(3) %1 bytes written to %2.").arg(outputdata.count()).arg(m_entry_file[0]->fileName()), true);
				m_alreadyread += streamdata.count();

				if(inflate_status == Z_STREAM_END)
					nextFile(reply);
				else if(inflate_status < 0)
				{
					m_error_occured = true;
					emit errorOccurred();
					reply->abort();
					info.critical(tr("I/O error"), tr("Decompress error"));
					return;
				}
			}
		}
		while(!reply->atEnd());
	}
}


#include "bundle.moc"
