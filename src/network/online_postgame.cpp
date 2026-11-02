#include "halo2/online_postgame.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <type_traits>
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
  for(unsigned i=0;i<16;++i) {
    const auto &p=state.players[i];if(!p.connected)continue;
    FullScoreboardPlayer row;row.datum=p.source_player;row.current_unit=p.source_unit;
    row.flags=p.source_flags;row.membership=std::int8_t(p.source_membership);
    row.lives_remaining=p.source_lives_remaining;row.wins=p.wins;row.total_score=p.total_score;
    row.stat8=std::int16_t(p.kills);row.stat_a=std::int16_t(p.deaths);row.stat10=std::int16_t(p.assists);
    players.push_back(row);presence[i]={row.membership,row.flags,0};
  }
  const auto playing=source_team_presence(0,std::span(presence)).playing;
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

namespace bridge = online_protocol_bridge;
// Native observation/profile/ranking/numeric-publication IO only. Rust owns
// all report admission, roster/team checks and publication-plan decisions.
struct NativeResultAdapter final : bridge::NativeResultIo {
  const MultiplayerMatch *match{};
  const UiVariant &variant;
  std::span<const OnlineAdmission> roster;
  std::optional<MultiplayerState> snapshot;
  const MultiplayerState *state{};
  mutable const SourceResultStatistics *statistics{};
  mutable SourceResultRecording recording{};
  mutable const SourceGameEndStats *game_end{};
  mutable std::array<const MultiplayerPlayer *,16> live_players{};
  mutable std::exception_ptr exception;
  mutable VistaPcrConsumedSnapshot output;
  NativeResultAdapter(const MultiplayerMatch &m, const UiVariant &v,
                      std::span<const OnlineAdmission> r)
      : match(&m), variant(v), roster(r), snapshot(m.state()),
        state(&*snapshot) {}
  NativeResultAdapter(const MultiplayerState &s, const UiVariant &v,
                      std::span<const OnlineAdmission> r)
      : variant(v), roster(r), state(&s) {}
  template <class F> decltype(auto) capture(F &&f) const {
    try {
      return f();
    } catch (...) {
      exception = std::current_exception();
      throw;
    }
  }
  template <class F> decltype(auto) invoke(F &&f) const {
    try {
      return f();
    } catch (const std::exception &e) {
      if (exception)
        std::rethrow_exception(exception);
      throw FormatError(e.what());
    }
  }
  template <class Range> static rust::Vec<std::uint8_t> bytes(const Range &r) {
    rust::Vec<std::uint8_t> out;
    out.reserve(r.size());
    for (auto b : r) {
      if constexpr (std::is_same_v<decltype(b), std::byte>)
        out.push_back(std::to_integer<std::uint8_t>(b));
      else
        out.push_back(std::uint8_t(b));
    }
    return out;
  }
  template <std::size_t N>
  static void copy(const rust::Vec<std::uint8_t> &src,
                   std::array<std::byte, N> &dst) {
    if (src.size() != N)
      throw std::logic_error("Invalid native result ABI");
    for (std::size_t i = 0; i < N; ++i)
      dst[i] = std::byte(src[i]);
  }
  const MultiplayerMatch &actual_match() const {
    if (!match)
      throw std::logic_error("Native result view has no live match");
    return *match;
  }
  bridge::NativeResultState result_state() const override {
    return capture([&] {
      bridge::NativeResultState out{};
      out.finished = state->finished;
      out.results_finalized = state->results_finalized;
      out.game_over_countdown = state->game_over_countdown;
      out.team_mode = state->team_mode;
      out.retained_teams = state->retained_team_mask;
      for (const auto &p : state->players)
        out.players.push_back(bridge::NativeResultPlayer{
            p.connected, p.source_membership, p.total_score, p.kills, p.deaths,
            p.assists, p.suicides});
      for (const auto &team : state->team_statistics)
        out.team_scores.push_back(team[1]);
      out.accuracy_present = state->result_accuracy.has_value();
      if (state->result_accuracy)
        for (const auto &player : state->result_accuracy->players)
          for (const auto &weapon : player)
            for (auto value : weapon)
              out.accuracy.push_back(value);
      return out;
    });
  }
  rust::Vec<bridge::NativeResultAdmission> result_roster() const override {
    return capture([&] {
      rust::Vec<bridge::NativeResultAdmission> out;
      out.reserve(roster.size());
      for (const auto &p : roster)
        out.push_back(
            bridge::NativeResultAdmission{p.slot, p.team, bytes(p.identity)});
      return out;
    });
  }
  std::uint32_t result_round_flags() const override {
    return capture(
        [&] { return actual_match().source_round_lifecycle().flags; });
  }
  rust::Vec<std::uint8_t> result_variant() const override {
    return capture([&] {
      return bytes(
          vista_pcr_multiplayer_variant(source_game_variant(variant)).bytes);
    });
  }
  bridge::NativeResultRecording result_recording() const override {
    return capture([&] {
      statistics = actual_match().recorded_statistics();
      recording = actual_match().result_recording();
      return bridge::NativeResultRecording{statistics != nullptr,
                                           recording.enabled, recording.frozen};
    });
  }
  bridge::NativeResultRanks result_ranks(bool extended,
                                         std::uint32_t count) const override {
    return capture([&] {
      if (match)
        game_end = &match->source_match_statistics();
      const auto values = ranks(*state, extended, count);
      return bridge::NativeResultRanks{bytes(values.players),
                                       bytes(values.teams)};
    });
  }
  bool result_changing_teams() const override {
    return capture([&] {
      return source_game_variant_get(source_game_variant(variant), 22) != 0;
    });
  }
  bridge::NativeResultLivePlayer
  result_live_player(std::uint32_t slot) const override {
    return capture([&] {
      const auto &p = actual_match().player(slot);
      live_players.at(slot)=&p;
      return bridge::NativeResultLivePlayer{
          p.connected, p.source_datum, p.source_profile_request.has_value(),
          p.source_membership};
    });
  }
  rust::Vec<std::uint8_t>
  result_configuration(std::uint32_t slot) const override {
    return capture([&] {
      const auto *p=live_players.at(slot);
      if(!p)throw std::logic_error("Native result player was not observed");
      return bytes(native_vista_selected_profile_payload(p->source_profile_request.value()));
    });
  }
  rust::Vec<std::uint8_t> result_name(std::uint32_t index) const override {
    return capture([&] {
      std::array<std::uint8_t, 9> appearance{};
      const auto value = native_selected_player_configuration(
          roster[index].name, appearance, -1);
      return bytes(std::span(value).first(64));
    });
  }
  std::int32_t result_team_score(std::uint32_t team) const override {
    return capture([&] {
      if (!game_end)
        throw std::logic_error(
            "Native result game statistics were not observed");
      return std::int32_t(game_end->teams.at(team).fields[1]);
    });
  }
  std::int16_t result_accumulated_score(std::uint32_t datum) const override {
    return capture([&] {
      if (!game_end)
        throw std::logic_error(
            "Native result game statistics were not observed");
      return vista_pcr_accumulated_player_score(*game_end, datum);
    });
  }
  void
  result_write(const bridge::NativeResultPublication &plan) const override {
    capture([&] {
      if (plan.players.size() != 16 || plan.teams.size() != 16)
        throw std::logic_error("Invalid native result ABI");
      VistaPcrPublicationInput input;
      copy(plan.variant, input.variant.bytes);
      input.statistics = statistics;
      input.recording = recording;
      input.current = plan.current;
      input.finalized = plan.finalized;
      input.engine_present = plan.engine_present;
      for (unsigned i = 0; i < 16; ++i) {
        const auto &p = plan.players[i];
        auto &row = input.players[i];
        row.occupied = p.occupied;
        copy(p.identity, row.identity);
        copy(p.configuration, row.configuration);
        row.score = p.score;
        row.placement = p.placement;
        const auto &t = plan.teams[i];
        input.teams[i] = {t.occupied, t.score, t.placement};
      }
      output = vista_pcr_publish_consumed_snapshot(input);
    });
  }
};

} // namespace
VistaPcrConsumedSnapshot
online_postgame_publish(const MultiplayerMatch &match, const UiVariant &variant,
                        std::span<const OnlineAdmission> roster) {
  NativeResultAdapter io(match, variant, roster);
  io.invoke([&] { bridge::native_result_publish(io); });
  return std::move(io.output);
}
void online_postgame_validate(const MultiplayerState &state,
                              const UiVariant &variant,
                              std::span<const OnlineAdmission> roster,
                              std::span<const std::byte> raw) {
  NativeResultAdapter io(state, variant, roster);
  io.invoke([&] {
    bridge::native_result_validate(
        io, {reinterpret_cast<const std::uint8_t *>(raw.data()), raw.size()});
  });
}
std::shared_ptr<const VistaPcrScreenOwner> online_postgame_screen(
    FrontendSession& frontend,std::shared_ptr<OnlineMatchSession> session,std::function<void()> done) {
  const auto bytes=session->postgame();
  if(session->phase()!=OnlineMatchSession::Phase::Postgame || bytes.size()!=vista_pcr_source_size)
    throw FormatError("ONLINE report has no acknowledged completed publication");
  VistaPcrConsumedSnapshot source;std::copy(bytes.begin(),bytes.end(),source.source.begin());
  const auto weak=std::weak_ptr<OnlineMatchSession>(session);
  auto pcr=std::make_shared<NativeVistaPcr>(std::move(source),session->roster(),frontend.content(),floating_point(),[weak]()->std::optional<std::uint16_t>{
    if(const auto active=weak.lock())return active->postgame_presence();
    return {}; // PCR185 after Done has no authenticated room membership.
  });
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->results=pcr->results();owner->rows=pcr->rows(16);
  owner->online=true;
  owner->result_recorded=session->result_recorded();
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
  // ONLINE has no per-machine session host: a solo player or the party
  // leader owns Done, and a follower's Done asks with dialog98 while the
  // leader's Done brings it along. There is no Live details service behind
  // these room identities.
  owner->input.host=[&frontend]()->std::uint8_t{return frontend.state().online_postgame_host();};
  owner->input.details_available=[]()->std::uint8_t{return 0;};
  owner->input.continue_session=[session,done=std::move(done)]{session->close_postgame();done();};
  owner->input.dialog=[&frontend](const std::array<std::uint32_t,8>& request){frontend.state().vista_pcr_dialog(request);};
  owner->leave_session=[session]{session->close_postgame();};
  owner->select_row=[pcr](unsigned row){pcr->select_row(row);};
  owner->elapsed=[&frontend](std::uint32_t creation){return frontend.ui_clock()-creation;};
  owner->floating_point=pcr->floating_point();owner->controller=0;owner->creation_clock=frontend.ui_clock();
  const auto focus=vista_pcr_initial_focus(*owner->results);owner->pane=focus.pane;owner->selected_row=focus.selected_row;
  // TU1.5 2510AA ->233395 reopens the retained results as PCR185. Build
  // this owner from the copied publication; no match/voice callback survives
  // in lobby history after PCR16 Done closes the completed transport.
  auto review=std::make_shared<VistaPcrScreenOwner>();
  review->online=true;review->result_recorded=owner->result_recorded;review->results=pcr->results();review->rows=pcr->rows(185);
  review->appearance=[pcr,&frontend](unsigned row){return pcr->appearance(row,185,frontend.state().frame().context);};
  review->select_row=[pcr](unsigned row){pcr->select_row(row);};
  review->input.host=[]()->std::uint8_t{return 1;};
  review->input.details_available=[]()->std::uint8_t{return 0;};
  review->input.close=[&frontend](unsigned){frontend.state().close_online_prior_stats();};
  review->elapsed=[&frontend](std::uint32_t creation){return frontend.ui_clock()-creation;};
  review->floating_point=pcr->floating_point();review->controller=owner->controller;
  review->pane=focus.pane;review->selected_row=focus.selected_row;
  owner->lobby_review=std::move(review);
  pcr->select_row(focus.selected_row);return owner;
}
std::shared_ptr<const VistaPcrScreenOwner> online_terminal_postgame_screen(
    FrontendSession &frontend,const MultiplayerState &completed,VistaPcrConsumedSnapshot source,
    std::vector<OnlineAdmission> roster,std::function<void()> done) {
  if(!completed.finished||!completed.results_finalized||!completed.game_over_complete())
    throw FormatError("Terminal ONLINE report requires its actual completed source state");
  std::uint16_t present{};for(const auto &p:roster) {
    if(p.slot>=16||!completed.players[p.slot].connected)throw FormatError("Terminal ONLINE report lost an admitted source result row");
    if(!(completed.players[p.slot].source_flags&2u))present|=std::uint16_t(1u<<p.slot);
  }
  auto presence=std::make_shared<std::optional<std::uint16_t>>(present);
  auto pcr=std::make_shared<NativeVistaPcr>(std::move(source),roster,frontend.content(),floating_point(),
      [presence]{return *presence;});
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->online=true;owner->result_recorded=false;owner->results=pcr->results();owner->rows=pcr->rows(16);
  owner->appearance=[pcr,&frontend](unsigned row){return pcr->appearance(row,16,frontend.state().frame().context);};
  owner->voice=[](unsigned){return VistaPcrVoicePresentation{};};
  // No live session is represented here. The remaining physical owner can
  // close its own report; party membership remains with the real account API.
  owner->input.host=[]()->std::uint8_t{return 1;};
  owner->input.details_available=[]()->std::uint8_t{return 0;};
  owner->input.continue_session=[presence,done=std::move(done)]{presence->reset();done();};
  owner->input.dialog=[&frontend](const std::array<std::uint32_t,8>& request){frontend.state().vista_pcr_dialog(request);};
  owner->select_row=[pcr](unsigned row){pcr->select_row(row);};
  owner->elapsed=[&frontend](std::uint32_t creation){return frontend.ui_clock()-creation;};
  owner->floating_point=pcr->floating_point();owner->controller=0;owner->creation_clock=frontend.ui_clock();
  const auto focus=vista_pcr_initial_focus(*owner->results);owner->pane=focus.pane;owner->selected_row=focus.selected_row;
  auto review=std::make_shared<VistaPcrScreenOwner>();review->online=true;review->result_recorded=false;review->results=pcr->results();review->rows=pcr->rows(185);
  review->appearance=[pcr,&frontend](unsigned row){return pcr->appearance(row,185,frontend.state().frame().context);};
  review->select_row=[pcr](unsigned row){pcr->select_row(row);};review->input.host=[]()->std::uint8_t{return 1;};
  review->input.details_available=[]()->std::uint8_t{return 0;};
  review->input.close=[&frontend](unsigned){frontend.state().close_online_prior_stats();};
  review->elapsed=owner->elapsed;review->floating_point=owner->floating_point;review->controller=0;
  review->pane=focus.pane;review->selected_row=focus.selected_row;owner->lobby_review=std::move(review);
  pcr->select_row(focus.selected_row);return owner;
}
} // namespace halo2
