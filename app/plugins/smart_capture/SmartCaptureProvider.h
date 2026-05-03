#pragma once

#include "../../core/ICaptureProvider.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace Labs {

class SmartCaptureProvider : public ICaptureProvider {
public:
    SmartCaptureProvider() = default;
    ~SmartCaptureProvider() override { close(); }

    bool       open(const CaptureConfig& cfg) override;
    void       close() override;
    bool       isOpen() const override;
    QByteArray grabFrame() override;
    int        width()  const override { return m_width; }
    int        height() const override { return m_height; }
    QString    backendName() const override { return QStringLiteral("DXGI"); }

private:
    bool createDuplication(HMONITOR monitor);
    HMONITOR monitorForWindow(HWND hwnd) const;

    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_duplication;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_staging;

    CaptureConfig m_cfg;
    int           m_width       = 0;
    int           m_height      = 0;
    bool          m_open        = false;
    bool          m_needsReinit = false;
};

} // namespace Labs
