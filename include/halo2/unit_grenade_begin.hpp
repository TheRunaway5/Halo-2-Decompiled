#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Literal TU1.1 direct-owner entries; argument order is documented at each
// invocation. Required lower owners must execute or fail, never return
// defaults.
struct GrenadeBeginLeaf {
  std::uint32_t entry;
  std::array<std::uint32_t, 6> arguments{};
  unsigned count{};
  friend bool operator==(const GrenadeBeginLeaf &,
                         const GrenadeBeginLeaf &) = default;
};
struct GrenadeBeginLeafResult {
  std::uint32_t eax{}, xmm0_bits{};
};
struct SourceGrenadeBeginServices {
  // Genuine retained C52C0/BD270 storage. Spans remain valid for the entire
  // begin; lower callbacks mutate this same owned state. Index lookup/salted
  // caller admission are separate: E95E0 itself indexes original low16.
  std::function<std::span<std::byte>(std::uint32_t unit)> unit_body;
  // Actual matg selected signed index. Empty means the source bank is absent;
  // missing service is unavailable. Nonempty is the complete2c record.
  std::function<std::span<const std::byte>(std::int8_t type)>
      grenade_definition;
  // Actual parent definition's signed-index B0 seat record; no guessed flags.
  std::function<std::span<const std::byte>(std::uint32_t parent,
                                           std::int16_t seat)>
      seat_definition;
  std::function<std::span<const std::byte>(std::uint32_t player)> player_body;
  // output is caller-owned4 bytes for115650,12 bytes forB9C20, otherwise empty.
  // point/direction are original request vectors only for immediate E8F30.
  // Result carries defined low AL/AX or XMM0 only at the actual caller branch.
  std::function<GrenadeBeginLeafResult(
      const GrenadeBeginLeaf &, std::span<std::byte> output,
      std::span<const std::byte> point, std::span<const std::byte> direction)>
      invoke;
  // Literal original465048 zero word from the selected executable provider.
  std::uint32_t zero_bits;
};
enum class GrenadeBeginBuffer { action, request };
struct GrenadeBeginStore {
  std::uint32_t instruction, datum;
  GrenadeBeginBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const GrenadeBeginStore &,
                         const GrenadeBeginStore &) = default;
};
using GrenadeBeginObserver = std::function<void(const GrenadeBeginStore &)>;
// Source151513..15153c packet construction only, after its separate input
// gates. Eight original DWORD clears then type22; bytes beyond20 remain
// untouched.
void construct_source_grenade_begin_request(std::span<std::byte>,
                                            const GrenadeBeginObserver & = {});
// Resolve actual signed unit+346; no component allocation or initialization.
std::span<std::byte>
source_grenade_begin_action_storage(std::span<std::byte> unit);
// Complete E95E0 direct body, including immediate and no-count notification
// branches. Defined AL is returned. Every unavailable lower owner throws after
// preceding source stores; it is never converted into false/success/NONE.
bool begin_source_grenade(std::uint32_t unit,
                          std::span<const std::byte> request,
                          const SourceGrenadeBeginServices &,
                          const GrenadeBeginObserver & = {});
} // namespace halo2
