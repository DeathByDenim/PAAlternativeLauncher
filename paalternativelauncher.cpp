#define VERSION "0.3"

#include "paalternativelauncher.h"
#include "advanceddialog.h"
#include "information.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QImage>
#include <QScrollArea>
#include <QProgressBar>
#include <QMovie>
#include <QPushButton>
#include <QComboBox>
#include <QShowEvent>
#include <QResizeEvent>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QTextBrowser>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <QProcess>
#ifdef _WIN32
#  define QJSON_STATIC
#endif
#include <qjson/parser.h>

PAAlternativeLauncher::PAAlternativeLauncher()
 : m_network_access_manager(new QNetworkAccessManager(this))
 , m_patcher(this)
#ifdef linux
 , m_platform("Linux")
#elif _WIN32
 , m_platform("Windows")
#elif __APPLE__
 , m_platform("OSX")
#else
# error Not a valid os
#endif
 , m_download_button(NULL)
{
	setWindowIcon(QIcon(":/img/icon.png"));
	setWindowTitle("PA Alternative Launcher");
	info.setParent(this);

	QPalette* palette = new QPalette();
 	palette->setBrush(QPalette::Background,*(new QBrush(*(new QPixmap(":/img/img_bground_galaxy_01.png")))));
 	setPalette(*palette);

	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
	
	QLabel *disclaimerLabel = new QLabel(tr("This is an UNOFFICIAL launcher and not connected to Uber in any way."), mainWidget);
	disclaimerLabel->setStyleSheet("QLabel {color: red; font-weight: bold;}");
	disclaimerLabel->setAlignment(Qt::AlignCenter);
	disclaimerLabel->setAutoFillBackground(true);
	disclaimerLabel->setBackgroundRole(QPalette::Dark);
	mainLayout->addWidget(disclaimerLabel);

	QLabel *headerLabel = new QLabel(mainWidget);
	headerLabel->setPixmap(QPixmap(":/img/img_pa_logo_start_rest.png"));
	headerLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(headerLabel);

	QWidget *centreWidget = new QWidget(mainWidget);
	QHBoxLayout *centreLayout = new QHBoxLayout(centreWidget);

	QLabel *commanderLabel = new QLabel(centreWidget);
	commanderLabel->setPixmap(QPixmap(":img/img_imperial_invictus.png").scaled(10, 400, Qt::KeepAspectRatioByExpanding));
	centreLayout->addWidget(commanderLabel);

	m_login_widget = createLoginWidget(centreWidget);
	m_download_widget = createDownloadWidget(centreWidget);
	m_download_widget->setVisible(false);
	m_wait_widget = createWaitWidget(centreWidget);
	m_wait_widget->setVisible(false);
	centreLayout->addWidget(m_login_widget);
	centreLayout->addWidget(m_download_widget);
	centreLayout->addWidget(m_wait_widget);

	mainLayout->addWidget(centreWidget);

	m_patch_label = new QLabel(mainWidget);
	m_patch_label->setStyleSheet("QLabel {color: white; font-weight: bold}");
	m_patch_label->setAlignment(Qt::AlignCenter);
	connect(&m_patcher, SIGNAL(state(QString)), SLOT(patcherState(QString)));
	mainLayout->addWidget(m_patch_label);

	m_patch_progressbar = new QProgressBar(mainWidget);
	m_patch_progressbar->setMinimum(0);
	m_patch_progressbar->setMaximum(100);
	connect(&m_patcher, SIGNAL(progress(int)), SLOT(patcherProgress(int)));
	mainLayout->addWidget(m_patch_progressbar);

	QLabel *disclaimerLabel2 = new QLabel(tr("This is an UNOFFICIAL launcher and not connected to Uber in any way."), mainWidget);
	disclaimerLabel2->setStyleSheet("QLabel {color: red; font-weight: bold;}");
	disclaimerLabel2->setAlignment(Qt::AlignCenter);
	disclaimerLabel2->setAutoFillBackground(true);
	disclaimerLabel2->setBackgroundRole(QPalette::Dark);
	mainLayout->addWidget(disclaimerLabel2);

	QWidget *aboutWidget = new QWidget(mainWidget);
	QHBoxLayout *aboutLayout = new QHBoxLayout(aboutWidget);

	QLabel *createdByLabel = new QLabel(tr("Created by") + " DeathByDenim", aboutWidget);
	createdByLabel->setStyleSheet("QLabel {color: white}");
	aboutLayout->addWidget(createdByLabel);
	QLabel *blackmageLabel = new QLabel(aboutWidget);
	blackmageLabel->setPixmap(QPixmap(":img/blackmage.png"));
	blackmageLabel->setMaximumSize(16, 16);
	aboutLayout->addWidget(blackmageLabel);
	aboutLayout->addStretch();
	QLabel *versionLabel = new QLabel(tr("Version") + " " VERSION, aboutWidget);
	versionLabel->setStyleSheet("QLabel {color: white}");
	aboutLayout->addWidget(versionLabel);

	mainLayout->addWidget(aboutWidget);

	setCentralWidget(mainWidget);

	m_network_access_manager = new QNetworkAccessManager(this);
	connect(m_network_access_manager, SIGNAL(finished(QNetworkReply*)), SLOT(replyReceived(QNetworkReply*)));

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	restoreGeometry(settings.value("mainwindow/geometry").toByteArray());

	m_session_ticket = settings.value("login/sessionticket").toString();
	if(!m_session_ticket.isEmpty())
	{
		m_login_widget->setVisible(false);
		m_download_widget->setVisible(false);
		m_wait_widget->setVisible(true);

		QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + m_platform));
		request.setRawHeader("X-Authorization", m_session_ticket.toAscii());
		request.setAttribute(QNetworkRequest::User, QVariant("streams"));
		m_network_access_manager->get(request);
	}

	if(m_username_lineedit->text().isEmpty())
		m_username_lineedit->setFocus();
	else
		m_password_lineedit->setFocus();
}

