#include "XInputPlugin.h"

#include <QDateTime>
#include <QDebug>

#include <cmath>

#include <Windows.h>
#include <Xinput.h>

namespace Labs {

// ── XInputSource ────────────────────────────────────────────────────────────

XInputSource::XInputSource(QObject* parent) : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(8);  // ~125Hz polling
    connect(&m_timer, &QTimer::timeout, this, &XInputSource::poll);
}

XInputSource::~XInputSource() { stop(); }

bool XInputSource::start()
{
    if (m_running.load()) return true;
    m_activeSlot = -1;
    m_running.store(true);
    m_timer.start();
    return true;
}

void XInputSource::stop()
{
    if (!m_running.exchange(false)) return;
    m_timer.stop();
}

void XInputSource::poll()
{
    if (!m_sink) return;

    const int preferred = (m_activeSlot >= 0) ? m_activeSlot : 0;
    const int slotOrder[4] = { preferred,
                               (preferred + 1) & 3,
                               (preferred + 2) & 3,
                               (preferred + 3) & 3 };

    for (int slot : slotOrder) {
        if (m_skipMask & (1 << slot)) continue;
        XINPUT_STATE xs {};
        DWORD rc = XInputGetState(slot, &xs);
        if (rc == ERROR_SUCCESS) {
            if (m_activeSlot != slot) {
                qInfo() << "[XInput] active slot ->" << slot
                        << "(skipMask=" << m_skipMask << ")";
                m_activeSlot = slot;
            }

            ControllerState s;
            s.buttons      = xs.Gamepad.wButtons;
            s.leftTrigger  = xs.Gamepad.bLeftTrigger;
            s.rightTrigger = xs.Gamepad.bRightTrigger;
            s.leftThumbX   = xs.Gamepad.sThumbLX;
            s.leftThumbY   = xs.Gamepad.sThumbLY;
            s.rightThumbX  = xs.Gamepad.sThumbRX;
            s.rightThumbY  = xs.Gamepad.sThumbRY;
            // XInput-standard radial deadzone. Filters stick wear / center-drift
            // before it reaches the fan-out, so the virtual ViGEm pad stays
            // clean even if the user's physical pad is slightly worn.
            auto deadzone = [](qint16& x, qint16& y, int dz) {
                const double mag = std::sqrt(double(x) * x + double(y) * y);
                if (mag < dz) { x = 0; y = 0; }
            };
            deadzone(s.leftThumbX,  s.leftThumbY,  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            deadzone(s.rightThumbX, s.rightThumbY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            if (s.leftTrigger  < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) s.leftTrigger  = 0;
            if (s.rightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) s.rightTrigger = 0;
            s.timestampUs  = QDateTime::currentMSecsSinceEpoch() * 1000;
            s.slot         = slot;
            s.connected    = true;
            s.kind         = ControllerKind::Xbox;

            m_sink->pushState(s);
            return;
        }
    }

    if (m_idleEmit) {
        // Always-on idle baseline at 125Hz so InputOverride (Labs2K SHM) has
        // a stream to patch script-driven RT/buttons onto. NOT marked as a
        // connected Xbox pad — the monitor must reflect physical truth, not
        // the override pipeline. Output sinks (ViGEm) read sticks/buttons
        // directly and don't gate on `connected`.
        if (m_activeSlot != -1) {
            qInfo() << "[XInput] no connected slot found (skipMask="
                    << m_skipMask << "); falling back to idle baseline";
        }
        m_activeSlot = -1;
        ControllerState s;
        s.connected   = false;
        s.kind        = ControllerKind::None;
        s.slot        = 0;
        s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
        m_sink->pushState(s);
        return;
    }

    if (m_activeSlot != -1) {
        m_activeSlot = -1;
        ControllerState s;
        s.connected   = false;
        s.kind        = ControllerKind::None;
        s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
        m_sink->pushState(s);
    }
}

// ── XInputPlugin ────────────────────────────────────────────────────────────

XInputPlugin::XInputPlugin() : m_source(std::make_unique<XInputSource>(this)) {}
XInputPlugin::~XInputPlugin() = default;

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::XInputPlugin();
}
