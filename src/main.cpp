#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("hunt"));
    QApplication::setApplicationDisplayName(QStringLiteral("hunt - UVa Hunting"));
    QApplication::setOrganizationName(QStringLiteral("hunt"));

    MainWindow window;
    window.show();
    return app.exec();
}
