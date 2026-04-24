#include "InputOverride.h"

#include <Windows.h>
#include <QByteArray>
#include <QDebug>
#include <QString>
#include <chrono>

namespace Labs {

// SHM name bumped to v2 — the timebase changed (raw QPC ticks instead of
// std::chrono::steady_clock microseconds). Old Python clients using v1
// won't accidentally write an incompatible record into our v2 namespace.
static const wchar_t* kShmName = L"Labs_Input_Override_v2";

// Raw QPC counter ticks. Identical value across processes on Windows
// (QPC reads the same hardware counter regardless of process). Eliminates
// the clock-epoch ambiguity that std::chrono::steady_clock and Python's
// time.monotonic_ns() suffer from across the language boundary.
static uint64_t now_us()
{
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    return uint64_t(qpc.QuadPart);
}

// QPC frequency cached once. Used to compute "60 seconds in QPC ticks" so we
// can reject obviously bogus expires_qpc values (torn writes, stale data from
// a process that crashed mid-write).
static uint64_t qpc_frequency()
{
    static uint64_t freq = []() {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return uint64_t(f.QuadPart);
    }();
    return freq;
}

InputOverride& InputOverride::instance()
{
    static InputOverride s;
    return s;
}

InputOverride::InputOverride() = default;

InputOverride::~InputOverride()
{
    if (m_record)  ::UnmapViewOfFile(m_record);
    if (m_mapping) ::CloseHandle(static_cast<HANDLE>(m_mapping));
    m_record  = nullptr;
    m_mapping = nullptr;
}

bool InputOverride::ensure_mapped()
{
    if (m_record) return true;

    // Try OpenFileMapping first (Python script may have created it). Fall
    // back to creating it ourselves so we can read what the script later
    // writes. PAGE_READWRITE both ways — Windows is permissive about this.
    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kShmName);
    if (!h) {
        h = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
                                 PAGE_READWRITE, 0,
                                 sizeof(InputOverrideRecord), kShmName);
    }
    if (!h) return false;

    auto* p = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0,
                              sizeof(InputOverrideRecord));
    if (!p) {
        ::CloseHandle(h);
        return false;
    }
    m_mapping = h;
    m_record  = reinterpret_cast<InputOverrideRecord*>(p);
    return true;
}

void InputOverride::apply(ControllerState& state)
{
    if (!ensure_mapped()) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning() << "[OVERRIDE] SHM mapping unavailable";
        }
        return;
    }

    const uint64_t expiry = m_record->expires_qpc;
    if (expiry == 0) return;
    const uint64_t cur = now_us();
    if (cur >= expiry) return;
    // Sanity gate: a sane override is at most ~1s long. If expires_qpc
    // claims more than 60 seconds into the future, treat it as torn or
    // stale data from a process that died mid-write and ignore.
    if (expiry - cur > qpc_frequency() * 60) return;

    const uint8_t  flags = m_record->flags;
    const uint8_t  rt    = m_record->rt;
    const uint8_t  l2    = m_record->l2;
    const int16_t  rsy   = m_record->rs_y;
    const int16_t  lsy   = m_record->ls_y;
    const uint16_t btns  = m_record->buttons;
    if (flags & 0x01) state.rightTrigger = rt;
    if (flags & 0x02) state.leftTrigger  = l2;
    if (flags & 0x04) state.rightThumbY  = rsy;
    if (flags & 0x08) state.leftThumbY   = lsy;
    if (flags & 0x10) state.buttons     |= btns;  // OR so user buttons still work
}

} // namespace Labs
