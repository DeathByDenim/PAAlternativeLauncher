#include "paalternativelauncher.h"

#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPalette>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QComboBox>
#include <QTextBrowser>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMovie>
#include <QSettings>
#include <QFileDialog>
#include <QProcess>
#if defined(linux) || defined(__APPLE__)
#  include <sys/statvfs.h>
#  include <sys/errno.h>
#elif defined(_WIN32)
#  include <QProcessEnvironment>
#  include <windows.h>
#endif
#include <QFile>
#include <zlib.h>
#include "information.h"
#include "version.h"
#include "patcher.h"
#include "advanceddialog.h"
#include "moddatabaseframe.h"


PAAlternativeLauncher::PAAlternativeLauncher()
 : mNetworkAccessManager(new QNetworkAccessManager(this))
#if defined(linux)
 , mPlatform("Linux")
 , mDefaultInstallPath(QDir::homePath() + "/Games/PA")
#elif defined(_WIN32)
 , mPlatform("Windows")
 , mDefaultInstallPath("C:\\Games\\Uber Entertainment\\Planetary Annihilation Launcher\\Planetary Annihilation")
#elif defined(__APPLE__)
 , mPlatform("OSX")
 , mDefaultInstallPath("/Applications/Planetary Annihilation")
#else
# error Not a supported os
#endif
 , mBufferSize(1024*1024), mBuffer(NULL), mPatcher(NULL)
{
	setWindowIcon(QIcon(":/img/icon.png"));
	setWindowTitle("PA Alternative Launcher");
	info.setParent(this);
	info.log("Starting launcher", QDateTime::currentDateTime().toString());

	if(mNetworkAccessManager == NULL)
		info.critical("Init", "Network Access Manager failed to initialize.");

	QPalette palette;
	QPixmap backgroundPixmap(":/img/img_bground_galaxy_01.png");
	QBrush backgroundBrush(backgroundPixmap);
 	palette.setBrush(QPalette::Background, backgroundBrush);
 	setPalette(palette);

	QWidget *main_widget = new QWidget(this);
	QVBoxLayout *main_layout = new QVBoxLayout(main_widget);
	
	QLabel *disclaimer_label = new QLabel(tr("This is an UNOFFICIAL launcher and not connected to Uber in any way."), main_widget);
	disclaimer_label->setStyleSheet("QLabel {color: red; font-weight: bold;}");
	disclaimer_label->setAlignment(Qt::AlignCenter);
	disclaimer_label->setAutoFillBackground(true);
	disclaimer_label->setBackgroundRole(QPalette::Dark);
	main_layout->addWidget(disclaimer_label);

	QLabel *header_label = new QLabel(main_widget);
	header_label->setPixmap(QPixmap(":/img/img_pa_logo_start_rest.png"));
	header_label->setAlignment(Qt::AlignCenter);
	main_layout->addWidget(header_label);

	QWidget *centre_widget = new QWidget(main_widget);
	QHBoxLayout *centre_layout = new QHBoxLayout(centre_widget);

	QWidget *commander_widget = new QWidget(centre_widget);
	QVBoxLayout *commander_layout = new QVBoxLayout(commander_widget);
	QLabel *commander_label = new QLabel(commander_widget);
	commander_label->setPixmap(QPixmap(":img/img_imperial_invictus.png"));
	commander_layout->addWidget(commander_label);

	centre_layout->addWidget(commander_widget);

	QWidget *middle_horizontal_widget = new QWidget(centre_widget);
	QVBoxLayout *middle_horizontal_layout = new QVBoxLayout(middle_horizontal_widget);

	QWidget *stream_widget = new QWidget(middle_horizontal_widget);
	QHBoxLayout *stream_layout = new QHBoxLayout(stream_widget);
	QLabel *streams_label = new QLabel(tr("Stream"), stream_widget);
	QPalette palettewhite = main_widget->palette();
	palettewhite.setColor(QPalette::WindowText, Qt::white);
	streams_label->setPalette(palettewhite);
	stream_layout->addWidget(streams_label);
	mStreamsComboBox = new QComboBox(stream_widget);
	stream_layout->addWidget(mStreamsComboBox);
	connect(mStreamsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(streamsComboBoxCurrentIndexChanged(int)));
	mUpdateAvailableLabel = new QLabel("", stream_widget);
	mUpdateAvailableLabel->setStyleSheet("QLabel {font-weight: bold; color: white}");
	mUpdateAvailableLabel->setPalette(palettewhite);
	stream_layout->addWidget(mUpdateAvailableLabel);
	stream_layout->addStretch();
	middle_horizontal_layout->addWidget(stream_widget);

	mLoginWidget = createLoginWidget(middle_horizontal_widget);
	mDownloadWidget = createDownloadWidget(middle_horizontal_widget);
	mDownloadWidget->setVisible(false);
	mWaitWidget = createWaitWidget(middle_horizontal_widget);
	mWaitWidget->setVisible(false);
	middle_horizontal_layout->addWidget(mLoginWidget);
	middle_horizontal_layout->addWidget(mDownloadWidget);
	middle_horizontal_layout->addWidget(mWaitWidget);

	QWidget *install_path_widget = new QWidget(middle_horizontal_widget);
	QHBoxLayout *install_path_layout = new QHBoxLayout(install_path_widget);

	mInstallPathLineEdit = new QLineEdit(install_path_widget);
	mInstallPathLineEdit->setPlaceholderText(tr("Enter the PA install directory here."));
	install_path_layout->addWidget(mInstallPathLineEdit);

	QPushButton *install_path_push_button = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), "", install_path_widget);
	install_path_push_button->setFlat(true);
	connect(install_path_push_button, SIGNAL(clicked(bool)), SLOT(installPathButtonClicked(bool)));
	install_path_layout->addWidget(install_path_push_button);
	middle_horizontal_layout->addWidget(install_path_widget);

	centre_layout->addWidget(middle_horizontal_widget);
