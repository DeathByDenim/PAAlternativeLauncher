#include "patcher.h"
#include <QStringList>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <zlib.h>
#include <qjson/parser.h>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrentMap>
#include <QFuture>
#include <QFutureWatcher>
#include "sha1.h"

QString Patcher::m_install_path;

Patcher::Patcher(QWidget* parent)
 : m_parent(parent)
{
	m_access_manager = new QNetworkAccessManager(this);
	connect(m_access_manager, SIGNAL(finished(QNetworkReply*)), SLOT(replyFinished(QNetworkReply*)));
}

Patcher::~Patcher()
{
	m_manifest_file.close();
}

void Patcher::decodeStreamsData(QByteArray data)
{
	int curlylevel = 0;
	int squarelevel = 0;
	bool insidequotation = false;
	bool afterbackslash = false;

	QString buffer;

	Stream currentstream;

	enum
	{
		none,
		streamname,
		buildid,
		description,
		downloadurl,
		authsuffix,
		manifestname,
		titlefolder
	} next = none;

	for(int i = 0; i < data.size(); i++)
	{
		if(insidequotation)
		{
			afterbackslash = false;

			if(data[i] == '\\' && !afterbackslash)
				afterbackslash = true;
			else if(data[i] == '"')
			{
				insidequotation = false;
				
				if(buffer == "StreamName")
					next = streamname;
				else if(buffer == "BuildId")
					next = buildid;
				else if(buffer == "Description")
					next = description;
				else if(buffer == "DownloadUrl")
					next = downloadurl;
				else if(buffer == "AuthSuffix")
					next = authsuffix;
				else if(buffer == "ManifestName")
					next = manifestname;
				else if(buffer == "TitleFolder")
					next = titlefolder;
				else
				{
					switch(next)
					{
						case streamname:
							currentstream.StreamName = buffer;
							break;
						case buildid:
							currentstream.BuildId = buffer;
							break;
						case description:
							currentstream.Description = buffer;
							break;
						case downloadurl:
							currentstream.DownloadUrl = buffer;
							break;
						case authsuffix:
							currentstream.AuthSuffix = buffer;
							break;
						case manifestname:
							currentstream.ManifestName = buffer;
							break;
						case titlefolder:
							currentstream.TitleFolder = buffer;
							break;
						default:
							break;
					}
					next = none;
				}

				buffer = "";
			}
			else
				buffer += data[i];
		}
		else
		{
			switch(data[i])
			{
				case '{':
					curlylevel++;
					break;
				case '}':
					if(curlylevel == 2)
					{
						m_streams << currentstream;
						currentstream.AuthSuffix = "";
						currentstream.BuildId = "";
						currentstream.Description = "";
						currentstream.DownloadUrl = "";
						currentstream.ManifestName = "";
						currentstream.StreamName = "";
						currentstream.TitleFolder = "";
					}
					curlylevel--;
					if(curlylevel < 0)
						return;
					break;
				case '[':
					squarelevel++;
					break;
				case ']':
					squarelevel--;
					if(squarelevel < 0)
						return;
					break;
				case '"':
					insidequotation = true;
					break;
				default:
					break;
			}
		}
	}
}

QStringList Patcher::streamNames()
{
	QStringList result;
	for(QList<Stream>::const_iterator stream = m_streams.constBegin(); stream != m_streams.constEnd(); ++stream)
	{
		result << stream->StreamName;
	}

	return result;
}

void Patcher::downloadStream(const Patcher::Stream &stream)
{
	m_manifest_file.setFileName("manifest.gz");
	m_manifest_file.open(QIODevice::WriteOnly);
	QUrl manifesturl(stream.DownloadUrl + '/' + stream.TitleFolder + '/' + stream.ManifestName + stream.AuthSuffix);
	QNetworkRequest request(manifesturl);
	request.setAttribute(QNetworkRequest::User, "manifest");
	QNetworkReply *reply = m_access_manager->get(request);
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), SLOT(manifestDownloadProgress(qint64,qint64)));
	connect(reply, SIGNAL(readyRead()), SLOT(manifestReadyRead()));
}

void Patcher::downloadStream(const QString streamname)
{
	for(QList<Stream>::const_iterator stream = m_streams.constBegin(); stream != m_streams.constEnd(); ++stream)
	{
		if(streamname == stream->StreamName)
		{
			downloadStream(*stream);
		}
	}
}

