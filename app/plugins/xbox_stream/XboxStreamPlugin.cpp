#include "XboxStreamPlugin.h"
#include "XboxSignInDialog.h"
#include "XboxConsolePickerDialog.h"
#include "SettingsManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#ifndef XBOX_SIDECAR_DIR_DEV
#define XBOX_SIDECAR_DIR_DEV ""
#endif

namespace Labs {

namespace {

constexpr int LBSX_HEADER_LEN = 9;
constexpr quint8 T_EVENT_JSON = 0x01;
constexpr quint8 T_CMD_JSON   = 0x10;

inline quint32 readU32BE(const char* p) {
    return  (quint32)(quint8)p[3]
         | ((quint32)(quint8)p[2] << 8)
         | ((quint32)(quint8)p[1] << 16)
         | ((quint32)(quint8)p[0] << 24);
}

inline void writeU32BE(char* p, quint32 v) {
    p[0] = char((v >> 24) & 0xFF);
    p[1] = char((v >> 16) & 0xFF);
    p[2] = char((v >>  8) & 0xFF);
    p[3] = char( v        & 0xFF);
}

QString sanitizeAccountKey(const QString& k) {
    QString out;
    out.reserve(k.size());
    for (QChar c : k) {
        if (c.isLetterOrNumber() || c == '-' || c == '_' || c == '.') out.append(c);
        else out.append('_');
    }
    if (out.isEmpty()) out = QStringLiteral("default");
    return out;
}

QString sessionDataDir(const QString& accountKey) {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString dir  = base + QStringLiteral("/xbox/sessions/") + sanitizeAccountKey(accountKey);
    QDir().mkpath(dir);
    return dir;
}

QString sessionTokensFile(const QString& accountKey) {
    return sessionDataDir(accountKey) + QStringLiteral("/xbox-tokens.json");
}

} // namespace

// ── life-cycle ──────────────────────────────────────────────────────────────

XboxStreamPlugin::XboxStreamPlugin()  = default;
XboxStreamPlugin::~XboxStreamPlugin() = default;

void XboxStreamPlugin::initialize(const PluginContext& ctx)
{
    m_settings   = ctx.settings;
    m_sidecarDir = locateSidecarDir();
}

void XboxStreamPlugin::shutdown()
{
    for (auto& ptr : m_sessions) {
        if (ptr) {
            ptr->stop();
            ptr->deleteLater();
        }
    }
    m_sessions.clear();
}

// ── path helpers ────────────────────────────────────────────────────────────

QString XboxStreamPlugin::locateSidecarDir() const
{
    auto usable = [](const QString& d) {
        if (d.isEmpty()) return false;
        return QFileInfo::exists(d + QStringLiteral("/main.js"))
            && QFileInfo::exists(d + QStringLiteral("/node_modules/electron/dist/electron.exe"));
    };
    if (m_settings) {
        const QString override = m_settings->value(QStringLiteral("xbox/sidecarDir")).toString();
        if (usable(override)) return override;
    }
    const QString devPath = QString::fromUtf8(XBOX_SIDECAR_DIR_DEV);
    if (usable(devPath)) return devPath;
    const QString adjacent = QCoreApplication::applicationDirPath()
                             + QStringLiteral("/sidecars/xbox-stream");
    if (usable(adjacent)) return adjacent;
    return {};
}

QString XboxStreamPlugin::locateElectronExe(const QString& sidecarDir) const
{
    const QString p = sidecarDir + QStringLiteral("/node_modules/electron/dist/electron.exe");
    return QFileInfo::exists(p) ? p : QString();
}

// ── session management ─────────────────────────────────────────────────────

void XboxStreamPlugin::pruneTombstones()
{
    for (int i = m_sessions.size() - 1; i >= 0; --i) {
        if (!m_sessions[i]) m_sessions.removeAt(i);
    }
}

int XboxStreamPlugin::sessionCount() const
{
    int n = 0;
    for (const auto& p : m_sessions) if (p) ++n;
    return n;
}

QObject* XboxStreamPlugin::sessionAtObject(int index) const
{
    return static_cast<QObject*>(sessionAt(index));
}

QObject* XboxStreamPlugin::sessionByIdObject(int id) const
{
    return static_cast<QObject*>(sessionById(id));
}

XboxStreamSession* XboxStreamPlugin::sessionAt(int index) const
{
    int seen = 0;
    for (const auto& p : m_sessions) {
        if (!p) continue;
        if (seen == index) return p.data();
        ++seen;
    }
    return nullptr;
}

XboxStreamSession* XboxStreamPlugin::sessionById(int id) const
{
    for (const auto& p : m_sessions) {
        if (p && p->id() == id) return p.data();
    }
    return nullptr;
}

XboxStreamSession* XboxStreamPlugin::createSession(const QString& accountKey,
                                                   XboxStreamMode mode,
                                                   const QString& consoleId)
{
    pruneTombstones();
    if (sessionCount() >= kMaxSessions) {
        qWarning() << "[xbox-stream] session cap reached" << kMaxSessions;
        return nullptr;
    }
    if (m_sidecarDir.isEmpty()) m_sidecarDir = locateSidecarDir();
    if (m_sidecarDir.isEmpty()) {
        qWarning() << "[xbox-stream] no sidecar dir";
        return nullptr;
    }

    XboxSessionInfo info;
    info.id         = m_nextId++;
    info.accountKey = accountKey.isEmpty() ? QStringLiteral("default") : accountKey;
    info.mode       = mode;
    info.consoleId  = consoleId;
    info.label      = (mode == XboxStreamMode::Cloud)
        ? QStringLiteral("xCloud · %1").arg(info.accountKey)
        : QStringLiteral("Xbox RP · %1%2").arg(
              info.accountKey,
              consoleId.isEmpty() ? QString() : QStringLiteral(" / %1").arg(consoleId));

    auto* session = new XboxStreamSession(this, info, m_sidecarDir);
    connect(session, &XboxStreamSession::event,
            this, &XboxStreamPlugin::onSessionEvent);
    connect(session, &XboxStreamSession::finished, this,
            [this](int id, int /*code*/) {
                // Sidecar exited on its own — leave the session object in place
                // so consumers can inspect it; engine UI may call terminateSession.
                emit sessionEvent(id, QStringLiteral("finished"), {});
            });

    m_sessions.append(QPointer<XboxStreamSession>(session));
    emit sessionAdded(info.id);
    return session;
}

void XboxStreamPlugin::terminateSession(int id)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        auto& p = m_sessions[i];
        if (!p || p->id() != id) continue;
        p->stop();
        p->deleteLater();
        m_sessions.removeAt(i);
        emit sessionRemoved(id);
        return;
    }
}

