#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include <bit>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourceResultEvent& event,unsigned offset) {
  std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(event[offset+i])<<(i*8);return value;
}
std::array<float,3> interior(const SourceKingState& hill) {
  SourceKingPlayer player;player.datum=0;player.current_unit=0;
  for(const float z:{hill.center[2],(hill.minimum_z+hill.maximum_z)*.5f})
    for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y) {
      const std::array<float,3> point{hill.center[0]+hill.radius*float(x)*.1f,hill.center[1]+hill.radius*float(y)*.1f,z};
      if(source_king_contains(hill,player,[&](std::uint32_t){return point;}))return point;
    }
  throw FormatError("Authored hill has no sampled original-contains interior");
}
void run(const Cache& cache,const LevelContent& content,bool teams,bool moving) {
  auto rules=king_rules({"King recording",3,{{91,int(teams)},{40,moving?1:0},{2,2},{0x59,5}}},true);
  rules.weapon=cache.slayer_starting_weapon(5,4);rules.time_ticks=0;
  MultiplayerKingServices observers;unsigned changes{},notices{};
  // Presentation is observed, but event recording has no supplied callback.
  observers.score_changed=[&](const MultiplayerMatch&,std::uint32_t,std::int32_t){++changes;};
  observers.notification=[&](const MultiplayerMatch&,const SourceKingNotice&){++notices;};
  std::uint32_t now=1000;unsigned queries{};
  // Declared platform time32 inputs isolate scheduling from test wall time.
  // Production's omitted provider reads the actual system wall clock.
  auto match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules,observers,[&]{++queries;return now;});
  check(queries==1 && match->recorded_events() && match->recorded_events()->last==1000 &&
    match->recorded_events()->start==0 && !match->result_recording_started(),"Native event startup sampled a fabricated start time");
  for(unsigned i=0;i<2;++i)match->join("Actual King recorder "+std::to_string(i),teams?std::int8_t(i):-1);
  std::array<Controls,multiplayer_capacity> controls{};unsigned events{};auto previous_index=0u;
  for(unsigned tick=0;tick<100;++tick) {
    now=1000+tick;const auto point=interior(*match->source_king_state());Spawn spawn;spawn.position={point[0],point[1],point[2]};
    for(unsigned i=0;i<2;++i)test::place_player(*match,i,spawn);
    const auto prior_queries=queries;const auto prior_frozen=match->result_recording().frozen;
    match->step(controls);const auto& bank=*match->recorded_events();
    check(match->result_recording_started() && bank.start==1000,"Native first active tick lost the retained start producer");
    if(tick==0)check(queries==2,"Native first-tick timestamp was not sampled after pre-world engine work");
    if(prior_frozen)check(queries==prior_queries,"Frozen native King queried its recording wall clock");
    if(bank.index!=previous_index) {
      check(!teams && bank.index==previous_index+1,"Team King invented an FFA event or ring advanced incorrectly");
      const auto& event=bank.events[previous_index];check(event[0]==2 && event[1]==0 && event[2]==0xff && event[3]==0,"Native event header differs");
      for(unsigned i=0;i<3;++i)check(word(event,4+i*4)==std::bit_cast<std::uint32_t>(point[i]),"Native recording lost pre-world unit coordinates");
      check(word(event,0x10)==null_datum && word(event,0x14)==4 && word(event,0x20)==tick,"Native King event arguments/time differ");
      for(unsigned i=0x18;i<0x20;++i)check(event[i]==0,"Native recording invented reserved event bytes");
      ++events;previous_index=bank.index;
    }
  }
  check(match->finished() && match->state().results_finalized && match->result_recording().frozen && changes>0,"Native recorded King did not finish and freeze");
  check(events==(teams?0u:2u),"Actual King scoring did not produce the original event count");
  if(moving)check(notices>0,"Moving King did not deliver its notification boundary");
  const auto state=match->state();const auto recording=match->result_recording();const auto expected=*match->recorded_events();
  const auto outcome=retain_multiplayer_outcome(MultiplayerExitReason::headless_limit,MultiplayerOutcomeAuthority::local,&state,
    {&match->source_match_statistics(),match->recorded_statistics(),&recording,match->recorded_events()});
  match.reset();check(outcome.authoritative_banks && outcome.authoritative_banks->recorded_events &&
    std::memcmp(&*outcome.authoritative_banks->recorded_events,&expected,sizeof(expected))==0,"Native teardown discarded or changed the event bank");
  std::cout<<"King recorded teams="<<teams<<" moving="<<moving<<" events="<<events<<" platform_queries="<<queries<<'\n';
}
}
int main(int argc,char** argv)try {
  check(argc==4,"Supply original MAP SHARED_MAPS UPDATE_XBE");
  const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));const auto content=cache.load_content(0,true);
  for(const bool teams:{false,true})for(const bool moving:{false,true})run(cache,content,teams,moving);
  std::cout<<"PASS actual King FFA events, wall-clock services, pre-world pose, timestamp lifecycle, team absence, game-over freeze and deep runtime retention\n";
  std::cout<<"LIMIT declared wall-clock/debug positions and observed presentation; parent transforms, full retained identity/machine publication and normal King admission remain separate\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
