#pragma once

#include "IPlugin.h"
#include "InputTypes.h"

#include <QObject>
#include <QString>
#include <atomic>
#include <memory>

namespace Labs {

// Reads Sony DualSense / DualShock 4 controllers via Windows HID and pushes
// them straight into the engine's ControllerState fan-out. Requires no
// ViGEmBus — input bypasses XInput entirely and flows directly into whichever
// sink the main window has wired (PS Remote Play protocol in PS mode, ViGEm
// Out in Xbox mode).
class DualSenseSource : public QObject, public IControllerSource {
    Q_OBJECT
public:
    explicit DualSenseSource(QObject* parent = nullptr);
    ~DualSenseSource() override;

    void setSink(IControllerSink* sink) override { m_sink = sink; }
    bool start() override;
    void stop()  override;
    bool isRunning() const override { return m_running.load(); }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    IControllerSink*      m_sink = nullptr;
    std::atomic<bool>     m_running { false };

    friend struct Impl;
};

class DualSensePlugin : public QObject, public IControllerSourcePlugin {
    Q_OBJECT
public:
    DualSensePlugin();
    ~DualSensePlugin() override;

    QString  name()        const override { return QStringLiteral("DualSense"); }
    QString  author()      const override { return QStringLiteral("TM Labs"); }
    QString  description() const override { return QStringLiteral("Reads PlayStation controllers over HID as a ControllerState source."); }
    QString  version()     const override { return QStringLiteral("2.0.0"); }
    QObject* qobject() override           { return this; }

    void initialize(const PluginContext&) override {}
    void shutdown() override { if (m_source) m_source->stop(); }

    IControllerSource* controllerSource() override { return m_source.get(); }

private:
    std::unique_ptr<DualSenseSource> m_source;
};

} // namespace Labs
