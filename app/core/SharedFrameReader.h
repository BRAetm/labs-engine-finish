#pragma once

#include "LabsCore.h"
#include "FrameTypes.h"

#include <QString>
#include <QtGlobal>

namespace Labs {

// Single-reader, non-blocking consumer for a named Windows shared-memory
// block following the LIVB seqlock layout below. The block name is a
// constructor argument — the reader is a generic primitive over any
// producer that publishes frames in this format.
//
// Layout (all little-endian, header is 64 bytes, payload follows):
//   [ 0.. 3] magic     uint32 = "LIVB" (0x4256494C)
//   [ 4.. 7] version   uint32 = 1
//   [ 8..11] sequence  uint32 — seqlock: odd while writer is mid-write,
//                               even = stable. Monotonic; first stable
//                               frame is sequence 2.
//   [12..15] payload   uint32 — bytes immediately following the header
//   [16..19] width     uint32
//   [20..23] height    uint32
//   [24..27] stride    uint32 — must satisfy stride >= width * 4
//   [28..31] format    uint32 — 0 = BGRA8 (only format defined today)
//   [32..39] timestamp int64  — microseconds, source-defined epoch
//   [40..43] session   uint32 — producer-stamped routing id
//   [44..63] reserved          — writer zeroes
//   [64..  ] payload           — packed pixel bytes, length = payload field
//
// Producer protocol (mirror, for any future writer in this repo):
//   1. Initialise: zero the header, write magic + version once on open.
//   2. Per frame:
//        a. seq := current_even + 1                 (odd; mid-write)
//        b. publish seq at [8]; release fence
//        c. memcpy payload into [64..]
//        d. write payload_size, width, height, stride, format,
//           timestamp_us, session_id; release fence
//        e. seq := seq + 1                          (even; stable)
//        f. publish seq at [8]
//
// Reader protocol (this class):
//   1. Read seq1 at [8]. Skip if odd, zero, or already delivered.
//   2. Acquire fence. Snapshot header. Reject self-inconsistent values
//      (likely a torn read whose sequence was about to flip odd).
//   3. Deep-copy payload into out.data — the SHM bytes can be overwritten
//      the moment seq goes odd again, so we must own a copy.
//   4. Acquire fence. Read seq2. If seq2 != seq1 the writer raced our
//      copy; retry. After kMaxAttempts failures we return false and let
//      the caller poll again (they'll catch the next stable frame).
class LABSCORE_API SharedFrameReader {
public:
    static constexpr quint32 kMagic      = 0x4256494Cu; // 'LIVB' (LE)
    static constexpr quint32 kVersion    = 1u;
    static constexpr int     kHeaderSize = 64;
    static constexpr int     kMaxPayload = 1920 * 1080 * 4;
    static constexpr int     kBlockSize  = kHeaderSize + kMaxPayload;

    SharedFrameReader();
    ~SharedFrameReader();

    SharedFrameReader(const SharedFrameReader&)            = delete;
    SharedFrameReader& operator=(const SharedFrameReader&) = delete;

    // Opens an existing named mapping. Returns false on
    // OpenFileMapping/MapViewOfFile failure or if the header magic /
    // version don't match the LIVB schema. The producer must have
    // created the mapping before this call — open() does not retry.
    bool open(const QString& blockName);
    void close();
    bool isOpen() const { return m_view != nullptr; }

    // Non-blocking. On a stable, never-before-seen frame: deep-copies
    // the payload into `out`, stamps the header fields, and returns
    // true. Otherwise returns false (no new frame, header torn beyond
    // the retry budget, or block not open). Single-reader: callers
    // serialise all calls on a given instance.
    bool tryRead(Frame& out);

    quint32 lastSequence() const { return m_lastSeq; }

private:
    QString m_blockName;
    void*   m_mapping = nullptr;  // HANDLE; void* keeps Windows.h out of the header
    quint8* m_view    = nullptr;
    quint32 m_lastSeq = 0;
};

} // namespace Labs
