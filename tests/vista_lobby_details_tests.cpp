#include "halo2/vista_lobby_details.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
using Json = nlohmann::json;
namespace {
unsigned checks{};
constexpr VistaLobbyDetailsObject object = 0x1a00000, text = object + 0x9000,
    bitmap = object + 0x8000, definition = object + 0xa000, image = object + 0xb000;
void check(bool value, const std::string& reason) {
  ++checks;
  if (!value) throw FormatError(reason);
}
template<class F> void rejects(F&& f, const std::string& reason) {
  bool rejected{};
  try { f(); } catch (const FormatError&) { rejected = true; }
  check(rejected, reason);
}
std::string hex(std::uint32_t value) {
  std::ostringstream out;
  out << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
  return out.str();
}
std::string pointer(VistaLobbyDetailsObject value) {
  std::ostringstream out; out << "0x" << std::hex << value; return out.str();
}
std::u16string utf16(const std::string& value) {
  std::u16string result;
  for (std::size_t i = 0; i < value.size();) {
    const auto first = static_cast<unsigned char>(value[i++]);
    std::uint32_t code = first;
    unsigned trailing{};
    if (first >= 0xf0) { code &= 7; trailing = 3; }
    else if (first >= 0xe0) { code &= 15; trailing = 2; }
    else if (first >= 0xc0) { code &= 31; trailing = 1; }
    else if (first >= 0x80) throw FormatError("Malformed fixture UTF8");
    if (i + trailing > value.size()) throw FormatError("Truncated fixture UTF8");
    for (unsigned j = 0; j < trailing; ++j) {
      const auto next = static_cast<unsigned char>(value[i++]);
      if ((next & 0xc0) != 0x80) throw FormatError("Malformed fixture UTF8 continuation");
      code = (code << 6) | (next & 63);
    }
    if (code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
      throw FormatError("Invalid fixture Unicode scalar");
    if (code < 0x10000) result.push_back(char16_t(code));
    else {
      code -= 0x10000;
      result.push_back(char16_t(0xd800 + (code >> 10)));
      result.push_back(char16_t(0xdc00 + (code & 1023)));
    }
  }
  return result;
}
std::string utf8(std::u16string_view value) {
  std::string result;
  for (std::size_t i = 0; i < value.size(); ++i) {
    std::uint32_t code = value[i];
    if (code >= 0xd800 && code <= 0xdbff) {
      if (++i == value.size() || value[i] < 0xdc00 || value[i] > 0xdfff)
        throw FormatError("Malformed fixture UTF16");
      code = 0x10000 + ((code - 0xd800) << 10) + (value[i] - 0xdc00);
    } else if (code >= 0xdc00 && code <= 0xdfff) throw FormatError("Malformed fixture UTF16");
    if (code < 128) result.push_back(char(code));
    else {
      const unsigned count = code < 0x800 ? 2 : code < 0x10000 ? 3 : 4;
      result.push_back(char((count == 2 ? 0xc0 : count == 3 ? 0xe0 : 0xf0) | (code >> (6 * (count - 1)))));
      for (unsigned j = count - 1; j > 0; --j) result.push_back(char(0x80 | ((code >> (6 * (j - 1))) & 63)));
    }
  }
  return result;
}
std::vector<std::byte> bytes(const std::string& value) {
  std::vector<std::byte> result(value.size() / 2);
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = std::byte(std::stoul(value.substr(i * 2, 2), nullptr, 16));
  return result;
}
std::uint32_t word(std::span<const std::byte> data, std::size_t at) {
  std::uint32_t result{};
  for (unsigned i = 0; i < 4; ++i) result |= std::to_integer<std::uint32_t>(data[at + i]) << (i * 8);
  return result;
}
void put(std::span<std::byte> data, std::size_t at, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) data[at + i] = std::byte(value >> (i * 8));
}
struct Tags {
  std::map<std::uint32_t, VistaLobbyRuleParameter> parameters;
  VistaLobbyRuleCatalogue catalogue;
  VistaLobbyDetailsScreen screen;
  std::uint32_t screen_datum{};
  std::map<std::pair<std::uint32_t, std::uint32_t>, std::u16string> locale;
  explicit Tags(const Json& assets) {
    for (const auto& group : assets.at("rules").at(0).at("groups")) {
      VistaLobbyRuleGroup target;
      target.key = group.at("key");
      for (const auto& source : group.at("options")) {
        const auto datum = std::stoul(source.at("owner").at("datum").get<std::string>(), nullptr, 16);
        const auto metadata = bytes(source.at("owner").at("metadata").at("raw"));
        auto& parameter = parameters[std::uint32_t(datum)];
        parameter.datum = std::uint32_t(datum);
        parameter.parameter = source.at("parameter");
        parameter.locale = word(metadata, 12);
        parameter.title = word(metadata, 16);
        locale[{parameter.locale, parameter.title}] = utf16(source.at("title").get<std::string>());
        parameter.choices.clear();
        for (const auto& choice : source.at("choices")) {
          const auto sid = std::uint32_t(std::stoul(choice.at("string").get<std::string>(), nullptr, 16));
          parameter.choices.push_back({choice.at("value"), sid});
          locale[{parameter.locale, sid}] = utf16(choice.at("label").get<std::string>());
        }
        target.parameters.push_back(&parameter);
      }
      catalogue.groups.push_back(std::move(target));
    }
    for (const auto& source : assets.at("screens")) if (source.at("id") == 14) {
      const auto metadata = bytes(source.at("owner").at("metadata").at("raw"));
      screen_datum = std::uint32_t(std::stoul(source.at("owner").at("datum").get<std::string>(), nullptr, 16));
      screen.locale = word(metadata, 28);
      for (const auto& group : source.at("values")) {
        std::vector<std::uint32_t> values;
        for (const auto& entry : group.at("strings")) {
          const auto sid = std::uint32_t(std::stoul(entry.at("id").get<std::string>(), nullptr, 16));
          values.push_back(sid);
          if (!entry.at("label").is_null()) locale[{screen.locale, sid}] = utf16(entry.at("label").get<std::string>());
        }
        screen.values.push_back(std::move(values));
      }
    }
  }
};
void original(const Json& row, Tags& tags, unsigned ordinal) {
  const auto& q = row.at("input");
  const auto op = q.at("op").get<std::string>();
  const auto number = [&](const char* key, std::int64_t fallback = 0) { return q.value(key, fallback); };
  const auto byte = [&](const char* key, std::uint8_t fallback = 1) { return q.value(key, fallback); };
  Json events = Json::array();
  std::array<std::uint8_t, 52> visibility; visibility.fill(byte("initial", 0));
  std::uint8_t bitmap_visibility = byte("initial", 0);
  std::array<std::byte, 0x4c> variant{};
  put(variant, 0x44, std::uint32_t(number("engine", 2)));
  put(variant, 0x48, std::uint32_t(number("flags", 0)));
  std::array<std::byte, 0xa80> record{};
  for (unsigned i = 0; i < record.size(); ++i) record[i] = std::byte((i * 37 + unsigned(number("seed")) * 19) & 255);
  VistaLobbyDetailsServices s;
  s.text = [&](auto index) {
    const auto exists = q.contains("text_mask") ? std::uint8_t((q.at("text_mask").get<std::uint64_t>() >> index) & 1) : byte("texts");
    events.push_back({"text_lookup", index, exists}); return exists ? text + index * 0x100 : 0;
  };
  s.bitmap = [&](auto index) { events.push_back({"bitmap_lookup", index, byte("bitmap")}); return byte("bitmap") ? bitmap : 0; };
  s.visible = [&](auto child, auto value) {
    if (child == bitmap) bitmap_visibility = value;
    else { check(child >= text && (child - text) / 0x100 < visibility.size(), "Source text visibility owner"); visibility[(child - text) / 0x100] = value; }
  };
  s.string_id = [&](auto child, auto sid) { events.push_back({"string_id", (child - text) / 0x100, hex(sid)}); };
  s.string = [&](auto child, auto value) { events.push_back({"text", (child - text) / 0x100, utf8(value)}); };
  s.frame = [&](auto child, auto frame) { check(child == bitmap, "Source icon bitmap owner"); events.push_back({"bitmap_frame", frame}); };
  s.screen_tag = [&](auto id) { events.push_back({"screen_tag", id, byte("descriptor")}); return byte("descriptor") ? tags.screen_datum : 0xffffffffu; };
  s.screen_metadata = [&](auto datum) { check(datum == tags.screen_datum, "Source owning wgit datum"); return &tags.screen; };
  s.catalogue = [&] { events.push_back({"actual_ui_globals"}); return byte("globals") && byte("goof") ? &tags.catalogue : nullptr; };
  s.parameter = [&](auto raw, auto parameter) {
    check(word(raw, 0x44) == word(variant, 0x44) && word(raw, 0x48) == word(variant, 0x48), "Actual selected variant fields retained");
    const auto value = q.contains("values") ? q.at("values").value(std::to_string(parameter), number("value")) : number("value");
    events.push_back({"variant_parameter", parameter, value}); return std::int32_t(value);
  };
  s.localize = [&](auto owner, auto sid) {
    auto value = tags.locale[{owner, sid}];
    if (byte("empty_title", 0) && owner != tags.screen.locale)
      for (const auto& [datum, parameter] : tags.parameters) if (parameter.title == sid) { value.clear(); break; }
    events.push_back({"localize", hex(owner), hex(sid), utf8(value)}); return value;
  };
  s.format = [&](auto title, auto separator, auto value) {
    events.push_back({"format", "%s%s%s", utf8(title), utf8(separator), utf8(value)});
    return std::u16string(title) + std::u16string(separator) + std::u16string(value);
  };
  s.status_visibility = [&] { events.push_back({"service40B8C6", byte("visible")}); return byte("visible"); };
  s.map_query = [&](auto& map) {
    map = {std::int32_t(number("campaign_id", 33)), std::int32_t(number("map_id", 177)), byte("custom", 0) ? definition : 0};
    events.push_back({"map_query", byte("map_valid"), std::uint32_t(map.campaign_id), std::uint32_t(map.map_id), byte("custom", 0)}); return byte("map_valid");
  };
  s.campaign = [&] { events.push_back({"campaign", byte("campaign", 0)}); return byte("campaign", 0); };
  s.campaign_definition = [&](auto campaign, auto map) -> std::optional<std::uint32_t> {
    events.push_back({"campaign_definition", std::uint32_t(campaign), std::uint32_t(map), byte("definition")});
    if (!byte("definition")) return {};
    return byte("ref") ? 0x12345678u : 0;
  };
  s.multiplayer_definition = [&](auto map) -> std::optional<std::uint32_t> {
    events.push_back({"multiplayer_definition", std::uint32_t(map), byte("definition")});
    if (!byte("definition")) return {};
    return byte("ref") ? 0x12345678u : 0;
  };
  s.resolve_image = [&](auto reference, auto frame) { events.push_back({"resolve_image", reference, frame}); return byte("image") ? image : 0; };
  s.image = [&](auto child, auto source) { check(child == bitmap, "Actual source map image child"); events.push_back({"image", source == image, pointer(source)}); };
  s.identifier_record = [&](auto id, auto& result) {
    events.push_back({"identifier_lookup", id == definition, byte("custom_valid")}); result = record; return byte("custom_valid");
  };
  s.identifier_provider = [&] { events.push_back({"provider61AA42", byte("provider", 0)}); return byte("provider", 0); };
  VistaLobbyMapInfo info; info.fill(std::byte{0x5a}); bool returned{};
  if (op == "lobby_details") {
    VistaLobbyDetailsIndices indices;
    indices.icon = std::uint32_t(number("icon", 4)); indices.value_group = std::int32_t(number("value_group")); indices.value_index = std::int32_t(number("value_index"));
    vista_lobby_details(14, byte("variant") ? std::span<const std::byte>(variant) : std::span<const std::byte>{}, indices, s);
  } else if (op == "quick_row") vista_lobby_quick_detail(variant, byte("texts") ? text : 0, std::int32_t(number("index")), u": ", s);
  else if (op == "game_icon") vista_lobby_details_game_icon(variant, byte("bitmap") ? bitmap : 0, s);
  else if (op == "map") vista_lobby_map_bitmap(s);
  else if (op == "map_info") returned = vista_lobby_map_info(definition, info, s);
  else throw FormatError("Unexpected original details operation");
  const auto why = "Original details case " + std::to_string(ordinal) + " " + q.dump();
  ++checks; if (events != row.at("events")) throw FormatError(why + " calls: " + Json::diff(row.at("events"), events).dump());
  check(Json(visibility) == row.at("text_visibility"), why + " retained text visibility");
  check(bitmap_visibility == row.at("bitmap_visibility"), why + " retained bitmap visibility");
  if (op == "map_info") {
    const auto expected = bytes(row.at("info_hex"));
    check(std::equal(info.begin(), info.end(), expected.begin()), why + " complete map metadata output");
    check(returned == (row.at("returned").get<unsigned>() != 0), why + " original map metadata AL");
    check(word(info, 0) == 0, why + " source never populates preview dword");
  }
}
}
int main(int argc, char** argv) try {
  if (argc != 2) return 77;
  std::ifstream input(argv[1]); check(bool(input), "Original fixture exists");
  const auto fixture = Json::parse(input);
  check(fixture.at("schema") == "H2V_PC14_DETAILS_ORIGINAL_V1", "Original fixture schema");
  check(fixture.at("source_sha256") == "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109", "Original executable pin");
  check(fixture.at("map_sha256") == "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44", "Original tag pin");
  check(fixture.at("source_spans").at("generic_details").at("sha256") == "eff9a4869a35c67ae4708049517be0ec9680620e2dc6b278b0a295e28ccf9bd4", "Original full611243 span");
  check(fixture.at("cases").size() == 1532, "Complete original details/map corpus");
  Tags tags(fixture.at("assets"));
  unsigned ordinal{};
  for (const auto& row : fixture.at("cases")) original(row, tags, ordinal++);
  // A duplicate numeric SID belongs to its actual unic range.
  check(tags.locale.at({0xe6c204a0, 0x0100030a}) == u"Unlimited", "Owner-qualified Slayer score locale");
  check(tags.locale.at({0xe70c04ea, 0x0100030a}) == u"Off", "Same SID preserves a different source owner's choice");
  rejects([&] { vista_lobby_map_bitmap({}); }, "Missing consumed map query accepted");
  std::array<std::byte, 0x4c> variant{};
  rejects([&] { vista_lobby_quick_detail(variant, 0, 0, u": ", {}); }, "Missing consumed source tag globals accepted");
  rejects([&] { vista_lobby_details_game_icon(std::span(variant).first(0x47), 0, {}); }, "Missing child bypassed actual variant field read");
  VistaLobbyMapInfo info; VistaLobbyDetailsServices services;
  services.identifier_record = [](auto, auto& record) { record = {}; return std::uint8_t{1}; };
  rejects([&] { vista_lobby_map_info(definition, info, services); }, "Successful null original identifier record accepted");
  std::cout << "PASS " << checks << " checks, " << ordinal << " original details/map cases; no live frontend or pixel claim\n";
  return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
