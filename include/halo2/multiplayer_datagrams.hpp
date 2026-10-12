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
inline constexpr unsigned native_multiplayer_protocol = 50;
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
};
} // namespace halo2
