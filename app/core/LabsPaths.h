#pragma once

// Canonical filesystem locations shared by every Labs binary (engine,
// launcher, future Zen Coder). Resolved against `GenericDataLocation` so
// the path doesn't depend on the calling exe's `applicationName`. Without
// this, LabsEngine.exe and LabsHub.exe would land in different per-app
// AppData folders and the marketplace would install scripts the engine
// would never see.

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>

namespace Labs {
namespace Paths {

// All Labs user data lives under <AppData>/Labs. Independent of which exe
// asks. mkpath on first read so the directory is guaranteed to exist.
inline QString userDataRoot()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString dir = QDir::cleanPath(root + QStringLiteral("/Labs"));
    QDir().mkpath(dir);
    return dir;
}

// Where user-installed and user-authored .py scripts live. The engine's
// script combo scans this; the marketplace's install button writes to it.
// Drop a .py here manually and it shows up in the engine's combo on next
// refresh.
inline QString userScriptsDir()
{
    const QString dir = QDir::cleanPath(userDataRoot() + QStringLiteral("/scripts"));
    QDir().mkpath(dir);
    return dir;
}

// Sibling folder where the shared Python helper modules live (zp_core,
// xui.py, labs_input_bridge.py, etc.). Scripts in `userScriptsDir()`
// import from `../cv-scripts/...` so the structure is:
//   <userDataRoot>/scripts/      ← user .py files
//   <userDataRoot>/cv-scripts/   ← imports
inline QString cvHelpersDir()
{
    const QString dir = QDir::cleanPath(userDataRoot() + QStringLiteral("/cv-scripts"));
    QDir().mkpath(dir);
    return dir;
}

// Where the marketplace local manifest lives. Tracks scripts the user has
// published locally and which entries they've installed from remote.
inline QString marketplaceManifestPath()
{
    return userScriptsDir() + QStringLiteral("/marketplace.json");
}

} // namespace Paths
} // namespace Labs
