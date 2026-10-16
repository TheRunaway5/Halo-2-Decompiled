#pragma once
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer.hpp"
namespace halo2 {
// One source authority for campaign objects, weapons, targeting and physics.
// Simulation retains script/presentation values and borrows this owner's pool.
class CampaignSourceRuntime {
public:
  CampaignSourceRuntime(const Cache &,const std::filesystem::path &xbe,
      std::size_t bsp,unsigned difficulty,std::uint32_t seed);
  ~CampaignSourceRuntime();
  NativeObjectLifetimes lifetimes();
  SourceSceneObjectOwner scene_owner(const LevelContent &);
  void respawn(Simulation &,Spawn);
  void equip(Simulation &,std::uint32_t definition);
  void set_difficulty(unsigned);
  void set_team_allegiance(std::int16_t,std::int16_t,bool);
  void set_kill_volume_enabled(std::int16_t trigger,bool enabled);
  void switch_bsp(std::size_t);
  void publish(Simulation &simulation){adopt(simulation);}
  std::optional<unsigned> random_choice(unsigned count,std::uint32_t visited);
  void deliver_controls(Simulation &,const Controls &);
  void step(Simulation &,const Controls &);
  // Forward compiled AI values and optional signed-short counts unchanged.
  // The original source actions own placement and the pending reference.
  void place_ai(Simulation &,std::uint32_t reference,
      std::optional<std::int16_t> count={});
  std::size_t place_squad(Simulation &,std::size_t);
  MultiplayerState::Player hud_player(const Simulation &);
  MultiplayerObjectRuntime &objects(){return *objects_;}
  struct Checkpoint {
    MultiplayerObjectRuntime::AuthorityCheckpoint objects;
    NativeObjectLifetimes::SourceAuthorityCheckpoint lifetimes;
    std::uint32_t player;
    std::map<std::pair<int,int>,std::uint32_t> placements;
  };
  Checkpoint checkpoint(const NativeObjectLifetimes &)const;
  void restore(const Checkpoint &,NativeObjectLifetimes &);
private:
  std::filesystem::path files_;
  std::unique_ptr<MultiplayerObjectRuntime> objects_;
  std::uint32_t player_=null_datum;
  std::map<std::pair<int,int>,std::uint32_t> placements_;
  void adopt(Simulation &);
  void adopt_unit(Unit &,const LevelContent &,NativeObjectLifetimes &);
};
}
