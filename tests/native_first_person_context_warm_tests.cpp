#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/source_first_person_packet_tu15.hpp"
#include "halo2/source_first_person_pose_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <x86intrin.h>
using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,4> bytes;memory.read(at,bytes);return Bytes(bytes).u32(0);
}
struct Difference {
    std::size_t nodes{};float position{},basis{},scale{};
    void compare(const std::vector<SourceFirstPersonModelTu15> &actual,
        const std::vector<SourceFirstPersonModelTu15> &expected) {
        need(actual.size()==expected.size(),"Hot-cache native FP model count differs");
        for(unsigned model=0;model<actual.size();++model) {
            const auto &a=actual[model],&b=expected[model];
            need(a.owner==b.owner && a.render_model==b.render_model && a.mode==b.mode && a.nodes.size()==b.nodes.size(),
                "Hot-cache native FP model identity/order differs");
            for(unsigned node=0;node<a.nodes.size();++node) {
                const auto &x=a.nodes[node],&y=b.nodes[node];++nodes;
                position=std::max(position,length(x.position-y.position));scale=std::max(scale,std::abs(x.scale-y.scale));
                for(unsigned axis=0;axis<3;++axis)basis=std::max(basis,length(x.basis[axis]-y.basis[axis]));
            }
        }
        need(position<=.0001f && basis<=.0001f && scale<=.0001f,"Hot-cache native FP matrices differ");
    }
};
nlohmann::json run(unsigned representation,const char *map,const char *shared,const std::filesystem::path &scratch) {
    nlohmann::json result{{"representation",representation},{"passed",false},{"frames",0}};
    try {
        MultiplayerOptions options;options.map=map;options.shared_maps=shared;options.local_players=1;
        options.mute=true;options.source_object_world=true;options.starting_weapon=5;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
        options.source_object_savegame_directory=scratch/std::to_string(representation);
        if(representation==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
        MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
        auto &native=world.source_objects->tu15_world();const auto &memory=native.memory();const auto slot=world.local_slots.at(0);
        const auto primary=world.cache->slayer_starting_weapon(5);
        const auto other=std::find_if(world.content.weapons.begin(),world.content.weapons.end(),
            [](const auto &weapon){return weapon.second.name.ends_with("plasma_pistol");});
        need(other!=world.content.weapons.end(),"Hot mixed fixture lacks authored plasma pistol");
        const auto secondary=other->first;const NetgameEquipment *pickup{};
        for(const auto &row:world.content.netgame_equipment)
            if(!row.classification && source_netgame_equipment_selector(row,true,world.rules.source_engine)>=0 &&
                row.items.size()==1 && row.items.front().definition==secondary){pickup=&row;break;}
        need(pickup,"Hot mixed fixture lacks authored plasma-pistol pickup");
        SourcePhysicsWorldRuntimeServices cpu;
        cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
            _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
        cpu.timestamp_counter=[](){return __rdtsc();};
        SourceFirstPersonContextTu15 receiver(*world.cache,{},cpu);
        std::array<Controls,multiplayer_capacity> controls{};unsigned frames{},max_live_effects{},max_live_emitters{};
        Difference difference;
        for(unsigned tick=0;tick<160 && frames<64;++tick) {
            const auto before=world.match.state();const auto &old=before.players[slot];
            if(old.source_unit!=null_datum && !old.dead && old.secondary_weapon.datum==null_datum)
                native.set_object_position(old.source_unit,pickup->position+Vec3{-.2f,0,0});
            controls[slot]={};controls[slot].dual_wield=old.secondary_weapon.datum==null_datum;
            controls[slot].dual_wield_milliseconds=std::uint16_t(std::min(1000u,tick*34u));
            if(frames>=8){controls[slot].yaw=.3f*std::sin(float(frames)*.1f);controls[slot].pitch=.15f;}
            if(frames>=16 && frames<24)controls[slot].forward=.25f;
            if(frames>=24 && frames<40)controls[slot].fire=true;
            if(frames>=40 && frames<56)controls[slot].fire_secondary=true;
            world.match.step(controls);const auto state=world.match.state();const auto &player=state.players[slot];
            if(player.secondary_weapon.datum==null_datum)continue;
            need(player.weapon==primary && player.secondary_weapon.definition==secondary,
                "Genuine mixed pickup changed authored primary/offhand definitions");
            need(player.first_person_pose.has_value(),"Genuine native FP phase did not publish its hot mixed input packet");
            const auto &temporal=player.first_person_pose->temporal;
            need(temporal.identity.player==player.source_player && temporal.identity.unit==player.source_unit &&
                temporal.identity.weapons==std::array{player.source_weapon,player.secondary_weapon.datum} &&
                temporal.identity.weapon_definitions==std::array{primary,secondary} && temporal.source_tick+1u==state.tick,
                "Hot mixed packet differs from its genuine completed native phase");
            // Warm the actual active authority cache through its original model
            // preparation. The receiver intentionally borrows this hot owner,
            // unlike the independent cold asset-owner packet fixture.
            const unsigned camera=frames%4u;const Vec3 eye{1,-2,.5f};
            const Vec3 forward=camera==3?Vec3{.6f,0,-.8f}:camera==2?Vec3{.6f,0,.8f}:camera==1?Vec3{.6f,.8f,0}:Vec3{1,0,0};
            const Vec3 up=camera==3?Vec3{.8f,0,.6f}:camera==2?Vec3{.64f,.6f,-.48f}:Vec3{0,0,1};
            world.source_objects->begin_model_frame();std::vector<SourceFirstPersonModelTu15> expected;
            world.source_objects->render_model_view([&](auto &source){expected=source.prepare_first_person_models(0,eye,forward,up);});
            need(!expected.empty(),"Genuine mixed native168DED returned no models");
            const auto effects=word(memory,0x4f720cu),emitters=word(memory,0x4f7208u);
            max_live_effects=std::max(max_live_effects,word(memory,effects+0x3cu));
            max_live_emitters=std::max(max_live_emitters,word(memory,emitters+0x3cu));
            result["max_live_effects"]=max_live_effects;result["max_live_emitters"]=max_live_emitters;
            const auto caller=native.prepare_source_call();SourceRuntimeWriteSerial serial(memory);
            std::vector<SourceFirstPersonModelTu15> actual;
            receiver.with_sampling_inputs(memory,caller,temporal,[&](const auto &inputs){
                actual=source_sample_first_person_pose_tu15(*world.cache,inputs,caller,0,eye,forward,up,cpu);
            },cpu);
            need(serial.load()==0,"Hot-cache native FP receiver changed borrowed authority bytes");
            difference.compare(actual,expected);++frames;result["frames"]=frames;
        }
        need(frames==64 && max_live_effects>0,"Hot mixed fixture did not exercise an active native effect cache");
        result["nodes_checked"]=difference.nodes;result["max_position_error"]=difference.position;
        result["max_basis_error"]=difference.basis;result["max_scale_error"]=difference.scale;
        result["borrowed_source_writes"]=0;result["passed"]=true;
    } catch(const std::exception &error){result["error"]=error.what();}
    return result;
}
}
int main(int argc,char **argv)try {
    need(argc==4,"MAP SHARED REPORT");const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run through tools/run_test.py");
    nlohmann::json report{{"cases",nlohmann::json::array()}};
    for(const auto representation:{2u,3u})report["cases"].push_back(run(representation,argv[1],argv[2],scratch));
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
    return std::all_of(report["cases"].begin(),report["cases"].end(),[](const auto &row){return row["passed"].template get<bool>();})?0:1;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
