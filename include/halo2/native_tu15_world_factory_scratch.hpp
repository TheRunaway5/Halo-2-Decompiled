#pragma once
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/native_tu15_wrapped_factory_binding.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2 {
// World owns this interval throughout the closed native constructor chain.
// Low DATA through +0x1880 belongs to pair wake; high DATA from end-0x2800
// belongs to regroup/merge. List/transform parents descend separately from
// end-0x900 in 512-byte slots, including a 256-byte filter below each slot.
inline NativeTu15WrappedFactoryBindingScratch native_tu15_world_factory_scratch(
    NativeTu15PhysicsFrameScratch scratch,unsigned parent_depth) {
    const auto end=std::uint64_t(scratch.first)+scratch.bytes;
    const auto parent_bytes=0xa00ull+512ull*parent_depth;
    if((scratch.first&15u)||(end&15u)||scratch.bytes<0x5000u||
        end>std::numeric_limits<std::uint32_t>::max()||parent_bytes>scratch.bytes)
        throw std::runtime_error("Native wrapped factory requires disjoint retained DATA for its native chain");
    const auto first=std::uint64_t(scratch.first)+0x2000u;
    const auto limit=std::min<std::uint64_t>(end-0x2800u,end-parent_bytes);
    if(limit<first+512u)
        throw std::runtime_error("Native wrapped factory requires disjoint retained DATA for its native chain");
    // Admit all complete slots in the owned gap, without assuming an authored
    // shape has at most three wrappers. The binding still checks every depth.
    return {std::uint32_t(first),std::uint32_t((limit-first)&~511ull)};
}
} // namespace halo2
