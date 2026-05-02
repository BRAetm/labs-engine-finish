#pragma once

#include "LabsCore.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace Labs {

// Process-wide pool of reusable BGRA frame buffers. Sources (WGC, chiaki)
// acquire a slot, fill it, hand the slot's pointer to a Frame as a non-owning
// QByteArray::fromRawData. The slot's lifetime is governed by a shared_ptr
// stored on the Frame — when the last reference drops (after Display +
// CvPython are done with it) the slot returns to the pool automatically.
//
// Eliminates the per-frame 8 MB malloc that was the bulk of Labs Engine's
// 14 fps cliff at 1080p — instead of 480 MB/s of allocator traffic, the
// allocator only runs at startup.
class LABSCORE_API FrameBufferPool {
public:
    struct Slot {
        char*  data;        // pointer into the pool buffer (or transient malloc on fallback)
        size_t size;        // capacity in bytes
    };

    FrameBufferPool(size_t buf_size, int slot_count);
    ~FrameBufferPool();

    FrameBufferPool(const FrameBufferPool&)            = delete;
    FrameBufferPool& operator=(const FrameBufferPool&) = delete;

    // Acquire a free slot. If all are in flight, allocates a transient
    // (non-pooled) buffer for one frame so we never block / drop the source.
    // The returned shared_ptr's deleter returns the slot to the pool.
    std::shared_ptr<Slot> acquire(size_t needed_size);

    // Process-wide singleton, sized for 1920x1080 BGRA = 8 MB × 4 slots.
    // Source plugins call this at frame time.
    static FrameBufferPool& global();

private:
    void release(int idx);

    size_t                          m_buf_size;
    std::vector<std::vector<char>>  m_buffers;
    std::vector<bool>               m_in_use;
    std::mutex                      m_mx;
};

} // namespace Labs
