#include "TitanInputPlugin.h"

#include <QDateTime>
#include <QDebug>

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

// ── Titan Two HID identity ────────────────────────────────────────────────────
// VID/PID per ConsoleTuner's USB descriptor: 0x1993 / 0x00F0.
static constexpr USHORT kTitanVid = 0x1993;
static constexpr USHORT kTitanPid = 0x00F0;

struct HidProbe {
    QString path;
    USHORT  inputReportLen = 0;
};

// Walk every HID interface, keep only the T2 gamepad collection (Generic
// Desktop / Gamepad).  Same filter approach as DualSensePlugin — T2 also
// exposes multiple interfaces (config endpoint, gamepad pass-through, etc.)
// and only the gamepad collection produces the controller report we want.
static std::vector<HidProbe> enumerateTitanGamepads()
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
        if (!HidD_GetAttributes(h, &attr) || attr.VendorID != kTitanVid || attr.ProductID != kTitanPid) {
            ::CloseHandle(h);
            continue;
        }

        USHORT usagePage = 0, usage = 0, inputLen = 0;
        PHIDP_PREPARSED_DATA pre = nullptr;
        if (HidD_GetPreparsedData(h, &pre) && pre) {
            HIDP_CAPS caps{};
            if (HidP_GetCaps(pre, &caps) == HIDP_STATUS_SUCCESS) {
                usagePage = caps.UsagePage;
                usage     = caps.Usage;
                inputLen  = caps.InputReportByteLength;
            }
            HidD_FreePreparsedData(pre);
        }
        ::CloseHandle(h);

        // Generic Desktop (0x01) → Gamepad (0x05) or Joystick (0x04).
        if (usagePage == 0x01 && (usage == 0x05 || usage == 0x04)) {
            HidProbe p;
            p.path = path;
            p.inputReportLen = inputLen;
            out.push_back(p);
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return out;
}

// Decode a Titan Two report assuming the device is in default DS4
// pass-through mode.  Layout matches the DualShock 4 USB report (DualSense
// plugin's "Ds4Compat" path):
//   [0]    report ID
//   [1..4] LX, LY, RX, RY     (0..255, 0x80 == centered)
//   [5]    dpad (low nibble) + face buttons (high nibble)
//   [6]    L1/R1/L2c/R2c + Share/Options + L3/R3
//   [7]    PS/touchpad + counter
//   [8]    L2 analog (0..255)
//   [9]    R2 analog (0..255)
static ControllerState decodeDs4Compat(const BYTE* r)
{
    ControllerState s;

    auto cnv = [](BYTE v) -> qint16 {
        // Map 0..255 → -32767..32767 with a true center at 0x80.  Clamp to
        // ±32767 (not -32768) so callers can negate without signed overflow.
        const int c = (int(v) - 128) * 257;
        return qint16(qBound(-32767, c, 32767));
    };
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

    const BYTE btnBits      = r[5];
    const BYTE shoulderBits = r[6];
    const BYTE extraBits    = r[7];
    s.leftTrigger  = r[8];
    s.rightTrigger = r[9];

    quint16 buttons = 0;
    switch (btnBits & 0x0F) {
        case 0: buttons |= ButtonDpadUp; break;
        case 1: buttons |= ButtonDpadUp    | ButtonDpadRight; break;
        case 2: buttons |= ButtonDpadRight; break;
        case 3: buttons |= ButtonDpadDown  | ButtonDpadRight; break;
        case 4: buttons |= ButtonDpadDown; break;
        case 5: buttons |= ButtonDpadDown  | ButtonDpadLeft; break;
        case 6: buttons |= ButtonDpadLeft; break;
        case 7: buttons |= ButtonDpadUp    | ButtonDpadLeft; break;
        default: break;  // 8 = released
    }
    if (btnBits & 0x10) buttons |= ButtonX;   // Square   → X
    if (btnBits & 0x20) buttons |= ButtonA;   // Cross    → A
    if (btnBits & 0x40) buttons |= ButtonB;   // Circle   → B
    if (btnBits & 0x80) buttons |= ButtonY;   // Triangle → Y

    if (shoulderBits & 0x01) buttons |= ButtonLeftShoulder;
    if (shoulderBits & 0x02) buttons |= ButtonRightShoulder;
    if (shoulderBits & 0x10) buttons |= ButtonBack;          // Share/Create → Back
    if (shoulderBits & 0x20) buttons |= ButtonStart;         // Options      → Start
    if (shoulderBits & 0x40) buttons |= ButtonLeftThumb;
    if (shoulderBits & 0x80) buttons |= ButtonRightThumb;

    if (extraBits & 0x01) buttons |= ButtonGuide;            // PS button → Guide

    s.buttons     = buttons;
    s.connected   = true;
    s.kind        = ControllerKind::PlayStation;
    s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    return s;
}

// ── TitanInputSource ─────────────────────────────────────────────────────────

struct TitanInputSource::Impl {
    TitanInputSource* owner = nullptr;

    struct DeviceCtx {
        std::thread         thread;
        std::atomic<HANDLE> handle { INVALID_HANDLE_VALUE };
        std::atomic<bool>   active { false };
        QString             path;
    };
    std::vector<std::unique_ptr<DeviceCtx>> devices;

    static HANDLE takeHandle(DeviceCtx& ctx) {
        return ctx.handle.exchange(INVALID_HANDLE_VALUE,
                                   std::memory_order_acq_rel);
    }

