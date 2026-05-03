#pragma once

#include "../../core/IPlugin.h"
#include "../../core/ICaptureProvider.h"
#include "ProcessMonitor.h"
#include "SmartCaptureProvider.h"

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>

namespace Labs {

class SmartCapturePlugin : public QObject,
                            public ICaptureProviderPlugin,
                            public IFrameSourcePlugin,
                            public IFrameSource {
    Q_OBJECT
public:
    SmartCapturePlugin();
    ~SmartCapturePlugin() override;

    QString name()        const override { return QStringLiteral("SmartCapture"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("DXGI desktop duplication capture with process tracking"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;
    QObject* qobject() override { return this; }

    // ICaptureProviderPlugin
    ICaptureProvider* captureProvider() override { return &m_provider; }

    // IFrameSourcePlugin
    IFrameSource* frameSource() override { return this; }

    // IFrameSource
    void    setSink(IFrameSink* sink) override;
    bool    start() override;
    void    stop() override;
    bool    isRunning() const override;
    qint64  frameCount() const override;

private slots:
    void onTick();

private:
    void ensureOpen(HWND hwnd);

    ProcessMonitor     m_monitor;
    SmartCaptureProvider m_provider;
    IFrameSink*        m_sink      = nullptr;
    QTimer*            m_timer     = nullptr;
    std::atomic<bool>  m_running   { false };
    std::atomic<qint64>m_frameCount{ 0 };

    QString            m_pattern;
    int                m_fps;
    bool               m_hwAccel;

    HWND               m_lastHwnd  = nullptr;
};

} // namespace Labs
