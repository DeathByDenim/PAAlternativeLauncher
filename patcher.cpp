#include "patcher.h"
#include <QStringList>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <zlib.h>
#ifdef _WIN32
#  define QJSON_STATIC
#endif
#include <qjson/parser.h>
#include <QDir>
#include <QtConcurrentMap>
#include <QFuture>
#include <QFutureWatcher>
#include "sha1.h"
#include "information.h"

Patcher::Patcher(QWidget* parent)
 : m_parent(parent)
 , m_buffer_increment(8*1024)
 , m_manifest_bytearray(m_buffer_increment, Qt::Uninitialized)
 , m_error_occured(false)
{
	m_access_manager = new QNetworkAccessManager(this);
}

Patcher::~Patcher()
{
	while(!m_bundles.isEmpty())
	{
		delete m_bundles[0];
		m_bundles.removeFirst();
	}
}

void Patcher::decodeStreamsData(QByteArray data)
{
	QJson::Parser parser;
	bool ok;
	QVariantList streams = parser.parse(data, &ok).toMap()["Streams"].toList();
	if(!ok)
	{
		info.critical(tr("JSON error"), tr("Could not decode streams JSON. File was corrupted."));
		return;
	}
	
	for(QVariantList::const_iterator stream = streams.constBegin(); stream != streams.constEnd(); ++stream)
	{
		QVariantMap streammap = stream->toMap();

		Stream currentstream;
		currentstream.AuthSuffix = streammap["AuthSuffix"].toString();
		currentstream.BuildId = streammap["BuildId"].toString();
		currentstream.Description = streammap["Description"].toString();
		currentstream.DownloadUrl = streammap["DownloadUrl"].toString();
		currentstream.ManifestName = streammap["ManifestName"].toString();
		currentstream.StreamName = streammap["StreamName"].toString();
		currentstream.TitleFolder = streammap["TitleFolder"].toString();

		m_streams << currentstream;
	}
}

QString Patcher::buildId(QString streamname)
{
	for(QList<Stream>::const_iterator stream = m_streams.constBegin(); stream != m_streams.constEnd(); ++stream)
	{
		if(streamname == stream->StreamName)
		{
			return stream->BuildId;
		}
	}
	
	return QString();
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

void Patcher::downloadStream(const QString streamname)
{
	while(!m_bundles.isEmpty())
	{
		delete m_bundles[0];
		m_bundles.removeFirst();
	}

	for(QList<Stream>::const_iterator stream = m_streams.constBegin(); stream != m_streams.constEnd(); ++stream)
	{
		if(streamname == stream->StreamName)
		{
			downloadManifest(*stream);
		}
	}
}

void Patcher::downloadManifest(const Patcher::Stream &stream)
{
	m_error_occured = false;

	m_current_stream = stream;

	m_manifest_zstream.zalloc = Z_NULL;
	m_manifest_zstream.zfree = Z_NULL;
	m_manifest_zstream.opaque = Z_NULL;
	m_manifest_zstream.next_in = Z_NULL;
	m_manifest_zstream.avail_in = 0;
	m_manifest_zstream.next_out = (Bytef *)m_manifest_bytearray.data();
	m_manifest_zstream.avail_out = m_manifest_bytearray.count();

	// 16+MAX_WBITS means read as gzip.
#ifdef _WIN32
	if(inflateInit2(&m_manifest_zstream, 16 + MAX_WBITS) != Z_OK)
#else
	if(inflateInit2(&m_manifest_zstream, 16 + MAX_WBITS) != Z_OK)
#endif
	{
		m_error_occured = true;
		info.critical("ZLib", tr("Couldn't init zlibstream."));
		return;
	}

	QUrl manifesturl(stream.DownloadUrl + '/' + stream.TitleFolder + '/' + stream.ManifestName + stream.AuthSuffix);
	QNetworkRequest request(manifesturl);
	QNetworkReply *reply = m_access_manager->get(request);
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), SLOT(manifestDownloadProgress(qint64,qint64)));
	connect(reply, SIGNAL(readyRead()), SLOT(manifestReadyRead()));
	connect(reply, SIGNAL(finished()), SLOT(manifestFinished()));
	emit state("Downloading manifest");
}

void Patcher::manifestReadyRead()
{
	if(m_error_occured)
		return;

	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		int inflate_status;
		QByteArray inputdata = reply->readAll();

		m_manifest_zstream.next_in = (Bytef *)inputdata.data();
		m_manifest_zstream.avail_in = inputdata.count();
		do
		{
			inflate_status = inflate(&m_manifest_zstream, Z_SYNC_FLUSH);
			if(m_manifest_zstream.avail_out == 0)
			{
				int oldsize = m_manifest_bytearray.count();
				m_manifest_bytearray.resize(oldsize + m_buffer_increment);
				m_manifest_zstream.next_out = (Bytef *)m_manifest_bytearray.data() + oldsize;
				m_manifest_zstream.avail_out = m_buffer_increment;
			}
		}
		while(inflate_status == Z_OK && m_manifest_zstream.avail_in > 0);

		if(inflate_status < 0)
		{
			m_error_occured = true;
			reply->abort();
			emit state("Done");
			info.critical(tr("I/O error"), tr("Decompress error") + QString(" (1) (%1)").arg(inflate_status));
			return;
		}
	}
}

