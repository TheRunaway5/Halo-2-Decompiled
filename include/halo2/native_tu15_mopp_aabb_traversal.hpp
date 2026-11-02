#pragma once
#include "halo2/native_tu15_mopp_aabb_setup.hpp"
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
struct NativeTu15MoppAabbTraversalServices {NativeTu15ArrayGrowthServices allocation;};
struct NativeTu15MoppAabbTraversalStats {
    std::uint64_t calls{},completed{},opcodes{},recursive_calls{},leaves{},growths{},refusals{};
};
// Actual309940. MOPP is the authored collision-tree DATA format. This native
// query owns its geometric splits/quantization/key publication; it does not
// decode or execute Xbox CPU instructions or consume a guest register file.
void native_tu15_traverse_mopp_aabb(SourcePhysicsContactMemory&,
    std::uint32_t cache,std::uint32_t program,const NativeTu15MoppAabbVariables&,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MoppAabbTraversalServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MoppAabbTraversalStats* = nullptr);
} // namespace halo2