/*
	mModDatabaseFrame = new ModDatabaseFrame(centre_widget);
	centre_layout->addWidget(mModDatabaseFrame);
*/
	main_layout->addWidget(centre_widget);

	QDialogButtonBox *button_box = new QDialogButtonBox(main_widget);
	mLaunchButton = new QPushButton(tr("&Launch PA"), button_box);
	mLaunchButton->setEnabled(false);
	connect(mLaunchButton, SIGNAL(clicked(bool)), SLOT(launchPushButtonClicked(bool)));
	button_box->addButton(mLaunchButton, QDialogButtonBox::AcceptRole);
	QPushButton *launch_offline_button = new QPushButton(tr("Launch &offline"), button_box);
	connect(launch_offline_button, SIGNAL(clicked(bool)), SLOT(launchOfflinePushButtonClicked(bool)));
	button_box->addButton(launch_offline_button, QDialogButtonBox::AcceptRole);
	QPushButton *advanced_button = new QPushButton(tr("&Advanced"), button_box);
	connect(advanced_button, SIGNAL(clicked(bool)), SLOT(advancedPushButtonClicked(bool)));
	button_box->addButton(advanced_button, QDialogButtonBox::NoRole);
	mDownloadButton = new QPushButton(tr("&Download and install"), button_box);
	mDownloadButton->setEnabled(false);
	connect(mDownloadButton, SIGNAL(clicked(bool)), SLOT(downloadPushButtonClicked(bool)));
	button_box->addButton(mDownloadButton, QDialogButtonBox::NoRole);

	main_layout->addWidget(button_box);

	mPatchLabel = new QLabel(main_widget);
	mPatchLabel->setStyleSheet("QLabel {color: white; font-weight: bold}");
	mPatchLabel->setAlignment(Qt::AlignCenter);
//	connect(&m_patcher, SIGNAL(state(QString)), SLOT(patcherState(QString)));
	main_layout->addWidget(mPatchLabel);

	mPatchProgressbar = new QProgressBar(main_widget);
	mPatchProgressbar->setMinimum(0);
	mPatchProgressbar->setMaximum(100);
