#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_king_boundary.hpp"
#include <array>
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
// Only defined fields of the original25C4CF input. Bytes2/3 and absent region
// rows are zero: the caller's uninitialized stack padding is never a packet.
struct SourceObjectiveWaypointTu15 {
    std::array<std::byte,0x90> bytes{};
    bool operator==(const SourceObjectiveWaypointTu15&)const=default;
};
struct SourceObjectiveGoalTu15 {
    std::uint32_t shader=0xffffffffu;
    std::int32_t designator{};
    Vec3 center{},color{};
    float radius{},distance_scale{};
};
struct SourceObjectivePresentationTu15 {
    std::vector<SourceObjectiveWaypointTu15> waypoints;
    std::vector<SourceObjectiveGoalTu15> goals;
};
struct SourceObjectiveGoalPolygonTu15 {
    std::array<std::array<float,2>,32> points;
    float perimeter{};
};
// Original24D760's32-point circle and its single-spilled perimeter. The
// shared wall consumer retains original height, opacity and animated UVs.
SourceObjectiveGoalPolygonTu15 source_objective_goal_polygon_tu15(Vec3 center,float radius,
    SourceKingBoundaryFloatingPoint = {0x27f,0x1f80});
std::optional<SourceKingBoundary> source_objective_goal_boundary_tu15(
    const SourceObjectiveGoalTu15&,Vec3 actual_eye,std::int32_t tick,std::int16_t rate,
    SourceKingBoundaryFloatingPoint = {0x27f,0x1f80});
// Executes actual registered slots13/14 on a read-only world with a private
// caller/view binding. Output boundaries consume real original descriptors;
// no gameplay bank may be written and no missing helper returns success.
SourceObjectivePresentationTu15 source_objective_presentation_tu15(
    const SourceRuntimeMemory&,std::uint32_t player,SourcePhysicsFactoryFloatingPoint);
} // namespace halo2
