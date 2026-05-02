#include "ViGEmPlugin.h"

#include <QDebug>
#include <QMutexLocker>
#include <QThread>

#include <Windows.h>

// Static-linked ViGEmClient (vendored under app/third-party/ViGEmClient).
// No more LoadLibrary("ViGEmClient.dll") — the user-mode client is baked in.
// The ViGEmBus *kernel* driver still has to be installed for the bus endpoint
// to exist; we detect that and surface a clear message to the user.
#include "ViGEm/Client.h"
#include "ViGEm/Util.h"

namespace Labs {

struct ViGEmPlugin::Impl {
    PVIGEM_CLIENT client = nullptr;

    struct PadEntry {
        PVIGEM_TARGET            target      = nullptr;
        ViGEmPlugin::PadKind     kind        = ViGEmPlugin::PadKind::X360;
        int                      xinputIndex = -1;
        bool                     inUse       = false;
    };

    std::array<PadEntry, ViGEmPlugin::kMaxPads> pads {};
    mutable QMutex padsMx;
};

// --- helpers ----------------------------------------------------------------

namespace {

XUSB_REPORT toXusb(const ControllerState& s)
{
    XUSB_REPORT r {};
    r.wButtons      = s.buttons;
    r.bLeftTrigger  = s.leftTrigger;
    r.bRightTrigger = s.rightTrigger;
    r.sThumbLX      = s.leftThumbX;
    r.sThumbLY      = s.leftThumbY;
    r.sThumbRX      = s.rightThumbX;
    r.sThumbRY      = s.rightThumbY;
    return r;
}

DS4_REPORT toDs4(const ControllerState& s)
{
    DS4_REPORT out;
    DS4_REPORT_INIT(&out);
    XUSB_REPORT in = toXusb(s);
    XUSB_TO_DS4_REPORT(&in, &out);
    return out;
}

} // namespace

// --- ViGEmPlugin ------------------------------------------------------------

ViGEmPlugin::ViGEmPlugin()
    : m_impl(std::make_unique<Impl>())
{
    m_impl->client = vigem_alloc();
    if (!m_impl->client) {
        m_status = QStringLiteral("vigem_alloc failed (out of memory)");
        qWarning() << "ViGEm:" << m_status;
    } else {
        const VIGEM_ERROR ec = vigem_connect(m_impl->client);
        if (!VIGEM_SUCCESS(ec)) {
            if (ec == VIGEM_ERROR_BUS_NOT_FOUND) {
                m_status = QStringLiteral(
                    "ViGEmBus driver not installed — run "
                    "app/third-party/ViGEmBus/ViGEmBus_Setup.exe, then relaunch");
            } else {
                m_status = QStringLiteral("vigem_connect failed (0x%1)")
                                .arg(static_cast<quint32>(ec), 0, 16);
            }
            qWarning() << "ViGEm:" << m_status;
            vigem_free(m_impl->client);
            m_impl->client = nullptr;
        } else {
            m_status = QStringLiteral("ViGEm client online (pool of %1)")
                            .arg(kMaxPads);
            m_clientReady.store(true);
            qInfo() << "ViGEm:" << m_status;
        }
    }

    m_sink = std::make_unique<ViGEmSink>(this, this);
}

ViGEmPlugin::~ViGEmPlugin()
{
    shutdown();
}

void ViGEmPlugin::shutdown()
{
    // Free any pool-allocated pads first.
    for (int i = 0; i < kMaxPads; ++i) {
        releasePad(i);
    }

    QMutexLocker lock(&m_impl->padsMx);
    if (m_impl->client) {
        vigem_disconnect(m_impl->client);
        vigem_free(m_impl->client);
        m_impl->client = nullptr;
    }
    m_clientReady.store(false);
}

int ViGEmPlugin::allocatePad(int kindAsInt)
{
    if (!m_clientReady.load()) return -1;
    if (kindAsInt != 0 && kindAsInt != 1) return -1;

    const PadKind kind = (kindAsInt == 0) ? PadKind::X360 : PadKind::DS4;
    const int begin = (kind == PadKind::X360) ? 0       : kMaxX360;
    const int end   = (kind == PadKind::X360) ? kMaxX360 : kMaxPads;

    PVIGEM_CLIENT clientCopy = nullptr;
    int slot = -1;

    {
        QMutexLocker lock(&m_impl->padsMx);
        clientCopy = m_impl->client;
        if (!clientCopy) return -1;
        for (int i = begin; i < end; ++i) {
            if (!m_impl->pads[i].inUse) {
                slot = i;
                // Reserve the slot before releasing the lock so a concurrent
                // allocate doesn't grab the same index. We finalize the
                // target/kind under the lock at the end.
                m_impl->pads[i].inUse       = true;
                m_impl->pads[i].kind        = kind;
                m_impl->pads[i].target      = nullptr;
                m_impl->pads[i].xinputIndex = -1;
                break;
            }
        }
    }

    if (slot < 0) return -1;

    PVIGEM_TARGET target = (kind == PadKind::X360)
                               ? vigem_target_x360_alloc()
                               : vigem_target_ds4_alloc();
    if (!target) {
        QMutexLocker lock(&m_impl->padsMx);
        m_impl->pads[slot].inUse = false;
        qWarning() << "ViGEm: target alloc failed for slot" << slot;
        return -1;
    }

    const VIGEM_ERROR addEc = vigem_target_add(clientCopy, target);
    if (!VIGEM_SUCCESS(addEc)) {
        vigem_target_free(target);
        QMutexLocker lock(&m_impl->padsMx);
        m_impl->pads[slot].inUse = false;
        qWarning() << "ViGEm: vigem_target_add failed for slot" << slot
                   << "ec=" << QString::number(static_cast<quint32>(addEc), 16);
        return -1;
    }

    int xidx = -1;
    if (kind == PadKind::X360) {
        // Bus driver assigns the XInput slot asynchronously; retry briefly.
        for (int attempt = 0; attempt < 25; ++attempt) {
            ULONG idx = 0;
            const VIGEM_ERROR ec = vigem_target_x360_get_user_index(
                clientCopy, target, &idx);
            if (VIGEM_SUCCESS(ec) && idx <= 3) {
                xidx = static_cast<int>(idx);
                break;
            }
            QThread::msleep(20);
        }
    }

    {
        QMutexLocker lock(&m_impl->padsMx);
        m_impl->pads[slot].target      = target;
        m_impl->pads[slot].kind        = kind;
        m_impl->pads[slot].xinputIndex = xidx;
        m_impl->pads[slot].inUse       = true;
    }

    qInfo() << "ViGEm: allocated pad slot=" << slot
            << "kind=" << (kind == PadKind::X360 ? "X360" : "DS4")
            << "xinputIndex=" << xidx;
    return slot;
}

void ViGEmPlugin::releasePad(int padId)
{
    if (padId < 0 || padId >= kMaxPads) return;

    PVIGEM_CLIENT clientCopy = nullptr;
    PVIGEM_TARGET targetCopy = nullptr;

    {
        QMutexLocker lock(&m_impl->padsMx);
        if (!m_impl->pads[padId].inUse && !m_impl->pads[padId].target) return;
        clientCopy = m_impl->client;
        targetCopy = m_impl->pads[padId].target;
        m_impl->pads[padId].target      = nullptr;
        m_impl->pads[padId].xinputIndex = -1;
        m_impl->pads[padId].inUse       = false;
    }

    if (clientCopy && targetCopy) {
        vigem_target_remove(clientCopy, targetCopy);
        vigem_target_free(targetCopy);
    } else if (targetCopy) {
        vigem_target_free(targetCopy);
    }
}

bool ViGEmPlugin::pushStateTo(int padId, const Labs::ControllerState& state)
{
    if (padId < 0 || padId >= kMaxPads) return false;
    if (!m_clientReady.load()) return false;

    PVIGEM_CLIENT clientCopy = nullptr;
    PVIGEM_TARGET targetCopy = nullptr;
    PadKind       kindCopy   = PadKind::X360;

    {
        QMutexLocker lock(&m_impl->padsMx);
        const auto& e = m_impl->pads[padId];
        if (!e.inUse || !e.target) return false;
        clientCopy = m_impl->client;
        targetCopy = e.target;
        kindCopy   = e.kind;
    }

    VIGEM_ERROR ec = VIGEM_ERROR_NONE;
    if (kindCopy == PadKind::X360) {
        XUSB_REPORT r = toXusb(state);
        ec = vigem_target_x360_update(clientCopy, targetCopy, r);
    } else {
        DS4_REPORT r = toDs4(state);
        ec = vigem_target_ds4_update(clientCopy, targetCopy, r);
    }

    if (ec == VIGEM_ERROR_TARGET_NOT_PLUGGED_IN) {
        qWarning() << "ViGEm: target on slot" << padId
                   << "reports not-plugged; auto-freeing";
        releasePad(padId);
        return false;
    }
    return VIGEM_SUCCESS(ec);
}

int ViGEmPlugin::xinputIndexOf(int padId) const
{
    if (padId < 0 || padId >= kMaxPads) return -1;
    QMutexLocker lock(&m_impl->padsMx);
    const auto& e = m_impl->pads[padId];
    if (!e.inUse) return -1;
    return e.xinputIndex;
}

bool ViGEmPlugin::isAllocated(int padId) const
{
    if (padId < 0 || padId >= kMaxPads) return false;
    QMutexLocker lock(&m_impl->padsMx);
    return m_impl->pads[padId].inUse;
}

// --- ViGEmSink (legacy single-pad path) -------------------------------------

ViGEmSink::ViGEmSink(ViGEmPlugin* owner, QObject* parent)
    : QObject(parent), m_owner(owner)
{
    m_ready.store(owner && owner->clientReady());
}

ViGEmSink::~ViGEmSink() = default;

int ViGEmSink::userIndex()
{
    if (!m_owner) return -1;
    int slot = m_legacySlot.load();
    if (slot < 0) {
        slot = m_owner->allocatePad(0 /*X360*/);
        if (slot < 0) return -1;
        m_legacySlot.store(slot);
    }
    return m_owner->xinputIndexOf(slot);
}

void ViGEmSink::pushState(const ControllerState& state)
{
    if (!m_owner) return;
    if (!m_owner->clientReady()) return;

    int slot = m_legacySlot.load();
    if (slot < 0 || !m_owner->isAllocated(slot)) {
        // Lazy alloc / re-alloc on not-plugged recovery.
        slot = m_owner->allocatePad(0 /*X360*/);
        if (slot < 0) return;
        m_legacySlot.store(slot);
    }

    if (!m_owner->pushStateTo(slot, state)) {
        // pushStateTo auto-frees on not-plugged. Reset our cache so the next
        // push triggers a fresh alloc.
        m_legacySlot.store(-1);
    }
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::ViGEmPlugin();
}
