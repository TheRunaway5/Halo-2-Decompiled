#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/hud.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <fstream>
#include <thread>
#include <type_traits>

namespace halo2 {
namespace {
// Legacy callers without a source configuration supply ASCII names. Selected
// profiles use their original UTF16 field below.
std::u16string ascii_name(const std::string &name) {
  std::u16string out;
  for(unsigned char c:name) {
    if(c>=128)throw FormatError("Native scoreboard name requires a UTF16 provider");
    out.push_back(c);
  }
  return out;
}
FullScoreboardRect rectangle(const UiRect &r) {
  return {std::int16_t(r.top),std::int16_t(r.left),std::int16_t(r.bottom),std::int16_t(r.right)};
}
std::optional<std::u16string> player_status_text(const FullScoreboardAssets &assets,
    const MultiplayerState &state,unsigned slot,bool suppress,std::size_t capacity) {
  if(slot>=state.players.size() || !state.players[slot].connected)return {};
  const auto &player=state.players[slot];
  if(!player.source_status)return {};
  const auto &status=*player.source_status;
  SourcePlayerStatusTextInput input;
  input.team_mode=state.team_mode;
  input.lifecycle=state.source_round_lifecycle_available?state.round_phase:status.lifecycle;
  input.special_mode=state.source_round_lifecycle_available?state.finished:status.special_mode;
  input.status=status.status;
  input.suppress_playing=suppress;
  input.respawn_seconds=player.source_respawn_seconds;
  input.lives_remaining=player.source_lives_remaining;
  const auto score=[&](std::int32_t value) {return full_scoreboard_score_text(value,state.source_engine);};
  input.player_score=score(std::bit_cast<std::int16_t>(std::uint16_t(player.score)));
  input.score_to_win=score(state.score_limit);
  if(player.source_membership>=0 && player.source_membership<8)
    input.team_score=score(state.team_scores[unsigned(player.source_membership)]);
  return source_player_status_text(assets,input,capacity);
}
Material dead_emblem_material(const Cache &cache,std::uint32_t shader,const Texture &image) {
  auto material=cache.load_material(shader,true);
  if(material.passes.size()!=1)throw FormatError("Unsupported scoreboard death indicator pass");
  const auto &pass=material.passes.front();
  if(pass.name!="shaders\\shader_passes\\hud\\waypoint_icon_meter")
    throw FormatError("Unsupported scoreboard death indicator shader");
  unsigned src=0x302,dst=0x303;
  for(const auto &state:pass.states) {
    if(state.index==62)src=state.value;
    if(state.index==63)dst=state.value;
  }
  if(src!=0x302 || dst!=0x303)throw FormatError("Unsupported scoreboard death indicator blend");
  material.diffuse=image;material.hud_pass=material.forward_pass=true;
  material.static_lightmap_pass=false;material.forward_pass_index=0;
  material.forward_program=pass.program;material.forward_vertex=0;material.forward_blend=1;
  material.vertex_program.clear();material.program_constants=pass.constants;
  if(!pass.texture_states.empty())material.texture_states[0]=pass.texture_states.front();
  material.ui_full_surface=true;
  return material;
}
}
struct MultiplayerScoreboard::Continuity {
  std::thread::id executor=std::this_thread::get_id();std::uint64_t revision{};bool busy{},alive=true;
  bool own()const noexcept{return executor==std::this_thread::get_id()&&alive;}
};
class MultiplayerScoreboard::Call {
  std::shared_ptr<Continuity> owner_;
public:
  Call(const MultiplayerScoreboard &board,bool write):owner_(board.continuity_){
    if(!owner_||!owner_->own()||owner_->busy||(write&&owner_->revision==UINT64_MAX))
      throw FormatError("Scoreboard continuation requires its completed owning executor");
    owner_->busy=true;if(write)++owner_->revision;
  }
  ~Call(){owner_->busy=false;}
  Call(const Call &)=delete;Call &operator=(const Call &)=delete;
};
struct MultiplayerScoreboard::Checkpoint::Data {
  std::weak_ptr<Continuity> owner;const Mesh *mesh{};const Material *roots{};std::size_t materials{};
  const FullScoreboardPainter *painter{};FullScoreboardHeldState held;
};
struct MultiplayerScoreboard::PreparedRestore::Data {
  std::weak_ptr<Continuity> owner;std::uint64_t revision{};const Mesh *mesh{};const Material *roots{};std::size_t materials{};
  const FullScoreboardPainter *painter{};FullScoreboardHeldState replacement;bool consumed{};
};
FullScoreboardOptions multiplayer_scoreboard_options(FullScoreboardOptions options,
    const MultiplayerState &state) {
  // Game+180 selects15BB50's decimal/time formatter. The public source
  // engine kind is the serialized variant+44, not the frontend selector.
  options.display_mode=state.source_engine;
  options.round_limit=state.round_mode;
  options.score_limit=state.score_limit;
  options.life_limit=state.life_limit;
  options.team_mode=state.team_mode;
  options.engine_active=state.source_round_lifecycle_available?state.round_phase==1:!state.finished;
  options.game_over=state.source_round_lifecycle_available?state.finished:state.results_finalized;
  return options;
}
std::int16_t multiplayer_scoreboard_scroll_axis(std::int16_t native_right_y) {
  const int raw=std::clamp(-int(native_right_y),-32768,32767);
  if(raw>9000)return std::int16_t((raw-9000)*32767/23767);
  if(raw< -9000)return std::int16_t(-((-raw-9000)*32768/23768));
  return 0;
}
MultiplayerScoreboard::MultiplayerScoreboard(const Cache &cache,
    const std::filesystem::path &directory,Mesh &mesh,const UiVariant &variant,
    const std::filesystem::path &ui_assets)
    : original_(cache.original_data()),continuity_(std::make_shared<Continuity>()) {
  if(!original_ || !original_->full_scoreboard_constants ||
      !original_->full_scoreboard_name_mappings || !original_->full_scoreboard_palette_fallback ||
      !original_->full_scoreboard_paint_constants)return;
  assets_=cache.load_full_scoreboard_assets();
  layout_constants_=*original_->full_scoreboard_constants;
  for(auto &view:emblem_materials_)view.fill(null_datum);
  for(auto &view:dead_materials_)view.fill(null_datum);
  auto emblem=frontend_emblem_material(cache,assets_.emblem_shader);
  auto dead=dead_emblem_material(cache,assets_.dead_shader,cache.load_bitmap(assets_.dead_bitmap,0));
  emblem.ui_full_surface=true;emblem.name="scoreboard/original/emblem";
  dead.name="scoreboard/original/death";
  if(!ui_assets.empty()) {
    if(!is_mcc_classic_hud_pack(ui_assets)) {
      const auto style=load_full_scoreboard_vista_style(assets_,ui_assets);
      layout_constants_.header_rgb=style[0];layout_constants_.background_alpha=style[1];
      layout_constants_.text_scale=style[2];layout_constants_.text_bias=style[3];layout_constants_.dim_scale=style[4];
    }
    std::map<std::uint32_t,UiImage> images;
    const auto image=[&](std::uint32_t datum) {
      if(images.contains(datum))return;
      const auto tag=cache.query_tag({.handle=TagHandle{datum}, .group=tag_groups::bitmap});
      if(!tag)throw FormatError("Scoreboard image owner missing");
      UiImage bank;bank.name=tag->name;
      for(unsigned i=0;i<cache.bitmap_count(datum);++i)bank.images.push_back(cache.load_bitmap(datum,i));
      images.emplace(datum,std::move(bank));
    };
    for(const auto &bitmap:emblem.bitmaps)image(bitmap.datum);
    image(assets_.emblem_bitmap);image(assets_.dead_bitmap);
    const auto sampled=load_ui_asset_library(images,ui_assets,"scoreboard.tsv");
    for(const auto &[datum,bank]:images)
      if(!sampled.contains(bank.name))throw FormatError("Vista scoreboard image bank missing: "+bank.name);
    Texture *stages[]{&emblem.diffuse,&emblem.detail};
    for(const auto &binding:emblem.passes.front().textures) {
      const auto &remaps=emblem.passes.front().bitmap_remaps;
      const auto remap=std::find_if(remaps.begin(),remaps.end(),[&](const auto &r){return (r.destination&255)==binding.parameter;});
      if(binding.external || binding.stage>1 || remap==remaps.end() || remap->source>=emblem.bitmaps.size())
        throw FormatError("Unsupported scoreboard emblem image binding");
      const auto &bitmap=emblem.bitmaps[remap->source];
      const auto &bank=sampled.at(images.at(bitmap.datum).name);
      *stages[binding.stage]=bank.at(bitmap.index);emblem.animated_textures[binding.stage]=bank;
    }
    emblem.ui_full_surface=true;emblem.name="scoreboard/vista/emblem";
    dead.diffuse=sampled.at(images.at(assets_.dead_bitmap).name).at(0);
    dead.name="scoreboard/vista/death";
  }
  mesh_=&mesh;
  // Original mulg+54C/+55C own the emblem and death passes independently of
  // optional artwork. Each player/view also owns its appearance and fade.
  for(unsigned view=0;view<4;++view)for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
    emblem_materials_[view][slot]=unsigned(mesh.materials.size());mesh.materials.push_back(emblem);
    dead_materials_[view][slot]=unsigned(mesh.materials.size());mesh.materials.push_back(dead);
  }
  const auto font_source=load_gameplay_font_source(directory);
  const auto& names=font_source.names;
  font_=load_gameplay_font(font_source,6);
  font_horizontal_offset_=gameplay_font_horizontal_offset(font_source,6);
  if(!ui_assets.empty()) {
    const auto sampled=hud_sampled_font_path(ui_assets,6,names[6]);
    const auto donor=ui_assets/"fonts-source"/names[6];
    if(!std::filesystem::is_regular_file(sampled) ||
        (font_source.domain==GameplayFontDomain::Original && !(is_mcc_classic_hud_pack(ui_assets)?
          apply_mcc_classic_font_sampling(font_,load_retail_font(sampled)):
          std::filesystem::exists(ui_assets/"hud-reconstruction.tsv")?
          apply_retail_font_sampling(font_,load_retail_font(sampled),load_retail_font(donor)):
          apply_retail_font_sampling(font_,load_retail_font(sampled)))))
      throw FormatError("Vista scoreboard font artwork is missing; rerun asset setup");
    if(font_source.domain==GameplayFontDomain::Mcc) {
      if(!is_mcc_classic_hud_pack(ui_assets))throw FormatError("Custom MCC gameplay font redraw needs an explicit original-metric projection");
      require_mcc_gameplay_font_sample(font_source,6,sampled);
    }
  }
  apply_vista_prompt_sampling(font_,font_prompt_asset_directory(font_source.directory,ui_assets),6);
  const auto solid=unsigned(mesh.materials.size());mesh.materials.push_back({});
  const auto font=unsigned(mesh.materials.size());mesh.materials.push_back({"scoreboard/font6",font_.atlas});
  mesh.materials[solid].ui_full_surface=true;mesh.materials[font].ui_full_surface=true;
  painter_=std::make_unique<FullScoreboardPainter>(font_,font,solid);
  const auto source=source_game_variant(variant);
  const auto parameter=[&](unsigned id) { return source_game_variant_get(source,int(id)); };
  options_.engine_present=true;options_.engine_active=true;
  options_.display_mode=source_game_variant_type(source);
  // Game+184 bit4 is variant+48 bit4 (parameter12), used by248A70/248C90
  // for extended score comparison after wins and accumulated score tie.
  options_.extended_round_comparison=parameter(12)!=0;
  options_.round_limit=parameter(0);options_.life_limit=parameter(0x10);
}
MultiplayerScoreboard::~MultiplayerScoreboard(){continuity_->alive=false;}
std::optional<std::u16string> MultiplayerScoreboard::status_text(const MultiplayerState &state,
    unsigned slot,bool suppress,std::size_t capacity)const {
  Call call(*this,false);
  return player_status_text(assets_,state,slot,suppress,capacity);
}
bool MultiplayerScoreboard::Checkpoint::valid()const noexcept{const auto owner=data_?data_->owner.lock():nullptr;return owner&&owner->own();}
MultiplayerScoreboard::Checkpoint MultiplayerScoreboard::checkpoint()const {
  Call call(*this,false);Checkpoint out;out.data_=std::make_shared<const Checkpoint::Data>(Checkpoint::Data{
    continuity_,mesh_,mesh_?mesh_->materials.data():nullptr,mesh_?mesh_->materials.size():0,painter_.get(),held_});return out;
}
bool MultiplayerScoreboard::owns_checkpoint(const Checkpoint &checkpoint)const noexcept{
  return checkpoint.valid()&&checkpoint.data_->owner.lock()==continuity_;
}
MultiplayerScoreboard::PreparedRestore::PreparedRestore()=default;
MultiplayerScoreboard::PreparedRestore::~PreparedRestore()=default;
MultiplayerScoreboard::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
MultiplayerScoreboard::PreparedRestore &MultiplayerScoreboard::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
MultiplayerScoreboard::PreparedRestore MultiplayerScoreboard::prepare_restore(const Checkpoint &checkpoint)const {
  Call call(*this,false);
  if(!owns_checkpoint(checkpoint)||continuity_->revision==UINT64_MAX)
    throw FormatError("Scoreboard checkpoint belongs to another owner or exhausted continuation");
  const auto &p=*checkpoint.data_;
  if(p.mesh!=mesh_||p.painter!=painter_.get()||p.roots!=(mesh_?mesh_->materials.data():nullptr)||
      p.materials!=(mesh_?mesh_->materials.size():0))
    throw FormatError("Scoreboard checkpoint has different resident painter or Mesh bindings");
  PreparedRestore out;out.data_=std::make_unique<PreparedRestore::Data>(PreparedRestore::Data{
    continuity_,continuity_->revision,p.mesh,p.roots,p.materials,p.painter,p.held,false});return out;
}
bool MultiplayerScoreboard::restore_ready(const PreparedRestore &prepared)const noexcept{
  const auto *p=prepared.data_.get();return continuity_->own()&&!continuity_->busy&&p&&!p->consumed&&
    p->owner.lock()==continuity_&&p->revision==continuity_->revision&&continuity_->revision!=UINT64_MAX&&
    p->mesh==mesh_&&p->painter==painter_.get()&&p->roots==(mesh_?mesh_->materials.data():nullptr)&&
    p->materials==(mesh_?mesh_->materials.size():0);
}
bool MultiplayerScoreboard::commit_restore(PreparedRestore &prepared)noexcept{
  if(!restore_ready(prepared))return false;
  static_assert(std::is_nothrow_swappable_v<FullScoreboardHeldState>);
  auto &p=*prepared.data_;using std::swap;swap(held_,p.replacement);p.consumed=true;++continuity_->revision;return true;
}
void MultiplayerScoreboard::step(unsigned view,bool held) {
  Call call(*this,true);
  if(view>=held_.counters.size())throw FormatError("Scoreboard input view outside local bindings");
  if(!available())return;
  // Only the manual held/released branch of15E6BC is bound here. Do not create
  // source engine lifecycle or death-observer records to invoke other branches.
  const auto &constants=layout_constants_;
  volatile float cap=30.f*std::bit_cast<float>(constants.counter_seconds);
  const int previous=std::fegetround();
  if(previous==-1 || std::fesetround(FE_TONEAREST))throw FormatError("Scoreboard rounding provider unavailable");
  const float rounded=std::nearbyint(float(cap));std::fesetround(previous);
  if(!std::isfinite(rounded) || rounded<0 || rounded>255)throw FormatError("Invalid source scoreboard counter cap");
  auto &counter=held_.counters[view];
  counter=held?std::uint8_t(std::min(int(counter)+1,int(rounded))):counter?std::uint8_t(counter-1):0;
  held_.inactive_counter=0;
}
void MultiplayerScoreboard::step(unsigned view,bool requested,
    const MultiplayerState& state,unsigned slot,std::int16_t source_scroll_axis) {
  Call call(*this,true);
  if(view>=4 || slot>=state.players.size() || !state.players[slot].connected)
    throw FormatError("Scoreboard has no local participant binding");
  if(!available())return;
  const auto& player=state.players[slot];
  FullScoreboardHeldInput input;input.local_view=std::int16_t(view);input.input_user=int(view);
  input.processed_record_present=true;input.held=requested;input.engine_present=true;
  input.scroll_axis=source_scroll_axis;
  input.lifecycle=state.results_finalized?2:1;
  input.engine_status=state.finished?2:1;input.options_state=4;
  if(player.source_status) {
    input.lifecycle=player.source_status->lifecycle;
    input.engine_status=player.source_status->engine_status;
    input.options_state=player.source_status->options_state;
  }
  input.current_unit=player.source_unit;
  input.death_countdown=player.source_respawn_seconds;input.player_flags=player.source_flags;
  input.exhausted=source_player_lives_exhausted(state.life_limit,player.source_unit,
      player.source_lives_remaining);
  auto gate_tick=std::int32_t(state.tick);
  if(player.source_status && player.source_status->scoreboard_gate) {
    // The original counter tail precedes object death, late respawn and the
    // clock increment. Consume its captured owners from that same frame.
    const auto &gate=*player.source_status->scoreboard_gate;
    gate_tick=gate.tick;input.current_unit=gate.current_unit;
    input.death_countdown=gate.respawn_seconds;input.player_flags=gate.player_flags;
    input.lifecycle=gate.lifecycle;input.options_state=gate.options_state;input.engine_status=gate.engine_status;
    input.exhausted=source_player_lives_exhausted(gate.life_limit,gate.current_unit,gate.lives_remaining);
    input.death_observer_excluded=gate.death_observer_excluded;
  }
  if(!full_scoreboard_update_held(held_,input,{30,tick_seconds,gate_tick},
      FullScoreboardRounding::NearestEven,*original_->full_scoreboard_constants))
    throw FormatError("Scoreboard input outside original bounds");
}
MultiplayerScoreboard::Draw MultiplayerScoreboard::draw(const MultiplayerState &state,
    std::span<const unsigned> ids,unsigned view,unsigned width,unsigned height,std::optional<TwoPlayerSplit> split) {
  Call call(*this,false);
  Draw out;
  if(!available() || view>=ids.size() || !width || !height)return out;
  const FullScoreboardClock clock{30,tick_seconds,std::int32_t(state.tick)};
  const auto &constants=layout_constants_;
  out.normal_hud=full_scoreboard_normal_hud(true,std::int16_t(view),held_.counters[view],clock,constants);
  if(!held_.counters[view])return out;
  const float alpha=full_scoreboard_display_alpha(held_.counters[view],clock,constants,full_scoreboard_host_power);
  std::array<std::u16string,multiplayer_capacity> names;
  std::array<std::u16string,8> team_names;
  std::array<FullScoreboardTeam,8> teams;
  std::array<TeamRespawnPlayer,multiplayer_capacity> presence;
  unsigned presence_count=0;
  std::vector<FullScoreboardPlayer> roster;
  const FullScoreboardPalette palette{assets_.scenario_present,assets_.palette,*original_->full_scoreboard_palette_fallback};
  for(unsigned slot=0;slot<state.players.size();++slot) {
    const auto &player=state.players[slot];if(!player.connected)continue;
    auto sanitized=full_scoreboard_sanitize_name(player.source_configuration
        ? source_player_configuration_name(*player.source_configuration) : ascii_name(player.name),
        *original_->full_scoreboard_name_mappings);
    if(sanitized.status!=FullScoreboardNameStatus::Ready)throw FormatError("Scoreboard dynamic name provider unavailable");
    names[slot]=std::move(sanitized.text);
    FullScoreboardPlayer row;row.datum=player.source_player;row.current_unit=player.source_unit;
    row.flags=player.source_flags;row.membership=std::bit_cast<std::int8_t>(std::uint8_t(player.source_membership));
    row.lives_remaining=player.source_lives_remaining;
    const auto local=std::find(ids.begin(),ids.end(),slot);
    row.local_view=local==ids.end()?-1:std::int16_t(local-ids.begin());
    auto appearance=full_scoreboard_default_appearance();
    if(player.source_configuration)std::copy_n(player.source_configuration->begin()+0x40,4,appearance.begin());
    const auto bits=full_scoreboard_ffa_colors(appearance,palette)[0];
    row.score=std::bit_cast<std::int16_t>(std::uint16_t(player.score));row.display_name=names[slot];
    row.color={std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2])};
    row.total_score=player.total_score;row.wins=player.wins;
    if(row.membership==-1) { // original830C0 uses the imported fallback RGB
      for(unsigned j=0;j<3;++j)row.color[j]=std::bit_cast<float>((*original_->full_scoreboard_palette_fallback)[j]);
    }
    else if(state.team_mode) {
      if(row.membership<0 || row.membership>7 || assets_.team_palette.size()<8)
        throw FormatError("Native scoreboard team palette/membership unavailable");
      const auto &team_color=assets_.team_palette[unsigned(row.membership)];
      for(unsigned j=0;j<3;++j)row.color[j]=std::bit_cast<float>(team_color[j]);
    }
    presence[presence_count++]={row.membership,row.flags,0};
    row.stat8=std::int16_t(player.kills);row.stat_a=std::int16_t(player.deaths);row.stat10=std::int16_t(player.assists);
    roster.push_back(row);
  }
  const bool wide=std::uint64_t(width)*3>std::uint64_t(height)*4;
  auto viewport=hud_viewport(unsigned(ids.size()),view,wide,split);
  // Same native output delivery as the HUD: preserve original font pixels,
  // expand each physical player cell, and retain its authored safe margins.
  const float scale=std::min(width/640.f,height/480.f),
      left=(width-640*scale)/2,top=(height-480*scale)/2;
  const auto old=viewport.outer;
  viewport.outer={(old.top*height/480.f-top)/scale,(old.left*width/640.f-left)/scale,
      (old.bottom*height/480.f-top)/scale,(old.right*width/640.f-left)/scale};
  viewport.safe={viewport.outer.top+viewport.safe.top-old.top,viewport.outer.left+viewport.safe.left-old.left,
      viewport.outer.bottom+viewport.safe.bottom-old.bottom,viewport.outer.right+viewport.safe.right-old.right};
  const auto outer=rectangle(viewport.outer),inner=rectangle(viewport.safe);
  const auto options=multiplayer_scoreboard_options(options_,state);
  if(state.team_mode) {
    if(assets_.team_palette.size()<8)throw FormatError("Native scoreboard team palette unavailable");
    const auto playing=source_team_presence(0,std::span(presence).first(presence_count)).playing;
    for(unsigned i=0;i<8;++i) {
      auto &team=teams[i];team.retained=bool(state.retained_team_mask&(1u<<i));team.playing=bool(playing&(1u<<i));
      team.score=state.team_scores[i];team_names[i]=ascii_name(assets_.team_names[i]);team.display_name=team_names[i];
      team.total_score=state.team_total_scores[i];team.wins=state.team_wins[i];
      team.stat8=state.team_statistics[i][2];team.stat_a=state.team_statistics[i][3];team.stat10=state.team_statistics[i][6];
      for(unsigned j=0;j<3;++j)team.color[j]=std::bit_cast<float>(assets_.team_palette[i][j]);
    }
  }
  const auto name=ascii_name(assets_.name_label),wins=ascii_name(assets_.wins_label),score=ascii_name(assets_.score_label);
  const auto title=player_status_text(assets_,state,ids[view],false,80).value_or(u"");
  out.layout=full_scoreboard_layout(roster,state.players[ids[view]].source_player,options,
      outer,inner,held_.scroll[view],alpha,{title,name,wins,score},constants,
      state.team_mode ? std::span<const FullScoreboardTeam>(teams) : std::span<const FullScoreboardTeam>{});
  // Original1634B6/1634F0 writes the layout's per-view clamp back before
  // drawing. Retain it so reversing at either end responds on the next tick.
  if(out.layout.status==FullScoreboardStatus::Ready &&
      std::bit_cast<std::uint32_t>(held_.scroll[view])!=std::bit_cast<std::uint32_t>(out.layout.scroll)) {
    if(continuity_->revision==UINT64_MAX)throw FormatError("Scoreboard scroll continuation exhausted");
    held_.scroll[view]=out.layout.scroll;++continuity_->revision;
  }
  FullScoreboardPaintFrame frame;frame.clip={0,0,std::int16_t(outer.bottom-outer.top),std::int16_t(outer.right-outer.left)};
  frame.clock=clock;frame.font_horizontal_offset=font_horizontal_offset_;frame.shadow_enabled=true;frame.x87_control_word=0x27f;
  const FullScoreboardEmblem emblem=mesh_?FullScoreboardEmblem([&](const FullScoreboardRow &row)->std::optional<unsigned> {
    const auto player=std::find_if(state.players.begin(),state.players.end(),[&](const auto &p){return p.connected&&p.source_player==row.datum;});
    if(player==state.players.end())return {};
    const auto slot=unsigned(player-state.players.begin());
    const auto material=row.icon_kind==0?emblem_materials_[view][slot]:dead_materials_[view][slot];
    auto &m=mesh_->materials.at(material);
    const auto alpha=row.widgets[1].foreground[0];
    FrontendAppearance appearance;
    const auto bytes=player->source_configuration;
    if(bytes) {
      for(unsigned i=0;i<4;++i)appearance.colors[i]=std::bit_cast<std::int8_t>((*bytes)[0x40+i]);
      appearance.species=std::bit_cast<std::int8_t>((*bytes)[0x44]);
      appearance.foreground=(*bytes)[0x45];appearance.background=(*bytes)[0x46];appearance.flags=(*bytes)[0x47];
    }
    std::vector<Vec3> palette;
    for(const auto &color:assets_.palette)palette.push_back(std::bit_cast<Vec3>(color));
    if(row.icon_kind==0)apply_object_colors(m,frontend_appearance_colors(appearance,palette));
    const auto input=[&](std::uint32_t name) {
      // Vista62265C supplies opacity to the waypoint fade and fourth input;
      // 624B53/623875 supply this row's actual player appearance separately.
      if(name==0x0c0005cb || name==0x0a0005d2)return alpha;
      if(name==0x0d0005ca || name==0x0c0005cc || name==0x0e0005cd)return 0.f;
      return frontend_emblem_input(appearance,name,alpha);
    };
    m.forward_program=m.passes.front().program;
    animate_forward_material(m,0,input);animate_material_bitmaps(m,0,input);
    if(row.icon_kind==1) {
      // Original2325CA copies RGB(1,0,0) through [474C7C] to4F3220;
      // waypoint_icon_meter reads it as pixel extern33. Its alpha remains
      // owned by the authored row-fade overlays above (Vista62265C agrees).
      std::array<ShaderVector,256> colors{};colors[33]={1,0,0,1};
      const auto &pass=m.passes.front();
      remap_pixel_externs(m.forward_program,{},colors,pass.pixel_externs,
          pass.fragments,m.shader_switches);
    }
    return material;
  }):FullScoreboardEmblem{};
  out.paint=painter_->draw(out.layout,*original_->full_scoreboard_paint_constants,frame,
      full_scoreboard_host_pulse,emblem);
  for(auto &vertex:out.paint.draw.vertices) {
    vertex.position.x=((vertex.position.x+outer.left)*scale+left)*640/width;
    vertex.position.y=((vertex.position.y+outer.top)*scale+top)*480/height;
  }
  return out;
}
} // namespace halo2
