#ifndef PAAlternativeLauncher_H
#define PAAlternativeLauncher_H

#include <QMainWindow>
#include <QList>
#include "patcher.h"

class QProgressBar;
class QCheckBox;
class QLineEdit;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QShowEvent;
class QCloseEvent;
class QComboBox;

class PAAlternativeLauncher : public QMainWindow
{
Q_OBJECT
public:
	PAAlternativeLauncher();
	virtual ~PAAlternativeLauncher();

private:
	QProgressBar *m_progress_widget;
	QNetworkAccessManager *m_network_access_manager;
	QString m_session_ticket;
	QWidget *m_login_widget;
	QWidget *m_download_widget;
	QLabel *m_patch_text_label;
	QComboBox *m_streams_combo_box;
	QLineEdit *m_username_lineedit;
	QLineEdit *m_password_lineedit;
	QCheckBox *m_save_password_check_box;
	Patcher m_patcher;
	const QString m_platform;

	QWidget *createLoginWidget(QWidget* parent);
	QWidget *createDownloadWidget(QWidget* parent);
	QString decodeLoginData(const QByteArray& data);

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void installPushButtonPressed();
	void quitPushButtonPressed();
	void loginPushButtonClicked(bool);
	void downloadPushButtonClicked(bool);
	void replyReceived(QNetworkReply *reply);
	void patcherProgress(int percentage);
};

#endif // PAAlternativeLauncher_H
