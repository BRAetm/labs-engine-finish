#pragma once

#include "FrameTypes.h"

#include <QObject>
#include <QString>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace Labs {

struct CaptureDeviceInfo {
    int     Index = -1;
    QString FriendlyName;
    QString SymbolicLink;
    QString Vid;            // four hex chars, lowercase, e.g. "0fd9"
    int     Width  = 0;
    int     Height = 0;
    bool    IsSupported = false;
    QString UnsupportedReason;
};

class MfCaptureSource : public QObject, public IFrameSource {
    Q_OBJECT
public:
    explicit MfCaptureSource(QObject* parent = nullptr);
    ~MfCaptureSource() override;

    // Enumerate MF video-capture devices and probe each at 1920x1080 MJPG.
    // Devices whose VID isn't in the capture-card allowlist are filtered out.
    static std::vector<CaptureDeviceInfo> scanDevices();

    void setTarget(const CaptureDeviceInfo& info);

    // IFrameSource
    void setSink(IFrameSink* sink) override { m_sink = sink; }
    bool start() override;
    void stop()  override;
    bool isRunning()    const override { return m_running.load(); }
    qint64  frameCount() const override { return m_frameCount.load(); }
    QString targetLabel() const override { return m_targetLabel; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    IFrameSink*         m_sink = nullptr;
    QString             m_symbolicLink;
    QString             m_targetLabel;
    int                 m_deviceIndex = -1;

    std::atomic<bool>   m_running     { false };
    std::atomic<bool>   m_stopRequested { false };
    std::atomic<qint64> m_frameCount  { 0 };
    std::thread         m_worker;

    void workerLoop() noexcept;
};

} // namespace Labs