PAAlternativeLauncher::~PAAlternativeLauncher()
{
}

QWidget* PAAlternativeLauncher::createLoginWidget(QWidget *parent)
{
	QWidget *mainWidget = new QWidget(parent);
	QPalette palette = mainWidget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	mainWidget->setPalette(palette);
	QFormLayout *mainLayout = new QFormLayout(mainWidget);
	mainWidget->setLayout(mainLayout);

	QLabel *loginLabel = new QLabel(tr("LOGIN"), mainWidget);
	QFont font = loginLabel->font();
	font.setBold(true);
	font.setPointSizeF(3*font.pointSizeF());
	loginLabel->setFont(font);
	loginLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addRow(loginLabel);

	m_username_lineedit = new QLineEdit(mainWidget);
	mainLayout->addRow(tr("Uber ID"), m_username_lineedit);

	m_password_lineedit = new QLineEdit(mainWidget);
	m_password_lineedit->setEchoMode(QLineEdit::Password);
	connect(m_password_lineedit, SIGNAL(returnPressed()), SLOT(lineEditReturnPressed()));
	mainLayout->addRow(tr("Password"), m_password_lineedit);

	QPushButton *loginButton = new QPushButton(tr("Login"), mainWidget);
	mainLayout->addRow(loginButton);
	connect(loginButton, SIGNAL(clicked(bool)), SLOT(loginPushButtonClicked(bool)));

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	m_username_lineedit->setText(settings.value("login/username").toString());

	return mainWidget;
}

