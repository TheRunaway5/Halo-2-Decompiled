#pragma once
#include "halo2/hud.hpp"
namespace halo2 {
MultiplayerState::Player native_weapon_hud_player(const Unit &,const LevelContent &);
// Campaign weapon and binocular widgets, prepared before the mesh is uploaded.
// Reconstruct with the destination mesh after a BSP replacement.
class NativeWeaponHud {
public:
  NativeWeaponHud(const Cache &,const LevelContent &,Mesh &,
      const std::filesystem::path &fonts,const std::filesystem::path &assets);
  std::vector<Vertex> frame(const Unit &,std::uint32_t tick,unsigned width,unsigned height,
      std::optional<Vec2> aim_anchor={});
private:
  const LevelContent &content_;
  std::unique_ptr<MultiplayerHud> binocular_;
  std::map<std::uint32_t,std::unique_ptr<MultiplayerHud>> primary_,secondary_;
};
}
