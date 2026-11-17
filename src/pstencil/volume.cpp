#include "halo2/pstencil/geometry.hpp"
#include "halo2/pstencil/shadow.hpp"
#include <cmath>

namespace halo2::pstencil {
namespace {
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
void append_prisms(std::vector<ShadowVertex>& output,std::span<const Vec3> points,
    std::span<const std::array<std::uint16_t,3>> triangles,Vec3 light,unsigned stipple_level,bool sun,
    float sun_extrusion=0) {
    if(stipple_level>16)throw FormatError("pStencil opacity escapes original17-pattern bank");
    if(!stipple_level)return;
    if(!finite(light))throw FormatError("pStencil light is nonfinite");
    const auto emit=[&](std::uint16_t index,bool far) {
        if(index>=points.size())throw FormatError("pStencil triangle escapes source points");
        auto position=points[index];
        if(sun && far) {
            // At most the ordinary source view's 1024-unit far distance;
            // shorter when the caster's first visible receiver is nearer.
            position={position.x-light.x*sun_extrusion,position.y-light.y*sun_extrusion,position.z-light.z*sun_extrusion};
        }
        output.push_back({position,far && !sun?1.f:0.f,stipple_level});
    };
    for(const auto& face:triangles) {
        for(auto index:face)if(index>=points.size())throw FormatError("pStencil triangle escapes source points");
        const auto a=points[face[0]],b=points[face[1]],c=points[face[2]];
        const Vec3 ab{b.x-a.x,b.y-a.y,b.z-a.z},ac{c.x-a.x,c.y-a.y,c.z-a.z};
        const Vec3 n{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
        const Vec3 toward=sun?light:Vec3{light.x-a.x,light.y-a.y,light.z-a.z};
        const float facing=(n.x*toward.x+n.y*toward.y)+n.z*toward.z;
        // The original plane-facing bitvector complements sign bits. Zero
        // area produces no silhouette; a light exactly on a plane has no
        // volume in that plane and is omitted without a tuning epsilon.
        if(!(facing>0))continue;
        emit(face[0],false);emit(face[1],false);emit(face[2],false);
        emit(face[2],true);emit(face[1],true);emit(face[0],true);
        for(unsigned edge=0;edge<3;++edge) {
            // Use local doubled indices to avoid narrowing actual ushort
            // source indices above32767. Each prism's two endpoint slots
            // are exactly the original near/far0,1 and2,3 convention.
            const auto quad=stitch_quad(0,1,true,false);
            const auto first=face[edge],second=face[(edge+1)%3];
            const auto q=[&](unsigned slot) {
                const auto index=quad.indices[slot];emit(index/2?second:first,index&1u);
            };
            q(0);q(1);q(2);q(0);q(2);q(3);
        }
    }
}
}
void append_volume(std::vector<ShadowVertex>& output,std::span<const Vec3> points,
    std::span<const std::array<std::uint16_t,3>> triangles,Vec3 light,unsigned stipple_level) {
    append_prisms(output,points,triangles,light,stipple_level,false);
}
void append_sun_volume(std::vector<ShadowVertex>& output,std::span<const Vec3> points,
    std::span<const std::array<std::uint16_t,3>> triangles,Vec3 incident,float extrusion,
    unsigned stipple_level) {
    if(!std::isfinite(extrusion) || !(extrusion>0) || extrusion>1024.f)
        throw FormatError("pStencil sun extrusion escapes the source view distance");
    append_prisms(output,points,triangles,incident,stipple_level,true,extrusion);
}
} // namespace halo2::pstencil
