// Real-map check of the sun volume bound: a sunlit Coagulation spawn must get
// a sun direction, and its volume must end just past the ground beneath it
// instead of at the 1024-unit far distance.
#include "halo2/pstencil/lightmap.hpp"
#include "halo2/pstencil/frame.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/cache.hpp"
#include <cmath>
#include <iostream>

using namespace halo2;
namespace {
void require(bool b,const char* why){if(!b)throw FormatError(why);}
}
int main(int argc,char** argv){try {
    require(argc==2,"provide the Coagulation cache");
    Cache cache(argv[1]);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto surfaces=cache.load_bsp_surface_lighting(0);
    std::vector<std::optional<ObjectSkyLightingDefinition>> skies;
    for(unsigned i=0;i<atmosphere.skies.size();++i) {
        try{skies.push_back(cache.load_object_sky_lighting(i));}
        catch(const FormatError&){skies.emplace_back();}
    }
    // The spawn root retained by the 2026-11-10 multiplayer capture, raised
    // to a biped's centre.
    const Vec3 center{38.8024f,-60.8789f,1.21751f+.35f};
    const float radius=.5f;
    const auto sun=pstencil::sun_shadow_direction(atmosphere,surfaces,skies,center);
    require(bool(sun),"The retained sunlit Coagulation spawn has no sun direction");
    const float magnitude=std::sqrt((sun->x*sun->x+sun->y*sun->y)+sun->z*sun->z);
    require(std::fabs(magnitude-1.f)<.01f,"Sky incident is not a unit direction");
    const Vec3 top{center.x+sun->x*radius,center.y+sun->y*radius,center.z+sun->z*radius};
    const auto ground=original_bsp_vector_query(surfaces.collision.front(),top,
        {-sun->x*1024.f,-sun->y*1024.f,-sun->z*1024.f},1.f,0x2b);
    require(ground.contact && ground.contact->material!=-1,"No visible receiver lies beneath the spawn");
    const float reach=ground.contact->fraction*1024.f;
    const float length=pstencil::sun_shadow_extrusion(surfaces,*sun,center,radius);
    std::cout<<"sun="<<sun->x<<','<<sun->y<<','<<sun->z<<" receiver="<<reach<<" extrusion="<<length<<'\n';
    require(length>=reach,"Bounded sun volume stops short of its receiver");
    // A low sun stretches the lateral excess by 1/tan(elevation); eight
    // radii still bound the leak to a few caster sizes, not the far plane.
    require(length<reach+8.f*radius,"Bounded sun volume runs far past its receiver");
    require(length<32.f,"Grounded caster kept an unbounded sun volume");
    // A caster raised well above the ground needs a correspondingly longer volume.
    const Vec3 raised{center.x,center.y,center.z+10.f};
    const float lifted=pstencil::sun_shadow_extrusion(surfaces,*sun,raised,radius);
    require(lifted>length+5.f && lifted<1024.f,"Airborne caster no longer reaches the ground");
    // Degenerate inputs retain the unobstructed far distance.
    require(pstencil::sun_shadow_extrusion(surfaces,{0,0,0},center,radius)==pstencil::sun_shadow_far_distance &&
        pstencil::sun_shadow_extrusion(surfaces,*sun,center,0.f)==pstencil::sun_shadow_far_distance &&
        pstencil::sun_shadow_extrusion(BspSurfaceLighting{},*sun,center,radius)==pstencil::sun_shadow_far_distance,
        "Degenerate sun input produced a bounded volume");
    // The vertex overload derives the same radius from posed geometry.
    std::vector<Vertex> vertices(3);
    vertices[0].position={center.x+radius,center.y,center.z};
    vertices[1].position={center.x,center.y-radius*.5f,center.z};
    vertices[2].position={center.x,center.y,center.z+radius*.25f};
    require(pstencil::sun_shadow_extrusion(surfaces,*sun,center,vertices)==length,
        "Posed-vertex radius diverged from its bounding sphere");
    // World-only composite: every owner's posed surface joins the view's
    // exclusion batch, including an owner whose camouflage casts nothing.
    pstencil::Frame frame(2);
    pstencil::LightmapCaster hidden{vertices,*sun,true,length,0};
    frame.add_lightmap(0,hidden,{});
    require(frame.lightmaps(0).empty() && frame.lightmap_owners(0) &&
        frame.lightmap_owners(0)->volume.size()==3 && !frame.lightmap_owners(1),
        "A non-casting owner left the world-only exclusion batch or crossed views");
    for(unsigned n=0;n<3;++n) {
        const auto& out=frame.lightmap_owners(0)->volume[n].position;const auto& in=vertices[n].position;
        require(out.x==in.x && out.y==in.y && out.z==in.z,"Owner exclusion vertex differs from its posed surface");
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
