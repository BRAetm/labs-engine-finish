#include "ViGEmPlugin.h"

#include <QDebug>
#include <QMutexLocker>

#include <Windows.h>

// Static-linked ViGEmClient (vendored under app/third-party/ViGEmClient).
// No more LoadLibrary("ViGEmClient.dll") — the user-mode client is baked in.
// The ViGEmBus *kernel* driver still has to be installed for the bus endpoint
// to exist; we detect that and surface a clear message to the user.
#include "ViGEm/Client.h"

namespace Labs {

struct ViGEmSink::Impl {
    PVIGEM_CLIENT client = nullptr;
    PVIGEM_TARGET x360   = nullptr;
};

ViGEmSink::ViGEmSink(QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Impl>())
{
    m_impl->client = vigem_alloc();
    if (!m_impl->client) {
        m_status = QStringLiteral("vigem_alloc failed (out of memory)");
        qWarning() << "ViGEm:" << m_status;
        return;
    }

    const VIGEM_ERROR ec = vigem_connect(m_impl->client);
    if (!VIGEM_SUCCESS(ec)) {
        if (ec == VIGEM_ERROR_BUS_NOT_FOUND) {
            m_status = QStringLiteral("ViGEmBus driver not installed — run app/third-party/ViGEmBus/ViGEmBus_Setup.exe, then relaunch");
        } else {
            m_status = QStringLiteral("vigem_connect failed (0x%1)").arg(static_cast<quint32>(ec), 0, 16);
        }
        qWarning() << "ViGEm:" << m_status;
        vigem_free(m_impl->client);
        m_impl->client = nullptr;
        return;
    }

    m_impl->x360 = vigem_target_x360_alloc();
    if (!m_impl->x360) {
        m_status = QStringLiteral("vigem_target_x360_alloc failed");
        qWarning() << "ViGEm:" << m_status;
        vigem_disconnect(m_impl->client);
        vigem_free(m_impl->client);
        m_impl->client = nullptr;
        return;
    }

    const VIGEM_ERROR addEc = vigem_target_add(m_impl->client, m_impl->x360);
    if (!VIGEM_SUCCESS(addEc)) {
        m_status = QStringLiteral("vigem_target_add failed (0x%1)").arg(static_cast<quint32>(addEc), 0, 16);
        qWarning() << "ViGEm:" << m_status;
        vigem_target_free(m_impl->x360);
        vigem_disconnect(m_impl->client);
        vigem_free(m_impl->client);
        m_impl->client = nullptr;
        m_impl->x360   = nullptr;
        return;
    }

    m_status = QStringLiteral("virtual X360 pad online");
    m_ready.store(true);
    qInfo() << "ViGEm:" << m_status;
}

ViGEmSink::~ViGEmSink()
{
    QMutexLocker lock(&m_mx);
    if (m_impl->x360) {
        vigem_target_remove(m_impl->client, m_impl->x360);
        vigem_target_free(m_impl->x360);
        m_impl->x360 = nullptr;
    }
    if (m_impl->client) {
        vigem_disconnect(m_impl->client);
        vigem_free(m_impl->client);
        m_impl->client = nullptr;
    }
}

void ViGEmSink::pushState(const ControllerState& state)
{
    if (!m_ready.load()) return;

    XUSB_REPORT r {};
    r.wButtons      = state.buttons;
    r.bLeftTrigger  = state.leftTrigger;
    r.bRightTrigger = state.rightTrigger;
    r.sThumbLX      = state.leftThumbX;
    r.sThumbLY      = state.leftThumbY;
    r.sThumbRX      = state.rightThumbX;
    r.sThumbRY      = state.rightThumbY;

    QMutexLocker lock(&m_mx);
    if (m_impl->client && m_impl->x360) {
        vigem_target_x360_update(m_impl->client, m_impl->x360, r);
    }
}

ViGEmPlugin::ViGEmPlugin() : m_sink(std::make_unique<ViGEmSink>(this)) {}
ViGEmPlugin::~ViGEmPlugin() = default;

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::ViGEmPlugin();
}
