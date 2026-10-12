#include "halo2/vista_lobby_map_commit.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace halo2;
using Json = nlohmann::json;
namespace {
std::string hex(std::span<const std::byte> data) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (auto value : data) {
    const auto v = std::to_integer<unsigned>(value);
    result += digits[v >> 4]; result += digits[v & 15];
  }
  return result;
}
std::u16string utf16(const std::string& text) {
  std::u16string result;
  for (std::size_t i = 0; i < text.size();) {
    unsigned value = static_cast<unsigned char>(text[i++]);
    if (value >= 0xc0 && value < 0xe0 && i < text.size()) {
      value = (value & 31u) << 6;
      value |= static_cast<unsigned char>(text[i++]) & 63u;
    } else if (value >= 0x80) throw FormatError("Unsupported fixture UTF8");
    result.push_back(char16_t(value));
  }
  return result;
}
std::string utf8(std::u16string_view text) {
  std::string result;
  for (auto c : text) {
    if (c < 0x80) result += char(c);
    else if (c < 0x800) { result += char(0xc0 | (c >> 6)); result += char(0x80 | (c & 63)); }
    else throw FormatError("Unsupported fixture UTF16");
  }
  return result;
}
void wide(std::span<std::byte> dst, std::u16string_view text) {
  if ((text.size() + 1) * 2 > dst.size()) throw FormatError("Invalid fixture copy boundary");
  for (std::size_t i = 0; i <= text.size(); ++i) {
    const auto c = i < text.size() ? text[i] : char16_t{};
    dst[i * 2] = std::byte(c); dst[i * 2 + 1] = std::byte(c >> 8);
  }
}
template<class F> void rejects(F&& f) {
  try { f(); } catch (const FormatError&) { return; }
  throw FormatError("Missing map callback ownership was accepted");
}
}
int main(int argc, char** argv) try {
  if (argc < 2) return 77;
  Json fixtures = Json::array();
  for (int argument = 1; argument < argc; ++argument) {
    std::ifstream input(argv[argument]); if (!input) return 77;
    Json rows; input >> rows;
    for (auto& row : rows) fixtures.push_back(std::move(row));
  }
  unsigned cases{};
  for (const auto& fixture : fixtures) {
    const auto& q = fixture.at("input");
    const auto handle = q.at("handle").get<std::uint32_t>();
    const auto op = q.at("op").get<std::string>();
    const auto index = handle == 0xffffffffu ? 0u : handle & 0xffffu;
    const auto stride = op == "custom" ? 0x60u : 8u;
    std::vector<std::byte> rows((index + 1) * stride);
    auto record = std::span(rows).subspan(index * stride, stride);
    const auto map = q.at("map_id").get<std::uint32_t>();
    const auto label = utf16(q.at("label").get<std::string>());
    if (op == "custom") {
      for (unsigned i = 0; i < record.size(); ++i)
        record[i] = std::byte(i * 23 + q.at("seed").get<unsigned>());
      std::fill_n(record.begin(), 64, std::byte{});
      wide(record.first(64), utf16("Custom map " + std::to_string(q.at("seed").get<unsigned>())));
    } else {
      for (unsigned i = 0; i < 4; ++i) record[4 + i] = std::byte(map >> (8 * i));
    }
    std::array<std::byte, 0x298> edit{};
    for (unsigned i = 0; i < edit.size(); ++i) edit[i] = std::byte(i * 37 + 19);
    Json events = Json::array();
    VistaLobbyMapCommitState state{q.at("locked").get<std::uint8_t>(), q.at("edit").get<std::uint8_t>(), rows};
    const auto mutate = [&](std::uint32_t pc) {
      if (!q.contains("mutations")) return;
      for (const auto& change : q.at("mutations")) {
        if (change.at("pc").get<std::uint32_t>() != pc) continue;
        if (change.contains("edit")) state.edit = change.at("edit").get<std::uint8_t>();
        if (change.contains("map_id")) {
          const auto id = change.at("map_id").get<std::uint32_t>();
          for (unsigned i = 0; i < 4; ++i) record[4 + i] = std::byte(id >> (8 * i));
        }
      }
    };
    VistaLobbyMapCommitServices s;
    s.widget_v34 = [&] { events.push_back({"widget_v34", q.at("channel")}); return q.at("channel").get<std::uint32_t>(); };
    s.widget_v38 = [&] { events.push_back({"widget_v38", q.at("dialog_arg")}); return q.at("dialog_arg").get<std::uint32_t>(); };
    s.dialog = [&](auto a, auto b, auto c) { events.push_back({"dialog", a, b, c}); };
    s.edit_owner = [&]() -> std::span<std::byte> { events.push_back({"edit_owner"}); mutate(0x609b9d); return edit; };
    s.definition = [&](auto id) { events.push_back({"definition", id}); return VistaLobbyMapCommitObject{1}; };
    s.definition_name = [&](auto object) -> std::u16string_view {
      if (object != 1) throw FormatError("Changed source definition handle");
      events.push_back({"definition_name", q.at("label")}); return label;
    };
    s.copy_name = [&](auto destination, auto value) {
      events.push_back({"copy_name", 32, utf8(value)}); wide(destination, value);
    };
    s.publish_edit = [&] { events.push_back({"publish_edit"}); };
    s.select_map = [&](auto campaign, auto id) {
      events.push_back({"select_map", campaign, id, q.at("select_result")});
      return q.at("select_result").get<std::uint8_t>();
    };
    s.class_query = [&] { events.push_back({"class_query", q.at("class_query")}); mutate(0x6156b9); return q.at("class_query").get<std::uint8_t>(); };
    s.selection_context = [&] { events.push_back({"selection_context"}); };
    s.select_custom_context = [&](auto arg, auto identifier) {
      events.push_back({"select_custom_context", arg, hex(identifier), q.at("select_result")});
      return q.at("select_result").get<std::uint8_t>();
    };
    s.select_custom_session = [&](auto identifier) {
      events.push_back({"select_custom_session", hex(identifier), q.at("select_result")});
      return q.at("select_result").get<std::uint8_t>();
    };
    s.root_screen = [&] { events.push_back({"root_screen"}); return VistaLobbyMapCommitObject{2}; };
    s.screen_v80 = [&](auto object) {
      if (object != 2) throw FormatError("Changed screen handle");
      events.push_back({"screen_v80", q.at("screen_arg0")}); return q.at("screen_arg0").get<std::uint32_t>();
    };
    s.screen_v84 = [&](auto object) {
      if (object != 2) throw FormatError("Changed screen handle");
      events.push_back({"screen_v84", q.at("screen_arg1")}); return q.at("screen_arg1").get<std::uint32_t>();
    };
    s.close = [&](auto a, auto b) { events.push_back({"close", a, b}); };
    s.text = [&](auto ordinal) {
      events.push_back({"text_lookup", ordinal, q.at("text_exists")});
      return VistaLobbyMapCommitObject(q.at("text_exists").get<unsigned>() ? 3 : 0);
    };
    s.string = [&](auto object, auto value) {
      if (object != 3) throw FormatError("Changed text handle");
      events.push_back({"text", utf8(value)});
    };
    // memset is implemented by the native owner, not an external service.
    // Preserve its ordered original event in this fixture sink.
    s.definition = [&](auto id) {
      if (op == "standard" && state.edit) {
        if (edit[0xee] != std::byte{3} || !std::all_of(edit.begin() + 0xf8,
            edit.begin() + 0x158, [](auto value) { return value == std::byte{}; }))
          throw FormatError("Original edit writes must precede the definition service");
        events.push_back({"clear_identifier", 0x60});
      }
      events.push_back({"definition", id}); return VistaLobbyMapCommitObject{1};
    };
    if (op == "standard") vista_lobby_standard_map_commit(state, handle, s);
    else if (op == "custom") vista_lobby_custom_map_commit(state, handle, s);
    else vista_lobby_standard_map_label(state, handle, s);
    if (events != fixture.at("events") || hex(edit) != fixture.at("edit_bank").get<std::string>()) {
      std::cerr << "Map callback case " << cases << ' ' << q << "\nactual " << events
          << "\noriginal " << fixture.at("events") << '\n';
      throw FormatError("Original Vista map callback output/order mismatch");
    }
    ++cases;
  }
  VistaLobbyMapCommitState state{}; VistaLobbyMapCommitServices services;
  vista_lobby_standard_map_commit(state, 0xffffffffu, services);
  vista_lobby_custom_map_commit(state, 0xffffffffu, services);
  vista_lobby_standard_map_label(state, 0xffffffffu, services);
  rejects([&] { vista_lobby_standard_map_commit(state, 0, services); });
  std::array<std::byte, 8> row{}; state.rows = row;
  rejects([&] { vista_lobby_standard_map_commit(state, 0, services); });
  std::cout << "PASS " << cases << " unchanged original Vista map callback cases and explicit owner boundaries\n";
  return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
