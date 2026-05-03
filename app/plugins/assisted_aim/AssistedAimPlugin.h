#pragma once

#include "IPlugin.h"
#include "IFrameProcessor.h"
#include "InputTypes.h"

#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>

namespace Labs {

class SettingsManager;

// Accessibility-focused aim assist. Two halves wired to the engine:
//   1) IInferenceResultsSink — caches the on-screen target nearest the
//      crosshair (screen center) every time a new InferenceResults lands.
//   2) IControllerStateFilter — runs in-flight on the controller-source
//      thread, between InputOverride and the ViGEm output. While the
//      configured activation button is held AND a recent target exists,
//      it nudges the right thumb stick toward that target by a capped,
//      time-decaying amount.
//
// Hard rules baked in (this is assistance, not automation):
//   - No button held = filter is a no-op. Zero authored input.
//   - Per-press budget: strength starts at 1.0 on the press edge and
//     decays additively by `decay_rate` each frame. Once depleted the
//     player must release+repress to get more help.
//   - Correction magnitude clamped to `max_correction` of full stick.
//   - Stale targets (>250 ms since last detection) are ignored.
//
// Settings (under "assisted_aim/"):
//   activation_button — int, default 6 (LT). Xbox-standard convention:
//                       0=A 1=B 2=X 3=Y 4=LB 5=RB 6=LT 7=RT
//                       8=Back 9=Start 10=L3 11=R3
//                       12=DpadUp 13=DpadDown 14=DpadLeft 15=DpadRight
//   max_correction    — float 0..1, default 0.30. Cap on |correction|.
//   decay_rate        — float 0..1, default 0.10. Strength loss per frame.
//   dead_zone         — float 0..1, default 0.05. Skip if target offset
//                       magnitude (in normalized 0..1 space) is smaller.
class AssistedAimPlugin : public QObject,
                            public IInferenceResultsSinkPlugin,
                            public IInferenceResultsSink,
                            public IControllerStateFilterPlugin,
                            public IControllerStateFilter {
    Q_OBJECT
public:
    AssistedAimPlugin();
    ~AssistedAimPlugin() override;

    QString name()        const override { return QStringLiteral("Assisted Aim"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Accessibility aim assist (button-gated, capped, decaying)"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;
    QObject* qobject() override { return this; }

    // IInferenceResultsSinkPlugin
    IInferenceResultsSink* inferenceResultsSink() override { return this; }
    // IInferenceResultsSink — runs on the producing processor's worker thread.
    void onResults(const InferenceResults& results) override;

    // IControllerStateFilterPlugin
    IControllerStateFilter* controllerStateFilter() override { return this; }
    // IControllerStateFilter — runs on the controller-source thread.
    void apply(ControllerState& state) override;

private:
    SettingsManager* m_settings = nullptr;

    // Tunables (read once at initialize() — restart to re-tune).
    int   m_activationButton = 6;
    float m_maxCorrection    = 0.30f;
    float m_decayRate        = 0.10f;
    float m_deadZone         = 0.05f;

    // Shared state. The detection thread writes target_*; the controller
    // thread reads it and owns the press/strength state. One mutex guards
    // both — contention is microscopic (one read+write per frame on each
    // side at typical 30-60 Hz).
    std::mutex m_mx;
    bool       m_haveTarget       = false;
    float      m_targetCx         = 0.5f;     // normalized 0..1 (image space)
    float      m_targetCy         = 0.5f;
    qint64     m_targetReceivedUs = 0;        // monotonic, source: detection ts
    quint32    m_lastSeq          = 0;

    bool       m_prevHeld = false;
    float      m_strength = 0.0f;
};

} // namespace Labs
