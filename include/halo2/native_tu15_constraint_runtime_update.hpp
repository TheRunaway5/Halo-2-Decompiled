#pragma once
#include "halo2/native_tu15_constraint_geometry_frame.hpp"
#include "halo2/native_tu15_constraint_runtime_handles.hpp"
#include "halo2/native_tu15_constraint_runtime_parameters.hpp"
#include "halo2/native_tu15_constraint_body_slots.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ConstraintEffectSelection {
    std::uint32_t instance{},position{};
    std::uint16_t first_key{},second_key{};
    std::int32_t mode{};
    // Actual22EEC0 arguments5..10, ordinary caller-owned output DATA.
    std::array<std::uint32_t,6>outputs{};
};
struct NativeTu15ConstraintRuntimeUpdateServices {
    NativeTu15ConstraintRuntimeHandleServices handles;
    std::function<void(NativeTu15ConstraintEffectSelection,NativeTu15PhysicsFrameFp&)>select_effects; //22EEC0
    std::function<void(std::uint32_t position_packet,std::uint32_t effect_packet,
        NativeTu15PhysicsFrameFp&)>publish_effect; //18C440
    std::function<std::uint32_t(std::uint32_t request,NativeTu15PhysicsFrameFp&)>create_handle; //178E30
};
struct NativeTu15ConstraintRuntimeUpdateScratch {
    //224 ordinary DATA bytes. Unwritten request padding remains caller-owned;
    // it is never initialized from guest registers or computed postimages.
    std::uint32_t storage{};
    NativeTu15ConstraintGeometryScratch geometry;
};
struct NativeTu15ConstraintRuntimeUpdateStats {
    std::uint64_t calls{},transient_updates{},state_updates{},effect_selections{},effect_publications{},
        handle_creations{},unbound_effect_selections{},unbound_effect_publications{},
        unbound_handle_creations{},counter_updates{},geometry_updates{},unsupported_profiles{};
    NativeTu15ConstraintRuntimeHandleStats handles;
    NativeTu15ConstraintRuntimeParameterStats parameters;
    NativeTu15ConstraintGeometryStats geometry;
    NativeTu15ConstraintAuxiliaryMathStats math;
};
// Actual22FEA0 parent. An admitted native parent never retries original22FEA0.
// Unconverted positive effect children are explicit typed services, counted
// and refused if absent. Private DATA packets are not guest CPU frames.
void native_tu15_update_constraint_runtime(SourcePhysicsContactMemory&,
    std::uint32_t instance,std::uint32_t constraint_datum,NativeTu15ConstraintBodySlots,
    NativeTu15ConstraintRuntimeUpdateScratch,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15ConstraintRuntimeUpdateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintRuntimeUpdateStats* = nullptr);
} // namespace halo2
