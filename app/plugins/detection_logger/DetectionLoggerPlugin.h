#pragma once

#include "IPlugin.h"
#include "IFrameProcessor.h"

#include <QFile>
#include <QMutex>
#include <QObject>
#include <QString>

namespace Labs {

// IInferenceResultsSink that appends one JSONL record per detection to
// %APPDATA%/Labs/detections-YYYYMMDD-HHMMSS.jsonl. The file is opened
// lazily on the first detection and held open for the plugin's lifetime;
// shutdown() closes it. onResults() runs on the producing processor's
// worker thread, so all file I/O is mutex-guarded.
class DetectionLoggerPlugin : public QObject,
                                public IInferenceResultsSinkPlugin,
                                public IInferenceResultsSink {
    Q_OBJECT
public:
    DetectionLoggerPlugin();
    ~DetectionLoggerPlugin() override;

    QString name()        const override { return QStringLiteral("Detection Logger"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Appends inference detections to JSONL"); }
    QString version()     const override { return QStringLiteral("0.1.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;
    QObject* qobject() override { return this; }

    IInferenceResultsSink* inferenceResultsSink() override { return this; }

    void onResults(const InferenceResults& results) override;

private:
    bool ensureOpen();   // call under m_mx

    QMutex   m_mx;
    QFile    m_file;
    quint32  m_lastSeq = 0;
};

} // namespace Labs
