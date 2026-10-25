#pragma once
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace halo2::test {
// Read-only fixture input planning. These BSP queries select walking/looking
// inputs only; they never provide a contact, pose, projectile hit or damage.
class DestructionFiringLaneInputs {
    CollisionWorld terrain_, render_;
    float height_, radius_, camera_, slope_;

    static std::vector<CollisionTriangle> render_triangles(const Cache &cache,std::size_t bsp) {
        const auto mesh=cache.load_bsp(bsp,false);
        std::vector<CollisionTriangle> triangles;triangles.reserve(mesh.vertices.size()/3);
        for(std::size_t n=0;n+2<mesh.vertices.size();n+=3)
            triangles.push_back({mesh.vertices[n].position,mesh.vertices[n+1].position,mesh.vertices[n+2].position});
        return triangles;
    }
    std::optional<Vec3> floor(Vec3 near)const {
        auto origin=near+Vec3{0,0,height_};float reach=3*height_;
        for(unsigned layer=0;layer<8 && reach>0;++layer) {
            const auto hit=terrain_.raycast(origin,{0,0,-1},reach);if(!hit)break;
            const auto foot=hit->position+Vec3{0,0,radius_+.01f};
            if(hit->normal.z>=slope_ && std::abs(foot.z-near.z)<height_ &&
                !terrain_.overlaps(foot,height_,radius_))return foot;
            const auto advance=hit->distance+.01f;reach-=advance;origin.z-=advance;
        }
        return {};
    }
    bool walkable(Vec3 from,Vec3 to)const {
        const auto distance=std::hypot(to.x-from.x,to.y-from.y);
        const auto steps=std::max(1u,unsigned(std::ceil(distance/std::max(radius_,.05f))));
        auto previous=from;
        for(unsigned n=1;n<=steps;++n) {
            auto sample=from+(to-from)*(float(n)/float(steps));sample.z=previous.z;
            const auto support=floor(sample);
            if(!support || std::abs(support->z-previous.z)>radius_)return false;
            previous=*support;
        }
        return std::abs(previous.z-to.z)<=radius_;
    }
public:
    DestructionFiringLaneInputs(const Cache &cache,const LevelContent &content,std::size_t bsp)
        :terrain_(content.collision),render_(render_triangles(cache,bsp)),
         height_(content.player.height),radius_(content.player.radius),
         camera_(content.player.camera_height),slope_(content.player.maximum_slope_cosine){}

    bool clear(Vec3 eye,Vec3 point)const {
        const auto delta=point-eye;const auto distance=length(delta);
        if(!(distance>.02f))return false;
        const auto ray=delta/distance;
        // A conservative corridor keeps the declared input ray away from BSP
        // edges. Its width is an input-planning choice, not a rocket radius.
        const auto side=normalized(Vec3{-ray.y,ray.x,0})*radius_;
        for(const auto offset:std::array{Vec3{},side,side*(-1.f),Vec3{0,0,radius_},Vec3{0,0,-radius_}})
            if(terrain_.raycast(eye+offset,ray,distance-.02f) ||
                render_.raycast(eye+offset,ray,distance-.02f))return false;
        return true;
    }

    template<class Sight>
    std::optional<Vec3> vantage(Vec3 from,Vec3 root,float minimum_range,Sight sight)const {
        std::vector<Vec3> candidates;
        for(unsigned ring=0;ring<6;++ring)for(unsigned n=0;n<32;++n) {
            const auto angle=float(n)*6.28318530718f/32.f;
            const auto reach=minimum_range+height_*float(1+ring);
            const auto near=Vec3{root.x+std::cos(angle)*reach,root.y+std::sin(angle)*reach,from.z};
            if(const auto foot=floor(near))candidates.push_back(*foot);
        }
        std::ranges::sort(candidates,[&](Vec3 a,Vec3 b){return length(a-from)<length(b-from);});
        for(const auto foot:candidates)
            if(sight(foot+Vec3{0,0,camera_}) && walkable(from,foot))return foot;
        return {};
    }
};
}
