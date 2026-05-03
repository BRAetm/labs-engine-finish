#include "SmartCapturePlugin.h"

#include "../../core/SettingsManager.h"

#include <QDateTime>

namespace Labs {

SmartCapturePlugin::SmartCapturePlugin()
    : m_fps(60), m_hwAccel(true)
{}

SmartCapturePlugin::~SmartCapturePlugin()
{
    shutdown();
}

void SmartCapturePlugin::initialize(const PluginContext& ctx)
{
    if (ctx.settings) {
        m_pattern  = ctx.settings->value(QStringLiteral("smart_capture/process_pattern"), QStringLiteral("*")).toString();
        m_fps      = ctx.settings->value(QStringLiteral("smart_capture/fps"), 60).toInt();
        m_hwAccel  = ctx.settings->value(QStringLiteral("smart_capture/hw_accel"), true).toBool();
    }

    m_monitor.setPattern(m_pattern);

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &SmartCapturePlugin::onTick);
}

void SmartCapturePlugin::shutdown()
{
    stop();
    m_provider.close();
}

void SmartCapturePlugin::setSink(IFrameSink* sink)
{
    m_sink = sink;
}

bool SmartCapturePlugin::start()
{
    if (m_running)
        return true;
    if (!m_timer)
        return false;

    m_running    = true;
    m_frameCount = 0;
    m_timer->start(1000 / qMax(1, m_fps));
    return true;
}

void SmartCapturePlugin::stop()
{
    if (!m_running)
        return;
    m_running = false;
    if (m_timer)
        m_timer->stop();
    m_provider.close();
    m_lastHwnd = nullptr;
}

bool SmartCapturePlugin::isRunning() const
{
    return m_running;
}

qint64 SmartCapturePlugin::frameCount() const
{
    return m_frameCount;
}

void SmartCapturePlugin::onTick()
{
    if (!m_running || !m_sink)
        return;

    HWND hwnd = m_monitor.findWindow();
    ensureOpen(hwnd);

    if (!m_provider.isOpen())
        return;

    QByteArray pixels = m_provider.grabFrame();
    if (pixels.isEmpty())
        return;

    Frame f;
    f.data        = std::move(pixels);
    f.width       = m_provider.width();
    f.height      = m_provider.height();
    f.stride      = f.width * 4;
    f.format      = PixelFormat::BGRA8;
    f.timestampUs = QDateTime::currentMSecsSinceEpoch() * 1000LL;

    m_sink->pushFrame(f);
    ++m_frameCount;
}

void SmartCapturePlugin::ensureOpen(HWND hwnd)
{
    // Re-open when the target window changed or the provider lost access.
    bool needsOpen = !m_provider.isOpen();

    if (hwnd != m_lastHwnd) {
        m_lastHwnd = hwnd;
        needsOpen  = true;
    }

    if (needsOpen) {
        m_provider.close();
        ICaptureProvider::CaptureConfig cfg;
        cfg.targetWindow = hwnd;
        cfg.targetFps    = m_fps;
        cfg.useHWAccel   = m_hwAccel;
        m_provider.open(cfg);
    }
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::SmartCapturePlugin();
}
