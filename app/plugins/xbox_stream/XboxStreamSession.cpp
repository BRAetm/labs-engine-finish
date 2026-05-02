#include "XboxStreamSession.h"
#include "FrameBufferPool.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QProcess>
#include <QStandardPaths>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Labs {

namespace {

// LBSX framing — see app/sidecars/xbox-stream/README.md.
constexpr int     LBSX_HEADER_LEN = 9;

constexpr quint8 T_EVENT_JSON = 0x01;
constexpr quint8 T_VIDEO_NAL  = 0x02;
// constexpr quint8 T_AUDIO_OPUS = 0x03; // reserved
constexpr quint8 T_CMD_JSON   = 0x10;
constexpr quint8 T_GAMEPAD    = 0x11;

inline quint32 readU32BE(const char* p) {
    return  (quint32)(quint8)p[3]
         | ((quint32)(quint8)p[2] << 8)
         | ((quint32)(quint8)p[1] << 16)
         | ((quint32)(quint8)p[0] << 24);
}

inline void writeU32BE(char* p, quint32 v) {
    p[0] = char((v >> 24) & 0xFF);
    p[1] = char((v >> 16) & 0xFF);
    p[2] = char((v >>  8) & 0xFF);
    p[3] = char( v        & 0xFF);
}

QString sanitizeAccountKey(const QString& k) {
    QString out;
    out.reserve(k.size());
    for (QChar c : k) {
        if (c.isLetterOrNumber() || c == '-' || c == '_' || c == '.') out.append(c);
        else out.append('_');
    }
    if (out.isEmpty()) out = QStringLiteral("default");
    return out;
}

} // namespace

// ── Impl ────────────────────────────────────────────────────────────────────

struct XboxStreamSession::Impl {
    QProcess*  proc = nullptr;
    QByteArray stdoutBuf;
    QByteArray stderrBuf;

    // FFmpeg H.264 → BGRA
    const AVCodec*  codec     = nullptr;
    AVCodecContext* codecCtx  = nullptr;
    AVPacket*       packet    = nullptr;
    AVFrame*        frame     = nullptr;
    SwsContext*     sws       = nullptr;
    int             swsWidth  = 0;
    int             swsHeight = 0;

    // controller throttling
    ControllerState lastState {};
    bool            haveLastState = false;
};

// ── ctor / dtor ─────────────────────────────────────────────────────────────

XboxStreamSession::XboxStreamSession(QObject* parent, XboxSessionInfo info, QString sidecarDir)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
    , m_info(std::move(info))
    , m_sidecarDir(std::move(sidecarDir))
{
    if (m_info.label.isEmpty()) {
        m_info.label = (m_info.mode == XboxStreamMode::Cloud)
            ? QStringLiteral("xCloud · %1").arg(m_info.accountKey)
            : QStringLiteral("Xbox RP · %1%2").arg(
                  m_info.accountKey,
                  m_info.consoleId.isEmpty() ? QString()
                                             : QStringLiteral(" / %1").arg(m_info.consoleId));
    }
}

XboxStreamSession::~XboxStreamSession()
{
    stop();
}

// ── path helpers ────────────────────────────────────────────────────────────

QString XboxStreamSession::dataDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString dir  = base + QStringLiteral("/xbox/sessions/")
                         + sanitizeAccountKey(m_info.accountKey);
    QDir().mkpath(dir);
    return dir;
}

QString XboxStreamSession::tokensFile() const
{
    return dataDir() + QStringLiteral("/xbox-tokens.json");
}

QString XboxStreamSession::locateElectronExe() const
{
    const QString p = m_sidecarDir + QStringLiteral("/node_modules/electron/dist/electron.exe");
    return QFileInfo::exists(p) ? p : QString();
}

// ── IFrameSource ────────────────────────────────────────────────────────────

void XboxStreamSession::setSink(IFrameSink* sink)
{
    m_sink = sink;
}

