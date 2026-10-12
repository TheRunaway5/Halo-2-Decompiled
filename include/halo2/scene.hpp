#pragma once
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/camera_motion.hpp"
#include "halo2/object_light_contacts.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include <functional>
#include <optional>
#include <map>
namespace halo2 {
struct Camera {
  Vec3 position;
  float yaw{}, pitch{}, field_of_view = 75;
  bool oriented{};
  bool horizontal_field_of_view{};
  Vec3 forward{1, 0, 0}, up{0, 0, 1};
  float roll{};
  // A recovered render-camera angle may differ from the observer's FOV.
  std::optional<float> projection_vertical_angle;
  Vec2 projection_clip_scale{1, 1};
  Vec2 projection_clip_offset{}; // Source unflipped clip XY += offset*W.
};
float camera_vertical_field_of_view(const Camera &, float aspect);
struct ScreenEffects {
  Vec3 fade_color;
  float fade{}, letterbox{}, hud = 1;
};
// Scenario names refer to placements, and resolve to a live generation handle.
// This scene layer owns transforms/lifetime; it does not emulate Havok bodies.
struct SourceSceneGameContext {
  int game_type{}, engine{}; // Literal original game-options+8/+180.
};
// Simulation supplies these before automatic placements. A source birth owns
// allocation, body construction and registration in the retained source world.
struct SourceSceneObjectOwner {
  std::function<std::uint32_t(const ScenarioObject &)> create;
  std::function<void(std::uint32_t)> destroy;
};
class SceneObjects {
public:
  using RenderAttributionDefinitionResolver =
      std::function<std::optional<ObjectLightContactDefinition>(std::uint32_t)>;
  struct Object {
    std::uint32_t handle{};
    std::size_t placement{};
    Vec3 position, rotation;
    float scale = 1;
    bool visible = true;
    bool bsp_hidden{};
    bool script_hidden{};
    bool inventory_attached{}; // Native item membership, independent of placement residency.
    int bsp = -1;
    bool cinematic_lod{};
    // Simulation dispatch tick of this generation's constructor. Rendering
    // may follow several simulation steps, so it cannot supply this clock.
    std::uint64_t created_tick{};
    // BF9A0/B7B40 construction-event projection for this exact generation.
    // No complete body flags or later/current flag31 state is represented.
    // Absent means this scene has no authored-definition resolver.
    std::optional<bool> render_attribution_at_birth;
    // Completed source physics root, including its exact basis/handedness.
    // Retain placement Euler angles as metadata; never round-trip solved
    // rotations through them for interpolation or model submission.
    std::optional<Transform> solved_transform;
    // Current source WORLD hierarchy. Empty is an actual empty source bank;
    // absent belongs to the existing native placement/animation owner.
    std::optional<std::vector<Transform>> solved_nodes;
    std::uint32_t attachment_parent=null_datum;
  };
  SceneObjects(const LevelContent &, std::size_t bsp,
               std::optional<SourceSceneGameContext> = {},
               RenderAttributionDefinitionResolver = {},
               NativeObjectLifetimes *shared_lifetimes = nullptr,
               SourceSceneObjectOwner = {});
  void restore(const SceneObjects &);
  std::uint32_t resolve(int name) const;
  std::uint32_t create(int name, bool replace = false,
                       std::uint64_t creation_tick = 0);
  std::uint32_t create_placement(std::size_t placement,std::uint64_t tick=0){return place(placement,tick);}
  void destroy(std::uint32_t handle);
  void destroy_all();
  void destroy_type_mask(std::uint32_t);
  Object &get(std::uint32_t);
  const Object &get(std::uint32_t) const;
  Vec3 center(std::uint32_t) const;
  void set_function_variable(std::uint32_t object, std::uint32_t name,
                             float value, float frames, int tick);
  std::optional<float> function_variable(std::uint32_t object,
                                         std::uint32_t name, int tick) const;
  const std::vector<Object> &objects() const { return objects_; }
  const NativeObjectLifetimes &object_lifetimes() const;
  bool contains(const TriggerVolume &, Vec3 center) const;
  bool retains(int previous_bsp, int next_bsp, Vec3 center) const;
  void switch_bsp(std::size_t, bool suppress_automatic = false);
  void visible(std::uint32_t,bool);

private:
  const LevelContent &content_;
  std::size_t bsp_;
  std::optional<SourceSceneGameContext> game_context_;
  RenderAttributionDefinitionResolver render_attribution_definition_;
  SourceSceneObjectOwner source_owner_;
  bool factory_game_allowed(const ScenarioObject &) const;
  // Standalone scenes own a value bank. Simulation supplies its shared bank
  // before any placement and snapshots/restores that bank separately.
  std::optional<NativeObjectLifetimes> local_lifetimes_;
  NativeObjectLifetimes *shared_lifetimes_{};
  NativeObjectLifetimes &lifetime_owner();
  std::vector<Object> objects_;
  std::map<std::uint32_t,std::size_t> slots_;
  std::vector<std::uint32_t> names_;
  struct FunctionVariable { float from{}, to{}; int start{}, duration{}; };
  std::map<std::pair<std::uint32_t,std::uint32_t>,FunctionVariable> functions_;
  std::uint32_t place(std::size_t, std::uint64_t creation_tick = 0);
};
class Presentation {
public:
  using AnimationLoader =
      std::function<AnimationClip(std::uint32_t, std::uint32_t)>;
  explicit Presentation(const LevelContent &, AnimationLoader = {});
  void restore(const Presentation &);
  void tick();
  void fade(Vec3 color, bool out, int ticks);
  void letterbox(bool show, bool immediate);
  void hud(float value, int ticks);
  void field_of_view(float degrees, int ticks);
  void cinematic_control(bool enabled);
  void camera_point(int index, int ticks);
  void control(bool enabled, const Camera &player);
  void camera_animation(std::uint32_t graph, std::uint32_t name, int flag);
  Camera camera(const Camera &player, float render_fraction = 0) const;
  ScreenEffects effects() const;
  int camera_time() const;
  bool cinematic{}, scripted{}, player_camera_enabled = true;

private:
  struct Transition {
    float from{}, to{};
    int duration{}, elapsed{};
    float value() const;
    void set(float target, int ticks);
    void tick();
  };
  const LevelContent &content_;
  AnimationLoader loader_;
  Transition fade_, letterbox_, hud_{1, 1}, fov_{70, 70};
  Vec3 fade_color_;
  Camera camera_, camera_from_, camera_to_;
  CameraMotion observer_;
  int camera_duration_{}, camera_elapsed_{};
  std::optional<AnimationClip> animation_;
  CutsceneFlag anchor_;
  void update_camera(float dt = 1.f / 30);
};
} // namespace halo2
