#ifndef PATCHER_H
#define PATCHER_H

#include <QObject>
#include <QJsonDocument>
#include <QMutex>

class QNetworkAccessManager;

class Patcher : public QObject
{
    Q_OBJECT

public:
	Patcher(QNetworkAccessManager *network_access_manager, QObject * parent = 0);
	~Patcher();

	void setInstallPath(QString install_path) {mInstallPath = install_path;}
    void setDownloadUrl(QString url) {mDownloadUrl = url;}
	void giveJsonData(QByteArray data);
	QByteArray getFile(QString filename);


private:
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

	void startVerifying();

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
	
public slots:
    void parseManifest();
};

#endif // PATCHER_H
