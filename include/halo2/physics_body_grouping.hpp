#pragma once
#include "halo2/physics_ragdoll_constraints.hpp"
#include "halo2/physics_shape_wrapper.hpp"

namespace halo2 {
// Temporary source-owned packets, allocated by the caller. They are never
// substitutes for current object nodes, resident shapes, bodies or world state.
struct SourcePhysicsBodyGroupingScratch {
  std::uint32_t grouping_frame;   // 1D9330: 0x4E94 bytes
  std::uint32_t selection_frame;  // 1D95E0: 0x100 bytes, aligned +0x30
  std::uint32_t collection_frame; // 1D98B0: 0x460 bytes
  std::uint32_t region_result, permutation_result;
  std::uint32_t constraints_frame; // 1DB1C0: 0x460 bytes
  std::uint32_t iterator_stride;
  SourcePhysicsBodyFactoryScratch factory;
  SourcePhysicsShapeWrapperScratch wrapper;
};
// Original1D9330/1D95E0/1D98B0/1DB1C0 and their native body/wrapper
// callers. Derives all temporary packet addresses from1D9330 entry ESP.
SourcePhysicsBodyGroupingScratch source_physics_body_grouping_scratch_1_1(
    std::uint32_t entry_esp);
struct SourcePhysicsBodyGroupingRequest {
  std::uint32_t component;
  std::uint32_t force_mode7_word, override_mode6_word, wrap_shape_word;
};
// Read-only observation of original1D980D factory argument ownership; this
// callback cannot supply an allocation, admission or body result.
using SourcePhysicsBodyGroupingCallObserver =
    std::function<void(const std::array<std::uint32_t, 14> &)>;
// Complete measured20E370→BB050→143860 unparented owner. Real pool/tag/node
// storage is required. A parent requires the separately owned current-node
// transform producer; this slice stops after the original root-matrix stores.
bool source_physics_body_runtime_context_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t output, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete183770 stored region/permutation selection; no default admission.
void source_physics_body_region_permutation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t model_datum,
    std::uint32_t object_datum, std::uint32_t region, std::uint32_t permutation,
    std::uint32_t region_output, std::uint32_t permutation_output,
    const SourcePhysicsContactRecordObserver & = {});
// Complete183500 group-bank owner, retaining unassigned bytes and row order.
void source_physics_body_groups_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object_datum,
    std::uint32_t output, std::uint32_t bypass_selection_word,
    std::uint32_t region_output, std::uint32_t permutation_output,
    const SourcePhysicsContactRecordObserver & = {});
// 1D9F50 and recursive1DA020, over current damage constraints and authored
//  rigid groups. These preserve actual group masks and redirect flags.
void source_physics_body_property_mask_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t context, std::uint32_t output,
    const SourcePhysicsContactRecordObserver & = {});
struct SourcePhysicsGroupCollectionRequest {
  std::uint32_t component, context, modes, properties, groups, group, indices,
      count, visited, force_mode7;
};
void source_physics_collect_body_groups_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsGroupCollectionRequest &,
    const SourcePhysicsContactRecordObserver & = {});
// 1DB1C0 admission/iteration and complete ragdoll/limited-hinge construction.
//  Other positive authored constraint classes retain their required factories.
//  Existing fixed-world constraints use the world's actual retained fixed body.
void source_physics_body_constraints_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t context, const SourcePhysicsBodyGroupingScratch &,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
// 1D9330→1D95E0 orchestration, including current non-root node transforms,
//  property masks, recursive grouping and ragdoll/limited-hinge constraints.
//  Compound shape aggregation and other constraint classes require their
//  owners. No world insertion, solver admission, contact, damage or scene is
//  supplied.
std::uint32_t source_group_physics_bodies_1_1(
    SourcePhysicsContactMemory &, const SourcePhysicsBodyGroupingRequest &,
    const SourcePhysicsBodyGroupingScratch &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsBodyGroupingCallObserver & = {});
} // namespace halo2
