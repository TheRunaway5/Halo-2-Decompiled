#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15LinearJacobianEmitterStats {
    std::uint64_t calls{},completed{},refusals{},rows{},rotated_first{},rotated_second{},
        unaligned_vectors{},unsupported_fp_profiles{};
};
// Actual3294E0 cdecl(first_point,second_point,jacobian_input,builder_packet).
// Input points and accumulator vectors are actual aligned16B DATA. Builder
// words0/4 are advanced by144/12 after three genuinely computed world axes.
// Source volatile registers and private loop-control bytes are not outputs.
void native_tu15_emit_world_linear_jacobians(SourcePhysicsContactMemory&,
    std::uint32_t first_point,std::uint32_t second_point,
    std::uint32_t jacobian_input,std::uint32_t builder_packet,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LinearJacobianEmitterStats* = nullptr);
}
