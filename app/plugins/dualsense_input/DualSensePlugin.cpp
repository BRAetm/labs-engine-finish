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

// Windows enumerates both DS4 and DualSense in "DS4 compat" mode by default
// (report id 0x01, dpad+buttons at [5], triggers at [8..9]). A DualSense only
// switches to its full 64-byte layout (sticks/triggers first, buttons at [8])
// after we send a feature report — which we don't. Detect by dpad location:
// at idle the dpad low nibble == 0x08 (neutral). If that's at byte 5, we're
// in DS4 compat; if at byte 8, full DualSense.
static ControllerState mapReport(const BYTE* r, Layout layout)
{
    ControllerState s;
    // Clamp to ±32767 (not -32768) so callers can safely negate the result —
    // negating qint16(-32768) overflows back to -32768, which manifested as
    // "stick fully up gets sent as fully down" downstream.
    auto cnv = [](BYTE v) -> qint16 {
        const int c = (int(v) - 128) * 257;
        return qint16(qBound(-32767, c, 32767));
    };
    s.leftThumbX  =  cnv(r[1]);
    s.leftThumbY  = -cnv(r[2]);
    s.rightThumbX =  cnv(r[3]);
    s.rightThumbY = -cnv(r[4]);

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
    s.slot        = 0;
    s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
    return s;
}

// ── DualSenseSource impl ─────────────────────────────────────────────────────
struct DualSenseSource::Impl {
    DualSenseSource*   owner  = nullptr;
    std::thread        readerThread;
    HANDLE             hidHandle = INVALID_HANDLE_VALUE;
    USHORT             pid = 0;

    void runReader() {
        while (owner->m_running.load()) {
            auto devs = enumPlaystationHids();
            if (devs.empty()) {
                // Emit a disconnected tick so any UI reflects "no pad".
                if (owner->m_sink) {
                    ControllerState s;
                    s.connected = false;
                    s.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
                    owner->m_sink->pushState(s);
                }
                ::Sleep(1000);
                continue;
            }
            const auto d = devs.front();
            std::wstring wpath = d.path.toStdWString();
            hidHandle = ::CreateFileW(wpath.c_str(),
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (hidHandle == INVALID_HANDLE_VALUE) { ::Sleep(500); continue; }
            pid = d.pid;
            qInfo().noquote() << QStringLiteral("[DualSense] connected — pid=0x%1 usage=0x%2 reportLen=%3")
                                     .arg(d.pid, 4, 16, QLatin1Char('0'))
                                     .arg(d.usage, 4, 16, QLatin1Char('0'))
                                     .arg(d.inputReportLen);

            BYTE buf[128] = {0};
            bool dumpedFirst = false;
            Layout layout = Layout::Ds4Compat;  // default; auto-detected below
            OVERLAPPED ov{};
            ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            while (owner->m_running.load()) {
                ::ResetEvent(ov.hEvent);
                DWORD got = 0;
                BOOL ok = ::ReadFile(hidHandle, buf, sizeof(buf), &got, &ov);
                if (!ok && ::GetLastError() == ERROR_IO_PENDING) {
                    DWORD wait = ::WaitForSingleObject(ov.hEvent, 1000);
                    if (wait != WAIT_OBJECT_0) {
                        if (!owner->m_running.load()) break;
                        continue;
                    }
                    ok = ::GetOverlappedResult(hidHandle, &ov, &got, FALSE);
                }
                if (!ok || got == 0) {
                    qInfo() << "[DualSense] disconnected";
                    break;
                }
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
                if (got < 10) continue;

                if (!dumpedFirst) {
                    dumpedFirst = true;
                    // Auto-detect layout: dpad-neutral is 0x8 in the low nibble
                    // of whichever button byte the device uses. Check both
                    // candidate positions and pick whichever looks neutral at
                    // the moment of first read (user isn't pressing anything).
                    const BYTE lo5 = report[5] & 0x0F;
                    const BYTE lo8 = report[8] & 0x0F;
                    if (lo8 == 0x8 && lo5 != 0x8) layout = Layout::DualSenseFull;
                    else                          layout = Layout::Ds4Compat;

                    const int n = qMin(DWORD(16), got);
                    QString hex;
                    for (int k = 0; k < n; ++k) hex += QString::asprintf("%02X ", buf[k]);
                    qInfo().noquote() << QStringLiteral("[DualSense] first report (%1 bytes): %2  → layout=%3")
                                             .arg(got).arg(hex.trimmed())
                                             .arg(layout == Layout::DualSenseFull ? "DualSenseFull" : "Ds4Compat");
                }

                ControllerState s = mapReport(report, layout);
                // Override application moved to FanOutCtrlSink — see
                // app/engine/LabsMainWindow.cpp. Doing it per-source loses
                // the race against XInputSource pushes; doing it at the
                // fan-out covers every source uniformly.
                if (owner->m_sink) owner->m_sink->pushState(s);
            }
            ::CloseHandle(ov.hEvent);
            ::CloseHandle(hidHandle);
            hidHandle = INVALID_HANDLE_VALUE;
        }
    }

    void start() {
        owner->m_running.store(true);
        readerThread = std::thread([this]{ runReader(); });
    }

    void stop() {
        owner->m_running.store(false);
        if (hidHandle != INVALID_HANDLE_VALUE) ::CancelIoEx(hidHandle, nullptr);
        if (readerThread.joinable()) readerThread.join();
        if (hidHandle != INVALID_HANDLE_VALUE) { ::CloseHandle(hidHandle); hidHandle = INVALID_HANDLE_VALUE; }
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
