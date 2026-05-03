#include "OnnxInferencePlugin.h"

#include "DetectionPostprocessor.h"
#include "FramePreprocessor.h"
#include "OnnxSession.h"
#include "SettingsManager.h"

#include <QDebug>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QThread>
#include <QWaitCondition>

#include <atomic>
#include <utility>
#include <vector>

namespace Labs {

class OnnxInferencePlugin::Worker : public QThread {
public:
    Worker(std::unique_ptr<OnnxSession> session,
           float confThreshold,
           float nmsIou)
        : m_session(std::move(session))
        , m_inputW(m_session->inputW())
        , m_inputH(m_session->inputH())
        , m_confThreshold(confThreshold)
        , m_nmsIou(nmsIou)
    {
    }

    void requestStop() {
        m_stop.store(true);
        QMutexLocker lk(&m_mx);
        m_cv.wakeAll();
    }

    // Single-slot drop-old: replace any unprocessed pending frame.
    // Capture is faster than inference, so we'd rather skip stale
    // frames than build a lag queue.
    void offerFrame(const Frame& frame) {
        QMutexLocker lk(&m_mx);
        m_pending     = frame;
        m_havePending = true;
        m_cv.wakeAll();
    }

    void setSink(IInferenceResultsSink* sink) { m_sink.store(sink); }

protected:
    void run() override {
        std::vector<float>          inputBuf(static_cast<size_t>(3) * m_inputH * m_inputW);
        std::vector<float>          outputData;
        std::vector<int64_t>        outputShape;
        QVector<InferenceDetection> dets;
        bool firstResultLogged = false;

        while (!m_stop.load()) {
            Frame frame;
            {
                QMutexLocker lk(&m_mx);
                while (!m_stop.load() && !m_havePending) m_cv.wait(&m_mx);
                if (m_stop.load()) break;
                frame         = m_pending;
                m_havePending = false;
            }
            if (!frame.isValid()) continue;

            const LetterboxParams lb = FramePreprocessor::toRgbFloatNCHW(
                frame, m_inputW, m_inputH, inputBuf.data());

            QString err;
            if (!m_session->run(inputBuf.data(), &outputData, &outputShape, &err)) {
                qWarning().nospace() << "OnnxInference: run failed — " << err;
                continue;
            }

            dets.clear();
            const bool ok = DetectionPostprocessor::parseYolov8(
                outputData.data(), outputShape,
                m_inputW, m_inputH,
                frame.width, frame.height,
                lb, m_confThreshold, m_nmsIou,
                &dets);
            if (!ok) {
                if (!firstResultLogged) {
                    QString shapeStr;
                    for (size_t i = 0; i < outputShape.size(); ++i) {
                        if (i) shapeStr += QStringLiteral(",");
                        shapeStr += QString::number(outputShape[i]);
                    }
                    qWarning().nospace()
                        << "OnnxInference: output shape ["
                        << shapeStr
                        << "] does not match expected YOLOv8 [1, 4+nc, N]; dropping.";
                    firstResultLogged = true;
                }
                continue;
            }

            const quint32 seq = ++m_seq;
            InferenceResults r;
            r.frameTimestampUs = frame.timestampUs;
            r.sessionId        = frame.sessionId;
            r.sequence         = seq;
            r.detections       = dets;

            if (!firstResultLogged) {
                QString shapeStr;
                for (size_t i = 0; i < outputShape.size(); ++i) {
                    if (i) shapeStr += QStringLiteral(",");
                    shapeStr += QString::number(outputShape[i]);
                }
                qInfo().nospace() << "OnnxInference: first result — src "
                                  << frame.width << "x" << frame.height
                                  << ", model " << m_inputW << "x" << m_inputH
                                  << ", out [" << shapeStr
                                  << "], dets=" << dets.size();
                firstResultLogged = true;
            }

            if (auto* sink = m_sink.load()) sink->onResults(r);
        }
    }

private:
    std::unique_ptr<OnnxSession>            m_session;
    int                                      m_inputW;
    int                                      m_inputH;
    float                                    m_confThreshold;
    float                                    m_nmsIou;

    QMutex                                   m_mx;
    QWaitCondition                           m_cv;
    Frame                                    m_pending;
    bool                                     m_havePending = false;
    std::atomic<bool>                        m_stop { false };
    std::atomic<IInferenceResultsSink*>      m_sink { nullptr };
    std::atomic<quint32>                     m_seq  { 0 };
};

OnnxInferencePlugin::OnnxInferencePlugin() = default;

OnnxInferencePlugin::~OnnxInferencePlugin() { stop(); }

void OnnxInferencePlugin::initialize(const PluginContext& ctx)
{
    m_settings = ctx.settings;
}

void OnnxInferencePlugin::shutdown() { stop(); }

void OnnxInferencePlugin::setResultsSink(IInferenceResultsSink* sink)
{
    if (m_worker) m_worker->setSink(sink);
    else          m_pendingSink = sink;
}

bool OnnxInferencePlugin::start()
{
    if (m_running.load()) return true;
    if (!m_settings) {
        qWarning() << "OnnxInference::start: no settings; refusing to start";
        return false;
    }

    const QString modelPath = m_settings->value(QStringLiteral("onnx_inference/model_path")).toString();
    if (modelPath.isEmpty()) {
        qWarning() << "OnnxInference::start: settings key 'onnx_inference/model_path' is empty.";
        return false;
    }
    if (!QFileInfo::exists(modelPath)) {
        qWarning().nospace() << "OnnxInference::start: model file not found: " << modelPath;
        return false;
    }

    const int     inputSize = m_settings->value(QStringLiteral("onnx_inference/input_size"),     640).toInt();
    const float   confThr   = m_settings->value(QStringLiteral("onnx_inference/conf_threshold"), 0.25f).toFloat();
    const float   nmsIou    = m_settings->value(QStringLiteral("onnx_inference/nms_iou"),        0.45f).toFloat();
    const QString prov      = m_settings->value(QStringLiteral("onnx_inference/provider"),       QStringLiteral("cpu")).toString();
    const auto provider     = (prov.compare(QStringLiteral("cuda"), Qt::CaseInsensitive) == 0)
                                ? OnnxSession::Provider::Cuda
                                : OnnxSession::Provider::Cpu;

    QString err;
    auto session = OnnxSession::create(modelPath, provider, inputSize, &err);
    if (!session) {
        qWarning().nospace() << "OnnxInference::start: session create failed — " << err;
        return false;
    }
    qInfo().nospace() << "OnnxInference: model loaded (" << modelPath
                      << "), input " << session->inputW() << "x" << session->inputH()
                      << ", provider=" << prov << ", conf=" << confThr
                      << ", nmsIou=" << nmsIou;

    m_worker = std::make_unique<Worker>(std::move(session), confThr, nmsIou);
    if (m_pendingSink) {
        m_worker->setSink(m_pendingSink);
        m_pendingSink = nullptr;
    }
    m_worker->start();
    m_running.store(true);
    return true;
}

void OnnxInferencePlugin::stop()
{
    if (!m_running.exchange(false)) return;
    if (m_worker) {
        m_worker->requestStop();
        m_worker->wait(2000);
        m_worker.reset();
    }
}

void OnnxInferencePlugin::pushFrame(const Frame& frame)
{
    if (!m_running.load() || !m_worker) return;
    m_worker->offerFrame(frame);
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::OnnxInferencePlugin();
}
