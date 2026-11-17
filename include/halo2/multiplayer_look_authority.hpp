#pragma once

namespace halo2 {
// Automatically selected Worlds and native replicas own processed look just
// like an explicitly requested source World. Keep raw input unconstrained so
// the original look controller consumes its complete current-minus-previous
// delta; reference adapters retain their separate host pitch limit.
constexpr bool multiplayer_source_look_authority(bool requested_source_world,
                                                bool active_source_relative_look) {
  return requested_source_world || active_source_relative_look;
}
} // namespace halo2
