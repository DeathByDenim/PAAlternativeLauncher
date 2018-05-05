#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

class QGroupBox;
class QLineEdit;
class QRadioButton;

class AdvancedDialog : public QDialog
{
	Q_OBJECT

public:
	enum optimus_t {nooptimus, optirun, primusrun};

	AdvancedDialog(const QString& extraparameters, const AdvancedDialog::optimus_t useoptimus, bool usesteamruntime, QWidget* parent = 0);
	~AdvancedDialog();

	QString parameters() {return m_parametersLineEdit->text();}
	optimus_t useOptimus();
	bool useSteamRuntime();

private:
	QString m_parameters;
	QLineEdit *m_parametersLineEdit;
	QGroupBox *m_optimusGroupBox;
	QRadioButton* m_nooptimusRadioButton;
	QRadioButton* m_optirunRadioButton;
	QRadioButton* m_primusrunRadioButton;
	QCheckBox* m_useSteamRuntimeCheckBox;
};

#endif // ADVANCEDDIALOG_H
