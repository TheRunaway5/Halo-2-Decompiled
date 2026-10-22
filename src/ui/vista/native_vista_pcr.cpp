#include "halo2/native_vista_pcr.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/full_scoreboard.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)throw FormatError("Truncated actual Vista PCR session owner");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);return v;
}
bool session_players(std::span<const std::byte> b,std::uint8_t active) {
  if(b.size()<vista_session_source_size)throw FormatError("Actual complete Vista PCR session bank required");
  const auto state=word(b,0x73a0);return active && state>=3 && state<=8;
}
std::uint32_t find_player(std::span<const std::byte> b,const VistaLocalUserIdentity& id) {
  const auto mask=word(b,0x1258);
  for(unsigned i=0;i<16;++i)if((mask&(1u<<i)) &&
      std::equal(id.begin(),id.end(),b.begin()+0x125c+i*0x128))return i;
  return 0xffffffffu;
}
std::u16string wide(std::string_view text) {
  std::u16string out;
  for(std::size_t i=0;i<text.size();) {
    auto c=std::uint32_t(static_cast<unsigned char>(text[i++]));unsigned n{};std::uint32_t minimum{};
    if(c<0x80){}
    else if(c>=0xc2 && c<=0xdf){c&=31;n=1;minimum=0x80;}
    else if(c>=0xe0 && c<=0xef){c&=15;n=2;minimum=0x800;}
    else if(c>=0xf0 && c<=0xf4){c&=7;n=3;minimum=0x10000;}
    else throw FormatError("Invalid imported Vista locale UTF8");
    while(n--) {
      if(i==text.size() || (static_cast<unsigned char>(text[i])&0xc0)!=0x80)
        throw FormatError("Invalid imported Vista locale UTF8 continuation");
      c=(c<<6)|(static_cast<unsigned char>(text[i++])&63);
    }
    if(c<minimum || c>0x10ffff || (c>=0xd800 && c<=0xdfff))throw FormatError("Invalid imported Vista locale scalar");
    if(c<0x10000)out+=char16_t(c);
    else {c-=0x10000;out+=char16_t(0xd800+(c>>10));out+=char16_t(0xdc00+(c&1023));}
  }
  return out;
}
VistaPcrRgb rgb(Vec3 v){return {std::bit_cast<std::uint32_t>(v.x),std::bit_cast<std::uint32_t>(v.y),std::bit_cast<std::uint32_t>(v.z)};}
template<class F> const F& required(const F& f,const char* message){if(!f)throw FormatError(message);return f;}
const FrontendContent& pc_content(const FrontendContent& c,unsigned screen) {
  if((screen!=16 && screen!=185) || !c.vista || !c.vista->screens.contains(screen))
    throw FormatError("Actual Vista PCR16/185 imported definitions required");
  return *c.vista;
}
}
bool native_vista_pcr_identity_query(std::span<const std::byte> b,std::uint8_t active,
    std::uint32_t kind,const VistaLocalUserIdentity& id) {
  return session_players(b,active) && kind>=1 && kind<=5 && find_player(b,id)!=0xffffffffu;
}
std::uint32_t native_vista_pcr_primary_player_query(std::span<const std::byte> b,
    std::uint8_t active,const VistaLocalUserIdentity& id) {
  return session_players(b,active)?find_player(b,id):0xffffffffu;
}
bool native_vista_pcr_player_query(std::span<const std::byte> b,std::uint8_t active,
    std::uint32_t datum) {
  if(!session_players(b,active))return false;
  const auto index=datum&0xffffu;
  if(!(word(b,0x1258)&(1u<<(index&31))))return false;
  // Original615820 has no explicit sixteen-record index guard. Reject only
  // when the actual source address would exceed this caller-owned bank.
  return word(b,0x1264+std::size_t(index)*0x128)==word(b,0x71c0);
}
std::uint32_t native_vista_pcr_placement_key(std::int32_t value) noexcept {
  const auto index=std::bit_cast<std::uint32_t>(value);
  if(index>15)return 0;
  return (index<9?0x0c0000ecu:0x0d0000ecu)+index;
}
std::u16string native_vista_pcr_format(std::u16string_view source,
    std::span<const std::int32_t> values) {
  std::u16string out;unsigned argument{};
  for(std::size_t i=0;i<source.size();++i) {
    if(source[i]!=u'%'){out+=source[i];continue;}
    if(++i==source.size())throw FormatError("Truncated actual Vista PCR format");
    if(source[i]==u'%'){out+=u'%';continue;}
    const bool zero=source[i]==u'0';unsigned width{};
    while(i<source.size() && source[i]>=u'0' && source[i]<=u'9') {
      width=width*10+source[i++]-u'0';
      if(width>128)throw FormatError("Unsupported actual Vista PCR format width");
    }
    if(i==source.size() || source[i]!=u'd' || argument==values.size())
      throw FormatError("Unsupported actual Vista PCR integer format");
    auto digits=wide(std::to_string(values[argument++]));
    if(digits.size()<width) {
      if(zero && digits.front()==u'-')digits.insert(1,width-digits.size(),u'0');
      else digits.insert(0,width-digits.size(),zero?u'0':u' ');
    }
    out+=digits;
    if(out.size()>509)throw FormatError("Actual Vista PCR CRT format exceeds admitted nontruncating range");
  }
  if(argument!=values.size())throw FormatError("Unused actual Vista PCR format argument");
  if(out.size()>509)throw FormatError("Actual Vista PCR CRT format exceeds admitted nontruncating range");
  return out;
}