QWidget* PAAlternativeLauncher::createDownloadWidget(QWidget* parent)
{
	QWidget *mainWidget = new QWidget(parent);
	QPalette palette = mainWidget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	mainWidget->setPalette(palette);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
	mainWidget->setLayout(mainLayout);

	QWidget *streamWidget = new QWidget(mainWidget);
	QHBoxLayout *streamLayout = new QHBoxLayout(streamWidget);
	QLabel *streamsLabel = new QLabel(tr("Stream"), streamWidget);
	streamLayout->addWidget(streamsLabel);
	m_streams_combo_box = new QComboBox(streamWidget);
	streamLayout->addWidget(m_streams_combo_box);
	connect(m_streams_combo_box, SIGNAL(currentIndexChanged(QString)), SLOT(streamsCurrentIndexChanged(QString)));
	streamLayout->addStretch();
	m_update_available_label = new QLabel("", streamWidget);
	m_update_available_label->setStyleSheet("QLabel {font-weight: bold; color: white}");
	streamLayout->addWidget(m_update_available_label);
	mainLayout->addWidget(streamWidget);

	m_patch_text_browser = new QTextBrowser(mainWidget);
	mainLayout->addWidget(m_patch_text_browser);

	QWidget *installPathWidget = new QWidget(mainWidget);
	QHBoxLayout *installPathLayout = new QHBoxLayout(installPathWidget);

	m_installPathLineEdit = new QLineEdit(installPathWidget);
	installPathLayout->addWidget(m_installPathLineEdit);

	QPushButton *installPathPushButton = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), "", installPathWidget);
	installPathPushButton->setFlat(true);
	connect(installPathPushButton, SIGNAL(clicked(bool)), SLOT(installPathButtonClicked(bool)));
	installPathLayout->addWidget(installPathPushButton);
	mainLayout->addWidget(installPathWidget);

	QDialogButtonBox *buttonbox = new QDialogButtonBox(mainWidget);
	QPushButton *launchButton = new QPushButton(tr("&Launch PA"), buttonbox);
	connect(launchButton, SIGNAL(clicked(bool)), SLOT(launchPushButtonClicked(bool)));
	buttonbox->addButton(launchButton, QDialogButtonBox::AcceptRole);
	QPushButton *advancedButton = new QPushButton(tr("&Advanced"), buttonbox);
	connect(advancedButton, SIGNAL(clicked(bool)), SLOT(advancedPushButtonClicked(bool)));
	buttonbox->addButton(advancedButton, QDialogButtonBox::NoRole);
	m_download_button = new QPushButton(tr("&Download and install"), buttonbox);
	connect(m_download_button, SIGNAL(clicked(bool)), SLOT(downloadPushButtonClicked(bool)));
	buttonbox->addButton(m_download_button, QDialogButtonBox::NoRole);

	mainLayout->addWidget(buttonbox);

	return mainWidget;
}

QWidget* PAAlternativeLauncher::createWaitWidget(QWidget* parent)
{
	QWidget *mainWidget = new QWidget(parent);
	QPalette palette = mainWidget->palette();
	palette.setColor(QPalette::WindowText, Qt::white);
	mainWidget->setPalette(palette);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
	mainWidget->setLayout(mainLayout);
	
	mainLayout->addStretch();

	QLabel *loggingInLabel = new QLabel(tr("Logging in..."), mainWidget);
	QFont font = loggingInLabel->font();
	font.setBold(true);
	font.setPointSizeF(3*font.pointSizeF());
	loggingInLabel->setFont(font);
	loggingInLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(loggingInLabel);

	mainLayout->addSpacing(30);

	QMovie *loadingMovie = new QMovie(":/img/loading.gif", "gif", this);
	QLabel *loadingLabel = new QLabel(this);
	loadingLabel->setMovie(loadingMovie);
	loadingMovie->start();
	loadingLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(loadingLabel);

	mainLayout->addStretch();

	return mainWidget;
}

void PAAlternativeLauncher::loginPushButtonClicked(bool)
{
	m_login_widget->setVisible(false);
	m_download_widget->setVisible(false);
	m_wait_widget->setVisible(true);

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");

	QNetworkRequest request(QUrl("https://uberent.com/GC/Authenticate"));
	QString data = QString("{\"TitleId\": 4,\"AuthMethod\": \"UberCredentials\",\"UberName\": \"%1\",\"Password\": \"%2\"}").arg(m_username_lineedit->text()).arg(m_password_lineedit->text());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=utf-8");
	request.setAttribute(QNetworkRequest::User, QVariant("login"));
	m_network_access_manager->post(request, data.toUtf8());
}

void PAAlternativeLauncher::lineEditReturnPressed()
{
	loginPushButtonClicked(true);
}