//	connect(&m_patcher, SIGNAL(progress(int)), SLOT(patcherProgress(int)));
	main_layout->addWidget(mPatchProgressbar);

	QLabel *disclaimer_label2 = new QLabel(tr("This is an UNOFFICIAL launcher and not connected to Uber in any way."), main_widget);
	disclaimer_label2->setStyleSheet("QLabel {color: red; font-weight: bold;}");
	disclaimer_label2->setAlignment(Qt::AlignCenter);
	disclaimer_label2->setAutoFillBackground(true);
	disclaimer_label2->setBackgroundRole(QPalette::Dark);
	main_layout->addWidget(disclaimer_label2);

	QWidget *about_widget = new QWidget(main_widget);
	QHBoxLayout *about_layout = new QHBoxLayout(about_widget);

	QLabel *created_by_label = new QLabel(tr("Created by") + " DeathByDenim", about_widget);
	created_by_label->setStyleSheet("QLabel {color: white}");
	about_layout->addWidget(created_by_label);
	QLabel *blackmage_label = new QLabel(about_widget);
	blackmage_label->setPixmap(QPixmap(":img/blackmage.png"));
	blackmage_label->setMaximumSize(16, 16);
	about_layout->addWidget(blackmage_label);
	about_layout->addStretch();
	QLabel *version_label = new QLabel(tr("Version") + " " VERSION, about_widget);
	version_label->setStyleSheet("QLabel {color: white}");
	about_layout->addWidget(version_label);

	main_layout->addWidget(about_widget);

	setCentralWidget(main_widget);


	QSettings settings;
	restoreGeometry(settings.value("mainwindow/geometry").toByteArray());

	QStringList groups = settings.childGroups();
	for(QStringList::const_iterator stream = groups.constBegin(); stream != groups.constEnd(); ++stream)
	{
		if(*stream != "login" && *stream != "mainwindow")
		{
			mStreamsComboBox->addItem(*stream);
		}
	}
	mStreamsComboBox->setCurrentText("stable");

	mSessionTicket = settings.value("login/sessionticket").toString();
	if(!mSessionTicket.isEmpty())
	{
		setState(wait_state);

		QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + mPlatform));
		request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
		request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
		request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());
		QNetworkReply *reply = mNetworkAccessManager->get(request);
		connect(reply, SIGNAL(finished()), SLOT(streamsFinished()));
	}

	if(mUserNameLineEdit->text().isEmpty())
		mUserNameLineEdit->setFocus();
	else
		mPasswordLineEdit->setFocus();
}

PAAlternativeLauncher::~PAAlternativeLauncher()
{
	info.log("Stopping launcher", QDateTime::currentDateTime().toString());

	mPatcherThread.exit();
	mPatcherThread.wait(1000);

	if(mBuffer)
		delete[] mBuffer;
}


/* Create the various widgets that can be on the right side. These are the login window, where
 * the users enter their username/password, the logging in widget that shows the spinning thingy,
 * and the news screen if the login was successful.
 */

QWidget* PAAlternativeLauncher::createLoginWidget(QWidget *parent)
{
	QWidget *main_widget = new QWidget(parent);
	QVBoxLayout *main_layout = new QVBoxLayout(main_widget);

	main_layout->addStretch();

	QWidget *form_widget = new QWidget(main_widget);
	QPalette palette = form_widget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	form_widget->setPalette(palette);
	QFormLayout *form_layout = new QFormLayout(form_widget);
	form_widget->setLayout(form_layout);

	QLabel *login_label = new QLabel(tr("LOGIN"), form_widget);
	QFont font = login_label->font();
	font.setBold(true);
	font.setPointSizeF(3*font.pointSizeF());
	login_label->setFont(font);
	login_label->setAlignment(Qt::AlignCenter);
    login_label->setPalette(palette);
	form_layout->addRow(login_label);

	mUserNameLineEdit = new QLineEdit(form_widget);
    QLabel *user_name_label = new QLabel(tr("Uber ID"), form_widget);
    user_name_label->setPalette(palette);
    form_layout->addRow(user_name_label, mUserNameLineEdit);

	mPasswordLineEdit = new QLineEdit(form_widget);
	mPasswordLineEdit->setEchoMode(QLineEdit::Password);
    QLabel *password_label = new QLabel(tr("Password"), form_widget);
    password_label->setPalette(palette);
	connect(mPasswordLineEdit, SIGNAL(returnPressed()), SLOT(passwordLineEditReturnPressed()));
    form_layout->addRow(password_label, mPasswordLineEdit);

	QPushButton *login_button = new QPushButton(tr("Login"), form_widget);
	form_layout->addRow(login_button);
	connect(login_button, SIGNAL(clicked(bool)), SLOT(loginPushButtonClicked(bool)));

	main_layout->addWidget(form_widget);
	
	main_layout->addStretch();

	QSettings settings;
	mUserNameLineEdit->setText(settings.value("login/username").toString());

	return main_widget;
}

