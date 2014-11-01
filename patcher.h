#ifndef PATCHER_H
#define PATCHER_H

#include <QObject>
#include <QFile>
#include <QVariantMap>
#include <QList>
#include <zlib.h>
#include <QUrl>
#include "bundle.h"

class QStringList;
class QNetworkAccessManager;
class QWidget;

class Patcher : public QObject
{
	Q_OBJECT

public:
	Patcher(QWidget* parent = 0);
	~Patcher();

	struct Stream
	{
		QString StreamName;
		QString BuildId;
		QString Description;
		QString DownloadUrl;
		QString AuthSuffix;
		QString ManifestName;
		QString TitleFolder;
	};

	void decodeStreamsData(QByteArray data);
	QStringList streamNames();
	QString buildId(QString streamname);
	void downloadStream(const QString streamname);
	void setInstallPath(QString installpath);

private:
	QWidget *m_parent;
	QList<Stream> m_streams;
	z_stream m_manifest_zstream;
	const int m_buffer_increment;
	QByteArray m_manifest_bytearray;
	QNetworkAccessManager *m_access_manager;
	QVariantMap m_manifest;
	QString m_install_path;
	int m_num_total_items;
	size_t m_num_current_verified_items;
	Stream m_current_stream;
	QList<Bundle *> m_bundles;
	size_t m_num_current_downloaded_items;
	size_t m_num_total_download_items;
	size_t m_current_bundle_downloaded;
	size_t m_total_bundle_download_size;
	bool m_error_occured;
	int m_inflate_status;

	void downloadManifest(const Patcher::Stream& stream);
	void processManifest(QVariantMap manifest);
	void verify();

private slots:
	void manifestFinished();
	void manifestDownloadProgress(qint64 value, qint64 total);
	void manifestReadyRead();
	void verifyProgressValueChanged(int value);
	void bundleVerifyDone(size_t size);
	void bundleDownloadProgress(qint64 value);
	void bundleDownloadDone();

signals:
	void progress(int);
	void state(QString);
};

#endif // PATCHER_H
