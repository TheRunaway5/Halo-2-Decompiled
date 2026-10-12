#include "halo2/frontend.hpp"
#include "halo2/vista_lobby_metadata.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json = nlohmann::json;
namespace {
unsigned checks{};
void check(bool value, const std::string& reason) {
  ++checks; if (!value) throw FormatError(reason);
}
std::uint32_t id(const Json& value) { return std::stoul(value.get<std::string>(), nullptr, 16); }
std::u16string utf16(const std::string& value) {
  // Independent conversion for the source fixture's valid Unicode strings.
  std::u16string out;
  for (std::size_t i = 0; i < value.size();) {
    auto c = static_cast<unsigned char>(value[i++]);
    unsigned code = c, count = 0;
    if (c >= 0xf0) { code = c & 7u; count = 3; }
    else if (c >= 0xe0) { code = c & 15u; count = 2; }
    else if (c >= 0xc0) { code = c & 31u; count = 1; }
    while (count--) code = (code << 6) | (static_cast<unsigned char>(value.at(i++)) & 63u);
    if (code < 0x10000) out.push_back(static_cast<char16_t>(code));
    else { code -= 0x10000; out.push_back(static_cast<char16_t>(0xd800 | (code >> 10))); out.push_back(static_cast<char16_t>(0xdc00 | (code & 1023))); }
  }
  return out;
}
std::string sha(std::span<const std::byte> data) {
  unsigned char result[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), result);
  constexpr char digits[] = "0123456789abcdef";
  std::string text; for (const auto b : result) { text += digits[b >> 4]; text += digits[b & 15]; }
  return text;
}
void source(const VistaLobbyMetadataBytes& actual, const Json& expected) {
  check(actual.file_offset == expected.at("file_offset"), "raw record source offset");
  check(actual.bytes.size() == expected.at("size"), "raw record source extent");
  check(sha(actual.bytes) == expected.at("sha256").get<std::string>(), "raw record source bytes");
}
void tag(const VistaLobbyMetadataTag& actual, const Json& expected) {
  check(actual.datum == id(expected.at("datum")) && actual.group == expected.at("group").get<std::string>() &&
      actual.name == expected.at("name").get<std::string>(), "source-local tag identity");
  source(actual.source, expected.at("metadata"));
}
void maps(const std::vector<VistaLobbyMetadataMap>& actual, const Json& expected, bool campaign) {
  check(actual.size() == expected.size(), "complete raw map slots");
  for (std::size_t i = 0; i < actual.size(); ++i) {
    const auto& a = actual[i]; const auto& e = expected.at(i);
    source(a.source, e.at("metadata"));
    check(a.map_id == e.at("map_id") && a.bitmap == id(e.at("preview").at("datum")), "map ID/preview owner");
    check(a.english_name == utf16(e.at("name")) && a.english_description == utf16(e.at("description")), "map UTF16 source labels");
    check(a.scenario_path == (e.at("scenario_path").is_null() ? "" : e.at("scenario_path").get<std::string>()), "map source path");
    if (campaign) check(a.campaign_id == e.at("campaign_id"), "campaign source ID");
    else {
      check(a.campaign_id == -1 && a.sort_order == e.at("sort_order") && a.flags == e.at("flags"), "MP source fields");
      check(Json(a.team_counts) == e.at("team_counts"), "MP sixteen raw source bytes");
    }
  }
}
}
int main(int argc, char** argv) try {
  if (argc != 3 || !std::filesystem::is_regular_file(argv[1])) return 77;
  std::ifstream input(argv[2]); check(bool(input), "missing original metadata fixture");
  const auto fixture = Json::parse(input);
  check(fixture.at("schema") == "H2V_LOBBY_METADATA_SOURCE_V1", "fixture schema");
  std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
  check(bool(file), "missing original map");
  std::vector<std::byte> data(static_cast<std::size_t>(file.tellg())); file.seekg(0);
  file.read(reinterpret_cast<char*>(data.data()), data.size()); check(bool(file), "original map read");
  check(sha(data) == fixture.at("source_map_sha256").get<std::string>(), "original Vista map pin");
  const auto content = load_vista_settings(argv[1]);
  check(bool(content->vista_lobby_metadata), "actual native importer retains PC metadata");
  const auto& bank = *content->vista_lobby_metadata;
  const auto& m = fixture.at("maps"); const auto& r = fixture.at("rules");
  tag(bank.globals, m.at("owner"));
  maps(bank.campaign, m.at("campaign"), true); maps(bank.multiplayer, m.at("multiplayer"), false);
  check(bank.level_headers.size() == m.at("level_headers").size() && bank.path_headers.size() == m.at("path_headers").size(), "source header order/count");
  for (std::size_t i = 0; i < bank.level_headers.size(); ++i) source(bank.level_headers[i], m.at("level_headers").at(i));
  for (std::size_t i = 0; i < bank.path_headers.size(); ++i) source(bank.path_headers[i], m.at("path_headers").at(i));
  check(bank.paths.size() == m.at("campaign_paths").size(), "campaign path source count");
  for (std::size_t i = 0; i < bank.paths.size(); ++i) {
    const auto& a = bank.paths[i]; const auto& e = m.at("campaign_paths").at(i);
    source(a.source, e.at("metadata"));
    check(a.campaign_id == e.at("campaign_id") && a.map_id == e.at("map_id") &&
        a.scenario_path == e.at("scenario_path").get<std::string>(), "campaign paired path identity");
    const auto first = std::find_if(bank.paths.begin(), bank.paths.end(), [&](const auto& p) { return p.campaign_id == a.campaign_id && p.map_id == a.map_id; });
    check(bank.campaign_path(a.campaign_id, a.map_id) == &*first, "campaign path original first match");
  }
  tag(bank.ui_globals, r.at("wgtz").at("owner")); tag(bank.scenario, r.at("wgtz").at("scenario"));
  check(bank.scenario_type == 2 && bank.shared_ui_globals == id(r.at("wgtz").at("shared_globals").at("datum")), "scenario-selected wgtz owner");
  const auto& catalogue = r.at("catalogues").at(0);
  tag(bank.game_settings, catalogue.at("owner"));
  check(bank.quick_settings_screen == (catalogue.at("quick_screen").is_null() ? 0xffffffffu : id(catalogue.at("quick_screen").at("datum"))) &&
      bank.variant_locale == id(catalogue.at("locale")), "goof source owners");
  check(bank.parameters.size() == r.at("parameters").size(), "all sily records");
  for (const auto& [key, e] : r.at("parameters").items()) {
    const auto& a = bank.parameters.at(std::stoul(key, nullptr, 16)); tag(a.owner, e.at("owner"));
    check(a.details.datum == a.owner.datum && a.details.parameter == e.at("parameter") &&
        a.default_value == e.at("default_value") && a.details.locale == id(e.at("locale")) &&
        a.details.title == id(e.at("title_sid")) && a.header == id(e.at("header_sid")) &&
        a.description == id(e.at("description_sid")), "sily source ID/default/locale/SIDs");
    check(a.details.choices.size() == e.at("choices").size(), "ordered sily choices");
    for (std::size_t i = 0; i < a.details.choices.size(); ++i) {
      const auto& q = e.at("choices").at(i); source(a.choice_records.at(i), q.at("metadata"));
      check(a.details.choices[i].value == q.at("value") && a.details.choices[i].string == id(q.at("string")), "sily value/SID");
    }
  }
  check(bank.groups.size() == catalogue.at("groups").size(), "goof source group count");
  for (std::size_t i = 0; i < bank.groups.size(); ++i) {
    const auto& a = bank.groups[i]; const auto& e = catalogue.at("groups").at(i); source(a.source, e.at("metadata"));
    check(a.key == e.at("key") && a.parameters.size() == e.at("parameters").size(), "goof group key/order");
    for (std::size_t j = 0; j < a.parameters.size(); ++j) {
      source(a.reference_records.at(j), e.at("parameters").at(j).at("reference"));
      check(a.parameters[j] == id(e.at("parameters").at(j).at("datum")), "goof ordered sily reference");
    }
  }
  check(bank.variants.size() == catalogue.at("variants").size(), "complete raw authored variant bank");
  for (std::size_t i = 0; i < bank.variants.size(); ++i) {
    const auto& a = bank.variants[i]; const auto& e = catalogue.at("variants").at(i); source(a.source, e.at("metadata"));
    check(a.name == id(e.at("name_sid")) && a.engine == e.at("engine") && a.values.size() == e.at("values").size(), "raw variant name/engine/values");
    for (std::size_t j = 0; j < a.values.size(); ++j) {
      const auto& q = e.at("values").at(j); source(a.value_records.at(j), q.at("metadata"));
      check(a.values[j].first == q.at("parameter") && a.values[j].second == q.at("value"), "raw variant ordered parameter/value");
    }
  }
  for (const auto& [key, e] : r.at("locales").items()) {
    const auto& a = bank.locales.at(std::stoul(key, nullptr, 16)); tag(a.owner, e.at("owner"));
    check(a.first == e.at("first") && a.count == e.at("count"), "owning unic range");
    for (const auto& entry : e.at("entries")) {
      const auto& value = bank.locale_entries.at(entry.at("ordinal").get<std::size_t>());
      check(value.string == id(entry.at("string")) && value.utf8 == entry.at("text").get<std::string>() &&
          value.utf16 == utf16(entry.at("text")), "exact source UTF8/UTF16 locale row");
    }
  }
  for (const auto& e : r.at("value_blocks")) {
    const auto& a = bank.screens.at(id(e.at("owner").at("datum"))); tag(a.owner, e.at("owner"));
    check(a.id == e.at("screen") && a.details.locale == id(e.at("locale")) && a.details.values.size() == e.at("blocks").size(), "owning wgit value bank");
    for (std::size_t i = 0; i < a.details.values.size(); ++i) {
      const auto& q = e.at("blocks").at(i); source(a.value_records.at(i), q.at("metadata"));
      check(a.value_names[i] == id(q.at("name_sid")) && a.details.values[i].size() == q.at("values").size(), "wgit value group order");
      for (std::size_t j = 0; j < a.details.values[i].size(); ++j) {
        source(a.string_records.at(i).at(j), q.at("values").at(j).at("metadata"));
        check(a.details.values[i][j] == id(q.at("values").at(j).at("string")), "wgit value SID order");
      }
    }
  }
  check(bank.preview_datums.size() == fixture.at("previews").size(), "complete preview reference bank");
  for (const auto& [key, e] : fixture.at("previews").items()) {
    const auto datum = std::stoul(key, nullptr, 16); tag(bank.preview_tags.at(datum), e.at("owner"));
    const auto& image = content->images.at(datum); check(image.name == e.at("owner").at("name").get<std::string>() &&
        image.images.size() == e.at("images").size(), "native source preview image bank");
    for (std::size_t i = 0; i < image.images.size(); ++i) {
      const auto& a = image.images[i]; const auto& q = e.at("images").at(i);
      check(a.width == q.at("width") && a.height == q.at("height") && a.format == 11 &&
          a.mips.size() == q.at("additional_mips").get<unsigned>() + 1, "native preview dimensions/mips");
      auto bgra = a.pixels; for (std::size_t p = 0; p < bgra.size(); p += 4) std::swap(bgra[p], bgra[p + 2]);
      check(bgra.size() == q.at("expanded_bytes") && sha(bgra) == q.at("pixels_sha256").get<std::string>(), "native preview retains every source channel byte");
    }
  }
  // Catalogue backing is reconstructed from the destination bank after copies
  // and moves, and the adapter owns it even after callers release their bank.
  auto copy = std::make_shared<VistaLobbyMetadata>(bank);
  auto moved = std::make_shared<VistaLobbyMetadata>(std::move(*copy)); copy.reset();
  auto services = vista_lobby_metadata_services(moved); moved.reset();
  check(services.catalogue()->groups.size() == 31 && services.catalogue()->groups[0].parameters[0]->datum == bank.groups[0].parameters[0], "retained adapter catalogue lifetime");
  check(services.multiplayer_definition(50) == 0xe78b0567u && !services.multiplayer_definition(17) &&
      services.multiplayer_definition(-1) == bank.multiplayer[23].bitmap, "original MP lookup includes NONE slot");
  check(services.campaign_definition(999, 105) == bank.campaign[2].bitmap, "original campaign lookup ignores campaign argument");
  check(services.localize(0xe6c204a0, 0x0100030a) == u"Unlimited" &&
      services.localize(0xe70c04ea, 0x0100030a) == u"Off" &&
      services.localize(0xe5cb03a9, 0x1a001a90) == u": " && services.localize(0xe5cb03a9, 0x7fffffffu).empty(), "source-local SID collision/separator/miss");
  check(!services.map_query && !services.campaign && !services.resolve_image && !services.screen_tag &&
      !services.text && !services.parameter && !services.identifier_record, "adapter leaves unowned runtime services explicit");
  check(content->campaign_maps.empty() && content->multiplayer_maps.empty(), "metadata import does not switch existing menu selection policy");
  for (const auto& text : {std::string("\xc0\x80"), std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80"), std::string("\xe1\x80")}) {
    bool rejected{}; try { (void)vista_lobby_metadata_utf16(text); } catch (const FormatError&) { rejected = true; }
    check(rejected, "invalid source UTF8 rejected");
  }
  std::cout << "PASS Vista lobby metadata: " << checks << " checks, 20/50 raw maps, 70 source preview banks, 31 groups/92 parameters/39 variants\n";
  return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
