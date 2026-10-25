#pragma once
#include "halo2/source_objective_waypoint_tu15.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/cache.hpp"
#include <map>
#include <tuple>
namespace halo2 {
// Fixed resident authored HUD resources. Each actual quad has an independent
// material input bank, so two waypoints never overwrite each other's fade.
class ObjectiveWaypoints {
    Mesh& mesh_;
    unsigned views_;
    std::vector<Vec3> palette_;
    std::vector<SourceObjectiveBitmapViewTu15> bitmaps_;
    using Key=std::tuple<std::uint32_t,std::uint32_t,std::uint32_t>;
    std::map<Key,std::vector<std::vector<unsigned>>> materials_;
public:
    ObjectiveWaypoints(const Cache&,Mesh&,unsigned views);
    std::span<const SourceObjectiveBitmapViewTu15> bitmap_views()const{return bitmaps_;}
    std::vector<Vertex> draw(std::span<const SourceObjectiveQuadTu15>,const MultiplayerState&,
        unsigned player,unsigned view,unsigned width,unsigned height);
};
}
