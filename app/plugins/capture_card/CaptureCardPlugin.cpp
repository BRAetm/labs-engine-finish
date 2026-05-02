#include "CaptureCardPlugin.h"
#include "CaptureDevicePickerDialog.h"

#include <QApplication>
#include <QWidget>

namespace Labs {

CaptureCardPlugin::CaptureCardPlugin()
    : m_source(std::make_unique<MfCaptureSource>(this)) {}

CaptureCardPlugin::~CaptureCardPlugin() = default;

void CaptureCardPlugin::initialize(const PluginContext& /*ctx*/) {}

void CaptureCardPlugin::shutdown()
{
    if (m_source) m_source->stop();
}

bool CaptureCardPlugin::pair(QWidget* parent)
{
    CaptureDevicePickerDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted) return false;
    if (!dlg.hasSelection()) return false;

    if (m_source) {
        m_source->stop();
        m_source->setTarget(dlg.selectedDevice());
    }
    return true;
}

bool CaptureCardPlugin::pickAndStart(quintptr parentHwnd)
{
    QWidget* parent = nullptr;
    if (parentHwnd) {
        // Best-effort parent lookup — if no QWidget owns this HWND, the dialog
        // still parents to the active app window via Qt's default behavior.
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (w && static_cast<quintptr>(w->winId()) == parentHwnd) {
                parent = w; break;
            }
        }
    }
    if (!pair(parent)) return false;
    return startCapture();
}

bool CaptureCardPlugin::startCapture()
{
    return m_source ? m_source->start() : false;
}

void CaptureCardPlugin::stopCapture()
{
    if (m_source) m_source->stop();
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::CaptureCardPlugin();
}
