#include "SmartCaptureProvider.h"

#include <QByteArray>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace Labs {

bool SmartCaptureProvider::open(const CaptureConfig& cfg)
{
    close();
    m_cfg         = cfg;
    m_needsReinit = false;

    HMONITOR monitor = cfg.targetWindow
        ? monitorForWindow(cfg.targetWindow)
        : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

    return createDuplication(monitor);
}

void SmartCaptureProvider::close()
{
    m_staging.Reset();
    m_duplication.Reset();
    m_context.Reset();
    m_device.Reset();
    m_width  = 0;
    m_height = 0;
    m_open   = false;
}

bool SmartCaptureProvider::isOpen() const
{
    return m_open;
}

QByteArray SmartCaptureProvider::grabFrame()
{
    if (!m_open || m_needsReinit)
        return {};

    DXGI_OUTDUPL_FRAME_INFO info{};
    Microsoft::WRL::ComPtr<IDXGIResource> resource;

    HRESULT hr = m_duplication->AcquireNextFrame(0, &info, &resource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        return {};
    if (hr == DXGI_ERROR_ACCESS_LOST) {
        m_needsReinit = true;
        return {};
    }
    if (FAILED(hr))
        return {};

    Microsoft::WRL::ComPtr<ID3D11Texture2D> desktopTex;
    resource.As(&desktopTex);

    int srcX = 0, srcY = 0;
    int cropW = m_width, cropH = m_height;

    if (m_cfg.targetWindow) {
        RECT clientRect{};
        GetClientRect(m_cfg.targetWindow, &clientRect);

        POINT tl{ clientRect.left, clientRect.top };
        MapWindowPoints(m_cfg.targetWindow, nullptr, &tl, 1);

        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(monitorForWindow(m_cfg.targetWindow), &mi);

        srcX  = tl.x - mi.rcMonitor.left;
        srcY  = tl.y - mi.rcMonitor.top;
        cropW = clientRect.right - clientRect.left;
        cropH = clientRect.bottom - clientRect.top;

        if (cropW <= 0 || cropH <= 0) {
            m_duplication->ReleaseFrame();
            return {};
        }
    }

    // Rebuild staging texture if crop dimensions changed.
    if (!m_staging) {
        D3D11_TEXTURE2D_DESC sd{};
        sd.Width          = static_cast<UINT>(cropW);
        sd.Height         = static_cast<UINT>(cropH);
        sd.MipLevels      = 1;
        sd.ArraySize      = 1;
        sd.Format         = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.SampleDesc     = { 1, 0 };
        sd.Usage          = D3D11_USAGE_STAGING;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        HRESULT hrs = m_device->CreateTexture2D(&sd, nullptr, &m_staging);
        if (FAILED(hrs)) {
            m_duplication->ReleaseFrame();
            return {};
        }
    }

    D3D11_BOX box{};
    box.left   = static_cast<UINT>(srcX);
    box.top    = static_cast<UINT>(srcY);
    box.front  = 0;
    box.right  = static_cast<UINT>(srcX + cropW);
    box.bottom = static_cast<UINT>(srcY + cropH);
    box.back   = 1;

    m_context->CopySubresourceRegion(m_staging.Get(), 0, 0, 0, 0, desktopTex.Get(), 0, &box);
    m_duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = m_context->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr))
        return {};

    const int rowBytes = cropW * 4;
    QByteArray result(rowBytes * cropH, Qt::Uninitialized);
    char* dst = result.data();

    for (int row = 0; row < cropH; ++row) {
        const char* src = static_cast<const char*>(mapped.pData) + row * mapped.RowPitch;
        memcpy(dst + row * rowBytes, src, static_cast<size_t>(rowBytes));
    }

    m_context->Unmap(m_staging.Get(), 0);
    return result;
}

bool SmartCaptureProvider::createDuplication(HMONITOR monitor)
{
    static const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, levels, 1, D3D11_SDK_VERSION,
        &m_device, &featureLevel, &m_context);
    if (FAILED(hr))
        return false;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDev;
    m_device.As(&dxgiDev);
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    dxgiDev->GetAdapter(&adapter);

    UINT outputIdx = 0;
    while (true) {
        Microsoft::WRL::ComPtr<IDXGIOutput> output;
        if (adapter->EnumOutputs(outputIdx++, &output) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_OUTPUT_DESC od{};
        output->GetDesc(&od);
        if (od.Monitor != monitor)
            continue;

        Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
        output.As(&output1);
        hr = output1->DuplicateOutput(m_device.Get(), &m_duplication);
        if (FAILED(hr))
            return false;

        DXGI_OUTDUPL_DESC dd{};
        m_duplication->GetDesc(&dd);
        m_width  = static_cast<int>(dd.ModeDesc.Width);
        m_height = static_cast<int>(dd.ModeDesc.Height);
        m_open   = true;
        return true;
    }
    return false;
}

HMONITOR SmartCaptureProvider::monitorForWindow(HWND hwnd) const
{
    RECT r{};
    GetWindowRect(hwnd, &r);
    HMONITOR mon = MonitorFromRect(&r, MONITOR_DEFAULTTONEAREST);
    return mon ? mon : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
}

} // namespace Labs
