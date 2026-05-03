#include "SharedFrameReader.h"

#include <QDebug>

#include <Windows.h>
#include <atomic>
#include <cstring>

namespace Labs {

namespace {

inline quint32 loadU32(const quint8* base, int off) {
    quint32 v;
    std::memcpy(&v, base + off, 4);
    return v;
}
inline qint64 loadI64(const quint8* base, int off) {
    qint64 v;
    std::memcpy(&v, base + off, 8);
    return v;
}

} // namespace

SharedFrameReader::SharedFrameReader() = default;
SharedFrameReader::~SharedFrameReader() { close(); }

bool SharedFrameReader::open(const QString& blockName)
{
    close();
    m_blockName = blockName;

    HANDLE mapping = ::OpenFileMappingW(FILE_MAP_READ, FALSE,
        reinterpret_cast<LPCWSTR>(m_blockName.utf16()));
    if (!mapping) {
        qWarning().nospace() << "SharedFrameReader: OpenFileMapping failed for "
                             << m_blockName << " err=" << ::GetLastError();
        return false;
    }
    void* view = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, kBlockSize);
    if (!view) {
        qWarning().nospace() << "SharedFrameReader: MapViewOfFile failed for "
                             << m_blockName << " err=" << ::GetLastError();
        ::CloseHandle(mapping);
        return false;
    }

    auto* p = static_cast<quint8*>(view);
    const quint32 magic   = loadU32(p, 0);
    const quint32 version = loadU32(p, 4);
    if (magic != kMagic || version != kVersion) {
        qWarning().nospace() << "SharedFrameReader: header mismatch in "
                             << m_blockName
                             << " (magic=0x" << QString::number(magic, 16)
                             << " version=" << version << ")";
        ::UnmapViewOfFile(view);
        ::CloseHandle(mapping);
        return false;
    }

    m_mapping = mapping;
    m_view    = p;
    m_lastSeq = 0;
    return true;
}

void SharedFrameReader::close()
{
    if (m_view)    { ::UnmapViewOfFile(m_view); m_view = nullptr; }
    if (m_mapping) { ::CloseHandle(static_cast<HANDLE>(m_mapping)); m_mapping = nullptr; }
    m_blockName.clear();
    m_lastSeq = 0;
}

bool SharedFrameReader::tryRead(Frame& out)
{
    if (!m_view) return false;

    // Bounded retries. Under heavy writer contention we'd rather miss
    // this frame than spin — the caller's poll cadence will catch the
    // next stable publish within ~one frame interval.
    constexpr int kMaxAttempts = 4;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        const quint32 seq1 = loadU32(m_view, 8);
        if (seq1 & 1u)             continue;          // writer mid-write
        if (seq1 == 0)             return false;      // never published
        if (seq1 == m_lastSeq)     return false;      // already delivered

        std::atomic_thread_fence(std::memory_order_acquire);

        const int    payloadBytes = static_cast<int>(loadU32(m_view, 12));
        const int    width        = static_cast<int>(loadU32(m_view, 16));
        const int    height       = static_cast<int>(loadU32(m_view, 20));
        const int    stride       = static_cast<int>(loadU32(m_view, 24));
        const int    format       = static_cast<int>(loadU32(m_view, 28));
        const qint64 tsUs         = loadI64(m_view, 32);
        const int    sessionId    = static_cast<int>(loadU32(m_view, 40));

        if (payloadBytes <= 0 || payloadBytes > kMaxPayload ||
            width <= 0 || height <= 0 || stride < width * 4 ||
            payloadBytes < stride * height) {
            // Header self-inconsistent — almost certainly we sampled
            // seq1 right before the writer flipped it odd. Retry.
            continue;
        }

        QByteArray payload(payloadBytes, Qt::Uninitialized);
        std::memcpy(payload.data(), m_view + kHeaderSize, payloadBytes);

        std::atomic_thread_fence(std::memory_order_acquire);
        const quint32 seq2 = loadU32(m_view, 8);
        if (seq2 != seq1) continue;                   // writer raced our copy

        out.data        = std::move(payload);
        out.width       = width;
        out.height      = height;
        out.stride      = stride;
        out.format      = (format == 0) ? PixelFormat::BGRA8 : PixelFormat::Unknown;
        out.timestampUs = tsUs;
        out.sessionId   = sessionId;
        out._pool_holder.reset();                     // SHM read does not pool

        m_lastSeq = seq1;
        return true;
    }
    return false;
}

} // namespace Labs
