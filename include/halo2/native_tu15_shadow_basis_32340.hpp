#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
struct NativeTu15ShadowGateInput {
    std::uint32_t owner_fade{},history_fade{},cached_opacity{},radius{};
};
struct NativeTu15ShadowGateResult {
    std::uint32_t fade{},opacity{},minimum_radius{};
    bool cached_positive{},admitted{};
};
//32386..32395,323D9..323EB and32416..3243B, with the source's ordered
// COMISS gates and single-precision MULSS operations under explicit controls.
NativeTu15ShadowGateResult native_tu15_shadow_gates_32340(
    const SourcePhysicsContactMemory&,const NativeTu15ShadowGateInput&,NativeTu15PhysicsFrameFp&);
struct NativeTu15ShadowBasisInput {
    std::array<std::uint32_t,3> center{},direction{};
    std::uint32_t transverse{},axial{};
};
struct NativeTu15ShadowBasisResult {
    std::array<std::uint32_t,3> direction{},up{};
    std::uint32_t near_offset{},far_offset{},width{};
    bool direction_normalized{},up_normalized{},second_basis{};
};
using NativeTu15ShadowBasisStore=std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>;
// Numeric prefix32459..32635, including both complete30BD0 calls. This is a
// typed value/publication kernel, not the complete32340 ABI or its GPU owner.
// Publications run with the host environment restored; callbacks cannot change
// source arithmetic. Input and constant memory must remain ordinary and stable.
NativeTu15ShadowBasisResult native_tu15_shadow_basis_32340(
    const SourcePhysicsContactMemory&,const NativeTu15ShadowBasisInput&,
    NativeTu15PhysicsFrameFp&,const NativeTu15ShadowBasisStore& = {});
struct NativeTu15ShadowProjectionTail {
    std::array<std::uint32_t,3> left{},volume_center{},half_extent{},virtual_eye{};
    std::uint32_t virtual_distance{},query_radius{};
};
// Numeric32656..327D4 and32817..32852 using the actual completedC3AB0
// projection words. Does not run320A0, C3DC0 or any visibility/GPU operation.
NativeTu15ShadowProjectionTail native_tu15_shadow_projection_tail_32340(
    const SourcePhysicsContactMemory&,const std::array<std::uint32_t,3>& center,
    const NativeTu15ShadowBasisResult&,const std::array<std::uint32_t,31>& projection,
    NativeTu15PhysicsFrameFp&);
}