QWidget* PAAlternativeLauncher::createDownloadWidget(QWidget* parent)
{
	QWidget *main_widget = new QWidget(parent);
	QPalette palette = main_widget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	main_widget->setPalette(palette);
	QVBoxLayout *main_layout = new QVBoxLayout(main_widget);
	main_widget->setLayout(main_layout);

	mPatchTextBrowser = new QTextBrowser(main_widget);
	main_layout->addWidget(mPatchTextBrowser);

	return main_widget;
}

QWidget* PAAlternativeLauncher::createWaitWidget(QWidget* parent)
{
	QWidget *main_widget = new QWidget(parent);
	QPalette palette = main_widget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	main_widget->setPalette(palette);
	QVBoxLayout *main_layout = new QVBoxLayout(main_widget);
	main_widget->setLayout(main_layout);
	
	main_layout->addStretch();

	QLabel *logging_in_label = new QLabel(tr("Logging in..."), main_widget);
	QFont font = logging_in_label->font();
	font.setBold(true);
	font.setPointSizeF(3*font.pointSizeF());
	logging_in_label->setFont(font);
	logging_in_label->setAlignment(Qt::AlignCenter);
	logging_in_label->setPalette(palette);
	main_layout->addWidget(logging_in_label);

	main_layout->addSpacing(30);

	QMovie *loading_movie = new QMovie(":/img/loading.gif", "gif", this);
	QLabel *loading_label = new QLabel(this);
	loading_label->setMovie(loading_movie);
	loading_movie->start();
	loading_label->setAlignment(Qt::AlignCenter);
	loading_label->setPalette(palette);
	main_layout->addWidget(loading_label);

	main_layout->addStretch();

	return main_widget;
}




/* Handlers for interactions with QWidgets.
 */

void PAAlternativeLauncher::passwordLineEditReturnPressed()
{
	loginPushButtonClicked(true);
}

void PAAlternativeLauncher::loginPushButtonClicked(bool)
{
	if(mUserNameLineEdit->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Login"), tr("No user name given."));
		return;
	}

	if(mPasswordLineEdit->text().isEmpty())
	{
		QMessageBox::warning(this, tr("Login"), tr("No password given."));
		return;
	}

	setState(wait_state);

	QNetworkRequest request(QUrl("https://uberent.com/GC/Authenticate"));
	QString data = QString("{\"TitleId\": 4,\"AuthMethod\": \"UberCredentials\",\"UberName\": \"%1\",\"Password\": \"%2\"}").arg(mUserNameLineEdit->text()).arg(mPasswordLineEdit->text());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=utf-8");
	request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
	request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());

	info.log("Button", "Attempting to log in", true);
	QNetworkReply *reply = mNetworkAccessManager->post(request, data.toUtf8());
	connect(reply, SIGNAL(finished()), SLOT(authenticateFinished()));
}

void PAAlternativeLauncher::streamsComboBoxCurrentIndexChanged(int)
{
	QString current_stream = mStreamsComboBox->currentText();
	if(current_stream.isEmpty())
		return;

	mPatchTextBrowser->setHtml(mStreamNews[current_stream]);

	QSettings settings;
	QString install_path = settings.value(current_stream + "/installpath").toString();
	if(install_path.isEmpty())
#ifdef _WIN32
		install_path = mDefaultInstallPath + "\\" + current_stream;;
#else
		install_path = mDefaultInstallPath + "/" + current_stream;;
#endif

	mInstallPathLineEdit->setText(install_path);
	mExtraParameters = settings.value(current_stream + "/extraparameters").toString();
	mUseOptimus = (AdvancedDialog::optimus_t)settings.value(current_stream + "/useoptirun").toInt();

	QString uber_version = mStreamsComboBox->currentData().toMap()["BuildId"].toString();
	QString current_version = currentInstalledVersion();

	if(uber_version.isEmpty())
	{
		mUpdateAvailableLabel->setVisible(false);
	}
	else
	{
		if(current_version.isEmpty())
			mUpdateAvailableLabel->setText(tr("Build %2 available").arg(uber_version));
		else
			mUpdateAvailableLabel->setText(tr("Update from %1 to %2 available").arg(current_version).arg(uber_version));

		mUpdateAvailableLabel->setVisible(uber_version != current_version);
	}

//	mModDatabaseFrame->setVisible(!mExtraParameters.contains("--nomods"));
}

