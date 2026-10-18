#pragma once
#include "halo2/source_scenario_birth.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/camera_motion.hpp"
#include "halo2/object_light_contacts.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/source_world_pose.hpp"
#include "halo2/native_authority_boundary.hpp"
#include <functional>
#include <optional>
#include <map>
#include <exception>
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
  std::function<std::uint32_t(const ScenarioObject &,SourceScenarioBirth)> create;
  std::function<void(std::uint32_t)> destroy;
  // Original construction owns child allocation. Presentation binds the
  // already constructed child to this exact retained parent generation.
  std::function<std::uint32_t(const ScenarioObject &,std::uint32_t)> adopt_child;
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
    std::optional<SourceModelRegionBank> source_model_regions;
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
  std::uint32_t create_attached_placement(std::size_t placement,std::uint32_t parent,std::uint64_t tick=0) {
    if(parent==null_datum)throw FormatError("Model child requires an actual parent generation");
    return place(placement,tick,SourceScenarioBirth::runtime,parent);
  }
  void destroy(std::uint32_t handle);
  // Source deletion has already retired this full generation. Remove only
  // its presentation and lifetime sidecar; never invoke the destructor again.
  void observe_source_retirement(std::uint32_t handle);
  // Completed authority snapshots own replica identities. This presentation
  // does not reserve headers, construct bodies or advance another world.
  // Vehicles are presented by the separate authoritative vehicle bank.
  void adopt_source_replicas(std::span<const SourceSceneRoot>,std::span<const SourceWorldPose>);
  bool source_replica()const{return source_replica_;}
  bool source_factory_bound()const{return bool(source_owner_.create);}
  void destroy_all();
  void destroy_type_mask(std::uint32_t);
  Object &get(std::uint32_t);
  const Object &get(std::uint32_t) const;
  Vec3 center(std::uint32_t) const;
  void set_function_variable(std::uint32_t object, std::uint32_t name,
                             float value, float frames, int tick);
  std::optional<float> function_variable(std::uint32_t object,
                                         std::uint32_t name, int tick) const;
  void clear_function_variable(std::uint32_t object,std::uint32_t name);
  // Completed samples from the retained source owner. No native interpolation
  // or slot replacement policy advances these borrowed shader inputs.
  void publish_source_function_samples(const std::map<std::pair<std::uint32_t,std::uint32_t>,float> &);
  const std::vector<Object> &objects() const { return objects_; }
  const NativeObjectLifetimes &object_lifetimes() const;
  bool contains(const TriggerVolume &, Vec3 center) const;
  bool retains(int previous_bsp, int next_bsp, Vec3 center) const;
  void switch_bsp(std::size_t, bool suppress_automatic = false);
  void visible(std::uint32_t,bool);
  struct AuthorityFunction {float from{},to{};int start{},duration{};};
  enum class AuthorityPool {LocalOwned,SharedOwned,SharedSource};
  struct AuthorityCheckpoint {
    std::uint32_t version=1;
    std::array<std::byte,32> content_binding{};
    std::size_t bsp{};
    AuthorityPool pool=AuthorityPool::LocalOwned;
    bool source_factory{},render_attribution_resolver{},source_child_factory{};
    std::vector<Object> objects;
    std::map<std::uint32_t,std::size_t> slots;
    std::vector<std::uint32_t> names;
    std::map<std::pair<std::uint32_t,std::uint32_t>,AuthorityFunction> functions;
    std::optional<NativeObjectLifetimes> local_lifetimes;
  };
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const SceneObjects &) const noexcept;
  private:
    friend class SceneObjects;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Pointer-free scene values; local content, factory/resolver callbacks and
  // shared identity-owner addresses never import. The enclosing transaction
  // must authenticate the complete map/image and quiesce every scene borrower.
  // A display replica is not an authority checkpoint.
  AuthorityCheckpoint authority_checkpoint() const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &); // Standalone owned scene.
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &,const NativeObjectLifetimes &incoming_owned);
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &,
      const NativeObjectLifetimes::SourceAuthorityCheckpoint &,const SourceRuntimeMemory &replacement);
  // Shared identity owners must publish first; source views must rebind to the
  // exact staged bank. Owner and shared bank outlive preparation. Commit/cancel
  // on the enclosing executor; commit consumes the token only on success.
  bool commit_authority(PreparedAuthority &&) noexcept;

