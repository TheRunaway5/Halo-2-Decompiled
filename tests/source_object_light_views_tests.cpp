#include "halo2/source_prop_scene.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
bool same(const ObjectLightingState &a,const ObjectLightingState &b){
    const auto x=a.source_values(),y=b.source_values();
    for(unsigned i=0;i<x.size();++i)
        if(std::bit_cast<std::uint32_t>(x[i])!=std::bit_cast<std::uint32_t>(y[i]))return false;
    return true;
}
ObjectLightQueryAdmission query(const BspSurfaceLighting &surface,const CollisionWorld &collision,
    const ObjectLightQueryContext &context,Vec3 origin,bool initial){
    return source_object_light_query(context,[&](unsigned mode){
        return query_object_lightmap(surface,collision,origin,ObjectLightMapQueryContext{mode,initial,1});
    },[]{return ObjectLightQueryResult{};});
}
SourceObjectViewLod view(unsigned index,float distance){
    SourceObjectViewLod result;result.view_index=index;result.local_players=2;
    result.input=[distance](std::size_t object,std::uint32_t handle,Vec3,float radius){
        require(object==0 && handle!=null_datum && radius>=0,"paired-view source owner identity changed");
        // Explicit pre4BCC0 metrics isolate cache cadence from geometry detail.
        // Flag19 fixes the resident geometry; it does not modify this metric.
        return SourceObjectLodInput{distance,0x80000,0};
    };
    return result;
}
ObjectLightingState submit(SourcePropScene &scene,unsigned index,float distance){
    std::vector<Vertex> vertices;std::vector<ObjectLightingBinding> banks;
    scene.append_view(vertices,banks,view(index,distance));
    require(!vertices.empty() && banks.size()==1,"paired-view bus omitted its ordinary shader binding");
    return banks.front().state;
}
}
int main(){try{
    const Cache cache("assets/disc/maps/headlong.map");
    const auto content=cache.load_content(0,true);
    const auto found=std::find_if(content.objects.begin(),content.objects.end(),[](const auto &object){
        return object.type==11 && object.definition==0xeb4309ceu;
    });
    require(found!=content.objects.end(),"literal Headlong bus placement is absent");
    LevelContent selected=content;selected.objects={*found};
    const auto model=cache.load_model(found->definition,false,found->variant);
    const auto basis=authored_basis(found->rotation);
    const auto object=source_object_transform(found->position,basis.forward,basis.up,found->scale,false);
    const auto node0=compose_transform(object,pose_model_node(model,0));
    const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(found->definition),node0,found->scale);
    const auto surface=cache.load_bsp_surface_lighting();
    const CollisionWorld collision(content.collision);
    const auto conversion=cache.load_object_lighting_definition();
    const auto definition=cache.load_source_prop_definition(found->definition);
    ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(surface.lighting);
    context.object_type=11;context.definition_flags=definition.object_flags;
    const auto initial=query(surface,collision,context,sphere.center,true);
    require(initial.accepted && initial.query.sample,"actual bus constructor sphere query is unresolved");

    // The later nonfrozen physics/cache packet is a controlled caller input.
    // Its origin is an actual authored spawn with a resolved source surface;
    // this test does not claim a live physics transition or move the bus mesh.
    ObjectLightingCache expected;
    update_object_lighting_cache(expected,conversion,11,0,sphere.center,{},true,initial.query.sample,true,false);
    std::optional<Vec3> later_origin;ObjectLightQueryAdmission later;
    for(const auto &spawn:cache.spawns()){
        if(length(spawn.position-sphere.center)<.1f)continue;
        const auto candidate=query(surface,collision,context,spawn.position,false);
        if(!candidate.accepted || !candidate.query.sample)continue;
        auto near=expected;
        update_object_lighting_cache(near,conversion,11,4,spawn.position,{40,0,0},false,{},true,false);
        auto far=near;
        update_object_lighting_cache(far,conversion,11,4,spawn.position,{40,0,0},true,candidate.query.sample,true,false);
        if(!same(near.lighting,far.lighting)){later_origin=spawn.position;later=candidate;break;}
    }
    require(bool(later_origin),"authored Headlong query origins have no meaningful paired-view light response");
    Mesh mesh;SourcePropScene scene(cache,selected.objects,0,mesh,collision);
    SceneObjects owners(selected,0);scene.synchronize(owners);
    Vec3 origin=sphere.center,velocity{};
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform &,float scale){
        require(index==0 && handle!=null_datum && scale==found->scale,"query callback lost actual bus owner");
        return SourcePropQueryState{{origin,sphere.radius},velocity,0,false};
    });
    scene.update_pose(0,mesh);
    const auto first=submit(scene,0,1);
    require(same(first,expected.lighting),"initial scene binding differs from the actual source query/conversion");
    origin=*later_origin;velocity={40,0,0};scene.update_pose(4,mesh);
    require(!object_lighting_query_due(expected,4,origin,1,false),"near-view literal interval60 did not suppress query");
    update_object_lighting_cache(expected,conversion,11,4,origin,velocity,false,{},true,false);
    require(expected.last_query_frame==0,"suppressed first-view query changed its cache stamp");
    const auto near=submit(scene,0,1);
    require(same(near,expected.lighting),"first view did not preserve the source retained lighting");
    require(object_lighting_query_due(expected,4,origin,401,false),"later-view literal interval3 did not admit query");
    auto capped=expected; // Negative control: the removed blanket frame gate.
    update_object_lighting_cache(expected,conversion,11,4,origin,velocity,true,later.query.sample,true,false);
    require(expected.last_query_frame==4,"later-view real query did not stamp the current render frame");
    const auto far=submit(scene,1,401);
    require(same(far,expected.lighting),"later view retained the suppressed first-view lighting bank");
    require(!same(far,capped.lighting),"first-view-only negative control did not change the actual binding");
    const auto repeated=submit(scene,0,1);
    require(same(repeated,far),"source last-query-frame guard changed an already queried bank");
    std::cout<<"Headlong bus: real attributed initial/later samples; frame4 near1 suppressed, far401 queried, "
        "first-view-only control differs; repeated queried frame unchanged\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
