#pragma once

#include "FrameTypes.h"
#include "InputTypes.h"
#include "IPlugin.h"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMutex>
#include <atomic>
#include <memory>

class QProcess;

namespace Labs {

enum class XboxStreamMode { Home, Cloud };

struct XboxSessionInfo {
    int            id = -1;             // 0..7, monotonic
    QString        accountKey;          // scopes the token-cache directory
    XboxStreamMode mode = XboxStreamMode::Home;
    QString        consoleId;           // home only
    QString        label;               // human-readable
};

class XboxStreamSession : public QObject,
                          public IFrameSource,
                          public IControllerSink {
    Q_OBJECT
public:
    XboxStreamSession(QObject* parent, XboxSessionInfo info, QString sidecarDir);
    ~XboxStreamSession() override;

    // Q_INVOKABLE so the engine (which doesn't link the plugin DLL) can read
    // the session id via QMetaObject::invokeMethod across the DLL boundary.
    Q_INVOKABLE int id() const { return m_info.id; }
    const XboxSessionInfo& info() const { return m_info; }

    // IFrameSource
    void    setSink(IFrameSink* sink) override;
    bool    start() override;
    void    stop() override;
    bool    isRunning() const override { return m_running.load(); }
    qint64  frameCount() const override { return m_frameCount.load(); }
    QString targetLabel() const override { return m_info.label; }

    // IControllerSink — forwarded as gamepad JSON over the LBSX pipe.
    void pushState(const ControllerState& state) override;

    QString dataDir() const;
    QString tokensFile() const;

    // Last reported native window handle of the sidecar's BrowserWindow
    // (cloud mode only). 0 if not reported yet. Q_INVOKABLE so the engine
    // can read it across the plugin DLL boundary.
    Q_INVOKABLE quintptr windowHwnd() const;

signals:
    void event(int sessionId, const QByteArray& jsonPayload);
    void finished(int sessionId, int exitCode);
    void videoSize(int sessionId, int w, int h);
    // Cloud mode: sidecar reported its visible BrowserWindow HWND. The engine
    // listens for this and reparents the window into a Qt host widget so the
    // browser appears inside the Labs Engine window.
    void windowHwndReady(int sessionId, quintptr hwnd);

private slots:
    void onStdoutReady();
    void onStderrReady();
    void onProcessFinished(int exitCode);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    XboxSessionInfo       m_info;
    QString               m_sidecarDir;
    IFrameSink*           m_sink = nullptr;
    std::atomic<bool>     m_running    { false };
    std::atomic<qint64>   m_frameCount { 0 };
    QMutex                m_writeMx;
    quintptr              m_windowHwnd { 0 };  // cloud mode only; 0 until reported

    QString locateElectronExe() const;
    void    sendFrame(quint8 type, const QByteArray& payload);
    void    sendCommand(const QByteArray& jsonCompact);
    void    handleEventJson(const QByteArray& payload);
    bool    feedNal(const uint8_t* data, int size);
    bool    initFfmpeg();
    void    finiFfmpeg();
};

} // namespace Labs