void Patcher::replyFinished(QNetworkReply* reply)
{
	if(reply->error() != QNetworkReply::NoError)
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		QMessageBox::critical(m_parent, QString("Communication error"), "Error while doing " + type + ". Details: " + reply->errorString() + ".");
	}
	else if(reply->isFinished())
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		if(type == "manifest")
		{
			QByteArray data = reply->readAll();
			qDebug() << "read" << data.size() << "bytes";
			m_manifest_file.write(data);
			m_manifest_file.close();
		}
	}
	reply->deleteLater();
}

void Patcher::manifestDownloadProgress(qint64, qint64)
{
}

void Patcher::manifestReadyRead()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		QByteArray data = reply->readAll();
		qDebug() << "read" << data.size() << "bytes";
		m_manifest_file.write(data);
	}
}

void Patcher::test()
{
	QByteArray manifestdata(10*1024*1024, Qt::Uninitialized);
	gzFile gzf = gzopen("manifest.gz", "r");
	if(gzf)
	{
		int n_read;
		char *buffer = new char[10*1024*1024];
		if((n_read = gzread(gzf, manifestdata.data(), 10*1024*1024)) > 0)
		{
			manifestdata.truncate(n_read);

			bool ok;
			QJson::Parser parser;
			m_manifest = parser.parse(manifestdata, &ok).toMap();
			manifestdata.resize(0);
			if(ok)
				processManifest(m_manifest);
		}
		else
		{
			int errnum;
			QMessageBox::critical(m_parent, "Decompress error", QString("Error while decompressing manifest.gz.\nError was \"%1\".").arg(gzerror(gzf, &errnum)));
		}
		delete[] buffer;
		gzclose(gzf);
	}
}

void Patcher::processManifest(QVariantMap manifest)
{
	QVariantList bundles = manifest["bundles"].toList();
	m_num_items = bundles.count();
	QFuture<bool> status = QtConcurrent::mapped(bundles, verify);
	QFutureWatcher<bool> *statuswatcher = new QFutureWatcher<bool>(this);
	connect(statuswatcher, SIGNAL(progressValueChanged(int)), SLOT(verifyProgressValueChanged(int)));
	connect(statuswatcher, SIGNAL(finished()), SLOT(verifyFinished()));
	statuswatcher->setFuture(status);
}

bool Patcher::verify(const QVariant& bundle)
{
	QVariantList entries = bundle.toMap()["entries"].toList();
	for(QVariantList::const_iterator entry = entries.constBegin(); entry != entries.constEnd(); ++entry)
	{
		QVariantMap entrymap = entry->toMap();
		if(QFileInfo(m_install_path + entrymap["filename"].toString()).exists())
		{
			if(entrymap["checksum"].toString() != (calculateSHA1(m_install_path + entrymap["filename"].toString())))
				return false;
		}
	}

	return true;
}

void Patcher::setInstallPath(QString installpath)
{
	QDir installdir(installpath);
	if(!installdir.mkpath("."))
	{
		QMessageBox::critical(m_parent, "Directory creation", QString("Failed to create directory at \"%1\".)").arg(installpath));
		return;
	}

	m_install_path = installdir.absolutePath() + '/';
}

QString Patcher::calculateSHA1(QString filename)
{
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		return QString();

	SHA1 sha;
	
	QByteArray data = file.readAll();
	sha.Input(data.data(), data.size());
	
	unsigned int result[5];
	sha.Result(result);

	return QString("%1%2%3%4%5").arg(result[0], 8, 16, QChar('0')).arg(result[1], 8, 16, QChar('0')).arg(result[2], 8, 16, QChar('0')).arg(result[3], 8, 16, QChar('0')).arg(result[4], 8, 16, QChar('0'));
}

void Patcher::verifyProgressValueChanged(int value)
{
	emit progress(100. * value / m_num_items);
}

void Patcher::verifyFinished()
{
	QFutureWatcher<bool> *watcher = dynamic_cast< QFutureWatcher<bool>* >(sender());
	if(watcher)
	{
		QFuture<bool> status = watcher->future();
		for(QFuture<bool>::const_iterator bundle_ok = status.constBegin(); bundle_ok != status.constEnd(); ++bundle_ok)
		{
			if(!*bundle_ok)
			{
				QMessageBox::critical(m_parent, "Verify", "Verify error!");
				return;
			}
		}
		QMessageBox::information(m_parent, "Verify", "Verification was ok!");
	}
}

#include "patcher.moc"
