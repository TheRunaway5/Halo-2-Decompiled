#pragma once
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace halo2 {
// Original19D220 initializes source variant+44 from the frontend engine enum.
inline std::optional<int> frontend_lobby_variant_type(unsigned engine) {
  constexpr std::array<int, 7> types{2, 3, 7, 4, 1, 9, 8};
  if (engine >= types.size()) return {};
  return types[engine];
}
// Original250A43, including its generic frame10 for unsupported source types.
inline int frontend_lobby_game_icon(int type) {
  switch (type) {
  case 1: return 6;
  case 2: return 0;
  case 3: return 3;
  case 4: return 1;
  case 7: return 4;
  case 8: return 8;
  case 9: return 7;
  default: return 10;
  }
}
inline std::optional<unsigned> frontend_lobby_quick_category(int type) {
  switch (type) {
  case 1: return 22;
  case 2: return 23;
  case 3: return 24;
  case 4: return 25;
  case 7: return 28;
  case 8: return 29;
  case 9: return 30;
  default: return {};
  }
}
// Original22DC2A consumes the signed16 difficulty query; unsupported values
// localize Normal. The caller expands E428 using the original global strings.
inline std::uint32_t frontend_lobby_difficulty_string(std::int16_t difficulty) {
  switch (difficulty) {
  case 0: return 0x0400028eu;
  case 2: return 0x0600028fu;
  case 3: return 0x09000290u;
  default: return 0x060000b8u;
  }
}
// Original19D220 ->2374F0, through the shared source buffer/descriptor path.
inline std::optional<int> frontend_lobby_parameter_default(unsigned engine,
                                                          unsigned parameter) {
  if (engine >= 7 || parameter >= 112) return {};
  return source_game_variant_get(source_game_variant_default(engine), int(parameter));
}
// Original25092C uses the first matching sily option and title+separator+value.
// Original2501C0 fills only the first five category entries; other authored rows
// remain empty. Current values are supplied by the caller's selected variant.
template<std::size_t Count>
inline std::array<std::optional<std::string>,Count> frontend_variant_quick_options(
    const FrontendContent &content, unsigned engine,
    const std::map<unsigned, int> &values, std::string_view separator) {
  std::array<std::optional<std::string>,Count> result;
  const auto type = frontend_lobby_variant_type(engine);
  const auto category = type ? frontend_lobby_quick_category(*type) : std::nullopt;
  if (!category) return result;
  const auto found = content.categories.find(*category);
  if (found == content.categories.end()) return result;
  for (unsigned row = 0; row < result.size() && row < found->second.size(); ++row) {
    const auto parameter = content.parameters.find(found->second[row]);
    if (parameter == content.parameters.end() || parameter->second.title.empty()) continue;
    const auto id = parameter->second.parameter;
    const auto current = values.find(id);
    const auto value = current != values.end() ? std::optional<int>{current->second}
        : frontend_lobby_parameter_default(engine, id);
    if (!value) continue;
    const auto choice = std::find_if(parameter->second.choices.begin(),
        parameter->second.choices.end(), [&](const UiChoice &item) { return item.value == *value; });
    result[row] = parameter->second.title + std::string(separator) +
        (choice != parameter->second.choices.end() ? choice->label : std::string{});
  }
  return result;
}
inline std::array<std::optional<std::string>,5> frontend_lobby_quick_options(
    const FrontendContent &content,unsigned engine,const std::map<unsigned,int>& values,std::string_view separator) {
  return frontend_variant_quick_options<5>(content,engine,values,separator);
}
// TU1.5 2580B2 uses the same256C3A title/value formatter for six rows.
inline std::array<std::optional<std::string>,6> frontend_matchmaking_quick_options(
    const FrontendContent &content,unsigned engine,const std::map<unsigned,int>& values,std::string_view separator) {
  return frontend_variant_quick_options<6>(content,engine,values,separator);
}
struct FrontendLobbyHelp {
  std::optional<unsigned> text_index;
  bool visible{};
  std::optional<unsigned> value_index;
  std::uint32_t string_id{};
};
// Original2508A8: protocol2 is online;199F34 supplies the machine count.
inline FrontendLobbyHelp frontend_lobby_help(int pane, int protocol,
    int machines, int privacy, int session) {
  FrontendLobbyHelp out;
  if (pane == 0) out.text_index = 46;
  else if (pane == 1) out.text_index = 41;
  else if (pane == 2) out.text_index = 47;
  else return out;
  out.visible = protocol == 2 && machines <= 1;
  if (!out.visible) return out;
  out.value_index = privacy == 2 ? 0 : privacy == 1 ? 1 : session == 6 ? 3 : 2;
  // Literal screen14 value block1; the caller resolves this ID in screen.strings.
  constexpr std::array<std::uint32_t, 4> strings{
      0x17001824u, 0x16001825u, 0x18001826u, 0x16001827u};
  out.string_id = strings[*out.value_index];
  return out;
}
// Original22DAD1 does not clear the cached display count on zero/negative input.
inline int frontend_lobby_player_count(int previous, int count_a, int count_b) {
  const auto count = std::min(16, std::max(count_a, count_b));
  return count > 0 ? count : previous;
}
inline std::optional<std::uint32_t> frontend_lobby_player_count_string(int count) {
  if (count < 0 || count > 16) return {};
  return count < 10 ? 0x0100030au + count : 0x02000314u + count - 10;
}
} // namespace halo2
