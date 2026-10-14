#pragma once
#include <cstdint>
#include <functional>
#include <optional>

namespace halo2 {
// Internal seam for the original ai_place overloads. The world supplies whole
// original actions and a write to its actual PVS pending-reference field.
struct SourceAiPlacementOwners {
  std::function<void(std::uint32_t)> place_reference;
  std::function<void(std::int16_t, std::int16_t)> place_squad;
  std::function<void(std::uint32_t)> remember_reference;
};
void source_place_ai(std::uint32_t reference, std::optional<std::int16_t> count,
                     const SourceAiPlacementOwners &);
}
