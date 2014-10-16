#ifndef PAAlternativeLauncher_H
#define PAAlternativeLauncher_H

#include <QMainWindow>
#include <QList>
#include "patcher.h"
#include "advanceddialog.h"

class QShowEvent;
class QTextBrowser;
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
	QProgressBar *m_patch_progressbar;
	QLabel *m_patch_label;
	QNetworkAccessManager *m_network_access_manager;
	QString m_session_ticket;
	QWidget *m_login_widget;
	QWidget *m_download_widget;
	QWidget *m_wait_widget;
	QTextBrowser *m_patch_text_browser;
	QComboBox *m_streams_combo_box;
	QLineEdit *m_username_lineedit;
	QLineEdit *m_password_lineedit;
	Patcher m_patcher;
	const QString m_platform;
    QLineEdit* m_installPathLineEdit;
	QString m_extraParameters;
	QMap<QString,QString> m_stream_news;
	QMap<QString,bool> m_requires_update;
	AdvancedDialog::optimus_t m_use_optimus;
    QLabel* m_update_available_label;
    QPushButton* m_download_button;

	QWidget *createLoginWidget(QWidget* parent);
	QWidget *createDownloadWidget(QWidget* parent);
	QWidget *createWaitWidget(QWidget *parent);
	QString decodeLoginData(const QByteArray& data);
    void checkForUpdates(QStringList streamnames);

protected:
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);

private slots:
	void loginPushButtonClicked(bool);
	void downloadPushButtonClicked(bool);
	void installPathButtonClicked(bool);
	void launchPushButtonClicked(bool);
	void advancedPushButtonClicked(bool);
	void replyReceived(QNetworkReply *reply);
	void patcherProgress(int percentage);
	void patcherState(QString state);
	void streamsCurrentIndexChanged(QString streamname);
    void lineEditReturnPressed();
};

#endif // PAAlternativeLauncher_H
