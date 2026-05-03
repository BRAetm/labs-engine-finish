#include "DisplayPlugin.h"

#include <QMetaObject>
#include <QMutexLocker>
#include <QPainter>
#include <QPalette>

namespace Labs {

// ── DisplaySurface ──────────────────────────────────────────────────────────

DisplaySurface::DisplaySurface(QWidget* parent) : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(8, 10, 16));
    setPalette(pal);
    setMinimumSize(640, 360);
}

void DisplaySurface::setImage(const QImage& img)
{
    {
        QMutexLocker lock(&m_mx);
        m_image       = img;
        m_scaledDirty = true;     // invalidate cache — new frame
    }
    update();
}

void DisplaySurface::setDetections(const QVector<InferenceDetection>& dets)
{
    {
        QMutexLocker lock(&m_mx);
        m_detections = dets;
    }
    update();   // QWidget::update() is documented thread-safe — posts an event.
}

void DisplaySurface::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    QImage img;
    QImage scaled;
    QSize  target;
    qreal  dpr = 1.0;
    QVector<InferenceDetection> dets;
    {
        QMutexLocker lock(&m_mx);
        img    = m_image;
        dpr    = devicePixelRatioF();
        target = size() * dpr;

        if (img.isNull()) {
            // empty state — fall through to placeholder render below
        } else if (m_scaledDirty || m_scaledFor != target || m_scaled.isNull()) {
            // Lanczos-resize once per new frame OR resize, not per repaint.
            m_scaled = img.scaled(target, Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);
            m_scaled.setDevicePixelRatio(dpr);
            m_scaledFor   = target;
            m_scaledDirty = false;
        }
        scaled = m_scaled;
        dets   = m_detections;
    }

    if (img.isNull()) {
        p.setPen(QColor(120, 130, 150));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("waiting for stream…"));
        return;
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int sw = int(scaled.width()  / dpr);
    const int sh = int(scaled.height() / dpr);
    const int x  = (size().width()  - sw) / 2;
    const int y  = (size().height() - sh) / 2;
    p.drawImage(x, y, scaled);

    if (dets.isEmpty()) return;

    // Overlay: detections are normalized 0..1 against the source frame.
    // Resolve them against the rendered image rect (NOT the widget rect)
    // so boxes align under the image's letterbox/pillarbox margins.
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont font = p.font();
    font.setPointSizeF(qMax(8.0, font.pointSizeF()));
    p.setFont(font);
    const QFontMetrics fm(font);

    for (const InferenceDetection& d : dets) {
        const int bx = x + int(d.x * sw);
        const int by = y + int(d.y * sh);
        const int bw = int(d.w * sw);
        const int bh = int(d.h * sh);
        if (bw <= 0 || bh <= 0) continue;

        QPen pen(QColor(0, 220, 130));
        pen.setWidth(2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(bx, by, bw, bh);

        const QString label = QStringLiteral("%1 %2%")
            .arg(d.classId)
            .arg(int(d.confidence * 100.0f + 0.5f));
        const QSize ts = fm.size(0, label);
        const int   pad = 3;
        QRect tagRect(bx, by - ts.height() - pad * 2, ts.width() + pad * 2, ts.height() + pad * 2);
        if (tagRect.top() < y) tagRect.moveTop(by);   // flip below if clipped above
        p.fillRect(tagRect, QColor(0, 220, 130, 200));
        p.setPen(QColor(10, 18, 12));
        p.drawText(tagRect.adjusted(pad, pad, -pad, -pad), Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

// ── DisplayPlugin ───────────────────────────────────────────────────────────

DisplayPlugin::DisplayPlugin()  = default;
DisplayPlugin::~DisplayPlugin() = default;

void DisplayPlugin::initialize(const PluginContext&) {}
void DisplayPlugin::shutdown() {}

QWidget* DisplayPlugin::createWidget(QWidget* parent)
{
    m_surface = new DisplaySurface(parent);
    return m_surface;
}

void DisplayPlugin::pushFrame(const Frame& frame)
{
    if (!frame.isValid() || !m_surface) return;

    // We use RGB32 instead of ARGB32 because WGC often returns 0-alpha
    // for hardware-accelerated windows (like Xbox Remote Play), which
    // would otherwise render as fully transparent (black).
    QImage::Format qfmt = QImage::Format_Invalid;
    switch (frame.format) {
        case PixelFormat::BGRA8: qfmt = QImage::Format_RGB32; break;
        case PixelFormat::RGBA8: qfmt = QImage::Format_RGBX8888; break;
        default: return;
    }
    // Wrap the frame's pixels as a non-owning QImage view. The Guard
    // holds BOTH the QByteArray refcount AND the FrameBufferPool slot's
    // shared_ptr so the underlying memory survives until the QImage
    // (and any shared copies) are destroyed.
    //
    // This replaces the old `img.copy()` path which deep-copied an 8 MB
    // buffer per 1080p frame. The pool_holder field is the critical
    // bit — when the source's Frame goes out of scope, the shared_ptr
    // refcount stays at 1 here, keeping the pool slot reserved until
    // Display is done with it.
    struct Guard {
        QByteArray             data;
        std::shared_ptr<void>  pool_holder;
    };
    auto* guard = new Guard{ frame.data, frame._pool_holder };
    QImage img(reinterpret_cast<uchar*>(const_cast<char*>(guard->data.constData())),
               frame.width, frame.height, frame.stride, qfmt,
               [](void* p) { delete static_cast<Guard*>(p); }, guard);

    QPointer<DisplaySurface> surface = m_surface;
    QMetaObject::invokeMethod(surface, [surface, img]() {
        if (surface) surface->setImage(img);
    }, Qt::QueuedConnection);
}

void DisplayPlugin::onResults(const InferenceResults& r)
{
    if (!m_surface) return;

    // Drop replays — sequence is monotonic per-processor (see IFrameProcessor.h).
    // Per-processor scope is fine here since the engine wires a single processor
    // to this sink today; if multiple processors land, sequence collisions become
    // possible and we'll need to key by source.
    if (r.sequence != 0 && r.sequence == m_lastSeq) return;
    m_lastSeq = r.sequence;

    QPointer<DisplaySurface> surface = m_surface;
    const QVector<InferenceDetection> dets = r.detections;   // deep-copy, refcounted
    QMetaObject::invokeMethod(surface, [surface, dets]() {
        if (surface) surface->setDetections(dets);
    }, Qt::QueuedConnection);
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::DisplayPlugin();
}