void PAAlternativeLauncher::replyReceived(QNetworkReply* reply)
{
	if(reply->error() != QNetworkReply::NoError)
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		if(type == "streamnews" && reply->error() == QNetworkReply::AuthenticationRequiredError)
		{
			m_login_widget->setVisible(true);
			m_download_widget->setVisible(false);
			m_wait_widget->setVisible(false);
			if(m_username_lineedit->text().isEmpty())
				m_username_lineedit->setFocus(Qt::TabFocusReason);
			else
				m_password_lineedit->setFocus(Qt::TabFocusReason);

			info.log(tr("Communication error"), tr("session ticket expired."), false);
		}
		else
		{
			info.critical(tr("Communication error"), "Error while handling " + type + ". Details: " + reply->errorString() + ".");
			m_login_widget->setVisible(true);
			m_download_widget->setVisible(false);
			m_wait_widget->setVisible(false);
		}
	}
	else if(reply->isFinished())
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		if(type == "login")
		{
			QByteArray data = reply->readAll();
			m_session_ticket = decodeLoginData(data);
			if(m_session_ticket.isEmpty())
			{
				info.critical(tr("Communication error"), "Error while doing login. Details: Failed to get session ticket.");
				m_login_widget->setVisible(true);
				m_download_widget->setVisible(false);
				m_wait_widget->setVisible(false);
				return;
			}
			QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
			settings.setValue("login/sessionticket", m_session_ticket);
			QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + m_platform));
			request.setRawHeader("X-Authorization", m_session_ticket.toAscii());
			request.setAttribute(QNetworkRequest::User, "streams");
			m_network_access_manager->get(request);
		}
		else if(type == "streams")
		{
			QByteArray data = reply->readAll();
			m_patcher.decodeStreamsData(data);
			QStringList streamnames = m_patcher.streamNames();

			m_streams_combo_box->clear();
			m_streams_combo_box->addItems(streamnames);

			m_login_widget->setVisible(false);
			m_wait_widget->setVisible(false);
			m_download_widget->setVisible(true);

			for(QStringList::const_iterator streamname = streamnames.constBegin(); streamname != streamnames.constEnd(); ++streamname)
			{
				QNetworkRequest request(QUrl("https://uberent.com/Launcher/StreamNews?StreamName=" + *streamname + "&ticket=" + m_session_ticket));
				request.setRawHeader("X-Authorization", m_session_ticket.toAscii());
				request.setAttribute(QNetworkRequest::User, "streamnews");
				request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User+1), QString(*streamname));
				m_network_access_manager->get(request);
			}
			checkForUpdates(streamnames);
		}
		else if(type == "streamnews")
		{
			QString stream = reply->request().attribute((QNetworkRequest::Attribute)(QNetworkRequest::User+1)).toString();
			QByteArray data = reply->readAll();
			m_stream_news[stream] = QString::fromUtf8(data);
			if(m_streams_combo_box->currentText() == stream)
				m_patch_text_browser->setHtml(QString::fromUtf8(data));
		}
	}
	reply->deleteLater();
}

QString PAAlternativeLauncher::decodeLoginData(const QByteArray &data)
{
	QJson::Parser parser;
	bool ok;
	
	QVariantMap logindata = parser.parse(data, &ok).toMap();
	if(!ok)
	{
	}

	const QString sessionticketstring = "\"SessionTicket\"";
	int pos = data.indexOf(sessionticketstring);
	if(pos > 0)
	{
		pos += sessionticketstring.count() + 1;
		pos = data.indexOf("\"", pos);
		if(pos >= 0)
		{
			int begin = pos + 1;
			pos = data.indexOf("\"", begin);
			if(pos >= 0)
			{
				int end = pos;
				return data.mid(begin, end - begin);
			}
		}
	}

	return QString("");
}

void PAAlternativeLauncher::closeEvent(QCloseEvent* event)
{
	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	settings.setValue("mainwindow/geometry", saveGeometry());
	settings.setValue("login/username", m_username_lineedit->text());

	QWidget::closeEvent(event);
}

