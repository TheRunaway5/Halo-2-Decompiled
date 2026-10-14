#pragma once
#include <cstddef>
#include <array>
#include <span>
#include <string>
#include <vector>
namespace halo2 {
//35 carries the complete authoritative device bank, including shared groups
// and current motion/lifetimes. Older peers keep their initial machinery.
//39 adds each player's authoritative weapon/control action prompt context.
//40 carries the original round selector used by the limit-one HUD query.
//43 admits the original ally-trade, seat and vehicle-flip prompt kinds.
//45 carries completed source WORLD body poses and their retained spheres.
//52 retains detached corpse appearance/poses and widens complete snapshot fragments.
//53 carries the collision owner's chosen projectile response and effect scales.
//54 carries each local dead player's completed authoritative observer view.
//55 requires completed vehicle WORLD hierarchies alongside player/corpse poses.
//56 retains the actual parent seat for source vehicle occupants.
//57 carries each vehicle's ultimate-root origin, original physics velocity,
// runtime query flags and frozen-lighting state for independent client views.
//58 retains authoritative melee contact presentation separately from bullets.
//59 carries the actual completed TU1.5 seated observer for remote local views.
//60 retains the independent raw heat word for primary, secondary and world weapons.
//61 retains automatic-door flags and signed autoclose timers with device state.
//62 carries the completed source explosion location names and world frames.
//63 retains completed live prop model-region choices and looping handles.
//64 carries each vehicle model owner's geometry LOD flags, independent of lighting roots.
//65 retains both original explosion effect colors alongside its location bank.
//66 carries complete source scene roots and their adopted WORLD banks.
//67 retains completed original attached unit-effect lighting colors.
//68 carries every prepared-shot effect, original colors/velocity and held WORLD poses.
//69 carries completed scene-root velocity, runtime flags and frozen state.
//70 retains grenade-selector input edges across loss/reordering.
//71 carries authoritative trigger input, mode, ticks and charge for both hands.
//72 retains actual scene creation ticks and admits authoritative replica generations.
//73 carries source-created equipment/debris/crate roots, model regions and colors.
//75 carries current full attached-effect identities, controls, colors and flags.
//76 adds complete host configuration discovery before client preparation.
//80 carries all512 possible linked source firing locations, preserving names/transforms.
//81 carries source HUD aim flags, lock admission and per-hand marker positions.
inline constexpr unsigned native_multiplayer_protocol = 81;
// Endpoint numbers are authenticated room slots for ONLINE, opaque UDP peers
// for local transport. Packet contents retain the native host/client protocol.
class MultiplayerDatagrams {
public:
  virtual ~MultiplayerDatagrams() = default;
  virtual void send(std::span<const std::byte>, unsigned peer) = 0;
  virtual bool receive(std::vector<std::byte> &, unsigned &peer) = 0;
};
struct OnlineAdmission {
  unsigned slot{}; std::string name; int team{};
  // Private, random per-room native presentation key, not a Discord ID/XUID.
  std::array<std::byte,8> identity{};
  // Connection endpoint owning this player; legacy solo descriptors use slot.
  unsigned peer = ~0u;
  bool guest{};
};
inline unsigned online_admission_peer(const OnlineAdmission &player) {
  return player.peer == ~0u ? player.slot : player.peer;
}
} // namespace halo2
