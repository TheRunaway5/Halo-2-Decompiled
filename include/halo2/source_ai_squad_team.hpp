#pragma once
#include <cstdint>

namespace halo2 {
// Base200C20. The canonical tag importer supplies the direct char+10 unit's
// team byte, or char+18 creature's byte only when char+10 is NONE. General
// type is the first inherited nonempty block; absent/invalid types use -1.
constexpr std::uint8_t source_ai_squad_team(std::uint8_t authored_team,
                                           std::uint8_t local_unit_team,
                                           std::int16_t general_type) {
  if (authored_team)
    return authored_team;
  if (local_unit_team)
    return local_unit_team;
  constexpr std::int8_t teams[]{3, 7, 3, 3, 3, 3, 1, 2, 2, 4,
                                4, 4, 5, 5, -1, 3, 7, 7, 7, 4};
  return general_type >= 0 && general_type < 20
             ? std::uint8_t(teams[general_type]) : 0;
}
} // namespace halo2
