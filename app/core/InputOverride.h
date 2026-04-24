#pragma once

#include "LabsCore.h"
#include "InputTypes.h"

#include <cstdint>

namespace Labs {

// 32-byte record laid out in a Win32 named shared-memory section so external
// processes (the Labs2K Python script) can briefly override pieces of the
// physical pad's state — most importantly, the right trigger pulse for shot
// releases. Keeping this small and lock-free: writers stamp `expires_us` last,
// readers compare against now and only apply if not expired.
//
// SHM name: "Labs_Input_Override_v1". Packed so the layout is identical
// across C++ and Python (which writes via struct.pack with no padding).
#pragma pack(push, 1)
struct InputOverrideRecord {
    char     magic[4];        // "LBSO"
    uint64_t expires_qpc;     // QPC counter ticks; 0 = inactive
    uint8_t  rt;              // override RT value (0-255)
    uint8_t  l2;              // override L2 value (0-255)
    uint8_t  flags;           // bit 0 RT, bit 1 L2, bit 2 RS_Y, bit 3 LS_Y, bit 4 BUTTONS
    int16_t  rs_y;            // override right-stick Y (-32767..32767)
    int16_t  ls_y;            // override left-stick Y
    uint16_t buttons;         // ControllerButton bitmask — OR'd onto state.buttons
    uint8_t  reserved[11];    // pad to 32 bytes
};
#pragma pack(pop)
static_assert(sizeof(InputOverrideRecord) == 32, "InputOverrideRecord must be 32 bytes");

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