bool XboxStreamSession::start()
{
    if (m_running.load()) return true;
    if (m_sidecarDir.isEmpty()) {
        qWarning() << "[xbox-stream/" << m_info.id << "] no sidecar dir";
        return false;
    }
    const QString electronExe = locateElectronExe();
    if (electronExe.isEmpty()) {
        qWarning() << "[xbox-stream/" << m_info.id << "] electron missing in" << m_sidecarDir;
        return false;
    }
    // Cloud mode signs in via the visible browser, no token cache is needed
    // beforehand. Home (Remote Play) requires cached XBL tokens.
    if (m_info.mode == XboxStreamMode::Home && !QFileInfo::exists(tokensFile())) {
        qWarning() << "[xbox-stream/" << m_info.id << "] no tokens — pair first";
        return false;
    }

    const QString modeStr = (m_info.mode == XboxStreamMode::Cloud)
                            ? QStringLiteral("cloud") : QStringLiteral("home");

    auto* proc = new QProcess(this);
    m_impl->proc = proc;
    proc->setWorkingDirectory(m_sidecarDir);
    proc->setProgram(electronExe);
    QStringList args = {
        QStringLiteral("."),
        QStringLiteral("--data-dir=%1").arg(dataDir()),
        QStringLiteral("--tokens-file=%1").arg(tokensFile()),
        QStringLiteral("--mode=%1").arg(modeStr),
    };
    if (!m_info.consoleId.isEmpty())
        args << QStringLiteral("--console-id=%1").arg(m_info.consoleId);
    proc->setArguments(args);
    proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(proc, &QProcess::readyReadStandardOutput, this, &XboxStreamSession::onStdoutReady);
    connect(proc, &QProcess::readyReadStandardError,  this, &XboxStreamSession::onStderrReady);
    connect(proc, &QProcess::finished, this,
        [this](int code, QProcess::ExitStatus) { onProcessFinished(code); });

    proc->start();
    if (!proc->waitForStarted(5000)) {
        qWarning() << "[xbox-stream/" << m_info.id << "] sidecar start failed:" << proc->errorString();
        proc->deleteLater();
        m_impl->proc = nullptr;
        return false;
    }

    // Cloud mode renders inside the visible browser; we don't get H.264 NALs
    // to decode in the plugin. Frames are captured externally via WGC against
    // the BrowserWindow HWND reported by the sidecar (see onSessionEvent).
    if (m_info.mode == XboxStreamMode::Home) {
        if (!initFfmpeg()) {
            qWarning() << "[xbox-stream/" << m_info.id << "] FFmpeg init failed";
            proc->kill();
            proc->deleteLater();
            m_impl->proc = nullptr;
            return false;
        }

        QJsonObject startCmd {
            { QStringLiteral("cmd"),       QStringLiteral("start_stream") },
            { QStringLiteral("mode"),      modeStr        },
            { QStringLiteral("consoleId"), m_info.consoleId },
        };
        sendCommand(QJsonDocument(startCmd).toJson(QJsonDocument::Compact));
    }
    // Cloud mode: nothing more to do. The sidecar opened the visible browser
    // window in main.js's whenReady → cloud branch and the user signs in there.

    m_running.store(true);
    return true;
}

void XboxStreamSession::stop()
{
    bool wasRunning = m_running.exchange(false);
    auto* proc = m_impl->proc;
    if (proc) {
        if (proc->state() == QProcess::Running) {
            const QByteArray stop = QJsonDocument(QJsonObject{
                { QStringLiteral("cmd"), QStringLiteral("stop") }
            }).toJson(QJsonDocument::Compact);
            sendCommand(stop);
            if (!proc->waitForFinished(1500)) proc->kill();
        }
        proc->deleteLater();
        m_impl->proc = nullptr;
    }

    finiFfmpeg();

    m_impl->stdoutBuf.clear();
    m_impl->stderrBuf.clear();
    m_impl->haveLastState = false;
    m_frameCount.store(0);
    (void)wasRunning;
}

// ── frame writing ──────────────────────────────────────────────────────────

void XboxStreamSession::sendFrame(quint8 type, const QByteArray& payload)
{
    QMutexLocker lock(&m_writeMx);
    auto* proc = m_impl->proc;
    if (!proc || proc->state() != QProcess::Running) return;
    QByteArray hdr;
    hdr.resize(LBSX_HEADER_LEN);
    hdr[0] = 'L'; hdr[1] = 'B'; hdr[2] = 'S'; hdr[3] = 'X';
    hdr[4] = char(type);
    writeU32BE(hdr.data() + 5, quint32(payload.size()));
    proc->write(hdr);
    if (!payload.isEmpty()) proc->write(payload);
}

void XboxStreamSession::sendCommand(const QByteArray& jsonCompact)
{
    sendFrame(T_CMD_JSON, jsonCompact);
}

// ── stdout LBSX parser + dispatch ──────────────────────────────────────────

