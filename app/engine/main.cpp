#include "LabsMainWindow.h"
#include "LabsTheme.h"
#include "SettingsManager.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QSharedMemory>
#include <QStandardPaths>
#include <QTextStream>
#include <QDir>

#include <Windows.h>
#include <csignal>
#include <cstdio>

static QFile* gLogFile = nullptr;

static void labsMsgHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    const char* prefix = "I";
    switch (type) {
        case QtDebugMsg:    prefix = "D"; break;
        case QtWarningMsg:  prefix = "W"; break;
        case QtCriticalMsg: prefix = "E"; break;
        case QtFatalMsg:    prefix = "F"; break;
        case QtInfoMsg:     prefix = "I"; break;
    }
    const QString line = QStringLiteral("[%1] %2").arg(prefix, msg);
    if (gLogFile) {
        QTextStream(gLogFile) << line << '\n';
        gLogFile->flush();
    }
    ::OutputDebugStringW(reinterpret_cast<LPCWSTR>(line.utf16()));
    fwprintf(stderr, L"%s\n", reinterpret_cast<LPCWSTR>(line.utf16()));
    fflush(stderr);
    if (type == QtFatalMsg) std::abort();
}

int main(int argc, char* argv[])
{
    // Install abort/crash breadcrumb before anything else can fail.
    std::signal(SIGABRT, [](int) {
        const char* m = "[F] SIGABRT — see log above for the last qWarning/qInfo\n";
        fwrite(m, 1, strlen(m), stderr);
        fflush(stderr);
    });

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Labs Engine"));
    app.setOrganizationName(QStringLiteral("Labs"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // QSharedMemory MUST be constructed AFTER QApplication. Per Qt docs it
    // depends on the QCoreApplication event loop and instance(); a file-scope
    // static would construct before main()'s QApplication and is UB.
    QSharedMemory guard(QStringLiteral("LabsEngine_SingleInstance"));
    const bool singleInstance = guard.create(1);
    if (!singleInstance) {
        // Another instance owns the segment — attach so we don't leave a
        // stale handle, then bail.
        guard.attach();
    }

    // App icon — resource-embedded .ico. Flows to taskbar, window title bars,
    // and every child dialog (QDialog inherits the app icon by default).
    const QIcon appIcon(QStringLiteral(":/labs/labs.ico"));
    app.setWindowIcon(appIcon);

    // Load the saved theme (if any) before creating widgets.
    Labs::SettingsManager bootstrapSettings;
    app.setStyleSheet(Labs::labsStylesheet(Labs::labsThemeLoad(&bootstrapSettings)));

    // Open a log file at %APPDATA%/Labs/labs.log and install a Qt message handler
    // that tees to the file, OutputDebugString, and stderr so every qWarning /
    // qInfo / qDebug is captured regardless of how the exe was launched.
    const QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    gLogFile = new QFile(logDir + QStringLiteral("/labs.log"));
    gLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    qInstallMessageHandler(labsMsgHandler);
    qInfo() << "Labs Engine starting; log ->" << gLogFile->fileName();

    if (!singleInstance) {
        qWarning() << "another instance is already running";
        // Tear down the message handler + log file before bailing so we
        // don't leak the QFile* through the early return.
        qInstallMessageHandler(nullptr);
        if (gLogFile) {
            gLogFile->close();
            delete gLogFile;
            gLogFile = nullptr;
        }
        return 1;
    }

    qInfo() << "constructing main window...";
    Labs::LabsMainWindow window;
    qInfo() << "main window constructed; showing";
    window.show();
    const int appResult = app.exec();

    // Uninstall the message handler BEFORE we delete gLogFile, otherwise a
    // late shutdown qDebug/qWarning would reach a dangling QFile*. Then
    // close + delete the log file. Order matters here.
    qInstallMessageHandler(nullptr);
    if (gLogFile) {
        gLogFile->close();
        delete gLogFile;
        gLogFile = nullptr;
    }
    return appResult;
}
