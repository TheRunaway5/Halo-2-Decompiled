#include "halo2/source_grenade_scene.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "../src/gameplay/objects/source_object_geometry.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool b,const char*m){if(!b)throw FormatError(m);}
float scalar(std::uint32_t u){return std::bit_cast<float>(u);}
template<class F>void rejected(F f,const char*m){bool bad=false;try{f();}catch(const FormatError&){bad=true;}check(bad,m);}
SourceObjectViewLod view(unsigned n,unsigned players){
    SourceObjectViewLod v;v.view_index=n;v.local_players=players;
    v.input=[n](std::size_t index,std::uint32_t owner,Vec3,float){
        check(index==(owner&65535u),"grenade LOD producer lost full native owner index");
        return SourceObjectLodInput{index%2?100.f:float(n)*4.f,0,0};};
    v.shader=SourceModelShaderView{{{0,0,-1,1000},240},source_shader_lod_view(0,480,false,false),
        source_geometry_lod_policy(players).coverage_multiplier,source_shader_lod_player_multiplier(players),false};return v;
}
void poses(const Cache&c,const char*path){
    std::ifstream input(path);check(bool(input),"missing independent original grenade pose fixtures");
    unsigned id,image,datum,rows=0;
    while(input>>std::hex>>id>>image>>datum){
        std::array<std::uint32_t,9> values;for(auto&w:values)input>>w;
        std::uint32_t scale;input>>scale;
        std::array<std::uint32_t,13> node;for(auto&w:node)input>>w;
        std::array<std::uint32_t,4> sphere;for(auto&w:sphere)input>>w;
        unsigned variant,returned;std::array<unsigned,10> region;input>>variant;for(auto&w:region)input>>w;input>>returned;
        check(bool(input),"truncated original grenade pose fixture");
        const auto point=[&](unsigned offset){return Vec3{scalar(values[offset]),scalar(values[offset+1]),scalar(values[offset+2])};};
        const auto selection=c.load_initial_model_selection(datum);
        check(selection.variant_index==-1 && variant==255 && selection.permutations==std::vector<std::uint8_t>{0},
              "native grenade variant/permutation differs from original BB430/BB480");
        check(region==std::array<unsigned,10>{0,0,255,0,0,0,255,255,255,255},"original grenade region lifecycle changed");
        const auto model=c.load_model(datum,false,0,selection);
        const auto root=source_object_transform(point(0),point(3),point(6),scalar(scale),false);
        const auto nodes=pose_model_nodes_world(model,root);
        check(nodes.size()==1 && std::bit_cast<std::array<std::uint32_t,13>>(nodes[0])==node,
              "native grenade default WORLD node differs from original BDF80");
        const auto actual=source_shadow_sphere(c.load_object_shadow_definition(datum),nodes[0],scalar(scale));
        check(std::bit_cast<std::array<std::uint32_t,4>>(actual)==sphere,
              "native regular grenade sphere differs from original BEDE0 arithmetic");
        ++rows;
    }
    check(input.eof() && rows==8,"original grenade fixture count changed");
    std::cout<<"8 complete original variant/default-pose and regular-sphere controls match all words\n";
}
size_t pixels(const Material&m){
    size_t n=m.diffuse.pixels.size()+m.detail.pixels.size()+m.bump.pixels.size()+m.auxiliary.pixels.size();
    for(const auto&frames:m.animated_textures)for(const auto&t:frames)n+=t.pixels.size();
    if(m.environment_pass)n+=pixels(*m.environment_pass);
    for(const auto&p:m.world_overlays_before)n+=pixels(*p);for(const auto&p:m.world_overlays_after)n+=pixels(*p);
    return n;
}
void scene(const Cache&cache,std::uint32_t one){
    // Scene consumes the authored BSP collision bank. load_content moves this
    // exact bank into the broader gameplay record; no gameplay fields are
    // needed to validate rendering/lighting ownership here.
    const auto content=cache.load_bsp_content(0);CollisionWorld collision(content.collision);Mesh mesh;
    // The actual native multiplayer allocator is2048 slots. Geometry
    // templates remain two banks regardless of active/allocator capacity.
    SourceGrenadeScene scene(cache,0,mesh,collision,0x800,one);
    check(scene.banks().size()==2 && mesh.materials.size()==8,"grenade authored residency bank changed");
    std::vector<NativeGrenadeProjectile> objects;
    for(unsigned i=0;i<4;++i){NativeGrenadeProjectile p;p.datum=0x12340002+i;p.type=std::int16_t(i/2);
        p.definition=scene.banks()[unsigned(p.type)].definition;p.position={float(i),0,3};
        p.forward={1,0,0};p.up={0,0,1};p.parent=0xffffffffu;objects.push_back(p);}
    struct Reference {SourceObjectGeometry geometry;Mesh mesh;ShadowCasterSphere sphere;};
    std::vector<Reference> references;
    for(const auto&p:objects){
        Reference reference;
        const auto initial=cache.load_initial_model_definition(p.definition);
        const auto selection=cache.load_initial_model_selection(p.definition);
        ScenarioObject request;request.type=5;request.definition=p.definition;
        reference.geometry.load(cache,request,initial,selection,initial.render_model);
        const auto root=source_object_transform(p.position,p.forward,p.up,1,false);
        const auto nodes=pose_model_nodes_world(reference.geometry.model(),root);
        reference.sphere=source_shadow_sphere(cache.load_object_shadow_definition(p.definition),nodes[0],1);
        reference.geometry.pose_world([&](const RenderModel&m){return pose_model_world(m,root);});
        reference.geometry.prepare(cache,source_object_initial_colors(cache.load_source_object_color_definition(p.definition),{},0),
            nodes,cache.load_source_prt_lighting(0),reference.mesh);
        references.push_back(std::move(reference));
    }
    scene.synchronize(objects);unsigned submitted=0;
    for(unsigned players:{1u,4u})for(unsigned index=0;index<players;++index){
        auto v=view(index,players);const auto saved=scene.draw_view(31,v);
        check(saved.size()==4,"grenade scene omitted a live owner");
        const auto again=scene.draw_view(31,v);check(again.size()==saved.size(),"same-frame grenade draw count changed");
        for(unsigned i=0;i<saved.size();++i){const auto&a=saved[i],&b=again[i];
            check(a.owner==objects[i].datum && a.definition==objects[i].definition && a.type==unsigned(objects[i].type),
                "grenade draw lost actual source identity/type");
            check(a.vertices.size()==b.vertices.size() && !a.vertices.empty() &&
                !std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex)),
                "same-frame grenade selection/pose is not byte-stable");
            check(a.first_material==scene.banks()[a.type].first_material &&
                a.material_count==scene.banks()[a.type].material_count,"grenade draw lost resident material identity");
            for(const auto&vertex:a.vertices)check(vertex.material>=a.first_material &&
                vertex.material-a.first_material<a.material_count,"grenade selected an unresident material");
            // Shared geometry scratch must not retain another owner's origin.
            if(i%2==1 && saved[i-1].selection.lod==a.selection.lod){
                check(a.vertices.front().position.x!=saved[i-1].vertices.front().position.x,
                    "shared grenade template overwrote a previous owner's posed draw");}
            auto&ref=references[i];
            const auto selected=ref.geometry.select(objects[i].datum&65535u,objects[i].datum,
                ref.sphere.center,ref.sphere.radius,v);
            auto expected=ref.geometry.view(index).vertices;
            check(selected.lod==a.selection.lod && selected.transition==a.selection.transition &&
                selected.source_distance==a.selection.source_distance && selected.vertices==a.selection.vertices,
                "shared owner LOD history differs from original independent geometry owners");
            for(auto&vertex:expected)vertex.material=vertex.material-ref.geometry.first_material+a.first_material;
            check(expected.size()==a.vertices.size() && !std::memcmp(expected.data(),a.vertices.data(),expected.size()*sizeof(Vertex)),
                "shared source grenade draws changed exact independent geometry/shader output");
            submitted+=unsigned(a.vertices.size());
        }
    }
    auto duplicate=objects;duplicate.push_back(objects[0]);
    rejected([&]{scene.synchronize(duplicate);},"duplicate grenade full owner accepted");
    auto held=objects;held[0].parent=0xe26f0000u;
    rejected([&]{scene.synchronize(held);},"unverified held grenade scene branch accepted");
    auto duplicate_slot=objects;duplicate_slot[1].datum=duplicate_slot[0].datum+0x10000u;
    rejected([&]{scene.synchronize(duplicate_slot);},"two live salts sharing one native slot accepted");
    auto changed=objects;changed[0].definition=objects[2].definition;
    rejected([&]{scene.synchronize(changed);},"grenade definition/type mismatch accepted");
    auto invalid=objects;invalid[0].datum=0xffffffffu;
    rejected([&]{scene.synchronize(invalid);},"grenade NONE owner accepted");
    invalid=objects;invalid[0].datum=0x12340800;
    rejected([&]{scene.synchronize(invalid);},"grenade index outside actual allocator accepted");
    scene.synchronize({});check(scene.draw_view(32,view(0,1)).empty(),"destroyed grenade retained view output");
    objects[0].datum+=0x10000u;scene.synchronize(std::span(objects).first(1));
    check(scene.draw_view(33,view(0,1)).front().owner==objects[0].datum,"new salted generation retained old identity");
    size_t bytes=0;for(const auto&m:mesh.materials)bytes+=pixels(m);
    std::cout<<cache.name()<<": 2shared banks/"<<mesh.materials.size()<<" materials/"<<bytes
        <<" texture bytes, "<<submitted<<" selected vertices/20 owner-view comparisons; native2048 capacity\n";
}
}
int main(int argc,char**argv){try{
    check(argc==5,"Supply MAP SHARED_MAPS UPDATE_XBE ORIGINAL_FIXTURES");
    auto original=OriginalData::load_xbe(argv[3]);check(bool(original->object_construction_operands),"missing imported constructor one");
    Cache cache(argv[1],argv[2],original);
    if(cache.name()=="lockout")poses(cache,argv[4]);
    scene(cache,original->object_construction_operands->one_bits);
    std::cout<<"LIMIT native current pose/velocity/visibility inputs; no source world registration or released angular physics claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
