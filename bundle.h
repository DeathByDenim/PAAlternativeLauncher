#ifndef BUNDLE_H
#define BUNDLE_H

#include <QObject>
#include <QFuture>
#ifdef _WIN32
#	include <QMap>
#endif
#include <zlib.h>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class QJsonObject;
class Patcher;

extern bool error_occurred;

class Bundle : public QObject
{
    Q_OBJECT

public:
    Bundle(QString install_path, Patcher* patcher);
    ~Bundle();

	void verify(QJsonObject* array);
	QString checksum() {return mChecksum;}
	qint64 totalSize() {return mTotalSize;}
	void downloadAndExtract(QNetworkAccessManager* network_access_manager, QString download_url);
	const QMap<QString,QString> & symLinks() {return mSymLinkLater;}

private:
	struct File
	{
		QString filename;
		QString checksum;
		ulong offset;
		ulong size;
		ulong sizeZ;
		bool executable;
		bool verified;
		QFuture<bool> future;
	};

	ulong mTotalSize;
	QList<File> mFiles;
	int mNumVerified;
	int mNumToVerify;
	QString mInstallPath;
	Patcher *mPatcher;
	bool mNeedsDownloading;
	QString mChecksum;
	z_stream mZstream;
	const qint64 mBufferSize;
	Bytef * mBuffer;
	qint64 mBytesDownloaded;
	qint64 mBytesProgress;
	QFile *mCurrentFile;
	bool mCurrentFileIsGzipped;
	int mFilesCurrentIndex;
	QMap<QString,QString> mSymLinkLater;

	static bool verifySHA1(Bundle::File file_entry, bool* downloading, Patcher* patcher, QString install_path);
	void prepareZLib();
    void prepareFile();
    void nextFile();
    void processData(QNetworkReply* reply, qint64 bytes_available);
	bool createSymbolicLink(const QString& from, const QString& to);
	bool createEmptyFile(const QString& file_name);

private slots:
	void verifyFinished();
	void downloadFinished();
    void downloadReadyRead();
    void downloadProgress(qint64,qint64);

signals:
	void downloadMe();
	void downloadProgress(qint64);
	void verifyDone();
	void finished();
	void error(QString error);
};

#endif // BUNDLE_H
