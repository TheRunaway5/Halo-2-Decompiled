#include "halo2/source_object_light_mix.hpp"
#include "halo2/object_lighting.hpp"
#include <cmath>

namespace halo2 {
namespace {
float length(Vec3 p) {
    return float(std::sqrt((double(p.x)*p.x+double(p.y)*p.y)+double(p.z)*p.z));
}
Vec3 scale(Vec3 p,float s){return {p.x*s,p.y*s,p.z*s};}
Vec3 clamp(Vec3 p){const auto c=[](float v){return v<0.f?0.f:v>1.f?1.f:v;};return {c(p.x),c(p.y),c(p.z)};}
}
SourceObjectLightMix source_object_light_mix(
    const std::optional<SourcePrtContactSample>& prt,
    const std::optional<SourcePrtContactSample>& baked,Vec3 fallback) {
    if(!prt && !baked)return {fallback,0};
    if(!prt)return {baked->incident,pack_object_lighting_rgb(clamp(baked->color))};
    if(!baked)return {prt->incident,pack_object_lighting_rgb(clamp(prt->color))};
    const auto luminance=[](Vec3 c){return (c.z*.11f+c.y*.59f)+c.x*.3f;};
    const auto p=luminance(prt->color),b=luminance(baked->color);
    constexpr float epsilon=.0001f;
    if(!(p>epsilon))return {baked->incident,pack_object_lighting_rgb(clamp(baked->color))};
    if(!(b>epsilon))return {prt->incident,pack_object_lighting_rgb(clamp(prt->color))};
    auto a=baked->incident,c=prt->incident;
    const auto la=length(a),lc=length(c);
    if(la>epsilon)a=scale(a,1.f/la);
    if(lc>epsilon)c=scale(c,1.f/lc);
    // Source weights both directions by color luminance, then normalizes.
    Vec3 incident{c.x*p+a.x*b,c.y*p+a.y*b,c.z*p+a.z*b};
    const double magnitude=std::sqrt((double(incident.x)*incident.x+
        double(incident.y)*incident.y)+double(incident.z)*incident.z);
    if(!(epsilon>magnitude))incident=scale(incident,1.f/float(magnitude));
    incident=scale(incident,lc>la?lc:la);
    const Vec3 color{baked->color.x+prt->color.x,baked->color.y+prt->color.y,baked->color.z+prt->color.z};
    return {incident,pack_object_lighting_rgb(clamp(color))};
}
}
