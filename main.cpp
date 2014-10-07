#include <QtGui/QApplication>
#include "PAAlternativeLauncher.h"
#include "information.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	
	if(app.arguments().contains("-v"))
		info.setVerbose(true);
	
	PAAlternativeLauncher foo;
	foo.show();
	return app.exec();
}
