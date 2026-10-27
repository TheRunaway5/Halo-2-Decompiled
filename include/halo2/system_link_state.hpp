#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_datagrams.hpp"

namespace halo2 {
// Native port session state. These are not Xbox/Vista lifecycle numbers.
enum class SystemLinkPhase : unsigned { running, reviewing, postgame, lobby, starting };
struct SystemLinkState {
  SystemLinkPhase phase=SystemLinkPhase::running;
  std::uint32_t revision=1,generation=1;
  std::uint64_t fingerprint{};
  std::uint16_t active_players{};
  std::int32_t countdown=-1;
  // Host's netcode mode for the published world (protocol 113); lobby read point.
  NetcodeMode netcode_mode=NetcodeMode::original;
  std::string map;
  UiVariant variant;
  std::vector<OnlineAdmission> roster;
  std::array<std::optional<SourcePlayerConfiguration>,16> profiles;
  std::vector<std::byte> report;
};
}
