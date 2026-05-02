#pragma once

#include "IPlugin.h"

#include <QObject>
#include <QMutex>
#include <atomic>
#include <array>
#include <memory>

namespace Labs {

class ViGEmPlugin;

class ViGEmSink : public QObject, public IControllerSink {
    Q_OBJECT
public:
    explicit ViGEmSink(ViGEmPlugin* owner, QObject* parent = nullptr);
    ~ViGEmSink() override;

    bool isReady() const { return m_ready.load(); }

    // 0..3 once the bus has assigned a slot, -1 otherwise. The bus typically
    // assigns asynchronously, so we retry on first read for a short window.
    int userIndex();

    // Single-pad legacy path: lazily allocates slot 0 (X360) and pushes there.
    void pushState(const ControllerState& state) override;

private:
    ViGEmPlugin*      m_owner = nullptr;
    std::atomic<bool> m_ready { false };
    std::atomic<int>  m_legacySlot { -1 };
};

class ViGEmPlugin : public QObject, public IControllerSinkPlugin {
    Q_OBJECT
public:
    ViGEmPlugin();
    ~ViGEmPlugin() override;

    QString name()        const override { return QStringLiteral("ViGEm Output"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Virtual Xbox 360 / DualShock 4 pads via ViGEmBus (pool of 8)"); }
    QString version()     const override { return QStringLiteral("0.2.0"); }

    void initialize(const PluginContext&) override {}
    void shutdown() override;

    QObject* qobject() override { return this; }
    IControllerSink* controllerSink() override { return m_sink.get(); }

    // Cross-DLL accessor. LabsMainWindow uses this to set XInput's skip mask
    // so we don't read back the slot we just wrote (feedback loop).
    Q_INVOKABLE int userIndex() { return m_sink ? m_sink->userIndex() : -1; }

    QString statusText() const { return m_status; }
    bool clientReady() const { return m_clientReady.load(); }

    // Pool API for multi-session use.
    static constexpr int kMaxX360 = 4;
    static constexpr int kMaxDS4  = 4;
    static constexpr int kMaxPads = kMaxX360 + kMaxDS4;  // 8

    enum class PadKind { X360, DS4 };

    // Allocate a pad. Returns padId (0..7) on success, -1 if pool exhausted.
    Q_INVOKABLE int  allocatePad(int kindAsInt /*0=X360, 1=DS4*/);

    // Release a pad and disconnect its ViGEm target. Idempotent.
    Q_INVOKABLE void releasePad(int padId);

    // Push state to a specific allocated pad. Returns false if padId not allocated
    // or if the target reports not-plugged (in which case the slot is auto-freed).
    Q_INVOKABLE bool pushStateTo(int padId, const Labs::ControllerState& state);

    // For X360 pads (padId 0..3): the user-index (0..3) the bus assigned.
    // For DS4 pads (padId 4..7) or unallocated: -1.
    Q_INVOKABLE int  xinputIndexOf(int padId) const;
    Q_INVOKABLE bool isAllocated(int padId) const;

private:
    friend class ViGEmSink;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::unique_ptr<ViGEmSink> m_sink;
    QString               m_status;
    std::atomic<bool>     m_clientReady { false };
};

} // namespace Labs
