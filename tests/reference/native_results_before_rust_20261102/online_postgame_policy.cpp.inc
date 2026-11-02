VistaPcrConsumedSnapshot online_postgame_publish(const MultiplayerMatch& match,
    const UiVariant& variant,std::span<const OnlineAdmission> roster) {
  const auto state=match.state();
  if(!state.finished || !state.results_finalized || !state.game_over_complete() ||
      !vista_pcr_ranking_available(match.source_round_lifecycle().flags) || roster.empty() || roster.size()>16)
    throw FormatError("ONLINE postgame requires a completed admitted original match");
  unsigned slots{};for(const auto& p:roster){if(p.slot>=16 || (slots&(1u<<p.slot)))throw FormatError("Invalid native network result roster");slots|=1u<<p.slot;}
  VistaPcrPublicationInput input;input.variant=vista_pcr_multiplayer_variant(source_game_variant(variant));
  input.statistics=match.recorded_statistics();input.recording=match.result_recording();
  if(!input.statistics || !input.recording.enabled || !input.recording.frozen)
    throw FormatError("ONLINE postgame requires actual frozen numeric statistics");
  if(!state.result_accuracy)throw FormatError("ONLINE postgame lacks its original native accuracy observations");
  input.current=input.finalized=1;input.engine_present=true;
  const auto &statistics=match.source_match_statistics();
  const auto standings=ranks(state,(std::to_integer<unsigned>(input.variant.bytes[0x48])&0x10)!=0,unsigned(roster.size()));
  const bool changing=source_game_variant_get(source_game_variant(variant),22)!=0;
  unsigned admitted_teams{};
  if(state.team_mode)for(const auto &p:roster) {
    if(p.team<0 || p.team>7)throw FormatError("Invalid admitted ONLINE team");
    admitted_teams|=1u<<p.team;
  }
  if(state.team_mode && (changing?(admitted_teams&state.retained_team_mask)!=admitted_teams:admitted_teams!=state.retained_team_mask))
    throw FormatError("ONLINE postgame team history differs from admission");
  if(state.team_mode)admitted_teams=state.retained_team_mask;
  for(unsigned i=0;i<8;++i)if(admitted_teams&(1u<<i)) {
    input.teams[i].occupied=1;input.teams[i].score=statistics.teams[i].fields[1];
    input.teams[i].placement=standings.teams[i]/2;
  }
  for(unsigned i=0;i<16;++i) {
    const auto &p=match.player(i);
    const auto admitted=std::ranges::find(roster,i,&OnlineAdmission::slot);
    if(admitted==roster.end()){if(p.connected)throw FormatError("Unadmitted native network postgame participant");continue;}
    if(!p.connected || admitted->slot!=i || p.source_datum==null_datum ||
        (p.source_datum&0xffffu)!=i || !p.source_profile_request)
      throw FormatError("ONLINE postgame participant lacks its actual selected profile");
    auto &row=input.players[i];row.occupied=1;row.identity=admitted->identity;
    row.configuration=native_vista_selected_profile_payload(*p.source_profile_request);
    // The relay admission owns the chosen display name. Retain the selected
    // PC-compatible appearance instead of copying Xbox sanitized species2/3.
    std::array<std::uint8_t,9> appearance{};
    const auto name=native_selected_player_configuration(admitted->name,appearance,-1);
    for(unsigned j=0;j<64;++j)row.configuration[j]=std::byte(name[j]);
    if(state.team_mode && (p.source_membership<0 || p.source_membership>=8 ||
        !(state.retained_team_mask&(1u<<p.source_membership)) || (!changing && p.source_membership!=admitted->team)))
      throw FormatError("ONLINE final team differs from its admitted rule and source history");
    row.configuration[0x7c]=std::byte(state.team_mode?p.source_membership:0xff);
    row.placement=standings.players[i]/2;
    row.score=vista_pcr_accumulated_player_score(statistics,p.source_datum);
  }
  return vista_pcr_publish_consumed_snapshot(input);
}
void online_postgame_validate(const MultiplayerState& state,const UiVariant& variant,
    std::span<const OnlineAdmission> roster,std::span<const std::byte> raw) {
  auto check=[](bool ok){if(!ok)throw FormatError("ONLINE match results differ from the final admitted game state");};
  check(raw.size()==vista_pcr_source_size && !roster.empty() && roster.size()<=16);
  check(state.finished && state.results_finalized && state.game_over_complete());
  check(state.result_accuracy.has_value());
  check(raw[0]==std::byte{1} && raw[1]==std::byte{1} && raw[0x10]==std::byte{1} && raw[0x11]==std::byte{1} && raw[0x13]==std::byte(state.team_mode));
  unsigned slots{};for(const auto& p:roster){check(p.slot<16);check(!(slots&(1u<<p.slot)));slots|=1u<<p.slot;}
  const auto expected=vista_pcr_multiplayer_variant(source_game_variant(variant));
  check(std::equal(expected.bytes.begin(),expected.bytes.end(),raw.begin()+0x20));
  auto word=[&](std::size_t at){return std::uint16_t(std::to_integer<unsigned>(raw[at])|(std::to_integer<unsigned>(raw[at+1])<<8));};
  const bool extended=(std::to_integer<unsigned>(expected.bytes[0x48])&0x10)!=0;
  const auto standings=ranks(state,extended,unsigned(roster.size()));
  const bool changing=source_game_variant_get(source_game_variant(variant),22)!=0;
  unsigned admitted_teams{};
  for(unsigned i=0;i<16;++i) {
    const auto at=0x370+i*0x94;const auto &p=state.players[i];
    const auto admitted=std::ranges::find(roster,i,&OnlineAdmission::slot);
    if(admitted==roster.end()){check(!p.connected && raw[at]==std::byte{});continue;}
    check(p.connected && admitted->slot==i && raw[at]==std::byte{1});
    check(std::equal(admitted->identity.begin(),admitted->identity.end(),raw.begin()+at+2));
    std::array<std::uint8_t,9> appearance{};const auto name=native_selected_player_configuration(admitted->name,appearance,-1);
    for(unsigned j=0;j<64;++j)check(raw[at+12+j]==std::byte(name[j]));
    if(state.team_mode) {
      check(admitted->team>=0 && admitted->team<8 && p.source_membership>=0 && p.source_membership<8);
      check(changing || p.source_membership==admitted->team);
      admitted_teams|=1u<<admitted->team;
      check(state.retained_team_mask&(1u<<p.source_membership));
    }
    check(raw[at+0x88]==std::byte(state.team_mode?p.source_membership:0xff));
    check(std::bit_cast<std::int16_t>(word(at+0x92))==p.total_score);
    check(std::to_integer<unsigned>(raw[at+0x90])==standings.players[i]/2);
    const std::array<std::pair<unsigned,int>,4> stats{{{7,p.kills},{9,p.deaths},{8,p.assists},{11,p.suicides}}};
    for(auto [index,value]:stats)check(value>=0 && value<=32767 && int(word(0xe30+2*(i*0x1b5+index))&0x7fff)==value);
    for(unsigned weapon=0;weapon<SourceResultAccuracy::weapon_count;++weapon)
      for(unsigned kind=0;kind<2;++kind) {
        const auto at=0xecc+2*(i*437+(weapon+SourceResultAccuracy::first_weapon)*8+4+kind);
        check((word(at)&0x7fffu)==state.result_accuracy->players[i][weapon][kind]);
      }
  }
  check(!state.team_mode || (changing?(admitted_teams&state.retained_team_mask)==admitted_teams:admitted_teams==state.retained_team_mask));
  if(state.team_mode)admitted_teams=state.retained_team_mask;
  for(unsigned i=0;i<16;++i) {
    const auto at=0xcb0+i*0x18;const bool present=i<8 && (admitted_teams&(1u<<i));
    check(raw[at]==std::byte(present));
    if(present) {
      check(std::to_integer<unsigned>(raw[at+1])==standings.teams[i]/2);
      check(std::bit_cast<std::int16_t>(word(at+2))==state.team_statistics[i][1]);
    }
  }
}
