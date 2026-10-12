#include "halo2/unit_constructors.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool v, const std::string &why) {
  if (!v)
    throw std::runtime_error("Source constructor differs: " + why);
}
std::uint32_t num(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> decode(const std::string &s) {
  if (s == "-")
    return {};
  check(s.size() % 2 == 0, "hex extent");
  std::vector<std::byte> b;
  for (std::size_t i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(num(s.substr(i, 2))));
  return b;
}
using State = std::map<std::string, std::vector<std::byte>>;
using Store = halo2::UnitConstructorStore;
using Kind = halo2::UnitConstructorBuffer;
Kind kind(const std::string &s) {
  if (s == "M")
    return Kind::Manager;
  if (s == "H")
    return Kind::Header;
  if (s == "U")
    return Kind::Unit;
  if (s == "F")
    return Kind::Failure;
  throw std::runtime_error("Unknown constructor storage label");
}
struct Leaf {
  halo2::UnitConstructorLeaf call;
  unsigned result{};
  State before, after;
  std::vector<std::byte> backing;
};
struct Case {
  std::string label;
  std::uint32_t entry{}, unit{}, graph{}, model{}, flag{}, field{}, size{},
      alignment{}, capacity{}, one{}, result{}, game{};
  State before, after;
  std::vector<Leaf> leaves;
  std::vector<Store> stores;
  std::map<std::uint32_t, std::int32_t> graph_nodes, model_nodes;
  std::map<std::uint32_t, std::vector<std::byte>> definitions, regions;
};
std::uint32_t read(std::span<const std::byte> b, unsigned at, unsigned n = 4) {
  check(at + n <= b.size(), "test-owned storage extent");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
void run(const Case &c, unsigned &leaf_count, unsigned &store_count) {
  auto state = c.before;
  std::vector<std::byte> headers(std::size_t(c.capacity) * 12);
  const auto at = std::size_t(c.unit & 65535) * 12;
  if (state.contains("H"))
    std::copy(state.at("H").begin(), state.at("H").end(), headers.begin() + at);
  auto h = [&]() { return std::span<std::byte>(headers).subspan(at, 12); };
  auto visible = [&](const std::string &key) -> std::span<const std::byte> {
    if (key == "H")
      return h();
    if (key == "U")
      return std::span<const std::byte>(state.at(key)).first(read(h(), 6, 2));
    return state.at(key);
  };
  auto compare = [&](const State &expected, const std::string &where) {
    for (const auto &[key, b] : expected) {
      auto v = visible(key);
      check(v.size() == b.size() && std::equal(v.begin(), v.end(), b.begin()),
            c.label + " " + where + " " + key);
    }
  };
  auto apply = [&](const State &after, const std::vector<std::byte> &backing) {
    for (const auto &[key, b] : after) {
      if (key == "H")
        std::copy(b.begin(), b.end(), h().begin());
      else
        state[key] = b;
    }
    if (!backing.empty())
      state["U"] = backing;
  };
  std::size_t next = 0;
  std::vector<Store> stores;
  halo2::UnitConstructorServices s{};
  s.one_bits = c.one;
  s.objects.headers = headers;
  s.objects.capacity = c.capacity;
  s.objects.body = [&](std::uint16_t index) -> std::span<std::byte> {
    check(index == std::uint16_t(c.unit), c.label + " low16 pool index");
    return state.at("U");
  };
  auto callback = [&](const halo2::UnitConstructorLeaf &packet) {
    check(next < c.leaves.size(), c.label + " extra leaf");
    const auto &l = c.leaves[next++];
    check(packet == l.call,
          c.label + " exact nested leaf order/ABI " + std::to_string(next));
    compare(l.before, "leaf entry " + std::to_string(next));
    // Differential-only literal output of the original unchanged service.
    // Production has no default allocation/selection/reset success.
    apply(l.after, l.backing);
    ++leaf_count;
    return l.result;
  };
  s.objects.grow = [&](std::uint32_t id, std::int32_t n) {
    check(id == c.unit && next < c.leaves.size(),
          c.label + " growth full identity");
    const auto &l = c.leaves[next];
    check(l.call.entry == 0x18fd70, c.label + " growth owner");
    check(std::bit_cast<std::int32_t>(l.call.eax) == n,
          c.label + " original signed allocation request");
    check(l.call.count == 3 && l.call.arguments[1] == 0 &&
              l.call.arguments[2] == 0,
          c.label + " growth flags");
    return callback(l.call) != 0;
  };
  s.graph_nodes = [&](std::uint32_t g) {
    return std::int16_t(c.graph_nodes.at(g));
  };
  s.model_nodes = [&](std::uint32_t m) { return c.model_nodes.at(m); };
  s.select_default = [&](std::span<std::byte> b,
                         const halo2::UnitConstructorLeaf &l) {
    check(b.data() == state.at("M").data(),
          c.label + " actual manager conduit");
    callback(l);
  };
  s.unit_definition = [&](std::uint32_t tag) -> std::span<const std::byte> {
    return c.definitions.at(tag);
  };
  s.model_node_records = [&](std::uint32_t m) -> std::span<const std::byte> {
    return c.regions.at(m);
  };
  s.game_type = [&]() {
    check(next < c.leaves.size() && c.leaves[next].call.entry == 0x110bf0,
          c.label + " game+8 read at its original owner stage");
    auto expected = c.leaves[next].before;
    // Between this actual source read and110BF0 only the optional category
    // rewrite may store. Reverse that literal recorded write to recover the
    // complete original buffer at the read, without native-derived defaults.
    for (const auto &w : c.stores) {
      if (w.instruction != 0xc5608)
        continue;
      auto &u = expected.at("U");
      for (unsigned i = 0; i < w.bytes; ++i)
        u[w.offset + i] = std::byte((w.before >> (8 * i)) & 255);
    }
    compare(expected, "actual game+8 reader checkpoint");
    return c.game;
  };
  s.invoke = [&](const halo2::UnitConstructorLeaf &l) { callback(l); };
  auto observe = [&](const Store &w) { stores.push_back(w); };
  unsigned result = 0;
  switch (c.entry) {
  case 0x1ce8c0:
    halo2::construct_source_unit_animation_manager(state.at("M"), c.one,
                                                   observe);
    break;
  case 0x1ce9b0:
    halo2::reset_source_unit_animation_manager(state.at("M"), c.one, observe);
    break;
  case 0x1ceac0:
    result = halo2::bind_source_unit_animation_manager(
        state.at("M"), c.graph, c.model, c.flag != 0, s, observe);
    break;
  case 0xbd270:
    result = halo2::append_source_unit_component(
        c.unit, std::bit_cast<std::int16_t>(std::uint16_t(c.field)),
        std::bit_cast<std::int16_t>(std::uint16_t(c.size)),
        std::uint8_t(c.alignment), s.objects, observe);
    break;
  case 0xc52c0:
    result = halo2::construct_source_unit(c.unit, state.at("D"), state.at("F"),
                                          s, observe);
    break;
  default:
    throw std::runtime_error("Unknown source constructor owner");
  }
  if (c.entry != 0x1ce8c0 && c.entry != 0x1ce9b0)
    check(result == c.result, c.label + " defined AL");
  check(next == c.leaves.size(), c.label + " all service boundaries consumed");
  compare(c.after, "complete output");
  check(stores == c.stores,
        c.label + " exact ordered store width/value/previous bytes");
  store_count += stores.size();
}
void bounds_and_missing_services() {
  std::vector<std::byte> manager(0x90, std::byte{0x7d});
  halo2::UnitConstructorServices s{};
  // Source's model-NONE early path really does bypass the unavailable tag
  // owner.
  const auto before = manager;
  bool blocked = false;
  try {
    halo2::bind_source_unit_animation_manager(manager, 0xffffffff, 0xffffffff,
                                              false, s);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked, "missing1d0a60 must stop rather than succeed");
  check(manager != before,
        "source bind prefix remains visible at required service stop");
  blocked = false;
  try {
    halo2::reset_source_unit_animation_manager(
        std::span<std::byte>(manager).first(0x8f), 0);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked, "truncated manager rejects");
  std::vector<std::byte> headers(12), body(0x350, std::byte{0x63});
  headers[6] = std::byte{0x50};
  headers[7] = std::byte{3};
  halo2::UnitConstructorObjectPool p{
      headers,
      1,
      [&](std::uint16_t) -> std::span<std::byte> { return body; },
      {}};
  const auto old = body;
  blocked = false;
  try {
    halo2::append_source_unit_component(0x12340000, 0x344, 0x44, 0, p);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked && body == old,
        "missing growth never fabricates header/component writes");
  p.grow = [](auto, auto) { return false; };
  check(!halo2::append_source_unit_component(0x12340000, 0x344, 0x44, 0, p) &&
            body == old,
        "declared allocator failure retains storage");
  blocked = false;
  try {
    halo2::append_source_unit_component(0x12340001, 0x344, 0, 0, p);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked, "pool index bounds");
  blocked = false;
  try {
    halo2::append_source_unit_component(0x12340000, -1, 0, 0, p);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked, "negative descriptor host bounds");
  p.grow = [](auto, auto) { return true; };
  blocked = false;
  try {
    halo2::append_source_unit_component(0x12340000, 0x344, 0x44, 0, p);
  } catch (const std::runtime_error &) {
    blocked = true;
  }
  check(blocked,
        "unsupported backing service cannot imply allocation capacity");
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2 || !std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream in(argv[1]);
    std::string line;
    Case c;
    Leaf *leaf = nullptr;
    unsigned cases = 0, leaves = 0, stores = 0;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      std::istringstream row(line);
      std::string type;
      row >> type;
      if (type == "CASE") {
        c = {};
        row >> c.label;
        std::uint32_t *fields[]{&c.entry,    &c.unit,  &c.graph, &c.model,
                                &c.flag,     &c.field, &c.size,  &c.alignment,
                                &c.capacity, &c.one,   &c.result};
        for (auto f : fields) {
          std::string s;
          row >> s;
          *f = num(s);
        }
        leaf = nullptr;
      } else if (type == "BEFORE" || type == "AFTER" || type == "LBEFORE" ||
                 type == "LAFTER") {
        std::string k, v;
        row >> k >> v;
        auto &dest = type == "BEFORE"    ? c.before
                     : type == "AFTER"   ? c.after
                     : type == "LBEFORE" ? leaf->before
                                         : leaf->after;
        dest[k] = decode(v);
      } else if (type == "LEAF") {
        c.leaves.emplace_back();
        leaf = &c.leaves.back();
        std::string e, h, v, r;
        row >> e >> h >> v >> r >> leaf->call.count;
        leaf->call.entry = num(e);
        leaf->call.eax_input = num(h) != 0;
        leaf->call.eax = num(v);
        leaf->result = num(r);
        for (unsigned i = 0; i < leaf->call.count; ++i) {
          row >> v;
          leaf->call.arguments[i] = num(v);
        }
      } else if (type == "BACKING") {
        std::string v;
        row >> v;
        leaf->backing = decode(v);
      } else if (type == "ENDLEAF") {
        leaf = nullptr;
      } else if (type == "STORE") {
        std::string ip, k, at, n, b, v;
        row >> ip >> k >> at >> n >> b >> v;
        c.stores.push_back({num(ip), kind(k), num(at), num(n), num(b), num(v)});
      } else if (type == "NODES") {
        std::string k, id, n;
        row >> k >> id >> n;
        (k == "graph" ? c.graph_nodes : c.model_nodes)[num(id)] =
            std::bit_cast<std::int32_t>(num(n));
      } else if (type == "DEF" || type == "REGIONS") {
        std::string id, v;
        row >> id >> v;
        (type == "DEF" ? c.definitions : c.regions)[num(id)] = decode(v);
      } else if (type == "GAME") {
        std::string v;
        row >> v;
        c.game = num(v);
      } else if (type == "ENDCASE") {
        run(c, leaves, stores);
        ++cases;
      } else
        throw std::runtime_error("Unknown source constructor fixture row");
    }
    bounds_and_missing_services();
    check(cases > 0, "no original cases");
    std::cout << cases << " original complete constructor buffers, " << leaves
              << " explicit original service boundaries, " << stores
              << " exact owning stores passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
