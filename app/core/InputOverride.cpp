#include "InputOverride.h"

#include <Windows.h>
#include <QByteArray>
#include <QDebug>
#include <QString>
#include <atomic>
#include <chrono>
#include <cstring>

namespace Labs {

// Build the wide-char SHM name once from the shared kSharedMemName constant
// (defined in the header so the writer side and any cross-language doc
// reference a single source of truth).
static const std::wstring& kShmNameW()
{
    static const std::wstring name = []{
        std::wstring w;
        const char* s = kSharedMemName;
        while (*s) w.push_back(static_cast<wchar_t>(*s++));
        return w;
    }();
    return name;
}

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
    const wchar_t* name = kShmNameW().c_str();
    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (!h) {
        h = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
                                 PAGE_READWRITE, 0,
                                 sizeof(InputOverrideRecord), name);
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

    // Magic check — rejects an uninitialized SHM section (all zeros) and
    // anything written by an unrelated process that happened to grab the same
    // name. "LBSO" little-endian = 0x4F53424C.
    {
        char m[4];
        std::memcpy(m, m_record->magic, 4);
        if (m[0] != 'L' || m[1] != 'B' || m[2] != 'S' || m[3] != 'O') return;
    }

    // Seqlock read: snapshot all override fields between two reads of
    // `sequence`. Retry if the writer was mid-update (odd seq) or if the
    // sequence changed across the read (writer raced us). Cap at 3 attempts
    // so the input thread never stalls on a runaway writer.
    InputOverrideRecord snap;
    auto* volatile seqp = reinterpret_cast<volatile uint32_t*>(&m_record->sequence);
    bool ok = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        const uint32_t s1 = *seqp;
        if (s1 & 1u) { YieldProcessor(); continue; }
        std::atomic_thread_fence(std::memory_order_acquire);
        std::memcpy(&snap, m_record, sizeof(snap));
        std::atomic_thread_fence(std::memory_order_acquire);
        const uint32_t s2 = *seqp;
        if (s1 == s2) { ok = true; break; }
        YieldProcessor();
    }
    if (!ok) return;

    const uint64_t expiry = snap.expires_qpc;
    if (expiry == 0) return;
    const uint64_t cur = now_us();
    if (cur >= expiry) return;
    // Sanity gate: a sane override is at most ~1s long. If expires_qpc
    // claims more than 60 seconds into the future, treat it as torn or
    // stale data from a process that died mid-write and ignore.
    if (expiry - cur > qpc_frequency() * 60) return;

    const uint8_t  flags = snap.flags;
    if (flags & 0x01) state.rightTrigger = snap.rt;
    if (flags & 0x02) state.leftTrigger  = snap.l2;
    if (flags & 0x04) state.rightThumbY  = snap.rs_y;
    if (flags & 0x08) state.leftThumbY   = snap.ls_y;
    if (flags & 0x10) state.buttons     |= snap.buttons;  // OR so user buttons still work
    if (flags & 0x20) state.rightThumbX  = snap.rs_x;
    if (flags & 0x40) state.leftThumbX   = snap.ls_x;
}

} // namespace Labs