void PAAlternativeLauncher::installPathButtonClicked(bool)
{
	QString install_path = QFileDialog::getExistingDirectory(this, tr("Choose installation directory"), mInstallPathLineEdit->text(), QFileDialog::ShowDirsOnly);
    if(!install_path.isEmpty())
        mInstallPathLineEdit->setText(install_path);
}

void PAAlternativeLauncher::downloadPushButtonClicked(bool)
{
	if(mInstallPathLineEdit->text().isEmpty())
	{
		info.critical(tr("Download"), tr("Install path is empty."));
		mInstallPathLineEdit->setFocus();
		return;
	}

	QVariantMap object = mStreamsComboBox->currentData().toMap();
	QString downloadurl = object["DownloadUrl"].toString();
	QString titlefolder = object["TitleFolder"].toString();
	QString manifestname = object["ManifestName"].toString();
	QString authsuffix = object["AuthSuffix"].toString();

	QSettings settings;
	settings.setValue(mStreamsComboBox->currentText() + "/installpath", mInstallPathLineEdit->text());

#ifdef _WIN32
	// Pamm or Pahub on Windows may use this registry setting.
	QSettings win32settings(QSettings::SystemScope, "Uber Entertainment", "Planetary Annihilation");
	win32settings.setValue("InstallDirectory", mInstallPathLineEdit->text());
#endif

	quint64 freespace = getFreeDiskspaceInMB(mInstallPathLineEdit->text());
	if(freespace < 1024)
	{
		if(!info.warning(tr("Available disk space"), tr("Only %1 MB available. Continue anyway?").arg(freespace)))
			return;
	}

	if(!downloadurl.isEmpty() && !titlefolder.isEmpty() && !manifestname.isEmpty() && !authsuffix.isEmpty())
	{
		mDownloadButton->setEnabled(false);
		mPatchProgressbar->setEnabled(true);

		mPatchLabel->setText("Downloading manifest");

		prepareZLib();
		QUrl manifesturl(downloadurl + '/' + titlefolder + '/' + manifestname + authsuffix);
		QNetworkRequest request(manifesturl);
		request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
		request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());
		QNetworkReply *reply = mNetworkAccessManager->get(request);
		connect(reply, SIGNAL(readyRead()), SLOT(manifestReadyRead()));
		connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(manifestDownloadProgress(qint64,qint64)));
		connect(reply, SIGNAL(finished()), SLOT(manifestFinished()));
	}
}

void PAAlternativeLauncher::patcherDone()
{
	mDownloadButton->setEnabled(true);
	mUpdateAvailableLabel->setVisible(false);
	mPatchProgressbar->setEnabled(false);
	mPatcherThread.quit();
	mNetworkAccessManager = new QNetworkAccessManager(this);

	info.log("Patcher", "Done");
}

void PAAlternativeLauncher::patcherError(QString error)
{
	mDownloadButton->setEnabled(true);
	mPatchProgressbar->setEnabled(false);
	mPatcherThread.quit();
	mNetworkAccessManager = new QNetworkAccessManager(this);

	info.critical("Patcher", error);
}

void PAAlternativeLauncher::patcherProgress(int percentage)
{
	mPatchProgressbar->setValue(percentage);
}

void PAAlternativeLauncher::launchOfflinePushButtonClicked(bool)
{
	launchPA(true);
}

void PAAlternativeLauncher::launchPushButtonClicked(bool)
{
	launchPA(false);
}


