#pragma once

#include "IPlugin.h"
#include "MfCaptureSource.h"

#include <memory>

namespace Labs {

class CaptureCardPlugin : public QObject,
                          public IFrameSourcePlugin,
                          public IPairablePlugin {
    Q_OBJECT
public:
    CaptureCardPlugin();
    ~CaptureCardPlugin() override;

    QString name()        const override { return QStringLiteral("Capture Card"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("USB capture card input via Windows Media Foundation"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;

    QObject*       qobject()     override { return this; }
    IFrameSource*  frameSource() override { return m_source.get(); }

    // Pair flow shows the device picker. On accept, the chosen device is set
    // on the source so the next start() opens the right card. We don't call
    // start() here — that's the engine's job once the user hits START ENGINE.
    bool pair(QWidget* parent) override;

    // Cross-DLL invokables — same shape as WgcCapturePlugin so LabsMainWindow
    // can drive capture without dynamic_cast across plugin boundaries.
    Q_INVOKABLE bool pickAndStart(quintptr parentHwnd);
    Q_INVOKABLE bool startCapture();
    Q_INVOKABLE void stopCapture();

private:
    std::unique_ptr<MfCaptureSource> m_source;
};

} // namespace Labs
