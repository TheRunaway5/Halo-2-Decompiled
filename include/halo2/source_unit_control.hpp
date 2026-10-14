#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
struct NativeUnitInteractionDefinition;
// Borrowed actual object bodies/header types and canonical unit definitions.
// Body/definition lifetimes must survive synchronous callbacks. The enclosing
// world owns storage, checkpoints and complete effect/actor creation services.
struct SourceUnitControlServices {
  std::function<std::span<std::uint8_t>(std::uint32_t)> object;
  std::function<std::uint8_t(std::uint32_t)> object_type;
  std::function<const NativeUnitInteractionDefinition &(std::uint32_t)> definition;
  std::function<void(std::uint32_t)> refresh_item_effect; // Base15E300.
  std::function<void(std::uint32_t)> driver_changed; // Base1BBDF0.
};
// BaseCBF60/10CCC0/CC810 control, held-item and recursive seat ownership.
// Mandatory downstream owners are invoked synchronously; no fallback/no-op
// implementation is supplied. This does not create source object/actor bodies.
class SourceUnitControl {
public:
  explicit SourceUnitControl(SourceUnitControlServices);
  void set_active(std::uint32_t unit, bool requested);
  void refresh_item(std::uint32_t item);
  void refresh_seats(std::uint32_t unit);
private:
  friend class SourceAiActorBank;
  friend class SourceAiActorLifecycle;
  friend class SourceAiActorControl;
  SourceUnitControlServices services_;
  std::span<std::uint8_t> object(std::uint32_t, unsigned minimum_size) const;
  void refresh_seats(std::uint32_t unit, std::vector<std::uint32_t> &parents);
};
} // namespace halo2
