#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <SDL.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool ok,const char *why){if(!ok)throw FormatError(why);}
}
// Observe the actual source producer with the same unit local and nonlocal.
// This deliberately makes no joined-client delivery or waveform claim.
int main(int argc,char **argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use owned test storage");
    need(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Could not select private manual audio");
    Cache cache(argv[1],argv[2]);need(cache.is_mcc(),"Recipient test requires prepared MCC");
    const auto content=cache.load_content(0,true);const auto choices=cache.multiplayer_weapon_choices();
    unsigned selected{};std::uint32_t definition=null_datum;
    for(unsigned option=3;option<=20 && !selected;++option) {
        std::uint32_t random{};const auto candidate=source_starting_weapon(null_datum,std::int8_t(option),null_datum,choices,random);
        if(candidate!=null_datum && content.weapons.at(candidate).zoom.levels>0){selected=option;definition=candidate;}
    }
    need(selected,"No authored scoped weapon");nlohmann::json rows=nlohmann::json::array();
    for(bool remote:{false,true}) {
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=2;
        options.starting_weapon=selected;options.mute=true;options.source_object_world=true;
        options.session_random_seed=0x105240;options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,int(selected)}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/(remote?"remote":"local");
        MultiplayerPreparedWorld world(options);auto &native=world.source_objects->tu15_world();
        const auto host=world.local_slots.at(0),target=world.local_slots.at(1);
        const std::array locals{host};if(remote)world.bind_source_local_views(locals);else world.bind_source_local_views(world.local_slots);
        std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned tick=0;tick<120;++tick)world.match.step(controls);
        auto state=world.match.state();const auto unit=state.players[target].source_unit;
        need(state.players[target].weapon==definition,"Recipient starting weapon differs");
        const auto word=[&](auto at){return Bytes(world.source_objects->memory().view(at,4)).u32(0);};
        const auto tags=word(0x4f0414u),data=word(tags+(definition&65535u)*16u+8u);
        const auto in_sound=word(data+0x270u),out_sound=word(data+0x278u);
        need(in_sound!=null_datum && out_sound!=null_datum,"Scoped weapon has no authored sound pair");
        std::vector<nlohmann::json> pending;
        std::vector<std::array<std::byte,160>> actual_requests;
        const auto baseline=state.weapon_zoom_audio.empty()?0:state.weapon_zoom_audio.back().serial;
        struct Lifetime {PhysicsLiveWorldTu15 &world;~Lifetime(){world.observe_sound_requests_with_context({});}} lifetime{native};
        native.observe_sound_requests_with_context([&](auto sound,std::span<const std::byte,160> bytes,auto zoom_unit) {
            if(zoom_unit!=unit)return;const Bytes raw(bytes);
            std::array<std::byte,160> copy;std::copy(bytes.begin(),bytes.end(),copy.begin());actual_requests.push_back(copy);
            pending.push_back({{"sound",sound},{"flags",raw.u32(0)},{"location",std::to_integer<unsigned>(bytes[15])&15u},
                {"control_bits",raw.u32(0x10)},{"gain_bits",raw.u32(0x14)},{"owner",raw.u32(0x50)}});
        });
        controls[target].zoom=0;controls[target].zoom_reset_serial=state.players[target].zoom_reset_serial;
        world.match.step(controls);state=world.match.state();
        need(state.players[target].source_zoom.current==0,"Actual recipient zoom was not admitted");
        for(const auto &event:state.weapon_zoom_audio)if(event.serial>baseline && event.unit==unit)
            need(!event.forced_reset,"Explicit zoom-in acquired an invented forced reset");
        for(const auto &event:pending)need(event.at("sound")==in_sound,"Recipient zoom selected another sound");
        const auto zoom_in=pending;pending.clear();actual_requests.clear();
        bool melee_admitted{},reset_observed{};
        for(unsigned tick=0;tick<36;++tick) {
            controls[target].melee=tick==0;world.match.step(controls);state=world.match.state();
            melee_admitted|=state.players[target].melee_action.active();
            if(state.players[target].source_zoom.current==-1) {
                reset_observed=true;controls[target].zoom=-1;
                controls[target].zoom_reset_serial=state.players[target].zoom_reset_serial;
            }
        }
        need(melee_admitted,remote?"Remote melee not admitted by its source owner":"Local melee not admitted by its source owner");
        need(reset_observed && state.players[target].source_zoom.current==-1,
            remote?"Actual remote melee did not reset scope":"Actual local melee did not reset scope");
        std::vector<NativeWeaponZoomAudioEventTu15> resets;
        for(const auto &event:state.weapon_zoom_audio)if(event.serial>baseline && event.unit==unit && event.forced_reset)resets.push_back(event);
        need(resets.size()==1,"Actual native CABC0 reset decision was omitted or duplicated");
        const auto &event=resets.front();
        const auto player_rows=word(word(0x4f54f4u)+0x44u);
        const auto source_local_user=std::bit_cast<std::int16_t>(Bytes(world.source_objects->memory().view(
            player_rows+(state.players[target].source_player&65535u)*0x21cu+0x28u,2)).u16(0));
        need(event.player==state.players[target].source_player && event.sound==out_sound && event.control_bits==0x3f800000u &&
            source_local_user==(remote?-1:1),"Native reset lost its original owner/sound/local-user inputs");
        need(event.tick && event.tick<=state.tick,"Native reset lost its actual source tick");
        for(const auto &event:pending)need(event.at("sound")==out_sound,"Recipient reset selected another sound");
        if(!remote)need(zoom_in.size()==1 && pending.size()==1,"Local source zoom/reset pair missing");
        if(!remote) {
            // Equal native RNG inputs and real independent PCM owners. One
            // consumes the actual local18CDD0 packet, the other consumes the
            // committed reset through immutable recipient metadata/delivery.
            MultiplayerAudio local_audio(world.cache,0x715541,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
            MultiplayerAudio recipient_audio(world.cache,0x715541,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
            local_audio.bind_native_sound_metadata(native.sound_variation_provider(*world.cache));
            recipient_audio.bind_native_loop_metadata(MccSoundMetadataPublication::create(world.cache));
            const std::array<Vec3,2> listeners{state.players[host].position,state.players[target].position};
            local_audio.listeners(listeners);recipient_audio.listeners(listeners);
            need(local_audio.source_impulse(out_sound,actual_requests.at(0),MultiplayerAudioKind::FirstPerson,1).has_value(),"Actual local reset has no PCM voice");
            std::vector<NativeWeaponZoomAudioPendingTu15> projected{{event,1}};
            need(deliver_native_weapon_zoom_audio_tu15(recipient_audio,projected)==1,"Recipient reset has no PCM voice");
            double energy{};
            for(unsigned block=0;block<8;++block) {
                const auto a=local_audio.render(4096),b=recipient_audio.render(4096);need(a.size()==b.size(),"Recipient PCM extent differs");
                for(unsigned n=0;n<a.size();++n) {
                    need(std::isfinite(a[n]) && std::bit_cast<std::uint32_t>(a[n])==std::bit_cast<std::uint32_t>(b[n]),"Recipient PCM differs from actual local reset request");
                    energy+=double(a[n])*a[n];
                }
            }
            need(energy>0,"Reset PCM comparison was silent");
        }
        rows.push_back({{"recipient_local",!remote},{"unit",unit},{"definition",definition},
            {"zoom_in_requests",zoom_in},{"forced_zoom_out_requests",pending},{"reset_state",state.players[target].source_zoom.current},
            {"committed_reset",{{"tick",event.tick},{"player",event.player},{"sound",event.sound},
                {"control_bits",event.control_bits},{"source_local_user",source_local_user}}}});
    }
    nlohmann::json report{{"passed",true},{"rows",rows},{"actual_source_execution",true},
        {"joined_client_delivery_proven",false},{"local_and_recipient_reset_pcm_equal",true},
        {"test_controls_supplied",true},{"request_observers_read_only",true}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