NativeVistaPcr::NativeVistaPcr(const MultiplayerMatch& match,unsigned slot,
    const NativeVistaSession& session,const FrontendContent& content,
    const VistaPcrPcVariant& variant,NativeVistaPcrServices services)
  :session_(&session),content_(content),services_(std::move(services)),results_(std::make_shared<VistaPcrResults>()) {
  const auto& pc=pc_content(content_,16);
  (void)pc_content(content_,185);
  required(services_.lifecycle_active,"Native Vista PCR needs the actual lifecycle active owner");
  required(services_.lifecycle_kind,"Native Vista PCR needs the actual lifecycle kind owner");
  if(slot>=16 || match.team_mode() || variant.bytes[0x44]!=std::byte{2} ||
      (variant.bytes[0x48]&std::byte{1})!=std::byte{})
    throw FormatError("Native Vista PCR currently admits local offline FFA Slayer");
  const auto state=match.state();
  if(!state.finished || !state.results_finalized || !state.game_over_complete())
    throw FormatError("Native Vista PCR requires actual completed match results");
  const auto mapping=session_->player_for_controller(0);
  if(!mapping || mapping->player!=slot)
    throw FormatError("Native Vista PCR needs the actual identical native/PC participant slot");
  if(!std::equal(variant.bytes.begin(),variant.bytes.end(),session_->bank().begin()+0x4e3c))
    throw FormatError("Native Vista PCR selected variant differs from its actual PC session owner");
  std::array<std::optional<NativeVistaSessionPlayer>,16> mappings;
  std::uint32_t participant_mask{};
  for(unsigned controller=0;controller<4;++controller)if(const auto local=session_->player_for_controller(controller)) {
    if(local->player>=16 || mappings[local->player])throw FormatError("Native Vista PCR duplicate local participant");
    mappings[local->player]=local;
  }
  std::vector<FullScoreboardPlayer> participants;
  for(unsigned index=0;index<16;++index) {
    const auto& player=match.player(index);if(!player.connected)continue;
    const auto& local=mappings[index];
    if(!local || !player.source_profile_request || !player.source_configuration ||
        player.source_datum==null_datum || (player.source_datum&0xffffu)!=index ||
        player.source_membership!=std::int8_t(index) || (player.source_flags&2) ||
        native_vista_selected_profile_payload(*player.source_profile_request)!=local->requested_payload)
      throw FormatError("Native Vista PCR participant lost its actual profile, identity or FFA membership");
    participant_mask|=1u<<index;
    // Same recovered ranking comparator as the native full scoreboard. PC
    //469462..469495 divides the encoded rank by2 before publishing placement.
    const auto& stats=match.source_match_statistics().players[index].fields;
    FullScoreboardPlayer rank;rank.datum=player.source_datum;rank.current_unit=player.source_current_object;
    rank.flags=player.source_flags;rank.membership=player.source_membership;
    rank.lives_remaining=player.source_lives_remaining;
    rank.score=stats[0];rank.total_score=stats[1];rank.stat8=stats[2];rank.stat_a=stats[3];rank.stat10=stats[6];rank.wins=stats[7];
    participants.push_back(rank);
  }
  if(participants.empty() || participant_mask!=word(session_->bank(),0x1258) ||
      !vista_pcr_ranking_available(match.source_round_lifecycle().flags))
    throw FormatError("Native Vista PCR roster differs from its actual session/ranking owner");
  FullScoreboardOptions rank_options;rank_options.engine_present=true;rank_options.game_over=true;
  rank_options.life_limit=state.life_limit;
  rank_options.extended_round_comparison=(variant.bytes[0x48]&std::byte{0x10})!=std::byte{};
  const auto ranks=full_scoreboard_ranks(participants,rank_options);
  const auto publication=required(services_.publication,"Native Vista PCR publication owner is unavailable")();
  if(!publication.current || !publication.finalized || !publication.engine_present)
    throw FormatError("Native Vista PCR actual publication owner has not finalized");
  VistaPcrPublicationInput input;
  input.variant=variant;input.statistics=match.recorded_statistics();input.recording=match.result_recording();
  if(!input.statistics || !input.recording.enabled || !input.recording.frozen)
    throw FormatError("Native Vista PCR requires the actual frozen numeric recording owner");
  input.current=publication.current;input.finalized=publication.finalized;input.engine_present=publication.engine_present;
  for(const auto& player:participants) {
    const auto index=player.datum&0xffffu;const auto& local=*mappings[index];
    auto& row=input.players[index];row.occupied=true;row.identity=local.identity;row.configuration=local.final_payload;
    row.score=vista_pcr_accumulated_player_score(match.source_match_statistics(),player.datum);
    row.placement=std::int8_t(ranks.players[index]/2);
  }
  source_=vista_pcr_publish_consumed_snapshot(input);
  VistaPcrServices build;build.engine_present=publication.engine_present;build.team_play=false;
  build.score=[](std::int32_t value){return native_vista_pcr_format(u"%d",std::span(&value,1));};
  build.placement=[&](std::int32_t value){return wide(detail::frontend_pc_compiled_localized(pc,pc.screens.at(16),native_vista_pcr_placement_key(value)));};
  build.live_identity=[&](unsigned index)->std::optional<VistaLocalUserIdentity> {
    const auto& actual=match.player(index);
    if(!actual.connected || actual.source_datum==0xffffffffu)return {};
    if(index>=mappings.size() || !mappings[index])throw FormatError("Native Vista PCR reached an unmapped actual participant");
    return mappings[index]->identity;
  };
  // Original40E8B0 is the actual supplied XUserGetXUID import, controller0.
  // It zeroes its output on failure; the original achievement caller ignores
  // HRESULT and compares those output bytes.
  build.selected_identity=[&]{return session_->xuser_get_xuid(0).output;};
  build.medal_achievement=services_.medal_achievement;
  vista_pcr_build(*results_,source_.source,build);
  if(results_->player_count!=participants.size() || results_->team_count || results_->variant!=2)
    throw FormatError("Native Vista PCR result producer lost an actual local FFA participant");
  initialize_assets(pc);
}
void NativeVistaPcr::initialize_assets(const FrontendContent& pc) {
  for(const auto color:pc.appearance_palette)palette_.push_back(rgb(color));
  for(const auto color:pc.vista_pcr_team_palette)teams_.push_back(rgb(color));
  if(palette_.empty())throw FormatError("Actual Vista appearance palette owner is unavailable");
  assets_.globals_present=true;assets_.appearance_palette=palette_;assets_.team_palette=teams_;
  assets_.rank_owner_present=pc.vista_pcr_rank_owner_present;assets_.rank_ranges=pc.vista_pcr_rank_ranges;
}
NativeVistaPcr::NativeVistaPcr(VistaPcrConsumedSnapshot source,std::vector<OnlineAdmission> roster,
    const FrontendContent& content,VistaLoadingFloatingPoint fp)
    :online_roster_(std::move(roster)),content_(content),source_(std::move(source)),results_(std::make_shared<VistaPcrResults>()) {
  const auto& pc=pc_content(content_,16);(void)pc_content(content_,185);services_.floating_point=fp;
  const bool team=source_.source[0x13]==std::byte{1};
  if(online_roster_.size()<2 || online_roster_.size()>16 ||
      source_.source[0x13]!=std::byte(team) || ((source_.source[0x68]&std::byte{1})!=std::byte(team)))
    throw FormatError("ONLINE PCR requires its admitted Slayer roster");
  const auto order=vista_pcr_order(source_.source);
  if(order.player_count!=online_roster_.size() || word(source_.source,0x64)!=2)
    throw FormatError("ONLINE PCR does not match its completed Slayer roster");
  unsigned team_mask{};
  for(unsigned i=0;i<16;++i) {
    const auto at=0x370+i*0x94;
    if(i<online_roster_.size()) {
      if(online_roster_[i].slot!=i || source_.source[at]!=std::byte{1} ||
          !std::equal(online_roster_[i].identity.begin(),online_roster_[i].identity.end(),source_.source.begin()+at+2))
        throw FormatError("ONLINE PCR identity differs from authenticated admission");
      const auto expected=wide(online_roster_[i].name);
      if(team) {
        const auto selected=online_roster_[i].team;
        if(selected<0 || selected>7 || source_.source[at+0x88]!=std::byte(selected))
          throw FormatError("ONLINE PCR team differs from admission");
        team_mask|=1u<<selected;
      } else if(source_.source[at+0x88]!=std::byte{0xff})
        throw FormatError("FFA PCR contains team membership");
      for(unsigned j=0;j<32;++j) {
        const auto at_name=at+0xc+j*2;
        const auto value=std::to_integer<unsigned>(source_.source[at_name]) | (std::to_integer<unsigned>(source_.source[at_name+1])<<8);
        if(value!=(j<expected.size()?expected[j]:0))throw FormatError("ONLINE PCR screen name differs from admission");
      }
    } else if(source_.source[at]!=std::byte{})throw FormatError("ONLINE PCR contains an unadmitted player");
  }
  for(unsigned i=0;i<16;++i)if(source_.source[0xcb0+i*0x18]!=std::byte(i<8 && (team_mask&(1u<<i))))
    throw FormatError("ONLINE PCR team rows differ from admission");
  VistaPcrServices build;build.engine_present=true;build.team_play=team;
  build.team_name=[&](std::uint32_t n){
    if(n>8)throw FormatError("Invalid Vista team-name ordinal");
    const auto &name=pc.vista_team_names[n];
    if(name.empty())throw FormatError("Actual Vista team-name locale unavailable");
    return wide(name);
  };
  build.score=[](std::int32_t n){return native_vista_pcr_format(u"%d",std::span(&n,1));};
  build.placement=[&](std::int32_t n){return wide(detail::frontend_pc_compiled_localized(pc,pc.screens.at(16),native_vista_pcr_placement_key(n)));};
  // No retail Live achievement service exists for these native room keys.
  // The live-PC identity lookup is explicitly absent; display rows still use
  // the retained native keys through this adapter's appearance queries.
  build.live_identity=[](unsigned)->std::optional<VistaLocalUserIdentity>{return {};};
  vista_pcr_build(*results_,source_.source,build);initialize_assets(pc);
}
VistaPcrRowServices NativeVistaPcr::rows(unsigned screen) {
  (void)pc_content(content_,screen);VistaPcrRowServices out;
  out.selected_row=[this]{return selected_row_;};out.format=native_vista_pcr_format;
  out.localized=[this,screen](std::uint32_t key){const auto& pc=pc_content(content_,screen);return wide(detail::frontend_pc_compiled_localized(pc,pc.screens.at(screen),key));};
  out.team_color=[this](std::uint16_t team) {
    const auto color=vista_pcr_appearance_colors({},team,0,assets_)[0];
    return VistaPcrTeamColor{vista_pcr_profile_name_color(color,services_.floating_point),color};
  };
  return out;
}
VistaPcrPlayerPresentation NativeVistaPcr::appearance(unsigned row,unsigned screen,unsigned pane) {
  if(pane==0 || pane>=6)throw FormatError("Native Vista PCR appearance requires an actual player pane");
  const auto& pc=pc_content(content_,screen);const auto& definition=pc.screens.at(screen).panes.at(pane);
  if(definition.lists.empty())throw FormatError("Actual Vista PCR pane has no row skin");
  const auto& outer=pc.skins.at(definition.lists.front().skin);
  if(outer.grids.empty())throw FormatError("Actual Vista PCR row has no appearance group");
  const auto& skin=pc.skin_groups.at(outer.grids.front().skin);
  if(skin.emblems.empty())throw FormatError("Actual Vista PCR appearance has no emblem definition");
  auto assets=assets_;assets.emblem_definition_flags=skin.emblems.front().flags;
  for(const auto& bitmap:skin.bitmaps) {
    const auto image=pc.images.find(bitmap.datum);
    const bool present=image!=pc.images.end() && !image->second.images.empty();
    if(detail::frontend_pc_compiled_matches(pc,bitmap.placeholder,0x14000698)){assets.primary_bitmap_present=present;assets.primary_page_count=present?image->second.images.size():0;assets.previous_primary_frame=bitmap.frame;}
    if(detail::frontend_pc_compiled_matches(pc,bitmap.placeholder,0x0900069a)){assets.rank_bitmap_present=present;assets.rank_page_count=present?image->second.images.size():0;assets.previous_rank_frame=bitmap.frame;}
  }
  const auto view=vista_pcr_player_presentation(*results_,row,assets,*this,services_.floating_point);
  return {view.appearance,view.team,view.rank_range,view.primary_color,view.name_color,
      view.primary_visible,view.secondary_visible,view.rank_visible,view.emblem_visible,view.primary_frame,view.rank_frame};
}
VistaPcrVoicePresentation NativeVistaPcr::voice(unsigned row,std::uint8_t lobby,
    const VistaPcrVoiceServices& services) const {return vista_pcr_voice(*results_,row,lobby,services);}
void NativeVistaPcr::select_row(std::uint32_t row) {
  if(row!=0xffffffffu && row>=std::max(results_->player_count,results_->team_count))
    throw FormatError("Native Vista PCR focus lies outside actual result rows");
  selected_row_=row;
}
bool NativeVistaPcr::identity_query(std::array<std::byte,8> id) {
  if(!session_)return std::any_of(online_roster_.begin(),online_roster_.end(),[&](const auto &p){return p.identity==id;});
  const auto active=services_.lifecycle_active();
  // Source5AD782 does not consume lifecycle kind when disabled.
  return native_vista_pcr_identity_query(session_->bank(),active,active?services_.lifecycle_kind():0,id);
}
std::uint32_t NativeVistaPcr::primary_player_query(std::array<std::byte,8> id) {
  if(!session_){for(const auto &p:online_roster_)if(p.identity==id)return p.slot;return 0xffffffffu;}
  return native_vista_pcr_primary_player_query(session_->bank(),services_.lifecycle_active(),id);
}
bool NativeVistaPcr::player_query(std::uint32_t player) {
  if(!session_)return player<online_roster_.size();
  return native_vista_pcr_player_query(session_->bank(),services_.lifecycle_active(),player);
}
} // namespace halo2
