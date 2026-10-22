#pragma once
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/source_object_location.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/math.hpp"
#include <array>
#include <cmath>
#include <algorithm>

namespace halo2::test {
enum class CampaignObserverSurvey { first_clear_floor, bound_clearance };
struct CampaignObserverStartProof {
    std::size_t triangle{};
    CollisionSurfaceIdentity source;
    bool triangle_survey{};
    unsigned visible_bound_samples{};
    float observer_distance{};
};
// Declared initial inputs only, using the same cached-BSP survey as the
// retained class3 causal matrix. A cached-BSP ray must reach the authored object bounding center from the standing eye.
// All later motion belongs to the source world.
template<std::size_t Count=2>
inline std::array<Spawn,Count> campaign_vehicle_observer_starts(const Cache &cache,
    const LevelContent &content,std::size_t bsp,const ScenarioObject &placement,
    std::array<CampaignObserverStartProof,Count> *proof=nullptr,
    CampaignObserverSurvey survey=CampaignObserverSurvey::first_clear_floor) {
    const CollisionWorld terrain(content.collision);
    // Fixture vantage selection must also respect the submitted BSP geometry.
    // Decorative beams can hide a vehicle without owning a collision surface.
    // This conservative render-triangle survey only selects initial inputs;
    // actual movement, cameras and contacts still belong to the source world.
    std::optional<CollisionWorld> render_occluders;
    if(survey==CampaignObserverSurvey::bound_clearance) {
        const auto bsp_mesh=cache.load_bsp(bsp,false);
        std::vector<CollisionTriangle> render_triangles;
        render_triangles.reserve(bsp_mesh.vertices.size()/3);
        for(std::size_t n=0;n+2<bsp_mesh.vertices.size();n+=3)
            render_triangles.push_back({bsp_mesh.vertices[n].position,
                bsp_mesh.vertices[n+1].position,bsp_mesh.vertices[n+2].position});
        render_occluders.emplace(std::move(render_triangles));
    }
    const auto radius=std::abs(placement.scale)*placement.bounding_radius;
    const auto center=placement.position+authored_basis(placement.rotation).rotate(placement.bounding_offset*placement.scale);
    const auto atmosphere=cache.load_bsp_atmosphere(bsp);
    static_assert(Count>0 && Count<=4);
    std::array<Spawn,Count> starts{};
    for(unsigned slot=0;slot<starts.size();++slot) {
        bool found=false;
        unsigned best_visible{};float best_distance{};
        const auto admit=[&](const RayHit &floor) {
            const auto foot=floor.position+Vec3{0,0,content.player.radius};
            const auto location=source_object_point_location(atmosphere,std::int16_t(bsp),foot+Vec3{0,0,.5f*content.player.height});
            const auto eye=foot+Vec3{0,0,content.player.camera_height};
            const auto sight=center-eye;const auto distance=length(sight);
            if(!(floor.normal.z>=content.player.maximum_slope_cosine) || location.cluster<0 ||
                terrain.overlaps(foot,content.player.height,content.player.radius) ||
                !(distance>0) || terrain.raycast(eye,sight/distance,distance) ||
                (render_occluders && render_occluders->raycast(eye,sight/distance,distance)))return false;
            for(unsigned previous=0;previous<slot;++previous)
                if(length(foot-starts[previous].position)<2*content.player.radius)return false;
            // A clear ray to one point can pass through a gap between beams.
            // Rank the complete bounded floor survey by clear rays to the six
            // cardinal points of the authored object bound. Prefer the more
            // distant admitted floor on ties to leave room for visible motion.
            unsigned visible{};
            if(render_occluders)for(const auto offset:std::array{Vec3{radius,0,0},Vec3{-radius,0,0},
                    Vec3{0,radius,0},Vec3{0,-radius,0},Vec3{0,0,radius},Vec3{0,0,-radius}}) {
                const auto ray=center+offset-eye;const auto reach=length(ray);
                if(reach>0 && !render_occluders->raycast(eye,ray/reach,reach))++visible;
            }
            if(found && (visible<best_visible || (visible==best_visible && distance<=best_distance)))return true;
            best_visible=visible;best_distance=distance;
            starts[slot].position=foot;
            starts[slot].facing=std::atan2(center.y-foot.y,center.x-foot.x);
            if(proof)(*proof)[slot]={floor.triangle,floor.source,false,visible,distance};
            return true;
        };
        for(unsigned n=slot;n<128;n+=Count) {
            const auto angle=float(n%32)*6.28318530718f/32.f;
            auto candidate=center+Vec3{std::cos(angle),std::sin(angle),0}*((radius+2*content.player.height)*float(1+n/32));
            auto origin=candidate+Vec3{0,0,2*radius+2*content.player.height};
            float reach=4*radius+8*content.player.height;
            for(unsigned layer=0;layer<16 && reach>0;++layer) {
                const auto floor=terrain.raycast(origin,{0,0,-1},reach);if(!floor)break;
                if(admit(*floor)){found=true;break;}
                reach-=floor->distance+content.player.radius*.1f;
                origin=floor->position-Vec3{0,0,content.player.radius*.1f};
            }
            if(found && survey==CampaignObserverSurvey::first_clear_floor)break;
        }
        if(!found)throw FormatError("Authored vehicle has no clear nearby cached BSP observer start");
    }
    return starts;
}
}
