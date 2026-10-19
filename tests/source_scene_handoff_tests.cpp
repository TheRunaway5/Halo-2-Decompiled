#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_grenade_scene.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class T>bool same(const T &a,const T &b){
    static_assert(std::is_trivially_copyable_v<T>);return !std::memcmp(&a,&b,sizeof(T));
}
void vertices(const std::vector<Vertex> &a,const std::vector<Vertex> &b){
    check(a.size()==b.size()&&!a.empty(),"Scene handoff changed visible vertex coverage");
    for(unsigned i=0;i<a.size();++i)check(same(a[i],b[i]),"Scene handoff retained old pose/section/material data");
}
template<class T>void model(const T &a,const T &b){
    vertices(a.vertices,b.vertices);
    check(a.definition==b.definition&&a.ordinary_lighting==b.ordinary_lighting&&
        same(a.lighting.source_values(),b.lighting.source_values()),"Scene handoff retained future lighting or definition");
}
SourceObjectViewLod view(unsigned index){
    SourceObjectViewLod v;v.view_index=index;v.local_players=4;
    v.input=[index](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{float(index)*4.f,0,0};};
    v.shader=SourceModelShaderView{{{0,0,-1,1000},240},source_shader_lod_view(0,480,false,false),
        source_geometry_lod_policy(4).coverage_multiplier,source_shader_lod_player_multiplier(4),false};return v;
}
std::uint32_t projectile(const LevelContent &content,const char *suffix){
    for(const auto &[id,w]:content.weapons)if(w.name.ends_with(suffix)){
        check(w.projectile!=0xffffffffu,"Authored weapon has no projectile");return w.projectile;
    }throw FormatError(std::string("Missing authored weapon ")+suffix);
}
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply XBE MAP REPORT");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Scene handoff requires owned scratch");
    const auto original=OriginalData::load_xbe(argv[1]);
    check(original->image==OriginalData::Image::TitleUpdate15&&original->object_construction_operands,
        "Scene handoff requires actual TU1.5 construction operands");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
    const auto content=cache.load_content(0,false);CollisionWorld collision(content.collision);
    // Declared presentation inputs derived from authored definitions/default
    // WORLD nodes. These controls do not claim original native births/physics,
    // live ONLINE adoption or an exact retained lighting checkpoint.
    const auto placement=std::find_if(content.objects.begin(),content.objects.end(),
        [](const auto &o){return o.type==11&&o.definition!=0xffffffffu;});
    check(placement!=content.objects.end(),"Scene handoff lacks an authored crate");
    const auto selection=cache.load_initial_model_selection(placement->definition,placement->variant);
    SourceDynamicObject dynamic;dynamic.object=0x12340011;dynamic.definition=placement->definition;dynamic.type=11;
    dynamic.transform=source_object_transform(placement->position,{1,0,0},{0,0,1},1,false);
    dynamic.visible=dynamic.has_world_nodes=true;dynamic.regions=source_model_region_bank(selection);
    const auto render=cache.load_model(placement->definition,false,placement->variant,selection);
    SourceWorldPose pose;pose.object=dynamic.object;pose.nodes=pose_model_nodes_world(render,dynamic.transform);
    check(!pose.nodes.empty(),"Scene handoff lacks authored crate WORLD nodes");
    dynamic.sphere=pose.sphere=source_shadow_sphere(cache.load_object_shadow_definition(dynamic.definition),pose.nodes.front(),1);
    Mesh used_mesh,fresh_mesh;
    SourceDynamicObjectScene used(cache,0,used_mesh,collision),fresh(cache,0,fresh_mesh,collision);
    const auto dynamic_materials=used_mesh.materials.size();
    auto future=dynamic;auto future_pose=pose;
    for(auto &node:future_pose.nodes)node.position.x+=10;
    future.sphere.center.x+=10;future_pose.sphere=future.sphere;future.transform.position.x+=10;
    used.synchronize(std::span{&future,1},std::span{&future_pose,1});
    std::array<const std::vector<SourceDynamicObjectDraw> *,4> dynamic_views;
    for(unsigned v=0;v<4;++v){dynamic_views[v]=&used.draw_view(300,view(v));check(dynamic_views[v]->size()==1,"Warm dynamic owner is missing");}
    static_assert(noexcept(used.reset_lifetimes()));used.reset_lifetimes();used.reset_lifetimes();
    for(const auto *v:dynamic_views)check(v->empty(),"Retired dynamic owner retained a borrowed draw list");
    bool retired=false;try{(void)used.get(dynamic.object);}catch(const std::out_of_range&){retired=true;}
    check(retired,"Retired dynamic owner retained identity lookup");
    used.synchronize(std::span{&dynamic,1},std::span{&pose,1});fresh.synchronize(std::span{&dynamic,1},std::span{&pose,1});
    unsigned dynamic_vertices{};
    for(unsigned v=0;v<4;++v){const auto &a=used.draw_view(0,view(v)),&b=fresh.draw_view(0,view(v));
        check(a.size()==1&&b.size()==1&&a[0].object==b[0].object&&same(a[0].colors,b[0].colors),"Dynamic handoff changed identity/color");
        model(a[0],b[0]);check(a[0].prt.size()==b[0].prt.size(),"Dynamic handoff changed PRT coverage");
        for(unsigned i=0;i<a[0].prt.size();++i){const auto &x=a[0].prt[i],&y=b[0].prt[i];
            check(x.material==y.material&&x.section==y.section&&x.bank->first_constant==y.bank->first_constant&&
                x.bank->constants.size()==y.bank->constants.size(),"Dynamic handoff changed resident PRT identity");
            for(unsigned j=0;j<x.bank->constants.size();++j)check(same(x.bank->constants[j],y.bank->constants[j]),"Dynamic handoff retained old PRT values");
        }dynamic_vertices+=unsigned(a[0].vertices.size());
    }
    check(used_mesh.materials.size()==dynamic_materials,"Dynamic handoff changed resident materials");

    const std::array definitions{projectile(content,"\\rocket_launcher"),projectile(content,"\\plasma_rifle")};
    Mesh projectile_mesh,reference_mesh;
    NativeWeaponProjectileScene shots(cache,0,projectile_mesh,collision,definitions,2048);
    NativeWeaponProjectileScene reference(cache,0,reference_mesh,collision,definitions,2048);
    const auto projectile_materials=projectile_mesh.materials.size();
    std::array<NativeWeaponProjectile,2> projectiles;
    for(unsigned n=0;n<2;++n){auto &p=projectiles[n];p.datum=0x23450021+n;p.definition=definitions[n];
        p.position=placement->position;p.position.x+=float(n);p.forward={1,0,0};p.up={0,0,1};}
    auto future_projectiles=projectiles;for(auto &p:future_projectiles){p.position.x+=10;p.traveled=25;}
    shots.synchronize(future_projectiles);SourceViewProjectionBank camera;
    std::array<const std::vector<NativeWeaponProjectileDraw> *,4> shot_views;
    std::array<const std::vector<SourceLightVolumeDraw> *,4> glow_views;
    for(unsigned v=0;v<4;++v){shot_views[v]=&shots.draw_view(300,view(v));
        glow_views[v]=&shots.light_volume_view(v,{1,0,0},camera,{0,0,640,480},1024);
        check(shot_views[v]->size()==1&&!glow_views[v]->empty(),"Warm projectile model/glow controls are missing");}
    static_assert(noexcept(shots.reset_lifetimes()));shots.reset_lifetimes();shots.reset_lifetimes();
    for(unsigned v=0;v<4;++v)check(shot_views[v]->empty()&&glow_views[v]->empty(),"Retired projectile retained a borrowed model/glow list");
    shots.synchronize(projectiles);reference.synchronize(projectiles);unsigned projectile_vertices{},glows{};
    for(unsigned v=0;v<4;++v){const auto &a=shots.draw_view(0,view(v)),&b=reference.draw_view(0,view(v));
        check(a.size()==1&&b.size()==1&&a[0].owner==b[0].owner&&same(a[0].colors,b[0].colors),"Projectile handoff changed identity/color");
        model(a[0],b[0]);projectile_vertices+=unsigned(a[0].vertices.size());
        const auto &x=shots.light_volume_view(v,{1,0,0},camera,{0,0,640,480},1024),
            &y=reference.light_volume_view(v,{1,0,0},camera,{0,0,640,480},1024);
        check(x.size()==y.size()&&!x.empty(),"Projectile handoff changed glow coverage");
        for(unsigned i=0;i<x.size();++i){check(x[i].object==y[i].object&&x[i].definition==y[i].definition&&
            x[i].material==y[i].material&&same(x[i].command,y[i].command)&&same(x[i].state,y[i].state),
                "Projectile handoff retained a future glow command");++glows;}
    }
    check(projectile_mesh.materials.size()==projectile_materials,"Projectile handoff changed resident materials");

    Mesh grenade_mesh,grenade_reference_mesh;
    const auto one=original->object_construction_operands->one_bits;
    SourceGrenadeScene grenades(cache,0,grenade_mesh,collision,2048,one),cold(cache,0,grenade_reference_mesh,collision,2048,one);
    const auto grenade_materials=grenade_mesh.materials.size();std::array<NativeGrenadeProjectile,2> thrown;
    for(unsigned n=0;n<2;++n){auto &p=thrown[n];p.datum=0x34560031+n;p.type=std::int16_t(n);
        p.definition=grenades.banks()[n].definition;p.position=placement->position;p.position.x+=float(n);
        p.forward={1,0,0};p.up={0,0,1};}
    auto future_thrown=thrown;for(auto &p:future_thrown)p.position.x+=10;
    grenades.synchronize(future_thrown);std::array<const std::vector<SourceGrenadeDraw> *,4> grenade_views;
    for(unsigned v=0;v<4;++v){grenade_views[v]=&grenades.draw_view(300,view(v));check(grenade_views[v]->size()==2,"Warm grenade owners are missing");}
    static_assert(noexcept(grenades.reset_lifetimes()));grenades.reset_lifetimes();grenades.reset_lifetimes();
    for(const auto *v:grenade_views)check(v->empty(),"Retired grenade retained a borrowed draw list");
    grenades.synchronize(thrown);cold.synchronize(thrown);unsigned grenade_vertices{};
    for(unsigned v=0;v<4;++v){const auto &a=grenades.draw_view(0,view(v)),&b=cold.draw_view(0,view(v));
        check(a.size()==2&&b.size()==2,"Grenade handoff changed owner coverage");
        for(unsigned n=0;n<2;++n){model(a[n],b[n]);check(a[n].owner==b[n].owner&&a[n].type==b[n].type&&
            a[n].first_material==b[n].first_material&&a[n].material_count==b[n].material_count&&
            a[n].selection.source_object==b[n].selection.source_object&&a[n].selection.lod==b[n].selection.lod&&
            a[n].selection.transition==b[n].selection.transition&&a[n].selection.vertices==b[n].selection.vertices&&
            a[n].selection.source_distance==b[n].selection.source_distance,
            "Grenade handoff changed full identity/LOD/material binding");grenade_vertices+=unsigned(a[n].vertices.size());}
    }
    check(grenade_mesh.materials.size()==grenade_materials,"Grenade handoff changed resident materials");
    static_assert(noexcept(std::declval<SourceSceneryScene&>().reset_lifetimes()));
    static_assert(noexcept(std::declval<SourceMachineScene&>().reset_lifetimes()));
    static_assert(noexcept(std::declval<SourcePropScene&>().reset_lifetimes()));
    nlohmann::json report={{"passed",true},{"views_per_owner",4},{"old_frame",300},{"incoming_frame",0},
        {"same_full_handles",true},{"borrowed_draw_lists_retired",true},{"reset_idempotent",true},
        {"fresh_scene_equality",true},{"resident_materials_unchanged",true},
        {"dynamic_definition",dynamic.definition},{"dynamic_vertices",dynamic_vertices},
        {"projectile_definitions",definitions},{"projectile_vertices",projectile_vertices},{"glow_commands",glows},
        {"grenade_vertices",grenade_vertices},{"full_goal_complete",false},
        {"boundary","Declared presentation inputs and authored TU1.5 resources; no native birth, exact retained-light checkpoint, GPU or live ONLINE migration acceptance"}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
