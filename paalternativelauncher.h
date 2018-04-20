#ifndef PAAlternativeLauncher_H
#define PAAlternativeLauncher_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <zlib.h>
#include <QBuffer>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QComboBox>
#include <QTextBrowser>
#include <QPushButton>
#include <QMap>
#include <QThread>
#include "advanceddialog.h"

class Patcher;
class ModDatabaseFrame;

class PAAlternativeLauncher : public QMainWindow
{
	Q_OBJECT

public:
	PAAlternativeLauncher();
	virtual ~PAAlternativeLauncher();

private:
	enum EState
	{
		login_state,
		wait_state,
		download_state
	};

	QNetworkAccessManager *mNetworkAccessManager;
	const QString mPlatform;
	const QString mDefaultInstallPath;
	QString mSessionTicket;
	z_stream mZstream;
	const qint64 mBufferSize;
	Bytef * mBuffer;
	QBuffer mManifestJson;
	QWidget * mLoginWidget;
	QWidget * mDownloadWidget;
	QWidget * mWaitWidget;
	QLabel * mPatchLabel;
	QProgressBar * mPatchProgressbar;
	QLineEdit * mUserNameLineEdit;
	QLineEdit * mPasswordLineEdit;
	QComboBox * mStreamsComboBox;
	QLabel * mUpdateAvailableLabel;
	QTextBrowser * mPatchTextBrowser;
	QLineEdit * mInstallPathLineEdit;
	QPushButton * mDownloadButton;
	QMap<QString, QString> mStreamNews;
	Patcher *mPatcher;
	QString mExtraParameters;
	AdvancedDialog::optimus_t mUseOptimus;
	bool mUseSteamRuntime;
	QThread mPatcherThread;
//	ModDatabaseFrame *mModDatabaseFrame;
    QPushButton* mLaunchButton;

	void prepareZLib();
	QWidget * createLoginWidget(QWidget * parent);
	QWidget * createDownloadWidget(QWidget * parent);
	QWidget * createWaitWidget(QWidget * parent);
	void setState(EState state);
	QString currentInstalledVersion();
	quint64 getFreeDiskspaceInMB(QString directory);
	void launchPA(bool offline);
	void convertOldSettings();

protected:
	void closeEvent(QCloseEvent *event);

public:
	void resetTaskbarProgressBar();

private slots:
	void loginPushButtonClicked(bool);
	void authenticateFinished();
	void streamsFinished();
	void manifestFinished();
	void manifestReadyRead();
	void passwordLineEditReturnPressed();
	void streamsComboBoxCurrentIndexChanged(int);
	void streamNewsReplyFinished();
	void installPathButtonClicked(bool);
	void downloadPushButtonClicked(bool);
	void launchOfflinePushButtonClicked(bool);
	void patcherError(QString error);
	void patcherDone();
	void patcherProgress(int percentage);
	void advancedPushButtonClicked(bool);
	void launchPushButtonClicked(bool);
	void manifestDownloadProgress(qint64,qint64);
	void patcherStateChange(QString state);
};

#endif // PAAlternativeLauncher_H
