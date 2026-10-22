#include "halo2/source_ai_placement.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
void source_place_ai(std::uint32_t reference, std::optional<std::int16_t> count,
                     const SourceAiPlacementOwners &owners) {
  if (!count) {
    if (!owners.place_reference)
      throw std::invalid_argument(
          "AI placement requires its original reference action");
    owners.place_reference(reference);
    return;
  }
  if (reference == 0xffffffffu)
    return;
  if ((reference & 0xc0000000u) == 0 && *count > 0) {
    if (!owners.place_squad)
      throw std::invalid_argument(
          "Counted AI placement requires its original squad action");
    owners.place_squad(std::bit_cast<std::int16_t>(std::uint16_t(reference)),
                       *count);
  }
  // TU1.5 wrapper 2AED30 records every non-NONE reference after the action,
  // including packed references and nonpositive counts. A failed action must
  // propagate before this write, exactly as in the original call sequence.
  if (!owners.remember_reference)
    throw std::invalid_argument(
        "AI placement requires its actual pending-reference owner");
  owners.remember_reference(reference);
}
} // namespace halo2