private:
  struct AuthorityCalls {
    std::size_t depth{};
    std::uint64_t mutations{};
    bool staging{},fault{};
    AuthorityCalls()=default;
    AuthorityCalls(const AuthorityCalls &other):fault(other.fault){}
    struct Scope {
      AuthorityCalls &owner;int exceptions=std::uncaught_exceptions();std::uint64_t before;
      explicit Scope(AuthorityCalls &owner):owner(owner),before(owner.mutations) {
        if(owner.staging)throw FormatError("Scene mutation during authority preparation");
        ++owner.depth;
      }
      ~Scope(){if(std::uncaught_exceptions()>exceptions && before!=owner.mutations)owner.fault=true;--owner.depth;}
    };
  } authority_calls_;
  const LevelContent &content_;
  std::size_t bsp_;
  std::optional<SourceSceneGameContext> game_context_;
  RenderAttributionDefinitionResolver render_attribution_definition_;
  SourceSceneObjectOwner source_owner_;
  bool source_replica_{};
  bool factory_game_allowed(const ScenarioObject &) const;
  // Standalone scenes own a value bank. Simulation supplies its shared bank
  // before any placement and snapshots/restores that bank separately.
  std::optional<NativeObjectLifetimes> local_lifetimes_;
  NativeObjectLifetimes *shared_lifetimes_{};
  NativeObjectLifetimes &lifetime_owner();
  PreparedAuthority prepare_authority_values(const AuthorityCheckpoint &);
  std::vector<Object> objects_;
  std::map<std::uint32_t,std::size_t> slots_;
  std::vector<std::uint32_t> names_;
  using FunctionVariable=AuthorityFunction;
  std::map<std::pair<std::uint32_t,std::uint32_t>,FunctionVariable> functions_;
  std::uint32_t place(std::size_t, std::uint64_t creation_tick = 0,
                      SourceScenarioBirth = SourceScenarioBirth::runtime,
                      std::uint32_t attachment_parent = null_datum);
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
  // Sample the current authored transition interval without ticking it.
  // Zero is the completed game state; one meets the next tick endpoint.
  // Immediate commands retain their current value at every render fraction.
  ScreenEffects effects(float render_fraction) const;
  int camera_time() const;
  bool cinematic{}, scripted{}, player_camera_enabled = true;
  bool cinematic_outro{}; // Retail HUD/cinematic globals+22; retained until level teardown.
  struct AuthorityTransition {float from{},to{};int duration{},elapsed{};};
  struct AuthorityCheckpoint {
    std::uint32_t version=1;
    std::array<std::byte,32> content_binding{};
    bool loader{},cinematic{},scripted{},player_camera_enabled{},cinematic_outro{};
    std::array<AuthorityTransition,4> transitions;
    Vec3 fade_color;
    Camera camera,camera_from,camera_to;
    CameraMotion observer;
    int camera_duration{},camera_elapsed{};
    std::optional<AnimationClip> animation;
    CutsceneFlag anchor;
  };
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const Presentation &)const noexcept;
  private:
    friend class Presentation;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Value-only continuation with a target-local content reference/loader.
  // The enclosing root owns quiescence of direct public flags and consumers.
  // It authenticates the complete image/map/schema and publishes all owners
  // before resuming. Campaign restore remains a separate policy/API.
  AuthorityCheckpoint authority_checkpoint()const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &);
  bool commit_authority(PreparedAuthority &&) noexcept;

private:
  NativeAuthorityBoundary authority_;
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