void XboxStreamPlugin::onSessionEvent(int id, const QByteArray& payload)
{
    QJsonParseError err {};
    const auto doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QString topic = doc.object().value(QStringLiteral("type")).toString();
    emit sessionEvent(id, topic, payload);
}

// ── pair() — sign-in (always) + console picker (Home only) → createSession ─

bool XboxStreamPlugin::pair(QWidget* parent)
{
    if (m_sidecarDir.isEmpty()) m_sidecarDir = locateSidecarDir();
    if (m_sidecarDir.isEmpty()) {
        qWarning() << "[xbox-stream] cannot pair — sidecar dir missing";
        return false;
    }
    const QString electronExe = locateElectronExe(m_sidecarDir);
    if (electronExe.isEmpty()) {
        qWarning() << "[xbox-stream] cannot pair — electron missing in" << m_sidecarDir;
        return false;
    }

    // Mode selector — read from settings, default Home. The engine UI sets
    // xbox/mode before calling pair() based on which session card the user
    // clicked. Cloud and Home need different flows.
    const QString modeStr = m_settings
        ? m_settings->value(QStringLiteral("xbox/mode"), QStringLiteral("home")).toString()
        : QStringLiteral("home");
    const XboxStreamMode mode = (modeStr == QStringLiteral("cloud"))
                                 ? XboxStreamMode::Cloud
                                 : XboxStreamMode::Home;

    // Suggest the next free acctN slot but let the user name it whatever they
    // want. The label scopes the per-account profile/cookies on disk.
    QString suggested;
    for (int i = 1; i <= kMaxSessions; ++i) {
        const QString candidate = QStringLiteral("acct%1").arg(i);
        bool taken = false;
        for (const auto& s : m_sessions) {
            if (s && s->info().accountKey == candidate) { taken = true; break; }
        }
        if (!taken) { suggested = candidate; break; }
    }
    if (suggested.isEmpty()) suggested = QStringLiteral("acct1");

    bool ok = false;
    const QString accountKey = QInputDialog::getText(
        parent,
        mode == XboxStreamMode::Cloud
            ? QStringLiteral("New xCloud session")
            : QStringLiteral("New Xbox Remote Play session"),
        QStringLiteral("Name this session (used to scope the per-account profile):"),
        QLineEdit::Normal,
        suggested,
        &ok).trimmed();
    if (!ok) return false;
    const QString safeKey = accountKey.isEmpty() ? suggested : accountKey;

    // Cloud: skip the device-code sign-in dialog entirely. The sidecar's
    // visible BrowserWindow at xbox.com/play handles auth itself. Spawn the
    // session AND auto-start it so the browser pops immediately.
    if (mode == XboxStreamMode::Cloud) {
        XboxStreamSession* sess = createSession(safeKey, mode, QString());
        if (!sess) return false;
        sess->start();
        return true;
    }

    // Home (Remote Play): existing device-code sign-in + console picker flow.
    const QString tokensFile = sessionTokensFile(safeKey);
    XboxSignInDialog dlg(electronExe, m_sidecarDir, tokensFile, parent);
    if (dlg.exec() != QDialog::Accepted) return false;

    QString consoleId, consoleName;
    if (!pickConsoleForAccount(parent, safeKey, &consoleId, &consoleName))
        return false;

    XboxStreamSession* sess = createSession(safeKey, mode, consoleId);
    return sess != nullptr;
}

