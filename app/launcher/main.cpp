#include "HubWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Labs Launcher"));
    app.setOrganizationName(QStringLiteral("Labs"));

    Labs::HubWindow w;
    w.show();
    return app.exec();
}
