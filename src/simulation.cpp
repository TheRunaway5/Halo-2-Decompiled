#include "native_startup_memory_hints.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/frontend_model_lighting.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/inventory.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/math.hpp"
#include "halo2/source_script.hpp"
#include "halo2/pickup_visibility.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/weapon_hand_feedback.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/script_clock_builtin.hpp"
#include "halo2/source_script_rumble_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_scripted_sound_point_tu15.hpp"
#include "halo2/source_campaign_lighting_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/source_unit_cinematic_tu15.hpp"
#include "halo2/source_script_effect_tu15.hpp"
#include "simulation_world.hpp"
#include "native_heap_reclaim.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>
namespace halo2 {
std::uint32_t source_cinematic_emotion_name_tu15(const CinematicAnimationLoader* assets,std::uint32_t id) {
  if(!assets || !assets->assets)throw FormatError("Native emotion names require the retained cinematic Cache owner");
  return assets->assets->unit_emotion_name_tu15(id);
}
namespace {
Vec3 center(const Unit &);
float direct_damage_amount(const WeaponDefinition &weapon) {
  if(weapon.damage_amount && weapon.damage_amount->upper_minimum==weapon.damage_amount->upper_maximum) {
    // RetailD6660 initializes event54/event5C to1; D9B60 interpolates the
    // lower envelope with the selected upper. Equal upper bounds use no RNG.
    const auto &definition=*weapon.damage_amount;
    return damage_amount(definition.lower,definition.upper_minimum,1.f,1.f,1.f);
  }
  if(weapon.smg)throw FormatError("SMG random damage amount selection remains unported");
  // Retain the existing path for synthetic definitions and unrecovered random
  // upper selection. This helper does not invent a primary-stream draw.
  return weapon.damage_min;
}
}
const CollisionWorld &Simulation::collision() const { return prop_collision_?*prop_collision_:*world_->collision; }
void Simulation::replicate_source_scene(std::span<const SourceSceneRoot> roots,
    std::span<const SourceWorldPose> poses) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(source_physics_movement_)throw FormatError("Source authority cannot consume replica scene poses");
  const bool first=!scene_.source_replica();
  scene_.adopt_source_replicas(roots,poses);
  if(first) {
    // Startup handles belonged to a different allocator. Rebuild derived
    // collision/region owners from the accepted authoritative identities.
    props_=PropSandbox(content_);scenery_collision_=StaticSceneryCollision(content_);
  }
  synchronize_props(first);
}
void Simulation::replicate_devices(const SourceDeviceWorldSnapshot &snapshot){
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  devices_.apply_snapshot(snapshot);synchronize_props();
}
void Simulation::replicate_prop_regions(std::span<const PropRegionSnapshot> rows){
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  props_.apply_regions(rows);synchronize_props();
}
void Simulation::synchronize_props(bool force){
  devices_.synchronize(scene_);
  const bool devices_changed=devices_.synchronize_collision();
  const bool props_changed=props_.synchronize(scene_);
  const bool scenery_changed=scenery_collision_.synchronize(scene_);
  const bool fixed_changed = scenery_changed || devices_changed || force || !static_collision_;
  if (props_changed || fixed_changed || !prop_collision_) {
    // Loading and checkpoint publication can retain two worlds. Release dead
    // loader/layer buffers before allocating the newly composed collision bank.
    if (force) reclaim_unused_host_heap_pages();
    const auto layers = CollisionWorld::compose(world_->collision,
        {scenery_collision_.triangles(), devices_.triangles(), props_.triangles()},
        {std::move(static_collision_), std::move(prop_collision_), fixed_changed});
    world_ = std::make_shared<BspWorld>(world_->sectors, layers.bsp);
    static_collision_ = layers.fixed;
    prop_collision_ = layers.all;
  }
}
void Simulation::replicate_prop_removals(std::span<const PropRemoval> rows){
  NativeAuthorityBoundary::Call active(authority_calls_);
  if(rows.size()>2048)throw FormatError("Native prop removal bank exceeds scene capacity");
  std::set<std::uint32_t> handles,placements;
  for(const auto& row:rows){
    if(row.handle==null_datum || !(row.handle>>16) || (row.handle&0xffff)>=2048 ||
        row.placement>=content_.objects.size() || !handles.insert(row.handle).second ||
        !placements.insert(row.placement).second)
      throw FormatError("Invalid native prop removal identity");
    const auto& authored=content_.objects[row.placement];
    if(authored.type!=11 || authored.definition!=row.definition)
      throw FormatError("Native prop removal differs from authored placement");
    for(const auto& object:scene_.objects())if(object.handle==row.handle && object.placement!=row.placement)
      throw FormatError("Native prop removal handle names another placement");
    for(const auto& object:scene_.objects())if(object.handle!=null_datum && object.placement==row.placement && object.handle!=row.handle)
      throw FormatError("Native prop removal has a stale scene generation");
  }
  ++authority_calls_.mutations;
  for(const auto& row:rows){
    for(const auto& object:scene_.objects())if(object.handle==row.handle){scene_.destroy(row.handle);break;}
  }
  prop_removals_.assign(rows.begin(),rows.end());synchronize_props();
}
void Simulation::request_bsp(int target) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  // Original 0x12B850: invalid targets are ignored; requesting the current
  // BSP cancels an earlier pending transition. Both HS forms compile ordinals.
  if (target < 0 || std::size_t(target) >= content_.bsp_count)
    return;
  ++authority_calls_.mutations;
  if (std::size_t(target) == bsp_)
    pending_bsp_.reset();
  else
    pending_bsp_ = std::size_t(target);
}
void Simulation::commit_bsp() {
  if (!pending_bsp_)
    return;
  if (!bsp_loader_)
    throw FormatError("campaign BSP loader unavailable");
  const auto target = *pending_bsp_;
  auto next = std::make_shared<BspWorld>(bsp_loader_(target));
  if(source_campaign_)source_campaign_->switch_bsp(target);
  for (auto &actor : actors_) {
    actor.bsp_hidden = !scene_.retains(actor.bsp,int(target),center(actor));
    if (!actor.bsp_hidden) actor.bsp = int(target);
  }
  std::erase_if(projectiles_,[&](auto &p) {
    if (!scene_.retains(p.bsp,int(target),p.position)) {
      if(release_object_ && p.source_datum!=null_datum)release_object_(p.source_datum);
      return true;
    }
    p.bsp=int(target);
    return false;
  });
  scene_.switch_bsp(target,presentation_.cinematic && suppress_automatic_objects_);
  // The interpreter, globals and game clock remain live across a BSP change.
  world_ = std::move(next);
  bsp_ = target;
  if(source_campaign_)source_campaign_->publish(*this);
  synchronize_props(true);
  player_.bsp = int(target);
  pending_bsp_.reset();
  ++bsp_revision_;
  for (auto &actor : actors_) {
    actor.route.clear();
    actor.waypoint = 0;
  }
}
struct Simulation::SavedState {
  std::shared_ptr<const BspWorld> world;
  std::size_t bsp;
  std::optional<NativeObjectLifetimes> object_lifetimes;
  SourceObjectBodyStorage object_bodies;
  SceneObjects scene;
  SourceDeviceWorld devices;
  std::vector<NativeWeaponPickup> weapon_pickups;
  std::vector<VehicleFlip> vehicle_flips;
  std::vector<NetgameWeaponState> vehicle_rows;
  PropSandbox props;
  std::vector<PropRemoval> prop_removals;
  Presentation presentation;
  std::array<std::array<bool, 16>, 16> peace;
  Unit player;
  std::vector<Unit> actors;
  ScriptVm::Snapshot scripts;
  std::vector<Projectile> projectiles;
  std::uint64_t ticks, shots, hits;
  float player_pitch;
  bool player_input_enabled, cinematic_skip_enabled;
  bool player_movement_disabled;
  std::map<std::uint32_t, SoundEvent> active_loops;
  std::string mission_segment;
  std::uint32_t random_state;
  SoundClassFades sound_classes;
  std::array<int,5> objectives;
  bool flashlights_allowed, ambience_details_enabled, suppress_ambience_rebuild;
  std::uint64_t flashlights_changed_tick;
  bool suppress_automatic_objects;
  SourceWeatherRecord weather_record;
  PlayerTrainingState training;
  CampaignSubtitle subtitle;
  std::array<CampaignTitle,4> titles;
  bool ai_dialogue_enabled;
  bool level_finished,reset_requested;
  std::array<ImpulseClock,16> impulse_clocks;
  std::map<std::uint32_t,std::vector<std::uint32_t>> script_lists;
  std::uint32_t next_script_list;
  std::optional<SourceHsListBank> multiplayer_script_lists;
  std::map<std::uint32_t,ObjectAnimation> object_animations;
  std::optional<CampaignSourceRuntime::Checkpoint> source_campaign;
  std::optional<CampaignCinematicState> cinematic_state;
};
void Simulation::request_checkpoint() {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if (revert_pending_)
    throw FormatError("checkpoint requested during revert");
  ++authority_calls_.mutations;save_pending_ = true;
}
CampaignFrameTime Simulation::advance_frame_time(float seconds,bool paused) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  auto &state=mutable_cinematic_state();
  CampaignFrameClock clock(state.frame_fraction);
  const auto result=clock.advance(seconds,presentation_.cinematic,paused);
  ++authority_calls_.mutations;state.frame_fraction=result.fraction;return result;
}
float Simulation::frame_fraction()const {
  const auto *state=cinematic_state();return state?state->frame_fraction:0.f;
}
void Simulation::cinematic_screen_consumer(bool ready) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  mutable_cinematic_state().screen_consumer=ready;
}
void Simulation::advance_cinematic_render_time(float seconds,int requested,int completed) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  auto &state=mutable_cinematic_state();
  const auto accepted=accepted_campaign_frame_elapsed(seconds,requested,completed);
  if(source_campaign_)source_campaign_->objects().advance_effect_frame(accepted);
  state.render_clock+=double(accepted);
  if(source_campaign_) {
    auto &world=source_campaign_->objects().tu15_world();
    source_campaign_screen_clock_tu15(world.memory(),state.render_clock);
    source_campaign_screen_update_tu15(world);
    scene_.publish_source_function_samples(source_object_function_samples_tu15(world));
    state.crossfade=source_campaign_crossfade_state_tu15(world.memory());
  } else state.crossfade.update(state.render_clock);
}
void Simulation::advance_cinematic_camera(float raw_seconds) {
  if(!source_campaign_ || !presentation_.scripted)return;
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  source_campaign_camera_frame_tu15(source_campaign_->objects().tu15_world(),raw_seconds);
}
CinematicCrossfadeFrame Simulation::cinematic_crossfade_frame()const {
  if(source_campaign_)return source_campaign_crossfade_frame_tu15(source_campaign_->objects().memory());
  const auto *state=cinematic_state();return state?state->crossfade.frame():CinematicCrossfadeFrame{};
}
void Simulation::cinematic_crossfade_capture_complete() {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(source_campaign_) {
    auto &world=source_campaign_->objects().tu15_world();
    source_campaign_screen_capture_complete_tu15(world);
    mutable_cinematic_state().crossfade=source_campaign_crossfade_state_tu15(world.memory());
  } else mutable_cinematic_state().crossfade.capture_complete();
}
void Simulation::request_revert() {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if (!checkpoint_)
    throw FormatError("no recoverable campaign checkpoint");
  ++authority_calls_.mutations;revert_pending_ = true;
  scripts_->suspend_tick();
}
void Simulation::capture_checkpoint() {
  checkpoint_ = std::make_shared<SavedState>(SavedState{
      world_, bsp_, source_campaign_?std::optional<NativeObjectLifetimes>{}:std::optional(object_lifetimes_), object_bodies_, scene_, devices_, campaign_weapon_pickups_, vehicle_flips_, vehicle_rows_, props_, prop_removals_, presentation_, peace_, player_, actors_,
      scripts_->snapshot(), projectiles_, ticks_, shots_, hits_, player_pitch_,
      player_input_enabled_, cinematic_skip_enabled_, player_movement_disabled_,
      active_loops_, mission_segment_, random_state_, sound_classes_, objectives_,
      flashlights_allowed_, ambience_details_enabled_, suppress_ambience_rebuild_,
      flashlights_changed_tick_,suppress_automatic_objects_,weather_record_,training_,subtitle_,titles_,ai_dialogue_enabled_,level_finished_,reset_requested_,impulse_clocks_,script_lists_,next_script_list_,multiplayer_script_lists_,object_animations_,
      source_campaign_?std::optional(source_campaign_->checkpoint(object_lifetimes_)):std::nullopt,
      cinematic_state()?std::optional(*cinematic_state()):std::nullopt});
  save_pending_ = false;
}
void Simulation::restore_checkpoint() {
  if (!checkpoint_)
    throw FormatError("no recoverable campaign checkpoint");
  const auto &saved = *checkpoint_;
  if (bsp_ != saved.bsp) {
    bsp_ = saved.bsp;
    ++bsp_revision_;
  }
  world_ = saved.world;
  pending_bsp_.reset();
  if(source_campaign_) {
    if(!saved.source_campaign || saved.object_lifetimes)throw FormatError("Campaign checkpoint lost its source world ownership");
    source_campaign_->restore(*saved.source_campaign,object_lifetimes_);
  } else {
    if(!saved.object_lifetimes || saved.source_campaign)throw FormatError("Campaign checkpoint changed its object authority");
    object_lifetimes_=*saved.object_lifetimes;
  }
  object_bodies_=saved.object_bodies;
  scene_.restore(saved.scene);
  devices_=saved.devices;
  campaign_weapon_pickups_=saved.weapon_pickups;
  vehicle_flips_=saved.vehicle_flips;
  vehicle_rows_=saved.vehicle_rows;
  // Transient presentation requests are not replayed from a checkpoint.
  // Keep the native serial monotonic across rewinds observed by this runtime.
  device_events_.clear();(void)devices_.take_events();
  props_=saved.props;
  prop_events_.clear();
  prop_removals_=checkpoint_->prop_removals;
  synchronize_props(true);
  presentation_.restore(saved.presentation);
  peace_ = saved.peace;
  player_ = saved.player;
  actors_ = saved.actors;
  scripts_->restore(saved.scripts);
  projectiles_ = saved.projectiles;
  ticks_ = saved.ticks;
  shots_ = saved.shots;
  hits_ = saved.hits;
  player_pitch_ = saved.player_pitch;
  player_input_enabled_ = saved.player_input_enabled;
  cinematic_skip_enabled_ = saved.cinematic_skip_enabled;
  player_movement_disabled_ = saved.player_movement_disabled;
  active_loops_ = saved.active_loops;
  mission_segment_ = saved.mission_segment;
  random_state_ = saved.random_state;
  sound_classes_ = saved.sound_classes;
  objectives_ = saved.objectives;
  flashlights_allowed_ = saved.flashlights_allowed;
  flashlights_changed_tick_ = saved.flashlights_changed_tick;
  ambience_details_enabled_ = saved.ambience_details_enabled;
  suppress_automatic_objects_ = saved.suppress_automatic_objects;
  // The source24-byte game-state record is restored;398D0 imports it on the
  // next view0 update. Its realtime clock and current globals stay outside it.
  weather_record_ = saved.weather_record;
  training_=saved.training;
  object_animations_=saved.object_animations;
  // Copy the value: the callable's shared service context is not a snapshot.
  const auto *current_cinematic=cinematic_state();
  const double render_clock=current_cinematic?current_cinematic->render_clock:0.;
  const bool screen_consumer=current_cinematic && current_cinematic->screen_consumer;
  if(saved.cinematic_state)mutable_cinematic_state()=*saved.cinematic_state;
  else if(auto *adapter=animation_loader_.target<CinematicAnimationLoader>())adapter->state.reset();
  if(saved.cinematic_state || screen_consumer || render_clock!=0) {
    auto &state=mutable_cinematic_state();state.render_clock=render_clock;state.screen_consumer=screen_consumer;
    if(source_campaign_)source_campaign_screen_clock_tu15(source_campaign_->objects().memory(),render_clock);
  }
  subtitle_=saved.subtitle;titles_=saved.titles;ai_dialogue_enabled_=saved.ai_dialogue_enabled;level_finished_=saved.level_finished;reset_requested_=saved.reset_requested;
  impulse_clocks_=saved.impulse_clocks;
  script_lists_=saved.script_lists;next_script_list_=saved.next_script_list;
  multiplayer_script_lists_=saved.multiplayer_script_lists;
  rumble_events_.clear();
  unit_damage_effects_.clear();
  unit_damage_voices_.clear();
  // Original restore 188AC0 consumes this one-shot flag. Portal ambience
  // traversal remains a separate dependency of the native listener service.
  suppress_ambience_rebuild_ = false;
  sounds_.clear();
  sounds_.push_back({null_datum, {}, 1, SoundEvent::Reset});
  for (const auto &[id, loop] : active_loops_) {
    sounds_.push_back(loop);
    if (loop.alternate)
      sounds_.push_back({id, {}, 1, SoundEvent::LoopAlternate, true});
  }
  save_pending_ = revert_pending_ = false;
  // 0x124620 stamps restored game time outside the checkpoint. 0x2A9D40
  // compares it to the current tick, rather than latching a boolean.
  reverted_tick_ = ticks_;
  ++checkpoint_restores_;
  source_admit_queued_script_sounds_tu15(*this,sounds_);
}
void Simulation::debug_save_snapshot(const std::string& name) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if(multiplayer_weapon_steps_)throw FormatError("multiplayer snapshots need the complete match-state owner");
  if(name.empty() || name.size()>64 || std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32;}))
    throw FormatError("snapshot name must contain 1..64 printable bytes");
  if(save_pending_ || revert_pending_ || pending_bsp_)throw FormatError("wait for the pending campaign transition before saving a snapshot");
  if(!debug_snapshots_.contains(name) && debug_snapshots_.size()>=16)throw FormatError("session snapshot limit is sixteen");
  const auto checkpoint=checkpoint_;
  ++authority_calls_.mutations;
  capture_checkpoint();
  const auto saved=checkpoint_;
  checkpoint_=checkpoint;
  debug_snapshots_.insert_or_assign(name,std::pair{saved,checkpoint});
}
void Simulation::debug_load_snapshot(const std::string& name) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if(multiplayer_weapon_steps_)throw FormatError("multiplayer snapshots need the complete match-state owner");
  const auto it=debug_snapshots_.find(name);
  if(it==debug_snapshots_.end())throw FormatError("session snapshot no longer exists");
  ++authority_calls_.mutations;checkpoint_=it->second.first;
  restore_checkpoint();
  checkpoint_=it->second.second;
}
void Simulation::debug_delete_snapshot(const std::string& name) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if(!debug_snapshots_.erase(name))throw FormatError("session snapshot no longer exists");
  ++authority_calls_.mutations;
}
std::vector<std::string> Simulation::debug_snapshot_names() const {
  std::vector<std::string> names;
  for(const auto& [name,state]:debug_snapshots_)names.push_back(name);
  return names;
}
namespace {
float ray_sphere(Vec3 origin, Vec3 dir, Vec3 center, float radius,
                 float limit) {
  const auto delta = origin - center;
  const float b = dot(delta, dir), c = dot(delta, delta) - radius * radius,
              disc = b * b - c;
  if (c <= 0)
    return 0;
  if (disc < 0)
    return limit;
  const float t = -b - std::sqrt(disc);
  return t >= 0 ? std::min(t, limit) : limit;
}
float ray_body(Vec3 origin, Vec3 dir, const Body &body, float limit) {
  const auto bottom = body.position + Vec3{0, 0, body.radius},
             top = body.position + Vec3{0, 0, body.height - body.radius};
  float result = ray_sphere(origin, dir, bottom, body.radius, limit);
  result = ray_sphere(origin, dir, top, body.radius, result);
  const auto q = origin - bottom;
  const float a = dir.x * dir.x + dir.y * dir.y, b = q.x * dir.x + q.y * dir.y,
              c = q.x * q.x + q.y * q.y - body.radius * body.radius;
  if (a > 1e-8f && b * b - a * c >= 0) {
    const float t = (-b - std::sqrt(b * b - a * c)) / a;
    const float z = origin.z + dir.z * t;
    if (t >= 0 && z >= bottom.z && z <= top.z)
      result = std::min(result, t);
  }
  return result;
}
Vec3 eye(const Unit &unit) {
  return ordinary_unit_eye(unit.body.position, unit.definition.camera_height,
                           unit.definition.crouch_camera_height, unit.crouch);
}
Vec3 center(const Unit &unit) {
  const auto p = unit.definition.bounding_offset;
  return unit.body.position + direction(unit.yaw) * p.x +
         Vec3{-std::sin(unit.yaw), std::cos(unit.yaw), 0} * p.y +
         Vec3{0, 0, p.z};
}
void cube(std::vector<Vertex> &vertices, Vec3 low, Vec3 high, Vec3 color) {
  const Vec3 p[]{low,
                 {high.x, low.y, low.z},
                 {high.x, high.y, low.z},
                 {low.x, high.y, low.z},
                 {low.x, low.y, high.z},
                 {high.x, low.y, high.z},
                 high,
                 {low.x, high.y, high.z}};
  constexpr unsigned indices[]{0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
                               0, 1, 5, 0, 5, 4, 2, 3, 7, 2, 7, 6,
                               1, 2, 6, 1, 6, 5, 3, 0, 4, 3, 4, 7};
  for (auto i : indices)
    vertices.push_back({p[i], color, {0, 0}, 0});
}
} // namespace
Simulation::Simulation(const LevelContent &content, std::size_t bsp,
                       Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign)
    : Simulation(content,bsp,spawn,std::move(loader),std::move(bsp_loader),
        std::move(predictor),scene_game_context,std::move(object_lifetimes),
        std::move(source_scene_owner),std::move(source_campaign),{}) {}
Simulation::Simulation(const LevelContent &content, std::size_t bsp,
                       Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign,
                       ScriptTickRate script_tick_rate)
    : Simulation(content,bsp,spawn,std::move(loader),std::move(bsp_loader),
        std::move(predictor),scene_game_context,std::move(object_lifetimes),
        std::move(source_scene_owner),std::move(source_campaign),std::move(script_tick_rate),{}) {}
