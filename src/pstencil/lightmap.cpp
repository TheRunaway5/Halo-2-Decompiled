#include "halo2/pstencil/lightmap.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/object_sky_lighting.hpp"
#include <algorithm>
#include <cmath>

namespace halo2::pstencil {
std::optional<Vec3> sun_shadow_direction(const BspAtmosphere& atmosphere,
    const BspSurfaceLighting& surfaces,
    std::span<const std::optional<ObjectSkyLightingDefinition>> skies,Vec3 origin) {
    const auto cluster=atmosphere.cluster_at(origin);
    if(cluster<0 || std::size_t(cluster)>=atmosphere.clusters.size() || surfaces.collision.empty())return {};
    const auto sky=atmosphere.clusters[cluster].visible_sky;
    if(sky<0 || std::size_t(sky)>=skies.size() || !skies[sky])return {};
    const auto frame=object_sky_light_frame(*skies[sky]);
    const auto d=frame.incident;
    if(!frame.test_collision || !(d.x*d.x+d.y*d.y+d.z*d.z>.5f) ||
       !(frame.radiosity.x>0 || frame.radiosity.y>0 || frame.radiosity.z>0))return {};
    // Invisible collision boundaries (such as Coagulation's ceiling) do
    // not obstruct sunlight. Keep visible world geometry in the ray query.
    const auto ray=original_bsp_vector_query(surfaces.collision.front(),origin,
        {d.x*100.f,d.y*100.f,d.z*100.f},1.f,0x2b);
    if(ray.contact && ray.contact->material!=-1)return {};
    return d;
}

namespace {
// Distance along a unit direction to the first visible world surface.
std::optional<float> visible_surface_distance(const BspSurfaceLighting::CollisionBank& bank,
    Vec3 origin,Vec3 direction,float length) {
    const auto ray=original_bsp_vector_query(bank,origin,
        {direction.x*length,direction.y*length,direction.z*length},1.f,0x2b);
    if(!ray.contact || ray.contact->material==-1)return {};
    return ray.contact->fraction*length;
}
}

float sun_shadow_extrusion(const BspSurfaceLighting& surfaces,Vec3 incident,Vec3 center,float radius) {
    constexpr float far=sun_shadow_far_distance;
    const float magnitude=std::sqrt((incident.x*incident.x+incident.y*incident.y)+incident.z*incident.z);
    if(surfaces.collision.empty() || !std::isfinite(magnitude) || !(magnitude>.0001f) ||
       !std::isfinite(radius) || !(radius>0) || !std::isfinite(center.x+center.y+center.z))return far;
    const Vec3 sun{incident.x/magnitude,incident.y/magnitude,incident.z/magnitude};
    const Vec3 away{-sun.x,-sun.y,-sun.z};
    // Any unit vector not parallel to the sun completes the lateral basis.
    const Vec3 seed=std::fabs(sun.z)<.9f?Vec3{0,0,1}:Vec3{1,0,0};
    Vec3 u{sun.y*seed.z-sun.z*seed.y,sun.z*seed.x-sun.x*seed.z,sun.x*seed.y-sun.y*seed.x};
    const float scale=std::sqrt((u.x*u.x+u.y*u.y)+u.z*u.z);
    u={u.x/scale,u.y/scale,u.z/scale};
    const Vec3 v{sun.y*u.z-sun.z*u.y,sun.z*u.x-sun.x*u.z,sun.x*u.y-sun.y*u.x};
    const Vec3 top{center.x+sun.x*radius,center.y+sun.y*radius,center.z+sun.z*radius};
    const auto& bank=surfaces.collision.front();
    const auto middle=visible_surface_distance(bank,top,away,far);
    if(!middle)return far;
    float lateral=*middle;
    for(const auto side:{u,Vec3{-u.x,-u.y,-u.z},v,Vec3{-v.x,-v.y,-v.z}})
        if(const auto hit=visible_surface_distance(bank,
            {top.x+side.x*radius,top.y+side.y*radius,top.z+side.z*radius},away,far))
            lateral=std::max(lateral,*hit);
    // Caster extrusion is measured from each vertex, which lies at most one
    // diameter below the tangent plane; the small bias closes coplanar caps.
    return std::min(far,*middle+1.5f*(lateral-*middle)+.05f);
}

float sun_shadow_extrusion(const BspSurfaceLighting& surfaces,Vec3 incident,Vec3 center,
    std::span<const Vertex> vertices) {
    float squared=0;
    for(const auto& vertex:vertices) {
        const Vec3 o{vertex.position.x-center.x,vertex.position.y-center.y,vertex.position.z-center.z};
        squared=std::max(squared,(o.x*o.x+o.y*o.y)+o.z*o.z);
    }
    return sun_shadow_extrusion(surfaces,incident,center,std::sqrt(squared));
}

Vec3 lightmap_light_origin(Vec3 incident) {
    const double x=incident.x,y=incident.y,z=incident.z;
    const auto magnitude=std::sqrt((x*x+y*y)+z*z);
    if(!std::isfinite(magnitude) || magnitude<.0001)
        throw FormatError("pStencil lightmap shadow requires a valid incident sample");
    const auto inverse=float(1.0/magnitude);
    return {(incident.x*inverse)*20000.f,(incident.y*inverse)*20000.f,(incident.z*inverse)*20000.f};
}
}
