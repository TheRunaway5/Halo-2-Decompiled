#pragma once
#include "halo2/physics_body_factory.hpp"
#include "halo2/physics_pair_manager.hpp"

namespace halo2 {
enum class SourceRagdollConstraintKind { ragdoll, limited_hinge };
// Original318E80/3198C0 packet owners. Unassigned words are retained.
void source_initialize_ragdoll_constraint_packet_1_1(
    SourcePhysicsContactMemory &, std::uint32_t packet,
    SourceRagdollConstraintKind,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceRagdollConstraintAxes {
  std::uint32_t packet, body_a, body_b;
  std::array<std::uint32_t, 6> vectors;
};
// Complete1D20F0/1D1CE0 axes/pivots and source Newton refinement. The
// explicit platform instruction provider has the same boundary as body math.
void source_set_ragdoll_constraint_axes_1_1(
    SourcePhysicsContactMemory &, const SourceRagdollConstraintAxes &,
    SourceRagdollConstraintKind, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete319B90 predicate, including axis and angular-span checks.
bool source_validate_limited_hinge_1_1(const SourcePhysicsContactMemory &,
                                       std::uint32_t constraint,
                                       SourcePhysicsFactoryFloatingPoint);
// Complete3192B0/319AF0 over an actual allocated object. Body reference and
// listener ownership are composed with322060/3220D0; no world insertion.
std::uint32_t source_construct_ragdoll_constraint_1_1(
    SourcePhysicsContactMemory &, std::uint32_t constraint,
    std::uint32_t packet, SourceRagdollConstraintKind,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsPairManagerServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D3B40 retained constraint-bank growth and commit.
void source_commit_ragdoll_constraint_1_1(
    SourcePhysicsContactMemory &, std::uint32_t component,
    std::uint32_t constraint, std::uint16_t type, std::uint16_t index,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceRagdollConstraintFrameRequest {
  std::uint32_t component, context, node_slots, authored_constraint;
  std::uint32_t frame_a, frame_b, body_a_output, body_b_output;
};
// Compound nodes require the actual1DBDD0 relative-frame producer. It must
// deliver the source0x34 transform from current shape children, not a pose.
using SourceRagdollRelativeFrame = std::function<std::array<float, 13>(
    std::uint32_t component, std::uint32_t slot, std::int32_t first_node,
    std::int32_t constraint_node)>;
// 1DB8E0 current body/local-frame owner, including its fixed-world-body branch.
void source_ragdoll_constraint_frames_1_1(
    SourcePhysicsContactMemory &, const SourceRagdollConstraintFrameRequest &,
    SourcePhysicsFactoryFloatingPoint, const SourceRagdollRelativeFrame & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
