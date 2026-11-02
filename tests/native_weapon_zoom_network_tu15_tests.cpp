#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/native_weapon_zoom_audio_tu15.hpp"
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
}
int main(int argc,char **argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use owned test storage");
    need(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Could not select private manual audio");
    Cache cache(argv[1],argv[2]);need(cache.is_mcc(),"Native zoom UDP requires MCC");
    const auto content=cache.load_content(0,true);const auto choices=cache.multiplayer_weapon_choices();
    need(content.binocular_zoom.has_value(),"Native binocular definition missing");
    unsigned worlds{},packets{},resets{},transitions{};double energy{};nlohmann::json rows=nlohmann::json::array();
    std::set<std::uint32_t> definitions;
    for(unsigned option=3;option<=20;++option) {
        std::uint32_t random{};const auto definition=source_starting_weapon(null_datum,std::int8_t(option),null_datum,choices,random);
        if(definition==null_datum || !definitions.insert(definition).second)continue;
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=1;
        options.starting_weapon=option;options.mute=true;options.source_object_world=true;options.session_random_seed=0x105240;
        options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,int(option)}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(option);
        MultiplayerPreparedWorld world(options);LanHost host(world.match,0,789);LanClient client(host.join_address(),789,4);
        std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,4> local{};
        for(unsigned n=0;n<32 && !client.ready();++n){client.send(local);host.receive(controls);host.broadcast();client.receive();}
        need(client.ready() && client.error().empty() && client.slots().size()==4,"Four-view native UDP recipient did not join");
        const auto step=[&] {
            client.send(local);host.receive(controls);world.match.step(controls);host.broadcast();client.receive();
            need(client.error().empty() && client.state().tick==world.match.state().tick,"Native zoom source tick did not reach UDP recipient");
            need(client.state().weapon_zoom_audio==world.match.state().weapon_zoom_audio,"Native UDP changed zoom journal bytes/owners/order");
        };
        for(unsigned n=0;n<120;++n)step();
        const auto native_word=[&](auto at){return Bytes(world.source_objects->memory().view(at,4)).u32(0);};
        const auto tags=native_word(0x4f0414u),data=native_word(tags+(definition&65535u)*16u+8u);
        const auto in_sound=native_word(data+0x270u),out_sound=native_word(data+0x278u);
        const bool audible=content.weapons.at(definition).zoom.levels>0;
        NativeWeaponZoomAudioCursorTu15 cursor,host_cursor;std::vector<NativeWeaponZoomAudioPendingTu15> pending,host_pending;
        collect_native_weapon_zoom_audio_tu15(client.state(),client.slots(),cursor,pending);
        collect_native_weapon_zoom_audio_tu15(world.match.state(),world.local_slots,host_cursor,host_pending);
        unsigned submitted{};MultiplayerAudio audio(world.cache,0x715541,
            {SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto &event){need(event.voice && event.frames,"Recipient has no real PCM voice");++submitted;});
        audio.bind_native_loop_metadata(MccSoundMetadataPublication::create(world.cache));
        std::vector<Vec3> listeners;for(auto id:client.slots())listeners.push_back(client.state().players[id].position);audio.listeners(listeners);
        const auto deliver=[&](unsigned expected,unsigned view,bool forced) {
            collect_native_weapon_zoom_audio_tu15(client.state(),client.slots(),cursor,pending);
            collect_native_weapon_zoom_audio_tu15(world.match.state(),world.local_slots,host_cursor,host_pending);
            need(host_pending.empty(),"Remote zoom leaked to the host's local player");
            unsigned audible_pending{};
            for(const auto &p:pending) {
                need(p.view==view && p.event.forced_reset==forced,"Native zoom crossed recipient views or action kinds");
                if(p.event.sound!=null_datum){++audible_pending;need(p.event.sound==(forced?out_sound:(local[view].zoom==-1?out_sound:in_sound)),"Recipient selected another authored sound");}
            }
            need(audible_pending==expected,"Recipient zoom request count differs");
            const auto before=submitted;need(deliver_native_weapon_zoom_audio_tu15(audio,pending)==expected,"Recipient did not deliver its native PCM count");
            need(submitted-before==expected && pending.empty(),"Native zoom duplicated/retained a consumed request");
            collect_native_weapon_zoom_audio_tu15(client.state(),client.slots(),cursor,pending);need(pending.empty(),"Repeated snapshot replayed zoom audio");
            double sum{};for(unsigned n=0;n<(expected?8u:1u);++n)for(auto sample:audio.render(4096)){need(std::isfinite(sample),"Recipient PCM is nonfinite");sum+=double(sample)*sample;}
            need(expected?sum>0:sum==0,"Recipient native PCM audible/silent outcome differs");energy+=sum;audio.stop();packets+=expected;
        };
        const auto count=audible?content.weapons.at(definition).zoom.levels:content.binocular_zoom->levels;
        for(unsigned view=0;view<4;++view) {
            const auto id=client.slots()[view];
            for(int level=0;level<count;++level) {
                local[view].zoom=std::int16_t(level);local[view].zoom_reset_serial=client.state().players[id].zoom_reset_serial;step();
                need(client.state().players[id].source_zoom.current==level,"Recipient native scope level was not admitted");deliver(audible?1:0,view,false);++transitions;
            }
            local[view].zoom=-1;local[view].zoom_reset_serial=client.state().players[id].zoom_reset_serial;step();deliver(audible?1:0,view,false);
            local[view].zoom=0;local[view].zoom_reset_serial=client.state().players[id].zoom_reset_serial;step();deliver(audible?1:0,view,false);
            bool admitted{},reset{};unsigned committed{};
            for(unsigned n=0;n<36;++n) {
                local[view].melee=n==0;step();const auto &p=client.state().players[id];admitted|=p.melee_action.active();
                if(p.source_zoom.current==-1){reset=true;local[view].zoom=-1;local[view].zoom_reset_serial=p.zoom_reset_serial;}
                unsigned new_resets{};
                for(const auto &event:client.state().weapon_zoom_audio)if(event.serial>cursor.serial &&
                    event.player==p.source_player && event.forced_reset)++new_resets;
                committed+=new_resets;deliver(audible?new_resets:0,view,true);
            }
            need(admitted && reset && committed==1,"Real remote melee did not admit/reset/publish exactly once");++resets;
            collect_native_weapon_zoom_audio_tu15(client.state(),client.slots(),cursor,pending);need(pending.empty(),"Melee generated a second reset sound");
        }
        need(!cursor.missing_prefix && !host_cursor.missing_prefix,"Native zoom journal lost a prefix during UDP qualification");
        rows.push_back({{"option",option},{"definition",definition},{"views",4},{"packets",submitted},{"source_events",world.match.state().weapon_zoom_audio.size()}});++worlds;
    }
    if(worlds!=15 || resets!=60 || packets<=50)
        std::cerr<<nlohmann::json{{"worlds",worlds},{"resets",resets},{"packets",packets},{"rows",rows}}.dump()<<'\n';
    need(worlds==15 && resets==60 && packets>50,"Incomplete native recipient weapon/view scope");
    nlohmann::json report{{"passed",true},{"worlds",worlds},{"recipient_views",4},{"explicit_levels",transitions},
        {"melee_resets",resets},{"native_pcm_submissions",packets},{"pcm_energy",energy},{"rows",rows},
        {"transport","native UDP loopback"},{"audio_device","SDL dummy/manual"},{"physical_xbox_or_device_parity",false}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
