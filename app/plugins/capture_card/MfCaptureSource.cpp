#include "MfCaptureSource.h"
#include "FrameBufferPool.h"

#include <QDebug>

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cwchar>
#include <unordered_set>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

namespace {

constexpr int kTargetWidth  = 1920;
constexpr int kTargetHeight = 1080;
constexpr int kTargetFps    = 60;

// USB vendor IDs of supported capture-card vendors. Anything else (built-in
// laptop webcams, Logitech, OBS Virtual Camera, NVIDIA Broadcast, etc.) gets
// filtered out so the picker only shows real cards.
const std::unordered_set<std::wstring>& vidAllowlist()
{
    static const std::unordered_set<std::wstring> kVids = {
        L"0fd9", // Elgato
        L"07ca", // AVerMedia
        L"2935", // Magewell
        L"1532", // Razer
        L"534d", // MS2109 / MS2130 generic
        L"1de1", // generic USB video
        L"eb1a", // eMPIA
        L"1f4d", // G-Tek
        L"2040", // Hauppauge
        L"1c6b", // Blackmagic
        L"0458", // Genius
        L"06f8", // Hercules
        L"2b73", // Elgato alt
    };
    return kVids;
}

// Pull the four-char VID out of an MF symbolic link. The link contains
// "vid_XXXX" (lowercase) somewhere in the device interface path.
QString parseVidLower(const std::wstring& sym)
{
    // case-insensitive search for "vid_"
    auto npos = std::wstring::npos;
    auto pos  = npos;
    for (size_t i = 0; i + 4 <= sym.size(); ++i) {
        if ((sym[i]   == L'v' || sym[i]   == L'V') &&
            (sym[i+1] == L'i' || sym[i+1] == L'I') &&
            (sym[i+2] == L'd' || sym[i+2] == L'D') &&
            (sym[i+3] == L'_'))
        { pos = i; break; }
    }
    if (pos == npos || pos + 8 > sym.size()) return {};
    std::wstring v = sym.substr(pos + 4, 4);
    for (auto& c : v) c = towlower(c);
    return QString::fromWCharArray(v.c_str(), 4);
}

struct MfRuntimeGuard {
    bool started = false;
    MfRuntimeGuard()
    {
        // CoInitialize is no-op safe across threads. MF needs it; calling on
        // worker too is fine — each thread inits independently.
        ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(::MFStartup(MF_VERSION, MFSTARTUP_FULL))) started = true;
    }
    ~MfRuntimeGuard()
    {
        if (started) ::MFShutdown();
        // Don't CoUninitialize — Qt and other plugins may still depend on it.
    }
};

// Try setting MJPG / 1920x1080 / 60fps on a source reader. Reads back the
// actual delivered size (some adapters ignore parts of our request and
// fall back to lower res) and writes to outW/outH.
HRESULT applyMjpg1080p60(IMFSourceReader* reader, int& outW, int& outH)
{
    ComPtr<IMFMediaType> mt;
    HRESULT hr = ::MFCreateMediaType(&mt);
    if (FAILED(hr)) return hr;
    mt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mt->SetGUID(MF_MT_SUBTYPE,    MFVideoFormat_MJPG);
    ::MFSetAttributeSize(mt.Get(), MF_MT_FRAME_SIZE, kTargetWidth, kTargetHeight);
    ::MFSetAttributeRatio(mt.Get(), MF_MT_FRAME_RATE, kTargetFps, 1);
    mt->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    hr = reader->SetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mt.Get());
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaType> actual;
    if (SUCCEEDED(reader->GetCurrentMediaType(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &actual)) && actual) {
        UINT32 w = 0, h = 0;
        if (SUCCEEDED(::MFGetAttributeSize(actual.Get(), MF_MT_FRAME_SIZE, &w, &h))) {
            outW = static_cast<int>(w);
            outH = static_cast<int>(h);
        }
    }
    return S_OK;
}

} // namespace

namespace Labs {

struct MfCaptureSource::Impl {
    MfRuntimeGuard mfGuard;
    ComPtr<IMFSourceReader> reader;
    ComPtr<IWICImagingFactory> wic;

    bool openReader(const QString& symbolicLink, int& outW, int& outH)
    {
        ComPtr<IMFAttributes> srcAttr;
        if (FAILED(::MFCreateAttributes(&srcAttr, 2))) return false;
        srcAttr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                         MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        const std::wstring sym = symbolicLink.toStdWString();
        srcAttr->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                           sym.c_str());

        ComPtr<IMFMediaSource> source;
        if (FAILED(::MFCreateDeviceSource(srcAttr.Get(), &source))) return false;

