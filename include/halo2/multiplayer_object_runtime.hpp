#pragma once
#include "halo2/death_startup_native.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/observer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"

namespace halo2 {
class Cache;
struct Vec3;
struct ShadowCasterSphere;
struct SourceGameVariant;
struct ScenarioObject;
class PhysicsLiveWorldTu15;
struct SourceObjectDrawCollector;
struct MultiplayerSourceMotion {
  Vec3 position,velocity,forward;
  float crouch;
  std::uint8_t physical_state;
};
// One retained source bank for the native multiplayer object world. Native
// process/cache bindings own host resources; complete original constructors
// produce pool, heap, physics and object state. This is not Xbox boot emulation.
class MultiplayerObjectRuntime {
public:
  // Cache owns resource I/O and must outlive this runtime.
  MultiplayerObjectRuntime(const std::filesystem::path &source_image,
      const Cache &,std::size_t bsp,const std::filesystem::path &savegame_directory);
  ~MultiplayerObjectRuntime();
  MultiplayerObjectRuntime(const MultiplayerObjectRuntime &)=delete;
  MultiplayerObjectRuntime &operator=(const MultiplayerObjectRuntime &)=delete;
  SourceRuntimeMemory &memory();
  std::uint32_t object_pool() const;
  std::uint32_t player_pool() const;
  bool owns_physics() const;
  std::optional<std::uint32_t> physical_body_count() const;
  std::uint32_t source_clock() const;
  std::uint32_t object_body(std::uint32_t object) const;
  MultiplayerSourceMotion biped_motion(std::uint32_t object) const;
  void deliver_command(std::uint32_t player,std::uint32_t object,
                       std::span<const std::byte,0x5c> command);
  // Native presentation owns local views; the source simulation receives the
  // actual allocated player, OS machine identity and selected configuration.
  // Complete14D800/14D600 construct its record, without a parallel player pool.
  void construct_player(std::uint32_t player,unsigned native_local_user,
                        const SourcePlayerConfiguration &);
  void bind_player(std::uint32_t player,std::uint32_t object);
  SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,
                                                        std::uint32_t object);
  SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const;
  // Native confirmed-fatal stimulus. D7CD0 produces its actual damage record;
  // D7E00/CBDC0 own the ordinary death transition and player detachment.
  void kill_biped(std::uint32_t object);
  // Explicit native session inputs at the original game-options delivery
  // boundary. Clock reset consumes this rate; no captured options are loaded.
  void configure_session(const SourceGameVariant &,std::uint16_t simulation_mode,
      std::int16_t tick_rate,std::uint32_t random_seed);
  // Native placement input, complete B8820/B8A30 admission and construction.
  // Returns the actual salted source handle; never reserves a parallel header.
  std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,
                             const Vec3 &forward);
  std::uint32_t create_scenario_object(const ScenarioObject &);
  std::uint32_t create_netgame_object(unsigned row);
  void destroy_object(std::uint32_t full_datum);
  // Actual source clock increment followed by the five complete object/world
  // phases. Retains the same bank; does not supply contacts or death admission.
  void step_objects();
  // Read-only presentation of the current source WORLD matrices. A stale
  // full handle or unavailable node backing is an error, never an idle pose.
  std::vector<Transform> world_nodes(std::uint32_t object) const;
  //133790 consumes the retained object+30/+3C sphere. Resolve the same full
  //salted owner as WORLD nodes; preview-model bounds are not this bank.
  ShadowCasterSphere object_sphere(std::uint32_t object) const;
  // Native presentation inputs over this same retained TU1.1 bank. Resources
  // finish through the original queue/read/fixup graph before their first draw.
  void preload_render_model(std::uint32_t render_model);
  void begin_model_frame();
  SourceObjectDrawCollector build_world_model(std::uint32_t object,unsigned view,
      unsigned local_players,bool view_modifier,float geometry_metric,float lighting_metric);
  std::optional<DeathCameraTarget> camera_target(std::uint32_t object) const;
  SourceObserverWorldServices observer_world_services();
  SourceObserverWorldFrame observer_world_frame(std::int32_t view,
      std::uint32_t current_unit,std::uint16_t player_flags,
      const DeathCameraSavedClusters &) const;
  // Original full-ABI entry on the same bank, including reached native resource
  // owners. Scratch return/arguments are native caller storage, never snapshots.
  std::uint32_t call(std::uint32_t entry,std::span<const std::uint32_t> arguments={},
      std::optional<std::uint32_t> eax={},std::optional<std::uint32_t> ecx={},
      std::optional<std::uint32_t> edx={});
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::unique_ptr<PhysicsLiveWorldTu15> tu15_;
};
} // namespace halo2
