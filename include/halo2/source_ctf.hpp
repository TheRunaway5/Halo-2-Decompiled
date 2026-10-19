#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Original scenario+118/+11C rows, in their authored order. Unknown fields,
// flags and float payloads survive; converted MCC metadata supplies these bytes.
struct SourceCtfMarker {
  std::array<std::uint8_t,0x20> bytes{};
};
static_assert(sizeof(SourceCtfMarker)==0x20);

// Entire shared CTF/Assault engine bank at engine-globals+FC. Source type1/9
// uses this same layout; its entity identifier1 is a different numbering domain.
struct SourceCtfState {
  std::array<std::uint8_t,0x208> bytes{};
};
static_assert(sizeof(SourceCtfState)==0x208);
struct SourceCtfContext {
  std::int32_t source_engine=1; // Original game-options+180; 9 selects Assault.
  std::int32_t objective_kind{}; // Original game-options+240, not a UI enum.
};
struct SourceCtfMarkerQuery {
  std::int16_t type=-1,team=-1,channel=-1;
  std::optional<std::array<float,3>> point;
  float maximum_distance{},maximum_vertical_distance{};
};

// Whole retail19EC40 / TU1.1 1A25A0 / TU1.5 1A8850. NONE filters are wildcards.
// Returns stored indices, limited by output capacity, preserving all other
// output words. The original one-slot nearest replacement starts at distance0;
// it must not be "fixed" into a conventional nearest-marker search.
std::int32_t source_ctf_query_markers(std::span<const SourceCtfMarker>,
    const SourceCtfMarkerQuery&,std::span<std::int32_t> output);

// Whole243B10/245C20/24CE00. The source admits only the first8 query matches;
// variant flags+16 then select the first two eligible rows. The optional second
// output is untouched unless an actual second eligible row exists.
std::int32_t source_ctf_select_marker(std::span<const SourceCtfMarker>,
    const SourceCtfContext&,std::int32_t team,bool spawn,std::int32_t* second=nullptr);

// Whole243C00/245D10/24CEF0. Reads the selected signed16 primary/secondary
// marker indices from the bank and changes only each available region's three
// float words. Missing indices preserve that region byte-for-byte.
void source_ctf_build_capture_regions(SourceCtfState&,
    std::span<const SourceCtfMarker>,const SourceCtfContext&,std::int32_t team);

// Whole240160/242270/249460 initializer and complete marker/region dependency
// closure. Clears exactly208 bytes, installs every original sentinel, and visits
// teams0..8 before the final teamNONE selection. Returns the defined original AL.
// The typed state object owns the bank; publishing a source virtual pointer and
// binding an actual selected engine/world remain separate runtime owners.
bool source_ctf_initialize(SourceCtfState&,std::span<const SourceCtfMarker>,
    const SourceCtfContext&);
} // namespace halo2
