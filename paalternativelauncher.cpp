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

//#include <QDebug>
#include <QFile>
#include <zlib.h>
#include "information.h"
#include "version.h"
#include "patcher.h"
#include "advanceddialog.h"
#include "moddatabaseframe.h"


PAAlternativeLauncher::PAAlternativeLauncher()
 : mNetworkAccessManager(new QNetworkAccessManager(NULL))
#if defined(linux)
 , mPlatform("Linux")
#elif defined(_WIN32)
 , mPlatform("Windows")
#elif defined(__APPLE__)
 , mPlatform("OSX")
#else
# error Not a supported os
#endif
 , mBufferSize(1024*1024), mBuffer(NULL), mPatcher(NULL)
{
	setWindowIcon(QIcon(":/img/icon.png"));
	setWindowTitle("PA Alternative Launcher");
	info.setParent(this);
	info.log("Starting launcher", QDateTime::currentDateTime().toString());

	QPalette* palette = new QPalette();
 	palette->setBrush(QPalette::Background,*(new QBrush(*(new QPixmap(":/img/img_bground_galaxy_01.png")))));
 	setPalette(*palette);

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

	QLabel *commander_label = new QLabel(centre_widget);
	commander_label->setPixmap(QPixmap(":img/img_imperial_invictus.png").scaled(10, 400, Qt::KeepAspectRatioByExpanding));
	centre_layout->addWidget(commander_label);

	mLoginWidget = createLoginWidget(centre_widget);
	mDownloadWidget = createDownloadWidget(centre_widget);
	mDownloadWidget->setVisible(false);
	mWaitWidget = createWaitWidget(centre_widget);
	mWaitWidget->setVisible(false);
	centre_layout->addWidget(mLoginWidget);
	centre_layout->addWidget(mDownloadWidget);
	centre_layout->addWidget(mWaitWidget);

	mModDatabaseFrame = new ModDatabaseFrame(centre_widget);
	centre_layout->addWidget(mModDatabaseFrame);

	main_layout->addWidget(centre_widget);

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

	mSessionTicket = settings.value("login/sessionticket").toString();
	if(!mSessionTicket.isEmpty())
	{
		setState(wait_state);

		QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + mPlatform));
		request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
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
	QPalette palette = main_widget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	main_widget->setPalette(palette);
	QFormLayout *main_layout = new QFormLayout(main_widget);
	main_widget->setLayout(main_layout);

	QLabel *login_label = new QLabel(tr("LOGIN"), main_widget);
	QFont font = login_label->font();
	font.setBold(true);
	font.setPointSizeF(3*font.pointSizeF());
	login_label->setFont(font);
	login_label->setAlignment(Qt::AlignCenter);
	main_layout->addRow(login_label);

	mUserNameLineEdit = new QLineEdit(main_widget);
	main_layout->addRow(tr("Uber ID"), mUserNameLineEdit);

	mPasswordLineEdit = new QLineEdit(main_widget);
	mPasswordLineEdit->setEchoMode(QLineEdit::Password);
	connect(mPasswordLineEdit, SIGNAL(returnPressed()), SLOT(passwordLineEditReturnPressed()));
	main_layout->addRow(tr("Password"), mPasswordLineEdit);

	QPushButton *login_button = new QPushButton(tr("Login"), main_widget);
	main_layout->addRow(login_button);
	connect(login_button, SIGNAL(clicked(bool)), SLOT(loginPushButtonClicked(bool)));

	QPushButton *launch_offline_button = new QPushButton(tr("Launch offline (no password needed)"), main_widget);
	main_layout->addRow(launch_offline_button);
	connect(launch_offline_button, SIGNAL(clicked(bool)), SLOT(launchOfflinePushButtonClicked(bool)));

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

	QWidget *stream_widget = new QWidget(main_widget);
	QHBoxLayout *stream_layout = new QHBoxLayout(stream_widget);
	QLabel *streams_label = new QLabel(tr("Stream"), stream_widget);
	streams_label->setPalette(palette);
	stream_layout->addWidget(streams_label);
	mStreamsComboBox = new QComboBox(stream_widget);
	stream_layout->addWidget(mStreamsComboBox);
	connect(mStreamsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(streamsComboBoxCurrentIndexChanged(int)));
	stream_layout->addStretch();
	mUpdateAvailableLabel = new QLabel("", stream_widget);
	mUpdateAvailableLabel->setStyleSheet("QLabel {font-weight: bold; color: white}");
	mUpdateAvailableLabel->setPalette(palette);
	stream_layout->addWidget(mUpdateAvailableLabel);
	main_layout->addWidget(stream_widget);

	mPatchTextBrowser = new QTextBrowser(main_widget);
	main_layout->addWidget(mPatchTextBrowser);

	QWidget *install_path_widget = new QWidget(main_widget);
	QHBoxLayout *install_path_layout = new QHBoxLayout(install_path_widget);

	mInstallPathLineEdit = new QLineEdit(install_path_widget);
	install_path_layout->addWidget(mInstallPathLineEdit);

	QPushButton *install_path_push_button = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), "", install_path_widget);
	install_path_push_button->setFlat(true);
	connect(install_path_push_button, SIGNAL(clicked(bool)), SLOT(installPathButtonClicked(bool)));
	install_path_layout->addWidget(install_path_push_button);
	main_layout->addWidget(install_path_widget);

	QDialogButtonBox *button_box = new QDialogButtonBox(main_widget);
	QPushButton *launch_button = new QPushButton(tr("&Launch PA"), button_box);
	connect(launch_button, SIGNAL(clicked(bool)), SLOT(launchPushButtonClicked(bool)));
	button_box->addButton(launch_button, QDialogButtonBox::AcceptRole);
	QPushButton *advanced_button = new QPushButton(tr("&Advanced"), button_box);
	connect(advanced_button, SIGNAL(clicked(bool)), SLOT(advancedPushButtonClicked(bool)));
	button_box->addButton(advanced_button, QDialogButtonBox::NoRole);
	mDownloadButton = new QPushButton(tr("&Download and install"), button_box);
	connect(mDownloadButton, SIGNAL(clicked(bool)), SLOT(downloadPushButtonClicked(bool)));
	button_box->addButton(mDownloadButton, QDialogButtonBox::NoRole);

	main_layout->addWidget(button_box);

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

	QPushButton *launch_offline_button = new QPushButton(tr("Launch offline"), main_widget);
	main_layout->addWidget(launch_offline_button);
	connect(launch_offline_button, SIGNAL(clicked(bool)), SLOT(launchOfflinePushButtonClicked(bool)));

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
	request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());

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
	mInstallPathLineEdit->setText(settings.value(current_stream + "/installpath").toString());
	mExtraParameters = settings.value(current_stream + "/extraparameters").toString();
	mUseOptimus = (AdvancedDialog::optimus_t)settings.value(current_stream + "/useoptirun").toInt();

	QString uber_version = mStreamsComboBox->currentData().toMap()["BuildId"].toString();
	QString current_version = currentInstalledVersion();

	if(current_version.isEmpty())
		mUpdateAvailableLabel->setText(tr("Build %2 available").arg(uber_version));
	else
		mUpdateAvailableLabel->setText(tr("Update from %1 to %2 available").arg(current_version).arg(uber_version));

	mUpdateAvailableLabel->setVisible(uber_version != current_version);
	
	mModDatabaseFrame->setInstallPath(mInstallPathLineEdit->text());
	mModDatabaseFrame->setVisible(!mExtraParameters.contains("--nomods"));
}

