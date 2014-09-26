#ifndef PATCHER_H
#define PATCHER_H

#include <QObject>
#include <QFile>
#include <QVariantMap>

class QStringList;
class QNetworkAccessManager;
class QNetworkReply;
class QWidget;

class Patcher : public QObject
{
	Q_OBJECT

public:
	Patcher(QWidget *parent = 0);
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
	void downloadStream(const QString streamname);
	void setInstallPath(QString installpath);
	void test();

private:
	QList<Stream> m_streams;
	QNetworkAccessManager *m_access_manager;
	QFile m_manifest_file;
	QVariantMap m_manifest;
	static QString m_install_path;
	QWidget *m_parent;
	int m_num_items;

	void downloadStream(const Patcher::Stream& stream);
	void processManifest(QVariantMap manifest);
	static bool verify(const QVariant &entry);
	static QString calculateSHA1(QString filename);

private slots:
	void replyFinished(QNetworkReply *reply);
	void manifestDownloadProgress(qint64, qint64);
	void manifestReadyRead();
	void verifyProgressValueChanged(int value);
	void verifyFinished();
	
signals:
	void progress(int);
};

#endif // PATCHER_H
