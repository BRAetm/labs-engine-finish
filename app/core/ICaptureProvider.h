#pragma once

#include "LabsCore.h"
#include "IPlugin.h"

#include <QString>
#include <QByteArray>

#include <windows.h>

namespace Labs {

class ICaptureProvider {
public:
    struct CaptureConfig {
        HWND   targetWindow  = nullptr;
        int    targetWidth   = 0;
        int    targetHeight  = 0;
        int    targetFps     = 60;
        bool   useHWAccel    = true;
    };

    virtual ~ICaptureProvider() = default;

    virtual bool        open(const CaptureConfig& cfg) = 0;
    virtual void        close() = 0;
    virtual bool        isOpen() const = 0;
    virtual QByteArray  grabFrame() = 0;
    virtual int         width()  const = 0;
    virtual int         height() const = 0;
    virtual QString     backendName() const = 0;
};

class ICaptureProviderPlugin : public virtual IPlugin {
public:
    ~ICaptureProviderPlugin() override = default;
    virtual ICaptureProvider* captureProvider() = 0;
};

} // namespace Labs
