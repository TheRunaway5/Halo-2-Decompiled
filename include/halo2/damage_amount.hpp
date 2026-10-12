#pragma once
#include <cstdint>
namespace halo2 {
struct DamageAmountDefinition {
  float lower{}, upper_minimum{}, upper_maximum{};
};
// TU DB160 after upper-bound selection, with the original separate SSE stores.
// Random upper-bound selection and 15F520's player/variant gates are callers.
float damage_amount(float lower, float selected_upper, float fraction,
                    float event_scale, float multiplayer_scale);
// TU 15F520's raw player+C1 branch, after its identity/engine admission gates.
// Session property sanitization is a separate producer.
float damage_strength_scale(std::uint8_t strength);
}
