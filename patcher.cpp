#include "patcher.h"
#include "bundle.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QApplication>

bool error_occurred = false;

Patcher::Patcher(QObject* parent)
 : QObject(parent), mRawJsonData(NULL)
{
}

Patcher::~Patcher()
{
	if(mRawJsonData)
		delete mRawJsonData;
	
	if(mNetworkAccessManager)
		delete mNetworkAccessManager;
}


void Patcher::giveJsonData(QByteArray data)
{
	// Force a deep copy for thread safety.
	mRawJsonData = new QByteArray(data.constData(), data.size());
}

void Patcher::parseManifest()
{
	mNetworkAccessManager = new QNetworkAccessManager(this);

	QJsonParseError parseerror;
	mDocument = QJsonDocument::fromJson(*mRawJsonData, &parseerror);
	delete mRawJsonData;
	mRawJsonData = NULL;

	if(parseerror.error == QJsonParseError::NoError)
	{
		startVerifying();
	}
	else
		emit error(parseerror.errorString());
}

void Patcher::startVerifying()
{
	emit stateChange("Verifying files");

	mBytesToDownload = 0;
	mBytesDownloaded = 0;
	mBundlesVerified = 0;
	mBundlesFinished = 0;
	if(mDocument.isObject())
	{
		QJsonArray bundles = mDocument.object()["bundles"].toArray();
		mNumBundles = bundles.count();
		for(QJsonArray::const_iterator b = bundles.constBegin(); b != bundles.constEnd(); ++b)
		{
			QJsonObject bundle_json_object = (*b).toObject();
			Bundle *bundle = new Bundle(mInstallPath, this);
			connect(bundle, SIGNAL(downloadMe()), SLOT(bundleDownloadMe()));
			connect(bundle, SIGNAL(downloadProgress(qint64)), SLOT(bundleDownloadProgress(qint64)));
			connect(bundle, SIGNAL(verifyDone()), SLOT(bundleVerifyDone()));
			connect(bundle, SIGNAL(finished()), SLOT(bundleFinished()));
			connect(bundle, SIGNAL(error(QString)), SLOT(bundleError(QString)));
			bundle->verify(&bundle_json_object);
		}
	}
}


// To avoid trashing the disk, just allow read access to one file at a time.
QByteArray Patcher::getFile(QString filename)
{
	mHardDriveAccessMutex.lock();

	QByteArray data;
	QFile file(filename);
	if(file.open(QFile::ReadOnly))
	{
		data = file.readAll();
		file.close();
	}

	mHardDriveAccessMutex.unlock();

	return data;
}

void Patcher::bundleDownloadMe()
{
	Bundle *bundle = dynamic_cast<Bundle *>(sender());
	if(bundle)
	{
		mBytesToDownload += bundle->totalSize();
		bundle->downloadAndExtract(mNetworkAccessManager, mDownloadUrl);
	}
}

void Patcher::bundleDownloadProgress(qint64 downloaded)
{
	mBytesDownloaded += downloaded;

	if(mBundlesVerified == mNumBundles)
		emit progress(100. * mBytesDownloaded / mBytesToDownload);
}

void Patcher::bundleVerifyDone()
{
	mBundlesVerified++;
	emit progress(100. * mBundlesVerified / mNumBundles);
	
	if(mBundlesVerified == mNumBundles)
	{
		emit stateChange("Downloading and extracting");
	}
}

void Patcher::bundleFinished()
{
	mBundlesFinished++;
	if(mBundlesFinished == mNumBundles)
	{
		emit stateChange("Done");
		emit done();
	}
}

void Patcher::bundleError(QString error_string)
{
	emit stateChange("Error occurred");
	emit error(error_string);
}


#include "patcher.moc"
