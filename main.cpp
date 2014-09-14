#include <QtGui/QApplication>
#include "PAAlternativeLauncher.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    PAAlternativeLauncher foo;
    foo.show();
    return app.exec();
}
