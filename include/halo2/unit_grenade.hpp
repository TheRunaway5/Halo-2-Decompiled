#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Source unit+signed346 component. Keep every original byte independently of
// the native player State; this module never invents a completed begin callback.
using SourceGrenadeAction = std::array<std::byte,0x44>;
void initialize_source_grenade_action(SourceGrenadeAction &);

enum class GrenadeCallback {
  complete, prepare_projectile, release_projectile, animation_query, unavailable
};
struct GrenadeCallbackStep {
  GrenadeCallback callback;
  std::optional<bool> returned;
  std::uint32_t animation_identifier{};
};
// E99B0: always increment the raw byte counter first; comparisons are signed.
// prepare/release/query are actual original callback requests, not successes.
GrenadeCallbackStep source_grenade_update(SourceGrenadeAction &);
// Exact phase3 query order. nullopt retains the unavailable provider boundary.
GrenadeCallbackStep source_grenade_animation_reply(SourceGrenadeAction &,
    std::uint32_t identifier,std::optional<bool> actually_active);
// E8F30's whole held-NONE path; a held object requires original world owners.
bool source_grenade_release_empty(SourceGrenadeAction &);

// E9896..E98C6 only, after actual channel selection and duration consumers have
// returned. No admission, feedback, scope, effect completion or active-bit store
// is inferred. The original effect request still prevents callback completion.
void source_grenade_begin_channel_prefix(SourceGrenadeAction &,
    std::uint8_t request_byte4,std::uint8_t actual_release_ticks);

struct GrenadePreparation {
  std::uint32_t unit_object_12c;
  std::int8_t current;
  std::span<std::uint8_t> counts;
  // Actual original matg block lookup. Unavailable is distinct from count0.
  std::optional<bool> definition_present;
};
enum class GrenadePreparationStop { complete, marker_request, unavailable };
struct GrenadePreparationResult {
  GrenadePreparationStop stop;
  bool count_debited;
  // If debited, A87E0(unit,400000) precedes B9C20(unit,09000100,1).
  std::uint32_t changed_flags{},marker_identifier{};
};
// E8D60 through the marker request. A positive count is debited before the
// record/creation gates, and is not refunded on a missing constructor result.
GrenadePreparationResult source_grenade_prepare_prefix(SourceGrenadeAction &,
    const GrenadePreparation &);

struct GrenadeDirectionConstants {float one,normalization_epsilon;};
// E8E69..E8ED3 after actual B8820 output and marker production. Preserve all
// other descriptor bytes, including externally owned location/scratch bytes.
// This returns a source constructor request, never an allocated/attached object.
void patch_source_grenade_creation_request(std::span<std::byte,0xc4>,
    const std::array<float,3> &unit_aim,const std::array<float,3> &marker_position,
    const GrenadeDirectionConstants &);
} // namespace halo2
