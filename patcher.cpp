#include "patcher.h"
#include "bundle.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QApplication>
#if !defined(_WIN32)
#  include <unistd.h>
#  include <cstring>
#  include <cerrno>
#endif

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

void Patcher::bundleError(QString error_string)
{
	emit stateChange("Error occurred");
	emit error(error_string);
}

void Patcher::bundleFinished()
{
	Bundle *bundle = dynamic_cast<Bundle *>(sender());
	if(bundle)
	{
		for(QMap<QString,QString>::const_iterator link = bundle->symLinks().constBegin(); link != bundle->symLinks().constEnd(); ++link)
		{
			mSymLinkLater.insert(link.key(), link.value());
		}
	}

	mBundlesFinished++;
	if(mBundlesFinished == mNumBundles)
	{
		processSymLinks();

		emit stateChange("Done");
		emit done();
	}
}

void Patcher::processSymLinks()
{
	bool something_changed;

	while(mSymLinkLater.size() > 0)
	{
		something_changed = false;
		for(QMap<QString,QString>::const_iterator slink = mSymLinkLater.constBegin(); slink != mSymLinkLater.constEnd(); ++slink)
		{
			QFile source_file(slink.value());
			if(source_file.exists())
			{
				QFile target_file(slink.key());
				if(target_file.exists())
					target_file.remove();

#ifdef _WIN32
				if(!source_file.copy(slink.key()))
				{
					emit error(tr("Error copying duplicate file \"%1\" to \"%2\".\n%3").arg(slink.value()).arg(slink.value()).arg(source_file.errorString()));
					return;
				}
#else
				// For proper symlinks, we need to find the relative path.
				QStringList target_path = slink.key().split(QRegExp("[\\\\/]"));
				QStringList source_path = slink.value().split(QRegExp("[\\\\/]"));
				int i;
				for(i = 0; i < target_path.count() - 1; i++)
				{
					if(source_path[i] == target_path[i])
					{
						source_path[i] = "";
						target_path[i] = "";
					}
					else
						break;
				}
				for(; i < target_path.count() - 1; i++)
				{
					source_path.push_front("..");
				}

				for(int i = source_path.count() - 1; i >= 0; i--)
				{
					if(source_path[i].isEmpty())
						source_path.removeAt(i);
				}

				int res = symlink(source_path.join("/").toStdString().c_str(), slink.key().toStdString().c_str());
				if(res != 0)
				{
					int error_code = errno;
					emit error(tr("Error creating symlink from \"%1\" to \"%2\".\n%3").arg(source_file.fileName()).arg(source_path.join('/')).arg(strerror(error_code)));
					return;
				}
#endif

				mSymLinkLater.remove(slink.key());
				something_changed = true;
				break;
			}
		}

		if(!something_changed)
		{
			emit error(tr("Error copying duplicate file. Source does not exist."));
			return;
		}
	}
}


#include "patcher.moc"
