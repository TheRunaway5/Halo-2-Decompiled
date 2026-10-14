#include "halo2/scene.hpp"
#include "halo2/object_initialization.hpp"
#include "halo2/source_object_render_flags.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>
#include <numeric>
namespace halo2 {
float camera_vertical_field_of_view(const Camera &camera, float aspect) {
  if (!std::isfinite(aspect) || aspect <= 0)
    throw FormatError("invalid camera aspect ratio");
  if (camera.projection_vertical_angle)
    return *camera.projection_vertical_angle;
  const float radians = camera.field_of_view * .017453292f;
  return camera.horizontal_field_of_view
             ? float(2.0 *
                     std::atan2(std::tan(double(radians) * .5) / aspect, 1.0))
             : radians;
}
namespace {
std::vector<std::size_t> ordered_placements(std::span<const ScenarioObject> objects) {
  std::vector<std::size_t> order(objects.size());
  std::iota(order.begin(), order.end(), std::size_t{});
  std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) {
    return std::pair{objects[a].type, objects[a].placement} <
           std::pair{objects[b].type, objects[b].placement};
  });
  return order;
}
int placement_policy(const ScenarioObject &p) {
  if (p.bsp_policy == 1) return 0;
  if (p.bsp_policy == 2) return 2;
  if (p.source == 0) return 1;
  if (p.source == 1)
    return p.origin_bsp == -1 || ((1u << p.type) & 0x181f) ? 0 : 1;
  return -1;
}
bool persistent(const ScenarioObject &p) {
  return p.name >= 0 || p.damageable || ((1u << p.type) & 0x199f);
}
bool globally_created(const ScenarioObject &p) {
  return persistent(p) || placement_policy(p) != 1;
}
}
bool SceneObjects::factory_game_allowed(const ScenarioObject &object) const {
  return game_context_ ? source_scenario_factory_game_allowed(object,
      game_context_->game_type,game_context_->engine) : !(object.flags&64u);
}
SceneObjects::SceneObjects(const LevelContent &content, std::size_t bsp,
                           std::optional<SourceSceneGameContext> context,
                           RenderAttributionDefinitionResolver resolver,
                           NativeObjectLifetimes *shared_lifetimes,
                           SourceSceneObjectOwner source_owner)
    : content_(content), bsp_(bsp), game_context_(context),
      render_attribution_definition_(std::move(resolver)),
      source_owner_(std::move(source_owner)),
      shared_lifetimes_(shared_lifetimes),
      names_(content.object_names.size(), null_datum) {
  if(!shared_lifetimes_)local_lifetimes_.emplace();
  if(bool(source_owner_.create)!=bool(source_owner_.destroy) ||
      (source_owner_.create && !shared_lifetimes_))
    throw FormatError("Source scene ownership requires paired birth/retirement and its shared pool");
  // D5560 attempts types0..12, then each authored placement ordinal.
  // Native factory admission remains separate from that ordering.
  // Difficulty bits 2/4/8 are not excluded by this placement path.
  for (const auto i : ordered_placements(content.objects)) {
    const auto &p=content.objects[i];
    if (p.definition != null_datum && !(p.flags & 1) &&
        factory_game_allowed(p) && globally_created(p))
      place(i);
  }
  switch_bsp(bsp);
}
bool SceneObjects::retains(int previous, int next, Vec3 point) const {
  if (next < 0) return false;
  if (previous == next || previous < 0) return true;
  const auto key = (std::uint32_t(std::max(previous,next)) << 16) |
                   std::uint16_t(std::min(previous,next));
  bool listed=false;
  for (const auto &entry:content_.bsp_transitions)
    if (entry.pair == key && entry.volume >= 0) {
      listed=true;
      if (contains(content_.trigger_volumes.at(entry.volume),point)) return true;
    }
  return !listed;
}
void SceneObjects::switch_bsp(std::size_t next, bool suppress_automatic) {
  for (auto &o:objects_) {
    if (o.handle == null_datum) continue;
    const auto &p=content_.objects[o.placement];
    if(o.inventory_attached){o.bsp=int(next);continue;}
    if (p.source == -1) continue;
    const auto policy=placement_policy(p);
    const bool keep=policy == 0 ? retains(o.bsp,int(next),center(o.handle)) :
        policy == 1 ? p.origin_bsp == int(next) :
        policy == 2 ? next < 16 && (p.manual_bsps & (1u << next)) : false;
    if (keep) {
      if (o.bsp_hidden) { o.visible=!o.script_hidden; o.bsp_hidden=false; }
      o.bsp=int(next);
    } else if (!o.bsp_hidden) {
      if (persistent(p)) { o.visible=false; o.bsp_hidden=true; }
      else destroy(o.handle);
    }
  }
  bsp_=next;
  if (suppress_automatic) return;
  // Retail D5640: named/damageable entries retain their runtime state; only
  // missing automatic scenery is constructed for the newly active BSP.
  for (const auto i : ordered_placements(content_.objects)) {
    const auto &p=content_.objects[i];
    if (p.definition == null_datum || (p.flags & 1) ||
        !factory_game_allowed(p) || persistent(p)) continue;
    if (std::any_of(objects_.begin(),objects_.end(),[&](const auto &o) {
          return o.handle != null_datum && o.placement == i; })) continue;
    const bool allowed=p.bsp_policy == 2 ? next < 16 && (p.manual_bsps & (1u<<next)) :
        p.origin_bsp == -1 || p.origin_bsp == int(next) || globally_created(p);
    if (allowed) place(i);
  }
}
void SceneObjects::visible(std::uint32_t handle,bool visible) {
  if (handle==null_datum) return;
  auto &o=get(handle); o.script_hidden=!visible; o.visible=visible && !o.bsp_hidden;
}
void SceneObjects::restore(const SceneObjects &saved) {
  if (&content_ != &saved.content_)
    throw FormatError("scene checkpoint belongs to another level");
  if(shared_lifetimes_!=saved.shared_lifetimes_)
    throw FormatError("scene checkpoint belongs to another shared identity owner");
  bsp_ = saved.bsp_;
  game_context_ = saved.game_context_;
  render_attribution_definition_ = saved.render_attribution_definition_;
  source_owner_ = saved.source_owner_;
  functions_ = saved.functions_;
  objects_ = saved.objects_;
  local_lifetimes_ = saved.local_lifetimes_;
  slots_ = saved.slots_;
  names_ = saved.names_;
}
NativeObjectLifetimes &SceneObjects::lifetime_owner() {
  return shared_lifetimes_?*shared_lifetimes_:*local_lifetimes_;
}
const NativeObjectLifetimes &SceneObjects::object_lifetimes() const {
  return shared_lifetimes_?*shared_lifetimes_:*local_lifetimes_;
}
std::uint32_t SceneObjects::place(std::size_t placement,
                                  std::uint64_t creation_tick) {
  if (placement >= content_.objects.size())
    throw FormatError("object placement outside scenario");
  const auto &p = content_.objects[placement];
  if (p.definition == null_datum)
    throw FormatError("object placement has no definition");
  if (p.name >= 0) {
    if (std::size_t(p.name) >= names_.size())
      throw FormatError("object name outside scenario");
    if (names_[p.name] != null_datum)
      throw FormatError("duplicate live object name");
  }
  std::size_t slot = 0;
  while (slot < objects_.size() && objects_[slot].handle != null_datum)
    ++slot;
  if (slot == objects_.size() && slot >= 2048)
    throw FormatError("scene object capacity exhausted");
  std::optional<bool> render_attribution_at_birth;
  if (render_attribution_definition_) {
    const auto definition = render_attribution_definition_(p.definition);
    if (definition && definition->datum != p.definition)
      throw FormatError("scene render attribution definition identity mismatch");
    // Only the construction predicate survives. Scratch zero is not exposed
    // as complete native body flags or a later/current object flag31 word.
    render_attribution_at_birth = source_object_render_flag31_construction_event(
        0, definition).model_attribution_available;
  }
  if(p.type<0 || p.type>12)throw FormatError("scene object has unsupported original type");
  const auto handle=source_owner_.create ? source_owner_.create(p) :
      lifetime_owner().reserve(std::uint8_t(p.type));
  if(handle==null_datum)throw FormatError(source_owner_.create ?
      "original source rejected scene placement type="+std::to_string(p.type)+
      " ordinal="+std::to_string(p.placement)+" definition="+std::to_string(p.definition)+
      " netgame_row="+std::to_string(p.netgame_row) : "shared native object capacity exhausted");
  if(slots_.contains(handle))throw FormatError("source object generation already belongs to another scene placement");
  if(source_owner_.create)lifetime_owner().observe_source_birth(handle);
  if (slot == objects_.size())objects_.push_back({});
  objects_[slot] = {handle, placement, p.position, p.rotation, p.scale, true, false, false, false, int(bsp_)};
  objects_[slot].created_tick = creation_tick;
  objects_[slot].render_attribution_at_birth = render_attribution_at_birth;
  slots_.emplace(handle,slot);
  if (p.name >= 0) {
    names_[p.name] = handle;
  }
  if(!source_owner_.create)lifetime_owner().publish_native(handle,p.definition);
  return handle;
}
std::uint32_t SceneObjects::resolve(int name) const {
  if (name == -1)
    return null_datum;
  if (name < 0 || std::size_t(name) >= names_.size())
    throw FormatError("object name index outside scenario");
  return names_[name];
}
std::uint32_t SceneObjects::create(int name, bool replace,
                                   std::uint64_t creation_tick) {
  const auto existing = resolve(name);
  if (name == -1)
    return null_datum;
  if (existing != null_datum) {
    if (!replace)
      return existing;
    destroy(existing);
  }
  const auto &n = content_.object_names[name];
  // RetailBB670 admits only names bound to a placement (+22 != NONE).
  // Compiled name searches also visit unbound scenario name rows.
  if(n.placement==-1)return null_datum;
  for (std::size_t i = 0; i < content_.objects.size(); ++i)
    if (content_.objects[i].type == n.type &&
        content_.objects[i].placement == n.placement) {
      if(game_context_ && !factory_game_allowed(content_.objects[i]))
        return null_datum;
      return place(i, creation_tick);
    }
  throw FormatError("named object has no supported placement: " + n.name);
}
SceneObjects::Object &SceneObjects::get(std::uint32_t handle) {
  return const_cast<Object &>(std::as_const(*this).get(handle));
}
const SceneObjects::Object &SceneObjects::get(std::uint32_t handle) const {
  const auto index=slots_.find(handle);
  if (index==slots_.end() || !object_lifetimes().contains(handle) ||
      objects_[index->second].handle != handle)
    throw FormatError("stale/invalid scene object handle");
  return objects_[index->second];
}
void SceneObjects::destroy(std::uint32_t handle) {
  if (handle == null_datum)
    return;
  auto &o = get(handle);
  if(source_owner_.destroy) {
    source_owner_.destroy(handle);
    lifetime_owner().observe_source_retirement(handle);
  }else lifetime_owner().release(handle);
  slots_.erase(handle);
  const auto name = content_.objects[o.placement].name;
  if (name >= 0 && names_[name] == handle)
    names_[name] = null_datum;
  o.handle = null_datum;
  o.render_attribution_at_birth.reset();
  std::erase_if(functions_,[&](const auto &v){return v.first.first==handle;});
}
void SceneObjects::observe_source_retirement(std::uint32_t handle) {
  const auto slot=slots_.find(handle);
  if(!source_owner_.create || slot==slots_.end() ||
      objects_[slot->second].handle!=handle || !lifetime_owner().header(handle).empty())
    throw FormatError("Scene retirement observation requires an actually retired source generation");
  lifetime_owner().observe_source_retirement(handle);
  auto &o=objects_[slot->second];
  const auto name=content_.objects[o.placement].name;
  if(name>=0 && names_[name]==handle)names_[name]=null_datum;
  o.handle=null_datum;o.render_attribution_at_birth.reset();
  o.solved_transform.reset();o.solved_nodes.reset();
  slots_.erase(slot);
  std::erase_if(functions_,[&](const auto &v){return v.first.first==handle;});
}
std::optional<float> SceneObjects::function_variable(std::uint32_t object,
    std::uint32_t name,int tick) const {
  get(object);
  const auto it=functions_.find({object,name});
  if(it==functions_.end()) return {};
  const auto &v=it->second;
  const auto elapsed=tick-v.start;
  return v.duration && elapsed<v.duration
      ? float(elapsed)/float(v.duration)*(v.to-v.from)+v.from : v.to;
}
void SceneObjects::set_function_variable(std::uint32_t object,std::uint32_t name,
    float value,float frames,int tick) {
  if(object==null_datum || !name || name==null_datum) return;
  get(object);
  if(!std::isfinite(value) || !std::isfinite(frames) || frames>INT32_MAX)
    throw FormatError("invalid object shader function interpolation");
  const float initial=function_variable(object,name,tick).value_or(value);
  functions_[{object,name}]={initial,value,tick,int(std::nearbyint(std::max(frames,0.f)))};
}
Vec3 SceneObjects::center(std::uint32_t handle) const {
  const auto &o = get(handle);
  const auto offset = content_.objects[o.placement].bounding_offset;
  if(o.solved_transform) {
    const auto &t=*o.solved_transform;
    return t.position+(t.basis[0]*offset.x+t.basis[1]*offset.y+t.basis[2]*offset.z)*t.scale;
  }
  const glm::quat rotation(
      glm::vec3(o.rotation.z, -o.rotation.y, o.rotation.x));
  const auto p = rotation * glm::vec3(offset.x, offset.y, offset.z) * o.scale;
  return o.position + Vec3{p.x, p.y, p.z};
}
void SceneObjects::destroy_all() {
  for (auto &o : objects_)
    if (o.handle != null_datum)
      destroy(o.handle);
}
void SceneObjects::destroy_type_mask(std::uint32_t mask) {
  for (auto &o:objects_)
    if (o.handle!=null_datum && (mask & (1u<<content_.objects[o.placement].type)))
      destroy(o.handle);
}
bool SceneObjects::contains(const TriggerVolume &v, Vec3 center) const {
  if (v.object_name >= 0)
    throw FormatError("object-attached trigger transform unsupported");
  const auto delta = center - v.position;
  const Vec3 left = cross(v.up, v.forward);
  const float x = dot(delta, v.forward), y = dot(delta, left),
              z = dot(delta, v.up);
  return x > 0 && y > 0 && z > 0 && x < v.extents.x && y < v.extents.y &&
         z < v.extents.z;
}
float Presentation::Transition::value() const {
  return duration <= 0
             ? to
             : from + (to - from) *
                          std::clamp(float(elapsed) / duration, 0.f, 1.f);
}
void Presentation::Transition::set(float target, int ticks) {
  if (ticks < 0)
    throw FormatError("negative presentation duration");
  from = value();
  to = target;
  duration = ticks;
  elapsed = 0;
}
void Presentation::Transition::tick() {
  if (elapsed < duration)
    ++elapsed;
}
Presentation::Presentation(const LevelContent &content, AnimationLoader loader)
    : content_(content), loader_(std::move(loader)) {}
