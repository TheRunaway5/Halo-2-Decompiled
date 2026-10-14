#pragma once
#include "halo2/datum_pool.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
struct LevelContent;
// Canonical scenario import resolves the character/unit/general-type team
// fallback before construction. Starting-location names are authored SIDs,
// not placement ordinals or native object handles.
struct SourceAiSquadDefinition {
  std::uint32_t flags{};
  std::int16_t parent_group = -1;
  std::uint8_t resolved_team{};
  std::vector<std::uint32_t> starting_location_names;
};
// Base200B30/200C20/2009D0/205280: separate source squad/group banks and
// authored linked tree. Actor birth, joining and activation own later writes.
class SourceAiSquadBank {
public:
  SourceAiSquadBank(std::span<const SourceAiSquadDefinition>,
                    std::span<const std::int16_t> group_parents);
  const DatumPool &squads() const { return squads_; }
  const DatumPool &groups() const { return groups_; }
  // Recovered lifecycle/activation kernel boundary, borrowing this same bank.
  // Normal gameplay reaches records through the owning AI operations.
  DatumPool &source_squad_records() { return squads_; }
  std::span<const SourceAiSquadDefinition> definitions() const {
    return definitions_;
  }
private:
  DatumPool squads_, groups_;
  std::vector<SourceAiSquadDefinition> definitions_;
};
// Single adapter from the canonical scenario import; no second tag decoder.
SourceAiSquadBank source_ai_squad_bank(const LevelContent &);

// Borrowed, already-published owners. Checkpoints belong to the enclosing
// banks; this view stores no raw source/native addresses and owns no records.
struct SourceAiReferenceOwners {
  const DatumPool &actors, &squads, &groups;
  std::span<const SourceAiSquadDefinition> definitions;
  bool active;
};
// Base272B70/272D90/272E20 and204D30/70/DB0/E10. Packed references use
// actor-pool salts and real links. No unit-vitality or allocation-bitmap
// filter is added: even direct references read the source record's raw salt.
class SourceAiReferenceView {
public:
  explicit SourceAiReferenceView(SourceAiReferenceOwners);
  std::uint32_t actor(std::uint32_t packed_reference) const;
  std::vector<std::uint32_t> actors(std::uint32_t packed_reference) const;
private:
  SourceAiReferenceOwners owners_;
  std::vector<std::uint32_t> group_squads(std::uint32_t root) const;
  void append_squad_actors(std::uint32_t squad,
                           std::vector<std::uint32_t> &) const;
};
} // namespace halo2
