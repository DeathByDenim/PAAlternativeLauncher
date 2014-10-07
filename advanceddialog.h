#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <QDialog>
#include <QLineEdit>

class QCheckBox;
class QLineEdit;
class AdvancedDialog : public QDialog
{
	Q_OBJECT
public:
	AdvancedDialog(const QString &extraparameters, const bool useoptirun, QWidget *parent = 0);
	~AdvancedDialog();

	QString parameters() {return m_parametersLineEdit->text();}
	bool useOptirun();

private:
	QString m_parameters;
	QLineEdit *m_parametersLineEdit;
	QCheckBox *m_optirunCheckBox;
};

#endif // ADVANCEDDIALOG_H
