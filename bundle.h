#ifndef BUNDLE_H
#define BUNDLE_H

#include <QObject>
#include <QUrl>
#include <QList>
#include <QVariant>
#include <QFile>
#include <QStringList>
#include <zlib.h>

class QWidget;
class QNetworkAccessManager;
class QNetworkReply;

class Bundle : public QObject
{
	Q_OBJECT

public:
	Bundle(QString installpath, QVariant bundlevariant, QString downloadurl, QString titlefolder, QString authsuffix, QNetworkAccessManager* networkaccessmanager, QWidget* parent);
	~Bundle();
	
	enum verification_state_t
	{
		unknown,
		good,
		bad
	};

	void verifyAndMaybeDownload();
	int numEntries() {return m_entries.count();}
	verification_state_t state() {return m_verification_state;}
	size_t size() {return m_size;}

private:
	struct entry_t
	{
		size_t sizeZ;
		size_t offset;
		size_t size;
		QStringList fullfilename;
		QString checksumZ;
		QString checksum;
		bool executable;

		int next_offset;
	};

	QWidget *m_parent;
	QString m_checksum;
	size_t m_size;
	QUrl m_url;
	QList<entry_t> m_entries;
	z_stream m_gzipstream;
	QNetworkAccessManager *m_network_access_manager;
	verification_state_t m_verification_state;
	QList<QFile *> m_entry_file;
	int m_current_entry_index;
    int m_alreadyread;
	qint64 m_alreadydownloaded;
	bool m_error_occured;

	QFile m_cache_file;

	static bool verifyEntry(entry_t entry);
	void nextFile(QNetworkReply *reply);

private slots:
	void verifyFinished();
	void downloadFinished();
	void downloadProgress(qint64 value, qint64);
	void readyRead();
    void download();

signals:
	void verifyDone(size_t bytes_to_download);
	void downloadProgress(qint64);
	void errorOccurred();
	void done();
};

#endif // BUNDLE_H
