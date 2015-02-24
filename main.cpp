#include <QApplication>
#include "paalternativelauncher.h"


int main(int argc, char** argv)
{
	QCoreApplication::setOrganizationName("DeathByDenim");
	QCoreApplication::setApplicationName("PAAlternativeLauncher");

	QApplication app(argc, argv);
	PAAlternativeLauncher foo;
	foo.show();
	return app.exec();
}
