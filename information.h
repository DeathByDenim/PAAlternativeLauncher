#ifndef INFORMATION_H
#define INFORMATION_H

#include <QObject>
#include <QFile>

class QWidget;

class Information : public QObject
{
	Q_OBJECT

public:
	Information();
	~Information();

	void setParent(QWidget *parent);
	void setVerbose(bool verbose);
	void critical(const QString& title, const QString& text);
	void warning(const QString& title, const QString& text);
	void log(const QString& title, const QString& text, bool verbose = false);

private:
	QWidget *m_parent;
	QFile m_logfile;
	bool m_verbose;
};

extern Information info;

#endif // INFORMATION_H
