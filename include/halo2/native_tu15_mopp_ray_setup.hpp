#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Source MOPP domain DATA, not a machine register/control-frame image. All
// eight variable words and all eight bound lanes are initialized by30B890.
struct NativeTu15MoppTraversalVariables {
    std::array<std::uint32_t,4> translation;
    std::array<std::uint32_t,4> fields10_1c;
};
struct NativeTu15MoppTraversalBounds {
    std::array<std::uint32_t,4> start,end;
};
struct NativeTu15MoppRaySetupServices {
    std::function<void(std::uint32_t cache,std::uint32_t code,
        const NativeTu15MoppTraversalVariables&,const NativeTu15MoppTraversalBounds&,
        NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth)>traverse;
};
struct NativeTu15MoppRaySetupStats {
    std::uint64_t calls{},completed{},shape_calls{},cache_initializations{},
        traversal_requests{},unbound_traversals{},refusals{};
};
// Actual30B890 public DATA formals; cache is an ordinary initialized MOPP ray
// record. The typed child receives computed variables/bounds and current FP.
void native_tu15_prepare_mopp_ray(SourcePhysicsContactMemory&,
    std::uint32_t cache,std::uint32_t collection,std::uint32_t code,
    std::uint32_t ray_input,std::uint32_t query_context,std::uint32_t collector,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15MoppRaySetupServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MoppRaySetupStats* = nullptr);
// Actual2E9750 using caller-owned native arithmetic cache DATA. The original
// private stack/control lifetime is neither accepted nor serialized.
void native_tu15_cast_instanced_ray_shape(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t ray_input,std::uint32_t query_context,
    std::uint32_t collector,std::uint32_t cache,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const NativeTu15MoppRaySetupServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MoppRaySetupStats* = nullptr);
} // namespace halo2
