#include "DetectionLoggerPlugin.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

namespace Labs {

DetectionLoggerPlugin::DetectionLoggerPlugin()  = default;
DetectionLoggerPlugin::~DetectionLoggerPlugin() = default;

void DetectionLoggerPlugin::initialize(const PluginContext&) {}

void DetectionLoggerPlugin::shutdown()
{
    QMutexLocker lock(&m_mx);
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
}

bool DetectionLoggerPlugin::ensureOpen()
{
    if (m_file.isOpen()) return true;

    const QString appData = QString::fromLocal8Bit(qgetenv("APPDATA"));
    if (appData.isEmpty()) return false;
    const QString labsDir = appData + QStringLiteral("/Labs");
    if (!QDir().mkpath(labsDir)) return false;

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    m_file.setFileName(labsDir + QStringLiteral("/detections-") + stamp + QStringLiteral(".jsonl"));
    return m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void DetectionLoggerPlugin::onResults(const InferenceResults& r)
{
    if (r.detections.isEmpty()) return;

    QMutexLocker lock(&m_mx);

    // Drop replays — sequence is monotonic per-processor, see IFrameProcessor.h.
    if (r.sequence != 0 && r.sequence == m_lastSeq) return;
    m_lastSeq = r.sequence;

    if (!ensureOpen()) return;

    QByteArray buf;
    buf.reserve(r.detections.size() * 128);
    for (const InferenceDetection& d : r.detections) {
        buf += "{\"ts_us\":";    buf += QByteArray::number(qlonglong(r.frameTimestampUs));
        buf += ",\"session\":";  buf += QByteArray::number(r.sessionId);
        buf += ",\"seq\":";      buf += QByteArray::number(qulonglong(r.sequence));
        buf += ",\"class\":";    buf += QByteArray::number(d.classId);
        buf += ",\"conf\":";     buf += QByteArray::number(d.confidence, 'f', 4);
        buf += ",\"x\":";        buf += QByteArray::number(d.x, 'f', 6);
        buf += ",\"y\":";        buf += QByteArray::number(d.y, 'f', 6);
        buf += ",\"w\":";        buf += QByteArray::number(d.w, 'f', 6);
        buf += ",\"h\":";        buf += QByteArray::number(d.h, 'f', 6);
        buf += "}\n";
    }

    if (m_file.write(buf) > 0) m_file.flush();
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::DetectionLoggerPlugin();
}
