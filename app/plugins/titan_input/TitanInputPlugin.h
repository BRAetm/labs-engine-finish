#pragma once

#include "IPlugin.h"
#include "InputTypes.h"

#include <QObject>
#include <QString>
#include <atomic>
#include <memory>

namespace Labs {

// Reads a Titan Two (Collective Minds, VID 0x1993 PID 0x00F0) over Windows
// HID and exposes it as an IControllerSource — same lane as DualSensePlugin.
//
// Default Titan Two firmware emulates a Sony DualShock 4 on the gamepad
// collection.  We decode the report using the DS4 byte layout already proven
// out in DualSensePlugin.  Other emulation modes (X360, Switch Pro) need
// their own decode paths and aren't wired yet — track them via the user's
// configured T2 output mode if/when we surface that.
//
// Out of scope for this plugin (deliberately):
//   - Slot select / GPC3 bytecode upload — the protocol shipped in the
//     reference repo (feature reports 0x30/0x31/0x32) does NOT match the
//     real Gtuner IV protocol.  Programming firmware needs proper protocol
//     reverse-engineering against a physical T2 with a USB analyser.  See
//     TODO comments in the .cpp.
//   - GPC3 VM execution — real GPC3 runs on T2 firmware, not a host VM.
class TitanInputSource : public QObject, public IControllerSource {
    Q_OBJECT
public:
    explicit TitanInputSource(QObject* parent = nullptr);
    ~TitanInputSource() override;

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

class TitanInputPlugin : public QObject, public IControllerSourcePlugin {
    Q_OBJECT
public:
    TitanInputPlugin();
    ~TitanInputPlugin() override;

    QString  name()        const override { return QStringLiteral("Titan Input"); }
    QString  author()      const override { return QStringLiteral("Labs"); }
    QString  description() const override { return QStringLiteral("Reads Titan Two over HID as a ControllerState source."); }
    QString  version()     const override { return QStringLiteral("0.1.0"); }
    QObject* qobject() override           { return this; }

    void initialize(const PluginContext&) override {}
    void shutdown() override { if (m_source) m_source->stop(); }

    IControllerSource* controllerSource() override { return m_source.get(); }

private:
    std::unique_ptr<TitanInputSource> m_source;
};

} // namespace Labs
