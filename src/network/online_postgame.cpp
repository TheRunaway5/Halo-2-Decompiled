#include "halo2/online_postgame.hpp"
#include "halo2/vista_pcr_owner_state.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/full_scoreboard.hpp"
#include "halo2/spawn.hpp"
#include <algorithm>
#include <bit>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
VistaLoadingFloatingPoint floating_point() {
  VistaLoadingFloatingPoint fp{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fnstcw %0":"=m"(fp.x87_control_word));fp.mxcsr=_mm_getcsr();
#else
  throw FormatError("ONLINE Vista presentation requires an actual supported x87/SSE context");
#endif
  return fp;
}
// PC4692CC ->4D3045; original native248C90/248F20 comparators.
FullScoreboardRanks ranks(const MultiplayerState& state,bool extended,unsigned count) {
  std::vector<FullScoreboardPlayer> players;
  std::array<FullScoreboardTeam,8> teams{};
  std::array<TeamRespawnPlayer,16> presence{};
  for(unsigned i=0;i<count;++i) {
    const auto &p=state.players[i];
    FullScoreboardPlayer row;row.datum=p.source_player;row.current_unit=p.source_unit;
    row.flags=p.source_flags;row.membership=std::int8_t(p.source_membership);
    row.lives_remaining=p.source_lives_remaining;row.wins=p.wins;row.total_score=p.total_score;
    row.stat8=std::int16_t(p.kills);row.stat_a=std::int16_t(p.deaths);row.stat10=std::int16_t(p.assists);
    players.push_back(row);presence[i]={row.membership,row.flags,0};
  }
  const auto playing=source_team_presence(0,std::span(presence).first(count)).playing;
  if(state.team_mode)for(unsigned i=0;i<8;++i) {
    auto &team=teams[i];const auto &stats=state.team_statistics[i];
    team.retained=bool(state.retained_team_mask&(1u<<i));team.playing=bool(playing&(1u<<i));
    team.score=stats[0];team.total_score=stats[1];team.wins=stats[7];
    team.stat8=stats[2];team.stat_a=stats[3];team.stat10=stats[6];
  }
  FullScoreboardOptions options;options.engine_present=true;options.game_over=true;
  options.team_mode=state.team_mode;options.extended_round_comparison=extended;options.life_limit=state.life_limit;
  return full_scoreboard_ranks(players,options,state.team_mode?std::span<const FullScoreboardTeam>(teams):std::span<const FullScoreboardTeam>{});
}

}
VistaPcrConsumedSnapshot online_postgame_publish(const MultiplayerMatch& match,
    const UiVariant& variant,std::span<const OnlineAdmission> roster) {
  const auto state=match.state();
  if(!state.finished || !state.results_finalized || !state.game_over_complete() ||
      !vista_pcr_ranking_available(match.source_round_lifecycle().flags) || roster.size()<2 || roster.size()>16)
    throw FormatError("ONLINE postgame requires a completed admitted original match");
  VistaPcrPublicationInput input;input.variant=vista_pcr_multiplayer_variant(source_game_variant(variant));
  input.statistics=match.recorded_statistics();input.recording=match.result_recording();
  if(!input.statistics || !input.recording.enabled || !input.recording.frozen)
    throw FormatError("ONLINE postgame requires actual frozen numeric statistics");
  input.current=input.finalized=1;input.engine_present=true;
  const auto &statistics=match.source_match_statistics();
  const auto standings=ranks(state,(std::to_integer<unsigned>(input.variant.bytes[0x48])&0x10)!=0,unsigned(roster.size()));
  unsigned admitted_teams{};
  if(state.team_mode)for(const auto &p:roster) {
    if(p.team<0 || p.team>7)throw FormatError("Invalid admitted ONLINE team");
    admitted_teams|=1u<<p.team;
  }
  if(state.team_mode && admitted_teams!=state.retained_team_mask)
    throw FormatError("ONLINE postgame team presence differs from admission");
  for(unsigned i=0;i<8;++i)if(admitted_teams&(1u<<i)) {
    input.teams[i].occupied=1;input.teams[i].score=statistics.teams[i].fields[1];
    input.teams[i].placement=standings.teams[i]/2;
  }
  for(unsigned i=0;i<16;++i) {
    const auto &p=match.player(i);
    if(i>=roster.size()){if(p.connected)throw FormatError("Unadmitted ONLINE postgame participant");continue;}
    if(!p.connected || roster[i].slot!=i || p.source_datum==null_datum ||
        (p.source_datum&0xffffu)!=i || !p.source_profile_request)
      throw FormatError("ONLINE postgame participant lacks its actual selected profile");
    auto &row=input.players[i];row.occupied=1;row.identity=roster[i].identity;
    row.configuration=native_vista_selected_profile_payload(*p.source_profile_request);
    // The relay admission owns the chosen display name. Retain the selected
    // PC-compatible appearance instead of copying Xbox sanitized species2/3.
    std::array<std::uint8_t,9> appearance{};
    const auto name=native_selected_player_configuration(roster[i].name,appearance,-1);
    for(unsigned j=0;j<64;++j)row.configuration[j]=std::byte(name[j]);
    if(state.team_mode && p.source_membership!=roster[i].team)
      throw FormatError("ONLINE final team differs from admission");
    row.configuration[0x7c]=std::byte(state.team_mode?roster[i].team:0xff);
    row.placement=standings.players[i]/2;
    row.score=vista_pcr_accumulated_player_score(statistics,p.source_datum);
  }
  return vista_pcr_publish_consumed_snapshot(input);
}
void online_postgame_validate(const MultiplayerState& state,const UiVariant& variant,
    std::span<const OnlineAdmission> roster,std::span<const std::byte> raw) {
  auto check=[](bool ok){if(!ok)throw FormatError("ONLINE match results differ from the final admitted game state");};
  check(raw.size()==vista_pcr_source_size && roster.size()>=2 && roster.size()<=16);
  check(state.finished && state.results_finalized && state.game_over_complete());
  check(raw[0]==std::byte{1} && raw[1]==std::byte{1} && raw[0x10]==std::byte{1} && raw[0x11]==std::byte{1} && raw[0x13]==std::byte(state.team_mode));
  const auto expected=vista_pcr_multiplayer_variant(source_game_variant(variant));
  check(std::equal(expected.bytes.begin(),expected.bytes.end(),raw.begin()+0x20));
  auto word=[&](std::size_t at){return std::uint16_t(std::to_integer<unsigned>(raw[at])|(std::to_integer<unsigned>(raw[at+1])<<8));};
  const bool extended=(std::to_integer<unsigned>(expected.bytes[0x48])&0x10)!=0;
  const auto standings=ranks(state,extended,unsigned(roster.size()));
  unsigned admitted_teams{};
  for(unsigned i=0;i<16;++i) {
    const auto at=0x370+i*0x94;const auto &p=state.players[i];
    if(i>=roster.size()){check(!p.connected && raw[at]==std::byte{});continue;}
    check(p.connected && roster[i].slot==i && raw[at]==std::byte{1});
    check(std::equal(roster[i].identity.begin(),roster[i].identity.end(),raw.begin()+at+2));
    std::array<std::uint8_t,9> appearance{};const auto name=native_selected_player_configuration(roster[i].name,appearance,-1);
    for(unsigned j=0;j<64;++j)check(raw[at+12+j]==std::byte(name[j]));
    if(state.team_mode) {
      check(roster[i].team>=0 && roster[i].team<8 && p.source_membership==roster[i].team);
      admitted_teams|=1u<<roster[i].team;
    }
    check(raw[at+0x88]==std::byte(state.team_mode?roster[i].team:0xff));
    check(std::bit_cast<std::int16_t>(word(at+0x92))==p.total_score);
    check(std::to_integer<unsigned>(raw[at+0x90])==standings.players[i]/2);
    const std::array<std::pair<unsigned,int>,4> stats{{{7,p.kills},{9,p.deaths},{8,p.assists},{11,p.suicides}}};
    for(auto [index,value]:stats)check(value>=0 && value<=32767 && int(word(0xe30+2*(i*0x1b5+index))&0x7fff)==value);
  }
  check(!state.team_mode || admitted_teams==state.retained_team_mask);
  for(unsigned i=0;i<16;++i) {
    const auto at=0xcb0+i*0x18;const bool present=i<8 && (admitted_teams&(1u<<i));
    check(raw[at]==std::byte(present));
    if(present) {
      check(std::to_integer<unsigned>(raw[at+1])==standings.teams[i]/2);
      check(std::bit_cast<std::int16_t>(word(at+2))==state.team_statistics[i][1]);
    }
  }
}
std::shared_ptr<const VistaPcrScreenOwner> online_postgame_screen(
    FrontendSession& frontend,std::shared_ptr<OnlineMatchSession> session,std::function<void()> done) {
  const auto bytes=session->postgame();
  if(session->phase()!=OnlineMatchSession::Phase::Postgame || bytes.size()!=vista_pcr_source_size)
    throw FormatError("ONLINE report has no acknowledged completed publication");
  VistaPcrConsumedSnapshot source;std::copy(bytes.begin(),bytes.end(),source.source.begin());
  auto pcr=std::make_shared<NativeVistaPcr>(std::move(source),session->roster(),frontend.content(),floating_point());
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->results=pcr->results();owner->rows=pcr->rows(16);
  owner->online=true;
  owner->appearance=[pcr,&frontend](unsigned row){return pcr->appearance(row,16,frontend.state().frame().context);};
  auto slot=[session](const std::array<std::byte,8>& identity) {
    for(const auto &p:session->roster())if(p.identity==identity)return std::uint32_t(p.slot);
    return 0xffffffffu;
  };
  owner->voice_update=[session](bool focused){session->update_postgame_voice(focused);};
  owner->voice=[pcr,session,slot](unsigned row) {
    VistaPcrVoiceServices services;services.identity_slot=slot;
    services.enabled=[session](std::uint32_t id)->std::uint8_t {
      const auto audio=session->retained_voice();
      return audio && session->postgame_voice_enabled(id) && !(audio->snapshot().muted&(1u<<id));
    };
    services.talking=[session](std::uint32_t id)->std::uint8_t {
      const auto audio=session->retained_voice();if(!audio)return 0;
      const auto snapshot=audio->snapshot();return id==session->self()?snapshot.transmitting:snapshot.talking.at(id);
    };
    return vista_pcr_voice(*pcr->results(),row,0,services);
  };
  owner->voice_control=[pcr,session,slot](unsigned action,unsigned pane,unsigned row) {
    const auto audio=session->retained_voice();if(!audio)return;
    const auto snapshot=audio->snapshot();
    if(action==0)audio->enable(!snapshot.enabled);
    else if(action==1)audio->microphone_muted(!snapshot.microphone_muted);
    else if(action==2 && pane && row<pcr->results()->player_count) {
      std::array<std::byte,8> identity;std::copy_n(pcr->results()->players[row].begin()+0x100,8,identity.begin());
      const auto selected=slot(identity);
      if(selected<session->roster().size() && selected!=session->self())audio->mute(selected,!(snapshot.muted&(1u<<selected)));
    }
  };
  // Native solo queue owns local Done navigation. There is no retail party or
  // Live details service behind these room identities.
  owner->input.host=[]()->std::uint8_t{return 1;};owner->input.details_available=[]()->std::uint8_t{return 0;};
  owner->input.continue_session=[session,done=std::move(done)]{session->close_postgame();done();};
  owner->select_row=[pcr](unsigned row){pcr->select_row(row);};
  owner->elapsed=[&frontend](std::uint32_t creation){return frontend.ui_clock()-creation;};
  owner->floating_point=pcr->floating_point();owner->controller=0;owner->creation_clock=frontend.ui_clock();
  const auto focus=vista_pcr_initial_focus(*owner->results);owner->pane=focus.pane;owner->selected_row=focus.selected_row;
  pcr->select_row(focus.selected_row);return owner;
}
} // namespace halo2
