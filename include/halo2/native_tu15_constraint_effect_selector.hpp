#pragma once
#include "halo2/native_tu15_constraint_runtime_update.hpp"
#include "halo2/native_tu15_constraint_effect_location.hpp"
namespace halo2 {
struct NativeTu15ConstraintEffectSelectorScratch {
    std::uint32_t position{},location{},key{}; // Separate12/8/2-byte ordinary DATA.
};
struct NativeTu15ConstraintEffectSelectorStats {
    std::uint64_t calls{},secondary_components{},component_queries{},location_fallbacks{},
        event_queries{},event_fallbacks{},material_queries{},material_rows{},material_candidates{},
        proximity_queries{},camera_tests{},distance_refusals{},unknown_constant_targets{},selected_rows{};
    NativeTu15ConstraintEffectLocationStats location;
};
// Actual18AE80/18ADD0/18AF20/18B0C0; return ordinary authored row addresses.
std::uint32_t native_tu15_find_constraint_effect_event(const SourcePhysicsContactMemory&,
    std::uint32_t definition,std::uint32_t event_type,bool secondary,
    NativeTu15ConstraintEffectSelectorStats* = nullptr);
std::uint32_t native_tu15_find_constraint_effect_material(const SourcePhysicsContactMemory&,
    std::uint32_t event_row,std::uint16_t material,bool secondary,
    NativeTu15ConstraintEffectSelectorStats* = nullptr);
// Actual18B160 and1755B0. Classifications are authenticated read-only table
// DATA decoded into native branches, never executed original code.
std::uint32_t native_tu15_constraint_effect_radius(const SourcePhysicsContactMemory&,
    std::uint32_t event_type,NativeTu15ConstraintEffectSelectorStats* = nullptr);
bool native_tu15_constraint_effect_near_camera(const SourcePhysicsContactMemory&,
    std::uint32_t position,std::uint32_t radius_bits,bool scaled_distance,
    NativeTu15PhysicsFrameFp&,NativeTu15ConstraintEffectSelectorStats* = nullptr);
// Entire22EEC0/22EFD0/18B1D0 selection family, with the native children above.
// Inputs/outputs/scratch are ordinary DATA. No effect is published here.
void native_tu15_select_constraint_effects(SourcePhysicsContactMemory&,
    NativeTu15ConstraintEffectSelection,NativeTu15ConstraintEffectSelectorScratch,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintEffectSelectorStats* = nullptr);
} // namespace halo2