void Presentation::restore(const Presentation &saved) {
  if (&content_ != &saved.content_)
    throw FormatError("presentation checkpoint belongs to another level");
  cinematic = saved.cinematic;
  cinematic_outro = saved.cinematic_outro;
  scripted = saved.scripted;
  player_camera_enabled = saved.player_camera_enabled;
  fade_ = saved.fade_;
  letterbox_ = saved.letterbox_;
  hud_ = saved.hud_;
  fov_ = saved.fov_;
  fade_color_ = saved.fade_color_;
  camera_ = saved.camera_;
  camera_from_ = saved.camera_from_;
  camera_to_ = saved.camera_to_;
  observer_ = saved.observer_;
  camera_duration_ = saved.camera_duration_;
  camera_elapsed_ = saved.camera_elapsed_;
  animation_ = saved.animation_;
  anchor_ = saved.anchor_;
}
void Presentation::tick() {
  fade_.tick();
  letterbox_.tick();
  hud_.tick();
  fov_.tick();
  if (camera_elapsed_ < camera_duration_)
    ++camera_elapsed_;
  update_camera();
}
void Presentation::fade(Vec3 color, bool out, int ticks) {
  if (!std::isfinite(color.x) || !std::isfinite(color.y) ||
      !std::isfinite(color.z))
    throw FormatError("nonfinite fade color");
  fade_color_ = color;
  // Retail fade_in starts fully covered and reveals; fade_out starts clear.
  fade_ = {out ? 0.f : 1.f, out ? 0.f : 1.f};
  fade_.set(out ? 1.f : 0.f, ticks);
}
void Presentation::letterbox(bool show, bool immediate) {
  letterbox_.set(show ? 1.f : 0.f, immediate ? 0 : 30);
}
void Presentation::hud(float value, int ticks) {
  hud_.set(std::clamp(value, 0.f, 1.f), ticks);
}
void Presentation::field_of_view(float degrees, int ticks) {
  if (!std::isfinite(degrees) || degrees <= 1 || degrees >= 179)
    throw FormatError("invalid camera field of view");
  fov_.set(degrees, ticks);
}
void Presentation::cinematic_control(bool enabled) {
  cinematic = enabled;
  letterbox(enabled, false);
  if (!enabled)
    fov_.set(70, 0);
}
void Presentation::camera_point(int index, int ticks) {
  if (index < 0 || std::size_t(index) >= content_.camera_points.size() ||
      ticks < 0)
    throw FormatError("invalid cutscene camera point");
  const auto &p = content_.camera_points[index];
  if (p.flags & 1 || p.type != 0)
    throw FormatError("relative/dolly camera point unsupported");
  animation_.reset();
  camera_from_ = camera_;
  observer_.position = camera_.position;
  if (camera_.oriented) {
    observer_.forward = camera_.forward;
    observer_.up = camera_.up;
  } else {
    observer_.forward = direction(camera_.yaw, camera_.pitch);
    observer_.up = cross(observer_.forward,
                         {-std::sin(camera_.yaw), std::cos(camera_.yaw), 0});
  }
  camera_to_ = {p.position, p.rotation.x, p.rotation.y};
  camera_to_.roll = p.rotation.z;
  const auto basis = authored_basis(p.rotation);
  camera_to_.forward = basis.forward;
  camera_to_.up = basis.up;
  camera_to_.oriented = true;
  camera_duration_ = ticks;
  camera_elapsed_ = 0;
  // 16C3F1 flushes the newly adopted command with this literal, preserving
  // motion derivatives when another camera_set interrupts the transition.
  update_camera(.0001f);
}
void Presentation::control(bool enabled, const Camera &player) {
  if (enabled && !scripted && !animation_)
    camera_ = camera_from_ = camera_to_ = player;
  scripted = enabled;
}
void Presentation::camera_animation(std::uint32_t graph, std::uint32_t name,
                                    int flag) {
  if (!loader_)
    throw FormatError("camera animation loader unavailable");
  if (flag < -1 ||
      (flag >= 0 && std::size_t(flag) >= content_.cutscene_flags.size()))
    throw FormatError("invalid camera anchor flag");
  auto clip = loader_(graph, name);
  if (clip.nodes.size() != 1 || !clip.frame_count)
    throw FormatError("camera animation requires one camera node");
  anchor_ = flag < 0 ? CutsceneFlag{} : content_.cutscene_flags[flag];
  animation_ = std::move(clip);
  camera_duration_ = int(animation_->frame_count - 1);
  camera_elapsed_ = 0;
  update_camera();
}
void Presentation::update_camera(float dt) {
  if (animation_) {
    const auto pose = sample_animation(*animation_, 0, float(camera_elapsed_));
    const glm::quat anchor(glm::vec3(0, -anchor_.pitch, anchor_.yaw));
    const auto r = anchor * glm::quat(pose.rotation[3], pose.rotation[0],
                                      pose.rotation[1], pose.rotation[2]);
    const auto p = anchor * glm::vec3(pose.translation.x, pose.translation.y,
                                      pose.translation.z);
    camera_.position = anchor_.position + Vec3{p.x, p.y, p.z};
    const auto f = r * glm::vec3(1, 0, 0), u = r * glm::vec3(0, 0, 1);
    camera_.forward = {f.x, f.y, f.z};
    camera_.up = {u.x, u.y, u.z};
    camera_.oriented = true;
  } else {
    observer_.step(camera_to_.position, camera_to_.forward, camera_to_.up,
                   std::max(0, camera_duration_ - camera_elapsed_) * (1.f / 30),
                   dt);
    camera_.position = observer_.position;
    camera_.forward = observer_.forward;
    camera_.up = observer_.up;
    camera_.yaw = std::atan2(camera_.forward.y, camera_.forward.x);
    camera_.pitch = std::asin(std::clamp(camera_.forward.z, -1.f, 1.f));
    camera_.roll = camera_to_.roll;
    camera_.oriented = true;
  }
}
Camera Presentation::camera(const Camera &player, float render_fraction) const {
  auto result = scripted ? camera_ : player;
  if (scripted && !animation_ && camera_elapsed_ < camera_duration_ &&
      std::isfinite(render_fraction) && render_fraction > 0) {
    // Sample the same observer curve between simulation ticks, retaining the
    // authoritative derivatives for the next script command.
    auto sample = observer_;
    sample.step(camera_to_.position, camera_to_.forward, camera_to_.up,
                std::max(0, camera_duration_ - camera_elapsed_ - 1) * (1.f / 30),
                std::min(render_fraction, 1.f) * (1.f / 30));
    result.position = sample.position;
    result.forward = sample.forward;
    result.up = sample.up;
  }
  if (scripted) {
    result.field_of_view = fov_.value();
    result.horizontal_field_of_view = true;
    if (!animation_ && !result.oriented && result.roll != 0) {
      const glm::vec3 forward(std::cos(result.pitch) * std::cos(result.yaw),
                              std::cos(result.pitch) * std::sin(result.yaw),
                              std::sin(result.pitch));
      const glm::vec3 right(std::sin(result.yaw), -std::cos(result.yaw), 0);
      const auto up =
          glm::angleAxis(result.roll, forward) * glm::cross(right, forward);
      result.forward = {forward.x, forward.y, forward.z};
      result.up = {up.x, up.y, up.z};
      result.oriented = true;
    }
  }
  return result;
}
ScreenEffects Presentation::effects() const {
  const float fraction =
      fade_.duration > 0
          ? std::clamp(float(fade_.elapsed) / fade_.duration, 0.f, 1.f)
          : 1.f;
  if (fade_.to != 1 && fade_.from != 1)
    return {fade_color_, 0, letterbox_.value(), hud_.value()};
  if (!content_.original_data)
    throw FormatError("fade transition requires original game data");
  const float curve = fade_curve_fraction(fraction, content_.original_data->fade_curve);
  const float alpha =
      fade_.to == 1 ? curve : (fade_.from == 1 ? 1.f - curve : 0.f);
  return {fade_color_, alpha, letterbox_.value(), hud_.value()};
}
int Presentation::camera_time() const {
  return std::max(0, camera_duration_ - camera_elapsed_);
}
} // namespace halo2