void Patcher::manifestFinished()
{
	if(m_error_occured)
		return;

	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(!reply)
		return;

	if(reply->error() != QNetworkReply::NoError)
	{
		m_error_occured = true;
		emit state("Done");
		info.critical(QString("Communication error"), "Error while downloading manifest.\nDetails: " + reply->errorString() + ".");
	}
	else
	{
		emit progress(100);

		int inflate_status = inflate(&m_manifest_zstream, Z_SYNC_FLUSH);
		m_manifest_bytearray.truncate(m_manifest_bytearray.count() - m_manifest_zstream.avail_out);
		inflateEnd(&m_manifest_zstream);

		if(inflate_status != Z_STREAM_END)
		{
			m_error_occured = true;
			emit state("Done");
			info.critical(tr("I/O error"), tr("Decompress error") + " (2)");
		}
		else
			verify();
	}
	reply->deleteLater();
}

void Patcher::manifestDownloadProgress(qint64 value, qint64 total)
{
	emit progress(100. * value / total);
}

void Patcher::verify()
{
	bool ok;
	QJson::Parser parser;
	m_manifest = parser.parse(m_manifest_bytearray, &ok).toMap();

	m_manifest_bytearray.resize(0);
	if(ok)
		processManifest(m_manifest);
}

void Patcher::processManifest(QVariantMap manifest)
{
	emit state("Verifying installation");

	QVariantList bundles = manifest["bundles"].toList();
	m_num_current_verified_items = 0;
	m_num_total_items = bundles.count();
	m_total_bundle_download_size = 0;
	m_current_bundle_downloaded = 0;
	m_num_current_downloaded_items = 0;
	m_num_total_download_items = 0;
	for(QVariantList::const_iterator bundlevariant = bundles.constBegin(); bundlevariant != bundles.constEnd(); ++bundlevariant)
	{
		Bundle *bundle = new Bundle(m_install_path, *bundlevariant, m_current_stream.DownloadUrl, m_current_stream.TitleFolder, m_current_stream.AuthSuffix, m_access_manager, m_parent);
		connect(bundle, SIGNAL(verifyDone(size_t)), SLOT(bundleVerifyDone(size_t)));
		connect(bundle, SIGNAL(downloadProgress(qint64)), SLOT(bundleDownloadProgress(qint64)));
		connect(bundle, SIGNAL(done()), SLOT(bundleDownloadDone()));
		m_bundles.push_back(bundle);
		bundle->verifyAndMaybeDownload();
	}
}

void Patcher::bundleVerifyDone(size_t size)
{
	m_num_current_verified_items++;
	m_total_bundle_download_size += size;
	emit progress(100. * m_num_current_verified_items / m_num_total_items);
	
	if(m_num_current_verified_items == (size_t)m_bundles.count())
	{
		m_num_total_download_items = 0;
		for(QList<Bundle *>::const_iterator bundle = m_bundles.constBegin(); bundle != m_bundles.constEnd(); ++bundle)
		{
			if((*bundle)->state() == Bundle::bad)
			{
				m_num_total_download_items++;
			}
		}

		if(m_num_total_download_items == 0 || m_num_current_downloaded_items == m_num_total_download_items)
		{
			while(!m_bundles.isEmpty())
			{
				delete m_bundles[0];
				m_bundles.removeFirst();
			}
			emit state("Done");
		}
		else
			emit state("Downloading and installing bundles");
	}
}

void Patcher::bundleDownloadDone()
{
	m_num_current_downloaded_items++;
	if(m_num_current_downloaded_items == m_num_total_download_items)
	{
		while(!m_bundles.isEmpty())
		{
			delete m_bundles[0];
			m_bundles.removeFirst();
		}
		emit state("Done");
	}
}

void Patcher::setInstallPath(QString installpath)
{
	QDir installdir(installpath);
	if(!installdir.mkpath("."))
	{
		info.critical("Directory creation", QString("Failed to create directory at \"%1\".)").arg(installpath));
		return;
	}

	m_install_path = installdir.absolutePath() + '/';
}

void Patcher::verifyProgressValueChanged(int value)
{
	emit progress(100. * value / m_num_total_items);
}

void Patcher::bundleDownloadProgress(qint64 value)
{
	m_current_bundle_downloaded += value;
	if(m_num_current_verified_items == (size_t)m_bundles.count())
	{
		emit progress(100. * m_current_bundle_downloaded / m_total_bundle_download_size);
	}
}

#include "patcher.moc"
