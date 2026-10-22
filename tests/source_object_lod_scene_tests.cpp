#include "halo2/source_prop_scene.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/scene.hpp"
#include "halo2/math.hpp"
#include "../src/gameplay/objects/source_object_geometry.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <bit>
#include <map>
#include <set>
using namespace halo2;
namespace {
void require(bool value,const char*message){if(!value)throw FormatError(message);}
struct Submission {std::vector<Vertex>vertices;std::vector<ObjectLightingBinding>light;std::vector<SourceObjectViewSelection>selected;};
SourceObjectViewLod view(unsigned index,float metric,unsigned players=1,std::uint32_t flags=0,
    std::uint16_t controls=0,bool special=false){
    return {index,players,special,[=](std::size_t index,std::uint32_t handle,Vec3 center,float radius){
        require(index==0 && handle!=null_datum,"view lost source placement/full generation handle");
        require(std::isfinite(center.x) && radius>0,"view substituted an invalid sphere");
        return SourceObjectLodInput{metric,flags,controls};}};
}
LevelContent one_object(const LevelContent&content,const ScenarioObject&object){
    LevelContent result;result.objects={object};result.object_names=content.object_names;return result;
}
void prop_views(){
    Cache cache("assets/disc/maps/headlong.map");const auto content=cache.load_content(0,true);
    const auto source=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){return o.definition==0xeb4309ceu;});
    require(source!=content.objects.end(),"original Headlong bus fixture missing");
    auto one=one_object(content,*source);CollisionWorld collision(content.collision);Mesh mesh;
    SourcePropScene scene(cache,one.objects,0,mesh,collision);SceneObjects objects(one,0);scene.synchronize(objects);
    SourcePropInitialState initial;unsigned sphere_reads=0;
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform&node,float scale){
        require(index==0,"prop query escaped full source content identity");++sphere_reads;
        if(!initial.contains(handle))initial.capture(handle,source->definition,cache.load_object_shadow_definition(source->definition),node,scale);
        const auto state=initial.retained(handle,source->definition,node,scale,false);
        return SourcePropQueryState{{state.sphere.center,state.sphere.radius},state.velocity,state.query_runtime_flags,state.frozen};
    });
    const auto submit=[&](const SourceObjectViewLod&input){Submission r;scene.append_view(r.vertices,r.light,input,&r.selected);
        require(r.selected.size()==1 && r.light.size()==1,"selected bus lost its source owner light binding");return r;};
    const auto expected=cache.load_initial_model_selection(source->definition,source->variant);
    const auto size=[&](unsigned lod){return cache.load_model(source->definition,false,source->variant,expected,lod).mesh.vertices.size();};
    //The independent original4C2B0 fixture retains bus sections0/0/0/1/2/2.
    //Its literal cutoff bank is100/90/0/30/20/10/5/0/0. These distances
    //therefore exercise different authored streams, not a generated quad.
    scene.update_pose(0,mesh);require(sphere_reads==0,"pose-only prop update performed an unsubmitted light query");
    const auto a=submit(view(0,7)),b=submit(view(1,100));
    require(a.selected[0].lod==3 && b.selected[0].lod==0,"bus first-view source detail differs");
    require(a.vertices.size()==size(3) && b.vertices.size()==size(0) && a.vertices.size()!=b.vertices.size(),
        "two player views reused the same bus geometry stream");
    require(sphere_reads==1 && a.light[0].state.source_values()==b.light[0].state.source_values(),
        "later player view changed the first-submission prop light bank");
    scene.update_pose(1,mesh);const auto c=submit(view(0,18)),d=submit(view(1,18));
    require(c.selected[0].lod==3 && d.selected[0].lod==2,"per-player original LOD hysteresis leaked between views");
    require(sphere_reads==2 && c.light[0].state.source_values()==d.light[0].state.source_values(),
        "same render frame repeated the owner query producer");
    const auto capped=submit(view(2,0,4));require(capped.selected[0].lod==2,"four-player source ordinary cap ignored");
    auto pc_low=view(2,0);pc_low.detail_policy=native_detail_policy(2);
    const auto low=submit(pc_low);
    require(low.selected[0].lod==3 && low.vertices.size()==size(3),"Vista Low did not select the real capped bus geometry bank");
    const auto forced=submit(view(2,0,4,0x80000));require(forced.selected[0].lod==5,"source flag19 did not retainLOD5");
    const auto controlled=submit(view(3,25,1,0,4));require(controlled.selected[0].lod==0,"source+10A bit2 distance modifier ignored");
    const auto special=submit(view(3,25,1,0,4,true));
    require(special.selected[0].lod==2 && special.selected[0].source_distance==25,
        "geometry modifiers changed the unmodified lighting-distance input");
    std::vector<Vertex> legacy;std::vector<ObjectLightingBinding> banks;scene.append(legacy,banks);
    require(legacy.size()==size(5),"diagnosticLOD5 output was altered by per-view selection");
    bool rejected=false;try{submit(view(4,0));}catch(const FormatError&){rejected=true;}
    require(rejected,"invalid source player history bank admitted");
    objects.visible(objects.objects().front().handle,false);scene.synchronize(objects);scene.update_pose(2,mesh);
    Submission hidden;scene.append_view(hidden.vertices,hidden.light,view(0,0),&hidden.selected);
    require(hidden.vertices.empty() && hidden.selected.empty() && hidden.light.empty() && sphere_reads==2,
        "hidden source owner advanced view selection or queried lighting");
    std::cout<<"Headlong bus: realLOD0/2/3/5 streams, independent two-view hysteresis, caps/modifiers and first-view lighting\n";
}
void machine_views(){
    Cache cache("assets/map-pack/maps/gemini.map");const auto content=cache.load_content(0,true);
    const auto source=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){return o.type==7 && o.placement==1;});
    require(source!=content.objects.end(),"original Gemini high door fixture missing");
    auto one=one_object(content,*source);CollisionWorld collision(content.collision);Mesh mesh;
    SourceMachineScene scene(cache,one.objects,0,mesh,collision);SceneObjects objects(one,0);scene.synchronize(objects);scene.update_pose(0,0,mesh);
    Submission a,b;scene.append_view(a.vertices,a.light,view(0,0),&a.selected);scene.append_view(b.vertices,b.light,view(1,100),&b.selected);
    require(a.selected.size()==1 && b.selected.size()==1 && a.selected[0].lod==4 && b.selected[0].lod==0,
        "source machine view cap/distance changed");
    require(a.vertices.size()!=b.vertices.size(),"Gemini high door reusedLOD5 geometry for the far view");
    require(a.light.size()==1 && b.light.size()==1 && a.light[0].state.source_values()==b.light[0].state.source_values(),
        "later machine view changed its first-submitting light state");
    std::cout<<"Gemini high door: distinct original region streams and one retained owner light bank\n";
}
void prt_section_binding(){
    Cache cache("assets/disc/maps/zanzibar.map");const auto content=cache.load_content(0,true);
    const auto source=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){return o.definition==0xe93107bcu;});
    require(source!=content.objects.end(),"original crane source fixture missing");
    std::ifstream fixture("tests/fixtures/retail_prt_object_banks.tsv");std::uint32_t datum;
    std::vector<Transform>world_nodes;std::map<unsigned,std::vector<std::uint32_t>>expected;
    while(fixture>>datum){unsigned lod,count,nodes;fixture>>lod>>count;std::vector<unsigned>sections(count);
        for(auto&v:sections)fixture>>v;fixture>>nodes;std::vector<Transform>bank(nodes);
        for(auto&t:bank){std::array<float,13>v;for(auto&f:v){std::uint32_t bits;fixture>>std::hex>>bits>>std::dec;f=std::bit_cast<float>(bits);}
            t={v[0],{{{v[1],v[2],v[3]},{v[4],v[5],v[6]},{v[7],v[8],v[9]}}},{v[10],v[11],v[12]}};}
        std::map<unsigned,std::vector<std::uint32_t>>words;
        for(unsigned section:sections){unsigned id,size;fixture>>id>>size;require(id==section,"source PRT fixture section order changed");
            words[id].resize(size);for(auto&v:words[id])fixture>>std::hex>>v>>std::dec;}
        require(bool(fixture),"source PRT retained bank fixture truncated");
        if(datum==0xe93307beu){require(lod==0 && sections==std::vector<unsigned>({0,1,2}),"retained original crane bank identity changed");
            world_nodes=std::move(bank);expected=std::move(words);}
    }
    require(expected.size()==3,"source original crane section/palette fixture missing");
    const auto initial=cache.load_initial_model_definition(source->definition);
    const auto selection=cache.load_initial_model_selection(source->definition,source->variant);
    const auto prt=*cache.load_source_prt_definition(initial.render_model);
    require(prt.lods.size()==1,"lower crane bank equivalence requires the literal one-LODInfo source record");
    SourceObjectGeometry geometry;geometry.load(cache,*source,initial,selection,initial.render_model);
    const auto basis=authored_basis(source->rotation);
    const auto object=source_object_transform(source->position,basis.forward,basis.up,source->scale,false);
    geometry.pose_world([&](const RenderModel&m){return pose_model_world(m,object);});Mesh mesh;
    geometry.prepare(cache,cache.load_object_colors(source->definition,source->variant),world_nodes,cache.load_source_prt_lighting(),mesh);
    const auto sh=cache.load_source_prt_lighting();
    unsigned banks=0,components=0;
    for(unsigned lod=0;lod<6;++lod){const auto&g=geometry.at(lod);
        require(g.sections==std::vector<unsigned>({0,1,2}),"lower source crane collector differs from retained palette order");
        const auto owner=geometry.owner_banks(cache,world_nodes,sh,lod);
        auto moved=world_nodes;for(auto &node:moved)node.position.x+=10.f;
        const auto another=geometry.owner_banks(cache,moved,sh,lod);
        require(owner.size()==another.size() && !owner.empty(),"independent PRT owner lost selected section banks");
        std::set<std::pair<unsigned,unsigned>>used;
        for(unsigned i=0;i<g.vertices.size();++i){const auto material=g.vertices[i].material;const auto&m=mesh.materials.at(material);
            if(m.environment_pass && m.environment_pass->forward_vertex==9)used.emplace(g.model.vertex_origins[i].section,material);}
        require(used.size()==3,"resident crane LOD lost a PRT section/material identity");
        for(const auto[section,material]:used){
            const auto binding=std::find_if(owner.begin(),owner.end(),[&](const auto &b){return b.material==material;});
            require(binding!=owner.end(),"owner PRT draw lost source material identity");
            const auto&bank=*binding->bank;
            require(bank.constants==mesh.materials.at(material).environment_pass->vertex_bank->constants,
                "another owner modified the source resident or retained PRT coefficients");
            const auto&words=expected.at(section);require(bank.first_constant==68 && bank.constants.size()*4==words.size(),"LOD coefficient bank range differs");
            for(unsigned i=0;i<words.size();++i){if(i==3 || (i>=8 && (i-8)%16==3))continue;
                require(std::bit_cast<std::uint32_t>(bank.constants[i/4][i%4])==words[i],"resident LOD bound another section's original PCA bank");++components;}
            ++banks;
        }
    }
    std::cout<<"Crane resident bank routing: "<<banks<<" section/material/LOD bindings / "<<components
        <<" original retained words; one sourceLODInfo and fixed source section/palette order\n";
}
void empty_collectors(){
    Cache cache("assets/disc/maps/headlong.map");const auto content=cache.load_content(0,true);
    const auto source=std::find_if(content.objects.begin(),content.objects.end(),[](const auto&o){return o.definition==0xeb4309ceu;});
    auto selected=cache.load_initial_model_selection(source->definition,source->variant);
    std::fill(selected.permutations.begin(),selected.permutations.end(),255);
    const auto definition=cache.load_initial_model_definition(source->definition);SourceObjectGeometry geometry;
    geometry.load(cache,*source,definition,selected,definition.render_model);
    for(unsigned lod=0;lod<6;++lod)require(geometry.at(lod).model.mesh.vertices.empty() && geometry.at(lod).sections.empty(),
        "empty original collector acquired another resident LOD");
    const auto first=geometry.select(0,0x10000,{},1,view(0,7));
    const auto second=geometry.select(0,0x10000,{},1,view(0,18));
    require(first.lod==3 && second.lod==2 && first.vertices==0 && second.vertices==0,
        "empty collector changed source NONE history or acquired geometry");
    std::cout<<"Empty source collectors: all six stay empty and do not acquire prior player LOD\n";
}
}
int main(){try{prop_views();machine_views();prt_section_binding();empty_collectors();return 0;}
catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
