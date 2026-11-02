#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15LookVectorAngleStats {
    std::uint64_t calls{},completed{},refusals{},zero_products{},acos_calls{},
        alternate_control_words{},required_crt_policy_calls{};
};
// Actual75-site11F7E0 with native finite32CCD8 acos arithmetic. Inputs are
// three-word vector DATA, result is the semantic scalar originally inXMM0.
// Normal policy/CW27F and native direct-CW-restoration paths are implemented;
// genuine3334F0 policy-hook/error paths remain explicit refusals.
std::uint32_t native_tu15_look_vector_angle(SourcePhysicsContactMemory&,
    std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,NativeTu15LookVectorAngleStats* = nullptr);
// Native E195A..E1977 camera phase with the SAME sole finite32CCD8 acos.
// The selected source domain is -1<vertical<0; return is the original rounded
// half-pi-minus-acos(-vertical) scalar. Exceptional CRT paths still refuse.
std::uint32_t native_tu15_camera_downward_angle(SourcePhysicsContactMemory&,
    std::uint32_t vertical_bits,NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth,
    NativeTu15LookVectorAngleStats* = nullptr);
// Shared native32CCD8 arithmetic for an already admitted native FP kernel.
// Its real native x87 ST0 input/result remains live; caller owns the hardware
// FP scope and source scalar load/spill phases. No emulated register state.
void native_tu15_look_acos_active(SourcePhysicsContactMemory&,
    NativeTu15LookVectorAngleStats* = nullptr);
} // namespace halo2
