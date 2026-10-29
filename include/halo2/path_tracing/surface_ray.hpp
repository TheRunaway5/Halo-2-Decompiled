#pragma once
#include "halo2/path_tracing/scene.hpp"
#include <cmath>
#include <limits>

namespace halo2::path_tracing {
namespace surface_ray_detail {
inline float ulp(float value) {
    const auto magnitude=std::abs(value);
    return std::nextafter(magnitude,std::numeric_limits<float>::infinity())-magnitude;
}
inline Vec3 xyz(const ShaderVector& value){return {value[0],value[1],value[2]};}
inline Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline float dot(Vec3 a,Vec3 b){return (a.x*b.x+a.y*b.y)+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 unit(Vec3 value) {
    const float squared=dot(value,value);if(!(squared>1.e-18f))return {};
    const float reciprocal=1.f/std::sqrt(squared);return {value.x*reciprocal,value.y*reciprocal,value.z*reciprocal};
}
inline float tolerance(Vec3 vertex,Vec3 position,Vec3 normal) {
    const Vec3 error{(ulp(vertex.x)+ulp(position.x))*4.f,(ulp(vertex.y)+ulp(position.y))*4.f,
        (ulp(vertex.z)+ulp(position.z))*4.f};
    const float roundoff=(std::abs(normal.x)*error.x+std::abs(normal.y)*error.y)+std::abs(normal.z)*error.z;
    return .0002f+roundoff;
}
inline bool edge_contains(Vec3 a,Vec3 b,Vec3 position,Vec3 normal) {
    const auto inward=unit(cross(normal,sub(b,a)));
    return dot(sub(position,a),inward)>=-tolerance(a,position,inward);
}
}
// CPU validation mirror of surface_ray.glsl. Production traversal stays on
// the GPU; no extra visibility query or ray-origin displacement is introduced.
inline bool contains_starting_surface(const GpuTriangle& triangle,Vec3 position) {
    using namespace surface_ray_detail;
    const auto a=xyz(triangle.p0),b=xyz(triangle.p1),c=xyz(triangle.p2),normal=unit(cross(sub(b,a),sub(c,a)));
    if(dot(normal,normal)==0.f)return false;
    return std::abs(dot(sub(position,a),normal))<=tolerance(a,position,normal) &&
        edge_contains(a,b,position,normal) && edge_contains(b,c,position,normal) && edge_contains(c,a,position,normal);
}
inline bool skip_starting_surface(const GpuTriangle& triangle,std::uint32_t index,Vec3 position,Vec3 geometric,
    Vec3 direction,bool smooth_terminator,std::uint32_t source_triangle=0xffffffffu) {
    using namespace surface_ray_detail;
    if(!smooth_terminator)return false;
    auto normal=unit(cross(sub(xyz(triangle.p1),xyz(triangle.p0)),sub(xyz(triangle.p2),xyz(triangle.p0))));
    if(dot(normal,normal)==0.f)return false;
    if(dot(normal,geometric)<0.f)normal={-normal.x,-normal.y,-normal.z};
    return dot(normal,direction)<=0.f && (index==source_triangle || contains_starting_surface(triangle,position));
}
}
