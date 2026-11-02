#pragma once
#include "halo2/native_tu15_mopp_ray_setup.hpp"
#include <functional>
#include <map>
namespace halo2 {
struct NativeTu15MoppChildShape {std::uint32_t identity{};};
// Ordinary source ray-body DATA passed to a shape's collector consumer.
// No saved machine registers, return frame or guest execution state is used.
struct NativeTu15MoppRayBody {
    std::uint32_t shape{},primitive_key{},parent_field8{},query_context{};
};
struct NativeTu15MoppRayTraversalServices {
    std::function<bool(std::uint32_t receiver,std::uint32_t owner,
        std::uint32_t ray,std::uint32_t collection,std::uint32_t primitive_key,
        NativeTu15PhysicsFrameFp&)> filter;
    // The provider owns any temporary child-shape DATA until its following
    // cast returns. Recursive/reentrant providers must retain distinct DATA.
    std::function<NativeTu15MoppChildShape(std::uint32_t collection,
        std::uint32_t owner,std::uint32_t primitive_key,
        NativeTu15PhysicsFrameFp&)> child_shape;
    std::function<bool(NativeTu15MoppChildShape,std::uint32_t owner,
        std::uint32_t ray,std::uint32_t output,
        NativeTu15PhysicsFrameFp&)> cast_direct;
    std::function<void(NativeTu15MoppChildShape,std::uint32_t owner,
        std::uint32_t ray,NativeTu15MoppRayBody,std::uint32_t collector,
        NativeTu15PhysicsFrameFp&)> cast_collector;
};
struct NativeTu15MoppRayTraversalStats {
    std::uint64_t calls{},completed{},refusals{},opcodes{},branches{},recursive_calls{},
        leaves{},filters{},child_shapes{},direct_casts{},collector_casts{},
        invalid_opcodes{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Semantic30AA70 MOPP-asset traversal. Variables are all32 source-written
// arithmetic bytes from30B890; bounds are both complete source Vec4 values.
// Only initialized cache/ray DATA, code-asset bytes and typed native services
// cross this boundary. The bytecode is geometry data, never Xbox CPU code.
// Copy-on-write variables are owned values. Bounds remain mutable semantic
// DATA, matching the original input/output contract;30B890 itself discards
// its bounds afterreturn. No raw private CPU-frame observation is promised.
void native_tu15_traverse_mopp_ray(SourcePhysicsContactMemory&,
    std::uint32_t cache_identity,std::uint32_t code_start,
    const NativeTu15MoppTraversalVariables&,NativeTu15MoppTraversalBounds&,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const NativeTu15MoppRayTraversalServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15MoppRayTraversalStats* = nullptr);
} // namespace halo2