void PAAlternativeLauncher::advancedPushButtonClicked(bool)
{
	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");

	AdvancedDialog *advanceddialog = new AdvancedDialog(mExtraParameters, mUseOptimus, this);
	if(advanceddialog->exec() == QDialog::Accepted)
	{
		mExtraParameters = advanceddialog->parameters();
		mUseOptimus = advanceddialog->useOptimus();
		settings.setValue(mStreamsComboBox->currentText() + "/extraparameters", mExtraParameters);
		settings.setValue(mStreamsComboBox->currentText() + "/useoptirun", (int)mUseOptimus);
//		mModDatabaseFrame->setVisible(!mExtraParameters.contains("--nomods"));
	}

	delete advanceddialog;
}

void PAAlternativeLauncher::patcherStateChange(QString state)
{
	if(state == "Done")
		mPatchProgressbar->setValue(100);
	else if(state != "Error occurred")
		mPatchProgressbar->setValue(0);

	mPatchLabel->setText(state);
}




/* Handlers for the various network requests pertaining to authenticating and stream info.
 */

void PAAlternativeLauncher::authenticateFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			info.log("Login", "Authentication information received.", true);
			QJsonDocument authjson = QJsonDocument::fromJson(reply->readAll());
			info.log("Login", "Authentication information decoded.", true);
			mSessionTicket = authjson.object()["SessionTicket"].toString();
			info.log("Login", QString("Session ticket is %1.").arg(mSessionTicket), true);
			if(!mSessionTicket.isEmpty())
			{
				QSettings settings;
				settings.setValue("login/sessionticket", mSessionTicket);
				settings.setValue("login/username", mUserNameLineEdit->text());

				QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + mPlatform));
				request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
				request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
				request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());
				info.log("Login", "Retrieving available streams.", true);
				QNetworkReply *reply = mNetworkAccessManager->get(request);
				connect(reply, SIGNAL(finished()), SLOT(streamsFinished()));
			}
			else
			{
				QMessageBox::critical(this, tr("Authenticate"), tr("Empty session ticket"));
				setState(login_state);
			}
		}
		else
		{
			QMessageBox::critical(this, tr("Authenticate"), tr("Error while authenticating.\n%1").arg(reply->errorString()));
			setState(login_state);
		}

		reply->deleteLater();
	}
}

void PAAlternativeLauncher::streamsFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			info.log("Streams", "Streams received.", true);
			setState(download_state);

			mStreamsComboBox->clear();
			info.log("Streams", "Decoding data.", true);
			QJsonDocument authjson = QJsonDocument::fromJson(reply->readAll());
			QJsonArray streams = authjson.object()["Streams"].toArray();
			info.log("Streams", "Processing streams.", true);
			for(QJsonArray::const_iterator stream = streams.constBegin(); stream != streams.constEnd(); ++stream)
			{
				QJsonObject object = (*stream).toObject();
				QString download_url = object["DownloadUrl"].toString();
				QString title_folder = object["TitleFolder"].toString();
				QString manifest_name = object["ManifestName"].toString();
				QString auth_suffix = object["AuthSuffix"].toString();
				QString stream_name = object["StreamName"].toString();

				info.log("Streams", QString("Adding %1.").arg(stream_name), true);
				mStreamsComboBox->addItem(stream_name, object.toVariantMap());

				info.log("Streams", "Retrieving news", true);
				// Get the news
				QNetworkRequest request(QUrl("https://uberent.com/Launcher/StreamNews?StreamName=" + stream_name + "&ticket=" + mSessionTicket));
				request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
				request.setRawHeader("X-Clacks-Overhead", "GNU Terry Pratchett");
				request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());
				request.setAttribute(QNetworkRequest::User, stream_name);
				QNetworkReply *stream_news_reply = mNetworkAccessManager->get(request);
				connect(stream_news_reply, SIGNAL(finished()), SLOT(streamNewsReplyFinished()));
			}
		}
		else
		{
			QMessageBox::critical(this, tr("Streams"), tr("Error while getting streams.\n%1").arg(reply->errorString()));
			setState(login_state);
		}

		reply->deleteLater();
	}
}