        ComPtr<IMFAttributes> rdrAttr;
        if (FAILED(::MFCreateAttributes(&rdrAttr, 1))) return false;
        rdrAttr->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, FALSE);

        ComPtr<IMFSourceReader> r;
        if (FAILED(::MFCreateSourceReaderFromMediaSource(
                source.Get(), rdrAttr.Get(), &r))) return false;

        if (FAILED(applyMjpg1080p60(r.Get(), outW, outH))) return false;

        reader = std::move(r);
        return true;
    }
};

// ── life-cycle ──────────────────────────────────────────────────────────────

MfCaptureSource::MfCaptureSource(QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Impl>()) {}

MfCaptureSource::~MfCaptureSource() { stop(); }

// ── enumeration ─────────────────────────────────────────────────────────────

std::vector<CaptureDeviceInfo> MfCaptureSource::scanDevices()
{
    MfRuntimeGuard guard;
    std::vector<CaptureDeviceInfo> out;
    if (!guard.started) return out;

    ComPtr<IMFAttributes> attr;
    if (FAILED(::MFCreateAttributes(&attr, 1))) return out;
    attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    IMFActivate** devices = nullptr;
    UINT32 count = 0;
    if (FAILED(::MFEnumDeviceSources(attr.Get(), &devices, &count)) || !devices) {
        return out;
    }

    int candidateIndex = 0;
    for (UINT32 i = 0; i < count; ++i) {
        IMFActivate* dev = devices[i];
        if (!dev) continue;

        WCHAR* nameRaw = nullptr; UINT32 nameLen = 0;
        WCHAR* symRaw  = nullptr; UINT32 symLen  = 0;
        dev->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                &nameRaw, &nameLen);
        dev->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            &symRaw, &symLen);

        const std::wstring sym = symRaw ? symRaw : L"";
        const QString vid = parseVidLower(sym);

        if (!vid.isEmpty() &&
            vidAllowlist().count(vid.toStdWString()) > 0)
        {
            CaptureDeviceInfo info;
            info.Index        = candidateIndex++;
            info.FriendlyName = nameRaw ? QString::fromWCharArray(nameRaw, (int)nameLen)
                                        : QStringLiteral("Capture Device");
            info.SymbolicLink = QString::fromWCharArray(sym.c_str(), (int)sym.size());
            info.Vid          = vid;

            // Probe at 1080p60 MJPG — read back actual delivered size.
            ComPtr<IMFMediaSource> source;
            ComPtr<IMFAttributes> srcAttr;
            if (SUCCEEDED(::MFCreateAttributes(&srcAttr, 2))) {
                srcAttr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                 MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
                srcAttr->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                   sym.c_str());
                if (SUCCEEDED(::MFCreateDeviceSource(srcAttr.Get(), &source)) && source) {
                    ComPtr<IMFSourceReader> r;
                    if (SUCCEEDED(::MFCreateSourceReaderFromMediaSource(
                            source.Get(), nullptr, &r)))
                    {
                        int w = 0, h = 0;
                        applyMjpg1080p60(r.Get(), w, h);  // best-effort; reads actual back
                        info.Width  = w;
                        info.Height = h;
                        // Loose support check: any device that the OS opens AND
                        // delivers at least 720p qualifies. Capture cards routinely
                        // round 1080p to 1088 or fall back to native UYVY at 1080p
                        // when MJPG isn't honored — those are still usable.
                        if (w >= 1280 && h >= 720) {
                            info.IsSupported = true;
                        } else if (w > 0 && h > 0) {
                            info.IsSupported = true;
                            info.UnsupportedReason =
                                QStringLiteral("low res: %1x%2").arg(w).arg(h);
                        } else {
                            info.IsSupported = false;
                            info.UnsupportedReason =
                                QStringLiteral("device opened but no readable format");
                        }
                    }
                    source->Shutdown();
                }
            }
            out.push_back(std::move(info));
        }

        if (nameRaw) ::CoTaskMemFree(nameRaw);
        if (symRaw)  ::CoTaskMemFree(symRaw);
        dev->Release();
    }
    ::CoTaskMemFree(devices);
    return out;
}

// ── target / start / stop ───────────────────────────────────────────────────

void MfCaptureSource::setTarget(const CaptureDeviceInfo& info)
{
    m_symbolicLink = info.SymbolicLink;
    m_targetLabel  = info.FriendlyName;
    m_deviceIndex  = info.Index;
}

