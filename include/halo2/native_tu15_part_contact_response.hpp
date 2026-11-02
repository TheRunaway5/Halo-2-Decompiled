#pragma once
#include "halo2/native_tu15_empty_part_contact.hpp"
#include "halo2/native_tu15_surface_conveyor.hpp"
namespace halo2 {
struct NativeTu15PartContactResponseScratch {
    std::uint32_t material_vector{},removed_property{};
    NativeTu15SurfaceConveyorScratch conveyor;
};
struct NativeTu15PartContactResponseStats {
    std::uint64_t calls{},inactive{},materials{},opposing_contacts{},response_vectors{},
        zero_properties{},vector_properties{};
    NativeTu15SurfaceConveyorStats conveyor;
};
// Actual1E0A60. Local vectors/material ids are native semantic DATA. The caller
// owns the12B vector,4B removed-value and two4B conveyor outputs independently.
// Native13DE70 evaluation remains a typed required child; no CPU fallback.
void native_tu15_update_part_contact_response(SourcePhysicsContactMemory&,
    std::uint32_t part_slot,std::int32_t part,std::uint32_t component,
    NativeTu15PartContactResponseScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_x87_depth,const NativeTu15ArrayGrowthServices&,
    const NativeTu15ShaderFunctionEvaluationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15PartContactResponseStats* = nullptr);
} // namespace halo2
