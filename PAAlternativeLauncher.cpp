#include "PAAlternativeLauncher.h"

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
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QSettings>

PAAlternativeLauncher::PAAlternativeLauncher()
 : m_patcher(this)
#ifdef linux
 , m_platform("Linux")
#elif _WIN32
 , m_platform("Windows")
#elif __APPLE__
 , m_platform("MacOS")
#else
# error Not a valid os
#endif
{
	QPalette* palette = new QPalette();
 	palette->setBrush(QPalette::Background,*(new QBrush(*(new QPixmap(":/img/img_bground_galaxy_01.png")))));
 	setPalette(*palette);

	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

	QLabel *headerLabel = new QLabel(mainWidget);
	headerLabel->setPixmap(QPixmap(":/img/img_pa_logo_start_rest.png"));
	mainLayout->addWidget(headerLabel);

	QWidget *centreWidget = new QWidget(mainWidget);
	QHBoxLayout *centreLayout = new QHBoxLayout(centreWidget);

	QLabel *commanderLabel = new QLabel(centreWidget);
	commanderLabel->setPixmap(QPixmap(":img/img_imperial_invictus.png").scaled(10, 400, Qt::KeepAspectRatioByExpanding));
	centreLayout->addWidget(commanderLabel);

	m_login_widget = createLoginWidget(centreWidget);
	m_download_widget = createDownloadWidget(centreWidget);
	m_download_widget->setVisible(false);
	centreLayout->addWidget(m_login_widget);
	centreLayout->addWidget(m_download_widget);

	mainLayout->addWidget(centreWidget);
	
	m_progress_widget = new QProgressBar(this);
	m_progress_widget->setMinimum(0);
	m_progress_widget->setMaximum(100);
	connect(&m_patcher, SIGNAL(progress(int)), SLOT(patcherProgress(int)));
	mainLayout->addWidget(m_progress_widget);

	setCentralWidget(mainWidget);

	m_network_access_manager = new QNetworkAccessManager(this);
	connect(m_network_access_manager, SIGNAL(finished(QNetworkReply*)), SLOT(replyReceived(QNetworkReply*)));

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
}

PAAlternativeLauncher::~PAAlternativeLauncher()
{
}

void PAAlternativeLauncher::installPushButtonPressed()
{
	if(m_progress_widget)
	{
		m_progress_widget->setVisible(true);
	}
}

void PAAlternativeLauncher::quitPushButtonPressed()
{
	close();
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

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");

	m_username_lineedit = new QLineEdit(mainWidget);
	mainLayout->addRow(tr("Uber ID"), m_username_lineedit);
	m_username_lineedit->setText(settings.value("login/username").toString());

	m_password_lineedit = new QLineEdit(mainWidget);
	m_password_lineedit->setEchoMode(QLineEdit::Password);
	m_password_lineedit->setText(settings.value("login/password").toString());
	mainLayout->addRow(tr("Password"), m_password_lineedit);

	m_save_password_check_box = new QCheckBox(mainWidget);
	if(settings.value("login/savepassword").toBool())
		m_save_password_check_box->setChecked(true);
	mainLayout->addRow(tr("Save password (INSECURE)"), m_save_password_check_box);

	QPushButton *loginButton = new QPushButton(tr("Login"), mainWidget);
	mainLayout->addRow(loginButton);
	connect(loginButton, SIGNAL(clicked(bool)), SLOT(loginPushButtonClicked(bool)));

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
	QFormLayout *streamLayout = new QFormLayout(streamWidget);
	m_streams_combo_box = new QComboBox(streamWidget);
	streamLayout->addRow(tr("Stream"), m_streams_combo_box);
	mainLayout->addWidget(streamWidget);

	QScrollArea *updateTextScrollArea = new QScrollArea(mainWidget);
	m_patch_text_label = new QLabel(updateTextScrollArea);
	updateTextScrollArea->setWidget(m_patch_text_label);
	mainLayout->addWidget(updateTextScrollArea);

	QDialogButtonBox *buttonbox = new QDialogButtonBox(mainWidget);
	QPushButton *downloadButton = new QPushButton(tr("&Download"), buttonbox);
	connect(downloadButton, SIGNAL(clicked(bool)), SLOT(downloadPushButtonClicked(bool)));
	buttonbox->addButton(downloadButton, QDialogButtonBox::AcceptRole);
	mainLayout->addWidget(buttonbox);
	
	return mainWidget;
}

void PAAlternativeLauncher::loginPushButtonClicked(bool)
{
	m_patcher.setInstallPath("/home/jarno/Games/PA");
	m_patcher.test();
	return;

	QSettings settings(QSettings::UserScope, "DeathByDenim", "PAAlternativeLauncher");
	settings.setValue("login/savepassword", m_save_password_check_box->isChecked());
	if(m_save_password_check_box->isChecked())
	{
		settings.setValue("login/username", m_username_lineedit->text());
		settings.setValue("login/password", m_password_lineedit->text());
	}
	else
	{
		settings.setValue("login/username", "");
		settings.setValue("login/password", "");
	}

	QNetworkRequest request(QUrl("https://uberent.com/GC/Authenticate"));
	QString data = QString("{\"TitleId\": 4,\"AuthMethod\": \"UberCredentials\",\"UberName\": \"%1\",\"Password\": \"%2\"}").arg(m_username_lineedit->text()).arg(m_password_lineedit->text());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=utf-8");
	request.setAttribute(QNetworkRequest::User, QVariant("login"));
	m_network_access_manager->post(request, data.toUtf8());
}

void PAAlternativeLauncher::replyReceived(QNetworkReply* reply)
{
	if(reply->error() != QNetworkReply::NoError)
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		qDebug() << QString("Communication error for %1: ").arg(type) << reply->errorString();
		QMessageBox::critical(this, QString("Communication error"), "Error while doing " + type + ". Details: " + reply->errorString() + ".");
	}
	else if(reply->isFinished())
	{
		QString type = reply->request().attribute(QNetworkRequest::User).toString();
		qDebug() << type;
		if(type == "login")
		{
			QByteArray data = reply->readAll();
			m_session_ticket = decodeLoginData(data);
			if(m_session_ticket.isEmpty())
			{
				QMessageBox::critical(this, QString("Communication error"), "Error while doing login. Details: Failed to get session ticket.");
				return;
			}
			QNetworkRequest request(QUrl("https://uberent.com/Launcher/ListStreams?Platform=" + m_platform));
			request.setRawHeader("X-Authorization", m_session_ticket.toAscii());
			request.setAttribute(QNetworkRequest::User, QVariant("streams"));
			m_network_access_manager->get(request);
		}
		else if(type == "streams")
		{
			QByteArray data = reply->readAll();
			qDebug() << data;
			m_patcher.decodeStreamsData(data);
			m_streams_combo_box->clear();
			m_streams_combo_box->addItems(m_patcher.streamNames());

			m_login_widget->setVisible(false);
			m_download_widget->setVisible(true);
		}
	}
	reply->deleteLater();
}

QString PAAlternativeLauncher::decodeLoginData(const QByteArray &data)
{
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

	QWidget::closeEvent(event);
}

void PAAlternativeLauncher::downloadPushButtonClicked(bool)
{
	m_patcher.downloadStream(m_streams_combo_box->currentText());
}

void PAAlternativeLauncher::patcherProgress(int percentage)
{
	if(m_progress_widget)
		m_progress_widget->setValue(percentage);
}

#include "PAAlternativeLauncher.moc"
