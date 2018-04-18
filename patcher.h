#ifndef PATCHER_H
#define PATCHER_H

#include <QObject>
#include <QJsonDocument>
#include <QMutex>
#include <QMap>

class QNetworkAccessManager;

class Patcher : public QObject
{
	Q_OBJECT

public:
	Patcher(QObject* parent = 0);
	~Patcher();

	void setInstallPath(QString install_path) {mInstallPath = install_path;}
	void setDownloadUrl(QString url) {mDownloadUrl = url;}
	void giveJsonData(QByteArray data);
	QByteArray getFile(QString filename);


private:
	struct file_entry_t
	{
		int i;
		QString checksum;
		QString filename;
		ulong size;
		ulong sizeZ;
		bool operator<(file_entry_t const& b) const { return checksum < b.checksum; }
	};

	QJsonDocument mDocument;
	QString mInstallPath;
	QString mDownloadUrl;
	QByteArray *mRawJsonData;
	QMutex mHardDriveAccessMutex;
	QNetworkAccessManager *mNetworkAccessManager;
	qint64 mBytesDownloaded;
	qint64 mBytesToDownload;
	int mNumBundles;
	int mBundlesVerified;
	int mBundlesFinished;
	QMap<QString,QString> mSymLinkLater;

	void startVerifying();
	void processSymLinks();

signals:
	void error(QString);
	void progress(int);
	void done();
	void stateChange(QString state);

private slots:
	void bundleDownloadMe();
	void bundleDownloadProgress(qint64);
	void bundleVerifyDone();
	void bundleFinished();
	void bundleError(QString error);
	void findSymlinks();

public slots:
	void parseManifest();
};

#endif // PATCHER_H
