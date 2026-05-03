#include "CompetitiveAimPlugin.h"

#include "SettingsManager.h"

#include <QtMath>

#include <algorithm>
#include <chrono>

namespace Labs {

namespace {

constexpr qint64 kStaleAfterUs   = 250'000;
constexpr float  kStickFullScale = 32767.0f;

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

CompetitiveAimPlugin::CompetitiveAimPlugin()  = default;
CompetitiveAimPlugin::~CompetitiveAimPlugin() = default;

// static
DWORD CompetitiveAimPlugin::parseVK(const QString& key)
{
    if (key.isEmpty()) return VK_F1;

    // F1-F12
    if (key.length() == 2 && key[0].toUpper() == 'F') {
        bool ok = false;
        int n = key.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 12)
            return DWORD(VK_F1 + n - 1);
    }

    // Single letter A-Z
    if (key.length() == 1 && key[0].isLetter())
        return DWORD(key[0].toUpper().unicode());

    // Decimal or hex integer string
    bool ok = false;
    uint vk = key.toUInt(&ok, 0);
    if (ok) return DWORD(vk);

    return VK_F1;
}

// static
bool CompetitiveAimPlugin::foregroundIsMultiplayer()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;

    wchar_t buf[256] = {};
    if (GetWindowTextW(hwnd, buf, 256) == 0) return false;

    QString title = QString::fromWCharArray(buf).toLower();

    static const char* kKeywords[] = {
        "lobby", "matchmaking", "online", "ranked",
        "versus", "pvp", "multiplayer", nullptr
    };
    for (int i = 0; kKeywords[i]; ++i) {
        if (title.contains(QLatin1String(kKeywords[i])))
            return true;
    }
    return false;
}

void CompetitiveAimPlugin::initialize(const PluginContext& ctx)
{
    m_settings = ctx.settings;
    if (!m_settings) return;

    QString key = m_settings->value(QStringLiteral("competitive_aim/toggle_key"), QStringLiteral("F1")).toString();
    m_vkToggle   = parseVK(key);
    m_sensitivity = m_settings->value(QStringLiteral("competitive_aim/sensitivity"), 0.5f).toFloat();
    m_smoothing   = m_settings->value(QStringLiteral("competitive_aim/smoothing"),   0.3f).toFloat();

    m_sensitivity = std::clamp(m_sensitivity, 0.0f, 1.0f);
    m_smoothing   = std::clamp(m_smoothing,   0.0f, 1.0f);
}

void CompetitiveAimPlugin::shutdown()
{
    std::lock_guard<std::mutex> lk(m_mx);
    m_haveTarget  = false;
    m_smoothCorrX = 0.0f;
    m_smoothCorrY = 0.0f;
}

void CompetitiveAimPlugin::onResults(const InferenceResults& r)
{
    std::lock_guard<std::mutex> lk(m_mx);

    if (r.sequence != 0 && r.sequence == m_lastSeq) return;
    m_lastSeq = r.sequence;

    if (r.detections.isEmpty()) {
        m_haveTarget = false;
        return;
    }

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
    m_targetReceivedUs = r.frameTimestampUs > 0 ? r.frameTimestampUs : monotonicUs();
    m_haveTarget = true;
}

void CompetitiveAimPlugin::apply(ControllerState& state)
{
    const bool held = (GetAsyncKeyState(int(m_vkToggle)) & 0x8000) != 0;

    if (!held || foregroundIsMultiplayer()) {
        m_smoothCorrX = 0.0f;
        m_smoothCorrY = 0.0f;
        return;
    }

    bool   haveTarget = false;
    float  cx = 0.5f, cy = 0.5f;

    {
        std::lock_guard<std::mutex> lk(m_mx);
        if (m_haveTarget) {
            const qint64 nowUs = monotonicUs();
            if (m_targetReceivedUs <= 0 || (nowUs - m_targetReceivedUs) <= kStaleAfterUs) {
                haveTarget = true;
                cx = m_targetCx;
                cy = m_targetCy;
            }
        }
    }

    if (!haveTarget) {
        m_smoothCorrX = m_smoothCorrX * m_smoothing;
        m_smoothCorrY = m_smoothCorrY * m_smoothing;
        return;
    }

    const float rawX =  (cx - 0.5f) * m_sensitivity;
    const float rawY =  (cy - 0.5f) * m_sensitivity;

    m_smoothCorrX = m_smoothCorrX * m_smoothing + rawX * (1.0f - m_smoothing);
    m_smoothCorrY = m_smoothCorrY * m_smoothing + rawY * (1.0f - m_smoothing);

    const int dx =  int(m_smoothCorrX * kStickFullScale);
    const int dy = -int(m_smoothCorrY * kStickFullScale);
    state.rightThumbX = clampInt16(int(state.rightThumbX) + dx);
    state.rightThumbY = clampInt16(int(state.rightThumbY) + dy);
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::CompetitiveAimPlugin();
}
