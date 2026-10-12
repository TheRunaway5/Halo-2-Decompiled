#include "halo2/native_vista_announcer.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_king.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
double peak(const std::vector<float>& pcm){double result=0;for(const auto sample:pcm){
  check(std::isfinite(sample),"Announcer emitted nonfinite native PCM");result=std::max(result,std::abs(double(sample)));}return result;}
void drain_sentinel(NativeVistaAnnouncer& owner){for(unsigned n=0;n<62;++n)owner.pre_world_tick();
  check(owner.snapshot().source.queue.count==0,"Original two-second sentinel was not consumed by source ticks");}
void start_end(NativeVistaAnnouncer& owner,std::uint32_t tick){
  const VistaAnnouncerLocalPlayer local{0x12340000,1,0,0};
  check(owner.post_world_tick(false,1,true,tick,{&local,1}).empty() &&
      owner.post_world_tick(true,4,true,tick,{&local,1}).empty() &&
      owner.post_world_tick(true,1,false,tick,{&local,1}).empty(),"Original end guards dispatched an event");
  check(!owner.snapshot().game_over_published && owner.snapshot().pending_start_boundary,
      "A rejected end guard claimed publication or proved start activation");
  const auto selection=owner.post_world_tick(true,1,true,tick,{&local,1});
  check(selection.size()==1 && selection[0].announcer && selection[0].requested &&
      selection[0].sound!=vista_announcer_none,"Actual PC general19 response was not queued");
  check(owner.post_world_tick(true,1,true,tick+1,{&local,1}).empty() &&
      owner.post_world_tick(true,1,false,tick+2,{&local,1}).empty() &&
      owner.post_world_tick(true,1,true,tick+3,{&local,1}).empty(),"End event repeated after C48 publication");
}
}
int main(int argc,char** argv){try{
  check(argc==5,"Supply PC_MAPS ACTUAL_SCENARIO NATIVE_AUDIO_XBE ORIGINAL_KING_FIXTURE");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  SDL_setenv("SDL_AUDIODRIVER","dummy",1);
  auto original=OriginalData::load_xbe(argv[3]);
  NativeVistaAnnouncerConfiguration config{argv[1],argv[2],original,0,30,{0x27f,0x1f80},false,0x5821b36d,
      {SdlSoundPlatformConfiguration::Delivery::Manual,256,{}}};
  std::array<float,sound_class_count> gains{};
  for(const auto category:{2u,6u})for(const auto mode:{std::uint8_t(1),std::uint8_t(3),std::uint8_t(5)}) {
    auto owner=NativeVistaAnnouncer::open(config);drain_sentinel(*owner);
    const VistaAnnouncerLocalPlayer local{0x12340000,9,0,0};
    auto event=vista_announcer_event_construct(category,0);event.words[2]=local.datum;
    bool rejected=false;try{auto invalid=local;invalid.flags=8;
      (void)owner->publish_player_activation(event,mode,false,62,{&invalid,1});
    }catch(const FormatError&){rejected=true;}
    check(rejected && owner->snapshot().pending_start_boundary,
        "Unactivated local player claimed the initial activation boundary");
    const auto selected=owner->publish_player_activation(event,mode,false,62,{&local,1});
    check(!owner->snapshot().pending_start_boundary && selected.size()==1 &&
        selected[0].announcer && selected[0].requested && selected[0].sound!=vista_announcer_none,
        "Actual engine activation publication did not enter the authored start cue queue");
    check(owner->snapshot().voices.empty(),"Activation played before the genuine queue update");
    owner->audio_update(1,gains);owner->pre_world_tick();
    double actual_peak=0;for(unsigned n=0;n<240;++n)actual_peak=std::max(actual_peak,peak(owner->render_manual(4096)));
    owner->audio_update(1,gains);
    check(actual_peak>0 && owner->snapshot().completions.size()==1,
        "Actual Vista engine start cue failed native PCM/completion delivery");
  }
  {
    auto owner=NativeVistaAnnouncer::open(config);
    check(owner->assets().responses.king.size()==8,"Actual King response bank was not imported");
    std::ifstream fixture(argv[4]);check(bool(fixture),"Open independent King response fixture");
    const auto original_rows=nlohmann::json::parse(fixture).at("responses").at("king");
    check(original_rows.size()==8,"Independent King response coverage");
    for(unsigned i=0;i<8;++i){const auto hex=original_rows.at(i).get<std::string>();
      check(hex.size()==0x150,"Independent King response stride");
      for(unsigned j=0;j<0xa8;++j)check(owner->assets().responses.king.at(i)[j]==
          std::byte(std::stoul(hex.substr(j*2,2),nullptr,16)),"Actual imported King response differs from original parser");}
  }
  for(unsigned ordinal=0;ordinal<8;++ordinal){
    auto owner=NativeVistaAnnouncer::open(config);drain_sentinel(*owner);
    const Bytes row(owner->assets().responses.king.at(ordinal));const auto subtype=row.u16(4),audience=row.u16(6);
    if(!subtype)continue; // Targeted activation/start was exercised above.
    const VistaAnnouncerLocalPlayer local{0x12340000,1,0,0};
    SourceKingNotice notice{int(subtype),vista_announcer_none,-1};
    if(audience==0)notice.player=local.datum;
    if(subtype==5 || subtype==6){notice.membership=local.membership;
      if(audience==1)notice.player=0x12340003u;}
    const auto event=vista_announcer_king_notice(notice);
    check(owner->publish_king_event(event,4,false,62,{&local,1}).empty() &&
        owner->publish_king_event(event,1,true,62,{&local,1}).empty(),"King audio bypassed original playback/game-over guards");
    const auto selection=owner->publish_king_event(event,1,false,62,{&local,1});
    check(selection.size()==1 && selection[0].response_ordinal==ordinal && selection[0].announcer &&
        selection[0].requested && selection[0].sound==row.u32(0x48),"King event lost its actual authored audience/cue");
    check(owner->snapshot().voices.empty(),"King event bypassed the actual queue update");
    owner->audio_update(1,gains);owner->pre_world_tick();
    double actual_peak=0;for(unsigned n=0;n<240;++n)actual_peak=std::max(actual_peak,peak(owner->render_manual(4096)));
    owner->audio_update(1,gains);const auto completed=owner->snapshot();
    check(actual_peak>0 && completed.voices.empty() && completed.completions.size()==1 &&
        completed.completions[0].packet.status==SdlSoundCompletionStatus::Success &&
        completed.completions[0].packet.pcm_frames>0,"Actual King cue failed PCM and completion delivery");
  }
  {
    auto portuguese=config;portuguese.language=8;auto owner=NativeVistaAnnouncer::open(portuguese);drain_sentinel(*owner);
    const VistaAnnouncerLocalPlayer local{0x12340000,1,0,0};
    for(const auto& row:owner->assets().responses.king)check(Bytes(row).u32(0x88)==vista_announcer_none,
        "Original Portuguese King field is no longer absent");
    const auto event=vista_announcer_king_notice({4,vista_announcer_none,-1});
    const auto selection=owner->publish_king_event(event,1,false,62,{&local,1});
    check(selection.size()==1 && selection[0].sound==vista_announcer_none && !selection[0].requested &&
        owner->snapshot().source.queue.count==0,"Absent Portuguese King cue acquired a fallback");
    owner->pre_world_tick();check(owner->snapshot().voices.empty() && owner->snapshot().completions.empty(),
        "Missing King cue fabricated playback/completion");
  }
  {
    auto owner=NativeVistaAnnouncer::open(config);
    check(owner->assets().cache->is_vista() && owner->assets().cache->scenario()==argv[2] &&
        owner->audio_cache().is_vista() && &owner->audio_cache()!=owner->assets().cache.get(),
        "Announcer retained the wrong sound namespace or scenario");
    drain_sentinel(*owner);start_end(*owner,62);
    const auto before=owner->snapshot();check(before.source.queue.count==1 && before.voices.empty(),
        "Event delivery played ahead of the actual pre-world queue update");
    owner->audio_update(0,gains);owner->pre_world_tick();
    const auto submitted=owner->snapshot();
    check(submitted.voices.size()==1 && submitted.source.queue.slots[0].active &&
        submitted.source.queue.slots[0].voice==submitted.voices[0].queue_token &&
        submitted.voices[0].native_voice!=submitted.voices[0].queue_token,
        "Source queue voice was not a distinct registered native adapter token");
    for(unsigned n=0;n<240;++n)check(peak(owner->render_manual(4096))==0,"Actual master-volume zero became audible");
    check(owner->snapshot().voices.size()==1 && owner->snapshot().completions.empty(),
        "Manual mixer bypassed actual device completion accounting");
    owner->audio_update(0,gains);const auto completed=owner->snapshot();
    check(completed.voices.empty() && completed.completions.size()==1 &&
        completed.completions[0].queue_token==submitted.voices[0].queue_token &&
        completed.completions[0].packet.voice==submitted.voices[0].native_voice &&
        completed.completions[0].packet.status==SdlSoundCompletionStatus::Success &&
        completed.completions[0].packet.pcm_frames>0 && !completed.source.queue.slots[0].active,
        "Real PCM completion did not clear only its registered source token");
    owner->pre_world_tick();owner->audio_update(1,gains);owner->stop();
    check(owner->snapshot().source.queue.count==0 && owner->snapshot().completions.size()==1,
        "Completed cue failed to shift or completed twice");
  }
  {
    auto owner=NativeVistaAnnouncer::open(config);drain_sentinel(*owner);start_end(*owner,62);
    owner->audio_update(1,gains);owner->pre_world_tick();
    double actual_peak=0;for(unsigned n=0;n<30;++n)actual_peak=std::max(actual_peak,peak(owner->render_manual(4096)));
    check(actual_peak>0,"Actual Vista game-over sound had no decoded PCM output");
    for(unsigned n=0;n<240;++n)(void)owner->render_manual(4096);
    owner->audio_update(1,gains);check(owner->snapshot().completions.size()==1 &&
        owner->snapshot().completions[0].packet.status==SdlSoundCompletionStatus::Success,
        "Audible Vista packet did not complete through the native owner");
  }
  {
    auto owner=NativeVistaAnnouncer::open(config);drain_sentinel(*owner);start_end(*owner,62);
    owner->pre_world_tick();owner->stop();const auto stopped=owner->snapshot();
    check(stopped.voices.empty() && stopped.completions.size()==1 &&
        stopped.completions[0].packet.status==SdlSoundCompletionStatus::Flushed &&
        !stopped.source.queue.slots[0].active,"Pending native audio did not retain a flushed completion");
    owner->stop();check(owner->snapshot().completions.size()==1,"Stop delivered a completion twice");
  }
  {
    config.mute=true;config.platform.device_name="not-an-audio-device";
    auto owner=NativeVistaAnnouncer::open(config);drain_sentinel(*owner);start_end(*owner,62);
    owner->pre_world_tick();const auto muted=owner->snapshot();
    check(muted.muted && muted.voices.empty() && muted.completions.empty() &&
        muted.source.queue.slots[0].voice==vista_announcer_none && !muted.source.queue.slots[0].active,
        "Actual mute requested a device voice or fabricated a completion");
    owner->pre_world_tick();check(owner->snapshot().source.queue.count==0,"Muted failed-playback queue did not shift");
  }
  std::cout<<"PASS native Vista announcer: Slayer/King activation, all8 King responses and absent Portuguese cues, end queues, PC asset lifetime, actual PCM and gains, registered completion, flushed stop, mute\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
