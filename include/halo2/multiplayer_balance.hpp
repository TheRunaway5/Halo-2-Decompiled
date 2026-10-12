#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
// Cache bytes and original pointers are supplied by the asset adapter. Values
// are imported from the selected original executable, never native tuning.
struct MultiplayerBalanceMemory {
  std::span<std::byte> bytes;
  std::int64_t metadata_magic{};
  std::size_t tag_table_offset{}, tag_count{};
  std::uint32_t scenario_address{}, globals_address{};
};
struct MultiplayerBalanceStore {
  std::uint32_t instruction{};
  std::size_t file_offset{};
  std::uint32_t before{}, after{};
};
struct MultiplayerBalanceResult {
  bool admitted{};
  std::size_t stores{};
};
// Original 1.1 164750 / 1.5 165B30. The latter's store sites have delta 13E0.
// Host bounds failures throw FormatError; they are not invented source gates.
MultiplayerBalanceResult apply_multiplayer_balance(
    MultiplayerBalanceMemory, std::span<const std::uint32_t, 19> constants,
    std::uint32_t instruction_delta = 0,
    std::vector<MultiplayerBalanceStore>* trace = nullptr);
} // namespace halo2
