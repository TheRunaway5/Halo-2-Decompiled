#pragma once
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer.hpp"
namespace halo2 {
class PhysicsLiveWorldTu15;
enum class CampaignRenderBspLifecycle { activation,checkpoint_restore };
using CampaignRenderBspObserver=std::function<void(PhysicsLiveWorldTu15 &,std::size_t,
    CampaignRenderBspLifecycle)>;
// One source authority for campaign objects, weapons, targeting and physics.
// Simulation retains script/presentation values and borrows this owner's pool.
class CampaignSourceRuntime {
public:
  CampaignSourceRuntime(const Cache &,std::size_t bsp,unsigned difficulty,std::uint32_t seed,
      std::optional<std::uint32_t> reference_auxiliary_seed={});
  CampaignSourceRuntime(const Cache &,const std::filesystem::path &xbe,
      std::size_t bsp,unsigned difficulty,std::uint32_t seed,
      std::optional<std::uint32_t> reference_auxiliary_seed={});
  ~CampaignSourceRuntime();
  NativeObjectLifetimes lifetimes();
  SourceSceneObjectOwner scene_owner(const LevelContent &);
  void respawn(Simulation &,Spawn);
  void equip(Simulation &,std::uint32_t definition);
  void set_difficulty(unsigned);
  void set_team_allegiance(std::int16_t,std::int16_t,bool);
  void set_kill_volume_enabled(std::int16_t trigger,bool enabled);
  void switch_bsp(std::size_t);
  // Local renderer binding only; never serialized/checkpointed. Activation
  // follows genuine gameplay callbacks and precedes scenario factories.
  // Restore is a distinct resource-rebind notification, NOT an implicit
  // weather/RNG initializer or a replay of gameplay BSP callbacks.
  void render_bsp_lifecycle(CampaignRenderBspObserver);
  void publish(Simulation &simulation){adopt(simulation);}
  std::optional<unsigned> random_choice(unsigned count,std::uint32_t visited);
  void deliver_controls(Simulation &,const Controls &);
  void step(Simulation &,const Controls &);
  // Forward compiled AI values and optional signed-short counts unchanged.
  // The original source actions own placement and the pending reference.
  void place_ai(Simulation &,std::uint32_t reference,
      std::optional<std::int16_t> count={});
  // Return the source actor unit datum for the unmodified AI reference.
  std::uint32_t ai_object(std::uint32_t reference);
  std::size_t place_squad(Simulation &,std::size_t);
  MultiplayerState::Player hud_player(const Simulation &);
  MultiplayerObjectRuntime &objects(){return *objects_;}
  // Read-only capture receipt from this actual World, never a replay bank.
  // The optional constructor input is delivered after session configuration,
  // before Simulation can invoke scenario factories (same146240 auxiliary
  // stream as the finite multiplayer reference capture).
  struct ReferenceState {
    // Stable original SOURCE tokens; native host addresses are read locally
    // and reverse-mapped before a fresh-process comparison receipt is emitted.
    std::uint32_t random_address{},clock_address{};
    std::uint32_t game_options_seed{};
    std::array<std::uint32_t,2> random_words{};
    std::array<std::byte,0x24> clock{};
  };
  ReferenceState reference_state()const;
  struct Checkpoint {
    MultiplayerObjectRuntime::AuthorityCheckpoint objects;
    NativeObjectLifetimes::SourceAuthorityCheckpoint lifetimes;
    std::uint32_t player;
    std::map<std::pair<int,int>,std::uint32_t> placements;
  };
  Checkpoint checkpoint(const NativeObjectLifetimes &)const;
  void restore(const Checkpoint &,NativeObjectLifetimes &);
  // Local actual-renderer registration; never checkpointed or serialized.
  void live_lens_renderer(bool active);
private:
  std::filesystem::path files_;
  std::unique_ptr<MultiplayerObjectRuntime> objects_;
  std::uint32_t player_=null_datum;
  std::map<std::pair<int,int>,std::uint32_t> placements_;
  CampaignRenderBspObserver render_bsp_observer_;
  void adopt(Simulation &);
  void adopt_unit(Unit &,const LevelContent &,NativeObjectLifetimes &);
};
}
