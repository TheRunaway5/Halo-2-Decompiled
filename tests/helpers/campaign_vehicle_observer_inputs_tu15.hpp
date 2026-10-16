#pragma once
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/source_object_location.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/math.hpp"
#include <array>
#include <cmath>

namespace halo2::test {
// Declared initial inputs only, using the same cached-BSP survey as the
// retained class3 causal matrix. All later motion belongs to the source world.
template<std::size_t Count=2>
inline std::array<Spawn,Count> campaign_vehicle_observer_starts(const Cache &cache,
    const LevelContent &content,std::size_t bsp,const ScenarioObject &placement) {
    const CollisionWorld terrain(content.collision);
    const auto radius=std::abs(placement.scale)*placement.bounding_radius;
    const auto atmosphere=cache.load_bsp_atmosphere(bsp);
    static_assert(Count>0 && Count<=4);
    std::array<Spawn,Count> starts{};
    for(unsigned slot=0;slot<starts.size();++slot) {
        bool found=false;
        for(unsigned n=slot;n<32;n+=Count) {
            const auto angle=float(n)*6.28318530718f/32.f;
            auto candidate=placement.position+Vec3{std::cos(angle),std::sin(angle),0}*(radius+2*content.player.height);
            auto origin=candidate+Vec3{0,0,2*radius+2*content.player.height};
            float reach=4*radius+8*content.player.height;
            for(unsigned layer=0;layer<16 && reach>0;++layer) {
                const auto floor=terrain.raycast(origin,{0,0,-1},reach);if(!floor)break;
                const auto foot=floor->position+Vec3{0,0,content.player.radius};
                const auto location=source_object_point_location(atmosphere,std::int16_t(bsp),foot+Vec3{0,0,.5f*content.player.height});
                if(floor->normal.z>=content.player.maximum_slope_cosine && location.cluster>=0 &&
                    !terrain.overlaps(foot,content.player.height,content.player.radius)) {
                    starts[slot].position=foot;
                    starts[slot].facing=std::atan2(placement.position.y-foot.y,placement.position.x-foot.x);
                    found=true;break;
                }
                reach-=floor->distance+content.player.radius*.1f;
                origin=floor->position-Vec3{0,0,content.player.radius*.1f};
            }
            if(found)break;
        }
        if(!found)throw FormatError("Authored vehicle has no clear nearby cached BSP observer start");
    }
    return starts;
}
}
