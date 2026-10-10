#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qthunt"));
    QApplication::setApplicationDisplayName(QStringLiteral("qthunt - UVa Hunting"));
    QApplication::setOrganizationName(QStringLiteral("qthunt"));

    MainWindow window;
    window.show();
    return app.exec();
}
