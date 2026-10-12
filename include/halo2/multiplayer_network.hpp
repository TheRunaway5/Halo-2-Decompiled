#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <memory>
namespace halo2 {
// Native port protocol. It does not speak the retail Xbox System Link protocol.
// Paths themselves are excluded: identical imported data may live in different
// directories on each computer. Every supported shared cache and font is part
// of the identity, together with the BSP and selected starting weapon.
std::uint64_t multiplayer_fingerprint(const std::filesystem::path &map,
                                     const std::filesystem::path &shared_maps,
                                     std::uint32_t bsp,
                                     std::uint32_t starting_weapon,
                                     const std::filesystem::path &gameplay_xbe = {});
class LanHost {
public:
  LanHost(MultiplayerMatch &, unsigned port, std::uint64_t fingerprint);
  LanHost(MultiplayerMatch &, std::shared_ptr<MultiplayerDatagrams>, std::uint64_t,
          std::vector<OnlineAdmission>);
  ~LanHost();
  unsigned port() const;
  void receive(std::array<Controls, multiplayer_capacity> &);
  void broadcast();
  unsigned peers() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
class LanClient {
public:
  LanClient(const std::string &address, std::uint64_t fingerprint,
            unsigned local_players, std::span<const std::int8_t> selected_teams = {},
            std::span<const SourcePlayerConfiguration> selected_profiles = {});
  LanClient(std::shared_ptr<MultiplayerDatagrams>, unsigned host, std::uint64_t fingerprint,
            std::span<const std::int8_t> selected_teams = {},
            std::span<const SourcePlayerConfiguration> selected_profiles = {});
  ~LanClient();
  void send(std::span<const Controls>);
  void receive();
  bool ready() const;
  const MultiplayerState &state() const;
  // Complete accepted snapshots from the most recent receive(), in delivery
  // order. Presentation owners must consume each real tick before rendering
  // the latest state. Empty receives clear this bank; no missing ticks are
  // synthesized and stale/duplicate packets never enter it.
  std::span<const MultiplayerState> received_states() const;
  std::span<const unsigned> slots() const;
  std::string error() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
