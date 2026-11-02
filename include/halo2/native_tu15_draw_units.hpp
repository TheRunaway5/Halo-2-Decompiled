#pragma once
#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/native_tu15_render_prepare.hpp"

namespace halo2 {
struct NativeTu15DrawUnitCall {
    std::uint32_t geometry{},instance{},distance_byte{}; // EAX, ECX, EDX.
    std::array<std::uint32_t,21> stack{}; // Original ESP+4..54; float bits stay literal.
};
struct NativeTu15DrawUnitServices {
    NativeTu15RenderLightingServices lighting;
    // Whole30270 writes four floats through ESI=output. Its camera/projection
    // roots are the original4C65C0/4C6634. No FP environment spans this call.
    // Absent an override, the native30270 owner runs directly.
    std::function<void(std::uint32_t center,std::uint32_t output,std::uint32_t radius,
        NativeTu15PhysicsFrameFp&)> rectangle;
    // The actual model caller retains two rewritten argument slots above its
    // entry stack. Publish them at their source phase, with host FP restored.
    // Native value callers need no stack publication.
    std::function<void(std::uint32_t instruction,unsigned index,std::uint32_t value)> argument_write;
};
void native_tu15_draw_units_begin_2cb10(SourcePhysicsContactMemory&);
struct NativeTu15MaskAllocation {std::uint32_t address{},words{};};
NativeTu15MaskAllocation native_tu15_mask_arena_135dc0(SourcePhysicsContactMemory&,
    std::uint32_t count,std::uint32_t context,const SourcePhysicsContactRecordObserver& = {});
std::int32_t native_tu15_floor_16ac0(const SourcePhysicsContactMemory&,
    std::uint32_t value_bits,NativeTu15PhysicsFrameFp&);
// Whole2CBD0 bank mutations and consumed signedAX result. Caller-private
// arguments are values; every observable unit/counter/context read stays live.
// The optional details retain the full return word for the sole model bridge.
// Other volatile GP/EFLAGS/XMM residues are overwritten by its continuation.
struct NativeTu15DrawUnitDetails {
    std::uint32_t return_word=0xffffffffu;
    bool early_refusal{},geometry_refusal{},rectangle{},mask_requested{};
};
std::int16_t native_tu15_draw_unit_2cbd0(SourcePhysicsContactMemory&,
    const NativeTu15DrawUnitCall&,NativeTu15PhysicsFrameFp&,const NativeTu15DrawUnitServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15DrawUnitDetails* = nullptr);
std::uint32_t native_tu15_draw_flags_135e00(const SourcePhysicsContactMemory&,
    std::uint16_t queue_flags,std::uint32_t context,NativeTu15PhysicsFrameFp&);
void native_tu15_draw_clusters_135ea0(SourcePhysicsContactMemory&,std::uint32_t context,
    NativeTu15PhysicsFrameFp&,const NativeTu15DrawUnitServices&);
void native_tu15_draw_instances_136030(SourcePhysicsContactMemory&,std::uint32_t context,
    NativeTu15PhysicsFrameFp&,const NativeTu15DrawUnitServices&);
}
