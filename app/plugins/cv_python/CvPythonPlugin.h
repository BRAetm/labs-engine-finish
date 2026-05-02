#pragma once

#include "IPlugin.h"
#include "ShmBus.h"

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <memory>
#include <unordered_map>

namespace Labs {

class SettingsManager;

class CvPythonPlugin : public QObject,
                      public IFrameSinkPlugin,
                      public IControllerSourcePlugin,
                      public IFrameSink,
                      public IControllerSource {
    Q_OBJECT
public:
    CvPythonPlugin();
    ~CvPythonPlugin() override;

    QString name()        const override { return QStringLiteral("CV Python"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Bridges frames ↔ Python script ↔ virtual pad"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;

    QObject* qobject() override { return this; }

    // IFrameSinkPlugin
    IFrameSink* frameSink() override { return this; }
    // IControllerSourcePlugin
    IControllerSource* controllerSource() override { return this; }

    // IFrameSink
    void pushFrame(const Frame& frame) override;

    // IControllerSource
    void setSink(IControllerSink* sink) override;
    bool start() override;
    void stop()  override;
    bool isRunning() const override { return m_running; }

public slots:
    // Called by the engine when a session ends (e.g. xbox_stream's
    // sessionRemoved signal). Closes that session's SHM writer so the
    // block name is freed and a future session reusing the id won't
    // inherit a stale event handle.
    void closeSession(int sessionId);

private:
    void stopPython();

    SettingsManager* m_settings = nullptr;
    // Per-session SHM frame writers, keyed by Frame.sessionId. Opened lazily
    // on first frame seen for each session; closed on closeSession or
    // shutdown. Guarded by m_writersMx because pushFrame can be called from
    // multiple capture threads (each xbox_stream session decodes on its own
    // thread, plus PS Remote Play's chiaki decode thread).
    QMutex m_writersMx;
    // std::unordered_map (not QHash) because QHash requires copyable values
    // and FrameShmWriter is held by unique_ptr (HANDLE ownership is move-only).
    std::unordered_map<int, std::unique_ptr<FrameShmWriter>> m_frameShmBySession;
    std::unique_ptr<GamepadShmReader> m_gamepadReader;
    QPointer<QProcess> m_process;
    IControllerSink* m_ctrlSink = nullptr;
    bool   m_running   = false;
    bool   m_loggedFirstFrame = false;
};

} // namespace Labs
