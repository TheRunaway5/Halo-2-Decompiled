#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
enum class MultiplayerNetgamePostprocessImage { MapPack11, TitleUpdate15 };

// Every member contains raw float words imported from the selected original
// executable. The shared Beaver Creek Z word is retained in both targets.
struct MultiplayerRelicPostprocessConstants {
  std::array<std::array<std::uint32_t, 3>, 2> targets{};
  std::uint32_t z_delta{};
};
struct MultiplayerNetgamePostprocessConstants {
  std::array<std::array<std::uint32_t, 3>, 2> beaver_creek_targets{};
  std::uint32_t distance_squared_threshold{}, beaver_creek_x_delta{};
  std::optional<MultiplayerRelicPostprocessConstants> relic;
};
struct MultiplayerNetgamePostprocessMemory {
  std::span<std::byte> bytes;
  std::int64_t metadata_magic{};
  std::uint32_t scenario_address{};
};
struct MultiplayerNetgamePostprocessStore {
  std::uint32_t instruction{};
  std::size_t file_offset{};
  std::uint32_t before{}, after{};
};
struct MultiplayerNetgamePostprocessProtection {
  std::uint32_t instruction{}, address{}, byte_count{}, protection{};
  std::size_t stores_completed{};
};
struct MultiplayerNetgamePostprocessTrace {
  std::vector<MultiplayerNetgamePostprocessStore> stores;
  std::vector<MultiplayerNetgamePostprocessProtection> protection_calls;
};
struct MultiplayerNetgamePostprocessResult {
  bool multiplayer{}, recognized_map{};
  std::uint32_t map_id{};
  std::size_t stores{};
};

// Original 1.1 11DF00 / 1.5 11F060. Each SSE arithmetic operation rounds to
// float separately, with each image's original operand/accumulation order.
// Host bounds failures throw FormatError rather than add source early-outs.
// Protection records retain the original requests and their position among
// stores. This leaf operates on caller-owned writable bytes; it does not
// implement Xbox page protection, teleporter traversal or world activation.
MultiplayerNetgamePostprocessResult apply_multiplayer_netgame_postprocess(
    MultiplayerNetgamePostprocessMemory,
    const MultiplayerNetgamePostprocessConstants&,
    MultiplayerNetgamePostprocessImage,
    MultiplayerNetgamePostprocessTrace* trace = nullptr);
} // namespace halo2
