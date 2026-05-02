#include "FrameBufferPool.h"

namespace Labs {

FrameBufferPool::FrameBufferPool(size_t buf_size, int slot_count)
    : m_buf_size(buf_size)
{
    m_buffers.resize(slot_count);
    m_in_use.assign(slot_count, false);
    for (auto& b : m_buffers) {
        b.resize(buf_size);
    }
}

FrameBufferPool::~FrameBufferPool() = default;

std::shared_ptr<FrameBufferPool::Slot>
FrameBufferPool::acquire(size_t needed_size)
{
    if (needed_size > m_buf_size) {
        // Slot too small for this resolution (e.g. 4K) — fall through to
        // the transient path so we never under-allocate.
    } else {
        std::lock_guard<std::mutex> lg(m_mx);
        for (size_t i = 0; i < m_in_use.size(); ++i) {
            if (!m_in_use[i]) {
                m_in_use[i] = true;
                const int idx = int(i);
                Slot* slot = new Slot{ m_buffers[i].data(), m_buf_size };
                return std::shared_ptr<Slot>(slot, [this, idx](Slot* s) {
                    delete s;
                    this->release(idx);
                });
            }
        }
    }

    // Fallback: all slots in flight (or oversized request). One-off heap
    // allocation — same cost as the pre-pool path, but only when consumers
    // are genuinely backed up. The frame still gets through.
    auto* fallback = new std::vector<char>(needed_size);
    Slot* slot = new Slot{ fallback->data(), needed_size };
    return std::shared_ptr<Slot>(slot, [fallback](Slot* s) {
        delete fallback;
        delete s;
    });
}

void FrameBufferPool::release(int idx)
{
    std::lock_guard<std::mutex> lg(m_mx);
    if (idx >= 0 && size_t(idx) < m_in_use.size()) {
        m_in_use[idx] = false;
    }
}

FrameBufferPool& FrameBufferPool::global()
{
    // 1920 × 1080 × 4 = 8,294,400 bytes per slot. Four slots covers:
    //   1 in DisplaySurface (m_image)
    //   1 in CvPython worker queue
    //   1 in CvPython worker actively writing SHM
    //   1 fresh from source
    // Cap at 4 — anything more in flight means a consumer is stuck and
    // the fallback path will absorb the extra without backpressuring.
    //
    // Deliberate leak: pool is heap-allocated and never deleted. Slot
    // deleters captured `this` via lambda; on process teardown a frame
    // sitting in a plugin queue can outlive a function-local-static pool
    // and call release() on dead memory. Leaking the singleton is cheap
    // (one ~33 MB block) and guarantees deleters always see a live pool.
    static FrameBufferPool* s_pool = new FrameBufferPool(1920u * 1080u * 4u, 4);
    return *s_pool;
}

} // namespace Labs