Simulation::Simulation(const LevelContent &content, std::size_t bsp,
                       Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign,
                       ScriptTickRate script_tick_rate,ScriptSetMaxRumble script_set_max_rumble)
    : Simulation(content,BspContent{content.collision,content.navigation},bsp,spawn,
        std::move(loader),std::move(bsp_loader),std::move(predictor),scene_game_context,
        std::move(object_lifetimes),std::move(source_scene_owner),std::move(source_campaign),std::move(script_tick_rate),std::move(script_set_max_rumble)) {}
Simulation::Simulation(const LevelContent &content, BspContent initial_geometry,
                       std::size_t bsp, Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign)
    : Simulation(content,std::move(initial_geometry),bsp,spawn,std::move(loader),
        std::move(bsp_loader),std::move(predictor),scene_game_context,
        std::move(object_lifetimes),std::move(source_scene_owner),std::move(source_campaign),{}) {}
Simulation::Simulation(const LevelContent &content, BspContent initial_geometry,
                       std::size_t bsp, Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign,
                       ScriptTickRate script_tick_rate)
    : Simulation(content,std::move(initial_geometry),bsp,spawn,std::move(loader),
        std::move(bsp_loader),std::move(predictor),scene_game_context,
        std::move(object_lifetimes),std::move(source_scene_owner),std::move(source_campaign),std::move(script_tick_rate),{}) {}
