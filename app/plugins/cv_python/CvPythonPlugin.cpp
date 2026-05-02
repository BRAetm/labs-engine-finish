#include "CvPythonPlugin.h"
#include "SettingsManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include <Windows.h>

namespace Labs {

CvPythonPlugin::CvPythonPlugin()
    : m_gamepadReader(std::make_unique<GamepadShmReader>(this))
{
}

CvPythonPlugin::~CvPythonPlugin()
{
    stop();
}

void CvPythonPlugin::initialize(const PluginContext& ctx)
{
    m_settings = ctx.settings;
    // No more cv/sessionId global — each Frame carries its own sessionId,
    // stamped by the source plugin (PS Remote Play=1, xbox_stream=100..107).
    // The settings key is intentionally not read so a stale value doesn't
    // get re-saved during this run.
}

void CvPythonPlugin::shutdown()
{
    stop();
}

// ── IFrameSink ──────────────────────────────────────────────────────────────

void CvPythonPlugin::pushFrame(const Frame& frame)
{
    if (!m_running) return;

    const int sid = frame.sessionId;
    FrameShmWriter* writer = nullptr;
    {
        QMutexLocker lock(&m_writersMx);
        auto it = m_frameShmBySession.find(sid);
        if (it == m_frameShmBySession.end()) {
            // First frame for this session — open a per-session SHM block at
            // Labs_<pid>_Frame_<sid>. Done under the lock so two concurrent
            // first-frame pushes (e.g. PS + xbox session 0 starting at the
            // same instant) don't both try to open the same block.
            auto w = std::make_unique<FrameShmWriter>();
            const quint32 pid = ::GetCurrentProcessId();
            if (!w->open(pid, sid)) {
                qWarning() << "CvPython: frame SHM open failed for session" << sid;
                return;
            }
            qInfo() << "CvPython: opened SHM for session" << sid
                    << "block=" << w->blockName()
                    << "pid="   << pid;
            auto inserted = m_frameShmBySession.emplace(sid, std::move(w));
            it = inserted.first;
        }
        writer = it->second.get();
    }
    writer->write(frame);

    if (!m_loggedFirstFrame) {
        qInfo() << "CvPython: first frame published"
                << frame.width << "x" << frame.height
                << "session=" << sid;
        m_loggedFirstFrame = true;
    }
}

void CvPythonPlugin::closeSession(int sessionId)
{
    QMutexLocker lock(&m_writersMx);
    auto it = m_frameShmBySession.find(sessionId);
    if (it == m_frameShmBySession.end()) return;
    qInfo() << "CvPython: closing SHM for session" << sessionId
            << "block=" << it->second->blockName();
    it->second->close();
    m_frameShmBySession.erase(it);
}

// ── IControllerSource ───────────────────────────────────────────────────────

void CvPythonPlugin::setSink(IControllerSink* sink)
{
    m_ctrlSink = sink;
    if (m_gamepadReader) m_gamepadReader->setSink(sink);
}

bool CvPythonPlugin::start()
{
    if (m_running) return true;

    // No SHM open here anymore — frames open their own block lazily on first
    // push (per-session). All we need at startup is the gamepad reader so
    // any already-running script can drive the controller.
    const quint32 pid = ::GetCurrentProcessId();
    m_gamepadReader->configure(pid);
    m_gamepadReader->start();

    m_running = true;
    return true;
}

void CvPythonPlugin::stop()
{
    if (!m_running) return;
    m_running = false;

    stopPython();

    if (m_gamepadReader) {
        m_gamepadReader->requestStop();
        m_gamepadReader->wait(500);
    }

    // Close every per-session SHM writer we ever opened.
    QMutexLocker lock(&m_writersMx);
    for (auto& [sid, writer] : m_frameShmBySession) {
        if (writer) writer->close();
    }
    m_frameShmBySession.clear();
}

// ── Python subprocess ───────────────────────────────────────────────────────

void CvPythonPlugin::stopPython()
{
    if (m_process) {
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) m_process->kill();
        m_process.clear();
    }
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::CvPythonPlugin();
}