void PAAlternativeLauncher::manifestReadyRead()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			qint64 bytes_available = reply->bytesAvailable();
			QByteArray input = reply->read(bytes_available);
			Q_ASSERT(input.size() == bytes_available);
			mZstream.next_in = (Bytef *)input.constData();
			mZstream.avail_in = bytes_available;

			uInt old_avail_out = mZstream.avail_out;
			int res = inflate(&mZstream, Z_SYNC_FLUSH);
			if(res != Z_OK && res != Z_STREAM_END)
			{
				reply->abort();
				QMessageBox::warning(this, "ZLib", mZstream.msg);
				return;
			}

			mManifestJson.write((const char *)mBuffer, old_avail_out - mZstream.avail_out);
			mZstream.avail_out = mBufferSize;
			mZstream.next_out = mBuffer;
			Q_ASSERT(mZstream.avail_in == 0);
		}
		else
		{
			reply->abort();
			QMessageBox::critical(this, tr("Manifest"), tr("Error while getting manifest (1).\n%1").arg(reply->errorString()));
		}
	}
}

void PAAlternativeLauncher::manifestDownloadProgress(qint64 downloaded, qint64 total)
{
	mPatchProgressbar->setValue(100. * downloaded / total);
}

void PAAlternativeLauncher::manifestFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			int res = inflateEnd(&mZstream);
			if(res != Z_OK && res != Z_STREAM_END)
			{
				QMessageBox::warning(this, "ZLib 2", mZstream.msg);
				return;
			}

			delete[] mBuffer;
			mBuffer = NULL;

			QVariantMap object = mStreamsComboBox->currentData().toMap();
			QString downloadurl = object["DownloadUrl"].toString();
			QString titlefolder = object["TitleFolder"].toString();
			QString authsuffix = object["AuthSuffix"].toString();

			mNetworkAccessManager->deleteLater();

			mPatcher = new Patcher;
			mPatcher->moveToThread(&mPatcherThread);

			mPatcher->setInstallPath(mInstallPathLineEdit->text());
			mPatcher->setDownloadUrl(downloadurl + "/" + titlefolder + "/hashed/%1" + authsuffix);
			mPatcher->giveJsonData(mManifestJson.buffer());
			mManifestJson.close();

			connect(mPatcher, SIGNAL(done()), this, SLOT(patcherDone()));
			connect(mPatcher, SIGNAL(error(QString)), this, SLOT(patcherError(QString)));
			connect(mPatcher, SIGNAL(progress(int)), this, SLOT(patcherProgress(int)));
			connect(mPatcher, SIGNAL(stateChange(QString)), this, SLOT(patcherStateChange(QString)));
			connect(&mPatcherThread, SIGNAL(finished()), mPatcher, SLOT(deleteLater()));
			connect(&mPatcherThread, SIGNAL(started()), mPatcher, SLOT(parseManifest()));

			mPatcherThread.start();
		}
		else
		{
			if(reply->error() == QNetworkReply::AuthenticationRequiredError)
				setState(login_state);
			else
				QMessageBox::critical(this, tr("Manifest"), tr("Error while getting manifest (2).\n%1").arg(reply->errorString()));
		}

		reply->deleteLater();
	}
}

void PAAlternativeLauncher::streamNewsReplyFinished()
{
	QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
	if(reply)
	{
		if(reply->error() == QNetworkReply::NoError)
		{
			info.log("News", "Retrieved.", true);
			QString stream_name = reply->request().attribute(QNetworkRequest::User).toString();
			info.log("News", QString("Is for %1.").arg(stream_name), true);
			if(!stream_name.isEmpty())
			{
				mStreamNews[stream_name] = reply->readAll();
				info.log("News", "Loading into text browser", true);
				if(mStreamsComboBox->currentText() == stream_name)
					mPatchTextBrowser->setHtml(mStreamNews[stream_name]);
			}
		}
		else
		{
			if(reply->error() == QNetworkReply::AuthenticationRequiredError)
				setState(login_state);
			else
				QMessageBox::critical(this, tr("Stream news"), tr("Error while getting stream news.\n%1").arg(reply->errorString()));
		}

		reply->deleteLater();
	}
}



/* Utility functions
 */

