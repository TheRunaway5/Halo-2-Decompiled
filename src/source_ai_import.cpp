#include "halo2/content.hpp"
#include "halo2/source_ai_references.hpp"

namespace halo2 {
SourceAiSquadBank source_ai_squad_bank(const LevelContent &content) {
  std::vector<SourceAiSquadDefinition> definitions;
  definitions.reserve(content.squads.size());
  for (const auto &squad : content.squads) {
    SourceAiSquadDefinition definition;
    definition.flags = squad.flags;
    definition.parent_group = squad.parent_group;
    definition.resolved_team = std::uint8_t(squad.team);
    definition.starting_location_names.reserve(squad.starts.size());
    for (const auto &start : squad.starts)
      definition.starting_location_names.push_back(start.starting_location_name);
    definitions.push_back(std::move(definition));
  }
  return {definitions, content.squad_group_parents};
}
} // namespace halo2
