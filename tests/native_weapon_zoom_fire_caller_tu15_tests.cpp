#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw FormatError(why);}
}
int main(int argc,char **argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use owned test storage");
    Cache cache(argv[1],argv[2]);need(cache.is_mcc(),"Zoom fire caller requires prepared MCC");
    const auto definition=cache.slayer_starting_weapon(5);const auto content=cache.load_content(0,true);
    need(content.weapons.at(definition).zoom.levels==0 && content.binocular_zoom && content.binocular_zoom->levels>0,
        "Expected the actual SMG/binocular caller contract");
    nlohmann::json rows=nlohmann::json::array();
    for(bool remote:{false,true}) {
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=2;
        options.starting_weapon=5;options.mute=true;options.source_object_world=true;options.session_random_seed=0x108080;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/(remote?"fire-remote":"fire-local");
        MultiplayerPreparedWorld world(options);auto &native=world.source_objects->tu15_world();
        const auto host=world.local_slots.at(0),target=world.local_slots.at(1);
        const std::array local{host};if(remote)world.bind_source_local_views(local);else world.bind_source_local_views(world.local_slots);
        std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned tick=0;tick<120;++tick)world.match.step(controls);
        auto state=world.match.state();const auto unit=state.players[target].source_unit,weapon=state.players[target].source_weapon;
        need(state.players[target].weapon==definition,"Actual caller weapon differs");
        controls[target].zoom=0;controls[target].zoom_reset_serial=state.players[target].zoom_reset_serial;
        world.match.step(controls);state=world.match.state();
        need(state.players[target].source_zoom.current==0,"Actual binocular zoom was not admitted");
        const auto word=[&](auto at){return Bytes(world.source_objects->memory().view(at,4)).u32(0);};
        const auto body=word(word(word(0x4ecbd0u)+0x44u)+(unit&65535u)*12u+8u);
        need(std::to_integer<unsigned>(world.source_objects->memory().view(body+0x240u,1)[0])==0,"Caller input lacks actual zoom state");
        const auto serial=state.weapon_zoom_audio.empty()?0:state.weapon_zoom_audio.back().serial;
        std::vector<std::uint32_t> zoom_requests;
        struct ObserverLifetime {PhysicsLiveWorldTu15 &world;~ObserverLifetime(){world.observe_sound_requests_with_context({});}} observer_lifetime{native};
        native.observe_sound_requests_with_context([&](auto sound,auto,auto zoom_unit) {
            if(zoom_unit==unit)zoom_requests.push_back(sound);
        });
        unsigned caller_pushes{},reset_stores{};
        // Original108094/1080BA consume (weapon, signed barrel) from the
        // caller stack; complete108080 returns RET8. The native world owns
        // this real stack/TLS/FP frame. No lower-owner result is supplied.
        const std::array<std::uint32_t,2> arguments{weapon,0};auto abi=native.prepare_source_call(arguments);const auto caller_sp=abi.gp[4];
        native.run_actual(0x108080u,abi,[&](const auto &store) {
            if(store.instruction==0x1088e5u) {
                need(store.written.size()==4 && Bytes(store.written).u32(0)==0x1088eau && word(store.address+4u)==1,
                    "Firing reset lost its real CALL/argument boundary");++caller_pushes;
            }
            if((store.instruction==0xcac80u && store.address==body+0x240u) ||
                (store.instruction==0xcac87u && store.address==body+0x241u) ||
                (store.instruction==0xcac8eu && store.address==body+0x26cu))++reset_stores;
        });
        need(abi.gp[4]==caller_sp+12u,"Actual barrel owner lost RET8 ABI");
        need(caller_pushes==1 && reset_stores==3,"Actual barrel owner did not reach its committed scope reset");
        need(std::to_integer<unsigned>(world.source_objects->memory().view(body+0x240u,1)[0])==255,"Actual firing caller did not reset zoom");
        state=world.match.state();std::vector<NativeWeaponZoomAudioEventTu15> resets;
        for(const auto &event:state.weapon_zoom_audio)if(event.serial>serial && event.unit==unit && event.forced_reset)resets.push_back(event);
        if(resets.size()!=1) {
            const nlohmann::json failed{{"passed",false},{"recipient_local",!remote},{"call_site",0x1088e5u},
                {"caller_pushes",caller_pushes},{"reset_stores",reset_stores},{"journal_resets",resets.size()}};
            std::ofstream(argv[3])<<failed.dump(2)<<'\n';std::cerr<<failed.dump()<<'\n';
        }
        need(resets.size()==1,"Actual1088E5 reset bypassed or duplicated the committed zoom journal");
        const auto &event=resets.front();need(event.player==word(body+0x13cu) && event.tick,"Actual firing reset lost full player/tick identity");
        const auto info=word(word(0x4ecc1cu)+0x134u),out_sound=word(info+0xd0u);
        need(event.sound==out_sound,"Actual firing reset replaced its authored binocular sound");
        const auto expected_requests=!remote && out_sound!=null_datum?1u:0u;
        need(zoom_requests.size()==expected_requests,"Actual firing reset emitted a different authored zoom-request count");
        for(auto sound:zoom_requests)need(sound==out_sound,"Actual firing reset emitted a replacement zoom sound");
        rows.push_back({{"recipient_local",!remote},{"unit",unit},{"weapon",weapon},{"call_site",0x1088e5u},
            {"caller_pushes",caller_pushes},{"reset_stores",reset_stores},{"reset_player",event.player},
            {"reset_tick",event.tick},{"reset_sound",event.sound},{"zoom_request_count",zoom_requests.size()},
            {"expected_zoom_request_count",expected_requests},{"authored_NONE_kept_silent",out_sound==null_datum && zoom_requests.empty()}});
    }
    const nlohmann::json report{{"passed",true},{"rows",rows},{"actual_full_108080_owner",true},
        {"prepared_MCC_inputs",true},{"explicit_original_stack_arguments",true},{"supplied_lower_outcomes",false},
        {"physical_Xbox_or_waveform_equality",false},{"retained_large_artifacts",nlohmann::json::array()}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
