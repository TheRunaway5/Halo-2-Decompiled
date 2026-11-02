#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/native_weapon_audio_route_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <SDL.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
void need(bool ok,const char *why){if(!ok)throw FormatError(why);}
struct Request {std::uint32_t sound;std::array<std::byte,160> bytes;std::optional<std::uint32_t> zoom_unit;};
}
int main(int argc,char **argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use the test storage runner");
    need(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Could not select the private manual audio test driver");
    Cache cache(argv[1],argv[2]);need(cache.is_mcc(),"Actual zoom audio requires MCC-only runtime inputs");
    const auto content=cache.load_content(0,true);
    const auto choices=cache.multiplayer_weapon_choices();
    std::set<std::uint32_t> definitions;
    unsigned worlds{},requests{},submissions{},levels{},binoculars{},firing_requests{},forced_resets{},unowned_zoom{},melee_resets{};
    double mixed_energy{};
    nlohmann::json rows=nlohmann::json::array();
    for(unsigned option=3;option<=20;++option) {
        std::uint32_t unused_random{};
        const auto definition=source_starting_weapon(null_datum,std::int8_t(option),null_datum,choices,unused_random);
        if(definition==null_datum || !definitions.insert(definition).second)continue;
        const auto &weapon=content.weapons.at(definition);
        const ZoomWeaponView zoom{weapon.zoom,{}};
        need(content.binocular_zoom.has_value(),"Authored binocular owner is missing");
        const bool zoom_available=source_zoom_next(-1,&zoom,*content.binocular_zoom)!=-1;
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
        options.local_players=2;options.starting_weapon=option;options.mute=true;
        options.source_object_world=true;options.session_random_seed=0x105240;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,int(option)}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(option);
        MultiplayerPreparedWorld world(options);
        auto &native=*world.source_objects;need(native.owns_physics(),"Actual native authority is absent");
        world.bind_source_local_views(world.local_slots);
        std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
        const auto player=world.local_slots.at(0),other=world.local_slots.at(1);
        auto state=world.match.state();
        need(state.players[player].weapon==definition,"Starting weapon owner differs");
        std::vector<Request> pending;
        struct ObserverLifetime {
            MultiplayerObjectRuntime &world;
            ~ObserverLifetime(){world.tu15_world().observe_sound_requests_with_context({});}
        } observer_lifetime{native};
        native.tu15_world().observe_sound_requests_with_context([&](auto sound,std::span<const std::byte,160> bytes,auto unit) {
            Request event{sound,{},unit};std::copy(bytes.begin(),bytes.end(),event.bytes.begin());pending.push_back(event);
        });
        unsigned packets{};
        MultiplayerAudio audio(world.cache,0x715541,
            {SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
            [&](const auto &s){need(s.voice && s.frames,"Zoom submission has no actual PCM voice");++packets;});
        audio.bind_native_sound_metadata(native.tu15_world().sound_variation_provider(*world.cache));
        const std::array<Vec3,2> listeners{state.players[player].position,state.players[other].position};
        audio.listeners(listeners);
        const auto word=[&](auto at){return Bytes(native.memory().view(at,4)).u32(0);};
        // Read the converted native definition actually consumed by C7DD0,
        // not an assumed filename or an independently selected sound.
        std::uint32_t in_sound=null_datum,out_sound=null_datum;
        if(weapon.zoom.levels>0) {
            const auto tags=word(0x4f0414u),data=word(tags+(definition&65535u)*16u+8u);
            in_sound=word(data+0x270u);out_sound=word(data+0x278u);
        }else if(zoom_available) {
            const auto info=word(word(0x4ecc1cu)+0x134u);
            in_sound=word(info+0xc8u);out_sound=word(info+0xd0u);++binoculars;
        }
        const auto check_transition=[&](std::int16_t selector,unsigned target=0) {
            const auto selected=world.local_slots.at(target),unselected=world.local_slots.at(1-target);
            pending.clear();const auto before=packets;
            controls[selected].zoom=selector;
            controls[selected].zoom_reset_serial=state.players[selected].zoom_reset_serial;
            world.match.step(controls);state=world.match.state();
            need(state.players[selected].source_zoom.current==selector,"Actual zoom transition was rejected");
            need(state.players[unselected].source_zoom.current==-1,"Zoom leaked to another local unit");
            const auto &p=state.players[selected];const auto &q=state.players[unselected];
            const auto &first=state.players[world.local_slots.at(0)],&second=state.players[world.local_slots.at(1)];
            const std::array views{NativeWeaponAudioViewTu15{first.source_unit,first.source_weapon,first.secondary_weapon.datum},
                NativeWeaponAudioViewTu15{second.source_unit,second.source_weapon,second.secondary_weapon.datum}};
            unsigned matched{};
            for(const auto &event:pending) {
                const Bytes raw(event.bytes);const auto owner=raw.u32(0x50);
                if(event.zoom_unit!=p.source_unit)continue;
                if(raw.u32(0)&8u)need(owner==p.source_unit,"Owned zoom request lost its actual unit owner");
                else ++unowned_zoom;
                const auto expected=selector==-1?out_sound:in_sound;
                need(expected!=null_datum && event.sound==expected,"Zoom dispatched a different authored sound");
                need(!(raw.u32(0)&0x10u),"Zoom did not produce a one-shot request");
                const auto view=native_weapon_audio_view_tu15(*event.zoom_unit,views,true);
                need(view && *view==target,"Actual unit-owned zoom sound was dropped or crossed views");
                // The original control is level/(count-1) only for multiple
                // levels; single-level zoom and zoom-out retain exactly one.
                const auto count=weapon.zoom.levels>0?weapon.zoom.levels:content.binocular_zoom->levels;
                const auto control=selector!=-1 && count>1?float(selector)/float(count-1):1.f;
                need(std::bit_cast<std::uint32_t>(raw.f32(0x10))==std::bit_cast<std::uint32_t>(control),
                    "Zoom control lost its authored level fraction bits");
                const auto kind=(std::to_integer<unsigned>(event.bytes[15])&15u)==1u?
                    MultiplayerAudioKind::Impulse:MultiplayerAudioKind::FirstPerson;
                need(audio.source_impulse(event.sound,event.bytes,kind,*view).has_value(),"Actual zoom sound has no native playback");
                ++matched;++requests;
            }
            need(matched==((selector==-1?out_sound:in_sound)!=null_datum?1u:0u),"Zoom request count differs");
            need(packets-before==matched,"Zoom native playback duplicated or omitted a source request");
            // Manual mixer consumption uses the same native SDL stream owner.
            double energy{};
            for(unsigned block=0;block<(matched?8u:1u);++block) {
                const auto pcm=audio.render(4096);need(pcm.size()==8192,"Zoom mixer did not consume stereo output");
                for(auto sample:pcm){need(std::isfinite(sample),"Zoom mixer emitted nonfinite PCM");energy+=double(sample)*sample;}
            }
            need(matched?energy>0:energy==0,"Zoom native request did not produce its expected audible/silent PCM");
            mixed_energy+=energy;audio.stop();submissions+=matched;
        };
        const auto count=!zoom_available?0:weapon.zoom.levels>0?weapon.zoom.levels:content.binocular_zoom->levels;
        for(unsigned target=0;target<2;++target) {
            for(int selector=0;selector<count;++selector){check_transition(std::int16_t(selector),target);++levels;}
            if(zoom_available)check_transition(-1,target);
        }
        // Execute actual held-fire/release through the same authority and the
        // native dispatcher. Every observed visual request must retain an
        // authored barrel reference, the current full weapon and unit owner.
        std::set<std::uint32_t> authored,allowed;
        const auto add_barrel=[&](const auto &barrel) {
            for(const auto &entry:barrel.firing_effects)for(unsigned selection=0;selection<3;++selection)
                if(entry.effects[selection]!=null_datum) {
                    const auto tag=cache.query_tag({.handle=TagHandle{entry.effects[selection]}});
                    if(tag && tag->group=="effe") {
                        allowed.insert(entry.effects[selection]);
                        if(!selection)authored.insert(entry.effects[selection]);
                    }
                }
            if(barrel.ready_effect!=null_datum)allowed.insert(barrel.ready_effect);
            for(const auto &trigger:barrel.triggers) {
                if(trigger.charging_effect!=null_datum)allowed.insert(trigger.charging_effect);
                if(trigger.discharging_effect!=null_datum)allowed.insert(trigger.discharging_effect);
            }
        };
        add_barrel(weapon);for(const auto &barrel:weapon.barrel_views)add_barrel(*barrel);
        // The SMG exercises CABC0's unowned forced-reset request. Other
        // weapons can leave binoculars through their action/admission gates;
        // their authored explicit transitions were checked above.
        const bool expect_forced_reset=zoom_available && definition==cache.slayer_starting_weapon(5) && !authored.empty();
        if(expect_forced_reset)check_transition(0);
        const auto expected_unit=state.players[player].source_unit,expected_weapon=state.players[player].source_weapon;
        unsigned shots{},forced{};
        std::uint64_t serial=state.weapon_effects.empty()?0:state.weapon_effects.back().serial;
        for(unsigned tick=0;tick<48;++tick) {
            pending.clear();
            controls[player].fire=tick<24;
            world.match.step(controls);state=world.match.state();
            for(const auto &event:pending)if(event.zoom_unit==expected_unit) {
                need(expect_forced_reset && event.sound==out_sound && state.players[player].source_zoom.current==-1,
                    "Firing reset selected a different authored zoom-out sound/state");
                const auto &p=state.players[player];const auto &q=state.players[other];
                const std::array views{NativeWeaponAudioViewTu15{p.source_unit,p.source_weapon,p.secondary_weapon.datum},
                    NativeWeaponAudioViewTu15{q.source_unit,q.source_weapon,q.secondary_weapon.datum}};
                const auto view=native_weapon_audio_view_tu15(*event.zoom_unit,views,true);
                need(view && *view==0,"Unowned forced zoom-out lost its actual calling unit/view");
                need(!(Bytes(event.bytes).u32(0)&8u),"Forced zoom reset did not exercise the original unowned request");
                need(audio.source_impulse(event.sound,event.bytes,MultiplayerAudioKind::FirstPerson,*view).has_value(),
                    "Forced zoom-out produced no native PCM submission");
                (void)audio.render(4096);audio.stop();++requests;++submissions;++forced;++forced_resets;++unowned_zoom;
            }
            if(expect_forced_reset && state.players[player].source_zoom.current==-1)controls[player].zoom=-1;
            for(const auto &event:state.weapon_effects)if(event.serial>serial) {
                serial=event.serial;if(event.weapon!=expected_weapon)continue;
                need(event.unit==expected_unit && allowed.contains(event.effect),
                    "Scheduled native firing changed its actual unit/weapon/authored effect");
                need(event.source_scheduled && event.source_presentation && event.source_instance!=null_datum &&
                    (!authored.contains(event.effect) || event.source_locations_observed),
                    "Native shooting lost its actual source birth/location publication");
                if(authored.contains(event.effect)){++shots;++firing_requests;}
            }
        }
        need(!expect_forced_reset || forced==(out_sound==null_datum?0u:1u),
            "Actual firing did not deliver the authored forced zoom-out count");
        need(!expect_forced_reset || state.players[player].source_zoom.current==-1,
            "Actual prepared shot did not reset binocular state when its authored sound is NONE");
        need(authored.empty() || shots>0,"Authored normal firing effect produced no completed native request");
        unsigned interrupted{};
        if(weapon.zoom.levels>0 && out_sound!=null_datum) {
            check_transition(0);
            for(unsigned tick=0;tick<36;++tick) {
                pending.clear();controls[player].melee=tick==0;
                world.match.step(controls);state=world.match.state();
                for(const auto &event:pending)if(event.zoom_unit==expected_unit) {
                    need(event.sound==out_sound && state.players[player].source_zoom.current==-1,
                        "Melee interruption lost its authored zoom-out sound/state");
                    need(!(Bytes(event.bytes).u32(0)&8u),"Melee reset did not exercise the unowned source request");
                    const auto &p=state.players[player];const auto &q=state.players[other];
                    const std::array views{NativeWeaponAudioViewTu15{p.source_unit,p.source_weapon,p.secondary_weapon.datum},
                        NativeWeaponAudioViewTu15{q.source_unit,q.source_weapon,q.secondary_weapon.datum}};
                    const auto view=native_weapon_audio_view_tu15(*event.zoom_unit,views,true);
                    need(view && *view==0,"Melee reset lost its actual unit/view");
                    need(audio.source_impulse(event.sound,event.bytes,MultiplayerAudioKind::FirstPerson,*view).has_value(),
                        "Melee reset produced no native PCM submission");
                    (void)audio.render(4096);audio.stop();++requests;++submissions;++interrupted;++melee_resets;++unowned_zoom;
                }
                if(state.players[player].source_zoom.current==-1)controls[player].zoom=-1;
            }
            need(interrupted==1,"Scoped melee did not emit exactly one authored zoom-out request");
        }
        native.tu15_world().observe_sound_requests_with_context({});
        rows.push_back({{"weapon",weapon.name},{"option",option},{"levels",count},
            {"zoom_in_sound",in_sound},{"zoom_out_sound",out_sound},{"packets",packets},
            {"forced_zoom_resets",forced},{"melee_zoom_resets",interrupted},{"firing_effect_requests",shots}});
        std::cout<<"PASS actual "<<weapon.name<<" zoom levels="<<count<<" PCM packets="<<packets
            <<" native firing effects="<<shots<<'\n'<<std::flush;
        ++worlds;
    }
    need(worlds>=6 && levels>=8 && binoculars>0 && melee_resets>0 && requests==submissions,"Incomplete native zoom coverage");
    nlohmann::json report{{"passed",true},{"worlds",worlds},{"levels",levels},{"binocular_weapons",binoculars},
        {"source_requests",requests},{"native_pcm_submissions",submissions},{"firing_effect_requests",firing_requests},
        {"forced_zoom_resets",forced_resets},{"melee_zoom_resets",melee_resets},{"weapons",rows},
        {"unowned_zoom_requests",unowned_zoom},{"native_pcm_energy",mixed_energy},
        {"audio_device","SDL dummy / manual mixer"},
        {"runtime_cache_source","MCC"},{"xbox_pcm_or_gpu_parity",false},{"retained_large_artifacts",nlohmann::json::array()}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