void PAAlternativeLauncher::showEvent(QShowEvent* event)
{
	if(m_username_lineedit->text().isEmpty())
		m_username_lineedit->setFocus(Qt::TabFocusReason);
	else
		m_password_lineedit->setFocus(Qt::TabFocusReason);

	QWidget::showEvent(event);
}

void PAAlternativeLauncher::downloadPushButtonClicked(bool)
{
	m_download_button->setEnabled(false);

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	settings.setValue(m_streams_combo_box->currentText() + "/installpath", m_installPathLineEdit->text());
	m_patcher.setInstallPath(m_installPathLineEdit->text());
	m_patcher.downloadStream(m_streams_combo_box->currentText());
}

void PAAlternativeLauncher::patcherProgress(int percentage)
{
	if(m_patch_progressbar)
		m_patch_progressbar->setValue(percentage);
}

void PAAlternativeLauncher::patcherState(QString state)
{
	if(m_patch_label)
		m_patch_label->setText(state);

	if(state == "Done")
	{
		m_download_button->setEnabled(true);
		m_requires_update[m_streams_combo_box->currentText()] = false;
		m_update_available_label->setText("");
	}
}

void PAAlternativeLauncher::installPathButtonClicked(bool)
{
	QString installPath = QFileDialog::getExistingDirectory(this, tr("Choose installation directory"));
	m_installPathLineEdit->setText(installPath);
	m_patcher.setInstallPath(installPath);
}

void PAAlternativeLauncher::launchPushButtonClicked(bool)
{
	QString command;
	QStringList parameters;
	
	if(m_requires_update[m_streams_combo_box->currentText()])
	{
		if(QMessageBox::warning(this, tr("Update required"), tr("There is an update available for this stream.\nPlanetary Annihilation will not work on-line.\nContinue anyway?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;
	}

	command =
		m_installPathLineEdit->text() +
#ifdef linux
		"/PA"
#elif _WIN32
		"\\bin_x64\\PA.exe"
#elif __APPLE__
#	error Right...
#endif
	;

	switch(m_use_optimus)
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


	parameters.append("--ticket");
	parameters.append(m_session_ticket);
	parameters.append(m_extraParameters.split(" "));

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

	AdvancedDialog *advanceddialog = new AdvancedDialog(m_extraParameters, m_use_optimus, this);
	if(advanceddialog->exec() == QDialog::Accepted)
	{
		m_extraParameters = advanceddialog->parameters();
		m_use_optimus = advanceddialog->useOptimus();
		settings.setValue(m_streams_combo_box->currentText() + "/extraparameters", m_extraParameters);
		settings.setValue(m_streams_combo_box->currentText() + "/useoptirun", (int)m_use_optimus);
	}

	delete advanceddialog;
}

void PAAlternativeLauncher::streamsCurrentIndexChanged(QString streamname)
{
	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	m_extraParameters = settings.value(streamname + "/extraparameters").toString();
	m_use_optimus = (AdvancedDialog::optimus_t)settings.value(streamname + "/useoptirun").toInt();

	m_installPathLineEdit->setText(settings.value(streamname + "/installpath").toString());

	m_patch_text_browser->setHtml(m_stream_news[streamname]);
	
	if(m_requires_update[streamname])
		m_update_available_label->setText(tr("Update available!"));
	else
		m_update_available_label->setText("");
}

void PAAlternativeLauncher::checkForUpdates(QStringList streamnames)
{
	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	for(QStringList::const_iterator streamname = streamnames.constBegin(); streamname != streamnames.constEnd(); ++streamname)
	{
		QString installpath = settings.value(*streamname + "/installpath").toString();
		QFile buildidfile(installpath + "/buildid.txt");
		if(!buildidfile.open(QIODevice::ReadOnly))
		{
			m_requires_update[*streamname] = true;
			break;
		}
		QByteArray buildidbytearray = buildidfile.readLine();
		m_requires_update[*streamname] = (buildidbytearray.mid(8).trimmed() != m_patcher.buildId(*streamname));
		
		if(m_requires_update[m_streams_combo_box->currentText()])
			m_update_available_label->setText(tr("Update available!"));
		else
			m_update_available_label->setText("");
	}
}

#include "paalternativelauncher.moc"
