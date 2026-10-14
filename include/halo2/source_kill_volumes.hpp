#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Borrow the original 0x24-byte game-state record and decoded scenario indices.
// Rebind this view after replacing the bank or its scenario. No state is copied.
class SourceKillVolumeView {
public:
  using ContainsObject = std::function<bool(std::uint32_t, std::int16_t)>;
  SourceKillVolumeView(std::span<std::byte> state,
                       std::span<const std::int16_t> trigger_kill_indices,
                       std::span<const std::int16_t> kill_triggers);
  void initialize_storage();
  void initialize_map();
  void dispose_map();
  void set_enabled(std::int16_t trigger, bool enabled);
  // Original 1EC500 / TU1.5 1F47E0: one scheduled volume, followed by the
  // actual object-containment owner. This does not apply fatal damage.
  bool test_object(std::uint32_t object, std::int32_t tick,
                   const ContainsObject &) const;

private:
  std::span<std::byte> state_;
  std::span<const std::int16_t> trigger_kill_indices_, kill_triggers_;
};
}