void PAAlternativeLauncher::installPathButtonClicked(bool)
{
	QString installPath = QFileDialog::getExistingDirectory(this, tr("Choose installation directory"), mInstallPathLineEdit->text(), QFileDialog::ShowDirsOnly);
	mInstallPathLineEdit->setText(installPath);
}

void PAAlternativeLauncher::downloadPushButtonClicked(bool)
{
	QVariantMap object = mStreamsComboBox->currentData().toMap();
	QString downloadurl = object["DownloadUrl"].toString();
	QString titlefolder = object["TitleFolder"].toString();
	QString manifestname = object["ManifestName"].toString();
	QString authsuffix = object["AuthSuffix"].toString();

	if(!downloadurl.isEmpty() && !titlefolder.isEmpty() && !manifestname.isEmpty() && !authsuffix.isEmpty())
	{
		mPatchLabel->setText("Downloading manifest");

		prepareZLib();
		QUrl manifesturl(downloadurl + '/' + titlefolder + '/' + manifestname + authsuffix);
		QNetworkRequest request(manifesturl);
		request.setRawHeader("User-Agent", "test");
		QNetworkReply *reply = mNetworkAccessManager->get(request);
		connect(reply, SIGNAL(readyRead()), SLOT(manifestReadyRead()));
		connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(manifestDownloadProgress(qint64,qint64)));
		connect(reply, SIGNAL(finished()), SLOT(manifestFinished()));
	}
}

