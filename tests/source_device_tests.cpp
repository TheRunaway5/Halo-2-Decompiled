#include "halo2/source_device.hpp"
#include "halo2/content.hpp"
#include "halo2/world_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_prt.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}}
int main(int argc,char**argv){try{
    std::cout<<std::setprecision(9);
    const auto read=[](BonePose&p){for(auto&v:p.rotation)std::cin>>v;std::cin>>p.translation.x>>p.translation.y>>p.translation.z>>p.scale;};
    const auto write=[](const BonePose&p){for(auto v:p.rotation)std::cout<<v<<' ';std::cout<<p.translation.x<<' '<<p.translation.y<<' '<<p.translation.z<<' '<<p.scale<<'\n';};
    if(argc==2 && std::string_view(argv[1])=="--overlay-probe"){
        unsigned mask;while(std::cin>>mask){BonePose base,delta;read(base);read(delta);write(source_device_overlay(base,delta,mask));}return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--phase-probe"){
        unsigned count;float value;while(std::cin>>count>>value)std::cout<<source_device_channel_frame(count,value)<<'\n';return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--gear-probe"){
        SourceMachineDefinition d;d.type=2;float position,power,dt;
        while(std::cin>>d.powered_position_rate>>d.unpowered_position_rate>>position>>power>>dt){
            const auto step=source_machine_gear_step(d,position,power,dt);
            std::cout<<step.position<<' '<<step.rate<<' '<<step.changed<<' '<<step.wrapped<<'\n';}return 0;
    }
    if(argc==3 && (std::string_view(argv[1])=="--posed-probe" || std::string_view(argv[1])=="--palette-probe")){
        Cache cache(argv[2]);std::uint32_t datum;float position,power,scale;Vec3 origin,forward,up;bool mirror;
        while(std::cin>>datum>>position>>power>>origin.x>>origin.y>>origin.z>>forward.x>>forward.y>>forward.z
            >>up.x>>up.y>>up.z>>scale>>mirror){
            const auto definition=cache.load_source_machine_definition(datum);const auto model=cache.load_model(datum,false);
            const auto locals=source_machine_local_pose(model,definition,position,power);
            const auto object=source_object_transform(origin,forward,up,scale,mirror);
            std::cout<<model.bones.size();
            for(unsigned node=0;node<model.bones.size();++node){
                auto posed=compose_transform(object,pose_model_locals_node(model,node,locals));
                if(std::string_view(argv[1])=="--palette-probe"){
                    const auto&b=model.bones[node];
                    const Transform inverse{b.inverse_scale,{{{b.inverse[0],b.inverse[1],b.inverse[2]},
                        {b.inverse[3],b.inverse[4],b.inverse[5]},{b.inverse[6],b.inverse[7],b.inverse[8]}}},
                        {b.inverse[9],b.inverse[10],b.inverse[11]}};
                    posed=compose_transform(posed,inverse);
                }
                std::cout<<' '<<posed.scale;
                for(auto v:posed.basis)std::cout<<' '<<v.x<<' '<<v.y<<' '<<v.z;
                std::cout<<' '<<posed.position.x<<' '<<posed.position.y<<' '<<posed.position.z;
            }std::cout<<'\n';
        }return 0;
    }
    std::ifstream fixture("tests/fixtures/retail_device_initialization.tsv");require(bool(fixture),"missing original device fixture");
    const bool material_audit=argc==2 && std::string_view(argv[1])=="--material-audit";
    std::unique_ptr<Cache>cache;std::string map,previous;unsigned ordinal,datum,model,type,flags,machine_flags,pflags,qflags,clip_count;
    float power,position;int pgroup,qgroup,selected,pselected;unsigned placements=0,channels=0,rendered=0,materials=0,unsupported=0,projected_materials=0;
    while(fixture>>map>>ordinal>>datum>>model>>type>>flags>>machine_flags>>pgroup>>qgroup>>power>>position>>pflags>>qflags>>selected>>clip_count>>pselected){
        if(map!=previous){cache=std::make_unique<Cache>(std::filesystem::path(std::filesystem::exists("assets/disc/maps/"+map+".map")?"assets/disc/maps":"assets/map-pack/maps")/(map+".map"));previous=map;}
        const auto d=cache->load_source_machine_definition(datum);const auto p=cache->load_source_machine_placement(ordinal);
        require(d.model==model && d.type==type,"original machine definition mismatch");
        require(!d.initialized_idle,"original machine primary should be NONE");
        require(p.device_flags==flags && p.machine_flags==machine_flags && p.power_group==pgroup && p.position_group==qgroup,
            "original machine placement mismatch");
        require(p.initial_power==power && p.initial_position==position && p.power_group_flags==pflags && p.position_group_flags==qflags,
            "original device group initialization mismatch");
        require(bool(d.position)==(selected!=-1) && bool(d.power)==(pselected!=-1),"original device channel admission mismatch");
        if(d.position){require(d.position->animation==selected && d.position->clip.frame_count==clip_count && d.position->animation_type==1,
            "original position overlay selection mismatch");++channels;}
        if(d.render_model!=0xffffffffu){
            const auto m=cache->load_model(datum,true);
            const auto locals=source_machine_local_pose(m,d,p.initial_position,p.initial_power);
            const auto vertices=pose_model_locals(m,locals);
            require(vertices.size()==m.mesh.vertices.size(),"device pose lost geometry");
            if(!m.bones.empty())require(std::isfinite(pose_model_locals_node(m,0,locals).scale),"invalid device query node");
            // All supplied machine models are regular model geometry (7FF),
            // so source2D000 selects implementation2 with a valid lightmap bank.
            if(material_audit)for(auto material:m.mesh.materials){
                if(material.lods.empty())continue;
                if(!prepare_dynamic_material(*cache,material,ObjectLightingState{},true)){
                    if(material.lods.front().available_layers&(1u<<5)){
                        const auto prt=cache->load_source_prt_definition(d.render_model);require(bool(prt),"PRT material lost source model bank");
                        const auto lighting=cache->load_source_prt_lighting();require(!lighting.records.empty(),"machine PRT source SH bank missing");
                        const auto bank=source_prt_bank(*prt,5,std::span<const SourcePrtSh>(&lighting.records.front().lighting,1));
                        if(prepare_prt_material(*cache,material,bank)){
                            require(material.environment_pass && material.environment_pass->forward_vertex==9 &&
                                material.environment_pass->vertex_bank==bank && material.forward_vertex==3 && !material.vertex_bank,
                                "source PRT pass scheduler/bank selection");++projected_materials;continue;
                        }
                        ++unsupported;std::cerr<<"PRT "<<map<<' '<<ordinal<<' '<<std::quoted(material.name)<<'\n';continue;
                    }
                    for(const auto&pass:material.passes)if(pass.implementation==2 || pass.implementation==0){
                        std::cerr<<pass.template_pass<<' '<<pass.implementation<<' '<<pass.name<<' '<<pass.vertex_shader<<'\n';
                        for(const auto&r:pass.vertex_externs)std::cerr<<"ve "<<r.destination<<' '<<unsigned(r.component)<<' '<<unsigned(r.source)<<'\n';
                        for(const auto&r:pass.pixel_externs)std::cerr<<"pe "<<r.destination<<' '<<unsigned(r.component)<<' '<<unsigned(r.source)<<'\n';
                        for(const auto&t:pass.textures)std::cerr<<"tex "<<unsigned(t.parameter)<<' '<<unsigned(t.external)<<' '<<unsigned(t.stage)<<'\n';
                        for(const auto&s:pass.states)std::cerr<<"rs "<<unsigned(s.index)<<' '<<s.value<<'\n';
                    }
                    throw std::runtime_error("machine dynamic pass not admitted: "+map+" "+d.name+" "+material.name);
                }
                ++materials;
            }++rendered;
        }++placements;
    }
    require(fixture.eof(),"invalid original device fixture");require(placements==49 && channels==37,"original machine inventory changed");
    // An overlay on a translated/scaled bind pose must keep those channels.
    const auto composed=source_device_overlay({{2,3,4},{0,0,0,1},2},{{1,1,1},{0,0,1,0},3},7);
    require(composed.translation.x==3 && composed.translation.z==5 && composed.scale==6 && composed.rotation[2]==1,"device overlay composition");
    require(source_device_channel_frame(10,2)==9.0001f,"source channel endpoint clamp");
    SourceMachineDefinition gear;gear.type=2;gear.powered_position_rate=3;
    require(source_machine_gear_step(gear,.5f,1,1).position==2.5f,"source gear must subtract one period once");
    std::cout<<"device source fixtures: "<<placements<<" placements, "<<channels<<" position overlays, "<<rendered<<" render models";
    if(material_audit)std::cout<<", "<<materials<<" dynamic materials, "<<projected_materials<<" projected materials, "<<unsupported<<" unresolved PRT materials";
    std::cout<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
