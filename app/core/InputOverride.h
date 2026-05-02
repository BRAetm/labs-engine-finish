#pragma once

#include "LabsCore.h"
#include "InputTypes.h"

#include <cstdint>

namespace Labs {

// 32-byte (now 40-byte v3) record laid out in a Win32 named shared-memory
// section so external processes (the Labs2K Python script) can briefly
// override pieces of the physical pad's state — most importantly, the right
// trigger pulse for shot releases. Lock-free seqlock: writers bump `sequence`
// to odd before writing, even after; readers reject odd seqs and verify
// sequence didn't change mid-read.
//
// SHM name: see kSharedMemName below. Packed so the layout is identical
// across C++ and Python (which writes via struct.pack with no padding).
//
// TODO(python): Python writers currently reference the SHM name as a string
// literal. Files to update when this name changes:
//   - labs-engine/scripts/labs_input_bridge.py
//   - labs-engine/scripts/Labs2K.py
//   - any other writer using `Labs_Input_Override_v*`
#pragma pack(push, 1)
struct InputOverrideRecord {
    char     magic[4];        // "LBSO"
    uint8_t  version;         // protocol version, currently 2
    uint8_t  _pad0[3];        // align sequence to 4 bytes
    uint32_t sequence;        // seqlock counter (odd = writer in progress)
    uint64_t expires_qpc;     // QPC counter ticks; 0 = inactive
    uint8_t  rt;              // override RT value (0-255)
    uint8_t  l2;              // override L2 value (0-255)
    uint8_t  flags;           // bit 0 RT, bit 1 L2, bit 2 RS_Y, bit 3 LS_Y,
                              // bit 4 BUTTONS, bit 5 RS_X, bit 6 LS_X
    int16_t  rs_y;            // override right-stick Y (-32767..32767)
    int16_t  ls_y;            // override left-stick Y
    uint16_t buttons;         // ControllerButton bitmask — OR'd onto state.buttons
    int16_t  rs_x;            // override right-stick X
    int16_t  ls_x;            // override left-stick X
    uint8_t  reserved[7];     // pad to 40 bytes
};
#pragma pack(pop)
static_assert(sizeof(InputOverrideRecord) == 40, "InputOverrideRecord must be 40 bytes");

// SHM section name. v2 timebase changed (raw QPC ticks instead of
// std::chrono::steady_clock microseconds); v2 record adds a seqlock.
// Both writer and reader reference this constant.
static constexpr const char* kSharedMemName = "Labs_Input_Override_v2";
static constexpr uint8_t     kInputOverrideVersion = 2;

// Process-wide singleton. Maps the SHM section on first use, applies any
// active overrides to the given ControllerState in place. Cheap enough to call
// per pad-push (one cmp + one read of a cached pointer).
class LABSCORE_API InputOverride {
public:
    static InputOverride& instance();
    void apply(ControllerState& state);

private:
    InputOverride();
    ~InputOverride();
    InputOverride(const InputOverride&) = delete;
    InputOverride& operator=(const InputOverride&) = delete;

    bool ensure_mapped();

    void*                m_mapping = nullptr;   // HANDLE
    InputOverrideRecord* m_record  = nullptr;
    uint64_t             m_origin_us = 0;       // perf_counter base (set on first use)
};

} // namespace Labs