void PAAlternativeLauncher::patcherDone()
{
	QMessageBox::information(this, "Patcher", "Done");
}

void PAAlternativeLauncher::patcherError(QString error)
{
	QMessageBox::critical(this, "Patcher", error);
}

void PAAlternativeLauncher::patcherProgress(int percentage)
{
	mPatchProgressbar->setValue(percentage);
}

void PAAlternativeLauncher::launchOfflinePushButtonClicked(bool)
{
	// Update manually, because no streams are selected yet.
	QSettings settings;
	QString install_path = settings.value("stable/installpath").toString();
	mExtraParameters = settings.value("stable/extraparameters").toString();
	mUseOptimus = (AdvancedDialog::optimus_t)settings.value("stable/useoptirun").toInt();

	QString command;
	QStringList parameters;

	command =
		install_path +
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

	parameters.append(mExtraParameters.split(" "));

	if(QProcess::startDetached(command, parameters))
	{
		close();
	}
	else
		info.critical(tr("Failed to launch"), tr("Error while starting PA with the following command:") + "\n" + command);
}

void PAAlternativeLauncher::launchPushButtonClicked(bool)
{
	QString command;
	QStringList parameters;

	QString uber_version = mStreamsComboBox->currentData().toMap()["BuildId"].toString();
	QString current_version = currentInstalledVersion();

	bool use_ticket = true;
	if(uber_version != current_version)
	{
		if(QMessageBox::warning(this, tr("Update required"), tr("There is an update available for this stream.\nPlanetary Annihilation will not work on-line.\n\nContinue in off-line mode?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;

		use_ticket = false;
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
	}

	delete advanceddialog;
}

void PAAlternativeLauncher::patcherStateChange(QString state)
{
	mPatchLabel->setText(state);
	if(state == "Done")
	{
		mPatchLabel->setVisible(false);
		mPatcherThread.quit();
	}
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
			QJsonDocument authjson = QJsonDocument::fromJson(reply->readAll());
			mSessionTicket = authjson.object()["SessionTicket"].toString();
			if(!mSessionTicket.isEmpty())
			{
				QSettings settings;
				settings.setValue("login/sessionticket", mSessionTicket);

				QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + mPlatform));
				request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
				request.setRawHeader("User-Agent", QString("PAAlternativeLauncher/%1").arg(VERSION).toUtf8());
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
			setState(download_state);

			mStreamsComboBox->clear();
			QJsonDocument authjson = QJsonDocument::fromJson(reply->readAll());
			QJsonArray streams = authjson.object()["Streams"].toArray();
			for(QJsonArray::const_iterator stream = streams.constBegin(); stream != streams.constEnd(); ++stream)
			{
				QJsonObject object = (*stream).toObject();
				QString download_url = object["DownloadUrl"].toString();
				QString title_folder = object["TitleFolder"].toString();
				QString manifest_name = object["ManifestName"].toString();
				QString auth_suffix = object["AuthSuffix"].toString();
				QString stream_name = object["StreamName"].toString();

				mStreamsComboBox->addItem(stream_name, object.toVariantMap());

				// Get the news
				QNetworkRequest request(QUrl("https://uberent.com/Launcher/StreamNews?StreamName=" + stream_name + "&ticket=" + mSessionTicket));
				request.setRawHeader("X-Authorization", mSessionTicket.toUtf8());
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

			mPatcher = new Patcher(mNetworkAccessManager, NULL);
			mPatcher->moveToThread(&mPatcherThread);
			mNetworkAccessManager->moveToThread(&mPatcherThread);

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
			QString stream_name = reply->request().attribute(QNetworkRequest::User).toString();
			if(!stream_name.isEmpty())
			{
				mStreamNews[stream_name] = reply->readAll();
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
}

QString PAAlternativeLauncher::currentInstalledVersion()
{
	if(mInstallPathLineEdit->text().isEmpty())
		return "";

#ifdef __APPLE__
	QFile versionfile(mInstallPathLineEdit->text() + "/PA.app/Resources/version.txt");
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

#include "paalternativelauncher.moc"