Simulation::Simulation(const LevelContent &content, BspContent initial_geometry,
                       std::size_t bsp, Spawn spawn, Presentation::AnimationLoader loader,
                       BspLoader bsp_loader, Predictor predictor,
                       std::optional<SourceSceneGameContext> scene_game_context,
                       std::optional<NativeObjectLifetimes> object_lifetimes,
                       SourceSceneObjectOwner source_scene_owner,std::shared_ptr<CampaignSourceRuntime> source_campaign,
                       ScriptTickRate script_tick_rate,ScriptSetMaxRumble script_set_max_rumble)
    : content_(content), bsp_(bsp),
      world_(std::make_shared<BspWorld>(std::move(initial_geometry))),
      bsp_loader_(std::move(bsp_loader)), predictor_(std::move(predictor)),
      source_campaign_(std::move(source_campaign)),
      object_lifetimes_(source_campaign_?source_campaign_->lifetimes():
          object_lifetimes ? std::move(*object_lifetimes) : NativeObjectLifetimes{}),
      object_bodies_(source_campaign_?SourceObjectBodyStorage::deferred_default():SourceObjectBodyStorage{}),
      scene_(content,bsp,source_campaign_?std::optional(SourceSceneGameContext{1,0}):scene_game_context,{},&object_lifetimes_,
          source_campaign_?source_campaign_->scene_owner(content):std::move(source_scene_owner)),devices_(content),props_(content),scenery_collision_(content),
      presentation_(content, loader),animation_loader_(std::move(loader)) {
  allocate_object_=[this](std::uint8_t type){return object_lifetimes_.reserve(type);};
  publish_object_=[this](std::uint32_t handle,std::uint32_t definition){object_lifetimes_.publish_native(handle,definition);};
  release_object_=[this](std::uint32_t handle){object_lifetimes_.release(handle);};
  if(source_campaign_) {
    source_physics_movement_=multiplayer_weapon_steps_=true;
    source_vehicle_seat_query_=[this](auto unit,auto vehicle){return source_campaign_->objects().vehicle_seat_target(unit,vehicle);};
    source_device_script_=[this](const auto &name,const auto &arguments){return source_campaign_->objects().script_device(name,arguments);};
  }
  for (int i = 0; i < 16; ++i)
    peace_[i][i] = true;
  if(!source_campaign_)peace_[1][2] = peace_[2][1] = true;
  if (content.player.datum == null_datum || content.player.radius <= 0 ||
      content.player.height < 2 * content.player.radius)
    throw FormatError("selected map has no supported player biped");
  vehicle_rows_.resize(content.netgame_equipment.size());
  // An attached source world constructs the real player/equipment later.
  // A legacy respawn here would reserve and publish a bodyless weapon in
  // that same original pool before the source player birth consumes it.
  if(scene_.source_factory_bound() && !source_campaign_) {
    player_={};player_.definition=content_.player;
    player_.exists=false;player_.vitality.dead=true;
    player_.body.height=content_.player.height;player_.body.radius=content_.player.radius;
    player_.body.maximum_slope_cosine=content_.player.maximum_slope_cosine;
  } else {
    if(source_campaign_)detail::release_native_startup_code_pages();
    respawn(spawn);
    if(source_campaign_)detail::release_native_startup_code_pages();
  }
  devices_.synchronize(scene_);
  synchronize_props(true);
  synchronize_weapon_pickups();
  if(!script_tick_rate && source_campaign_)
    script_tick_rate=[owner=source_campaign_]{return source_script_tick_rate(owner->objects().memory());};
  if(!script_set_max_rumble && source_campaign_)
    script_set_max_rumble=[owner=source_campaign_](float left,float right) {
      auto &world=owner->objects().tu15_world();
      source_script_max_rumble_tu15(world.memory(),[&](auto address,auto bytes){
        world.write_source_call_input(address,bytes);},left,right);
    };
  const auto clock_builtins=mcc_script_clock_builtins(std::move(script_tick_rate));
  scripts_ = std::make_unique<ScriptVm>(
      content.scripts,
      [this,clock_builtins,script_set_max_rumble=std::move(script_set_max_rumble)](const auto &name, const auto &args) {
        if(clock_builtins)if(auto value=clock_builtins(name,args))return *value;
        if(name=="player_effect_set_max_rumble") {
          if(args.size()!=2 || !script_set_max_rumble)
            throw FormatError("Script rumble requires two REAL arguments and its original effect owner");
          script_set_max_rumble(std::bit_cast<float>(scalar_cast_bits(args[0].bits,args[0].type,6)),
              std::bit_cast<float>(scalar_cast_bits(args[1].bits,args[1].type,6)));
          return ScriptValue{};
        }
        return host(name,args);
      },
      [this](int name, unsigned type) {
        const auto id = scene_.resolve(name);
        if (id != null_datum) {
          const auto kind = content_.objects[scene_.get(id).placement].type;
          const bool valid =
              type == 50 || (type == 51 && (kind == 0 || kind == 1)) ||
              (type == 52 && kind == 1) || (type == 53 && kind == 2) ||
              (type == 54 && kind >= 7 && kind <= 9) ||
              (type == 55 && kind == 6);
          if (!valid)
            return null_datum;
        }
        return id;
      }, [](unsigned index, const std::optional<ScriptValue> &value) {
        // Retail external 0x3E3 is a short with a NULL backing address.
        // 0x20A490 ignores its write; 0x20A2E0 refreshes reads to zero.
        if (index != 0x3e3 || (value && (value->type < 5 || value->type > 8)))
          throw FormatError("unsupported external campaign global " +
                            std::to_string(index));
        if (value)
          return ScriptValue{7,scalar_cast_bits(value->bits,value->type,7),{}};
        return ScriptValue::numeric(0, 7);
      },SourceHsObjectListServices{
        [this]{if(multiplayer_script_lists_){auto roots=scripts_->object_list_roots();std::erase(roots,0x80000000u);multiplayer_script_lists_->roots(roots);return multiplayer_script_lists_->allocate();}if(script_lists_.size()>=65536 || next_script_list_==null_datum)throw FormatError("Native script list bank exhausted");const auto id=next_script_list_++;script_lists_.emplace(id,std::vector<std::uint32_t>{});return id;},
        [this](std::uint32_t list,std::uint32_t object){if(multiplayer_script_lists_)multiplayer_script_lists_->append(list,object);else{(void)object_center(object);script_lists_.at(list).push_back(object);}}},clock_builtins);
  scripts_->set_random_choice([this](unsigned count,std::uint32_t visited) {
    if(source_campaign_)return source_campaign_->random_choice(count,visited);
    return source_hs_begin_random(random_state_,count,visited);
  });
  scripts_->command_ready([this](std::uint32_t actor){return !script_unit(actor).command.goal.has_value();});
  for (const auto &squad : content.squads)
    if (squad.flags & (1 << 12))
      place_squad(squad.name);
}
void Simulation::respawn(Spawn spawn) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(source_campaign_){source_campaign_->respawn(*this,spawn);return;}
  const auto equipped = player_.weapon.definition;
  if(!content_.multiplayer_spawn)
    for(const auto handle:player_.source_inventory.slots)if(handle!=null_datum) {
      const auto placed=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==handle;});
      if(placed!=scene_.objects().end())scene_.destroy(handle);else object_lifetimes_.release(handle);
    }
  player_ = {};
  player_.definition = content_.player;
  player_.vitality.body = content_.player.vitality.health > 0 ? 1.f : 0.f;
  player_.vitality.shield = content_.player.vitality.shield > 0 ? 1.f : 0.f;
  player_.team = 1;
  player_.bsp = int(bsp_);
  player_.yaw = spawn.facing;
  player_pitch_ = 0;
  player_.body = {
      spawn.position,         {},    content_.player.height,
      content_.player.radius, false, content_.player.maximum_slope_cosine};
  const auto ground =
      world_->collision->raycast(spawn.position + Vec3{0, 0, .1f}, {0, 0, -1}, 3);
  if (ground && ground->normal.z >= player_.body.maximum_slope_cosine)
    player_.body.position.z = ground->position.z + .002f;
  if (equipped != null_datum && content_.weapons.contains(equipped))
    equip(equipped);
  else if (content_.starting_weapon != null_datum &&
           content_.weapons.contains(content_.starting_weapon))
    equip(content_.starting_weapon);
}
void Simulation::set_team_allegiance(int a,int b,bool allied) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(a<0 || b<0 || a>=16 || b>=16)throw FormatError("team outside allegiance table");
  if(source_campaign_)source_campaign_->set_team_allegiance(std::int16_t(a),std::int16_t(b),allied);
  peace_[a][b]=peace_[b][a]=allied;
}
void Simulation::set_difficulty(unsigned value) {
  if(value>3)throw FormatError("invalid campaign difficulty");
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(source_campaign_)source_campaign_->set_difficulty(value);
  difficulty_=value;
}
void Simulation::equip(std::uint32_t id) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  const auto &weapon = content_.weapons.at(id);
  ++authority_calls_.mutations;
  if(source_campaign_){source_campaign_->equip(*this,id);return;}
  if(!content_.multiplayer_spawn)equip_inventory(player_,id);
  else player_.weapon = {id, weapon.capacity, std::max(0, weapon.reserve), 0, 0, 0};
}
std::size_t Simulation::place_squad(const std::string &name) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  const auto found =
      std::find_if(content_.squads.begin(), content_.squads.end(),
                   [&](const auto &s) { return s.name == name; });
  if (found == content_.squads.end())
    throw FormatError("squad not found: " + name);
  const auto index = std::size_t(found - content_.squads.begin());
  if(source_campaign_)return source_campaign_->place_squad(*this,index);
  if (found->bsp >= 0 && std::size_t(found->bsp) != bsp_)
    return 0;
  if (std::any_of(actors_.begin(), actors_.end(), [&](const auto &a) {
        return a.squad == index && a.exists && !a.vitality.dead;
      }))
    return 0;
  std::size_t count = 0;
  ++authority_calls_.mutations;
  for (const auto &start : found->starts) {
    if (int(count) >= found->count && !(start.flags & 8))
      continue;
    const auto c = content_.characters.find(start.character);
    if (c == content_.characters.end() || c->second.biped.radius <= 0)
      continue;
    // 1E0850 supplies start+4 to210850: resolve its current object-node
    // reference frame, falling back to the authored local point if unavailable.
    const auto position=reference_point(start.position,start.reference_frame).value_or(start.position);
    const auto ground =
        world_->collision->raycast(position + Vec3{0, 0, .1f}, {0, 0, -1}, 3);
    if (!ground || ground->normal.z < c->second.biped.maximum_slope_cosine)
      continue; // no guessed BSP assignment or airborne spawn
    if (actors_.size() >= 256)
      throw FormatError("actor capacity exhausted");
    Unit u;
    u.definition = c->second.biped;
    u.character = start.character;
    u.bsp = int(bsp_);
    u.vitality.body = u.definition.vitality.health > 0 ? 1.f : 0.f;
    u.vitality.shield = u.definition.vitality.shield > 0 ? 1.f : 0.f;
    u.body = {position,      {},    u.definition.height,
              u.definition.radius, false, u.definition.maximum_slope_cosine};
    u.body.position.z = ground->position.z + .002f;
    u.yaw = start.facing;
    u.squad = index;
    u.team = found->team ? found->team : u.definition.team;
    u.source_datum=object_lifetimes_.reserve(0);
    if(u.source_datum==null_datum)throw FormatError("Actor object pool exhausted");
    object_lifetimes_.publish_native(u.source_datum,u.definition.datum);
    if (const auto w = content_.weapons.find(start.weapon);
        w != content_.weapons.end())
      if(!content_.multiplayer_spawn)equip_inventory(u,w->first);
      else u.weapon = {w->first,w->second.capacity,std::max(0,w->second.reserve),0,0,0};
    actors_.push_back(u);
    ++count;
  }
  return count;
}
Unit &Simulation::script_unit(std::uint32_t handle) {
  if(handle==0)return player_;
  for(auto &actor:actors_)if(actor.exists && actor.source_datum==handle)return actor;
  throw FormatError("Script unit is not a live native object");
}
Vec3 Simulation::object_center(std::uint32_t handle)const {
  const auto unit_center=[&](const Unit &unit) {
    if(content_.unit_interactions)return native_unit_query_sphere(unit,
        content_.unit_interactions->units.at(unit.definition.datum),ticks_).center;
    return halo2::center(unit); // Explicit model-free diagnostic fixtures.
  };
  if(handle==0)return unit_center(player_);
  for(const auto &actor:actors_)if(actor.exists && actor.source_datum==handle)return unit_center(actor);
  const auto *animation=object_animation(handle);
  const auto *cinematic=cinematic_state();
  if((animation && animation->channel) || (cinematic && cinematic->attachments.contains(handle))) {
    const auto &object=scene_.get(handle);
    if(object.solved_nodes && !object.solved_nodes->empty()) {
      const auto &node=object.solved_nodes->front();const auto offset=content_.objects.at(object.placement).bounding_offset;
      return node.position+(node.basis[0]*offset.x+node.basis[1]*offset.y+node.basis[2]*offset.z)*node.scale;
    }
  }
  return scene_.center(handle);
}
std::vector<std::uint32_t> Simulation::script_objects(std::uint32_t list)const {
  if(list==null_datum)return {};
  if(list==0x80000000u)return {0};
  if(multiplayer_script_lists_)return multiplayer_script_lists_->objects(list);
  if(const auto at=script_lists_.find(list);at!=script_lists_.end())return at->second;
  throw FormatError("Script object list is not live");
}
std::optional<Vec3> Simulation::reference_point(Vec3 position,std::int16_t reference_frame)const {
  if(reference_frame==-1)return position;
  if(bsp_>=content_.command_frames.size())return std::nullopt;
  for(const auto &frame:content_.command_frames[bsp_])if(frame.frame==reference_frame) {
    for(const auto &[handle,owner]:devices_.owners())if(owner.placement==frame.placement) {
      const auto &definition=*content_.devices.at(owner.placement);
      if(!definition.machine || !definition.collision)return std::nullopt;
      const auto &model=definition.collision->hierarchy;
      const auto locals=source_machine_local_pose(model,*definition.machine,owner.motion.position,owner.motion.power);
      const auto nodes=pose_model_locals_nodes_world(model,owner.root,locals);
      if(frame.node>=nodes.size())return std::nullopt;
      // Unchanged original210D60 table at440BB8, including all six orders.
      constexpr unsigned axes[6][3]{{2,1,0},{2,0,1},{0,2,1},{1,2,0},{1,0,2},{0,1,2}};
      const int order=int(frame.axis)*2+frame.order;if(order<0 || order>=6)throw FormatError("Invalid command reference axes");
      const std::array p{position.x,position.y,position.z};const auto &t=nodes[frame.node];const auto &a=axes[order];
      return Vec3{t.position+(t.basis[0]*p[a[0]]+t.basis[1]*p[a[1]]+t.basis[2]*p[a[2]])*t.scale};
    }
    return std::nullopt;
  }
  return std::nullopt;
}
Vec3 Simulation::script_point(std::uint32_t reference)const {
  const auto set=reference>>16,index=reference&65535;
  if(set>=content_.command_points.size() || index>=content_.command_points[set].size())throw FormatError("Command point outside scenario");
  const auto &point=content_.command_points[set][index];
  if(const auto position=reference_point(point.position,point.reference_frame))return *position;
  throw FormatError("Command point has no current machine reference-frame owner");
}
void Simulation::update_command(Unit &actor) {
  auto &command=actor.command;Vec3 desired{};
  if(command.goal_reference)command.goal=script_point(*command.goal_reference);
  if(command.face_reference)command.face_point=script_point(*command.face_reference);
  if(command.look_reference)command.look_point=script_point(*command.look_reference);
  if(command.goal) {
    const auto delta=*command.goal-actor.body.position;
    // The existing native navigation/collision backend owns movement. This
    // radius admission is shared with its ordinary AI path follower.
    if(std::hypot(delta.x,delta.y)<=actor.body.radius*2) {command.goal.reset();command.goal_reference.reset();actor.route.clear();}
    else {
      if(actor.route.empty() || ticks_%30==0){actor.route=world_->navigation.route(actor.body.position,*command.goal);actor.waypoint=0;}
      while(actor.waypoint<actor.route.size() && std::hypot(actor.route[actor.waypoint].x-actor.body.position.x,actor.route[actor.waypoint].y-actor.body.position.y)<=actor.body.radius*2)++actor.waypoint;
      if(actor.waypoint<actor.route.size() || content_.navigation.empty()) {
        const auto d=actor.waypoint<actor.route.size()?actor.route[actor.waypoint]-actor.body.position:delta;
        desired=normalized(Vec3{d.x,d.y,0})*content_.movement.forward;
        actor.yaw=std::atan2(d.y,d.x);
      }
    }
  }
  std::optional<Vec3> target=command.face_point;
  if(command.face_object)target=object_center(*command.face_object);
  if(command.face_player)target=eye(player_);
  if(target){const auto d=*target-eye(actor);actor.yaw=std::atan2(d.y,d.x);}
  target=command.look_point;if(command.look_object)target=object_center(*command.look_object);
  if(target){const auto d=*target-eye(actor);actor.pitch=std::atan2(d.z,std::hypot(d.x,d.y));}
  move_unit(actor,desired,false,false);
}
void Simulation::move_unit(Unit &u, Vec3 desired, bool jump, bool crouch,
                           bool player_control) {
  if(source_physics_movement_ && u.source_datum!=null_datum)return;
  if(player_control && debug_settings_for(u).noclip) {
    const auto intent=movement_intent(content_.movement,desired,0);
    const auto facing=direction(u.yaw,&u==&player_?player_pitch_:u.pitch);
    const Vec3 side{-std::sin(u.yaw),std::cos(u.yaw),0};
    u.body.velocity=facing*intent.velocity.x+side*intent.velocity.y+
        Vec3{0,0,content_.movement.forward*(float(jump)-float(crouch))};
    u.body.position=u.body.position+u.body.velocity*tick_seconds;
    u.body.grounded=false;
    u.landing={};
    return;
  }
  const bool landing_enabled=content_.original_data && content_.original_data->unit_locomotion_deadzone_bits;
  if(landing_enabled) {
    const float magnitude=desired.x*desired.x+desired.y*desired.y+desired.z*desired.z;
    advance_source_landing(u.landing,magnitude>std::bit_cast<float>(*content_.original_data->unit_locomotion_deadzone_bits));
  }
  const auto move_and_land=[&] {
    const bool supported=u.body.grounded;
    collision().move(u.body,tick_seconds);
    // Explicit native capsule-contact adaptation of DEFC0's incoming impact
    // operand. The retained timer and authored thresholds are E5960's rules.
    if(landing_enabled && !supported && u.body.grounded && u.body.landing_speed>0) {
      bool force_hard=false;
      begin_source_landing(u.landing,u.definition.landing,u.body.landing_speed,30,force_hard);
    }
  };
  const float proposed =
      u.crouch + crouch_delta(u.crouch, tick_seconds,
                              u.definition.crouch_velocity, crouch, false);
  const float height =
      capsule_segment_height(u.definition.height, u.definition.crouch_height,
                             u.body.radius, proposed,
                             (u.definition.physics_flags & 2) != 0) +
      2 * u.body.radius;
  if (proposed >= u.crouch ||
      !collision().overlaps(u.body.position, height, u.body.radius)) {
    u.crouch = proposed;
    u.body.height = std::max(2 * u.body.radius, height);
  }
  if (player_control && !(u.definition.physics_flags & 8)) {
    const auto intent = movement_intent(content_.movement, desired, u.crouch,
                                        false, !u.body.grounded);
    if (jump_gate(u.jump, jump, true, !u.body.grounded, u.landing.kind==1, u.body.grounded,
                  tick_seconds, 30)) {
      u.body.velocity = jump_impulse(u.body.velocity, {0, 0, 1}, {},
                                     u.definition.jump_velocity);
      u.body.grounded = false;
    }
    WalkingInput input;
    input.input = intent.velocity;
    input.current = u.body.velocity;
    input.facing = input.basis = direction(u.yaw);
    input.normal = u.body.support_normal;
    input.slope = u.definition.slope;
    input.acceleration = intent.acceleration;
    input.air_acceleration = intent.air_acceleration;
    input.gravity = retail_gravity;
    input.dt = tick_seconds;
    input.flags = 8 | (u.body.grounded ? 0 : 4);
    u.body.velocity = walking_velocity(input).velocity;
    move_and_land();
    return;
  }
  const auto delta = desired - Vec3{u.body.velocity.x, u.body.velocity.y, 0};
  const auto acceleration =
      u.body.grounded ? (u.crouch > .5f ? content_.movement.crouch_acceleration
                                        : content_.movement.acceleration)
                      : content_.movement.air_acceleration;
  const auto change =
      normalized(delta) * std::min(length(delta), acceleration * tick_seconds);
  u.body.velocity.x += change.x;
  u.body.velocity.y += change.y;
  if (jump && u.body.grounded)
    u.body.velocity.z = u.definition.jump_velocity;
  u.body.velocity.z -= retail_gravity * tick_seconds;
  move_and_land();
}
float Simulation::player_damage_scale(const DamageOwner& owner,std::uint32_t victim)const {
  if(!multiplayer_weapon_steps_)return 1.f;
  if(!player_damage_scale_)throw FormatError("Multiplayer damage requires its actual player/variant owner");
  return player_damage_scale_(owner,victim);
}
void Simulation::disrupt_camouflage(Unit& unit,float amount,float limit) {
  SourceCamouflageServices services;
  services.unit=[&](std::uint32_t datum) {
    if(datum!=unit.source_datum)throw FormatError("Camouflage disruption lost its native object owner");
    auto& state=unit.source_camouflage;
    // Intrinsic suit activation and actual Xbox entity replication are not
    // installed in this native branch. Ordinary cold units have both NONE/0.
    return SourceCamouflageUnitView{unit.source_action_word,state.primary,state.secondary,
        state.fade_speed,null_datum,0};
  };
  source_camouflage_disrupt(unit.source_datum,amount,limit,services);
}
void Simulation::record_unit_damage_effect(const Unit &unit,const UnitDamageEffect &effect,
    const DamageOwner &owner) {
  unit_damage_effects_.push_back({++unit_damage_effect_serial_,ticks_+1,unit.source_datum,effect,owner});
  if(unit_damage_effects_.size()>64)unit_damage_effects_.erase(unit_damage_effects_.begin());
}
void Simulation::damage(Unit &unit, const WeaponDefinition &w, const DamageOwner &owner,int native_owner,
    std::optional<float> supplied_amount,bool radius,std::optional<DamageImpactContext> impact) {
  // Unit impervious gates reactions/impulses, not vitality. The separate
  // object damage-protection bit is the immunity checked by retail 0xD9020.
  if (unit.vitality.dead || unit.cannot_take_damage || debug_settings_for(unit).invulnerable ||
      (unit.team == 1 && (w.damage_flags & (1 << 15))))
    return;
  float remaining = supplied_amount ? *supplied_amount : direct_damage_amount(w);
  if(native_owner>=-1 && native_owner<15 && debug_settings_.players[unsigned(native_owner+1)].maximum_damage)
    remaining=std::max(remaining,1000000.f);
  // D90E0 returns unit+13C directly for a valid biped/vehicle. It follows
  // parents only for other object kinds; this target is an actual native Unit.
  remaining=remaining*player_damage_scale(owner,unit.source_player);
  const bool was_shield_depleted=unit.vitality.shield_depleted;
  const auto result = apply_unit_damage(unit.vitality, unit.definition.vitality, w,
      content_.damage_groups, content_.damage_materials, remaining,radius,std::uint16_t(unit.source_unit_10b)<<8);
  for(const auto &effect:result.effects())
    record_unit_damage_effect(unit,effect,effect.kind==UnitDamageEffectKind::ShieldDepleted?DamageOwner{}:owner);
  // D6A70 clears+F4 before D9490/D9640 consumes the normalized report.
  if(!was_shield_depleted && unit.vitality.shield_depleted)
    unit.damage_feedback.shield.visible=0.f;
  const auto prior_feedback=unit.damage_feedback;
  if(multiplayer_weapon_steps_ && (result.body>0 || result.shield>0)) {
    // CB410 takes this value from the selected jpt! definition+30, before
    // the later zoom reset. Friendly-fire rejection produces no request.
    disrupt_camouflage(unit,w.damage_camouflage_amount,0.f);
    // CB410 reads these original +44/+48 normalized requests, including
    // overflow, before its C9A40(0) reset. Vitality loss is not the predicate.
    source_zoom_reset(unit.source_zoom);
    ++unit.zoom_reset_serial;
  }
  report_damage_feedback(unit.damage_feedback,result.body,result.shield,result.report_flags);
  if(!(result.report_flags&0x20u) && (result.body>0 || result.shield>0 || (result.report_flags&1))) {
    unit_damage_voices_.push_back({++unit_damage_voice_serial_,ticks_+1,unit.source_datum,w.damage,
        result.report_flags,unit.body.position,unit.damage_feedback.body.accumulated,
        unit.damage_feedback.shield.visible,unit.damage_feedback.body.visible});
    if(unit_damage_voices_.size()>64)unit_damage_voices_.erase(unit_damage_voices_.begin());
  }
  if((result.body>0 || result.shield>0) && w.damage!=null_datum)
    if(rumble_events_.size()<256)rumble_events_.push_back({w.damage,unit.vitality.shield>0,1.f,&unit==&player_?-1:int(&unit-actors_.data())});
  if (unit.vitality.dead) {
    if(grenade_sandbox_ && !source_physics_movement_)grenade_sandbox_->cancel_unit(unit);
    unit.state = Unit::Dead;
    // Original ordinary CBDC0→E6050→1C70D0 rebuild retains object+88.
    // Keep that momentum; corpse motion/ten-body ragdoll ownership is separate.
  }
  if (multiplayer_weapon_steps_ && damage_hook_)
    damage_hook_(unit, owner, result,w,impact,prior_feedback);
  ++hits_;
}
void Simulation::area_damage(const PropAreaDamageDefinition& definition,Vec3 origin,
    const DamageOwner& owner,int native_owner,std::uint32_t ignored_scene_prop,std::uint32_t projectile_definition){
  NativeAuthorityBoundary::Call active(authority_calls_);
  const auto& w=definition.damage;
  if(!w.damage_amount || w.damage_amount->upper_minimum!=w.damage_amount->upper_maximum)
    throw FormatError("Area damage requires original random upper-bound owner");
  ++authority_calls_.mutations;
  const auto amount=[&](Vec3 point){
    const auto normalized=pickup_side_normalizer(point-origin);
    if(normalized.returned>prop_area_candidate_radius(definition))return 0.f;
    return prop_area_fraction(definition,normalized.returned);
  };
  const auto query=[&](const Unit* ignored_unit,std::uint32_t ignored_prop){return [&,ignored_unit,ignored_prop](Vec3 start,Vec3 delta,std::uint32_t){
    const float distance=length(delta);if(!(distance>0))return PropAreaCastResult{false,start+delta};
    const auto direction=delta/distance;float fraction=1;bool hit=false;
    if(const auto terrain=static_collision_->raycast(start,direction,distance)){
      fraction=terrain->distance/distance;hit=true;
    }
    if(const auto prop=props_.raycast(start,delta,fraction,ignored_scene_prop,ignored_prop)){
      fraction=prop->collision.fraction;hit=true;
    }
    // Same live native capsules used by the existing bullet backend. The
    // source raw Havok body/contact dispatcher is not represented here.
    const auto test=[&](const Unit& unit){
      if(!unit.exists || unit.bsp_hidden || &unit==ignored_unit)return;
      const auto d=ray_body(start,direction,unit.body,distance*fraction);
      if(d<distance*fraction){fraction=d/distance;hit=true;}
    };
    test(player_);for(const auto& unit:actors_)test(unit);
    return PropAreaCastResult{hit,start+delta*fraction};
  };};
  const auto apply=[&](Unit& unit){
    if(!unit.exists || unit.bsp_hidden || unit.vitality.dead ||
        ((definition.flags&2) && unit.source_player==null_datum) ||
        ((w.damage_flags&1) && unit.source_datum==owner.unit))return;
    // Explicit host nearest-point service over the current authored biped
    // capsule. It supplies geometry, never a recorded original contact.
    const auto& body=unit.body;
    Vec3 axis=body.position;axis.z=std::clamp(origin.z,body.position.z+body.radius,
        body.position.z+body.height-body.radius);
    const auto displacement=origin-axis;const float distance=length(displacement);
    const auto target=distance<=body.radius?origin:axis+displacement*(body.radius/distance);
    const float fraction=amount(target);if(!(fraction>0))return;
    if(prop_area_occluded(origin,target,true,definition.core_radius,query(&unit,null_datum)))return;
    const float damage_request=damage_amount(w.damage_amount->lower,w.damage_amount->upper_minimum,fraction,1,1);
    unit.last_damager=native_owner;
    damage(unit,w,owner,native_owner,damage_request,true,
        DamageImpactContext{DamageImpactContext::Kind::Radius,{},origin,
            (definition.flags&1u)?1.f:fraction,projectile_definition});
  };
  apply(player_);for(auto& unit:actors_)apply(unit);
  if(!(definition.flags&2))for(const auto& prop:props_.states()){
    if(prop.vitality.dead || prop.handle==ignored_scene_prop)continue;
    const auto target=props_.closest_point(prop.handle,origin);
    const float fraction=amount(target);if(!(fraction>0))continue;
    if(prop_area_occluded(origin,target,false,definition.core_radius,query(nullptr,prop.handle)))continue;
    // Authored native scene props have no object parent/controller; the
    // original target resolver returns NONE, preserving only attacker strength.
    props_.damage(prop.handle,w,damage_amount(w.damage_amount->lower,w.damage_amount->upper_minimum,fraction,1,
        player_damage_scale(owner,null_datum)),
        {owner.player,owner.unit,owner.designator,native_owner},true,{.random=&random_state_});
  }
}
bool Simulation::trace(Vec3 origin, Vec3 dir, float distance,
                       const WeaponDefinition &weapon, int owner, const DamageOwner &captured,
                       std::uint32_t projectile,const Vec3 *velocity,float traveled) {
  if(!(distance>0))return false;
  dir = normalized(dir);
  const auto incoming_velocity=velocity?*velocity:dir*weapon.projectile_speed;
  const float speed=length(incoming_velocity);
  const auto incoming=velocity?normalized(*velocity):dir;
  const auto retain_impact=[&](BulletImpact hit,
      std::optional<SourceProjectileDamagePresentation> object_presentation={}){
    if(object_presentation && object_presentation->material>=0 &&
        std::size_t(object_presentation->material)<content_.material_parents.size())
      hit.material=object_presentation->material;
    const auto index=source_projectile_material_response(weapon.projectile_material_responses,
        content_.material_parents,hit.material);
    // Terrain starts scaleB at zero; admitted parentless prop damage retains
    // the original material/scale consumer after its vitality update.
    if((hit.terrain || object_presentation) &&
        content_.original_data && content_.original_data->unit_animation_constants){
      const auto damage_scale=source_projectile_contact_damage_scale(weapon.projectile_flags,
          origin,hit.position,traveled,weapon.projectile_dry_damage_range,1.f);
      const auto object_scale=object_presentation?object_presentation->scale:0.f;
      if(!index){
        hit.presentation=source_projectile_default_impact_presentation(hit.normal,incoming_velocity,
            damage_scale,object_scale);
      }else{
        const auto raw=std::span<const std::byte>(weapon.projectile_material_responses).subspan(*index*0x58,0x58);
        const auto &constants=*content_.original_data->unit_animation_constants;
        const SourceProjectileResponseServices services{
            std::bit_cast<float>(constants.one_bits),std::bit_cast<float>(constants.rng_scale_bits),
            [&]{return std::uint8_t(11);},[&]()->std::uint32_t&{return random_state_;}};
        const auto angles=source_projectile_impact_angles(raw,hit.normal,hit.incoming,incoming_velocity,services);
        const auto response=choose_source_projectile_response(raw,{hit.terrain?1:4,angles.angle,speed},services);
        // Native projectiles currently end at their first contact. Their
        // original constructor scale is one, and dry body+170 retains the
        // authored air damage range, independently of current velocity.
        const auto scale=source_projectile_effect_scale(raw,angles.angle,damage_scale);
        hit.presentation=SourceProjectileImpactPresentation{response,scale,
            std::clamp(object_scale,0.f,1.f),angles.approach>.25f};
      }
    }
    bullet_impacts_.push_back(std::move(hit));
    if(bullet_impacts_.size()>64)bullet_impacts_.erase(bullet_impacts_.begin());
  };
  const auto world = static_collision_->raycast(origin, dir, distance);
  float nearest = world ? world->distance : distance;
  const auto prop=props_.raycast(origin,dir*distance,nearest/distance);
  if(prop)nearest=prop->collision.fraction*distance;
  Unit *target = nullptr;
  auto is_owner = [&](const Unit &unit, int slot) {
    return multiplayer_weapon_steps_ && captured.unit != null_datum
        ? unit.source_datum == captured.unit : owner == slot;
  };
  if (!is_owner(player_, -1) && player_.exists && !player_.vitality.dead) {
    const float d = ray_body(origin, dir, player_.body, nearest);
    if (d < nearest) {
      nearest = d;
      target = &player_;
    }
  }
  for (std::size_t i = 0; i < actors_.size(); ++i)
    if (!is_owner(actors_[i], int(i)) && actors_[i].exists && !actors_[i].vitality.dead && !actors_[i].bsp_hidden) {
      const float d = ray_body(origin, dir, actors_[i].body, nearest);
      if (d < nearest) {
        nearest = d;
        target = &actors_[i];
      }
    }
  if (target) {
    target->last_damager=owner;
    damage(*target, weapon, captured, owner,{},false,
        DamageImpactContext{DamageImpactContext::Kind::Direct,incoming,origin+dir*nearest,1.f,weapon.projectile});
    return true;
  }
  if(prop){
    float amount=direct_damage_amount(weapon);
    amount=amount*player_damage_scale(captured,null_datum);
    const auto damage=props_.damage_contact(prop->handle,weapon,amount,
        {captured.player,captured.unit,captured.designator,owner},false,{.random=&random_state_});
    retain_impact({++bullet_impact_serial_,ticks_+1,projectile,weapon.datum,
        origin+dir*nearest,prop->collision.plane.normal,incoming,
        prop->collision.global_material,false},damage.presentation);
    ++hits_;return true;
  }
  if(world){
    // The actual closest terrain contact supplies the global material and
    // plane. Retention is a native transport bank, independent of shot rate.
    retain_impact({++bullet_impact_serial_,ticks_+1,projectile,weapon.datum,
        world->position,world->normal,incoming,std::int16_t(world->material),true});
  }
  return bool(world);
}
std::optional<unsigned> Simulation::select_weapon_firing_effect(WeaponState &state,const WeaponDefinition &w) {
  if(!w.firing_effects.empty()) {
    // TU1.1 10741D..10767E: one retained entry, its repeat count and
    // visited mask. Random selection/repeat consume the actual shared LCG.
    const auto n=w.firing_effects.size();if(n>16)throw FormatError("Firing effect bank exceeds its source visited mask");
    const auto random=[&]{random_state_=random_state_*1664525u+1013904223u;return random_state_>>16;};
    if(state.firing_effect_remaining<=0) {
      int index=w.random_firing_effects?int(random()%n):state.firing_effect_index;
      if(state.firing_effect_visited==((1u<<n)-1))state.firing_effect_visited=0;
      unsigned tries=0;do {
        index=(index+1)%int(n);
        if(state.firing_effect_visited&(1u<<index))continue;
        const auto &row=w.firing_effects[index];state.firing_effect_index=std::int16_t(index);state.firing_effect_visited|=1u<<index;
        state.firing_effect_remaining=row.minimum==row.maximum?row.minimum:std::int16_t(row.minimum+((row.maximum-row.minimum)*random()>>16));
      }while(state.firing_effect_remaining<=0 && ++tries<n*2);
    }
    if(state.firing_effect_remaining>0) {
      --state.firing_effect_remaining;
      return unsigned(state.firing_effect_index);
    }
  }
  return {};
}
void Simulation::shot(Unit &unit, int owner, float pitch,
                      const Vec3 *prepared_direction, const Vec3 *prepared_velocity, WeaponState *selected,
                      bool effects_prepared,std::int16_t barrel_index) {
  auto &state=selected?*selected:unit.weapon;
  const auto &base=content_.weapons.at(state.definition);
  auto w=barrel_index<0?base:*base.barrel_views.at(unsigned(barrel_index));
  const auto &selected_barrel=barrel_index==1?state.alternate_barrel:state.smg.barrel;
  if(multiplayer_weapon_steps_ && native_hand_weapon(unit,1)) {
    w.damage_min*=w.dual_damage_scale;w.damage_max*=w.dual_damage_scale;
    if(w.damage_amount) {
      w.damage_amount->lower*=w.dual_damage_scale;
      w.damage_amount->upper_minimum*=w.dual_damage_scale;
      w.damage_amount->upper_maximum*=w.dual_damage_scale;
    }
  }
  const auto origin = eye(unit);
  const DamageOwner captured{unit.source_player, unit.source_datum, unit.source_membership};
  ++shots_;
  if(!multiplayer_weapon_steps_ && unit.source_zoom.current!=-1 &&
      (w.zoom.levels==0 || (state.magazine.mode>=1 && state.magazine.mode<=6))) {
    source_zoom_reset(unit.source_zoom);++unit.zoom_reset_serial;
  }
  state.last_shot = ticks_ + 1; // presentation observes the completed tick
  if(!prepared_direction && !effects_prepared)
    if(const auto index=select_weapon_firing_effect(state,w);index &&
        w.firing_effects[*index].damage_effects[0]!=null_datum && rumble_events_.size()<256)
      rumble_events_.push_back({w.firing_effects[*index].damage_effects[0],unit.vitality.shield>0,1.f,owner});
  if (w.sound != null_datum && sounds_.size() < 256 &&
      !(multiplayer_weapon_steps_ && (w.smg || w.firing_presentation) && weapon_effect_hook_))
    sounds_.push_back({w.sound, origin});
  const int count = std::clamp(w.projectiles_per_shot, 1, 64);
  for (int i = 0; i < count; ++i) {
    auto dir = prepared_direction ? *prepared_direction : direction(unit.yaw, pitch);
    if (w.distribution == 1 && count > 1)
      dir = direction(
          unit.yaw + w.distribution_angle * (2.f * i / (count - 1) - 1), pitch);
    if(!prepared_direction && multiplayer_weapon_steps_ && w.barrel_timing) {
      if(!content_.original_data)throw FormatError("Original direction data is required for barrel dispatch");
      dir=weapon_random_deviation(dir,weapon_error_angles(w.firing_error,selected_barrel.error,weapon_hand_blend(unit,state)),
          random_state_,content_.original_data->weapon_directions);
    } else if (!prepared_direction && w.minimum_error > 0) {
      auto random = [&] {
        random_state_ = random_state_ * 1664525u + 1013904223u;
        return float(random_state_ >> 8) / 16777216.f;
      };
      const float angle = random() * 6.283185307f,
                  error = std::sqrt(random()) * w.minimum_error;
      const auto right = normalized(cross(dir, {0, 0, 1})),
                 up = cross(right, dir);
      dir = normalized(dir + right * (std::cos(angle) * std::tan(error)) +
                       up * (std::sin(angle) * std::tan(error)));
    }
    if (w.instantaneous)
      trace(origin, dir, w.range > 0 ? w.range : 100, w, owner, captured);
    else if (w.projectile_speed > 0 && projectiles_.size() < 1024) {
      const auto datum = allocate_object_ ? allocate_object_(5) : null_datum;
      if (!allocate_object_ || datum != null_datum) {
        std::optional<SourceNeedlerProjectileBirth> birth;
        if(multiplayer_weapon_steps_ && w.needler_projectile) {
          if(prepared_velocity)
            throw FormatError("Needler speed override requires its original creation descriptor owner");
          birth=source_needler_projectile_birth(*w.needler_projectile,dir,{},30,random_state_);
        }
        projectiles_.push_back(
            {origin, birth ? birth->velocity : prepared_velocity ? *prepared_velocity : dir * (source_projectile_speed_scale(w.projectile_flags,captured.player,!multiplayer_weapon_steps_,std::int16_t(difficulty_),content_.projectile_difficulty_speed)*w.projectile_speed),
             0, w.datum, owner, int(bsp_), datum, captured,dir,source_weapon_projectile_up(dir),birth,barrel_index});
        // Retain the weapon adapter's actual projectile definition (or explicit
        // NONE); this does not claim complete original projectile creation.
        if(publish_object_ && datum!=null_datum)
          publish_object_(datum,w.projectile);
      }
    }
  }
}
void Simulation::update_weapon(Unit &u, bool fire, bool reload, int owner,
                               float pitch,const Controls *source_control,unsigned hand) {
  // The original unit/child-weapon owners consume delivered controls during
  // step_objects. A parallel native update would spend ammunition, prepare
  // projectiles and advance clocks a second time before that source phase.
  if(source_physics_movement_ && multiplayer_weapon_steps_)return;
  const bool infinite=debug_settings_for(u).unlimited_ammo;
  if(infinite)debug_refill_weapon(u);
  struct Refill {
    Simulation& simulation;Unit& unit;bool enabled;
    ~Refill(){if(enabled)simulation.debug_refill_weapon(unit);}
  } refill{*this,u,infinite};
  auto *selected=hand?native_hand_weapon(u,hand):&u.weapon;
  if(!selected)return;
  auto &state = *selected;
  const auto it = content_.weapons.find(state.definition);
  if (it == content_.weapons.end())
    return;
  const auto &w = it->second;
  if(w.source_runtime_only)
    throw FormatError("Authored item gameplay requires the retained source runtime");
  if(state.ready_clock)source_weapon_ready_tick(state.shader,state.ready_countdown);
  else if(multiplayer_weapon_steps_ && (w.smg || w.barrel_timing))
    state.shader.weapon_flags|=0x20u; // Recovered constructor's zero19E arms on first update.
  if(multiplayer_weapon_steps_ && hand==0) {
    const auto busy=[&](const WeaponState *weapon) {
      if(!weapon)return false;
      const auto &definition=content_.weapons.at(weapon->definition);
      return definition.smg?(weapon->smg.magazine_mode>=1 && weapon->smg.magazine_mode<=6):weapon->reload>0;
    };
    if(!busy(&u.weapon) && !busy(native_hand_weapon(u,1)))u.source_action_word&=~1u;
    if(!source_physics_movement_) {
      const bool switching=source_control && source_control->switch_grenade;
      const auto control_grenade=grenade_ready_type(u.grenade_current,u.grenades,
          switching?1:0);
      if(control_grenade!=-1)u.grenade_requested=control_grenade;
      if(u.grenade_requested!=u.grenade_current) {
        const auto selected=grenade_ready_type(u.grenade_requested,u.grenades);
        if(selected!=-1)u.grenade_current=selected;
      }
    }
    if(grenade_sandbox_ && !source_physics_movement_)grenade_sandbox_->update_unit(u,source_control&&source_control->grenade,
        owner,allocate_object_,[&](const Unit &unit,std::uint32_t action) {
          if(first_person_activity_hook_)first_person_activity_hook_(unit,state,action);
        },publish_object_);
  }
  if(multiplayer_weapon_steps_ && hand==0 && melee_animation_ && !source_physics_movement_) {
    const bool sword=(w.inventory_flags&(1u<<24))!=0;
    if(!fire)state.trigger_armed=true;
    const bool special=sword&&fire&&state.trigger_armed;
    if(special)state.trigger_armed=false;
    melee_animation_->update(u,(source_control&&source_control->melee)||special,std::uint32_t(ticks_+1),random_state_,
        [&](std::uint32_t action){weapon_activity(u,state,action);},
        [&](const NativeMeleeAction &action) {
          const auto hit=query_native_melee(eye(u),direction(u.yaw,pitch),melee_animation_->pyramid(u),
              [&](Vec3 origin,Vec3 delta,std::uint32_t mask)->std::optional<NativeMeleeContact> {
            if(mask!=0x2480000fu)throw FormatError("Melee collision mask changed");
            const float distance=length(delta);if(!(distance>0))return {};
            const auto dir=delta/distance;float nearest=distance;
            std::optional<NativeMeleeContact> contact;
            if(const auto world=static_collision_->raycast(origin,dir,distance)) {
              nearest=world->distance;contact=NativeMeleeContact{null_datum,0,nearest/distance,world->position,world->normal,std::int16_t(world->material)};
            }
            if(const auto prop=props_.raycast(origin,delta,nearest/distance,u.source_datum)) {
              nearest=prop->collision.fraction*distance;
              contact=NativeMeleeContact{prop->handle,7,prop->collision.fraction,origin+dir*nearest,prop->collision.plane.normal,prop->collision.global_material};
            }
            const auto test=[&](const Unit &other) {
              if(!other.exists || other.bsp_hidden || &other==&u)return;
              const float d=ray_body(origin,dir,other.body,nearest);
              if(d<nearest) {
                nearest=d;const auto point=origin+dir*d;
                contact=NativeMeleeContact{other.source_datum,0,d/distance,point,normalized(point-center(other)),-1};
              }
            };
            test(player_);for(const auto &other:actors_)test(other);
            return contact;
          });
          const auto definition=melee_animation_->damage_definition(u,action.activity);
          // D0830's D0340 presentation follows contact selection even without
          // an object target or when modifiers suppress direct damage. The
          // current native unit capsules supply no authored mesh material.
          if(hit.material>=0 && std::size_t(hit.material)<content_.material_parents.size()){
            melee_impacts_.push_back({++bullet_impact_serial_,ticks_+1,u.source_datum,
                u.weapon.definition,definition?definition->damage.damage:null_datum,
                hit.point,hit.normal,direction(u.yaw),u.body.position,hit.material});
            if(melee_impacts_.size()>64)melee_impacts_.erase(melee_impacts_.begin());
          }
          if(!hit.target || action.modifiers || !definition)return;
          const auto &damage_definition=definition->damage;
          const auto &amount=*damage_definition.damage_amount;
          if(amount.upper_minimum!=amount.upper_maximum)throw FormatError("Melee random upper damage owner is unavailable");
          const float fraction=std::min(1.f,float(action.speed_modifier)*std::bit_cast<float>(content_.original_data->melee_constants->impact_scale_bits));
          const float value=damage_amount(amount.lower,amount.upper_minimum,fraction,1,1);
          const DamageOwner captured{u.source_player,u.source_datum,u.source_membership};
          Unit *target=nullptr;
          if(player_.source_datum==hit.target->object)target=&player_;
          for(auto &other:actors_)if(other.source_datum==hit.target->object)target=&other;
          if(target){target->last_damager=owner;damage(*target,damage_definition,captured,owner,value,false,
              DamageImpactContext{DamageImpactContext::Kind::Melee,{},{},fraction});}
          else props_.damage(hit.target->object,damage_definition,value*player_damage_scale(captured,null_datum),
              {captured.player,captured.unit,captured.designator,owner},false,{.random=&random_state_});
        },special?2:1);
    if(sword)return;
  }
  const auto active_actions=source_physics_movement_?u.grenade_action.active_actions():u.source_action_word;
  if(source_control && content_.binocular_zoom && hand==0) {
    if(source_control->zoom_reset_serial==u.zoom_reset_serial)
      source_zoom_request(u.source_zoom,source_control->zoom);
    const std::array<std::int16_t,1> modes{w.smg?state.smg.magazine_mode:state.magazine.mode};
    const ZoomWeaponView primary{w.zoom,w.magazine?std::span(modes):std::span<const std::int16_t>{}};
    const UnitZoomContext context{u.source_parent,active_actions,{&primary,nullptr}};
    source_zoom_update(u.source_zoom,context,
        !(u.definition.unit_flags&0x400u) && !(u.source_unit_10a&4u));
  }
  const bool weapon_blocked=(active_actions & (0x400000u|(1u<<27)))!=0;
  if(weapon_blocked)fire=reload=false;
  if(multiplayer_weapon_steps_ && w.charged_cycle) {
    update_charged_weapon(u,state,w,fire,weapon_blocked,hand,owner);return;
  }
  if (multiplayer_weapon_steps_ && w.smg) {
    // Original player control ->151C90/C8140 ->C6E19/CD310. Recovered SMG
    // idle/fire_1/reload_1 with no blocking overlay admit this transition;
    // unsupported melee/vehicle/switch/other action names remain separate.
    auto &native = state.smg;
    smg_input(native, fire);
    for(unsigned timer=0;timer<2;++timer)native.hand_timers[timer]=u.source_weapon_hand_feedback[timer];
    native.input_flags=source_zoom_weapon_input(native.input_flags,u.source_zoom);
    const auto reset_zoom=[&] {
      source_zoom_reset(u.source_zoom);
      ++u.zoom_reset_serial;
    };
    const SmgActivityCallback activity=[&](std::uint32_t action) {
      // Whole107160 disrupts a player-owned unit after accepted firing and
      // before its prepared-shot FP/zoom publication and projectile postpass.
      if(action==0x06000006 && u.source_player!=null_datum)
        disrupt_camouflage(u,w.firing_camouflage_amount,w.firing_camouflage_limit);
      //107160 -> C9A40(1): a prepared shot owned by a unit retains genuine
      // weapon optics only when105120 admits them. SMG binocular zoom resets.
      if(action==0x06000006 && u.source_datum!=null_datum &&
          (w.zoom.levels==0 || (native.magazine_mode>=1 && native.magazine_mode<=6)))
        reset_zoom();
      weapon_activity(u,state,action);
    };
    WeaponAmmunitionContext ammunition;ammunition.has_unit=u.source_datum!=null_datum;ammunition.infinite=infinite;
    std::array<WeaponState *,5> ammo_owners{};ammo_owners[0]=&state;
    const auto ammo_view=[&](WeaponState &item) {
      const auto &authored=content_.weapons.at(item.definition);
      item.magazine.loaded=authored.smg?item.smg.loaded:std::bit_cast<std::int16_t>(std::uint16_t(item.loaded));
      item.magazine.reserve=authored.smg?item.smg.reserve:std::bit_cast<std::int16_t>(std::uint16_t(item.reserve));
      return WeaponAmmunitionView{item.source_datum,item.definition,{&item.magazine,1},
          authored.magazine?std::span(&*authored.magazine,1):std::span<const WeaponMagazineDefinition>{}};
    };
    ammunition.current=ammo_view(state);
    for(unsigned slot=0;slot<4;++slot)if(auto *other=native_inventory_weapon(u,int(slot))) {
      ammo_owners[slot+1]=other;ammunition.inventory[slot]=ammo_view(*other);
    }
    SmgRuntimeServices runtime;runtime.opposite_hand_blend=weapon_hand_blend(u,state);runtime.shared_hand_feedback=true;
    runtime.magazine_effect=[&](bool chamber) {
      if(!w.magazine)throw FormatError("SMG magazine effect metadata is unavailable");
      if(weapon_effect_hook_)weapon_effect_hook_(u,{chamber?w.magazine->chamber_effect:w.magazine->reload_effect,
          state.source_datum,null_datum,0,0});
    };
    runtime.available=[&]{return weapon_reserve_available(ammunition,0);};
    runtime.spend=[&](std::int32_t amount) {
      (void)weapon_reserve_spend(ammunition,0,amount,[]{});
      for(auto *item:ammo_owners)if(item) {
        item->reserve=item->magazine.reserve;
        if(content_.weapons.at(item->definition).smg)item->smg.reserve=item->magazine.reserve;
      }
    };
    // The bounded single-SMG definition contains its measured50-tick
    // reloads. Dual mode resolves the owning representation's graph; the
    // original110-tick clips cannot be replaced by those single timings.
    if(u.source_inventory.current[0]!=-1 && u.source_inventory.current[1]!=-1)
      runtime.timing=[&](std::uint32_t activity,unsigned section) {
        if(!first_person_timing_hook_)throw FormatError("Dual SMG needs its original represented graph timing");
        return first_person_timing_hook_(u,state,activity,std::int16_t(section));
      };
    if(w.barrel_timing)runtime.fire_gate=[&](SmgBarrelState &barrel) {
      WeaponBarrelTimingContext context;context.tick_rate=30;context.spent=state.shader.power_fraction;
      if(const auto *other=native_hand_weapon(u,hand?0:1);other && other->definition==state.definition)
        context.other_hand=WeaponBarrelTimingContext::OtherHand{other->smg.barrel.mode,other->smg.barrel.counter};
      return weapon_barrel_gate(barrel,w.barrel_timing->definition,context,w.barrel_timing->constants).admitted;
    };
    const bool explicit_reload=reload && smg_reload(native,*w.smg,activity,runtime);
    if(explicit_reload) {
      // Successful original E88B0 sets action bit0 and C7E90 calls C9A40(0).
      u.source_action_word|=1u;
      reset_zoom();
    }
    // Ordinary BD360 updates the biped (C7E90/DE960/E7E60) before its
    // attached weapon child. E8950 therefore sees the pre-weapon mode; a
    // magazine completing this tick releases bit0 at the next biped phase.
    
    const SmgEffectCallback effect=[&](const SmgState &prepared,unsigned selection,
                                    std::uint32_t a,std::uint32_t b) {
      if(w.firing_effects.size()!=1 || selection>=3)
        throw FormatError("Stock SMG effect selection escaped its original firing entry");
      const auto &entry=w.firing_effects[0];
      if(entry.damage_effects[selection]!=null_datum && rumble_events_.size()<256)
        rumble_events_.push_back({entry.damage_effects[selection],u.vitality.shield>0,1.f,owner});
      return weapon_effect_hook_?weapon_effect_hook_(u,{entry.effects[selection],
          state.source_datum,prepared.barrel.effect_handle,a,b}):prepared.barrel.effect_handle;
    };
    const bool automatic_reload=smg_update(native,*w.smg,tick_seconds,activity,effect,runtime);
    if (explicit_reload || automatic_reload) state.reload_started = ticks_ + 1;
    state.reload_empty = native.reload_empty;
    state.loaded = native.loaded;
    state.reserve = native.reserve;
    state.magazine.mode=native.magazine_mode;state.magazine.ticks=native.magazine_ticks;
    state.magazine.loaded=native.loaded;state.magazine.reserve=native.reserve;
    state.magazine.transfer_ticks=native.transfer_ticks;state.magazine.other_ticks=native.magazine_other_ticks;
    state.reload = native.magazine_mode == 1
        ? float(native.magazine_ticks) * tick_seconds : 0;
    return;
  }
  //1021D0 updates the barrel rate after107160 consumes its old value.
  //Keep that order even across this native adapter's early returns.
  const bool source_timing=multiplayer_weapon_steps_ && w.barrel_timing.has_value();
  const bool source_trigger=source_timing && w.triggers.size()==1 && w.triggers[0].input==0 &&
      (w.trigger_behavior==0 || w.trigger_behavior==1 || w.trigger_behavior==4);
  // The source reload consumer is composed for ordinary, single-magazine
  // definitions. Additional battery/parent/world admission owners remain separate.
  const bool source_magazine=source_timing && w.magazine &&
      !(w.inventory_flags&((1u<<11)|(1u<<19)|(1u<<25)));
  WeaponAmmunitionContext magazine_ammunition;
  WeaponMagazineServices magazine_services;
  std::array<WeaponState *,5> magazine_owners{};
  std::function<bool()> begin_reload;
  std::function<void(unsigned)> automatic_magazine_reload;
  if(source_magazine) {
    auto &magazine=state.magazine;
    const auto &definition=*w.magazine;
    auto &ammunition=magazine_ammunition;ammunition.has_unit=u.source_datum!=null_datum;ammunition.infinite=infinite;
    auto &owners=magazine_owners;owners[0]=&state;
    const auto view=[&](WeaponState &item) {
      item.magazine.loaded=std::bit_cast<std::int16_t>(std::uint16_t(item.loaded));
      item.magazine.reserve=std::bit_cast<std::int16_t>(std::uint16_t(item.reserve));
      const auto &authored=content_.weapons.at(item.definition);
      return WeaponAmmunitionView{item.source_datum,item.definition,{&item.magazine,1},
          authored.magazine?std::span(&*authored.magazine,1):std::span<const WeaponMagazineDefinition>{}};
    };
    ammunition.current=view(state);
    for(unsigned slot=0;slot<4;++slot)if(auto *other=native_inventory_weapon(u,int(slot))) {
      owners[slot+1]=other;ammunition.inventory[slot]=view(*other);
    }
    const auto symbol=[](unsigned action) -> std::uint32_t {
      // Complete1680B3/169493 table entries reached by the magazine owners.
      switch(action) {
        case 0:return 0x0400000cu;case 7:return 0x0c000064u;case 8:return 0x0b000065u;
        case 9:return 0x0c0005aeu;case 10:return 0x150005afu;case 11:return 0x140005b0u;
        case 12:return 0x0b0005b1u;
        default:throw FormatError("Magazine action exceeds its original dispatch contract");
      }
    };
    auto &services=magazine_services;
    services.transfer.available=[&]{return weapon_reserve_available(ammunition,0);};
    services.transfer.spend=[&](std::int32_t amount){(void)weapon_reserve_spend(ammunition,0,amount,[]{});};
    // Native snapshots publish these retained fields each tick. The source
    // dirty notifier adds no independent ammunition or animation state here.
    services.transfer.changed=[]{};
    services.transition.activity_symbol=symbol;
    services.transition.timing=[&](std::uint32_t activity,unsigned section) {
      return first_person_timing_hook_?std::int32_t(first_person_timing_hook_(u,state,activity,std::int16_t(section))):0;
    };
    services.transition.activity=[&,symbol](unsigned action) {
      if(action>=7 && action<=11) {
        state.reload_started=ticks_+1;state.reload_empty=magazine.loaded==0;
      }
      if(first_person_activity_hook_)first_person_activity_hook_(u,state,symbol(action));
    };
    services.idle=[&]{return !state.smg.trigger_state && !state.smg.second_trigger_state && !state.smg.weapon_state;};
    services.request_state=[&](unsigned requested) {
      if(state.smg.own_manager && (!state.smg.weapon_state ||
          (state.smg.weapon_state<3 && state.smg.weapon_state<=requested)))state.smg.weapon_state=requested;
    };
    services.effect=[&](std::uint32_t effect) {
      if(weapon_effect_hook_)weapon_effect_hook_(u,{effect,state.source_datum,null_datum,0,0});
    };
    services.fire_admitted=[&](unsigned index) {
      if(index>=w.barrel_ammunition.size())return false;
      const auto &authored=w.barrel_ammunition[index];
      const auto &barrel=index==unsigned(w.barrel_timing->index)?state.smg.barrel:state.alternate_barrel;
      if(barrel.mode==2 || (barrel.flags&4u) || (state.shader.weapon_flags&2u))return false;
      if(authored.magazine!=-1) {
        if(authored.magazine!=0)throw FormatError("Reload firing admission needs another actual magazine owner");
        if(magazine.loaded<authored.rounds && !(authored.flags&4u) &&
            (w.weapon_type==1 || (barrel.flags&0x40u)))return false;
        if(magazine.loaded<authored.minimum && (barrel.flags&0x40u))return false;
      }
      return true;
    };
    begin_reload=[&] {
      if(weapon_blocked || !weapon_magazine_reload(magazine,definition,w.weapon_type,0,30,services))return false;
      u.source_action_word|=1u;source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
      state.smg.trigger_state=state.smg.second_trigger_state=0;state.smg.trigger_ticks=0;
      for(unsigned index=0;index<w.barrel_ammunition.size();++index) {
        auto &barrel=index==unsigned(w.barrel_timing->index)?state.smg.barrel:state.alternate_barrel;
        if(barrel.mode!=1)barrel.flags&=~0x40u;
        if(w.weapon_type!=1)weapon_barrel_reset(barrel,{w.barrel_ammunition[index].flags});
      }
      return true;
    };
    services.request_reload=begin_reload;
    state.smg.input_flags=fire?std::uint16_t(2u|(!state.smg.held?8u:0u)):0;
    if(reload)begin_reload(); //105640 before the same tick's1021D0 update.
    weapon_magazine_step(magazine,definition,state.shader.item_flags,state.smg.input_flags,0,30,services);
    //1021D0's idle-barrel106580 asks the unit to reload even without a held
    // trigger. Preparation/dry-gate admission is resolved below first.
    const bool requested=fire && (w.trigger_behavior==0 ||
        (source_trigger?(state.smg.trigger_flags&1u)!=0:state.trigger_armed));
    const bool held=requested && services.fire_admitted(unsigned(w.barrel_timing->index));
    if(held && w.weapon_type==1 && magazine.mode!=0) {
      //1066F0/107610 with argument3=false interrupts a shotgun magazine
      //before preparing the shot, after this tick's transfer/update phase.
      services.transition.activity(0);
      weapon_magazine_transition(magazine,definition,0,30,services.transition);
    }
    automatic_magazine_reload=[&,held](unsigned index) {
      auto &barrel=index==unsigned(w.barrel_timing->index)?state.smg.barrel:state.alternate_barrel;
      const auto &authored=w.barrel_ammunition[index];
      const bool trigger=index==unsigned(w.barrel_timing->index) && held &&
          (magazine.mode==0 || w.weapon_type==1);
      if(barrel.mode!=0 || (barrel.flags&1u) || trigger || authored.magazine==-1)return;
      if(authored.magazine!=0)throw FormatError("Automatic reload requires another actual magazine owner");
      const bool insufficient=(magazine.loaded<authored.rounds && !(authored.flags&4u)) ||
          magazine.loaded<authored.minimum || magazine.loaded==0;
      if(!insufficient)return;
      if(services.transfer.available()>0 && magazine.loaded<definition.capacity)begin_reload();
      else {
        //106580/1074A0's no-reserve suffix stops every active barrel before
        //the next barrel's update, including the carbine's unused barrel0.
        state.smg.trigger_state=state.smg.second_trigger_state=0;state.smg.trigger_ticks=0;
        for(unsigned other=0;other<w.barrel_ammunition.size();++other) {
          auto &active=other==unsigned(w.barrel_timing->index)?state.smg.barrel:state.alternate_barrel;
          if(active.mode==1) {
            if(other!=unsigned(w.barrel_timing->index))
              throw FormatError("Alternate active barrel recovery requires its authored timing owner");
            weapon_barrel_begin_recovery(active,w.barrel_timing->definition,30,w.barrel_timing->constants);
          }
        }
      }
    };
    if(!weapon_blocked)for(unsigned index=0;index<=unsigned(w.barrel_timing->index);++index)
      automatic_magazine_reload(index);
    for(auto *item:owners)if(item){item->loaded=item->magazine.loaded;item->reserve=item->magazine.reserve;}
    state.reload=(magazine.mode>=1 && magazine.mode<=3) || magazine.mode==6?
        float(std::max<int>(0,magazine.ticks))*tick_seconds:0.f;
  }
  const auto advance_primary_barrel=[&] {
  bool energy_blocked=false;
  if(source_timing && w.energy) {
    const auto &energy=*w.energy;
    state.shader.weapon_flags&=~1u;
    WeaponEnergyState source{state.definition,state.shader.item_flags,state.shader.weapon_flags,
        state.shader.heat,state.shader.power_fraction,state.cooling_inhibitor,state.overheat_handle,state.energy_blocking_ticks};
    WeaponHeatServices services;services.has_unit=u.source_datum!=null_datum;
    services.reset_zoom=[&]{source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;};
    services.action=[&](unsigned action) {
      if(first_person_activity_hook_)first_person_activity_hook_(u,state,action==14?0x0b00006du:0x08000071u);
    };
    services.vent_timing_ticks=[&] {
      return first_person_timing_hook_?first_person_timing_hook_(u,state,0x08000071u,1):std::int16_t(0);
    };
    // A persistent overheat effect needs its own generic lifecycle owner.
    // No successful handle is manufactured by the numeric/FP adapter.
    weapon_heat_step(source,energy.heat,tick_seconds,energy.one,services);
    state.shader.heat=source.heat;state.cooling_inhibitor=source.cooling_inhibitor;
    state.shader.weapon_flags=source.weapon_flags;state.overheat_handle=source.overheat_handle;
    state.energy_blocking_ticks=source.blocking_ticks;
    energy_blocked=(source.weapon_flags&2u) || source.blocking_ticks>0;
    if(energy_blocked)fire=false;
  }
  const bool presentation_marker=!source_timing && fire && !reload && state.reload<=0 && state.loaded>0;
  struct FiringRate {
    WeaponState &state;const WeaponDefinition &definition;bool marker;float feedback_blend;
    ~FiringRate() {
      if(!definition.firing_presentation && !definition.barrel_timing)return;
      auto &rate=state.smg.barrel.rate;
      const auto rise=definition.barrel_timing?definition.barrel_timing->rate_rise:definition.firing_presentation->rate_rise;
      const auto fall=definition.barrel_timing?definition.barrel_timing->rate_fall:definition.firing_presentation->rate_fall;
      if(marker){rate=rise*tick_seconds+rate;if(rate>1.f)rate=1.f;}
      else if(rate>0.f){rate=rate-fall*tick_seconds;if(rate<0.f)rate=0.f;}
      if(definition.barrel_timing) {
        auto &barrel=state.smg.barrel;
        WeaponErrorState error{barrel.error,barrel.angle_change};
        weapon_error_step(error,definition.firing_error,tick_seconds,feedback_blend,marker);
        barrel.error=error.error;barrel.angle_change=error.recoil;
      }
    }
  } firing_rate{state,w,presentation_marker,weapon_hand_blend(u,state)};
  state.delay = std::max(0.f, state.delay - tick_seconds);
  const bool latched=w.trigger_behavior==1 || w.trigger_behavior==4 || w.trigger_behavior==5;
  // Original1021D0 trigger behavior1 arms on release and consumes that bit
  // only when its barrel request is accepted. Keep it with the item, so a
  // dropped/re-equipped weapon does not gain a second held-trigger request.
  if(latched && !fire)state.trigger_armed=true;
  if (!source_magazine && state.reload > 0) {
    state.reload -= tick_seconds;
    if (state.reload <= 0) {
      const int transfer =
          std::min({state.reserve, w.capacity - state.loaded,
                    w.reload_rounds > 0 ? w.reload_rounds : w.capacity});
      state.loaded += transfer;
      state.reserve -= transfer;
    }
    return;
  }
  // Original103365 asks for automatic reload only in idle barrel mode.
  // An empty last shot must complete its recovery instead of resetting it.
  const bool automatic_reload=fire && state.loaded==0 && (!source_timing || state.smg.barrel.mode==0);
  if (!source_magazine && (reload || automatic_reload) && state.loaded < w.capacity && state.reserve > 0) {
    state.reload_empty = state.loaded == 0;
    state.reload_started = ticks_ + 1;
    u.source_action_word|=1u;source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
    if (w.wastes_rounds)
      state.loaded = 0;
    state.reload = std::max(tick_seconds, w.reload_time);
    state.burst_left = 0;
    if(source_timing) {
      weapon_barrel_reset(state.smg.barrel,w.barrel_timing->definition);
      state.smg.barrel.flags&=~1u;
    }
    if(first_person_activity_hook_)first_person_activity_hook_(u,state,state.reload_empty?0x0c000064:0x0b000065);
    return;
  }
  // This ordinary input adapter supplies no alternate trigger selection.
  // Original1037FD and the latched firing helper check the selected NONE
  // before admission; a retained secondary barrel does not replace it.
  // Preserve ready/recovery/reload work above for these inactive triggers.
  if(!w.triggers.empty() && std::all_of(w.triggers.begin(),w.triggers.end(),
      [](const auto& trigger){
        if(trigger.behavior==0 || trigger.behavior==1 || trigger.behavior==4)
          return trigger.primary_barrel==-1;
        return trigger.primary_barrel==-1 && trigger.secondary_barrel==-1;
      }))
    return;
  bool dry_shot=false;
  if(source_timing) {
    auto &barrel=state.smg.barrel;
    const bool rising=fire && !state.smg.held;state.smg.held=fire;
    const bool empty_battery=w.energy && (w.inventory_flags&(1u<<11)) &&
        state.shader.power_fraction>=w.energy->one && !infinite;
    const bool eligible=barrel.mode!=2 && !(barrel.flags&4u) &&
        !(state.shader.weapon_flags&2u) &&
        (!source_magazine || state.magazine.mode==0 || w.weapon_type==1) &&
        !(source_magazine && w.weapon_type==1 && state.loaded<w.rounds_per_shot) &&
        !((state.loaded<w.rounds_per_shot || empty_battery) && (barrel.flags&0x40u));
    if(source_trigger) {
      //Actual102833/1031F0 phase. The existing single-barrel bridge supplies
      //its recovered1066F0 admission; charge/lock and distinct active barrels
      //retain their separate owners before they can enter this adapter.
      if(state.smg.trigger_state)throw FormatError("Ordinary native trigger unexpectedly requires charge/lock owners");
      const auto selected=unsigned(w.barrel_timing->index);
      if(selected>=2)throw FormatError("Original native trigger has no fixed barrel storage");
      std::array<SmgBarrelState,2> active;
      active[selected]=barrel;active[selected?0:1]=state.alternate_barrel;
      WeaponTriggerState trigger{state.smg.trigger_state,0,state.smg.trigger_ticks,
          state.smg.trigger_flags,state.smg.trigger_effect_handle};
      WeaponTriggerInputState analog;WeaponTriggerTarget target;
      WeaponTriggerPhaseContext phase;phase.tick_rate=30;phase.held=fire;phase.pressed=rising;
      phase.blocked=weapon_blocked || energy_blocked;phase.barrel_count=unsigned(w.barrel_ammunition.size());
      phase.spent=state.shader.power_fraction;phase.has_unit=(state.shader.item_flags&1u) && u.source_datum!=null_datum;
      phase.has_player=u.source_player!=null_datum;
      WeaponTriggerPhaseServices services;
      services.barrel_admitted=[&](std::int16_t requested) {
        if(requested!=int(selected))throw FormatError("Native trigger requires another active barrel admission owner");
        return eligible;
      };
      (void)weapon_trigger_step(trigger,w.triggers[0],active,analog,target,0,phase,services);
      barrel=active[selected];state.alternate_barrel=active[selected?0:1];
      state.smg.trigger_state=trigger.mode;state.smg.trigger_ticks=trigger.ticks;
      state.smg.trigger_flags=trigger.flags;state.smg.trigger_effect_handle=trigger.effect;
      if(w.trigger_behavior!=0)state.trigger_armed=(trigger.flags&1u)!=0;
    } else if(w.trigger_behavior==0) {
      if(fire && eligible)barrel.flags|=1u;
      if((!fire || rising) && barrel.mode!=1)barrel.flags&=~0x40u;
    } else {
      if(!fire) {
        state.trigger_armed=true;
        if(barrel.mode!=1)barrel.flags&=~0x40u;
      } else if(state.trigger_armed && eligible) {
        barrel.flags|=1u;state.trigger_armed=false;
      }
    }
    WeaponBarrelTimingContext context;context.tick_rate=30;context.spent=state.shader.power_fraction;
    if(const auto *other=native_hand_weapon(u,hand?0:1);other && other->definition==state.definition)
      context.other_hand=WeaponBarrelTimingContext::OtherHand{other->smg.barrel.mode,other->smg.barrel.counter};
    const auto &timing=*w.barrel_timing;
    const auto next=weapon_barrel_timing_step(barrel,timing.definition,context,timing.constants,weapon_blocked || energy_blocked);
    firing_rate.marker=next.marker;
    state.delay=float(barrel.ticks)*tick_seconds;
    if(!next.prepare)return;
    if((w.capacity>0 && state.loaded<std::max(w.rounds_per_shot,w.minimum_rounds)) ||
        empty_battery) {
      barrel.flags|=0xc0u;dry_shot=true;
    } else ++barrel.shot_count;
    barrel.counter=0;
  } else {
  if (state.delay > 1e-6f || (w.rounds_per_second<=0 && !latched))
    return;
  if (!state.burst_left && fire && (!latched || state.trigger_armed)) {
    state.burst_left = std::clamp(w.burst, 1, 16);
    if(latched)state.trigger_armed=false;
  }
  if (!state.burst_left)
    return;
  if (w.capacity > 0 &&
      state.loaded < std::max(w.rounds_per_shot, w.minimum_rounds)) {
    state.burst_left = 0;
    return;
  }
  }
  if(!dry_shot && (!source_timing || (state.shader.item_flags&8u)))
    state.loaded = std::max(0, state.loaded - w.rounds_per_shot);
  if(source_magazine) {
    state.magazine.loaded=std::bit_cast<std::int16_t>(std::uint16_t(state.loaded));
    //107160/108080: magazine flag1 enters chamber-pending mode after a
    //successful preparation with rounds left, before its effect request.
    if(!dry_shot && state.loaded>0 && (w.magazine->flags&2u))
      weapon_magazine_transition(state.magazine,*w.magazine,5,30,{});
  }
  std::optional<unsigned> prepared_effect;
  if(multiplayer_weapon_steps_ && w.firing_presentation)prepared_effect=select_weapon_firing_effect(state,w);
  const bool misfire=source_timing && w.energy && weapon_battery_misfire(state.shader.power_fraction,
      w.energy->misfire_threshold,w.energy->misfire_chance,w.energy->one,w.energy->random_scale,random_state_);
  if(!dry_shot && multiplayer_weapon_steps_ && !(w.zoom.flags&(1u<<24)) && u.source_player!=null_datum)
    disrupt_camouflage(u,w.firing_camouflage_amount,w.firing_camouflage_limit);
  if(!dry_shot)weapon_activity(u,state,
      misfire?(w.barrel_timing && w.barrel_timing->index?0x0900006bu:0x0900006au):
      (w.zoom.flags&(1u<<24))?0x0e000607u:(w.barrel_timing && w.barrel_timing->index?0x06000007u:0x06000006u));
  std::optional<WeaponEffectInput> prepared_request;
  if(prepared_effect) {
    //107160 emits the prepared-shot effect before the projectile postpass.
    const unsigned selection=dry_shot?2u:misfire?1u:0u;
    const float heat=w.energy && !dry_shot && !misfire?state.shader.heat/w.energy->heat.maximum:0.f;
    const float a=dry_shot?1.f:state.smg.barrel.rate;
    prepared_request=WeaponEffectInput{w.firing_effects[*prepared_effect].effects[selection],
        state.source_datum,state.smg.barrel.effect_handle,std::bit_cast<std::uint32_t>(a),
        std::bit_cast<std::uint32_t>(heat)};
    if(w.firing_effects[*prepared_effect].damage_effects[selection]!=null_datum && rumble_events_.size()<256)
      rumble_events_.push_back({w.firing_effects[*prepared_effect].damage_effects[selection],u.vitality.shield>0,1.f,owner});
  }
  if(source_timing && w.energy && !dry_shot) {
    const auto &energy=*w.energy;
    state.shader.heat=energy.heat_per_shot+state.shader.heat;
    if(!(state.shader.item_flags&8u) && state.shader.heat>energy.heat.maximum)state.shader.heat=energy.heat.maximum;
    else if(state.shader.heat>energy.one)state.shader.heat=energy.one;
    if(!(w.inventory_flags&(1u<<26))) {
      WeaponEnergyState source{state.definition,state.shader.item_flags,state.shader.weapon_flags,
          state.shader.heat,state.shader.power_fraction};
      weapon_age(source,energy.spent_per_shot,energy.one,infinite,energy.age,[&](std::uint32_t effect) {
        if(weapon_effect_hook_)weapon_effect_hook_(u,{effect,state.source_datum,null_datum,0,0});
      });
      state.shader.power_fraction=source.spent;
      // Aged-definition replacement is retained with the same item identity.
      state.definition=source.definition;
    }
  }
  if(source_timing && w.energy && weapon_heat_detonation(state.shader.heat,w.energy->detonation_threshold,
        w.energy->detonation_chance,w.energy->random_scale,random_state_))
    throw FormatError("Weapon self-detonation requires its source object owner");
  if(prepared_request && weapon_effect_hook_)
    state.smg.barrel.effect_handle=weapon_effect_hook_(u,*prepared_request);
  if(source_timing) {
    if(!dry_shot)state.last_shot=ticks_+1;
    if(!dry_shot && !misfire) {state.smg.barrel.flags|=2u;state.shader.weapon_flags|=1u;}
    return;
  }
  shot(u, owner, pitch, nullptr, nullptr, &state,prepared_effect.has_value());
  --state.burst_left;
  // Original1062B0 uses a zero interval below its rate threshold. Lockout's
  // shotgun has literal zero/zero rate bounds and a one-shot burst; zero is
  // not a disabled barrel. Zero-rate trigger styles outside this recovered
  // latch keep their prior unsupported behavior. Native recovery is separate
  // from the complete original discrete barrel/magazine update.
  const float interval=w.rounds_per_second>0?1.f/w.rounds_per_second:0.f;
  state.delay = state.burst_left?interval:std::max(interval,w.fire_recovery);
  };
  advance_primary_barrel();
  //1021D0 updates barrels in authored order. A later unused barrel can
  //request reload in the same tick that the selected barrel consumes its last
  //round (rocket launcher); the carbine's earlier barrel acts before it.
  if(source_magazine) {
    if(!weapon_blocked)for(unsigned index=unsigned(w.barrel_timing->index)+1;index<w.barrel_ammunition.size();++index)
      automatic_magazine_reload(index);
    for(auto *item:magazine_owners)if(item){item->loaded=item->magazine.loaded;item->reserve=item->magazine.reserve;}
    const auto &magazine=state.magazine;
    state.reload=(magazine.mode>=1 && magazine.mode<=3) || magazine.mode==6?
        float(std::max<int>(0,magazine.ticks))*tick_seconds:0.f;
  }
}
void Simulation::step(const Controls &controls) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  step_impl(controls,{});
}
void Simulation::step_players(std::span<const Controls> controls) {
  NativeAuthorityBoundary::Call active(authority_calls_);
  if (controls.size()!=16) throw FormatError("multiplayer controls require sixteen slots");
  step_impl(controls[0],controls);
}
void Simulation::step_impl(const Controls &controls,std::span<const Controls> participants) {
  if(devices_.replicated())throw FormatError("client device replica cannot step an authoritative simulation");
  ++authority_calls_.mutations;
  const bool skip = controls.skip_cinematic && !skip_held_;
  skip_held_ = controls.skip_cinematic;
  if (skip && cinematic_skippable())
    request_revert();
  if (revert_pending_) {
    restore_checkpoint();
    return;
  }
  const auto record=[&](bool value,unsigned bit){if(value)training_.actions|=1u<<bit;};
  record(controls.jump,1);record(controls.fire,4);record(controls.grenade,5);record(controls.flashlight,20);
  record(controls.melee,6);record(controls.switch_weapon,7);record(controls.switch_grenade,8);
  record(controls.pickup,0);record(controls.accept,2);record(controls.cancel,3);
  record(controls.zoom>=0,9);record(controls.look_stick>0,23);record(controls.look_stick<0,24);
  record(controls.pitch-player_pitch_>0,10);record(controls.pitch-player_pitch_<0,11);
  const auto yaw_delta=std::remainder(controls.yaw-player_.yaw,6.283185307f);
  record(yaw_delta>0,12);record(yaw_delta<0,13);
  record(controls.start,18);record(controls.back,19);
  record(controls.forward>0,14);record(controls.forward<0,15);record(controls.right>0,16);record(controls.right<0,17);
  scripts_->tick(ticks_);
  const auto roots=scripts_->object_list_roots();
  if(multiplayer_script_lists_) {
    auto owned_roots=roots;
    std::erase(owned_roots,0x80000000u); // Existing native players-list owner.
    multiplayer_script_lists_->roots(owned_roots);multiplayer_script_lists_->collect();
  }
  for(auto at=script_lists_.begin();at!=script_lists_.end();) {
    if(std::find(roots.begin(),roots.end(),at->first)==roots.end())at=script_lists_.erase(at);
    else ++at;
  }
  if (revert_pending_) {
    restore_checkpoint();
    return;
  }
  devices_.synchronize(scene_);
  step_cinematic_object_animations();
  step_devices();
  for(auto event:devices_.take_events()){
    event.serial=++device_event_serial_;event.tick=ticks_+1;
    device_events_.push_back(event);
  }
  if(device_events_.size()>64)device_events_.erase(device_events_.begin(),device_events_.end()-64);
  synchronize_props();
  // D5DE0's ordinary update finishes with recent-damage fading, after recharge
  // and before this tick's projectile postpass. Dead units still fade. The
  // native unit/prop traversal supplies that phase; actor bounds are separate
  // from hlmt damage-info and ordinary players use the original defaults.
  const auto update_vitality=[&](Unit &unit) {
    if(!unit.exists || unit.bsp_hidden)return;
    if(!source_physics_movement_) {
      const auto update=update_unit_vitality(unit.vitality,unit.definition.vitality,tick_seconds);
      if(update.effect)
        record_unit_damage_effect(unit,*update.effect,
            {unit.source_player,unit.source_datum,unit.source_membership});
    }
    DamageFadeDefinition fade;
    if(!unit.controlled && unit.character!=null_datum)
      if(const auto character=content_.characters.find(unit.character);
          character!=content_.characters.end() && character->second.damage_fade)
        fade=*character->second.damage_fade;
    update_damage_feedback(unit.damage_feedback,30,unit.vitality.dead,
        unit.vitality.shield_depleted,false,fade);
    if(multiplayer_weapon_steps_ && !source_physics_movement_ && !(unit.definition.unit_flags&(1u<<11)))
      weapon_hand_feedback_tick(unit.source_weapon_hand_feedback);
    if(multiplayer_weapon_steps_) {
      // Ordinary unit C7E90 updates camouflage before its attached weapon's
      // firing owner. Actor-NONE multiplayer units use the measured branch.
      SourceCamouflageServices camouflage;
      camouflage.unit=[&](std::uint32_t datum) {
        if(datum!=unit.source_datum)throw FormatError("Unit camouflage lost its native object owner");
        auto& state=unit.source_camouflage;
        return SourceCamouflageUnitView{unit.source_action_word,state.primary,state.secondary,
            state.fade_speed,null_datum};
      };
      source_camouflage_fade(unit.source_datum,tick_seconds,camouflage);
    }
  };
  update_vitality(player_);
  for(auto &unit:actors_)update_vitality(unit);
  if(participants.empty())synchronize_weapon_pickups();
  // Original ordinary object damage update precedes this frame's projectile
  // postpass. Pending words therefore age on successive completed game ticks.
  props_.tick([&](const auto& event){
    prop_events_.push_back(event);
    if(const auto d=content_.prop_area_damage.find(event.damage);d!=content_.prop_area_damage.end())
      area_damage(*d->second,event.position,{event.owner.player,event.owner.unit,event.owner.designator},
          event.owner.native_owner,event.handle);
    if(event.remove){
      const auto& object=scene_.get(event.handle);
      prop_removals_.push_back({event.handle,std::uint32_t(object.placement),content_.objects[object.placement].definition,
          event.effect,event.effect_position,event.effect_forward,ticks_+1});
      scene_.destroy(event.handle);
    }
  },{.random=&random_state_});
  synchronize_props();
  auto input =
      player_input_enabled_ ? controls : Controls{0, 0, player_.yaw, 0};
  //187C9B..187DE1 records raw training input before suppressing gameplay.
  for(const unsigned bit:{0u,2u,3u}) {
    const bool held=bit==0?input.pickup:bit==2?input.accept:input.cancel;
    if(training_.armed&(1u<<bit) || training_.suppressed&(1u<<bit)) {
      if(!(training_.armed&(1u<<bit)) && !held)training_.suppressed&=~(1u<<bit);
      if(bit==0)input.pickup=false;else if(bit==2)input.accept=false;else input.cancel=false;
    }
  }
  float yaw_change=std::remainder(input.yaw-player_.yaw,6.283185307f),pitch_change=input.pitch-player_pitch_;
  if(training_.actions&(1u<<21))pitch_change=std::abs(pitch_change);
  else if(training_.actions&(1u<<22))pitch_change=-std::abs(pitch_change);
  if(training_.suppressed&(1u<<12))yaw_change=std::min(yaw_change,0.f);
  if(training_.suppressed&(1u<<13))yaw_change=std::max(yaw_change,0.f);
  if(training_.suppressed&(1u<<10))pitch_change=std::min(pitch_change,0.f);
  if(training_.suppressed&(1u<<11))pitch_change=std::max(pitch_change,0.f);
  input.yaw=player_.yaw+yaw_change;input.pitch=std::clamp(player_pitch_+pitch_change,-1.4922565f,1.4922565f);
  if (player_movement_disabled_) {
    input.forward = input.right = 0;
    input.jump = input.crouch = false;
  }
  if (presentation_.player_camera_enabled) {
    player_.yaw = input.yaw;
    player_pitch_ = input.pitch;
    player_.pitch = input.pitch; // Retain the actual native look input for unit action owners.
  } else {
    input.yaw = player_.yaw;
    input.pitch = player_pitch_;
  }
  if(source_campaign_)source_campaign_->step(*this,input);
  else step_unit_interactions(input);
  if (player_.exists && !player_.vitality.dead) {
    if(player_.source_parent!=null_datum) {}
    else if (debug_settings_.players[0].noclip || !(player_.definition.physics_flags & 8)) {
      move_unit(player_, {input.forward, -input.right, 0}, input.jump,
                input.crouch, true);
    } else {
      // Climbing/animation-driven character modes still use the workbench's
      // steering bridge until their retail state dispatch is recovered.
      const auto intent = movement_intent(
          content_.movement, {input.forward, -input.right, 0}, player_.crouch);
      const auto facing = direction(input.yaw);
      move_unit(player_,
                facing * intent.velocity.x +
                    Vec3{-facing.y, facing.x, 0} * intent.velocity.y,
                input.jump, input.crouch);
    }
    update_weapon(player_, input.fire, input.reload, -1, input.pitch,&input);
    if(participants.empty() && !source_campaign_)step_campaign_pickups(input);

    if(native_hand_weapon(player_,1))
      update_weapon(player_,input.fire_secondary,input.reload,-1,input.pitch,&input,1);
  }
  for (std::size_t i = 0; i < actors_.size(); ++i) {
    if(source_campaign_)break; // The retained original actor/object phase already ran.
    auto &a = actors_[i];
    if(a.source_parent!=null_datum) {
      // A controlled passenger/driver still supplies camera look input.
      // Attachment suppresses this native movement/weapon path, while the
      // source vehicle and camera consume the completed delivered command.
      if(a.controlled) {
        if(participants.empty() || i+1>=participants.size())
          throw FormatError("missing seated multiplayer participant input");
        a.yaw=participants[i+1].yaw;a.pitch=participants[i+1].pitch;
      }
      continue;
    }
    if(a.command.active && (!a.exists || a.vitality.dead))scripts_->stop_command(a.source_datum);
    if(a.command.active && !scripts_->command_running(null_datum,a.source_datum))a.command={};
    if(a.command.active && a.exists && !a.vitality.dead){update_command(a);continue;}
    if (!a.exists || a.vitality.dead || a.bsp_hidden)
      continue;
    if (a.controlled) {
      if (participants.empty() || i+1>=participants.size())
        throw FormatError("missing multiplayer participant input");
      const auto &input=participants[i+1];
      a.yaw=input.yaw; a.pitch=input.pitch;
      move_unit(a,{input.forward,-input.right,0},input.jump,input.crouch,true);
      update_weapon(a,input.fire,input.reload,int(i),input.pitch,
          multiplayer_weapon_steps_?&input:nullptr);
      if(multiplayer_weapon_steps_ && native_hand_weapon(a,1))
        update_weapon(a,input.fire_secondary,input.reload,int(i),input.pitch,&input,1);
      continue;
    }
    const auto &squad = content_.squads[a.squad];
    const auto &character = content_.characters.at(a.character);
    int target = -2;
    float distance = character.vision_distance;
    Vec3 delta{};
    auto consider = [&](const Unit &other, int id) {
      if(debug_settings_for(other).enemies_ignore)return;
      if (!other.exists || other.vitality.dead || other.bsp_hidden || id == int(i) || a.team < 0 || a.team >= 16 ||
          other.team < 0 || other.team >= 16 || peace_[a.team][other.team])
        return;
      const auto d = eye(other) - eye(a);
      const float range = length(d);
      if (range >= distance || dot(direction(a.yaw), normalized(d)) <
                                   std::cos(character.vision_angle))
        return;
      const auto occluder = world_->collision->raycast(eye(a), d, range);
      if (occluder && occluder->distance < range - .001f)
        return;
      target = id;
      distance = range;
      delta = d;
    };
    if (!(squad.flags & ((1 << 8) | (1 << 10)))) {
      consider(player_, -1);
      for (std::size_t j = 0; j < actors_.size(); ++j)
        consider(actors_[j], int(j));
    }
    const bool sees = target != -2;
    if (target != a.target) {
      a.awareness = 0;
      a.route.clear();
      a.target = target;
    }
    a.awareness = sees ? a.awareness + tick_seconds : 0;
    const bool attacking = sees && a.awareness >= character.acknowledgement;
    a.state = attacking ? Unit::Attack : sees ? Unit::Alert : Unit::Idle;
    if (sees)
      a.yaw = std::atan2(delta.y, delta.x);
    Vec3 desired{};
    if (attacking && distance > 8) {
      const auto goal =
          target == -1 ? player_.body.position : actors_[target].body.position;
      if (a.route.empty() || ticks_ % 30 == 0) {
        a.route = world_->navigation.route(a.body.position, goal);
        a.waypoint = 0;
      }
      while (a.waypoint < a.route.size()) {
        const auto d = a.route[a.waypoint] - a.body.position;
        if (std::hypot(d.x, d.y) > a.body.radius * 2)
          break;
        ++a.waypoint;
      }
      auto d = a.waypoint < a.route.size()
                   ? a.route[a.waypoint] - a.body.position
                   : delta;
      d.z = 0;
      // A missing campaign route means unreachable, rather than steering
      // through a wall. Maps without authored navigation retain local steering.
      if (!a.route.empty() || content_.navigation.empty())
        desired = normalized(d) * content_.movement.forward;
    }
    move_unit(a, desired, false, false);
    update_weapon(a, attacking, a.weapon.loaded == 0 && a.weapon.reserve > 0,
                  int(i), std::atan2(delta.z, std::hypot(delta.x, delta.y)));
  }
  if(multiplayer_weapon_steps_ && !source_physics_movement_) {
    const auto passive=[&](Unit &unit) {
      if(!unit.exists)return;
      for(int slot=0;slot<4;++slot)
        if(slot!=unit.source_inventory.current[0] && slot!=unit.source_inventory.current[1])
          if(auto *weapon=native_inventory_weapon(unit,slot))update_passive_weapon(*weapon,&unit);
    };
    passive(player_);for(auto &unit:actors_)passive(unit);
  }
  // The original source object phase owns pending weapon dispatch too.
  if (multiplayer_weapon_steps_ && !source_physics_movement_ && !source_campaign_) {
    auto dispatch = [&](Unit &u, int owner, float pitch) {
      if (!u.exists || u.vitality.dead) return;
      for(unsigned hand=0;hand<2;++hand) {
        auto *state=hand?native_hand_weapon(u,hand):&u.weapon;
        if(!state || state->definition==null_datum)continue;
        const auto &weapon=content_.weapons.at(state->definition);
        if(!weapon.smg) {
          if(weapon.charged_cycle) {
            for(unsigned index=0;index<weapon.barrel_views.size();++index) {
              auto &barrel=index?state->alternate_barrel:state->smg.barrel;
              if(!(barrel.flags&2u))continue;
              barrel.flags&=~2u;state->shader.weapon_flags&=~1u;
              shot(u,owner,pitch,nullptr,nullptr,state,true,std::int16_t(index));
            }
          } else if(weapon.barrel_timing && (state->smg.barrel.flags&2u)) {
            state->smg.barrel.flags&=~2u;state->shader.weapon_flags&=~1u;
            shot(u,owner,pitch,nullptr,nullptr,state,true);
          }
          continue;
        }
        if(!content_.original_data)throw FormatError("Original direction data is required for SMG dispatch");
        const auto aim=direction(u.yaw,pitch);
        const auto prepared=smg_dispatch(state->smg,*weapon.smg,aim,random_state_,content_.original_data->weapon_directions,weapon_hand_blend(u,*state));
        if(prepared) {
          const auto inherited=smg_inherited_velocity(aim,*prepared,u.body.velocity);
          const auto velocity=smg_projectile_velocity(*prepared,inherited,weapon.projectile_speed);
          shot(u,owner,pitch,&*prepared,&velocity,state);
        }
      }
    };
    // Native slots are stable and ascending. All weapon updates finish before
    // consuming their pending requests, as in 1019F0's object postpass.
    dispatch(player_, -1, player_pitch_);
    for (std::size_t i = 0; i < actors_.size(); ++i)
      dispatch(actors_[i], int(i), actors_[i].pitch);
  }
  for (auto &p : projectiles_) {
    const auto &base=content_.weapons.at(p.weapon);
    const auto &w=p.barrel_index<0?base:*base.barrel_views.at(unsigned(p.barrel_index));
    const auto flight = projectile_flight(
        p.position, p.velocity,
        0.f - projectile_acceleration(w.projectile_gravity, retail_gravity),
        tick_seconds, p.traveled, w.range);
    p.velocity = flight.velocity;
    const auto movement = flight.displacement;
    const float distance = length(movement);
    if (trace(p.position, movement, distance, w, p.owner, p.captured_owner,p.source_datum,&p.velocity,p.traveled))
      p.traveled = INFINITY;
    else {
      p.position = flight.position;
      p.traveled = flight.traveled;
      if (flight.range_exhausted)
        p.traveled = INFINITY;
    }
  }
  std::erase_if(projectiles_, [&](const auto &p) {
    const auto &base=content_.weapons.at(p.weapon);
    const float range = p.barrel_index<0?base.range:base.barrel_views.at(unsigned(p.barrel_index))->range;
    const bool remove = !std::isfinite(p.traveled) || (range > 0 && p.traveled > range);
    if (remove && release_object_ && p.source_datum != null_datum)
      release_object_(p.source_datum);
    return remove;
  });
  if(grenade_sandbox_ && !source_physics_movement_)grenade_sandbox_->update_projectiles(ticks_+1,collision(),
      [this](const PropAreaDamageDefinition& definition,Vec3 origin,std::uint32_t player,
          std::uint32_t unit,std::int16_t designator,int owner,std::uint32_t projectile_definition) {
        area_damage(definition,origin,{player,unit,designator},owner,null_datum,projectile_definition);
      },release_object_,[this](std::size_t placement){
        const auto type=content_.objects.at(placement).type;
        if(type<0||type>255)throw FormatError("Invalid projectile collision scenario object type");
        return std::uint8_t(type);
      });
  presentation_.tick();
  if(training_.enabled && difficulty_<2 && player_.exists && !training_definitions_.empty()) {
    std::array<bool,32> admitted{},completed{};
    auto &automatic=training_.automatic;
    admitted[0]=(training_.requests&1)!=0;completed[0]=controls.flashlight;
    admitted[1]=(training_.requests&2)!=0;completed[1]=controls.crouch;
    admitted[31]=(training_.requests&4)!=0;completed[31]=controls.flashlight;
    if(automatic.current>=0 && automatic.transitions[automatic.current].state>=1 &&
        automatic.transitions[automatic.current].state<=3 && completed[automatic.current]) {
      if(automatic.current==0)training_.requests&=~1u;
      if(automatic.current==1)training_.requests&=~2u;
      if(automatic.current==31)training_.requests&=~4u;
    }
    // Original1E7AB0/1E83C0 on-foot look and movement admissions. They
    // read live orientation/location, not accumulated action-test bits.
    const bool on_foot=player_.source_parent==null_datum;
    admitted[12]=admitted[13]=!presentation_.cinematic && on_foot;
    admitted[2]=admitted[12] && automatic.learned[12]==3;
    if(automatic.current==2 || automatic.current==12 || automatic.current==13) {
      if(!automatic.baseline) {
        automatic.origin=automatic.current==13
            ?std::array<float,3>{player_.body.position.x,player_.body.position.y,player_.body.position.z}
            :std::array<float,3>{player_.yaw,player_pitch_,0};
        automatic.baseline=true;automatic.turned_left=automatic.turned_right=false;
      } else if(automatic.current==13) {
        const auto p=player_.body.position;
        const auto dx=p.x-automatic.origin[0],dy=p.y-automatic.origin[1],dz=p.z-automatic.origin[2];
        completed[13]=dx*dx+dy*dy+dz*dz>.25f;
      } else {
        const auto yaw=std::remainder(player_.yaw-automatic.origin[0],6.283185307f);
        const auto pitch=player_pitch_-automatic.origin[1];
        completed[12]=std::abs(yaw)>.1745329201f || std::abs(pitch)>.1745329201f;
        automatic.turned_left|=yaw<-.3490658402f;automatic.turned_right|=yaw>.3490658402f;
        completed[2]=automatic.turned_left && automatic.turned_right;
      }
    }
    admitted[22]=player_.vitality.shield<.25f ||
        (automatic.current==22 && automatic.learned[21]==3);
    // Original unit10B bit1 owns recharge admission; do not infer it from
    // a native timer that may have different scheduling.
    admitted[21]=(player_.source_unit_10b&2) &&
        (player_.vitality.shield<.25f || automatic.current==21);
    completed[21]=completed[22]=player_.vitality.shield>=.9999f;
    automatic.advance(training_definitions_,admitted,completed);
  }
  subtitle_.advance(tick_seconds);
  for(auto &title:titles_)if(title.index>=0 && std::size_t(title.index)<content_.cinematic_titles.size()) {
    ++title.ticks;const auto &definition=content_.cinematic_titles[title.index];
    if(title.ticks*tick_seconds>=definition.up_time+definition.fade_out)title={};
  }
  ++ticks_;
  // Retail 0x226050 services requests after script/world updates; the main
  // loop captures after 0x137FE0 has incremented game time.
  // Original main loop 0x12B450 commits streaming before the pending save.
  // No script in this dispatch observes half-loaded geometry.
  commit_bsp();
  if (save_pending_)
    capture_checkpoint();
}
ScriptValue Simulation::host(const std::string &name,
                             const std::vector<ScriptValue> &args) {
  auto arity = [&](std::size_t count) {
    if (args.size() != count)
      throw FormatError("invalid " + name + " arity");
  };
  if(name=="object_set_permutation") {
    arity(3);
    if(args[0].bits==null_datum)return {};
    if(!source_campaign_)throw FormatError("Object permutation requires its retained TU1.5 world");
    source_object_set_permutation_tu15(source_campaign_->objects().tu15_world(),
        args[0].bits==0?player_.source_datum:args[0].bits,args[1].bits,args[2].bits);
    return {};
  }
  if(name=="cinematic_lighting_set_primary_light" || name=="cinematic_lighting_set_secondary_light") {
    arity(5);const Vec3 color{float(args[2].number()),float(args[3].number()),float(args[4].number())};
    if(source_campaign_) {
      auto &world=source_campaign_->objects().tu15_world();
      if(name=="cinematic_lighting_set_primary_light")source_campaign_primary_light_tu15(world,float(args[0].number()),float(args[1].number()),color);
      else source_campaign_secondary_light_tu15(world,float(args[0].number()),float(args[1].number()),color);
    }
    auto &lighting=mutable_cinematic_state().lighting;
    if(name=="cinematic_lighting_set_primary_light")
      frontend_cinematic_primary(lighting,float(args[0].number()),float(args[1].number()),color);
    else frontend_cinematic_secondary(lighting,float(args[0].number()),float(args[1].number()),color);
    return {};
  }
  if(name=="cinematic_lighting_set_ambient_light") {
    arity(3);
    if(source_campaign_)source_campaign_ambient_light_tu15(source_campaign_->objects().tu15_world(),
        {float(args[0].number()),float(args[1].number()),float(args[2].number())});
    frontend_cinematic_ambient(mutable_cinematic_state().lighting,
        {float(args[0].number()),float(args[1].number()),float(args[2].number())});return {};
  }
  if(name=="cinematic_lightmap_shadow_enable" || name=="cinematic_lightmap_shadow_disable") {
    arity(0);
    if(!source_campaign_)throw FormatError("Cinematic lightmap shadow control requires its retained TU1.5 world");
    source_campaign_lightmap_shadow_tu15(source_campaign_->objects().tu15_world(),
        name=="cinematic_lightmap_shadow_enable");
    return {};
  }
  if(name=="render_lights_enable_cinematic_shadow") {
    arity(4);
    if(!source_campaign_)throw FormatError("Cinematic shadow control requires its retained TU1.5 world");
    source_cinematic_shadow_control_tu15(source_campaign_->objects().tu15_world(),
        args[0].truth(),args[1].bits,args[2].bits,float(args[3].number()));
    return {};
  }
  if(name=="object_uses_cinematic_lighting") {
    arity(2);if(args[0].bits==null_datum)return {};
    // The source selector changes bit24 on an actual live object. Validate
    // the full handle before retaining the equivalent per-object admission.
    (void)object_center(args[0].bits);
    if(source_campaign_)source_campaign_object_lighting_tu15(source_campaign_->objects().tu15_world(),
        args[0].bits==0?player_.source_datum:args[0].bits,args[1].truth());
    auto &objects=mutable_cinematic_state().lit_objects;
    if(args[1].truth())objects.insert(args[0].bits);else objects.erase(args[0].bits);
    return {};
  }
  if(name=="object_set_function_variable" || name=="object_clear_function_variable") {
    arity(name=="object_set_function_variable"?4:2);
    if(source_campaign_) {
      auto &world=source_campaign_->objects().tu15_world();
      const auto object=args[0].bits==0?player_.source_datum:args[0].bits;
      if(name=="object_set_function_variable")source_object_function_set_tu15(world,object,args[1].bits,
          float(args[2].number()),float(args[3].number()));
      else source_object_function_clear_tu15(world,object,args[1].bits);
      scene_.publish_source_function_samples(source_object_function_samples_tu15(world));return {};
    }
    if(name=="object_set_function_variable")scene_.set_function_variable(args[0].bits,args[1].bits,
        float(args[2].number()),float(args[3].number()),int(ticks_));
    else scene_.clear_function_variable(args[0].bits,args[1].bits);
    return {};
  }
  if(name=="rasterizer_bloom_override" || name=="rasterizer_bloom_override_threshold" ||
      name=="rasterizer_bloom_override_brightness") {
    arity(1);auto &bloom=mutable_cinematic_state().bloom;
    if(name=="rasterizer_bloom_override")bloom.enabled=args[0].truth();
    else if(name=="rasterizer_bloom_override_threshold")bloom.threshold=float(args[0].number());
    else bloom.brightness=float(args[0].number());
    return {};
  }
  if(name=="cinematic_screen_effect_set_depth_of_field") {
    arity(7);
    if(!source_campaign_)throw FormatError("Cinematic depth of field requires its retained TU1.5 world");
    std::array<std::uint32_t,7> words;
    for(unsigned i=0;i<words.size();++i)words[i]=scalar_cast_bits(args[i].bits,args[i].type,6);
    source_campaign_depth_of_field_tu15(source_campaign_->objects().tu15_world(),words);
    return {};
  }
  if(name=="cinematic_set_far_clip_distance") {
    arity(1);
    if(!source_campaign_)throw FormatError("Cinematic far clip control requires its retained TU1.5 world");
    source_campaign_far_clip_tu15(source_campaign_->objects().tu15_world(),
        std::bit_cast<float>(scalar_cast_bits(args[0].bits,args[0].type,6)));
    return {};
  }
  if(name=="cinematic_screen_effect_start") {
    arity(1);auto &state=mutable_cinematic_state();
    // The output bank is already zero until an implemented screen command
    // creates an effect. Original start preserves the timed control bank.
    if(source_campaign_) {
      auto &memory=source_campaign_->objects().memory();source_campaign_screen_start_tu15(memory,args[0].truth());
      state.crossfade=source_campaign_crossfade_state_tu15(memory);
    } else state.crossfade.screen_start(args[0].truth());
    state.screen_initialized=true;state.screen_active=true;return {};
  }
  if(name=="cinematic_screen_effect_set_crossfade" || name=="cinematic_screen_effect_set_crossfade2") {
    arity(name=="cinematic_screen_effect_set_crossfade"?1:2);auto &state=mutable_cinematic_state();
    if(!state.screen_consumer)throw FormatError("Cinematic crossfade requires its captured-frame GPU consumer");
    const auto strength=args.size()==2?float(args[1].number()):1.f;
    if(source_campaign_) {
      auto &memory=source_campaign_->objects().memory();
      source_campaign_crossfade_set_tu15(memory,float(args[0].number()),strength);
      state.crossfade=source_campaign_crossfade_state_tu15(memory);
    } else {state.crossfade.set(float(args[0].number()),state.render_clock);state.crossfade.strength=strength;}
    return {};
  }
  if(name=="objects_attach") {
    arity(4);attach_cinematic_objects(args[0].bits,args[1].bits,args[2].bits,args[3].bits);return {};
  }
  if(name=="effect_new_on_object_marker") {
    arity(3);
    if(args[0].bits==null_datum || args[1].bits==null_datum)return {};
    if(!source_campaign_)
      throw FormatError("Script marker effect requires its retained TU1.5 allocation and frame consumers");
    source_script_marker_effect_tu15(source_campaign_->objects().tu15_world(),
        args[0].bits,args[1].bits,args[2].bits);
    return {};
  }
  if(name=="cinematic_screen_effect_stop") {
    arity(0);auto &state=mutable_cinematic_state();
    if(source_campaign_) {
      auto &world=source_campaign_->objects().tu15_world();source_campaign_screen_stop_tu15(world);
      state.crossfade=source_campaign_crossfade_state_tu15(world.memory());
    } else state.crossfade.screen_stop();
    state.screen_active=false;state.screen_initialized=false;return {};
  }
  const auto ai_units=[&](const ScriptValue &value) {
    if(value.type!=19 || (value.bits!=null_datum && value.bits>>30))throw FormatError("Unsupported AI actor reference");
    std::vector<Unit *> units;
    if(value.bits!=null_datum)for(auto &actor:actors_)if(actor.exists && actor.squad==(value.bits&65535))units.push_back(&actor);
    return units;
  };
  if(name=="device_get_position" || name=="device_get_power") {
    arity(1);
    if(source_physics_movement_ && !devices_.replicated()) {
      if(!source_device_script_)throw FormatError("TU1.5 device scripts require their original bank owner");
      return source_device_script_(name,args);
    }
    devices_.synchronize(scene_);return ScriptValue::numeric(devices_.script_value(args[0].bits,name=="device_get_power"));
  }
  if(name=="device_set_position" || name=="device_set_position_immediate" || name=="device_set_power") {
    arity(2);
    if(source_physics_movement_ && !devices_.replicated()) {
      if(!source_device_script_)throw FormatError("TU1.5 device scripts require their original bank owner");
      return source_device_script_(name,args);
    }
    devices_.synchronize(scene_);const bool power=name=="device_set_power";
    const bool changed=devices_.script_set(args[0].bits,power,float(args[1].number()),power || name.ends_with("immediate"));
    return name=="device_set_position"?ScriptValue::numeric(changed,5):ScriptValue{};
  }
  if(name=="device_group_get") {
    arity(1);
    if(source_physics_movement_ && !devices_.replicated()) {
      if(!source_device_script_)throw FormatError("TU1.5 group scripts require their original bank owner");
      return source_device_script_(name,args);
    }
    return ScriptValue::numeric(devices_.script_group_value(args[0].bits));
  }
  if(name=="device_group_set" || name=="device_group_set_immediate") {
    arity(name=="device_group_set"?3:2);
    if(source_physics_movement_ && !devices_.replicated()) {
      if(!source_device_script_)throw FormatError("TU1.5 group scripts require their original bank owner");
      return source_device_script_(name,args);
    }
    const auto group=args[name=="device_group_set"?1:0].bits;
    const bool changed=devices_.script_group_set(group,float(args.back().number()),name.ends_with("immediate"));
    return name=="device_group_set"?ScriptValue::numeric(changed,5):ScriptValue{};
  }
  if(name=="ai_dialogue_enable") {arity(1);if(args[0].type!=5)throw FormatError("Invalid AI dialogue flag");ai_dialogue_enabled_=args[0].truth();return {};}
  if(name=="ai_get_object" || name=="ai_get_unit") {
    arity(1);
    if(args[0].type!=19)throw FormatError("Unsupported AI actor reference");
    const auto type=std::uint16_t(name=="ai_get_unit"?51:50);
    if(source_campaign_)return {type,source_campaign_->ai_object(args[0].bits),{}};
    const auto units=ai_units(args[0]);return {type,units.empty()?null_datum:units.front()->source_datum,{}};
  }
  if(name=="ai_cannot_die") {
    arity(2);if(args[1].type!=5)throw FormatError("Invalid AI immortality flag");
    for(auto *actor:ai_units(args[0]))actor->source_unit_10b=(actor->source_unit_10b&~0x40)|(args[1].truth()?0x40:0);return {};
  }
  if(name=="cs_run_command_script" || name=="cs_command_script_running") {
    arity(2);if(args[1].type!=21)throw FormatError("Invalid command script reference");bool running=false;
    for(auto *actor:ai_units(args[0])) {
      if(name=="cs_run_command_script"){actor->command={};actor->command.active=true;actor->route.clear();scripts_->start_command(args[1].bits&65535,actor->source_datum);}
      else running|=scripts_->command_running(args[1].bits&65535,actor->source_datum);
    }
    return name=="cs_run_command_script"?ScriptValue{}:ScriptValue::numeric(running,5);
  }
  if(name.starts_with("cs_")) {
    const auto handle=scripts_->command_actor();if(!handle)throw FormatError("Command has no live actor context");
    auto &actor=script_unit(*handle);auto &command=actor.command;
    if(name=="cs_enable_moving" || name=="cs_enable_looking" || name=="cs_enable_pathfinding_failsafe" || name=="cs_ignore_obstacles") {
      arity(1);if(args[0].type!=5)throw FormatError("Invalid command flag");
      if(name=="cs_enable_moving")command.moving=args[0].truth();
      else if(name=="cs_enable_looking")command.looking=args[0].truth();
      else if(name=="cs_enable_pathfinding_failsafe")command.pathfinding_failsafe=args[0].truth();
      else command.ignore_obstacles=args[0].truth();return {};
    }
    if(name=="cs_force_combat_status"){arity(1);command.combat_status=std::int16_t(args[0].bits);return {};}
    if(name=="cs_go_to" || name=="cs_go_to_and_face") {
      arity(name=="cs_go_to"?1:2);command.goal_reference=args[0].bits;command.goal=script_point(args[0].bits);actor.route.clear();actor.waypoint=0;
      if(args.size()==2){command.face_reference=args[1].bits;command.face_point=script_point(args[1].bits);}
      scripts_->wait_for_command();return {};
    }
    if(name=="cs_face_player") {arity(1);command.face_player=args[0].truth();command.face_point.reset();command.face_reference.reset();command.face_object.reset();return {};}
    if(name=="cs_face" || name=="cs_look") {
      arity(2);auto &point=name=="cs_face"?command.face_point:command.look_point;
      auto &reference=name=="cs_face"?command.face_reference:command.look_reference;
      reference=args[0].truth()?std::optional<std::uint32_t>(args[1].bits):std::nullopt;
      point=args[0].truth()?std::optional<Vec3>(script_point(args[1].bits)):std::nullopt;
      if(name=="cs_face"){command.face_player=false;command.face_object.reset();}else command.look_object.reset();return {};
    }
    if(name=="cs_face_object" || name=="cs_look_object" || name=="cs_aim_object") {
      arity(2);if(args[0].truth() && args[1].bits!=null_datum)(void)object_center(args[1].bits);
      auto &target=name=="cs_face_object"?command.face_object:command.look_object;
      target=args[0].truth() && args[1].bits!=null_datum?std::optional<std::uint32_t>(args[1].bits):std::nullopt;
      if(name=="cs_face_object"){command.face_player=false;command.face_point.reset();command.face_reference.reset();}else {command.look_point.reset();command.look_reference.reset();}return {};
    }
    throw FormatError("Unsupported actor command "+name);
  }
  if(name=="unit_set_maximum_vitality" || name=="unit_set_current_vitality") {
    arity(3);if(args[0].bits==null_datum)return {};auto &unit=script_unit(args[0].bits);
    if(name=="unit_set_current_vitality" && unit.vitality.dead)return {};
    const float body=float(args[1].number()),shield=float(args[2].number());
    if(name=="unit_set_maximum_vitality") {
      unit.definition.vitality.health=body;unit.definition.vitality.shield=shield;
      unit.vitality.body=body>0?1.f:0.f;unit.vitality.shield=shield>0?1.f:0.f;
    } else {
      unit.vitality.body=unit.definition.vitality.health>0?std::min(1.f,body/unit.definition.vitality.health):0;
      unit.vitality.shield=unit.definition.vitality.shield>0?std::min(1.f,shield/unit.definition.vitality.shield):0;
      unit.vitality.dead=unit.vitality.body<=0;
    }
    return {};
  }
  if(name=="unit_set_emotional_state") {
    arity(4);if(args[0].bits==null_datum)return {};
    if(source_campaign_) {
      const auto emotion=source_cinematic_emotion_name_tu15(
          animation_loader_.target<CinematicAnimationLoader>(),args[1].bits);
      source_unit_cinematic_emotion_tu15(source_campaign_->objects().tu15_world(),
          args[0].bits==0?player_.source_datum:args[0].bits,emotion,
          float(args[2].number()),std::int16_t(args[3].bits));return {};
    }
    auto &unit=script_unit(args[0].bits);
    unit.emotional_state=args[1].bits;unit.emotional_weight=float(args[2].number());unit.emotional_transition=std::int16_t(args[3].bits);return {};
  }
  if(name=="scenery_animation_start" || name=="scenery_animation_start_loop" ||
      name=="scenery_animation_start_relative" || name=="scenery_animation_start_relative_loop") {
    const bool relative=name=="scenery_animation_start_relative" || name=="scenery_animation_start_relative_loop";
    arity(relative?4:3);
    start_cinematic_object_animation(args[0].bits,args[1].bits,args[2].bits,
        relative?args[3].bits:null_datum,false,name.ends_with("_loop"),false);
    return {};
  }
  if(name=="custom_animation_relative" || name=="custom_animation_relative_loop") {
    arity(5);return ScriptValue::numeric(start_cinematic_object_animation(args[0].bits,args[1].bits,
        args[2].bits,args[4].bits,true,name.ends_with("_loop"),args[3].truth()),5);
  }
  if(name=="scenery_get_animation_time" || name=="unit_get_custom_animation_time") {
    arity(1);
    if(source_campaign_)return ScriptValue::numeric(source_campaign_cinematic_animation_time_tu15(
        source_campaign_->objects().tu15_world(),args[0].bits==0?player_.source_datum:args[0].bits,
        name=="unit_get_custom_animation_time"),7);
    const auto *animation=object_animation(args[0].bits);
    return ScriptValue::numeric(animation && animation->channel?cinematic_animation_time(*animation->channel):0,7);
  }
  if(name=="custom_animation" || name=="custom_animation_loop") {
    arity(4);if(args[0].bits==null_datum)return ScriptValue::numeric(false,5);
    if(source_campaign_)return ScriptValue::numeric(start_cinematic_object_animation(
        args[0].bits,args[1].bits,args[2].bits,null_datum,true,name=="custom_animation_loop",args[3].truth()),5);
    if(!animation_loader_)throw FormatError("Actor custom animation has no source loader");
    auto clip=std::make_shared<const AnimationClip>(animation_loader_(args[1].bits,args[2].bits));
    if(!clip->frame_count)throw FormatError("Custom animation has no authored frames");
    const auto actor=std::find_if(actors_.begin(),actors_.end(),[&](const auto &a){return a.exists && a.source_datum==args[0].bits;});
    if(args[0].bits==0 || actor!=actors_.end()) {
      auto &unit=args[0].bits==0?player_:*actor;
      unit.custom_animation=std::move(clip);unit.custom_animation_tick=ticks_;unit.custom_animation_loop=name=="custom_animation_loop";
    } else {
      const auto &object=scene_.get(args[0].bits);
      if(content_.objects.at(object.placement).type!=0)throw FormatError("Custom animation requires a live biped");
      object_animations_.insert_or_assign(args[0].bits,ObjectAnimation{std::move(clip),ticks_,name=="custom_animation_loop"});
    }
    return ScriptValue::numeric(true,5);
  }
  if(name=="objects_can_see_object") {
    arity(3);if(args[1].bits==null_datum)return ScriptValue::numeric(false,5);
    const float angle=float(args[2].number())*.017453292519943295f;
    bool visible=false;for(const auto handle:script_objects(args[0].bits)) {
      const auto &unit=script_unit(handle);const auto delta=object_center(args[1].bits)-eye(unit);const float distance=length(delta);
      // Retail29F7A0 ->CC2B0 is a facing-cone test with a strict boundary;
      // it never queries occlusion, including the target machine's collision.
      visible|=distance>0 && dot(direction(unit.yaw,handle==0?player_pitch_:unit.pitch),delta/distance)>std::cos(angle);
    }
    return ScriptValue::numeric(visible,5);
  }
  if(name=="objects_distance_to_flag" || name=="objects_distance_to_object") {
    arity(2);if(args[0].bits==null_datum || (name=="objects_distance_to_object" && args[1].bits==null_datum))return ScriptValue::numeric(-1);
    Vec3 target;if(name=="objects_distance_to_object")target=object_center(args[1].bits);
    else {const auto flag=std::int16_t(args[1].bits);if(flag<0 || std::size_t(flag)>=content_.cutscene_flags.size())throw FormatError("Invalid distance flag");target=content_.cutscene_flags[flag].position;}
    float distance=-1;for(const auto handle:script_objects(args[0].bits)){const float d=length(target-object_center(handle));if(distance<0 || d<distance)distance=d;}
    return ScriptValue::numeric(distance);
  }
  if(name=="object_get_health" || name=="object_set_shield") {
    arity(name=="object_get_health"?1:2);if(args[0].bits==null_datum)return ScriptValue::numeric(-1);
    auto &unit=script_unit(args[0].bits);if(name=="object_set_shield"){unit.vitality.shield=float(args[1].number());return {};}
    return ScriptValue::numeric(unit.vitality.body);
  }
  if(name=="unit_has_weapon") {
    arity(2);if(args[0].bits==null_datum || args[1].bits==null_datum)return ScriptValue::numeric(false,5);
    const auto &unit=script_unit(args[0].bits);
    // The HSC operand is a weapon tag datum, including its salt. The text
    // field is not populated for a compiled tag literal.
    bool found=unit.weapon.definition==args[1].bits;
    for(const auto &weapon:unit.source_inventory.inactive_weapons)
      if(weapon && weapon->definition==args[1].bits)found=true;
    return ScriptValue::numeric(found,5);
  }
  if(name=="cinematic_set_title" || name=="cinematic_set_title_delayed") {
    arity(name=="cinematic_set_title"?1:2);const auto index=std::int16_t(args[0].bits);
    if(index<0 || std::size_t(index)>=content_.cinematic_titles.size())throw FormatError("Invalid cinematic title");
    for(auto &title:titles_)if(title.index==-1){title={index,std::int16_t(args.size()>1?-std::nearbyint(args[1].number()*30):0)};break;}
    return {};
  }
  if(name=="cinematic_subtitle") {
    arity(2);if(args[0].type!=11 || args[1].type!=6)throw FormatError("Invalid cinematic subtitle arguments");
    if(!subtitle_localized_)throw FormatError("Cinematic subtitle locale owner unavailable");
    if(subtitle_localized_(args[0].bits))subtitle_.start(args[0].bits,float(args[1].number()));
    return {};
  }
  if(name=="sound_impulse_language_time" || name=="sound_impulse_time") {
    // TU1.5 descriptors459930/459944/459958 return LONG. The VM applies
    // the enclosing expression's conversion after this declared return.
    if(name=="sound_impulse_time" && args.size()==2) {
      if(args[0].type!=32)throw FormatError("Invalid sound time arguments");
      const auto ticks=std::bit_cast<std::int32_t>(scalar_cast_bits(args[1].bits,args[1].type,8));
      if(const auto result=source_query_script_sound_explicit_time_tu15(*this,args[0].bits,ticks))
        return ScriptValue{8,std::bit_cast<std::uint32_t>(*result),{}};
    } else {
      arity(1);if(args[0].type!=32)throw FormatError("Invalid sound time arguments");
      const auto result=name=="sound_impulse_time"?
          source_query_script_sound_stored_time_tu15(*this,args[0].bits):
          source_query_script_sound_language_time_tu15(*this,args[0].bits);
      if(result)return ScriptValue{8,std::bit_cast<std::uint32_t>(*result),{}};
    }
    // Preserve the unbound development path, including its one-argument
    // contract. Bound failures never use PCM or approximate impulse clocks.
    arity(1);if(args[0].type!=32 || !sound_duration_)throw FormatError("Sound duration owner unavailable");
    if(name=="sound_impulse_time")return ScriptValue::numeric(std::nearbyint(sound_duration_(args[0].bits)*30.f),8);
    for(const auto &clock:impulse_clocks_)if(clock.datum==args[0].bits)
      return ScriptValue::numeric(clock.start+clock.duration>ticks_?clock.start+clock.duration-ticks_:0,8);
    return ScriptValue::numeric(0,8);
  }
  if(name=="cache_block_for_one_frame") {
    arity(0);if(!predictor_)throw FormatError("Native asset residency unavailable");
    predictor_({Prediction::Barrier});return {};
  }
  if(name=="object_type_predict") {
    arity(1);
    if(args[0].type!=38)throw FormatError("Invalid object prediction argument");
    if(args[0].bits==null_datum)return {};
    if(!predictor_)throw FormatError("Native asset residency unavailable");
    predictor_({Prediction::Object,args[0].bits});return {};
  }
  if(name=="objects_predict_high") {
    arity(1);if(!predictor_)throw FormatError("Native asset residency unavailable");
    for(const auto handle:script_objects(args[0].bits))predictor_({Prediction::Model,script_unit(handle).definition.model,-1});
    return {};
  }
  if (name == "sound_impulse_predict" || name == "predict_model_section" ||
      name == "predict_structure_section") {
    arity(name == "sound_impulse_predict" ? 1 : name == "predict_model_section" ? 2 : 3);
    const auto tag_type=name=="sound_impulse_predict" ? 32 : name=="predict_model_section" ? 41 : 26;
    if (args[0].type!=tag_type || (args.size()>1 && args[1].type!=8) ||
        (args.size()>2 && args[2].type!=5))
      throw FormatError("invalid " + name + " arguments");
    Prediction p{name == "sound_impulse_predict" ? Prediction::Sound :
                 name == "predict_model_section" ? Prediction::Model : Prediction::Structure,
                 args[0].bits,args.size()>1 ? int(args[1].number()) : 0,
                 args.size()>2 && args[2].truth()};
    if (p.kind == Prediction::Structure) {
      const int target=std::int16_t(p.datum);
      // Retail 16E510 admits only the loaded BSP and ignores section NONE.
      if (target != int(bsp_) || p.section == -1) return {};
      p.datum=std::uint32_t(target);
    } else if (p.datum == null_datum) return {};
    if (!predictor_) throw FormatError("native asset residency unavailable");
    predictor_(p);
    return {};
  }
  if (name == "object_cinematic_lod") {
    arity(2);
    if(source_campaign_)source_campaign_cinematic_lod_tu15(
        source_campaign_->objects().tu15_world(),args[0].bits,args[1].truth());
    if (args[0].bits != null_datum && args[0].bits != 0)
      scene_.get(args[0].bits).cinematic_lod=args[1].truth();
    // Current native admission uses highest LOD without distance fades.
    // Future fade/shadow selection must consume this retail bit19 flag.
    return {};
  }
  if (name == "cinematic_suppress_bsp_object_creation") {
    arity(1); suppress_automatic_objects_=args[0].truth(); return {};
  }
  if (name == "sound_class_set_gain" || name == "sound_class_set_gain_db") {
    arity(3);
    if (args[0].type != 9 || args[1].type != 6 || args[2].type != 7)
      throw FormatError("invalid sound class arguments");
    sound_classes_.set(args[0].text,float(args[1].number()),
        float(std::int16_t(args[2].bits))*tick_seconds,name.ends_with("_db"));
    return {};
  }
  if (name == "cinematic_enable_ambience_details") {
    arity(1); ambience_details_enabled_=args[0].truth(); return {};
  }
  if (name == "sound_suppress_ambience_update_on_revert") {
    arity(0); suppress_ambience_rebuild_=true; return {};
  }
  if(name=="hud_enable_training" || name=="hud_show_training_text" || name=="hud_set_training_text") {
    arity(1);const bool text=name=="hud_set_training_text";
    if(args[0].type!=(text?11:5))throw FormatError("invalid training argument type");
    if(text)training_.text=args[0].bits;else if(name=="hud_enable_training")training_.enabled=args[0].truth();else training_.show=args[0].truth();return {};
  }
  if(name=="player_training_activate_flashlight" || name=="player_training_activate_crouch" || name=="player_training_activate_stealth") {
    arity(0);training_.requests|=name.ends_with("flashlight")?1:name.ends_with("crouch")?2:4;return {};
  }
  if(name=="controller_get_look_invert") {
    arity(0);if(!look_invert_get_)throw FormatError("Controller profile owner is unavailable");return ScriptValue::numeric(look_invert_get_(),5);
  }
  if(name=="controller_set_look_invert") {
    arity(1);if(args[0].type!=5 || !look_invert_set_)throw FormatError("Controller profile owner is unavailable");look_invert_set_(args[0].truth());return {};
  }
  if(name=="player_action_test_reset") {arity(0);training_.actions=training_.armed=0;return {};}
  if(name=="player_action_test_look_up_begin" || name=="player_action_test_look_down_begin") {
    arity(0);training_.suppressed|=0x3000;training_.actions|=name.ends_with("up_begin")?0x200000:0x400000;return {};
  }
  if(name=="player_action_test_look_pitch_end") {arity(0);training_.suppressed&=~0x3000u;return {};}
  if(name=="player0_looking_up" || name=="player0_looking_down") {
    arity(0);return ScriptValue::numeric(name.ends_with("up")?player_pitch_>=.2617993950843811f:player_pitch_<=-.2617993950843811f,5);
  }
  if(name.starts_with("player_action_test_")) {
    arity(0);const auto action=name.substr(19);
    static const std::map<std::string,std::uint32_t> masks{
      {"jump",2},{"primary_trigger",16},{"grenade_trigger",32},{"vision_trigger",1u<<20},{"zoom",512},
      {"rotate_weapons",128},{"rotate_grenades",256},{"melee",64},{"action",1},{"accept",4},{"cancel",8},
      {"look_relative_up",1u<<10},{"look_relative_down",1u<<11},{"look_relative_left",1u<<12},{"look_relative_right",1u<<13},
      {"look_relative_all_directions",0x3c00},{"move_relative_all_directions",0x3c000},{"start",1u<<18},{"back",1u<<19},
      {"lookstick_forward",1u<<23},{"lookstick_backward",1u<<24}};
    const auto at=masks.find(action);if(at==masks.end())throw FormatError("unsupported player action test "+action);
    if(action=="accept" || action=="cancel" || action=="action"){training_.armed|=at->second;training_.suppressed|=at->second;}
    return ScriptValue::numeric((training_.actions&at->second)!=0,5);
  }
  if(name=="game_is_playtest") {arity(0);return ScriptValue::numeric(false,5);}
  if (name == "game_is_cooperative") {
    arity(0); return ScriptValue::numeric(false,5); // Actual native solo mode.
  }
  if (name == "game_can_use_flashlights") {
    arity(1);
    if (flashlights_allowed_ != args[0].truth()) {
      flashlights_allowed_=args[0].truth(); flashlights_changed_tick_=ticks_;
    }
    return {};
  }
  if (name == "objectives_clear") {
    arity(0); objectives_.fill(0); return {};
  }
  if (name == "objectives_show_up_to" || name == "objectives_finish_up_to") {
    arity(1);
    if (args[0].type!=8) throw FormatError("invalid objective index type");
    const int last=std::int32_t(args[0].bits);
    if (last >= 0 && last < int(objectives_.size()))
      for (int i=0;i<=last;++i)
        if (name == "objectives_finish_up_to" || objectives_[i] == 0)
          objectives_[i]=name == "objectives_finish_up_to" ? 2 : 1;
    return {};
  }
  if (name == "switch_bsp" || name == "switch_bsp_by_name") {
    arity(1);
    if (args[0].type != (name == "switch_bsp" ? 7u : 26u))
      throw FormatError("invalid compiled BSP argument type");
    request_bsp(std::int16_t(args[0].bits));
    return {};
  }
  if (name == "structure_bsp_index") {
    arity(0); return ScriptValue::numeric(bsp_,7);
  }
  if (name == "cinematic_skip_start_internal" ||
      name == "cinematic_skip_stop_internal") {
    arity(0);
    cinematic_skip_enabled_ = name == "cinematic_skip_start_internal";
    if(!cinematic_skip_enabled_ && cinematic_state())mutable_cinematic_state().awaiting_camera=false;
    return {};
  }
  if(name=="game_safe_to_save") {
    arity(0);
    // Native checkpoint admission: the complete owned state is copied at
    // the tick boundary. Retail226190's Havok/animation/effect danger checks
    // are a separate original-world service, not fabricated by this backend.
    return ScriptValue::numeric(player_.exists && !player_.vitality.dead && !revert_pending_ && !pending_bsp_,5);
  }
  if(name=="game_save" || name=="game_save_unsafe") {arity(0);request_checkpoint();return {};}
  if(name=="game_save_cancel") {arity(0);save_pending_=false;return {};}
  if(name=="game_won" || name=="map_reset") {
    arity(0);if(name=="game_won")level_finished_=true;else reset_requested_=true;
    scripts_->suspend_tick();return {};
  }
  if (name == "game_save_cinematic_skip") {
    arity(0);
    if(!presentation_.cinematic && !presentation_.scripted)mutable_cinematic_state().awaiting_camera=true;
    request_checkpoint();
    return {};
  }
  if (name == "game_saving") {
    arity(0);
    return ScriptValue::numeric(save_pending_, 5);
  }
  if (name == "game_reverted") {
    arity(0);
    return ScriptValue::numeric(reverted_tick_ && *reverted_tick_ == ticks_, 5);
  }
  if (name == "game_revert") {
    arity(0);
    request_revert();
    return {};
  }
  if (name == "texture_cache_flush" || name == "geometry_cache_flush") {
    arity(0);
    // Retail builtins 0x213/0x214 have null evaluators. These do not request
    // native residency changes or stand in for unimplemented prediction.
    return {};
  }
  if (name == "time_code_reset") {
    // Retail 2AB130 / TU1.5 2AC980 only complete the HS call with void.
    // The release evaluator has no timer, game-clock or camera side effect.
    arity(0);
    return {};
  }
  if (name == "data_mine_set_mission_segment") {
    arity(1);
    if (args[0].type != 9)
      throw FormatError("mission segment needs a string");
    mission_segment_ = args[0].text;
    return {};
  }
  if (name == "player_camera_control") {
    arity(1);
    presentation_.player_camera_enabled = args[0].truth();
    return ScriptValue::numeric(args[0].truth(), 5);
  }
  if (name == "camera_control") {
    arity(1);
    if(source_campaign_)source_campaign_camera_control_tu15(
        source_campaign_->objects().tu15_world(),args[0].truth());
    presentation_.control(args[0].truth(), {eye(player_), player_.yaw, 0});
    ++cinematic_camera_discontinuity_;
    if(args[0].truth() && cinematic_state())mutable_cinematic_state().awaiting_camera=false;
    return {};
  }
  if (name == "cinematic_start" || name == "cinematic_stop") {
    arity(0);
    if(source_campaign_)source_campaign_cinematic_control_tu15(
        source_campaign_->objects().tu15_world(),name=="cinematic_start");
    presentation_.cinematic_control(name == "cinematic_start");
    ++cinematic_camera_discontinuity_;
    if (name == "cinematic_start") {
      ambience_details_enabled_=true;
      if(cinematic_state())mutable_cinematic_state().awaiting_camera=false;
    }
    return {};
  }
  if(name=="cinematic_outro_start") {
    // Retail2AC3E0 writes globals+22 independently of cinematic start/stop.
    arity(0);presentation_.cinematic_outro=true;return {};
  }
  if (name == "cinematic_show_letterbox" ||
      name == "cinematic_show_letterbox_immediate") {
    arity(1);
    presentation_.letterbox(args[0].truth(), name.ends_with("immediate"));
    return {};
  }
  if (name == "fade_in" || name == "fade_out") {
    arity(4);
    presentation_.fade({float(args[0].number()), float(args[1].number()),
                        float(args[2].number())},
                       name == "fade_out", int(args[3].number()));
    return {};
  }
  if (name == "hud_cinematic_fade") {
    arity(2);
    presentation_.hud(float(args[0].number()),
                      int(std::nearbyint(args[1].number() * 30)));
    return {};
  }
  if (name == "camera_set_field_of_view") {
    arity(2);
    if(source_campaign_)source_campaign_camera_field_of_view_tu15(
        source_campaign_->objects().tu15_world(),float(args[0].number()),std::int16_t(args[1].bits));
    presentation_.field_of_view(float(args[0].number()), int(args[1].number()));
    ++cinematic_camera_discontinuity_;
    return {};
  }
  if (name == "camera_set") {
    arity(2);
    presentation_.camera_point(std::int16_t(args[0].bits),
                               int(args[1].number()));
    if(source_campaign_)source_campaign_camera_point_tu15(
        source_campaign_->objects().tu15_world(),std::int16_t(args[0].bits),std::int16_t(args[1].bits));
    ++cinematic_camera_discontinuity_;
    return {};
  }
  if (name == "camera_set_animation_relative" ||
      name == "camera_set_animation") {
    arity(name == "camera_set_animation_relative" ? 4 : 2);
    if (args.size() == 4 && args[2].bits != null_datum)
      throw FormatError("object-relative animated camera unsupported");
    if(source_campaign_)source_campaign_camera_animation_tu15(
        source_campaign_->objects().tu15_world(),args[0].bits,args[1].bits,
        args.size()==4?args[2].bits:null_datum,args.size()==4?std::int16_t(args[3].bits):-1);
    else presentation_.camera_animation(args[0].bits, args[1].bits,
                                   args.size() == 4 ? std::int16_t(args[3].bits)
                                                    : -1);
    ++cinematic_camera_discontinuity_;
    return {};
  }
  if (name == "camera_time") {
    arity(0);
    if(source_campaign_)return ScriptValue::numeric(source_campaign_camera_time_tu15(
        source_campaign_->objects().tu15_world()),7);
    return ScriptValue::numeric(presentation_.camera_time(), 7);
  }
  if (name == "object_create" || name == "object_create_anew") {
    arity(1);
    if (args[0].type < 56 || args[0].type > 61)
      throw FormatError("object creation needs a scenario name");
    scene_.create(std::int16_t(args[0].bits), name == "object_create_anew", ticks_);
    return {};
  }
  if(name=="object_create_containing" || name=="object_create_anew_containing" || name=="object_destroy_containing") {
    arity(1);if(args[0].type!=9)throw FormatError("Object name search requires a string");
    // Retail29FF60 visits every case-sensitive substring match in scenario
    // name order;29FD40/29FD80 delegate ordinary creation/replacement to the
    // same named factory used by object_create/object_create_anew.
    for(unsigned i=0;i<content_.object_names.size();++i)if(content_.object_names[i].name.find(args[0].text)!=std::string::npos) {
      if(name=="object_destroy_containing") {const auto handle=scene_.resolve(i);if(handle!=null_datum)scene_.destroy(handle);}
      else scene_.create(i,name=="object_create_anew_containing",ticks_);
    }
    return {};
  }
  if (name == "object_destroy") {
    arity(1);
    if(args[0].bits==null_datum)return {};
    const auto actor=std::find_if(actors_.begin(),actors_.end(),[&](const auto &a){return a.exists && a.source_datum==args[0].bits;});
    if(actor!=actors_.end()){actor->exists=false;scripts_->stop_command(actor->source_datum);object_lifetimes_.release(actor->source_datum);}
    else scene_.destroy(args[0].bits);
    return {};
  }
  if (name == "object_destroy_all" || name == "object_destroy_type_mask") {
    arity(name == "object_destroy_all" ? 0 : 1);
    const auto mask=args.empty() || !args[0].bits ? null_datum : args[0].bits;
    scene_.destroy_type_mask(mask);
    if (mask & 1u)
      for (auto &actor:actors_)if(actor.exists) {actor.exists=false;scripts_->stop_command(actor.source_datum);object_lifetimes_.release(actor.source_datum);}
    if (mask & (1u<<5)) {
      for(const auto &projectile:projectiles_)
        if(release_object_ && projectile.source_datum!=null_datum)
          release_object_(projectile.source_datum);
      projectiles_.clear();
    }
    // The separately owned player and equipped children are protected, as
    // in original 29FE10's BEB30 predicate. General object parenting is pending.
    return {};
  }
  if (name == "object_hide") {
    arity(2);
    if (args[0].bits == 0)
      player_.visible = !args[1].truth();
    else if (args[0].bits != null_datum) {
      const auto actor=std::find_if(actors_.begin(),actors_.end(),[&](const auto &a){return a.exists && a.source_datum==args[0].bits;});
      if(actor!=actors_.end())actor->visible=!args[1].truth();else scene_.visible(args[0].bits,!args[1].truth());
    }
    return {};
  }
  if (name == "object_teleport") {
    arity(2);
    const auto flag = std::int16_t(args[1].bits);
    if (flag < 0 || std::size_t(flag) >= content_.cutscene_flags.size())
      throw FormatError("invalid teleport flag");
    const auto &f = content_.cutscene_flags[flag];
    if(source_campaign_ && args[0].bits!=null_datum)
      source_campaign_->objects().tu15_world().set_object_position(args[0].bits==0?player_.source_datum:args[0].bits,f.position);
    if (args[0].bits == 0) {
      player_.body.position = f.position;
      player_.body.velocity = {};
      player_.yaw = f.yaw;
    } else if (args[0].bits != null_datum) {
      const auto actor=std::find_if(actors_.begin(),actors_.end(),[&](const auto &a){return a.exists && a.source_datum==args[0].bits;});
      if(actor!=actors_.end()) {
        actor->body.position=f.position;actor->body.velocity={};
        actor->yaw=f.yaw;actor->pitch=f.pitch;actor->route.clear();actor->waypoint=0;
      } else {
        auto &o = scene_.get(args[0].bits);
        o.position = f.position;
        o.rotation = {f.yaw, f.pitch, 0};
      }
    }
    return {};
  }
  if(name=="volume_return_objects") {
    arity(1);const auto volume=std::int16_t(args[0].bits);
    if(volume<0 || std::size_t(volume)>=content_.trigger_volumes.size())throw FormatError("invalid trigger volume");
    const auto &v=content_.trigger_volumes[volume];
    const auto list=source_hs_volume_objects({
      [this]{if(multiplayer_script_lists_){auto roots=scripts_->object_list_roots();std::erase(roots,0x80000000u);multiplayer_script_lists_->roots(roots);return multiplayer_script_lists_->allocate();}const auto id=next_script_list_++;script_lists_.emplace(id,std::vector<std::uint32_t>{});return id;},
      [this](auto list,auto object){if(multiplayer_script_lists_)multiplayer_script_lists_->append(list,object);else script_lists_.at(list).insert(script_lists_.at(list).begin(),object);}},
      {[&]{if(v.object_name>=0)throw FormatError("Attached volume has no spatial transform owner");return true;},[&]{
        //Native spatial adapter uses BB050's sphere overlap and the authored
        //query radius. Original cluster-registration order is still a separate
        //owner boundary. Never silently truncate an overflowing native bank.
        std::vector<std::uint32_t> objects;
        const auto center=v.position+(v.forward*v.extents.x+cross(v.up,v.forward)*v.extents.y+v.up*v.extents.z)*.5f;
        const auto overlaps=[&](auto position,float radius){const auto d=position-center;const auto r=radius+v.query_radius;return (d.z*d.z+d.y*d.y)+d.x*d.x<=r*r;};
        for(const auto &o:scene_.objects())if(o.handle!=null_datum && !o.bsp_hidden &&
            overlaps(scene_.center(o.handle),content_.objects[o.placement].bounding_radius*o.scale))objects.push_back(o.handle);
        if(player_.exists && overlaps(object_center(0),player_.definition.bounding_radius))objects.push_back(0);
        for(const auto &actor:actors_)if(actor.exists && overlaps(object_center(actor.source_datum),actor.definition.bounding_radius))objects.push_back(actor.source_datum);
        return objects;
      },[&](auto object){return scene_.contains(v,object_center(object));}});
    return {31,list,{}};
  }
  if(source_campaign_ && (name=="kill_volume_enable" || name=="kill_volume_disable")) {
    arity(1);
    if(args[0].type!=13)throw FormatError("Invalid kill-volume trigger reference");
    const auto trigger=std::bit_cast<std::int16_t>(std::uint16_t(args[0].bits));
    source_campaign_->set_kill_volume_enabled(trigger,name=="kill_volume_enable");
    return {};
  }
  if (name == "volume_test_object" || name == "volume_test_objects" ||
      name == "volume_test_objects_all") {
    arity(2);
    const auto volume = std::int16_t(args[0].bits);
    if (volume < 0 || std::size_t(volume) >= content_.trigger_volumes.size())
      throw FormatError("invalid trigger volume");
    const auto &v = content_.trigger_volumes[volume];
    if(name=="volume_test_object")return ScriptValue::numeric(args[1].bits!=null_datum && scene_.contains(v,object_center(args[1].bits)),5);
    const auto objects=script_objects(args[1].bits);bool result=name=="volume_test_objects_all";
    for(const auto handle:objects) {
      const bool inside=scene_.contains(v,object_center(handle));
      if(name=="volume_test_objects_all")result&=inside;else result|=inside;
    }
    return ScriptValue::numeric(result,5);
  }
  if (name == "structure_bsp_index") {
    arity(0);
    return ScriptValue::numeric(bsp_, 7);
  }
  if (name == "player_enable_input") {
    if (args.size() != 1 || args[0].type != 5)
      throw FormatError("invalid player_enable_input");
    player_input_enabled_ = args[0].truth();
    return {};
  }
  if (name == "player_disable_movement") {
    arity(1);
    if (args[0].type != 5)
      throw FormatError("invalid player_disable_movement");
    player_movement_disabled_ = args[0].truth();
    return {};
  }
  if (name == "unit_impervious") {
    arity(2);
    if (args[0].type != 31 || args[1].type != 5)
      throw FormatError("invalid unit_impervious");
    for(const auto handle:script_objects(args[0].bits))script_unit(handle).impervious=args[1].truth();
    return {};
  }
  if (name == "object_cannot_take_damage" || name == "object_can_take_damage") {
    arity(1);
    if (args[0].type != 31)
      throw FormatError("damage protection needs an object list");
    for(const auto handle:script_objects(args[0].bits))script_unit(handle).cannot_take_damage=name=="object_cannot_take_damage";
    return {};
  }
  if (name == "unit_get_health" || name == "unit_get_shield") {
    if (args.size() != 1 || args[0].type != 51)
      throw FormatError("invalid unit vitality query");
    if(args[0].bits==null_datum)return ScriptValue::numeric(-1);
    const auto &unit=script_unit(args[0].bits);
    return ScriptValue::numeric(name == "unit_get_health"
                                    ? unit.vitality.body
                                    : unit.vitality.shield);
  }
  if (name == "ai_allegiance" || name == "ai_allegiance_remove") {
    if (args.size() != 2)
      throw FormatError("invalid allegiance arity");
    const int a = int(args[0].number()), b = int(args[1].number());
    if (a < 0 || b < 0 || a >= 16 || b >= 16)
      throw FormatError("team outside allegiance table");
    set_team_allegiance(a,b,name == "ai_allegiance");
    return {};
  }
  if (name == "ai_place" && source_campaign_) {
    // TU1.5 definitions 290/291: void(AI) and void(AI, SHORT). Preserve
    // NONE, packed references and nonpositive counts for the original owner.
    if((args.size()!=1 && args.size()!=2) || args[0].type!=19 ||
        (args.size()==2 && args[1].type!=7))
      throw FormatError("invalid source ai_place overload");
    std::optional<std::int16_t> count;
    if(args.size()==2)count=std::bit_cast<std::int16_t>(std::uint16_t(args[1].bits));
    source_campaign_->place_ai(*this,args[0].bits,count);
    return {};
  }
  if (name == "ai_place" || name == "ai_erase") {
    if (args.size() != 1 || args[0].type != 19)
      throw FormatError("unsupported " + name + " overload");
    const auto id = args[0].bits;
    if (id >> 30)
      throw FormatError("only squad AI literals are supported");
    const auto index = std::size_t(id & 0xffff);
    if (index >= content_.squads.size())
      throw FormatError("AI literal squad index outside table");
    if (name == "ai_place")
      place_squad(content_.squads[index].name);
    else {
      for (auto &a : actors_)
        if (a.squad == index) {
          a.exists=false;scripts_->stop_command(a.source_datum);object_lifetimes_.release(a.source_datum);
        }
    }
    return {};
  }
  if (name == "sound_impulse_start") {
    if (args.size() != 3 || args[0].type != 32)
      throw FormatError("invalid sound_impulse_start");
    const auto object=args[1].bits;
    const auto position=object==null_datum?Vec3{}:
        source_campaign_?source_scripted_sound_point_tu15(source_campaign_->objects().tu15_world(),
            object==0?player_.source_datum:object):
        object==0?eye(player_):object_center(object);
    sounds_.push_back({args[0].bits,position,float(args[2].number()),
        object==null_datum?SoundEvent::NonSpatialImpulse:SoundEvent::Impulse,false,
        object==0?player_.source_datum:object});
    if(sound_duration_) {
      auto at=std::find_if(impulse_clocks_.begin(),impulse_clocks_.end(),[&](const auto &clock){return clock.datum==args[0].bits || clock.datum==null_datum || clock.start+clock.duration<=ticks_;});
      if(at!=impulse_clocks_.end())*at={args[0].bits,ticks_,std::uint64_t(std::max(0.f,std::nearbyint(sound_duration_(args[0].bits)*30.f)))};
    }
    source_admit_queued_script_sounds_tu15(*this,sounds_);
    return {};
  }
  if (name == "sound_looping_start") {
    if (args.size() != 3 || args[0].type != 35)
      throw FormatError("invalid sound_looping_start");
    const auto position = args[1].bits == null_datum || args[1].bits == 0
                              ? eye(player_)
                              : object_center(args[1].bits);
    const SoundEvent event{args[0].bits, position, float(args[2].number()),
                           SoundEvent::LoopStart};
    sounds_.push_back(event);
    active_loops_[event.datum] = event;
    source_admit_queued_script_sounds_tu15(*this,sounds_);
    return {};
  }
  if (name == "sound_looping_stop" || name == "sound_looping_set_alternate" ||
      name == "sound_impulse_stop") {
    const bool alternate = name == "sound_looping_set_alternate";
    if (args.size() != (alternate ? 2u : 1u))
      throw FormatError("invalid sound stop/alternate arity");
    sounds_.push_back({args[0].bits,
                       {},
                       1,
                       name == "sound_impulse_stop" ? SoundEvent::ImpulseStop
                       : alternate                  ? SoundEvent::LoopAlternate
                                                    : SoundEvent::LoopStop,
                       alternate && args[1].truth()});
    if(name=="sound_impulse_stop")for(auto &clock:impulse_clocks_)if(clock.datum==args[0].bits)clock={};
    if (name == "sound_looping_stop")
      active_loops_.erase(args[0].bits);
    else if (alternate) {
      const auto loop = active_loops_.find(args[0].bits);
      if (loop != active_loops_.end())
        loop->second.alternate = args[1].truth();
    }
    source_admit_queued_script_sounds_tu15(*this,sounds_);
    return {};
  }
  if (name == "print") {
    if (args.size() != 1 || args[0].type != 9)
      throw FormatError("invalid print");
    std::cout << "Script: " << args[0].text << '\n';
    return {};
  }
  if (name == "random_range") {
    arity(2);
    if (args[0].type != 7 || args[1].type != 7)
      throw FormatError("random_range requires source short arguments");
    return {7, source_hs_random_range(random_state_,
                                    std::int16_t(args[0].bits),
                                    std::int16_t(args[1].bits)), {}};
  }
  if (name == "weather_start" || name == "weather_stop" ||
      name == "weather_change_intensity") {
    arity(name == "weather_change_intensity" ? 2 : 1);
    for(const auto &argument:args)
      if(argument.type!=6)throw FormatError("weather commands require source real arguments");
    const auto seconds=float(args[0].number());
    if(name=="weather_start")source_weather_start(weather_,seconds,&weather_record_);
    else if(name=="weather_stop")source_weather_stop(weather_,seconds,&weather_record_);
    else source_weather_change(weather_,seconds,float(args[1].number()),&weather_record_);
    return {};
  }
  if (name == "game_difficulty_get" || name == "game_difficulty_get_real") {
    if (!args.empty())
      throw FormatError("invalid difficulty arity");
    return {44, difficulty_, {}};
  }
  if (name == "players") {
    if (!args.empty())
      throw FormatError("invalid players arity");
    return {31, 0x80000000u, {}};
  }
  if (name == "list_count") {
    arity(1);return ScriptValue::numeric(multiplayer_script_lists_ && args[0].bits!=0x80000000u?
        multiplayer_script_lists_->count(args[0].bits):script_objects(args[0].bits).size(),7);
  }
  if (name == "list_get") {
    arity(2);if(multiplayer_script_lists_ && args[0].bits!=0x80000000u)
      return {50,multiplayer_script_lists_->get(args[0].bits,std::int16_t(args[1].bits)),{}};
    const auto objects=script_objects(args[0].bits);const int index=int(args[1].number());
    return {50,index>=0 && std::size_t(index)<objects.size()?objects[index]:null_datum,{}};
  }
  if (name == "unit") {
    if (args.size() != 1)
      throw FormatError("invalid unit cast");
    return {51, args[0].bits, {}};
  }
  throw FormatError("unsupported builtin '" + name + "'");
}
std::vector<SoundEvent> Simulation::take_sounds() {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  auto result = std::move(sounds_);
  sounds_.clear();
  return result;
}
std::vector<Vertex>
Simulation::debug_mesh(std::span<const std::uint32_t> rendered_models,
    std::span<const Unit> previous_actors,std::span<const NativeWeaponProjectile> previous_projectiles,float fraction) const {
  validate_render_fraction(fraction);
  std::vector<Vertex> vertices;
  vertices.reserve(actors_.size() * 36 + projectiles_.size() * 36);
  for (const auto &a : actors_)
    if (a.exists && !a.vitality.dead && !a.bsp_hidden &&
        std::find(rendered_models.begin(), rendered_models.end(),
                  a.definition.datum) == rendered_models.end()) {
      const auto r = a.body.radius;
      auto position=a.body.position;const auto index=std::size_t(&a-actors_.data());
      if(index<previous_actors.size()) {
        const auto &old=previous_actors[index];
        if(old.exists && !old.vitality.dead && !old.bsp_hidden && old.visible &&
           old.source_datum==a.source_datum && old.definition.datum==a.definition.datum && old.bsp==a.bsp)
          position=render_lerp(old.body.position,position,fraction);
      }
      cube(vertices, position - Vec3{r, r, 0},
           position + Vec3{r, r, a.body.height},
           a.team == 1 || a.team == 2 ? Vec3{.1f, .7f, .2f}
           : a.state == Unit::Attack  ? Vec3{1, .15f, .1f}
                                      : Vec3{.9f, .6f, .1f});
    }
  for (const auto &p : projectiles_) {
    auto position=p.position;
    const auto old=std::find_if(previous_projectiles.begin(),previous_projectiles.end(),[&](const auto &o){
      return p.source_datum!=null_datum && o.datum==p.source_datum && o.definition==(p.barrel_index<0?content_.weapons.at(p.weapon).projectile:content_.weapons.at(p.weapon).barrel_views.at(unsigned(p.barrel_index))->projectile);
    });
    if(old!=previous_projectiles.end())position=render_lerp(old->position,position,fraction);
    cube(vertices, position - Vec3{.025f, .025f, .025f},
         position + Vec3{.025f, .025f, .025f}, {1, .8f, .1f});
  }
  return vertices;
}
std::vector<NativeWeaponProjectile> Simulation::presentation_projectiles() const {
  std::vector<NativeWeaponProjectile> out;out.reserve(projectiles_.size());
  for(const auto &p:projectiles_)out.push_back({p.source_datum,p.barrel_index<0?content_.weapons.at(p.weapon).projectile:content_.weapons.at(p.weapon).barrel_views.at(unsigned(p.barrel_index))->projectile,
      p.position,p.velocity,p.forward,p.up,p.traveled});
  return out;
}
std::string Simulation::status() const {
  std::ostringstream out;
  out << "tick=" << ticks_ << " bsp=" << bsp_ << " position=" << std::fixed << std::setprecision(3)
      << player_.body.position.x << "," << player_.body.position.y << ","
      << player_.body.position.z << " grounded=" << player_.body.grounded
      << " health=" << player_.vitality.body
      << " shield=" << player_.vitality.shield
      << " ammo=" << player_.weapon.loaded << "/" << player_.weapon.reserve
      << " actors=" << std::count_if(actors_.begin(),actors_.end(),[](const auto &a){return a.exists;}) << " shots=" << shots_
      << " hits=" << hits_ << " scripts=" << scripts_->running()
      << " scene_objects="
      << std::count_if(scene_.objects().begin(), scene_.objects().end(),
                       [](const auto &o) { return o.handle != null_datum; })
      << " camera_time=" << (source_campaign_?source_campaign_camera_time_tu15(
          source_campaign_->objects().tu15_world()):presentation_.camera_time())
      << " script_faults=" << scripts_->diagnostics().size();
  out << " checkpoint=" << bool(checkpoint_)
      << " reverts=" << checkpoint_restores_;
  if(const auto *state=cinematic_state();state && state->screen_active)
    out << " cinematic_clock=" << state->render_clock
        << " crossfade_pending=" << state->crossfade.pending
        << " crossfade_capture=" << state->crossfade.capture
        << " crossfade_alpha=" << state->crossfade.alpha;
  return out.str();
}
} // namespace halo2
