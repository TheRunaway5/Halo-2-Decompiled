#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>

namespace halo2 {
struct NativeTu15ShaderLod1bf30 {
    std::int32_t index{};
    bool accepted{};
    //1C074 writes the scan index before1C096 writes the signed clamp.
    std::array<std::int32_t,2> index_stores{};
};
// Whole ordinary1BF30. EAX supplies the full bias selector; the shader,
// metric and output pointer are its three stack arguments. Dynamic MTRP/prt3
// shader services require their original owners and are explicitly rejected.
// Empty/negative counts retain the source's wrapping signed clamp, including
// INT_MIN. SSE sticky flags belong to the supplied arithmetic environment.
NativeTu15ShaderLod1bf30 native_tu15_shader_lod_1bf30(const SourcePhysicsContactMemory&,
    std::uint32_t bias,std::uint32_t shader,std::uint32_t metric_bits,NativeTu15PhysicsFrameFp&);
}
