#include "halo2/source_prop_scene.hpp"
#include "halo2/source_model_regions.hpp"
#include "halo2/scene.hpp"
#include "halo2/collision.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/math.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/renderer.hpp"
#include <cstring>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{
    if(argc!=4)return 77;
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    auto content=cache.load_content(0,true);const auto object=content.objects.at(39);
    if(object.definition!=3919972401u)throw FormatError("actual Turf truck placement missing");
    const auto definition=cache.load_initial_model_definition(object.definition);
    auto bank=source_model_region_bank(cache.load_initial_model_selection(object.definition,object.variant));
    // A single actual authored prop exercises CPU submission without a GPU or
    // audio device. Its root, hierarchy, model, shader and lighting data remain literal.
    LevelContent fixture;fixture.objects={object};fixture.object_names=content.object_names;
    content={};SceneObjects objects(fixture,0,SourceSceneGameContext{2,2});CollisionWorld collision({});Mesh mesh;
    SourcePropScene props(cache,fixture.objects,0,mesh,collision);
    props.region_state([&](std::uint32_t){return &bank.selection;});props.synchronize(objects);
    const auto material_count=mesh.materials.size();const auto original=bank.selection;
    for(unsigned step=0;step<4;++step){
        if(step==1 || step==2)source_model_regions_change(definition,bank,{0,4});
        if(step==3)bank=source_model_region_bank(original);
        props.synchronize(objects);props.update_pose(step,mesh);
        std::vector<Vertex> actual;std::vector<ObjectLightingBinding> bindings;props.append(actual,bindings);
        auto model=cache.load_model(object.definition,false,object.variant,bank.selection,5);
        if(cache.load_source_prt_definition(definition.render_model))cache.apply_source_prt_vertices(model,definition.render_model,5);
        const auto root=scene_object_transform(objects.objects().front());auto expected=pose_model_world(model,root);
        if(expected.size()!=actual.size())throw FormatError("prop renderer submitted inactive model-region geometry");
        for(unsigned i=0;i<actual.size();++i){actual[i].material=expected[i].material=0;
            if(std::memcmp(&actual[i],&expected[i],sizeof(Vertex)))throw FormatError("prop renderer changed active collector order/posed source vertices");}
        if(mesh.materials.size()!=material_count)throw FormatError("prop transition reallocated prepared material resources");
        std::cout<<"CPU prop submission step"<<step<<" vertices="<<actual.size()<<" materials="<<material_count<<'\n';
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
