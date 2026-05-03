#include "AssistedAimPlugin.h"

#include "SettingsManager.h"

#include <QtMath>

#include <algorithm>
#include <chrono>

namespace Labs {

namespace {

// Stale-target gate. The plugin assists only when the detector is
// actively producing fresh results — past this age the cached target
// is treated as "no detection".
constexpr qint64 kStaleAfterUs = 250'000;   // 250 ms

// Pressure at which we treat a trigger as "held" for activation purposes.
// Picked low so the player doesn't have to mash the trigger to engage,
// but above thumb-resting noise. ~12% pull.
constexpr quint8 kTriggerActivationLevel = 30;

// XInput thumb axes go to 32767 at full deflection. We treat 1.0 of
// "correction magnitude" as full deflection, so multiply by this to
// convert normalized → int16.
constexpr float kStickFullScale = 32767.0f;

bool buttonHeld(const ControllerState& s, int code)
{
    // Xbox-standard button index. See AssistedAimPlugin.h header doc for
    // the full table. Triggers (6, 7) check analog pressure rather than
    // a button bit because XInput exposes them as bytes, not buttons.
    switch (code) {
        case 0:  return (s.buttons & ButtonA) != 0;
        case 1:  return (s.buttons & ButtonB) != 0;
        case 2:  return (s.buttons & ButtonX) != 0;
        case 3:  return (s.buttons & ButtonY) != 0;
        case 4:  return (s.buttons & ButtonLeftShoulder)  != 0;
        case 5:  return (s.buttons & ButtonRightShoulder) != 0;
        case 6:  return s.leftTrigger  >= kTriggerActivationLevel;
        case 7:  return s.rightTrigger >= kTriggerActivationLevel;
        case 8:  return (s.buttons & ButtonBack)  != 0;
        case 9:  return (s.buttons & ButtonStart) != 0;
        case 10: return (s.buttons & ButtonLeftThumb)  != 0;
        case 11: return (s.buttons & ButtonRightThumb) != 0;
        case 12: return (s.buttons & ButtonDpadUp)    != 0;
        case 13: return (s.buttons & ButtonDpadDown)  != 0;
        case 14: return (s.buttons & ButtonDpadLeft)  != 0;
        case 15: return (s.buttons & ButtonDpadRight) != 0;
    }
    return false;
}

qint64 monotonicUs()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

qint16 clampInt16(int v)
{
    if (v >  32767) return  32767;
    if (v < -32767) return -32767;
    return qint16(v);
}

} // namespace

AssistedAimPlugin::AssistedAimPlugin()  = default;
AssistedAimPlugin::~AssistedAimPlugin() = default;

void AssistedAimPlugin::initialize(const PluginContext& ctx)
{
    m_settings = ctx.settings;
    if (!m_settings) return;

    m_activationButton = m_settings->value(QStringLiteral("assisted_aim/activation_button"), 6).toInt();
    m_maxCorrection    = m_settings->value(QStringLiteral("assisted_aim/max_correction"),    0.30f).toFloat();
    m_decayRate        = m_settings->value(QStringLiteral("assisted_aim/decay_rate"),        0.10f).toFloat();
    m_deadZone         = m_settings->value(QStringLiteral("assisted_aim/dead_zone"),         0.05f).toFloat();

    // Defensive clamps — a settings file with garbage shouldn't be able to
    // turn this into a controller hijacker.
    m_maxCorrection = std::clamp(m_maxCorrection, 0.0f, 1.0f);
    m_decayRate     = std::clamp(m_decayRate,     0.0f, 1.0f);
    m_deadZone      = std::clamp(m_deadZone,      0.0f, 1.0f);
}

void AssistedAimPlugin::shutdown()
{
    std::lock_guard<std::mutex> lk(m_mx);
    m_haveTarget = false;
    m_strength   = 0.0f;
    m_prevHeld   = false;
}

void AssistedAimPlugin::onResults(const InferenceResults& r)
{
    std::lock_guard<std::mutex> lk(m_mx);

    if (r.sequence != 0 && r.sequence == m_lastSeq) return;
    m_lastSeq = r.sequence;

    if (r.detections.isEmpty()) {
        m_haveTarget = false;
        return;
    }

    // Pick the detection whose center is closest to the screen center
    // (assumed crosshair). Accessibility intent: assist whatever the
    // player is already approximately aiming at, not the system's top
    // pick by confidence — so a high-confidence target across the screen
    // doesn't drag the stick away from the one they're going for.
    int   bestIdx = -1;
    float bestD2  = std::numeric_limits<float>::max();
    for (int i = 0; i < r.detections.size(); ++i) {
        const auto& d = r.detections[i];
        const float cx = d.x + d.w * 0.5f;
        const float cy = d.y + d.h * 0.5f;
        const float dx = cx - 0.5f;
        const float dy = cy - 0.5f;
        const float d2 = dx*dx + dy*dy;
        if (d2 < bestD2) { bestD2 = d2; bestIdx = i; }
    }
    if (bestIdx < 0) { m_haveTarget = false; return; }

    const auto& best = r.detections[bestIdx];
    m_targetCx = best.x + best.w * 0.5f;
    m_targetCy = best.y + best.h * 0.5f;
    // Use frame timestamp when available; otherwise stamp arrival time so
    // the staleness gate still works when the source skips it.
    m_targetReceivedUs = r.frameTimestampUs > 0 ? r.frameTimestampUs : monotonicUs();
    m_haveTarget       = true;
}

void AssistedAimPlugin::apply(ControllerState& state)
{
    const bool held = buttonHeld(state, m_activationButton);

    bool   doApply  = false;
    float  cx       = 0.5f;
    float  cy       = 0.5f;
    float  strength = 0.0f;

    {
        std::lock_guard<std::mutex> lk(m_mx);

        if (!held) {
            // Hard release: zero state, no output.
            m_strength = 0.0f;
            m_prevHeld = false;
            return;
        }

        // Press edge — full strength budget for this hold.
        if (!m_prevHeld) m_strength = 1.0f;
        m_prevHeld = true;

        // Bail early if no detection at all.
        if (!m_haveTarget) {
            // Still decay, so the budget doesn't reset by accident if the
            // target reappears mid-hold.
            m_strength = std::max(0.0f, m_strength - m_decayRate);
            return;
        }

        // Stale gate. Compare against monotonic clock so a missed/late
        // detection thread can't keep us applying corrections forever.
        const qint64 nowUs = monotonicUs();
        if (m_targetReceivedUs > 0 && (nowUs - m_targetReceivedUs) > kStaleAfterUs) {
            m_strength = std::max(0.0f, m_strength - m_decayRate);
            return;
        }

        cx = m_targetCx;
        cy = m_targetCy;
        strength = m_strength;

        // Decay applies once per frame regardless of dead-zone outcome
        // below — holding the trigger while target is dead-on still spends
        // the budget.
        m_strength = std::max(0.0f, m_strength - m_decayRate);

        if (strength > 0.0f) doApply = true;
    }

    if (!doApply) return;

    // Vector from screen center → target center, in normalized image space.
    const float ox = cx - 0.5f;
    const float oy = cy - 0.5f;
    const float mag = std::sqrt(ox*ox + oy*oy);
    if (mag < m_deadZone) return;

    // Scale toward target, clamped to the configured max in stick units.
    // The spec frames `max_correction` as fraction-of-stick, so 0.30 means
    // we never push the stick past 30% of its full deflection.
    float corrX = ox * strength;
    float corrY = oy * strength;

    const float corrMag = std::sqrt(corrX*corrX + corrY*corrY);
    if (corrMag > m_maxCorrection) {
        const float scale = m_maxCorrection / corrMag;
        corrX *= scale;
        corrY *= scale;
    }

    // Apply additively to the right stick. Y axis inverts: image Y grows
    // downward, but XInput Y grows upward (positive = stick UP).
    const int dx =  int(corrX * kStickFullScale);
    const int dy = -int(corrY * kStickFullScale);
    state.rightThumbX = clampInt16(int(state.rightThumbX) + dx);
    state.rightThumbY = clampInt16(int(state.rightThumbY) + dy);
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::AssistedAimPlugin();
}
