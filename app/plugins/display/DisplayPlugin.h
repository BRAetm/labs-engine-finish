#pragma once

#include "IPlugin.h"
#include "IFrameProcessor.h"

#include <QImage>
#include <QMutex>
#include <QPointer>
#include <QVector>
#include <QWidget>

namespace Labs {

class DisplaySurface : public QWidget {
    Q_OBJECT
public:
    explicit DisplaySurface(QWidget* parent = nullptr);
    void setImage(const QImage& img);
    // Thread-safe; can be called from any thread. Stores the latest
    // detections and triggers a repaint. Coords stay normalized — paintEvent
    // resolves them against the current rendered image rect.
    void setDetections(const QVector<InferenceDetection>& dets);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_image;
    QMutex m_mx;
    // Lanczos-resize cache. paintEvent rebuilds m_scaled only when the
    // input (m_image) or target (m_scaledFor) changes — Qt may repaint
    // for reasons unrelated to a new frame (focus, overlay, hover) and
    // each redundant scale is ~5-10 ms of CPU at 1080p.
    QImage m_scaled;
    QSize  m_scaledFor;
    bool   m_scaledDirty = true;

    QVector<InferenceDetection> m_detections;
};

class DisplayPlugin : public QObject,
                       public IUIPlugin,
                       public IFrameSinkPlugin,
                       public IFrameSink,
                       public IInferenceResultsSinkPlugin,
                       public IInferenceResultsSink {
    Q_OBJECT
public:
    DisplayPlugin();
    ~DisplayPlugin() override;

    QString name()        const override { return QStringLiteral("Display"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Renders captured frames + detection overlay"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;

    QObject* qobject() override { return this; }
    QWidget* createWidget(QWidget* parent) override;

    // IFrameSinkPlugin
    IFrameSink* frameSink() override { return this; }

    // IFrameSink
    void pushFrame(const Frame& frame) override;

    // IInferenceResultsSinkPlugin
    IInferenceResultsSink* inferenceResultsSink() override { return this; }

    // IInferenceResultsSink — runs on the producing processor's worker thread.
    void onResults(const InferenceResults& results) override;

private:
    QPointer<DisplaySurface> m_surface;
    quint32                  m_lastSeq = 0;
};

} // namespace Labs
