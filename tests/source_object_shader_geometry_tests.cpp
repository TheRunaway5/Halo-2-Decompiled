#include "../src/gameplay/objects/source_object_geometry.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/scenery.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <cstdlib>
#include <cstring>
using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
SourceObjectViewLod source_view(unsigned index,unsigned players,unsigned height) {
    SourceObjectViewLod result{index,players,false,[](std::size_t index,std::uint32_t handle,Vec3,float){
        require(index==37 && handle==0x12340025,"shader selection lost the caller's owner identity");
        //Keep geometry fixed while changing the separate source shader producer.
        return SourceObjectLodInput{0,0x80000,0};
    }};
    result.shader=SourceModelShaderView{{{0,0,-1,0},float(height)*.5f},
        source_shader_lod_view(0,std::int16_t(height),false,false),
        players==4?.7f:players>1?.8f:1.f,source_shader_lod_player_multiplier(players),false};
    return result;
}
void equal_geometry(std::span<const Vertex> a,std::span<const Vertex> b) {
    require(a.size()==b.size(),"shader detail changed the selected geometry count");
    for(unsigned i=0;i<a.size();++i) {
        require(a[i].position.x==b[i].position.x && a[i].position.y==b[i].position.y &&
            a[i].position.z==b[i].position.z && a[i].source_section==b[i].source_section &&
            a[i].source_part==b[i].source_part,"shader detail replaced a geometry stream or source part");
    }
}
struct Checked {unsigned vertices{},lower{},ordinary{},prt{};};
Checked check_draw(const Cache &cache,const SourceObjectGeometry &geometry,const Mesh &mesh,
    const SourceObjectViewLod &view,Vec3 center,float radius,const ObjectColors &colors) {
    const auto &base=geometry.at(5);const auto &draw=geometry.view(view.view_index);
    equal_geometry(base.vertices,draw.vertices);
    const auto collector=source_model_shader_collector(center,radius,*view.shader);
    const auto packed=source_object_render_colors(colors);Checked count;
    std::map<std::uint32_t,SourceShaderLodDefinition> definitions;
    bool ordinary=false;
    for(unsigned i=0;i<draw.vertices.size();++i) {
        const auto &origin=base.model.vertex_origins.at(i);
        const auto section=std::find_if(base.authored_parts.sections.begin(),base.authored_parts.sections.end(),
            [&](const auto &part){return part.section==origin.section;});
        require(section!=base.authored_parts.sections.end(),"source collector lost the literal section bank");
        const auto &part=section->parts.at(origin.part);
        auto [definition,inserted]=definitions.try_emplace(part.shader);
        if(inserted)definition->second=cache.load_source_shader_lod_definition(part.shader);
        const auto choice=source_model_shader_part(collector,part.lod_scale>0?part.lod_scale:1.f,
            definition->second,*view.shader).choice;
        require(choice.accepted && choice.index>=0,"authored fixture part was rejected by the source selector");
        const auto &material=mesh.materials.at(draw.vertices[i].material);
        require(material.source_shader_datum==part.shader && material.selected_shader_lod==unsigned(choice.index),
            "selected draw bound another source part's shader detail record");
        require(bool(material.object_render_colors),"resident lower shader lost the retained object color bank");
        for(unsigned c=0;c<4;++c)require((*material.object_render_colors)[c].x==packed[c].x &&
            (*material.object_render_colors)[c].y==packed[c].y && (*material.object_render_colors)[c].z==packed[c].z,
            "resident lower shader changed the retained quantized object color bank");
        const bool prt=bool(material.lods.at(material.selected_shader_lod).available_layers&(1u<<5));
        if(prt) {
            require(material.environment_pass && material.environment_pass->forward_vertex==9 &&
                material.environment_pass->vertex_bank && material.environment_pass->vertex_bank->prt_inputs,
                "selected source PRT pass lost its geometry-section coefficient bank");
            ++count.prt;
        } else {
            const auto &lighting=material.environment_pass?*material.environment_pass:material;
            require(lighting.forward_vertex==6 && !lighting.vertex_bank,
                "ordinary selected shader borrowed a PRT declaration or coefficient bank");
            ordinary=true;++count.ordinary;
        }
        ++count.vertices;count.lower+=choice.index>0;
    }
    require(draw.ordinary_lighting==ordinary,"ordinary shader binding was derived from an unused resident shader");
    return count;
}
void authored_views(const char *map,std::uint32_t datum,bool require_lower,bool require_prt,bool pure_prt=false) {
    Cache cache(std::filesystem::path("assets/disc/maps")/(std::string(map)+".map"));
    const auto content=cache.load_content(0,true);
    const auto object=std::find_if(content.objects.begin(),content.objects.end(),
        [&](const auto&o){return o.definition==datum;});
    require(object!=content.objects.end(),"original model shader fixture placement is absent");
    const auto definition=cache.load_initial_model_definition(datum);
    const auto selection=cache.load_initial_model_selection(datum,object->variant);
    SourceObjectGeometry geometry;geometry.load(cache,*object,definition,selection,definition.render_model);
    const auto basis=authored_basis(object->rotation);
    const auto transform=source_object_transform(object->position,basis.forward,basis.up,object->scale,false);
    geometry.pose_world([&](const auto&m){return pose_model_world(m,transform);});
    const auto nodes=pose_model_nodes_world(geometry.model(),transform);
    const ObjectColors colors{{{.1251f,.351f,.701f},{.911f,.202f,.052f},{.302f,.801f,.499f},{.043f,.577f,.877f}}};
    Mesh mesh;const auto resident=geometry.prepare(cache,colors,nodes,cache.load_source_prt_lighting(),mesh);
    const Vec3 near{0,0,-.1f},far{0,0,-10000};unsigned lower=0,prt=0,vertices=0;
    for(unsigned players:{1u,2u,4u})for(unsigned height:{480u,720u}) {
        auto close=source_view(0,players,height),distant=source_view(1,players,height);
        const auto a=geometry.select(37,0x12340025,near,1,close);
        const auto b=geometry.select(37,0x12340025,far,1,distant);
        require(a.lod==5 && b.lod==5,"shader producer changed forced source geometry detail");
        // Compare complete bytes and admission fields with the original
        // per-vertex selector on the same actual bank and retained history.
        for(const auto &[v,center]:std::array{std::pair{close,near},std::pair{distant,far}}) {
            const auto expected=geometry.view(v.view_index);
#ifdef _WIN32
            _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1");
#else
            setenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1",1);
#endif
            geometry.select(37,0x12340025,center,1,v);
#ifdef _WIN32
            _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","");
#else
            unsetenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION");
#endif
            const auto &reference=geometry.view(v.view_index);
            require(expected.ordinary_lighting==reference.ordinary_lighting && expected.vertices.size()==reference.vertices.size(),
                "run selector changed original ordinary admission or count");
            require(expected.vertices.empty() || !std::memcmp(expected.vertices.data(),reference.vertices.data(),expected.vertices.size()*sizeof(Vertex)),
                "run selector changed complete original object vertices");
        }
        const auto checked_a=check_draw(cache,geometry,mesh,close,near,1,colors);
        const auto checked_b=check_draw(cache,geometry,mesh,distant,far,1,colors);
        require(!pure_prt || (!checked_a.ordinary && !checked_b.ordinary &&
            !geometry.view(0).ordinary_lighting && !geometry.view(1).ordinary_lighting),
            "pure PRT submitted parts admitted an unused ordinary shader binding");
        lower+=checked_b.lower;prt+=checked_a.prt+checked_b.prt;vertices+=checked_a.vertices+checked_b.vertices;
        const auto preserved=geometry.view(1).vertices;
        geometry.select(37,0x12340025,far,1,close);
        const auto &unchanged=geometry.view(1).vertices;
        equal_geometry(preserved,unchanged);
        for(unsigned i=0;i<unchanged.size();++i)require(preserved[i].material==unchanged[i].material,
            "later player shader selection overwrote another view's material slots");
    }
    require(!require_lower || lower,"real lower shader variants were never selected by distant source coverage");
    require(!require_prt || prt,"real source PRT material was never submitted");
    if(require_lower) {
        const auto &legacy=geometry.at(5);
        const auto distant=source_view(0,1,480);geometry.select(37,0x12340025,far,1,distant);
        unsigned changed=0;for(unsigned i=0;i<legacy.vertices.size();++i)
            changed+=legacy.vertices[i].material!=geometry.view(0).vertices[i].material;
        require(changed,"highest-only negative control did not change any actual part's selected shader");
    }
    SourceObjectViewLod legacy{2,1,false,[](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{0,0x80000,0};}};
    geometry.select(37,0x12340025,near,1,legacy);
    const auto &old=geometry.at(5).vertices,&copy=geometry.view(2).vertices;
    equal_geometry(old,copy);for(unsigned i=0;i<copy.size();++i)
        require(old[i].material==copy[i].material,"shader-absent diagnostic changed the highest-detail material bank");
    geometry.reset_history();auto reference_geometry=geometry;
    for(unsigned frame=0;frame<64;++frame)for(unsigned index=0;index<4;++index) {
        auto v=source_view(index,frame%3==0?1:frame%3==1?2:4,frame%2?480:1080);
        const float metric=std::array{0.f,.01f,.1f,1.f,3.f,10.f,100.f,10000.f}[frame%8];
        v.input=[=](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{metric,0,0};};
        if(frame%7==0)v.shader.reset();
        const auto actual=geometry.select(37,0x12340025,frame%2?near:far,1,v);
#ifdef _WIN32
        _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1");
#else
        setenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1",1);
#endif
        const auto expected=reference_geometry.select(37,0x12340025,frame%2?near:far,1,v);
#ifdef _WIN32
        _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","");
#else
        unsetenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION");
#endif
        require(actual.lod==expected.lod && actual.transition==expected.transition && actual.vertices==expected.vertices &&
            actual.source_distance==expected.source_distance,"run selection changed original view LOD/history fields");
        const auto &a=geometry.view(index),&b=reference_geometry.view(index);
        require(a.ordinary_lighting==b.ordinary_lighting && a.vertices.size()==b.vertices.size(),"run selection changed ordinary draw admission");
        require(a.vertices.empty() || !std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex)),
            "run selection changed full source bytes with changing LOD/history/views");
    }
    // Repeated selections retain the same draw; a later actual pose, shader
    // change or lifetime reset must still replace every affected byte.
    geometry.reset_history();reference_geometry=geometry;
    unsigned retained_checks=0;
    for(unsigned frame=0;frame<48;++frame) {
        if(frame%8==0) {
            auto root=transform;root.position.x+=float(frame)*.125f;
            const auto pose=[&](const auto &m){return pose_model_world(m,root);};
            geometry.pose_world(pose);reference_geometry.pose_world(pose);
        }
        if(frame==23 || frame==39){geometry.reset_history();reference_geometry.reset_history();}
        for(unsigned index=0;index<4;++index) {
            auto v=source_view(index,4,1080);
            if(frame/4%3==0)v.shader.reset();
            const auto center=frame/6%2?near:far;
            const auto actual=geometry.select(37,0x12340025,center,1,v);
#ifdef _WIN32
            _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1");
#else
            setenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","1",1);
#endif
            const auto expected=reference_geometry.select(37,0x12340025,center,1,v);
#ifdef _WIN32
            _putenv_s("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION","");
#else
            unsetenv("HALO2_REFERENCE_VERTEX_OBJECT_SELECTION");
#endif
            const auto &a=geometry.view(index),&b=reference_geometry.view(index);
            require(actual.lod==expected.lod && actual.transition==expected.transition &&
                actual.vertices==expected.vertices && actual.source_distance==expected.source_distance,
                "retained view changed selection/history fields");
            require(a.ordinary_lighting==b.ordinary_lighting && a.vertices.size()==b.vertices.size() &&
                (a.vertices.empty() || !std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex))),
                "retained view changed complete posed source bytes");
            ++retained_checks;
        }
    }
    std::cout<<retained_checks<<" retained source view/pose/lifetime comparisons passed\n";
    geometry.reset_history();for(unsigned i=0;i<4;++i)require(geometry.view(i).vertices.empty() &&
        !geometry.view(i).ordinary_lighting,"source birth reset retained the previous generation's shader draw");
    std::cout<<map<<" definition "<<std::hex<<datum<<std::dec<<": "<<vertices<<" submitted source-part checks, "
        <<lower<<" lower selections, "<<prt<<" PRT vertices; resident "<<resident.union_dynamic_materials
        <<" ordinary + "<<resident.union_prt_materials<<" PRT roots\n";
}
void inventory() {
    const std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
        "foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock",
        "deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
    unsigned total=0,max=0;
    for(unsigned i=0;i<names.size();++i) {
        Cache cache(std::filesystem::path(i<12?"assets/disc/maps":"assets/map-pack/maps")/(std::string(names[i])+".map"));
        const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);Mesh mesh;
        SourceMachineScene machines(cache,content.objects,0,mesh,collision);const auto machine=mesh.materials.size();
        SourcePropScene props(cache,content.objects,0,mesh,collision);const auto prop=mesh.materials.size()-machine;
        require(machine==machines.stats().union_dynamic_materials+machines.stats().union_prt_materials &&
            prop==props.stats().union_dynamic_materials+props.stats().union_prt_materials,
            "resident union statistic was inferred from a submitted highest-detail draw");
        total+=unsigned(mesh.materials.size());max=std::max(max,unsigned(mesh.materials.size()));
        std::cout<<names[i]<<' '<<machine<<' '<<prop<<' '<<mesh.materials.size()<<'\n';
    }
    std::cout<<"TOTAL "<<total<<" MAX "<<max<<" MACH+BLOC resident roots only; excludes BSP/SCEN/player/FP\n";
}
}
int main(int argc,char **argv){try{
    if(argc>1 && std::string(argv[1])=="--inventory"){inventory();return 0;}
    authored_views("headlong",0xeb4309ceu,true,false);
    authored_views("zanzibar",0xe93107bcu,false,true);
    authored_views("zanzibar",0xebe20a6du,false,true,true);
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
