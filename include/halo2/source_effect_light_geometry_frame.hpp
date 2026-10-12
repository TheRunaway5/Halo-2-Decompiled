#pragma once
#include "halo2/source_bsp_draw_collector.hpp"
#include "halo2/source_effect_light_opaque.hpp"

namespace halo2 {
struct SourceEffectLightGeometryFrameContext {
    SourceBspDrawContext draw;
    SourceDrawUnitCullContext cull;
    SourceBspPartCommandContext commands;
    std::uint32_t command_selector{}; //31192 supplies0 to40E90.
};
struct SourceEffectLightGeometryFrameWorld {
    SourceBspDrawWorld draw;
    SourceBspPartCommandWorld commands;
    SourceTagMetadataProviders metadata;
    // Current133720 owner, after BSP collection. The current full main prefix,
    // shared850-unit bank and selected query's arena belong to the caller.
    // An absent callback is unresolved, never an empty object query.
    std::function<void(SourceDrawUnitState&,SourceBspAdmission&,
        const SourceBspDrawContext&)> dynamic;
    std::function<SourceCullDrawBanks(unsigned,const SourceCullDrawUnit&)> cull_bank;
    std::function<void(SourceBspPartCommandState&,std::uint32_t)> object_commands;
    SourceOpaqueGeometryService opaque_geometry;
};
enum class SourceEffectLightGeometryFramePhase {collected,culled};
using SourceEffectLightGeometryFrameObserver=std::function<void(
    SourceEffectLightGeometryFramePhase,const SourceDrawUnitState&,const SourceBspAdmission&)>;
// Join31EF0..31F09 and the secondary31192/40E90/3BAB0 owners. Receiver/query
// preparation has already supplied its actual reset arena and BSP queues.
// Native output of each stage feeds the next; no draw-unit, mask, coverage or
// command snapshot is supplied between stages. Deep dynamic/object/geometry
// callbacks remain explicit owners. Ordered1CAA0 binding/ranges and GPU packet
// production follow this entry; opaque selection alone is not a draw.
struct SourceEffectLightGeometryFrameResult {
    std::vector<SourceDrawUnitSphereCall> cull_calls;
    unsigned opaque_count{};
};
SourceEffectLightGeometryFrameResult source_effect_light_geometry_frame(SourceDrawUnitState&,
    SourceBspAdmission&,SourceBspPartCommandState&,
    std::span<SourceOpaqueSortRecord>,const SourceEffectLightGeometryFrameWorld&,
    const SourceEffectLightGeometryFrameContext&,
    const SourceEffectLightGeometryFrameObserver& observer={});
}
