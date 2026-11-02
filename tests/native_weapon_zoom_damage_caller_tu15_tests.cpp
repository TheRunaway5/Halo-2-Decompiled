#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
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
    Cache cache(argv[1],argv[2]);need(cache.is_mcc(),"Damage zoom caller requires prepared MCC");
    const auto content=cache.load_content(0,true);unsigned selected{};std::uint32_t definition=null_datum;
    for(unsigned option=3;option<=20 && !selected;++option) {
        const auto candidate=cache.slayer_starting_weapon(option);
        if(candidate!=null_datum && content.weapons.at(candidate).zoom.levels>0 && content.weapons.at(candidate).damage!=null_datum)
            {selected=option;definition=candidate;}
    }
    need(selected,"No actual scoped weapon damage definition");nlohmann::json rows=nlohmann::json::array();
    for(bool remote:{false,true}) {
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=2;
        options.starting_weapon=selected;options.mute=true;options.source_object_world=true;options.session_random_seed=0xcc590;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,int(selected)}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/(remote?"damage-remote":"damage-local");
        MultiplayerPreparedWorld world(options);auto &native=world.source_objects->tu15_world();
        const auto host=world.local_slots.at(0),target=world.local_slots.at(1);
        const std::array local{host};if(remote)world.bind_source_local_views(local);else world.bind_source_local_views(world.local_slots);
        std::array<Controls,multiplayer_capacity> controls{};for(unsigned tick=0;tick<120;++tick)world.match.step(controls);
        auto state=world.match.state();const auto unit=state.players[target].source_unit;
        controls[target].zoom=0;controls[target].zoom_reset_serial=state.players[target].zoom_reset_serial;
        world.match.step(controls);state=world.match.state();need(state.players[target].source_zoom.current==0,"Actual damage target did not zoom");
        const auto word=[&](auto at){return Bytes(world.source_objects->memory().view(at,4)).u32(0);};
        const auto body=world.source_objects->source_object_body(unit);
        need(world.source_objects->object_definition(unit)==world.content.player.datum,"Actual target vitality definition differs");
        const auto tags=word(0x4f0414u),data=word(tags+(definition&65535u)*16u+8u),out_sound=word(data+0x278u);
        need(out_sound!=null_datum,"Actual scoped weapon has no authored zoom-out sound");
        const auto serial=state.weapon_zoom_audio.empty()?0:state.weapon_zoom_audio.back().serial;
        std::vector<std::array<std::byte,160>> requests;unsigned caller_pushes{},reset_stores{};
        struct Observers {PhysicsLiveWorldTu15 &world;~Observers(){world.observe_physics_stores_read_only({});world.observe_sound_requests_with_context({});}} observers{native};
        native.observe_sound_requests_with_context([&](auto sound,std::span<const std::byte,160> bytes,auto zoom_unit) {
            if(zoom_unit!=unit)return;need(sound==out_sound,"Damage reset emitted a replacement zoom sound");
            std::array<std::byte,160> copy;std::copy(bytes.begin(),bytes.end(),copy.begin());requests.push_back(copy);
        });
        native.observe_physics_stores_read_only([&](auto,const auto &store) {
            if(store.instruction==0xcc95du) {
                need(store.written.size()==4 && Bytes(store.written).u32(0)==0xcc962u && word(store.address+4u)==0,
                    "Damage reset lost its actual CALL/argument boundary");++caller_pushes;
            }
            if((store.instruction==0xcac80u && store.address==body+0x240u) ||
                (store.instruction==0xcac87u && store.address==body+0x241u) ||
                (store.instruction==0xcac8eu && store.address==body+0x26cu))++reset_stores;
        });
        // Compute the upstream native damage result from the actual cache and
        // vitality. The explicit nonlethal amount is the test stimulus. The
        // complete request/report/reaction owners receive no lower outcomes.
        const auto before=world.source_objects->object_vitality(unit);RechargeState vitality{};
        vitality.body=before.body;vitality.shield=before.shield;const auto &weapon=world.content.weapons.at(definition);
        const auto result=apply_unit_damage(vitality,world.content.player.vitality,weapon,
            world.content.damage_groups,world.content.damage_materials,.125f);
        need((result.body>0 || result.shield>0) && !(result.report_flags&1u),"Native damage stimulus was not admitted nonlethally");
        const DamageOwner owner{state.players[host].source_player,state.players[host].source_unit};
        world.source_objects->deliver_damage_reaction(unit,weapon.damage,owner,result,
            {DamageImpactContext::Kind::Direct,{1,0,0},state.players[target].position,1.f,weapon.projectile});
        // The delivery adapter prepares the original report and physical
        // response. CompleteCC590 is the distinct unit-consequence owner
        // that consumes that report and calls CABC0. Exercise it explicitly
        // on the real completed packet; do not manufacture a reset event.
        constexpr std::uint32_t report_address=0x60041000u;
        need(std::bit_cast<float>(word(report_address+0x44u))>0 || std::bit_cast<float>(word(report_address+0x48u))>0,
            "Original damage owners did not prepare a positive unit report");
        const std::array<std::uint32_t,2> arguments{unit,report_address};auto response=native.prepare_source_call(arguments);const auto response_sp=response.gp[4];
        native.run_actual(0xcc590u,response);need(response.gp[4]==response_sp+12u,"Unit damage response lost its original RET8 boundary");
        need(caller_pushes==1 && reset_stores==3,"Damage reaction did not reach its actual scope-reset call");
        need(std::to_integer<unsigned>(world.source_objects->memory().view(body+0x240u,1)[0])==255,"Actual damage did not reset scope");
        state=world.match.state();std::vector<NativeWeaponZoomAudioEventTu15> resets;
        for(const auto &event:state.weapon_zoom_audio)if(event.serial>serial && event.unit==unit && event.forced_reset)resets.push_back(event);
        if(resets.size()!=1) {
            const nlohmann::json failed{{"passed",false},{"recipient_local",!remote},{"call_site",0xcc95du},
                {"caller_pushes",caller_pushes},{"reset_stores",reset_stores},{"journal_resets",resets.size()}};
            std::ofstream(argv[3])<<failed.dump(2)<<'\n';std::cerr<<failed.dump()<<'\n';
        }
        need(resets.size()==1,"Actual damage reset bypassed or duplicated its zoom journal");const auto event=resets.front();
        need(event.player==state.players[target].source_player && event.sound==out_sound && event.control_bits==0x3f800000u && event.tick,
            "Actual damage reset lost its full player/sound/control/tick identity");
        need(requests.size()==(remote?0u:1u),"Damage reset lost its original producer-local sound gate");
        double energy{};
        if(!remote) {
            MultiplayerAudio actual(world.cache,0x715541,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
            MultiplayerAudio recipient(world.cache,0x715541,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
            actual.bind_native_sound_metadata(native.sound_variation_provider(*world.cache));
            recipient.bind_native_loop_metadata(MccSoundMetadataPublication::create(world.cache));
            const std::array<Vec3,2> listeners{state.players[host].position,state.players[target].position};actual.listeners(listeners);recipient.listeners(listeners);
            need(actual.source_impulse(out_sound,requests.front(),MultiplayerAudioKind::FirstPerson,1).has_value(),"Actual damage-reset request has no PCM voice");
            std::vector<NativeWeaponZoomAudioPendingTu15> pending{{event,1}};need(deliver_native_weapon_zoom_audio_tu15(recipient,pending)==1,"Damage-reset recipient has no PCM voice");
            for(unsigned block=0;block<8;++block) {
                const auto a=actual.render(4096),b=recipient.render(4096);need(a.size()==b.size(),"Damage-reset PCM extents differ");
                for(unsigned i=0;i<a.size();++i){need(std::isfinite(a[i]) && std::bit_cast<std::uint32_t>(a[i])==std::bit_cast<std::uint32_t>(b[i]),"Recipient PCM differs from actual damage-reset request");energy+=double(a[i])*a[i];}
            }
            need(energy>0,"Actual damage-reset PCM was silent");
        }
        rows.push_back({{"recipient_local",!remote},{"unit",unit},{"definition",definition},{"call_site",0xcc95du},
            {"caller_pushes",caller_pushes},{"reset_stores",reset_stores},{"journal_resets",resets.size()},
            {"reset_player",event.player},{"reset_sound",event.sound},{"reset_tick",event.tick},
            {"source_zoom_requests",requests.size()},{"native_damage_body",result.body},{"native_damage_shield",result.shield},{"local_PCM_energy",energy}});
    }
    const nlohmann::json report{{"passed",true},{"rows",rows},{"actual_damage_request_report_reaction_owners",true},{"actual_complete_CC590_unit_response",true},
        {"upstream_native_damage_result_computed",true},{"explicit_nonlethal_damage_amount",.125f},
        {"supplied_lower_outcomes",false},{"damage_hit_admission_or_joined_transport_proven",false},
        {"local_actual_request_and_recipient_PCM_equal",true},{"Xbox_waveform_equality",false},{"retained_large_artifacts",nlohmann::json::array()}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
