#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <QDialog>
#include <QLineEdit>

class QGroupBox;
class QLineEdit;
class QRadioButton;

class AdvancedDialog : public QDialog
{
	Q_OBJECT
public:
	enum optimus_t {nooptimus, optirun, primusrun};

	AdvancedDialog(const QString& extraparameters, const AdvancedDialog::optimus_t useoptimus, QWidget* parent = 0);
	~AdvancedDialog();

	QString parameters() {return m_parametersLineEdit->text();}
	
	optimus_t useOptimus();

private:
	QString m_parameters;
	QLineEdit *m_parametersLineEdit;
	QGroupBox *m_optimusGroupBox;
    QRadioButton* m_nooptimusRadioButton;
    QRadioButton* m_optirunRadioButton;
    QRadioButton* m_primusrunRadioButton;
};

#endif // ADVANCEDDIALOG_H
