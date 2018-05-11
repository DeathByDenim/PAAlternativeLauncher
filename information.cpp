#include <QMessageBox>
#include <QDir>
#include "information.h"

Information::Information()
 : m_parent(NULL)
{
	m_logfile.setFileName(QDir::tempPath() + "/PAAlternativeLauncher.log");
	if(!m_logfile.open(QIODevice::WriteOnly))
		QMessageBox::warning(NULL, "Log error", "Couldn't open logfile for writing");
}

Information::~Information()
{
	m_logfile.close();
}

void Information::setParent(QWidget* parent)
{
	m_parent = parent;
}

void Information::setVerbose(bool verbose)
{
	m_verbose = verbose;
}

void Information::critical(const QString& title, const QString& text)
{
	mMutex.lock();
	QString errortext = text;
	m_logfile.write("[ERROR] (", 9);
	m_logfile.write(title.toUtf8());
	m_logfile.write(") ", 2);
	m_logfile.write(errortext.replace('\n', ' ').toUtf8());
	m_logfile.write("\n", 1);
	m_logfile.flush();
	QMessageBox::critical(m_parent, title, text);
	mMutex.unlock();
}

bool Information::warning(const QString& title, const QString& text, bool yesno)
{
	mMutex.lock();
	m_logfile.write("[WARN]  (", 9);
	m_logfile.write(title.toUtf8());
	m_logfile.write(") ", 2);
	m_logfile.write(text.toUtf8());
	m_logfile.write("\n", 1);
	m_logfile.flush();

	bool yes = true;
	if(yesno)
		yes = (QMessageBox::warning(m_parent, title, text, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
	else
		QMessageBox::warning(m_parent, title, text);

	mMutex.unlock();

	return yes;
}

void Information::log(const QString& title, const QString& text, bool verbose)
{
	if(verbose && !m_verbose)
		return;

	mMutex.lock();
	m_logfile.write("[LOG]   (", 9);
	m_logfile.write(title.toUtf8());
	m_logfile.write(") ", 2);
	m_logfile.write(text.toUtf8());
	m_logfile.write("\n", 1);
	m_logfile.flush();
	mMutex.unlock();
}

Information info;

#include "information.moc"
