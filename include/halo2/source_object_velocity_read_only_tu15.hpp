#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/math.hpp"
namespace halo2 {
class SourceRuntimeMemory;
// Declared native presentation scratch, separate from every committed source
// region. Complete original BC200 and reached pure accessors compute output;
// writes outside this private buffer are rejected. No gameplay byte changes.
inline constexpr std::uint32_t source_object_velocity_private_address=0x6e000000u;
using SourceObjectVelocityReadOnlyObserverTu15=std::function<void(bool,
    const SourcePhysicsWorldRuntimeAbi &,std::span<const std::byte>,
    std::span<const std::uint32_t>)>;
Vec3 source_object_linear_velocity_read_only_tu15(const SourceRuntimeMemory &,
    std::uint32_t object,SourcePhysicsFactoryFloatingPoint,
    const SourceObjectVelocityReadOnlyObserverTu15 & = {});
}
