#pragma once

#include "IPlugin.h"
#include "IFrameProcessor.h"
#include "InputTypes.h"

#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace Labs {

class SettingsManager;

// Keyboard-hotkey-gated aim assist for single-player games.
//
// Player explicitly holds a configurable keyboard key (default F1) to engage.
// On release the right stick correction drops to zero immediately — no decay,
// no lingering output.  The plugin also enforces a single-player guard: it
// checks the foreground window title for known multiplayer keywords and
// refuses to engage if any are found.
//
// Smoothing: each frame the stick output is lerped toward the raw computed
// correction by `smoothing` (0 = instant, 1 = never moves).  This keeps
// micro-target jitter from punching through to the controller.
//
// Settings (under "competitive_aim/"):
//   toggle_key   — string, default "F1". Accepts F1-F12, single letter A-Z,
//                  or decimal VK code as a string (e.g. "0x70").
//   sensitivity  — float 0..1, default 0.5. Scales the correction magnitude.
//   smoothing    — float 0..1, default 0.3. Lerp factor toward target correction.
class CompetitiveAimPlugin : public QObject,
                              public IInferenceResultsSinkPlugin,
                              public IInferenceResultsSink,
                              public IControllerStateFilterPlugin,
                              public IControllerStateFilter {
    Q_OBJECT
public:
    CompetitiveAimPlugin();
    ~CompetitiveAimPlugin() override;

    QString name()        const override { return QStringLiteral("Competitive Aim"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Keyboard-gated aim assist for single-player games"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;
    QObject* qobject() override { return this; }

    // IInferenceResultsSinkPlugin
    IInferenceResultsSink* inferenceResultsSink() override { return this; }
    void onResults(const InferenceResults& results) override;

    // IControllerStateFilterPlugin
    IControllerStateFilter* controllerStateFilter() override { return this; }
    void apply(ControllerState& state) override;

private:
    static DWORD parseVK(const QString& key);
    static bool  foregroundIsMultiplayer();

    SettingsManager* m_settings = nullptr;

    DWORD m_vkToggle    = VK_F1;
    float m_sensitivity = 0.5f;
    float m_smoothing   = 0.3f;

    std::mutex m_mx;
    bool       m_haveTarget       = false;
    float      m_targetCx         = 0.5f;
    float      m_targetCy         = 0.5f;
    qint64     m_targetReceivedUs = 0;
    quint32    m_lastSeq          = 0;

    // Smoothed stick correction state (owned by controller thread).
    float m_smoothCorrX = 0.0f;
    float m_smoothCorrY = 0.0f;
};

} // namespace Labs
