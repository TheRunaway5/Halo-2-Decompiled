#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original 1ECC40 (count +34), 1ECD00 (count +5C) and 1ECD40 (count +B4):
// from character datum ECX, walk the 4F0414 tag rows through their +8
// parent link until a positive signed count; return the paired pointer at
// count+4, else zero. ecx receives the original exit ECX (row, or NONE).
std::uint32_t native_campaign_ai_character_property_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t datum,std::uint32_t count_offset,std::uint32_t &ecx);
// Whole call ABI of one of those entries: ECX in; PUSH ESI/POP ESI on the real
// stack; EAX result, ECX/EDX exit values; plain RET.
// Also accepts 1ED700: ECX actor datum, [ESP+4] object-definition datum;
// exact +E4/+E8 character records (stride B4, key +4) with parent fallback.
// That entry preserves EBP/EBX/ESI/EDI, returns the matching record or zero
// in EAX with exact ECX/EDX scratch, and finishes with RET 4. Its native
// parent 2013B0 remains pending; accepting this entry is not a live binding.
void native_campaign_ai_character_property_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &);
// Actual 270696 CALL 1ECD40 (return 27069B) in the translated AI frame.
bool native_campaign_ai_character_property_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
void native_campaign_ai_character_property_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &);
}
