#include "halo2/cache.hpp"
#include "halo2/vista_map_selector.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string_view>
#include <vector>
using namespace halo2;
using Json = nlohmann::json;
namespace {
std::vector<std::byte> unhex(std::string_view text) {
  if (text.size() % 2)
    throw FormatError("Odd selector source hex");
  const auto digit = [](char c) -> unsigned {
    if (c >= '0' && c <= '9')
      return unsigned(c - '0');
    if (c >= 'a' && c <= 'f')
      return unsigned(c - 'a') + 10;
    throw FormatError("Invalid selector source hex");
  };
  std::vector<std::byte> out(text.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = std::byte{
        std::uint8_t(16 * digit(text[2 * i]) + digit(text[2 * i + 1]))};
  return out;
}
std::string hex_request(const VistaMapSelectorRequest &r) {
  std::array<std::uint32_t, 4> words{std::uint32_t(r.flags) |
                                         (std::uint32_t(r.controller) << 16),
                                     r.manager_kind, r.mask, r.edit};
  std::string text;
  constexpr char digits[] = "0123456789abcdef";
  for (auto word : words)
    for (unsigned i = 0; i < 4; ++i) {
      const auto b = std::uint8_t(word >> (8 * i));
      text += digits[b >> 4];
      text += digits[b & 15];
    }
  return text;
}
void check(bool ok, const char *message) {
  if (!ok)
    throw FormatError(message);
}
template <class F> void rejects(F &&f) {
  try {
    f();
  } catch (const FormatError &) {
    return;
  }
  throw FormatError("Invalid selector boundary admitted");
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 2)
    return 77;
  std::ifstream stream(argv[1]);
  if (!stream)
    return 77;
  Json data;
  stream >> data;
  std::size_t cases{};
  constexpr std::uint32_t base = 0x1a00000;
  for (const auto &row : data.at("owner_cases")) {
    const auto &q = row.at("input");
    const auto seed = q.at("seed").get<unsigned>();
    std::vector<std::byte> bank(vista_map_selector_bank_size + 17);
    for (std::size_t i = 0; i < bank.size(); ++i)
      bank[i] = std::byte{std::uint8_t(i * 37 + seed)};
    const auto original = bank;
    Json trace = Json::array();
    VistaMapSelectorConstructionServices s;
    s.allocate = [&](auto n, auto flags) {
      trace.push_back({"allocate_screen", n, flags});
      return VistaMapSelectorStorage{
          std::span(bank).first(vista_map_selector_bank_size), base};
    };
    s.screen = [&](auto storage, auto id, auto arg1, auto arg2,
                   auto controller) {
      trace.push_back({"base_screen", storage.source_identity - base, id, arg1,
                       arg2, controller});
    };
    s.custom_list = [&](auto storage, auto controller) {
      check(storage.bytes.size() == 0x808,
            "Custom list source extent mismatch");
      trace.push_back(
          {"custom_list", storage.source_identity - base, controller});
    };
    s.standard_list = [&](auto storage, auto controller) {
      check(storage.bytes.size() == 0x808,
            "Standard list source extent mismatch");
      trace.push_back(
          {"standard_list", storage.source_identity - base, controller});
    };
    s.text = [&](auto storage, auto controller) {
      check(storage.bytes.size() == 0x4bc,
            "Selector text source extent mismatch");
      trace.push_back(
          {"embedded_text", storage.source_identity - base, controller});
    };
    s.register_screen = [&](auto storage, const auto &request) {
      check(request.source_identity == base + 0x10000,
            "Selector request identity mismatch");
      trace.push_back({"register_screen", storage.source_identity - base,
                       hex_request(request)});
    };
    VistaMapSelectorRequest request{
        q.at("request_flags"), q.at("controller"),   4,
        q.at("mask"),          q.at("edit_request"), base + 0x10000};
    VistaMapSelectorStorage storage{
        std::span(bank).first(vista_map_selector_bank_size), base};
    if (q.contains("arguments")) {
      const auto &a = q.at("arguments");
      check(vista_map_selector_construct(
                storage, {a.at(0), a.at(1), a.at(2), a.at(3), a.at(4), a.at(5)},
                s) == base,
            "Selector constructor return identity mismatch");
    } else {
      storage = vista_map_selector_factory(
          request, q.at("alternate").get<unsigned>() != 0, s);
    }
    check(storage.source_identity == base,
          "Selector factory return identity mismatch");
    check(trace == row.at("events"),
          "Selector original constructor call sequence mismatch");
    const auto expected = unhex(row.at("bank").get<std::string>());
    if (!std::equal(expected.begin(), expected.end(), bank.begin())) {
      const auto at =
          std::mismatch(expected.begin(), expected.end(), bank.begin()).first -
          expected.begin();
      std::cerr << "owner case " << cases << " at " << std::hex << at
                << std::dec << '\n';
      throw FormatError("Selector original byte bank mismatch");
    }
    check(std::equal(bank.begin() + vista_map_selector_bank_size, bank.end(),
                     original.begin() + vista_map_selector_bank_size),
          "Selector changed storage after allocation");
    check(vista_map_selector_factory_identity(bank) ==
              (expected.at(0x1a70) != std::byte{0} ? 0x64f8bfu : 0x64f819u),
          "Selector factory identity mismatch");
    ++cases;
  }
  for (const auto &row : data.at("focus_cases")) {
    const auto &q = row.at("input");
    VistaMapSelectorWidget screen{base, 0}, list{base + 0x100, 1};
    list.parent = &screen;
    std::array<VistaMapSelectorWidget, 3> rows;
    const auto count = q.at("count").get<unsigned>();
    for (unsigned i = 0; i < 3; ++i) {
      rows[i].source_identity = base + 0x200 + i * 0x100;
      rows[i].kind = q.at("kinds").at(i);
      rows[i].handle = q.at("handles").at(i);
      rows[i].parent = &list;
      rows[i].next = i + 1 < count ? &rows[i + 1] : nullptr;
    }
    list.first_child = count ? &rows[0] : nullptr;
    VistaMapSelectorWidget nested{base + 0x600, 6}, outside{base + 0x700, 6};
    nested.parent = &rows[q.at("nested_row").get<unsigned>()];
    outside.parent = &screen;
    const auto name = q.at("focus").get<std::string>();
    const VistaMapSelectorWidget *focus{};
    if (name == "nested")
      focus = &nested;
    else if (name == "outside")
      focus = &outside;
    else if (name == "list")
      focus = &list;
    else if (name == "screen")
      focus = &screen;
    else if (name.starts_with("row"))
      focus = &rows[unsigned(name.back() - '0')];
    Json calls = Json::array();
    unsigned phase{};
    VistaMapSelectorFocusServices s;
    s.v38 = [&](const auto &) {
      check(phase == 0, "Selector focus v38 order mismatch");
      phase = 1;
      return q.at("root78").get<std::uint32_t>();
    };
    s.v34 = [&](const auto &) {
      check(phase == 1, "Selector focus v34 order mismatch");
      phase = 2;
      return q.at("root74").get<std::uint32_t>();
    };
    s.focused = [&](auto argument, auto c34, auto c38) {
      check(phase == 2, "Selector focus manager order mismatch");
      phase = 0;
      calls.push_back({argument, c34, c38});
      return focus;
    };
    check(vista_map_selector_selected_handle(list, s) == row.at("selected"),
          "Selector focused row identity mismatch");
    check(calls == row.at("manager_calls"),
          "Selector focus query trace mismatch");
    ++cases;
  }
  for (const auto &row : data.at("text_lookup_cases")) {
    const auto &q = row.at("input");
    VistaMapSelectorWidget screen{base, 0};
    std::vector<VistaMapSelectorWidget> children(q.at("kinds").size()),
        nested(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      children[i].source_identity = base + 0x100 + std::uint32_t(i) * 0x100;
      children[i].kind = q.at("kinds").at(i);
      children[i].next = i + 1 < children.size() ? &children[i + 1] : nullptr;
      children[i].first_child = &nested[i];
      nested[i].kind = 6;
    }
    screen.first_child = children.empty() ? nullptr : &children[0];
    const auto *result = vista_map_selector_text(screen, q.at("index"));
    if (row.at("relative").is_null())
      check(!result, "Selector text included nested/out-of-range child");
    else
      check(result && result->source_identity - base == row.at("relative"),
            "Selector direct text ordinal mismatch");
    ++cases;
  }
  for (const auto &row : data.at("mouse_cases")) {
    const auto &q = row.at("input");
    std::array<VistaMapSelectorWidget, 2> widgets;
    for (unsigned i = 0; i < 2; ++i) {
      widgets[i].bounds =
          q.at("rects").at(i).get<std::array<std::int16_t, 4>>();
      widgets[i].animation_x = q.at("translation").at(0);
      widgets[i].animation_y = q.at("translation").at(1);
    }
    VistaMapSelectorInputState state{
        q.at("tab"), 0,
        q.at("present").at(0).get<unsigned>() ? &widgets[0] : nullptr,
        q.at("present").at(1).get<unsigned>() ? &widgets[1] : nullptr};
    unsigned mouse{};
    VistaMapSelectorInputServices services;
    services.mouse = [&] {
      ++mouse;
      return q.at("point").get<std::array<std::int16_t, 2>>();
    };
    std::uint32_t event = 6;
    const auto result = vista_map_selector_mouse_tabs(state, event, services);
    Json actual{{"handled", result},
                {"event", event},
                {"tab", state.tab},
                {"changed", state.changed}};
    check(actual == row.at("result") && mouse == 1,
          "Selector original mouse bounds/input mismatch");
    ++cases;
  }
  for (const auto &row : data.at("input_cases")) {
    const auto &q = row.at("input");
    VistaMapSelectorInputState state{q.at("tab"), q.at("changed"), nullptr,
                                     nullptr};
    std::uint32_t event = q.at("event");
    Json calls = Json::array();
    VistaMapSelectorInputServices services;
    services.mouse = [] { return std::array<std::int16_t, 2>{}; };
    services.base_input = [&](auto &e) {
      calls.push_back(
          Json{{"event", e}, {"tab", state.tab}, {"changed", state.changed}});
      return q.at("base_return").get<std::uint8_t>();
    };
    const auto result = vista_map_selector_input(state, event, services);
    Json actual{{"handled", result},
                {"event", event},
                {"tab", state.tab},
                {"changed", state.changed}};
    check(actual == row.at("result") && calls == row.at("base_calls"),
          "Selector original DWORD tab/base input mismatch");
    ++cases;
  }
  std::vector<std::byte> bank(vista_map_selector_bank_size);
  VistaMapSelectorConstructionServices missing;
  rejects([&] { vista_map_selector_construct({bank, 0}, {}, missing); });
  rejects([&] {
    vista_map_selector_construct({std::span(bank).first(bank.size() - 1), base},
                                 {}, missing);
  });
  rejects(
      [&] { vista_map_selector_construct({bank, 0xfffff000}, {}, missing); });
  rejects([&] { vista_map_selector_construct({bank, base}, {}, missing); });
  rejects([&] { vista_map_selector_factory({}, false, missing); });
  missing.allocate = [](auto, auto) { return VistaMapSelectorStorage{}; };
  rejects([&] { vista_map_selector_factory({}, false, missing); });
  rejects([&] { vista_map_selector_factory_identity({}); });
  check(!vista_map_selector_focused(VistaMapSelectorWidget{base, 6}, 0, {}),
        "Detached nonscreen queried focus");
  std::cout << "PASS " << cases
            << " original Vista selector "
               "factory/constructor/focus/direct-text/mouse/input cases; exact "
               "banks and ordered services\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