void XboxStreamSession::onStdoutReady()
{
    auto* proc = m_impl->proc;
    if (!proc) return;
    m_impl->stdoutBuf.append(proc->readAllStandardOutput());

    for (;;) {
        if (m_impl->stdoutBuf.size() < LBSX_HEADER_LEN) return;
        const char* p = m_impl->stdoutBuf.constData();
        if (!(p[0] == 'L' && p[1] == 'B' && p[2] == 'S' && p[3] == 'X')) {
            int idx = m_impl->stdoutBuf.indexOf(QByteArray("LBSX", 4), 1);
            if (idx < 0) { m_impl->stdoutBuf.clear(); return; }
            m_impl->stdoutBuf.remove(0, idx);
            continue;
        }
        const quint8  type = quint8(p[4]);
        const quint32 len  = readU32BE(p + 5);
        if (len > 32u * 1024u * 1024u) {
            qWarning() << "[xbox-stream/" << m_info.id << "] implausible payload" << len << "— resync";
            m_impl->stdoutBuf.remove(0, LBSX_HEADER_LEN);
            continue;
        }
        if (quint32(m_impl->stdoutBuf.size()) < LBSX_HEADER_LEN + len) return;

        const char* payloadPtr = p + LBSX_HEADER_LEN;
        if (type == T_VIDEO_NAL) {
            feedNal(reinterpret_cast<const uint8_t*>(payloadPtr), int(len));
        } else if (type == T_EVENT_JSON) {
            QByteArray ev(payloadPtr, int(len));
            handleEventJson(ev);
        }
        m_impl->stdoutBuf.remove(0, LBSX_HEADER_LEN + int(len));
    }
}

void XboxStreamSession::onStderrReady()
{
    auto* proc = m_impl->proc;
    if (!proc) return;
    m_impl->stderrBuf.append(proc->readAllStandardError());
    int nl;
    while ((nl = m_impl->stderrBuf.indexOf('\n')) != -1) {
        const QByteArray line = m_impl->stderrBuf.left(nl).trimmed();
        m_impl->stderrBuf.remove(0, nl + 1);
        if (!line.isEmpty())
            qInfo().noquote() << "[xbox-sidecar/" << m_info.id << "]" << line;
    }
}

void XboxStreamSession::onProcessFinished(int exitCode)
{
    qInfo() << "[xbox-stream/" << m_info.id << "] sidecar exited, code=" << exitCode;
    if (m_impl->proc) {
        m_impl->proc->deleteLater();
        m_impl->proc = nullptr;
    }
    m_impl->stdoutBuf.clear();
    m_impl->stderrBuf.clear();
    m_impl->haveLastState = false;
    finiFfmpeg();
    m_running.store(false);
    emit finished(m_info.id, exitCode);
}

void XboxStreamSession::handleEventJson(const QByteArray& payload)
{
    // Forward verbatim to consumers first.
    emit event(m_info.id, payload);

    QJsonParseError err {};
    const auto doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QJsonObject o = doc.object();
    const QString type = o.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("video_size")) {
        const int w = o.value(QStringLiteral("width")).toInt();
        const int h = o.value(QStringLiteral("height")).toInt();
        qInfo() << "[xbox-stream/" << m_info.id << "] video size" << w << "x" << h;
        emit videoSize(m_info.id, w, h);
    } else if (type == QStringLiteral("error")) {
        const QString msg  = o.value(QStringLiteral("message")).toString();
        const QString code = o.value(QStringLiteral("code")).toString();
        qWarning().noquote() << "[xbox-stream/" << m_info.id << "] sidecar error" << code << ":" << msg;
    } else if (type == QStringLiteral("rtc_state")) {
        qInfo().noquote() << "[xbox-stream/" << m_info.id << "] rtc state:"
                          << o.value(QStringLiteral("state")).toString();
    } else if (type == QStringLiteral("window-hwnd")) {
        // Cloud mode: sidecar's visible BrowserWindow handle. JSON sends it
        // as a string ("12345678") because JS numbers don't fit a 64-bit HWND.
        bool ok = false;
        const quint64 raw = o.value(QStringLiteral("hwnd")).toString().toULongLong(&ok);
        if (ok && raw != 0) {
            m_windowHwnd = static_cast<quintptr>(raw);
            qInfo() << "[xbox-stream/" << m_info.id << "] window hwnd =" << raw;
            emit windowHwndReady(m_info.id, m_windowHwnd);
        }
    }
}

quintptr XboxStreamSession::windowHwnd() const
{
    return m_windowHwnd;
}

// ── controller forwarding ─────────────────────────────────────────────────

