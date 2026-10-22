#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/collision.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/renderer.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2 || !std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),OriginalData::native_tu15());
    if(!cache.is_mcc())throw FormatError("Machine resource regression requires actual imported MCC");
    auto content=cache.load_content(0,false);
    const auto machine=content.objects.at(355);
    if(machine.type!=7 || !cache.load_source_prt_lighting(0).records.empty())
        throw FormatError("FloodZone machine/empty-SH regression inputs changed");
    content.objects={machine};CollisionWorld collision(content.collision);Mesh mesh;
    mesh.materials.push_back({});mesh.materials.back().name="retained sentinel";
    SourceMachineScene machines(cache,content.objects,0,mesh,collision,true);
    if(machines.stats().deferred_resources!=1 || mesh.materials.size()!=1)
        throw FormatError("Deferred PRT preparation lost its exact resource boundary/rollback");
    SceneObjects lifetimes(content,0,SourceSceneGameContext{1,0});
    machines.synchronize(lifetimes);machines.update_pose(0,0,mesh);
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
    projection.outer=projection.safe={0,0,540,960};
    const auto rectangle=source_frustum_projection(projection).clipping_rectangle();
    const auto eye=machine.position+Vec3{0,-300,0};
    auto camera=source_frustum_camera({eye,{0,-1,0},{0,0,1},.015625f,1024,rectangle});
    SourceObjectViewLod view;view.in_frustum=[&](Vec3 center,float radius) {
        return source_sphere_frustum_visibility({camera.full_frustum.bounds,camera.full_frustum.planes},center,radius)
            !=SourceSphereVisibility::outside;
    };
    unsigned bound_queries{};
    const auto owner=lifetimes.objects().front().handle;
    view.source_bound=[&](std::uint32_t full_handle) {
        if(full_handle!=owner)throw FormatError("Machine bound lookup lost its full source generation");
        ++bound_queries;
        // Declared retained-bound input; native v1343 independently observes
        // this exact authored key sphere. Live callers use current snapshots.
        return ShadowCasterSphere{machine.position,machine.bounding_radius};
    };
    std::vector<Vertex> vertices;std::vector<ObjectLightingBinding> lighting;
    machines.append_view(vertices,lighting,view);
    if(!vertices.empty() || !lighting.empty() || mesh.materials.size()!=1)
        throw FormatError("Outside-frustum machine submitted unresolved geometry/resources");
    camera=source_frustum_camera({eye,{0,1,0},{0,0,1},.015625f,1024,rectangle});
    view.source_visible=[](std::uint32_t,ShadowCasterSphere){return false;};
    machines.append_view(vertices,lighting,view);
    if(!vertices.empty() || !lighting.empty())
        throw FormatError("Unregistered in-frustum machine submitted geometry");
    view.source_visible=[](std::uint32_t,ShadowCasterSphere){return true;};
    bool rejected=false;
    try {machines.append_view(vertices,lighting,view);}
    catch(const FormatError &e) {
        rejected=std::string_view(e.what()).find("requires original source resources:")!=std::string_view::npos;
    }
    if(!rejected || bound_queries!=3 || !vertices.empty() || !lighting.empty() || mesh.materials.size()!=1)
        throw FormatError("Visible unresolved machine bypassed its strict PRT resource boundary");
    for(const auto &object:lifetimes.objects())lifetimes.destroy(object.handle);
    machines.synchronize(lifetimes);machines.append_view(vertices,lighting,view);
    if(!vertices.empty() || !lighting.empty())throw FormatError("Retired machine submitted geometry");
    std::cout<<"PASS actual MCC empty-SH machine: camera-frustum rejection, visible resource failure, unchanged materials and retirement\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