    void runReader(DeviceCtx* ctx) {
        BYTE buf[128] = {0};
        OVERLAPPED ov{};
        ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        bool readPending = false;

        while (owner->m_running.load() && ctx->active.load()) {
            HANDLE h = ctx->handle.load(std::memory_order_acquire);
            if (h == INVALID_HANDLE_VALUE) break;

            if (!readPending) {
                ::ResetEvent(ov.hEvent);
                DWORD got0 = 0;
                if (::ReadFile(h, buf, sizeof(buf), &got0, &ov)) {
                    // Synchronous completion path.
                    if (got0 >= 10 && owner->m_sink) {
                        owner->m_sink->pushState(decodeDs4Compat(buf));
                    }
                    continue;
                }
                if (::GetLastError() != ERROR_IO_PENDING) {
                    // Device unplugged or driver stack reset.
                    HANDLE old = takeHandle(*ctx);
                    if (old != INVALID_HANDLE_VALUE) ::CloseHandle(old);
                    break;
                }
                readPending = true;
            }

            // Wait up to 50ms — short enough that stop() is responsive,
            // long enough to keep the thread mostly idle while T2 is quiet.
            const DWORD wr = ::WaitForSingleObject(ov.hEvent, 50);
            if (wr == WAIT_OBJECT_0) {
                DWORD got = 0;
                if (::GetOverlappedResult(h, &ov, &got, FALSE)) {
                    if (got >= 10 && owner->m_sink) {
                        owner->m_sink->pushState(decodeDs4Compat(buf));
                    }
                } else if (::GetLastError() != ERROR_IO_INCOMPLETE) {
                    HANDLE old = takeHandle(*ctx);
                    if (old != INVALID_HANDLE_VALUE) ::CloseHandle(old);
                    break;
                }
                readPending = false;
            }
            // WAIT_TIMEOUT — keep the I/O outstanding; loop tests m_running.
        }

        if (ov.hEvent) ::CloseHandle(ov.hEvent);
    }

    bool startAll() {
        const auto probes = enumerateTitanGamepads();
        if (probes.empty()) {
            qInfo() << "[Titan] no Titan Two gamepad collection found (VID 0x1993 PID 0x00F0)";
            return false;
        }

        for (const auto& p : probes) {
            HANDLE h = ::CreateFileW(reinterpret_cast<LPCWSTR>(p.path.utf16()),
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (h == INVALID_HANDLE_VALUE) continue;

            auto ctx = std::make_unique<DeviceCtx>();
            ctx->path = p.path;
            ctx->handle.store(h, std::memory_order_release);
            ctx->active.store(true, std::memory_order_release);
            DeviceCtx* raw = ctx.get();
            ctx->thread = std::thread([this, raw]{ runReader(raw); });
            devices.push_back(std::move(ctx));
            qInfo() << "[Titan] attached gamepad collection at" << p.path;
        }
        return !devices.empty();
    }

    void stopAll() {
        for (auto& ctx : devices) ctx->active.store(false, std::memory_order_release);
        for (auto& ctx : devices) {
            HANDLE h = takeHandle(*ctx);
            if (h != INVALID_HANDLE_VALUE) {
                ::CancelIoEx(h, nullptr);
                ::CloseHandle(h);
            }
        }
        for (auto& ctx : devices) {
            if (ctx->thread.joinable()) ctx->thread.join();
        }
        devices.clear();
    }
};

TitanInputSource::TitanInputSource(QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Impl>())
{
    m_impl->owner = this;
}

TitanInputSource::~TitanInputSource() { stop(); }

bool TitanInputSource::start()
{
    if (m_running.load()) return true;
    m_running.store(true, std::memory_order_release);
    if (!m_impl->startAll()) {
        m_running.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void TitanInputSource::stop()
{
    if (!m_running.exchange(false, std::memory_order_acq_rel)) return;
    m_impl->stopAll();
}

// ── Plugin shell ─────────────────────────────────────────────────────────────

TitanInputPlugin::TitanInputPlugin()
    : m_source(std::make_unique<TitanInputSource>())
{}

TitanInputPlugin::~TitanInputPlugin() { shutdown(); }

// ── Out of scope — see header note ───────────────────────────────────────────
//
// TODO(titan-firmware): GPC3 bytecode upload to a T2 slot.
//   The reference repo at github.com/BRAetm/labs-titan-gpc3 implements this
//   with HID feature reports 0x30 (slot select) / 0x31 (chunk) / 0x32
//   (finalize) — that protocol does NOT match real Gtuner IV.  Real T2 uses
//   a different vendor protocol with a handshake and per-block CRC checks
//   that has to be reverse-engineered against a physical T2 with USB
//   captures.  Don't ship the reference protocol verbatim — it'll fail
//   silently or, worse, leave the T2 in a partially-flashed state.
//
// TODO(titan-firmware): GPC3 VM execution.
//   GPC3 scripts run on the T2's MCU, not on the host.  The reference repo
//   implements a host-side VM with a homemade 17-opcode ISA — that doesn't
//   execute real Gtuner-compiled bytecode.  Either decline to support GPC3
//   execution at all (the cleanest answer — let users use Gtuner IV for
//   programming and Labs for input/capture) or implement a real GPC3
//   compiler + protocol bridge.  Both are substantial projects.

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::TitanInputPlugin();
}
