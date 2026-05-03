#pragma once

#include "IPlugin.h"
#include "IFrameProcessor.h"

#include <QObject>
#include <QString>

#include <atomic>
#include <memory>

namespace Labs {

class SettingsManager;

// IFrameProcessor backed by ONNX Runtime running in-process. Frames
// arrive on pushFrame() (any thread); a worker thread drains a single-
// slot queue (drop-old policy) and emits InferenceResults to the
// configured sink.
//
// Settings (under "onnx_inference/"):
//   model_path        — absolute path to the .onnx file (required)
//   input_size        — fallback for dynamic spatial dims (default 640)
//   conf_threshold    — per-anchor minimum class confidence (default 0.25)
//   nms_iou           — IoU above which we suppress a same-class box (0.45)
//   provider          — "cpu" (default) or "cuda" (requires GPU ORT build)
class OnnxInferencePlugin : public QObject,
                              public IFrameProcessorPlugin,
                              public IFrameProcessor {
    Q_OBJECT
public:
    OnnxInferencePlugin();
    ~OnnxInferencePlugin() override;

    QString name()        const override { return QStringLiteral("ONNX Inference"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("In-process ONNX Runtime IFrameProcessor (YOLOv8 detection)"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;
    QObject* qobject() override { return this; }

    IFrameProcessor* frameProcessor() override { return this; }

    void pushFrame(const Frame& frame) override;
    void setResultsSink(IInferenceResultsSink* sink) override;
    bool start() override;
    void stop()  override;
    bool isRunning() const override { return m_running.load(); }

private:
    class Worker;

    SettingsManager*               m_settings = nullptr;
    IInferenceResultsSink*         m_pendingSink = nullptr;  // staged before start()
    std::unique_ptr<Worker>        m_worker;
    std::atomic<bool>              m_running { false };
};

} // namespace Labs