bool MfCaptureSource::start()
{
    if (m_running.load()) return true;
    if (m_symbolicLink.isEmpty()) {
        qWarning() << "[CaptureCard] start: no symbolic link";
        return false;
    }

    int w = 0, h = 0;
    if (!m_impl->openReader(m_symbolicLink, w, h)) {
        qWarning() << "[CaptureCard] failed to open source reader for"
                   << m_targetLabel;
        return false;
    }
    if (w != kTargetWidth || h != kTargetHeight) {
        qWarning() << "[CaptureCard] device delivered" << w << "x" << h
                   << "instead of 1920x1080 — refusing";
        m_impl->reader.Reset();
        return false;
    }

    if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_impl->wic))))
    {
        qWarning() << "[CaptureCard] WIC factory create failed";
        m_impl->reader.Reset();
        return false;
    }

    m_stopRequested.store(false);
    m_running.store(true);
    m_worker = std::thread([this]() { workerLoop(); });
    return true;
}

void MfCaptureSource::stop()
{
    if (!m_running.exchange(false)) return;
    m_stopRequested.store(true);
    if (m_impl && m_impl->reader) m_impl->reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
    if (m_worker.joinable()) m_worker.join();
    if (m_impl) {
        m_impl->reader.Reset();
        m_impl->wic.Reset();
    }
}

// ── worker thread ───────────────────────────────────────────────────────────

void MfCaptureSource::workerLoop() noexcept
{
    // Worker thread needs its own COM init — MF callbacks expect it.
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    static constexpr size_t kMaxFramePayload =
        static_cast<size_t>(kTargetWidth) * kTargetHeight * 4;

    while (!m_stopRequested.load()) {
        DWORD streamIndex = 0;
        DWORD flags       = 0;
        LONGLONG ts100ns  = 0;
        ComPtr<IMFSample> sample;
        HRESULT hr = m_impl->reader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0, &streamIndex, &flags, &ts100ns, &sample);
        if (FAILED(hr)) {
            // Driver hiccup — most cards recover after Flush+retry.
            ::Sleep(5);
            continue;
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
        if (!sample) continue;

        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) || !buffer) continue;

        BYTE*  jpegBytes = nullptr;
        DWORD  maxLen    = 0;
        DWORD  curLen    = 0;
        if (FAILED(buffer->Lock(&jpegBytes, &maxLen, &curLen)) || curLen == 0) continue;

        // Decode MJPG payload to BGRA via WIC.
        ComPtr<IWICStream> stream;
        if (FAILED(m_impl->wic->CreateStream(&stream))) {
            buffer->Unlock(); continue;
        }
        if (FAILED(stream->InitializeFromMemory(jpegBytes, curLen))) {
            buffer->Unlock(); continue;
        }

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(m_impl->wic->CreateDecoderFromStream(
                stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)))
        {
            buffer->Unlock(); continue;
        }
        ComPtr<IWICBitmapFrameDecode> frameWic;
        if (FAILED(decoder->GetFrame(0, &frameWic))) { buffer->Unlock(); continue; }

        UINT fw = 0, fh = 0;
        if (FAILED(frameWic->GetSize(&fw, &fh)) || fw == 0 || fh == 0) {
            buffer->Unlock(); continue;
        }

        ComPtr<IWICFormatConverter> conv;
        if (FAILED(m_impl->wic->CreateFormatConverter(&conv))) {
            buffer->Unlock(); continue;
        }
        if (FAILED(conv->Initialize(frameWic.Get(),
                GUID_WICPixelFormat32bppBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0,
                WICBitmapPaletteTypeCustom)))
        {
            buffer->Unlock(); continue;
        }

        const int frameWidth  = static_cast<int>(fw);
        const int frameHeight = static_cast<int>(fh);
        const size_t rowBytes = static_cast<size_t>(frameWidth) * 4;
        const size_t total    = rowBytes * static_cast<size_t>(frameHeight);

        if (total > kMaxFramePayload) {
            buffer->Unlock();
            continue;
        }

        auto slot = FrameBufferPool::global().acquire(total);
        if (FAILED(conv->CopyPixels(nullptr,
                static_cast<UINT>(rowBytes),
                static_cast<UINT>(total),
                reinterpret_cast<BYTE*>(slot->data))))
        {
            buffer->Unlock(); continue;
        }
        buffer->Unlock();

        Frame out;
        out.width       = frameWidth;
        out.height      = frameHeight;
        out.stride      = static_cast<int>(rowBytes);
        out.format      = PixelFormat::BGRA8;
        out.timestampUs = ts100ns / 10; // 100ns ticks → microseconds
        out.data        = QByteArray::fromRawData(slot->data, static_cast<int>(total));
        out._pool_holder = slot;

        m_frameCount.fetch_add(1, std::memory_order_relaxed);
        if (m_sink) m_sink->pushFrame(out);
    }

    ::CoUninitialize();
}

} // namespace Labs