void XboxStreamSession::pushState(const ControllerState& state)
{
    if (!m_running.load()) return;
    if (m_impl->haveLastState
        && state.buttons       == m_impl->lastState.buttons
        && state.leftTrigger   == m_impl->lastState.leftTrigger
        && state.rightTrigger  == m_impl->lastState.rightTrigger
        && state.leftThumbX    == m_impl->lastState.leftThumbX
        && state.leftThumbY    == m_impl->lastState.leftThumbY
        && state.rightThumbX   == m_impl->lastState.rightThumbX
        && state.rightThumbY   == m_impl->lastState.rightThumbY)
        return;
    m_impl->lastState = state;
    m_impl->haveLastState = true;

    QJsonObject j {
        { QStringLiteral("buttons"),      static_cast<int>(state.buttons)      },
        { QStringLiteral("leftTrigger"),  static_cast<int>(state.leftTrigger)  },
        { QStringLiteral("rightTrigger"), static_cast<int>(state.rightTrigger) },
        { QStringLiteral("leftX"),        static_cast<int>(state.leftThumbX)   },
        { QStringLiteral("leftY"),        static_cast<int>(state.leftThumbY)   },
        { QStringLiteral("rightX"),       static_cast<int>(state.rightThumbX)  },
        { QStringLiteral("rightY"),       static_cast<int>(state.rightThumbY)  },
    };
    const QByteArray payload = QJsonDocument(j).toJson(QJsonDocument::Compact);
    // Marshal write to the session's owning thread — pushState is called from
    // the controller poll thread.
    QMetaObject::invokeMethod(this, [this, payload]() {
        sendFrame(T_GAMEPAD, payload);
    }, Qt::QueuedConnection);
}

// ── FFmpeg H.264 decode ───────────────────────────────────────────────────

bool XboxStreamSession::initFfmpeg()
{
    if (m_impl->codecCtx) return true;
    m_impl->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_impl->codec) return false;
    m_impl->codecCtx = avcodec_alloc_context3(m_impl->codec);
    if (!m_impl->codecCtx) return false;
    m_impl->codecCtx->thread_count = 0;
    m_impl->codecCtx->thread_type  = FF_THREAD_FRAME | FF_THREAD_SLICE;
    m_impl->codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    if (avcodec_open2(m_impl->codecCtx, m_impl->codec, nullptr) < 0) return false;
    m_impl->packet = av_packet_alloc();
    m_impl->frame  = av_frame_alloc();
    return m_impl->packet && m_impl->frame;
}

void XboxStreamSession::finiFfmpeg()
{
    if (m_impl->sws)      { sws_freeContext(m_impl->sws); m_impl->sws = nullptr; }
    if (m_impl->frame)    { av_frame_free(&m_impl->frame); }
    if (m_impl->packet)   { av_packet_free(&m_impl->packet); }
    if (m_impl->codecCtx) { avcodec_free_context(&m_impl->codecCtx); }
    m_impl->codec = nullptr;
    m_impl->swsWidth = m_impl->swsHeight = 0;
}

bool XboxStreamSession::feedNal(const uint8_t* data, int size)
{
    if (!m_impl->codecCtx || !m_impl->packet || !m_impl->frame) return false;

    m_impl->packet->data = const_cast<uint8_t*>(data);
    m_impl->packet->size = size;

    int rc = avcodec_send_packet(m_impl->codecCtx, m_impl->packet);
    if (rc < 0 && rc != AVERROR(EAGAIN)) {
        return false;
    }
    while (true) {
        rc = avcodec_receive_frame(m_impl->codecCtx, m_impl->frame);
        if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF) break;
        if (rc < 0) return false;

        AVFrame* src = m_impl->frame;
        const int w = src->width;
        const int h = src->height;
        if (w > 0 && h > 0) {
            if (w != m_impl->swsWidth || h != m_impl->swsHeight || !m_impl->sws) {
                if (m_impl->sws) { sws_freeContext(m_impl->sws); m_impl->sws = nullptr; }
                m_impl->sws = sws_getContext(w, h, (AVPixelFormat)src->format,
                                             w, h, AV_PIX_FMT_BGRA,
                                             SWS_LANCZOS, nullptr, nullptr, nullptr);
                m_impl->swsWidth  = w;
                m_impl->swsHeight = h;
                qInfo() << "[xbox-stream/" << m_info.id << "] decode size =" << w << "x" << h;
            }
            if (m_impl->sws) {
                const size_t needed = size_t(w) * size_t(h) * 4;
                auto slot = FrameBufferPool::global().acquire(needed);
                uint8_t* dstData[4]   = { reinterpret_cast<uint8_t*>(slot->data), nullptr, nullptr, nullptr };
                int      dstStride[4] = { w * 4, 0, 0, 0 };
                sws_scale(m_impl->sws, src->data, src->linesize, 0, h, dstData, dstStride);

                Frame f;
                f.width  = w;
                f.height = h;
                f.stride = w * 4;
                f.format = PixelFormat::BGRA8;
                f.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000;
                f.sessionId = 100 + m_info.id;   // see Frame.sessionId convention
                f.data = QByteArray::fromRawData(slot->data, int(needed));
                f._pool_holder = slot;

                m_frameCount.fetch_add(1, std::memory_order_relaxed);
                if (m_sink) m_sink->pushFrame(f);
            }
        }
        av_frame_unref(m_impl->frame);
    }
    return true;
}

} // namespace Labs
