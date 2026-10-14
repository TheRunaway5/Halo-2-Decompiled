#include "halo2/source_kill_volumes.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
SourceKillVolumeView::SourceKillVolumeView(
    std::span<std::byte> state,
    std::span<const std::int16_t> trigger_kill_indices,
    std::span<const std::int16_t> kill_triggers)
    : state_(state), trigger_kill_indices_(trigger_kill_indices),
      kill_triggers_(kill_triggers) {
  if (state.size() != 0x24 || kill_triggers.size() > 256)
    throw std::invalid_argument("Kill volumes require their original state and scenario capacity");
}
void SourceKillVolumeView::initialize_storage() { state_[0x20] = std::byte{}; }
void SourceKillVolumeView::initialize_map() {
  std::fill_n(state_.begin(), 0x20, std::byte{});
  state_[0x20] = std::byte{1};
}
void SourceKillVolumeView::dispose_map() { state_[0x20] = std::byte{}; }
void SourceKillVolumeView::set_enabled(std::int16_t trigger, bool enabled) {
  if (trigger == -1) return;
  if (trigger < 0 || std::size_t(trigger) >= trigger_kill_indices_.size())
    throw std::invalid_argument("Kill-volume trigger is outside the actual scenario");
  const auto index = trigger_kill_indices_[trigger];
  if (index == -1) return;
  if (index < 0 || index >= 256)
    throw std::invalid_argument("Kill-volume bit is outside the original state");
  auto &cell = state_[unsigned(index) / 8];
  const auto mask = std::byte(1u << (unsigned(index) & 7));
  // The original word bits mark disabled volumes; enable clears the bit.
  if (enabled) cell &= ~mask;
  else cell |= mask;
}
bool SourceKillVolumeView::test_object(std::uint32_t object, std::int32_t tick,
                                     const ContainsObject &contains) const {
  if (kill_triggers_.empty()) return false;
  const auto total = std::bit_cast<std::int32_t>(std::uint32_t(tick) + (object & 0xffff));
  const auto index = total % std::int32_t(kill_triggers_.size());
  if (index < 0)
    throw std::invalid_argument("Kill-volume clock selects outside the original state");
  if ((state_[unsigned(index) / 8] & std::byte(1u << (unsigned(index) & 7))) != std::byte{})
    return false;
  const auto trigger = kill_triggers_[index];
  if (trigger == -1) return false;
  if (trigger < 0 || std::size_t(trigger) >= trigger_kill_indices_.size())
    throw std::invalid_argument("Scheduled kill volume is outside the actual scenario");
  if (!contains)
    throw std::invalid_argument("Kill-volume query requires the actual containment owner");
  // The original query does not inspect state+20. Preserve that distinction
  // from lifecycle admission; the caller owns its active-world boundary.
  return contains(object, trigger);
}
}
