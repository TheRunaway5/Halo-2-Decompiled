#pragma once
#include "halo2/native_tu15_mopp_aabb_setup.hpp"
namespace halo2 {
// Actual30A020: projected oriented bounds and their source-ordered x87
// quantization. Matrix/extent input and cache are semantic DATA backing.
void native_tu15_prepare_mopp_oriented_aabb(SourcePhysicsContactMemory&,
    std::uint32_t cache,std::uint32_t code,std::uint32_t transform,
    std::uint32_t half_extents,std::uint32_t tolerance_bits,std::uint32_t key_array,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MoppAabbSetupServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2E9640, including the receiver's current code-pointer read.
void native_tu15_query_mopp_oriented_shape(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t transform,std::uint32_t half_extents,
    std::uint32_t tolerance_bits,std::uint32_t key_array,std::uint32_t cache,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MoppAabbSetupServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
