#include "halo2/source_netgame_weapon_scene.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/collision.hpp"
#include "halo2/renderer.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/math.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
constexpr const char*maps[]={"ascension","beavercreek","burial_mounds","coagulation","colossus",
    "cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar",
    "containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
}
int main(){try{
    unsigned rows=0,owners=0,materials=0,vertices=0;bool sword=false,carbine_display=false,carbine_lower=false;
    for(auto name:maps){
        auto path=std::filesystem::path("assets/disc/maps")/(std::string(name)+".map");
        if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(std::string(name)+".map");
        const Cache cache(path);const auto content=cache.load_content(0,true);
        const auto collision=CollisionWorld(content.collision);Mesh mesh;
        SourceNetgameWeaponScene scene(cache,content.netgame_equipment,0,mesh,collision);
        SlayerRules rules;rules.weapon=cache.slayer_starting_weapon(5);
        MultiplayerMatch match(content,0,cache.spawns(),rules);
        const auto participant=match.join("weapon-scene-test");(void)participant;
        match.configure_world_weapon_rows(scene.registrations());
        require(match.world_weapon_objects().empty(),"world weapons born before their actual clock phase");
        bool rejected=false;try{match.configure_world_weapon_rows(scene.registrations());}catch(const FormatError&){rejected=true;}
        require(rejected,"world weapon registration accepted a second lifetime initialization");
        std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned tick=0;tick<31;++tick){
            const auto old=match.world_weapon_objects().size();match.step(controls);
            unsigned births=0;
            for(const auto&record:scene.registrations())if(record.row%30==tick && tick<30)++births;
            require(match.world_weapon_objects().size()==old+births,"native row allocation differs from source15AE70 phase");
            std::set<std::uint32_t> unique;
            for(const auto&owner:match.world_weapon_objects()){
                require(owner.handle!=null_datum && unique.insert(owner.handle).second,"world objects have duplicate/full NONE handles");
                require(owner.created_tick==owner.row%30 && owner.row_bound,"actual native birth clock/row association changed");
                require(owner.handle!=match.unit(participant).source_datum &&
                    owner.handle!=match.unit(participant).weapon.source_datum,"world allocation aliases existing native owners");
                const auto& lifetime=match.simulation().object_lifetimes().owner(owner.handle);
                require(lifetime.published && lifetime.type==2 &&
                    lifetime.definition==scene.registrations()[owner.registration].definition.definition,
                    "source world weapon lacks its authoritative native lifetime publication");
                require(owner.row<match.world_weapon_rows().size() && match.world_weapon_rows()[owner.row].object==owner.handle,
                    "row dropped its actual full native object handle");
                require(source_weapon_turned_on(owner.state.shader).enabled,"cold source placed weapon is unexpectedly off");
                require(source_unattached_weapon_camouflage(owner.state.item)==0.f,
                    "actual unattached world weapon inherited unit camouflage");
            }
        }
        require(match.world_weapon_objects().size()==scene.registrations().size(),"admitted world rows did not spawn exactly once");
        SourceObjectViewLod view;view.local_players=1;view.view_index=0;
        view.input=[](std::size_t,std::uint32_t,Vec3,float){return SourceObjectLodInput{1.f,0,0};};
        SourceModelShaderView shader;
        shader.camera.inverse_view_z={0,0,1,1000};shader.camera.viewport_height_metric=240;
        shader.shader=source_shader_lod_view(0,480,false,false);
        shader.object_coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
        shader.part_shader_multiplier=source_shader_lod_player_multiplier(1);
        view.shader=shader;
        std::vector<Vertex> first,again;std::vector<ObjectLightingBinding> first_bindings,again_bindings;
        scene.append_view(first,first_bindings,match.world_weapon_objects(),4,view);
        scene.append_view(again,again_bindings,match.world_weapon_objects(),4,view);
        require(!scene.registrations().empty() || first.empty(),"empty world row bank emitted geometry");
        require(first.size()==again.size(),"same-frame world row selection retained different geometry");
        for(unsigned i=0;i<first.size();++i){
            require(first[i].material<mesh.materials.size() && first[i].material==again[i].material,
                "world row selected an unresident/stale material slot");
            require(first[i].position.x==again[i].position.x && first[i].position.y==again[i].position.y &&
                first[i].position.z==again[i].position.z,"same-frame world source pose changed");
        }
        for(unsigned material=0;material<mesh.materials.size();++material)
            if(const auto state=scene.shader_state(material,match.world_weapon_objects())){
                auto&pass=mesh.materials[material];
                animate_forward_material(pass,.2,[&](std::uint32_t name){return state->value(name).value_or(0.f);});
                if(std::string(name)=="zanzibar" && pass.source_shader_datum==0xe2da0165u){
                    require(pass.forward_program.words[43]==0xff000000u,"actual blade top turned_on did not reach authored combiner");
                    sword=true;
                }
                if(pass.name.ends_with("\\carbine_display")){
                    require(state->definitions.size()==1 && state->magazines.size()==1 &&
                        state->definitions[0].initial_rounds==36 && state->definitions[0].maximum_loaded==18 &&
                        state->magazines[0].loaded==18 && state->magazines[0].reserve==18,
                        "actual cold carbine magazine initialization differs from FDAD0");
                    require(state->functions.size()==1 &&
                        state->value(state->functions[0].output)==std::optional<float>{1.f},
                        "actual carbine ammo export did not resolve its initialized magazine");
                    auto empty=*state;auto magazines=std::vector(state->magazines.begin(),state->magazines.end());
                    magazines[0].loaded=0;empty.magazines=magazines;
                    auto empty_pass=pass;
                    animate_forward_material(empty_pass,.2,[&](std::uint32_t input){return empty.value(input).value_or(0.f);});
                    require(pass.forward_program.words!=empty_pass.forward_program.words ||
                        pass.program_constants!=empty_pass.program_constants,
                        "actual authored carbine meter does not consume its source magazine input");
                    require(pass.forward_pass && pass.passes.at(pass.forward_pass_index).name==
                        "shaders\\shader_passes\\transparent\\meter_active_camo",
                        "carbine display substituted its authored late pass");
                    unsigned remaps=0;
                    for(const auto &mapping:pass.passes.at(pass.forward_pass_index).pixel_externs)
                        if(mapping.source==37){
                            auto slot=(mapping.destination>>6)&15u;
                            const auto bank=(mapping.destination>>10)&1u;
                            if(slot!=8)slot+=pass.forward_program.fragment_offsets.at(mapping.destination&7);
                            const auto word=slot==8?43+bank:10+bank*8+slot;
                            require(mapping.component==3 && (pass.forward_program.words.at(word)>>24)==0,
                                "actual unattached carbine camouflage did not reach authored alpha");
                            ++remaps;
                        }
                    require(remaps==1,"actual carbine camouflage remap changed");
                    carbine_display=true;
                }
                if(pass.name.ends_with("\\carbine_dull") && pass.selected_shader_lod==1){
                    require(pass.world_overlays_after.size()==1 && pass.world_overlays_after.front()->vertex_declaration &&
                        pass.world_overlays_after.front()->passes.at(pass.world_overlays_after.front()->forward_pass_index).
                        vertex_shader.ends_with("\\stencil_env_super_fast"),
                        "carbine distant environment did not retain its actual authored program");
                    require(pass.world_overlays_after.front()->vertex_declaration->semantics==
                        std::vector<std::uint16_t>({0,4,5,6}),"carbine distant environment declaration changed");
                    carbine_lower=true;
                }
            }
        rows+=scene.stats().resident_rows;owners+=unsigned(match.world_weapon_objects().size());
        materials+=unsigned(mesh.materials.size());vertices+=unsigned(first.size());
        std::cout<<name<<": "<<scene.stats().resident_rows<<" source rows, "<<mesh.materials.size()<<" resident slots, "<<first.size()<<" submitted vertices\n";
    }
    require(sword && carbine_display && carbine_lower && rows==owners,
        "all-map placed weapon ownership/blade/carbine fixture was not exercised");
    std::cout<<rows<<" source rows/owned salted lifetimes, "<<materials<<" resident slots, "<<vertices<<" selected vertices across21maps\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
