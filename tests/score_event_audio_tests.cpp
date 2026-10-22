#include "helpers/gameplay_fixture.hpp"
#include "halo2/native_vista_announcer.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/source_score_event.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::array<float,3> interior(const SourceKingState& hill) {
  SourceKingPlayer player;player.datum=0;player.current_unit=0;
  for(const float z:{hill.center[2],(hill.minimum_z+hill.maximum_z)*.5f})
    for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y) {
      const std::array<float,3> point{hill.center[0]+hill.radius*float(x)*.1f,hill.center[1]+hill.radius*float(y)*.1f,z};
      if(source_king_contains(hill,player,[&](std::uint32_t){return point;}))return point;
    }
  throw FormatError("Authored hill has no sampled original-contains interior");
}
void run(const Cache& cache,const LevelContent& content,
    const NativeVistaAnnouncerConfiguration& config,bool teams,bool moving) {
  auto rules=king_rules({"King score audio",3,{{91,int(teams)},{40,moving?1:0},{2,61},{0x59,5}}},true);
  rules.weapon=cache.slayer_starting_weapon(5,4);rules.time_ticks=0;
  auto audio=NativeVistaAnnouncer::open(config);
  for(unsigned i=0;i<62;++i)audio->pre_world_tick();
  check(!audio->snapshot().source.queue.count,"Source initial announcer sentinel did not drain");
  SourceScoreEventOptions options{std::uint32_t(rules.score_limit),rules.player_flags,1};
  std::vector<VistaAnnouncerEvent> network_observations,local_events;
  std::vector<VistaAnnouncerAudioSelection> selected;
  unsigned notices{},changes{};double peak{};
  MultiplayerKingServices consumers;
  consumers.score_changed=[&](const MultiplayerMatch& actual,std::uint32_t player,std::int32_t score) {
    ++changes;SourceScoreEventServices services;
    services.options=[&]()->const SourceScoreEventOptions&{return options;};
    services.engine_present=[&]{return actual.source_engine()!=0;};
    services.membership=[&](std::uint32_t datum){return actual.player(datum&0xffffu).source_membership;};
    // This observes the original network service boundary. It deliberately
    // does not claim native session/event transport from this local test.
    services.network=[&](const VistaAnnouncerEvent& event){network_observations.push_back(event);};
    services.local=[&](const VistaAnnouncerEvent& event) {
      check(network_observations.size()==local_events.size()+1,"Local score audio preceded network publication");
      local_events.push_back(event);
      const auto& p=actual.player(0);
      const VistaAnnouncerLocalPlayer local{p.source_datum,p.source_flags,p.source_membership,0};
      const auto tick=std::uint32_t(actual.simulation().ticks());
      check(audio->publish_engine_event(event,4,false,tick,{&local,1}).empty() &&
          audio->publish_engine_event(event,1,true,tick,{&local,1}).empty(),"Score audio bypassed source playback/game-over guards");
      const auto result=audio->publish_engine_event(event,1,actual.finished(),tick,{&local,1});
      check(result.size()==1 && result[0].announcer && result[0].requested &&
          result[0].sound!=vista_announcer_none,"Actual score event did not select an authored queued response");
      selected.push_back(result[0]);
    };
    source_score_changed_event(player,score,services);
  };
  // Hill notices are an explicitly observed separate presentation boundary.
  consumers.notification=[&](const MultiplayerMatch&,const SourceKingNotice&){++notices;};
  MultiplayerMatch match(content,0,cache.spawns(),rules,consumers);
  match.join("Native King score recipient",teams?0:-1);
  match.set_game_engine_presentation_owner([&](const MultiplayerMatch&){audio->pre_world_tick();});
  std::array<float,sound_class_count> gains{};audio->audio_update(1,gains);
  for(unsigned tick=0;tick<1850;++tick) {
    const auto point=interior(*match.source_king_state());Spawn spawn;spawn.position={point[0],point[1],point[2]};
    test::place_player(match,0,spawn);match.step({});
    // Explicit Manual device provider:1600 stereo frames per30Hz game tick.
    for(const auto sample:audio->render_manual(1600)) {
      check(std::isfinite(sample),"Actual score cue emitted nonfinite PCM");peak=std::max(peak,std::abs(double(sample)));
    }
    audio->audio_update(1,gains);
  }
  check(match.finished() && match.state().results_finalized && changes==61 && local_events.size()==3 &&
      network_observations.size()==3 && selected.size()==3,"Actual King scoring did not publish the three original threshold events");
  constexpr std::array<unsigned,3> subtypes{7,9,47};
  for(unsigned i=0;i<3;++i) {
    const auto& event=local_events[i];
    check(event.words==network_observations[i].words && event.data==network_observations[i].data &&
        event.words[0]==0 && event.words[1]==subtypes[i]+unsigned(teams) &&
        event.words[3]==match.player(0).source_datum &&
        event.words[4]==std::uint32_t(std::int32_t(match.player(0).source_membership)),"Actual score publication lost its source prefix/identity");
    const auto& response=audio->assets().responses.general.at(selected[i].response_ordinal);const Bytes row(response);
    check(row.u16(4)==event.words[1] && row.u16(6)==(teams?1:0) &&
        selected[i].sound==row.u32(0x48) && selected[i].caption_key==row.u32(0xc) &&
        selected[i].display_key==row.u32(0x14),"Actual threshold response lost authored audience/sound/text references");
  }
  const auto completed=audio->snapshot();
  check(peak>0 && completed.voices.empty() && completed.completions.size()==3 &&
      completed.source.queue.count==0,"Actual threshold cues did not finish through PCM/device accounting");
  for(const auto& completion:completed.completions)
    check(completion.packet.status==SdlSoundCompletionStatus::Success && completion.packet.pcm_frames>0,
        "Actual score cue fabricated a successful completion");
  if(moving)check(notices>0,"Moving King did not exercise its separate notice boundary");
  std::cout<<"King score audio teams="<<teams<<" moving="<<moving<<" changes="<<changes
      <<" events="<<selected.size()<<" completions="<<completed.completions.size()<<" peak="<<peak<<'\n';
}
}
int main(int argc,char** argv)try {
  check(argc==7,"Supply PC_MAPS SCENARIO AUDIO_XBE MAP SHARED_MAPS UPDATE_XBE");
  const Cache cache(argv[4],argv[5],OriginalData::load_xbe(argv[6]));const auto content=cache.load_content(0,true);
  NativeVistaAnnouncerConfiguration config{argv[1],argv[2],OriginalData::load_xbe(argv[3]),0,30,
      {0x27f,0x1f80},false,0x5821b36d,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}}};
  for(const bool teams:{false,true})for(const bool moving:{false,true})run(cache,content,config,teams,moving);
  std::cout<<"PASS actual native King scores reach original threshold prefixes, authored general responses, native PCM and registered successful completions\n";
  std::cout<<"LIMIT declared debug positions, observed network/hill-notice boundaries and Manual device; captions, normal world handoff/admission and complete playable matches remain open\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
