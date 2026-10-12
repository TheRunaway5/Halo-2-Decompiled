#pragma once
#include "halo2/source_object_draw_collector.hpp"

namespace halo2 {
struct SourceDrawResourceWorld;
struct SourceObjectBuildRequest {
    std::uint32_t owner{}, scalar_1b8{}, scalar_1b4{}, scalar_1a8{}, mode{};
    Vec3 center{};
    std::uint8_t flags{};
};
struct SourceObjectPresentationServices {
    // Actual original object body+C/+10 links, using direct low16 lookup.
    std::function<std::uint32_t(std::uint32_t,unsigned)> link;
    std::function<SourceObjectDrawOwner(std::uint32_t)> owner;
    std::function<bool(std::uint32_t)> weapon_section_policy;
    // The original counter addressed by collector+1C8. Its mutation survives
    // every recursive effect copy and child in the caller's owned namespace.
    std::function<std::uint32_t&(std::uint32_t)> section_counter;
    // Give each temporary copy an actual caller-owned identity. Original
    // stack addresses are not native object/cache ownership.
    std::function<std::uint32_t()> copy_identity;
    // Actual4B5D0 production remains explicit. It writes collector+4 onward;
    // owner+0 belongs to this orchestrator and must survive the call.
    std::function<void(SourceObjectDrawCollector&,const SourceObjectBuildRequest&)> build;
    // Bind this to source_object_collect_draw_units with current draw state,
    // native resources, camera and mutable contexts. No frame reset occurs.
    std::function<void(const SourceObjectDrawCollector&,std::uint32_t)> collect;
};
struct SourceObjectPresentationContext {
    Vec3 eye{}, forward{};
    std::uint32_t mode{};
};
// Only direct body/link/weapon fields are supplied by this adapter. Remaining
// callbacks require the caller's live section/copy/cache/draw owners.
SourceObjectPresentationServices source_object_presentation_resource_services(
    const SourceDrawResourceWorld&);
// Whole133AB0 effect copies, parent flag mutations and ordered child traversal.
// Arguments retain original low-byte semantics. Missing consumed owners reject.
void source_object_present(SourceObjectDrawCollector&,std::uint32_t children,
    std::uint32_t skip_effects,std::uint32_t build_flags,
    const SourceObjectPresentationContext&,const SourceObjectPresentationServices&);
}