bool XboxStreamPlugin::pickConsoleForAccount(QWidget* parent,
                                             const QString& accountKey,
                                             QString* outConsoleId,
                                             QString* outConsoleName)
{
    const QString electronExe = locateElectronExe(m_sidecarDir);
    if (m_sidecarDir.isEmpty() || electronExe.isEmpty()) return false;

    auto* probe = new QProcess(this);
    probe->setWorkingDirectory(m_sidecarDir);
    probe->setProgram(electronExe);
    probe->setArguments({
        QStringLiteral("."),
        QStringLiteral("--data-dir=%1").arg(sessionDataDir(accountKey)),
        QStringLiteral("--tokens-file=%1").arg(sessionTokensFile(accountKey)),
        QStringLiteral("--mode=home"),
    });
    probe->setProcessChannelMode(QProcess::SeparateChannels);

    auto* dlg = new XboxConsolePickerDialog(parent);
    QPointer<XboxConsolePickerDialog> dlgGuard(dlg);

    QByteArray stdoutAccum;

    auto onStdout = [probe, &stdoutAccum, dlgGuard]() {
        stdoutAccum.append(probe->readAllStandardOutput());
        for (;;) {
            if (stdoutAccum.size() < LBSX_HEADER_LEN) return;
            const char* p = stdoutAccum.constData();
            if (!(p[0] == 'L' && p[1] == 'B' && p[2] == 'S' && p[3] == 'X')) {
                int idx = stdoutAccum.indexOf(QByteArray("LBSX", 4), 1);
                if (idx < 0) { stdoutAccum.clear(); return; }
                stdoutAccum.remove(0, idx);
                continue;
            }
            const quint8  type = quint8(p[4]);
            const quint32 len  = readU32BE(p + 5);
            if (len > 16u * 1024u * 1024u) { stdoutAccum.remove(0, LBSX_HEADER_LEN); continue; }
            if (quint32(stdoutAccum.size()) < LBSX_HEADER_LEN + len) return;
            QByteArray payload(p + LBSX_HEADER_LEN, int(len));
            stdoutAccum.remove(0, LBSX_HEADER_LEN + int(len));

            if (type == T_EVENT_JSON) {
                QJsonParseError err {};
                const auto doc = QJsonDocument::fromJson(payload, &err);
                if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
                const QJsonObject o = doc.object();
                const QString t = o.value(QStringLiteral("type")).toString();
                if (t == QStringLiteral("console_list") && dlgGuard) {
                    QVector<XboxConsoleEntry> consoles;
                    for (const auto& v : o.value(QStringLiteral("consoles")).toArray()) {
                        const auto co = v.toObject();
                        XboxConsoleEntry e;
                        e.id    = co.value(QStringLiteral("id")).toString();
                        e.name  = co.value(QStringLiteral("name")).toString();
                        e.state = co.value(QStringLiteral("state")).toString();
                        e.type  = co.value(QStringLiteral("type")).toString();
                        if (!e.id.isEmpty()) consoles.push_back(e);
                    }
                    dlgGuard->setConsoles(consoles);
                } else if (t == QStringLiteral("error") && dlgGuard) {
                    dlgGuard->setStatus(o.value(QStringLiteral("message")).toString(), true);
                }
            }
        }
    };
    auto onStderr = [probe]() {
        const QByteArray e = probe->readAllStandardError();
        for (const auto& l : e.split('\n')) {
            const auto t = QByteArray(l).trimmed();
            if (!t.isEmpty()) qInfo().noquote() << "[xbox-sidecar:probe]" << t;
        }
    };
    connect(probe, &QProcess::readyReadStandardOutput, this, onStdout);
    connect(probe, &QProcess::readyReadStandardError,  this, onStderr);

    probe->start();
    if (!probe->waitForStarted(5000)) {
        qWarning() << "[xbox-stream] probe failed to start";
        probe->deleteLater();
        return false;
    }

    QTimer::singleShot(800, this, [probe]() {
        if (probe->state() != QProcess::Running) return;
        const QByteArray cmd = QJsonDocument(QJsonObject{
            { QStringLiteral("cmd"), QStringLiteral("list_consoles") }
        }).toJson(QJsonDocument::Compact);
        QByteArray frame;
        frame.resize(LBSX_HEADER_LEN);
        frame[0] = 'L'; frame[1] = 'B'; frame[2] = 'S'; frame[3] = 'X';
        frame[4] = char(T_CMD_JSON);
        writeU32BE(frame.data() + 5, quint32(cmd.size()));
        frame.append(cmd);
        probe->write(frame);
    });

    const int rc = dlg->exec();

    if (probe->state() == QProcess::Running) {
        const QByteArray stop = QJsonDocument(QJsonObject{
            { QStringLiteral("cmd"), QStringLiteral("stop") }
        }).toJson(QJsonDocument::Compact);
        QByteArray frame;
        frame.resize(LBSX_HEADER_LEN);
        frame[0] = 'L'; frame[1] = 'B'; frame[2] = 'S'; frame[3] = 'X';
        frame[4] = char(T_CMD_JSON);
        writeU32BE(frame.data() + 5, quint32(stop.size()));
        frame.append(stop);
        probe->write(frame);
        if (!probe->waitForFinished(1500)) probe->kill();
    }
    probe->deleteLater();

    if (rc != QDialog::Accepted) return false;
    if (outConsoleId)   *outConsoleId   = dlg->selectedConsoleId();
    if (outConsoleName) *outConsoleName = dlg->selectedConsoleName();
    return true;
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::XboxStreamPlugin();
}
