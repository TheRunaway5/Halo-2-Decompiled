#pragma once
#include "halo2/source_objective_presentation_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <functional>
namespace halo2 {
struct SourceObjectiveQuadTu15 {
    std::array<float,8> positions{};
    std::array<float,4> uv{};
    std::uint32_t bitmap{},frame{},shader{},emblem_player=0xffffffffu;
    std::array<float,6> colors{};
    std::array<float,10> inputs{};
};
struct SourceObjectiveBitmapViewTu15 {
    std::uint32_t datum{};
    std::vector<std::array<std::int16_t,4>> frames;
};
struct SourceObjectiveViewTu15 {
    Vec3 eye;
    SourceViewProjectionBank projection;
    std::array<std::int16_t,4> outer{},safe{};
    float opacity{};
    std::span<const SourceObjectiveBitmapViewTu15> bitmaps;
    unsigned local_players=1;
};
using SourceObjectiveOcclusionTu15=std::function<bool(Vec3,Vec3,std::uint32_t,std::uint32_t,std::uint32_t)>;
// Complete original projection/background/icon/meter CPU consumers. The only
// external outputs are native UI quads and the actual world's collision owner.
std::vector<SourceObjectiveQuadTu15> source_objective_waypoint_quads_tu15(
    const SourceRuntimeMemory&,std::uint32_t player,
    std::span<const SourceObjectiveWaypointTu15>,const SourceObjectiveViewTu15&,
    const SourceObjectiveOcclusionTu15&,SourcePhysicsFactoryFloatingPoint);
} // namespace halo2
