#include "DualSensePlugin.h"

#include <QDateTime>
#include <QDebug>
#include <QString>

#include <Windows.h>
#include <SetupAPI.h>
extern "C" {
#include <hidsdi.h>
}

#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

namespace Labs {

// ── Sony HID identifiers ─────────────────────────────────────────────────────
static constexpr USHORT kSonyVid          = 0x054C;
static constexpr USHORT kPidDualSense     = 0x0CE6;
static constexpr USHORT kPidDualSenseEdge = 0x0DF2;
static constexpr USHORT kPidDualShock4_v1 = 0x05C4;
static constexpr USHORT kPidDualShock4_v2 = 0x09CC;

static bool isSupportedPid(USHORT pid)
{
    return pid == kPidDualSense || pid == kPidDualSenseEdge
        || pid == kPidDualShock4_v1 || pid == kPidDualShock4_v2;
}

struct HidDevicePath {
    QString path;
    USHORT  pid = 0;
};

struct HidProbe {
    QString path;
    USHORT  pid = 0;
    USHORT  usagePage = 0;
    USHORT  usage     = 0;
    USHORT  inputReportLen = 0;
};

// Enumerate Sony HID collections and keep only the ones whose top-level
// collection is a Gamepad / Joystick on the Generic Desktop page. DualSense
// exposes multiple interfaces (keyboard emulation, audio, gamepad). Opening
// the wrong one gives garbage reports — exactly the symptom the user hit.
static std::vector<HidProbe> enumPlaystationHids()
{
    std::vector<HidProbe> out;
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return out;

    SP_DEVICE_INTERFACE_DATA ifd{}; ifd.cbSize = sizeof(ifd);
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &hidGuid, i, &ifd); ++i) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifd, nullptr, 0, &needed, nullptr);
        if (needed == 0) continue;

        std::vector<BYTE> buf(needed);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifd, detail, needed, nullptr, nullptr))
            continue;

        const QString path = QString::fromWCharArray(detail->DevicePath);
        HANDLE h = ::CreateFileW(detail->DevicePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attr{}; attr.Size = sizeof(attr);
        if (!HidD_GetAttributes(h, &attr) || attr.VendorID != kSonyVid || !isSupportedPid(attr.ProductID)) {
            ::CloseHandle(h);
            continue;
        }

        HidProbe p;
        p.path = path;
        p.pid  = attr.ProductID;

        PHIDP_PREPARSED_DATA pre = nullptr;
        if (HidD_GetPreparsedData(h, &pre) && pre) {
            HIDP_CAPS caps{};
            if (HidP_GetCaps(pre, &caps) == HIDP_STATUS_SUCCESS) {
                p.usagePage      = caps.UsagePage;
                p.usage          = caps.Usage;
                p.inputReportLen = caps.InputReportByteLength;
            }
            HidD_FreePreparsedData(pre);
        }
        ::CloseHandle(h);

        // Only keep the gamepad top-level collection. Generic Desktop page (0x01),
        // Gamepad (0x05) or Joystick (0x04).
        if (p.usagePage == 0x01 && (p.usage == 0x05 || p.usage == 0x04)) {
            out.push_back(p);
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return out;
}

// DualSense and DualShock 4 share the sticks/dpad encoding but the offsets of
// the button bytes differ. Both pads emit report ID 0x01 so we must branch on
// PID. Layouts (the first data byte after report ID is index 1):
//
//   DualShock 4 USB (PID 0x05C4, 0x09CC):
//     [1..4] sticks  LX,LY,RX,RY
//     [5]    dpad + face buttons     (SQUARE=0x10, X=0x20, CIRCLE=0x40, TRI=0x80)
//     [6]    L1/R1/L2c/R2c + SHARE/OPTIONS + L3/R3
//     [7]    PS/TPAD + counter
//     [8]    L2 analog
//     [9]    R2 analog
//
//   DualSense USB (PID 0x0CE6, 0x0DF2):
//     [1..4] sticks  LX,LY,RX,RY
//     [5]    L2 analog
//     [6]    R2 analog
//     [7]    sequence counter
//     [8]    dpad + face buttons
//     [9]    L1/R1/L2c/R2c + CREATE/OPTIONS + L3/R3
//     [10]   PS/mute/touchpad
enum class Layout { Ds4Compat, DualSenseFull };

// Maximum byte index into `report` that mapReport() touches, given a layout.
// Used to bound-check after the BT shift so we don't read off the end of buf.
static constexpr size_t kDs4CompatMaxIdx       = 9;   // l2 at [8], r2 at [9]
static constexpr size_t kDualSenseFullMaxIdx   = 10;  // extraBits at [10]
static size_t requiredLen(Layout layout)
{
    return (layout == Layout::DualSenseFull ? kDualSenseFullMaxIdx
                                            : kDs4CompatMaxIdx) + 1;
}

// PID-keyed layout default. USB vs BT is decided separately by report length
// and prefix bytes; this just picks the family. Falls back to Ds4Compat for
// unknown PIDs (shouldn't happen — enum filters by isSupportedPid — but safe).
static Layout defaultLayoutForPid(USHORT pid)
{
    switch (pid) {
        case kPidDualSense:
        case kPidDualSenseEdge:    return Layout::DualSenseFull;
        case kPidDualShock4_v1:
        case kPidDualShock4_v2:    return Layout::Ds4Compat;
        default:                   return Layout::Ds4Compat;
    }
}

static ControllerState mapReport(const BYTE* r, Layout layout, int slot)
{
    ControllerState s;
    // Clamp to ±32767 (not -32768) so callers can safely negate the result —
    // negating qint16(-32768) overflows back to -32768, which manifested as
    // "stick fully up gets sent as fully down" downstream.
    auto cnv = [](BYTE v) -> qint16 {
        const int c = (int(v) - 128) * 257;
        return qint16(qBound(-32767, c, 32767));
    };
    // XInput-standard radial deadzone. Below this, the stick reads "centered"
    // so DualSense sensor noise / center-drift doesn't bleed into the virtual
    // X360 pad we expose to xbox.com/play. Numbers match XINPUT_GAMEPAD_*_THUMB_DEADZONE.
    auto deadzone = [](qint16& x, qint16& y, int dz) {
        const double mag = std::sqrt(double(x) * x + double(y) * y);
        if (mag < dz) { x = 0; y = 0; }
    };
    s.leftThumbX  =  cnv(r[1]);
    s.leftThumbY  = -cnv(r[2]);
    s.rightThumbX =  cnv(r[3]);
    s.rightThumbY = -cnv(r[4]);
    deadzone(s.leftThumbX,  s.leftThumbY,  7849);
    deadzone(s.rightThumbX, s.rightThumbY, 8689);

    BYTE btnBits, shoulderBits, extraBits;
    BYTE l2, r2;
    if (layout == Layout::DualSenseFull) {
        l2           = r[5];
        r2           = r[6];
        btnBits      = r[8];
        shoulderBits = r[9];
        extraBits    = r[10];
    } else {
        btnBits      = r[5];
        shoulderBits = r[6];
        extraBits    = r[7];
        l2           = r[8];
        r2           = r[9];
    }
    s.leftTrigger  = l2;
    s.rightTrigger = r2;

    const BYTE dpad = btnBits & 0x0F;
    quint16 buttons = 0;
    switch (dpad) {
        case 0: buttons |= ButtonDpadUp; break;
        case 1: buttons |= ButtonDpadUp    | ButtonDpadRight; break;
        case 2: buttons |= ButtonDpadRight; break;
        case 3: buttons |= ButtonDpadDown  | ButtonDpadRight; break;
        case 4: buttons |= ButtonDpadDown; break;
        case 5: buttons |= ButtonDpadDown  | ButtonDpadLeft; break;
        case 6: buttons |= ButtonDpadLeft; break;
        case 7: buttons |= ButtonDpadUp    | ButtonDpadLeft; break;
        default: break;
    }
    if (btnBits & 0x10) buttons |= ButtonX;   // Square   → X
    if (btnBits & 0x20) buttons |= ButtonA;   // Cross    → A
    if (btnBits & 0x40) buttons |= ButtonB;   // Circle   → B
    if (btnBits & 0x80) buttons |= ButtonY;   // Triangle → Y

    if (shoulderBits & 0x01) buttons |= ButtonLeftShoulder;
    if (shoulderBits & 0x02) buttons |= ButtonRightShoulder;
    if (shoulderBits & 0x10) buttons |= ButtonBack;   // Share/Create → Back
    if (shoulderBits & 0x20) buttons |= ButtonStart;  // Options      → Start
    if (shoulderBits & 0x40) buttons |= ButtonLeftThumb;
    if (shoulderBits & 0x80) buttons |= ButtonRightThumb;

    if (extraBits & 0x01) buttons |= ButtonGuide;     // PS → Guide

    s.buttons     = buttons;
    s.connected   = true;
    s.kind        = ControllerKind::PlayStation;
    s.slot        = slot;
    s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    return s;
}

// ── DualSenseSource impl ─────────────────────────────────────────────────────
struct DualSenseSource::Impl {
    DualSenseSource* owner = nullptr;

    // Per-device reader. Owns a thread + atomic handle so stop() and the
    // reader can both safely race to close the HID handle exactly once.
    struct DeviceCtx {
        std::thread             thread;
        std::atomic<HANDLE>     handle{INVALID_HANDLE_VALUE};
        std::atomic<bool>       active{false};
        QString                 path;          // path we last attached to
        USHORT                  pid = 0;
        int                     slot = 0;
    };
    // Up to 2 controllers (slot 0, slot 1). DeviceCtx is non-movable due to
    // the atomic — use unique_ptr so the vector is fine.
    std::vector<std::unique_ptr<DeviceCtx>> devices;

    // Atomically take ownership of the handle from `slot.handle`, returning
    // the prior value. Whoever sees a real (non-INVALID) handle is responsible
    // for CloseHandle. Anyone else just exits.
    static HANDLE takeHandle(DeviceCtx& ctx) {
        return ctx.handle.exchange(INVALID_HANDLE_VALUE,
                                   std::memory_order_acq_rel);
    }

    void runReader(DeviceCtx* ctx) {
        ControllerKind kind = ControllerKind::PlayStation;
        const Layout pidLayout = defaultLayoutForPid(ctx->pid);
        Layout layout = pidLayout;

        BYTE buf[128] = {0};
        OVERLAPPED ov{};
        ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        bool readPending = false;

        // First-frame heuristic only as a fallback when PID is unknown.
        bool needsHeuristic = false;
        bool dumpedFirst = false;

        while (owner->m_running.load() && ctx->active.load()) {
            // (Re-)issue ReadFile only when no prior I/O is queued. Reusing
            // OVERLAPPED while the kernel still owns it is undefined.
            if (!readPending) {
                ::ResetEvent(ov.hEvent);
                DWORD got0 = 0;
                BOOL ok = ::ReadFile(ctx->handle.load(), buf, sizeof(buf), &got0, &ov);
                if (ok) {
                    // Synchronous completion. Skip the wait, dispatch directly.
                    if (got0 == 0) { qInfo() << "[DualSense] disconnected"; break; }
                    DWORD got = got0;
                    if (!dispatchReport(ctx, buf, got, pidLayout, layout,
                                        needsHeuristic, dumpedFirst, kind)) {
                        // dispatchReport returns false only on hard error; treat
                        // as disconnect.
                        break;
                    }
                    continue;
                }
                if (::GetLastError() != ERROR_IO_PENDING) {
                    qInfo() << "[DualSense] ReadFile error" << ::GetLastError();
                    break;
                }
                readPending = true;
            }

            DWORD wait = ::WaitForSingleObject(ov.hEvent, 1000);
            if (wait == WAIT_OBJECT_0) {
                DWORD got = 0;
                BOOL ok = ::GetOverlappedResult(ctx->handle.load(), &ov, &got, FALSE);
                readPending = false;
                if (!ok || got == 0) {
                    qInfo() << "[DualSense] disconnected";
                    break;
                }
                if (!dispatchReport(ctx, buf, got, pidLayout, layout,
                                    needsHeuristic, dumpedFirst, kind)) {
                    break;
                }
                continue;
            }
            if (wait == WAIT_TIMEOUT) {
                // I/O is still pending. DO NOT re-issue ReadFile — keep waiting
                // on the same OVERLAPPED next iteration. If we're shutting
                // down, cancel and drain it before exiting.
                if (!owner->m_running.load() || !ctx->active.load()) {
                    HANDLE h = ctx->handle.load();
                    if (h != INVALID_HANDLE_VALUE) ::CancelIoEx(h, nullptr);
                    DWORD got = 0;
                    if (h != INVALID_HANDLE_VALUE) {
                        ::GetOverlappedResult(h, &ov, &got, TRUE);
                    }
                    readPending = false;
                    break;
                }
                continue;
            }
            // WAIT_FAILED or anything else — bail.
            qInfo() << "[DualSense] WaitForSingleObject failed" << ::GetLastError();
            break;
        }

        // If we exited with I/O still queued (e.g. ReadFile error path),
        // make sure the kernel isn't holding our buffer.
        if (readPending) {
            HANDLE h = ctx->handle.load();
            if (h != INVALID_HANDLE_VALUE) ::CancelIoEx(h, nullptr);
            DWORD got = 0;
            if (h != INVALID_HANDLE_VALUE) {
                ::GetOverlappedResult(h, &ov, &got, TRUE);
            }
        }

        ::CloseHandle(ov.hEvent);

        // Reader owns the close: exchange the handle to INVALID and only the
        // path that grabs the real value calls CloseHandle. stop() does the
        // same exchange dance so we can't double-close.
        HANDLE h = takeHandle(*ctx);
        if (h != INVALID_HANDLE_VALUE) ::CloseHandle(h);
    }

    // Returns true to keep reading, false on fatal error (caller breaks out).
    bool dispatchReport(DeviceCtx* ctx, BYTE* buf, DWORD got,
                        Layout pidLayout, Layout& layout,
                        bool& needsHeuristic, bool& dumpedFirst,
                        ControllerKind /*kind*/) {
        // USB: buf[0] == 0x01 and bytes line up with our USB layout
        // (report id at [0], LX at [1]).
        // BT: DualSense emits report 0x31 (>= 78 bytes), which has one
        //   "tag" byte between the report id and the USB-equivalent
        //   payload, so the payload starts at buf[2] and "LX" — which
        //   our mapping reads at report[1] — must come from buf[2].
        //   Shift by +1 (not +2) so report[1] == buf[2].
        // DS4 BT uses report 0x11 with a similar two-byte prefix.
        const BYTE* report = buf;
        if (got >= 78 && (buf[0] == 0x31 || buf[0] == 0x11)) {
            report = buf + 1;
        }

        // Bound the post-shift report by the layout's max read offset.
        const size_t shift     = static_cast<size_t>(report - buf);
        const size_t reportLen = (got >= shift) ? (static_cast<size_t>(got) - shift) : 0;
        if (reportLen < requiredLen(layout)) return true;

        if (!dumpedFirst) {
            dumpedFirst = true;
            // PID-keyed default — do not auto-detect from the first frame
            // unless the PID is unknown. Auto-detect mis-locks if the user
            // happened to be holding any direction at startup.
            const bool known = ctx->pid == kPidDualSense
                            || ctx->pid == kPidDualSenseEdge
                            || ctx->pid == kPidDualShock4_v1
                            || ctx->pid == kPidDualShock4_v2;
            if (known) {
                layout = pidLayout;
                needsHeuristic = false;
            } else {
                // Fallback: dpad neutral nibble (0x8) tells us where buttons live.
                const BYTE lo5 = (reportLen > 5) ? (report[5] & 0x0F) : 0xFF;
                const BYTE lo8 = (reportLen > 8) ? (report[8] & 0x0F) : 0xFF;
                if (lo8 == 0x8 && lo5 != 0x8) layout = Layout::DualSenseFull;
                else                          layout = Layout::Ds4Compat;
                needsHeuristic = true;
            }

            const int n = qMin(DWORD(16), got);
            QString hex;
            for (int k = 0; k < n; ++k) hex += QString::asprintf("%02X ", buf[k]);
            qInfo().noquote() << QStringLiteral("[DualSense] slot=%1 first report (%2 bytes): %3  → layout=%4 (%5)")
                                     .arg(ctx->slot).arg(got).arg(hex.trimmed())
                                     .arg(layout == Layout::DualSenseFull ? "DualSenseFull" : "Ds4Compat")
                                     .arg(needsHeuristic ? "heuristic" : "PID-keyed");

            // Re-check bound now that the layout may have changed.
            if (reportLen < requiredLen(layout)) return true;
        }

        ControllerState s = mapReport(report, layout, ctx->slot);
        if (owner->m_sink) owner->m_sink->pushState(s);
        return true;
    }

    void supervisor() {
        // Keep up to 2 device contexts alive; reattach on disconnect.
        while (owner->m_running.load()) {
            auto devs = enumPlaystationHids();
            if (devs.empty()) {
                if (owner->m_sink) {
                    ControllerState s;
                    s.connected = false;
                    s.kind      = ControllerKind::None;
                    s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
                    owner->m_sink->pushState(s);
                }
                ::Sleep(250);
                // Reap any finished reader threads.
                reapFinished();
                continue;
            }

            // Slot 0 and slot 1 — open the first two distinct enumerated
            // gamepad collections. (Common 2-controller case; we don't open
            // the entire list to keep this surgical.)
            const int kMaxSlots = 2;
            const int wanted = qMin(kMaxSlots, int(devs.size()));
            for (int slot = 0; slot < wanted; ++slot) {
                attachIfMissing(devs[slot], slot);
            }
            reapFinished();
            ::Sleep(500);
        }

        // Shutdown: signal each device, cancel pending I/O, join.
        for (auto& up : devices) {
            up->active.store(false);
            HANDLE h = takeHandle(*up);
            if (h != INVALID_HANDLE_VALUE) {
                ::CancelIoEx(h, nullptr);
                ::CloseHandle(h);
            }
        }
        for (auto& up : devices) {
            if (up->thread.joinable()) up->thread.join();
        }
        devices.clear();
    }

    void attachIfMissing(const HidProbe& d, int slot) {
        // If we already have an active reader for this slot+path, leave it.
        for (auto& up : devices) {
            if (up->slot == slot && up->active.load() && up->path == d.path) return;
        }
        // If something else is in this slot but stale, mark for reaping by
        // letting it finish naturally. We'll only attach a new one when no
        // active context occupies that slot.
        for (auto& up : devices) {
            if (up->slot == slot && up->active.load()) {
                // Slot held by a different path/device — don't double-attach.
                return;
            }
        }

        std::wstring wpath = d.path.toStdWString();
        HANDLE h = ::CreateFileW(wpath.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;

        auto ctx = std::make_unique<DeviceCtx>();
        ctx->handle.store(h);
        ctx->active.store(true);
        ctx->path = d.path;
        ctx->pid  = d.pid;
        ctx->slot = slot;
        DeviceCtx* raw = ctx.get();
        ctx->thread = std::thread([this, raw]{
            runReader(raw);
            raw->active.store(false);
        });
        qInfo().noquote() << QStringLiteral("[DualSense] slot=%1 connected — pid=0x%2 reportLen=%3")
                                 .arg(slot)
                                 .arg(d.pid, 4, 16, QLatin1Char('0'))
                                 .arg(d.inputReportLen);
        devices.push_back(std::move(ctx));
    }

    void reapFinished() {
        for (auto it = devices.begin(); it != devices.end();) {
            auto& up = *it;
            if (!up->active.load()) {
                if (up->thread.joinable()) up->thread.join();
                it = devices.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::thread supervisorThread;

    void start() {
        owner->m_running.store(true);
        supervisorThread = std::thread([this]{ supervisor(); });
    }

    void stop() {
        owner->m_running.store(false);
        // Pre-emptively unblock all readers: take each handle (reader will
        // see INVALID and exit; we close it). This is the same exchange
        // dance the reader does, so only one of us calls CloseHandle.
        for (auto& up : devices) {
            up->active.store(false);
            HANDLE h = takeHandle(*up);
            if (h != INVALID_HANDLE_VALUE) {
                ::CancelIoEx(h, nullptr);
                ::CloseHandle(h);
            }
        }
        if (supervisorThread.joinable()) supervisorThread.join();
        // supervisor() also joins reader threads; nothing to do here.
    }
};

DualSenseSource::DualSenseSource(QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Impl>())
{
    m_impl->owner = this;
}

DualSenseSource::~DualSenseSource() { stop(); }

bool DualSenseSource::start()
{
    if (m_running.load()) return true;
    m_impl->start();
    qInfo() << "[DualSense] source started";
    return true;
}

void DualSenseSource::stop()
{
    if (!m_running.load()) return;
    m_impl->stop();
}

// ── Plugin ───────────────────────────────────────────────────────────────────
DualSensePlugin::DualSensePlugin()
    : m_source(std::make_unique<DualSenseSource>(this)) {}

DualSensePlugin::~DualSensePlugin() = default;

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::DualSensePlugin();
}
