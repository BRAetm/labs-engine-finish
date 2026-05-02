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

void DisplaySurface::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    QImage img;
    QImage scaled;
    QSize  target;
    qreal  dpr = 1.0;
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
    }

    if (img.isNull()) {
        p.setPen(QColor(120, 130, 150));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("waiting for stream…"));
        return;
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int x = (size().width()  - int(scaled.width()  / dpr)) / 2;
    const int y = (size().height() - int(scaled.height() / dpr)) / 2;
    p.drawImage(x, y, scaled);
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

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::DisplayPlugin();
}
