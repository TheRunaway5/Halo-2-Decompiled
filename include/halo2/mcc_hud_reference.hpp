#pragma once
#include "halo2/mcc_gameplay_reference.hpp"

namespace halo2 {
struct MccHudReferenceAccess {
  // The caller proves each complete view belongs to retained cache metadata.
  // Views and the directory must remain stable for this call.
  std::function<std::span<const std::byte>(std::uint32_t, std::size_t)> read;
  std::function<std::optional<Tag>(TagHandle)> resolve;
  std::function<std::string(std::uint32_t)> string_id;
  std::span<const Tag> directory;
};
struct MccOwnedHudProjection {
  MccGameplayReferenceStatus status{
      MccGameplayReferenceStatus::AuthoredTupleUnrecognized};
  std::vector<std::byte> bytes;
  // Each pair is {field byte offset within bytes, target offset within bytes}.
  // The stored word equals target (relative to the new root). The publisher
  // relocates every pair to new_root+target before publishing the typed tag.
  std::vector<std::pair<std::uint32_t, std::uint32_t>> pointers;
};
// Complete canonical source and closure guards precede construction. The result
// owns all active rows/effects/curves; source banks, including shared curves,
// are never modified. Unknown or partial authoring is not projected.
std::optional<MccOwnedHudProjection>
project_mcc_scoreboard_hud_reference(const Tag &,
                                     const MccHudReferenceAccess &);
// Includes the unchanged scoreboard policy and finite complete canonical HUD
// geometry owners. Unknown/modded tuples remain unmodified.
std::optional<MccOwnedHudProjection>
project_mcc_hud_reference(const Tag &, const MccHudReferenceAccess &);
// Detached native HUD values use the existing Original callback operands.
// The actual cache StringID namespace and raw metadata words stay intact.
std::uint32_t normalize_mcc_hud_function_operand(std::uint32_t,
                                                 std::string_view);
} // namespace halo2