void PAAlternativeLauncher::prepareZLib()
{
	mBuffer = new Bytef[mBufferSize];

	mZstream.next_out = mBuffer;
	mZstream.avail_out = mBufferSize;
	mZstream.next_in = Z_NULL;
	mZstream.avail_in = 0;
	mZstream.zalloc = Z_NULL;
	mZstream.zfree = Z_NULL;
	mZstream.opaque = Z_NULL;
	if(inflateInit2(&mZstream, 16 + MAX_WBITS) != Z_OK)
	{
		QMessageBox::critical(this, "ZLib", mZstream.msg);
		return;
	}
	mManifestJson.close();
	mManifestJson.open(QBuffer::WriteOnly);
}

void PAAlternativeLauncher::setState(PAAlternativeLauncher::EState state)
{
	mLoginWidget->setVisible(state == login_state);
	mDownloadWidget->setVisible(state == download_state);
	mWaitWidget->setVisible(state == wait_state);
	
	if(state == login_state)
	{
		if(mUserNameLineEdit->text().isEmpty())
			mUserNameLineEdit->setFocus();
		else
			mPasswordLineEdit->setFocus();
	}
	else if(state == download_state)
	{
		mLaunchButton->setEnabled(true);
		mDownloadButton->setEnabled(true);
	}
}

QString PAAlternativeLauncher::currentInstalledVersion()
{
	if(mInstallPathLineEdit->text().isEmpty())
		return "";

#ifdef __APPLE__
    QFile versionfile(mInstallPathLineEdit->text() + "/PA.app/Contents/Resources/version.txt");
#else
	QFile versionfile(mInstallPathLineEdit->text() + "/version.txt");
#endif
	if(versionfile.open(QFile::ReadOnly))
	{
		return versionfile.readLine().trimmed();
	}
	else
		return "";
}

quint64 PAAlternativeLauncher::getFreeDiskspaceInMB(QString directory)
{
	QByteArray dir = directory.toUtf8();
#if defined(linux) || defined(__APPLE__)
	struct statvfs diskinfo;
	int rc = statvfs(dir.constData(), &diskinfo);
	if(rc != 0)
	{
		info.log(tr("Disk space"), QString("could not be determined (%1)").arg(errno));
		return ULONG_MAX;
	}
	return (diskinfo.f_bavail / 1024) * (diskinfo.f_bsize / 1024);
#elif defined(_WIN32)
	ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
	if(GetDiskFreeSpaceEx(dir.constData(), &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
	{
		return FreeBytesAvailable.QuadPart / (1024*1024);
	}
#endif

	return ULONG_MAX;
}

void PAAlternativeLauncher::launchPA(bool offline)
{
	if(mInstallPathLineEdit->text().isEmpty())
	{
		info.critical(tr("Launch"), tr("Install path is empty."));
		mInstallPathLineEdit->setFocus();
		return;
	}

	QString command;
	QStringList parameters;
	bool use_ticket = false;

	if(!offline)
	{
		QString uber_version = mStreamsComboBox->currentData().toMap()["BuildId"].toString();
		QString current_version = currentInstalledVersion();

		if(uber_version != current_version)
		{
			if(QMessageBox::warning(this, tr("Update required"), tr("There is an update available for this stream.\nPlanetary Annihilation will not work on-line.\n\nContinue in off-line mode?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				return;

			use_ticket = false;
		}
		else
			use_ticket = true;
	}

	command =
		mInstallPathLineEdit->text() +
#ifdef linux
		"/PA"
#elif _WIN32
		"\\bin_x64\\PA.exe"
// or:	"\\bin_x86\\PA.exe"
#elif __APPLE__
		"/PA.app/Contents/MacOS/PA"
#endif
	;

#ifdef linux
	switch(mUseOptimus)
	{
		case AdvancedDialog::optirun:
			parameters << command;
			command = "optirun";
			break;
		case AdvancedDialog::primusrun:
			parameters << command;
			command = "primusrun";
			break;
		default:
			break;
	}
#endif

	if(use_ticket)
	{
		parameters.append("--ticket");
		parameters.append(mSessionTicket);
	}
	parameters.append(mExtraParameters.split(" "));

	if(QProcess::startDetached(command, parameters))
	{
		close();
	}
	else
		info.critical(tr("Failed to launch"), tr("Error while starting PA with the following command:") + "\n" + command);
}

#include "paalternativelauncher.moc"
