#include "halo2/multiplayer_outcome.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <memory>
#include <type_traits>

using namespace halo2;
namespace {
void check(bool condition,const char* message) {
  if(!condition)throw FormatError(message);
}
template<class F> void rejects(F&& operation,const char* message) {
  bool rejected=false;
  try { operation(); } catch(const FormatError&) { rejected=true; }
  check(rejected,message);
}
template<class T> void pattern(T& bank,unsigned salt) {
  static_assert(std::is_trivially_copyable_v<T>);
  auto* bytes=reinterpret_cast<unsigned char*>(&bank);
  for(std::size_t i=0;i<sizeof(T);++i)
    bytes[i]=static_cast<unsigned char>((i*73u+(i>>3)*19u+salt)&255u);
}
template<class T> bool same_bank(const T& first,const T& second) {
  static_assert(std::is_trivially_copyable_v<T>);
  return std::memcmp(&first,&second,sizeof(T))==0;
}
MultiplayerState selected_state() {
  MultiplayerState state;
  state.tick=0xfedcba98u;
  state.finished=true;
  state.results_finalized=true;
  // Retention must not complete this countdown or infer an exit from it.
  state.game_over_countdown=17;
  state.game_over_fade_started=true;
  state.winner=15;state.winner_team=6;
  state.variant_name=std::string(180,'V')+" selected variant";
  state.team_mode=true;state.retained_team_mask=0xa5;
  for(unsigned i=0;i<state.team_scores.size();++i) {
    state.team_scores[i]=std::int16_t(31+i);
    state.team_total_scores[i]=std::int16_t(-100-int(i));
    state.team_wins[i]=std::int16_t(3+i);
  }
  for(unsigned i=0;i<state.players.size();++i) {
    auto& player=state.players[i];
    player.connected=i%3!=0;player.dead=i%2!=0;
    player.name=std::string(150,char('A'+i))+" participant "+std::to_string(i);
    player.position={float(i),float(i+10),float(i+20)};
    player.health=std::bit_cast<float>(0x80000000u); // Negative zero is data.
    player.pitch=std::bit_cast<float>(0x7fc12345u+i); // Preserve raw payload.
    player.source_player=0x12340000u+i;
    player.source_membership=std::int16_t(int(i)-1);
    player.source_flags=std::uint16_t(0x8100u+i);
    if(i%2==0)pattern(player.source_configuration.emplace(),i+47);
    for(unsigned j=0;j<player.grenade_action.source.size();++j)
      player.grenade_action.source[j]=std::byte((i*7+j*13)&255u);
  }
  for(unsigned i=0;i<3;++i) {
    PropRemoval removal;
    removal.handle=0x67890000u+i;removal.placement=i+11;
    removal.definition=i+23;removal.effect=0x33330000u+i;
    removal.effect_position={float(i),2.f,3.f};
    removal.event_tick=0x1234567800000000ull+i;
    state.prop_removals.push_back(removal);
    NativeGrenadeProjectile projectile;
    projectile.datum=0x43210000u+i;projectile.definition=0xf0000000u+i;
    projectile.position={1.f,2.f,float(i)};
    projectile.native_owner=int(i)-2;projectile.timer=float(i)+.5f;
    state.grenade_projectiles.push_back(projectile);
    NativeGrenadeDetonation detonation;
    detonation.serial=0x123456789abcdef0ull+i;
    detonation.tick=0x9876543210000000ull+i;
    detonation.projectile=projectile.datum;
    detonation.position=projectile.position;
    state.grenade_detonations.push_back(detonation);
  }
  return state;
}
void same_state(const MultiplayerState& actual,const MultiplayerState& expected) {
  check(actual.tick==expected.tick && actual.finished==expected.finished &&
      actual.results_finalized==expected.results_finalized &&
      actual.game_over_countdown==expected.game_over_countdown &&
      actual.game_over_fade_started==expected.game_over_fade_started,
      "Retention changed the actual completion/counter observation");
  check(actual.variant_name==expected.variant_name && actual.winner==expected.winner &&
      actual.winner_team==expected.winner_team && actual.team_mode==expected.team_mode &&
      actual.retained_team_mask==expected.retained_team_mask &&
      actual.team_scores==expected.team_scores && actual.team_total_scores==expected.team_total_scores &&
      actual.team_wins==expected.team_wins,"Retained selected match/team data changed");
  for(unsigned i=0;i<expected.players.size();++i) {
    const auto& player=actual.players[i];const auto& source=expected.players[i];
    check(player.connected==source.connected && player.dead==source.dead &&
        player.name==source.name && player.position.x==source.position.x &&
        player.position.y==source.position.y && player.position.z==source.position.z &&
        std::bit_cast<std::uint32_t>(player.health)==std::bit_cast<std::uint32_t>(source.health) &&
        std::bit_cast<std::uint32_t>(player.pitch)==std::bit_cast<std::uint32_t>(source.pitch) &&
        player.source_player==source.source_player && player.source_membership==source.source_membership &&
        player.source_flags==source.source_flags && player.source_configuration==source.source_configuration &&
        player.grenade_action.source==source.grenade_action.source,
        "Retained participant data, optional configuration or raw source action changed");
  }
  check(actual.prop_removals==expected.prop_removals,"Retained scene tombstones changed");
  check(actual.grenade_projectiles.size()==expected.grenade_projectiles.size() &&
      actual.grenade_detonations.size()==expected.grenade_detonations.size(),
      "Retained nested vector ownership was lost");
  for(unsigned i=0;i<expected.grenade_projectiles.size();++i) {
    const auto& a=actual.grenade_projectiles[i];const auto& b=expected.grenade_projectiles[i];
    check(a.datum==b.datum && a.definition==b.definition && a.position.z==b.position.z &&
        a.native_owner==b.native_owner && a.timer==b.timer,"Retained projectile changed");
    const auto& da=actual.grenade_detonations[i];const auto& db=expected.grenade_detonations[i];
    check(da.serial==db.serial && da.tick==db.tick && da.projectile==db.projectile &&
        da.position.z==db.position.z,"Retained detonation changed");
  }
}
struct Sources {
  MultiplayerState state=selected_state();
  SourceGameEndStats match;
  SourceResultStatistics recorded;
  SourceResultRecording recording;
  SourceResultEventState events;
  Sources() {pattern(match,39);pattern(recorded,81);pattern(events,113);recording={0x95,0xad};}
};
void ownership(MultiplayerExitReason reason,MultiplayerOutcomeAuthority authority,bool recorded) {
  auto source=std::make_unique<Sources>();
  const auto expected_state=source->state;
  const auto expected_match=source->match;
  const auto expected_recorded=source->recorded;
  const auto expected_recording=source->recording;
  const auto expected_events=source->events;
  const auto retained=retain_multiplayer_outcome(reason,authority,&source->state,
      {&source->match,recorded?&source->recorded:nullptr,&source->recording,recorded?&source->events:nullptr});
  check(retained.reason==reason && retained.authority==authority && retained.state && retained.authoritative_banks,
      "Retained exit reason/authority or actual bank presence changed");
  // Destroy/mutate every original owner before examining the retained result.
  source->state=MultiplayerState{};
  pattern(source->match,11);pattern(source->recorded,12);pattern(source->events,13);source->recording={0,0};
  source.reset();
  same_state(*retained.state,expected_state);
  const auto& banks=*retained.authoritative_banks;
  check(same_bank(banks.match_statistics,expected_match) &&
      same_bank(banks.result_recording,expected_recording),
      "Retention lost opaque match bytes or normalized raw recording bytes");
  check(banks.recorded_statistics.has_value()==recorded,
      "Absent recorded bank was manufactured or present recorded bank was discarded");
  if(recorded)check(same_bank(*banks.recorded_statistics,expected_recorded),
      "Retention changed opaque retained statistics bytes");
  check(banks.recorded_events.has_value()==recorded,"Absent event bank was manufactured or present event bank discarded");
  if(recorded)check(same_bank(*banks.recorded_events,expected_events),"Retention changed complete event ring/timestamps/index");
  auto copied=retained;
  copied.state->players[2].name="changed";
  copied.state->prop_removals.clear();
  copied.authoritative_banks->match_statistics.header^=0xffffffffu;
  if(recorded)copied.authoritative_banks->recorded_statistics->players[0][436]^=0xffff;
  if(recorded)copied.authoritative_banks->recorded_events->events[999][35]^=0xff;
  same_state(*retained.state,expected_state);
  check(same_bank(retained.authoritative_banks->match_statistics,expected_match),
      "Copying an outcome aliased its match bank");
  if(recorded)check(same_bank(*retained.authoritative_banks->recorded_statistics,expected_recorded),
      "Copying an outcome aliased its retained statistics bank");
  if(recorded)check(same_bank(*retained.authoritative_banks->recorded_events,expected_events),"Copying outcome aliased its event ring");
}
}
int main()try {
  constexpr std::array reasons{MultiplayerExitReason::headless_limit,MultiplayerExitReason::frame_limit,
      MultiplayerExitReason::captured_tick,MultiplayerExitReason::window_quit,MultiplayerExitReason::escape,
      MultiplayerExitReason::session_world_close};
  for(const auto authority:{MultiplayerOutcomeAuthority::local,MultiplayerOutcomeAuthority::host})
    for(const auto reason:reasons)
      for(bool present:{false,true})ownership(reason,authority,present);
  Sources source;
  for(const auto reason:reasons) {
    auto state=source.state;
    const auto retained=retain_multiplayer_outcome(reason,MultiplayerOutcomeAuthority::client_snapshot,&state);
    state=MultiplayerState{};
    check(retained.authority==MultiplayerOutcomeAuthority::client_snapshot && retained.reason==reason && retained.state &&
        !retained.authoritative_banks,"Client snapshot was represented as authoritative local results");
    same_state(*retained.state,source.state);
  }
  const auto pending_client=retain_multiplayer_outcome(MultiplayerExitReason::window_quit,
      MultiplayerOutcomeAuthority::client_snapshot,nullptr);
  check(pending_client.reason==MultiplayerExitReason::window_quit &&
      pending_client.authority==MultiplayerOutcomeAuthority::client_snapshot &&
      !pending_client.state && !pending_client.authoritative_banks,
      "Client exit before its first snapshot manufactured a local placeholder or source banks");
  // Any borrowed local bank in a client result is a caller ownership error,
  // even if the received snapshot says its remote results were finalized.
  for(unsigned mask=1;mask<16;++mask)rejects([&] {
    retain_multiplayer_outcome(MultiplayerExitReason::frame_limit,MultiplayerOutcomeAuthority::client_snapshot,
        &source.state,{mask&1?&source.match:nullptr,mask&2?&source.recorded:nullptr,mask&4?&source.recording:nullptr,mask&8?&source.events:nullptr});
  },"Client retained a local authoritative bank");
  for(unsigned mask=1;mask<16;++mask)rejects([&] {
    retain_multiplayer_outcome(MultiplayerExitReason::window_quit,MultiplayerOutcomeAuthority::client_snapshot,
        nullptr,{mask&1?&source.match:nullptr,mask&2?&source.recorded:nullptr,mask&4?&source.recording:nullptr,mask&8?&source.events:nullptr});
  },"Client with no snapshot retained a local authoritative bank");
  for(const auto authority:{MultiplayerOutcomeAuthority::local,MultiplayerOutcomeAuthority::host})
    for(unsigned mask=0;mask<4;++mask)if(mask!=3)rejects([&] {
      retain_multiplayer_outcome(MultiplayerExitReason::frame_limit,authority,&source.state,
          {mask&1?&source.match:nullptr,&source.recorded,mask&2?&source.recording:nullptr});
    },"Authoritative retention accepted missing actual owner banks");
  for(const auto authority:{MultiplayerOutcomeAuthority::local,MultiplayerOutcomeAuthority::host})
    rejects([&] {
      retain_multiplayer_outcome(MultiplayerExitReason::headless_limit,authority,nullptr,
          {&source.match,&source.recorded,&source.recording});
    },"Authoritative retention accepted an absent state");
  rejects([&]{retain_multiplayer_outcome(static_cast<MultiplayerExitReason>(99),
      MultiplayerOutcomeAuthority::client_snapshot,&source.state);},"Unknown native exit was accepted");
  rejects([&]{retain_multiplayer_outcome(MultiplayerExitReason::escape,
      static_cast<MultiplayerOutcomeAuthority>(99),&source.state);},"Unknown native authority was accepted");
  check(same_bank(source.match,Sources{}.match) && same_bank(source.recorded,Sources{}.recorded) &&
      source.recording.enabled==0x95 && source.recording.frozen==0xad,
      "Pure retention or rejection mutated an input source bank");
  std::cout<<"PASS native outcome ownership, complete opaque banks, absent snapshots, optional banks, exit reasons and client authority boundaries\n";
  return 0;
}catch(const std::exception& error) {
  std::cerr<<error.what()<<'\n';return 1;
}
