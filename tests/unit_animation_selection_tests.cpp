#include "halo2/unit_animation_selection.hpp"
#if HALO2_SELECTION_CACHE_TESTS
#include "halo2/cache.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#endif
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using Bytes = std::vector<std::byte>;
void check(bool value, const std::string &why) {
  if (!value)
    throw std::runtime_error(why);
}
Bytes decode(const std::string &s) {
  check(s.size() % 2 == 0, "Odd literal byte buffer");
  Bytes out;
  for (std::size_t i = 0; i < s.size(); i += 2)
    out.push_back(std::byte(std::stoul(s.substr(i, 2), nullptr, 16)));
  return out;
}
std::uint32_t hex(std::istringstream &in) {
  std::uint32_t v;
  check(bool(in >> std::hex >> v), "Missing fixture integer");
  return v;
}
std::uint32_t word(std::span<const std::byte> b, unsigned at) {
  check(at <= b.size() && 4 <= b.size() - at, "Truncated literal word");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
template <std::size_t N>
std::array<std::uint32_t, N> words(std::span<const std::byte> b) {
  check(b.size() == 4 * N, "Literal tuple width differs");
  std::array<std::uint32_t, N> out{};
  for (unsigned i = 0; i < N; ++i)
    out[i] = word(b, 4 * i);
  return out;
}
struct Leaf {
  std::uint32_t entry, offset;
  std::vector<std::uint32_t> args, output;
  Bytes before, after;
};
struct Case {
  std::uint32_t id, entry, result;
  std::vector<std::uint32_t> args;
  Bytes before, after;
  std::vector<Leaf> leaves;
  std::vector<UnitAnimationSelectionStore> stores;
};
struct Fixture {
  std::map<std::uint32_t, Bytes> memory;
  std::map<std::uint32_t, std::uint32_t> graphs, constants;
  std::map<std::pair<std::uint32_t, std::uint32_t>, UnitAnimationDefinition>
      definitions;
  std::vector<std::string> pure;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream f(path);
  check(bool(f), "Cannot open original selection fixture");
  Fixture out;
  Case *c = nullptr;
  Leaf *l = nullptr;
  for (std::string line; std::getline(f, line);) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream in(line);
    std::string kind, raw;
    in >> kind;
    if (kind == "MEM") {
      auto p = hex(in);
      in >> raw;
      auto b = decode(raw);
      auto [it, fresh] = out.memory.emplace(p, b);
      check(fresh || it->second == b, "Conflicting original backing");
    } else if (kind == "GRAPH") {
      auto d = hex(in), p = hex(in);
      out.graphs.emplace(d, p);
    } else if (kind == "DEF") {
      auto graph = hex(in), packed = hex(in), n = hex(in), flags = hex(in);
      UnitAnimationDefinition d;
      d.frame_count = std::bit_cast<std::int16_t>(std::uint16_t(n));
      d.authored_flags = std::uint8_t(flags);
      out.definitions.emplace(std::make_pair(graph, packed), std::move(d));
    } else if (kind == "CONSTANT") {
      auto p = hex(in), v = hex(in);
      out.constants.emplace(p, v);
    } else if (kind == "LOOKUP" || kind == "NORMAL" || kind == "NEXT" ||
               kind == "TREE")
      out.pure.push_back(line);
    else if (kind == "CASE") {
      Case v;
      v.id = hex(in);
      v.entry = hex(in);
      v.result = hex(in);
      auto n = hex(in);
      for (unsigned i = 0; i < n; ++i)
        v.args.push_back(hex(in));
      out.cases.push_back(std::move(v));
      c = &out.cases.back();
    } else if (kind == "BEFORE" || kind == "AFTER") {
      check(c != nullptr, "Buffer outside case");
      in >> raw;
      (kind == "BEFORE" ? c->before : c->after) = decode(raw);
    } else if (kind == "LEAF") {
      check(c != nullptr, "Leaf outside case");
      Leaf v;
      v.entry = hex(in);
      v.offset = hex(in);
      auto n = hex(in);
      for (unsigned i = 0; i < n; ++i)
        v.args.push_back(hex(in));
      n = hex(in);
      for (unsigned i = 0; i < n; ++i)
        v.output.push_back(hex(in));
      c->leaves.push_back(std::move(v));
      l = &c->leaves.back();
    } else if (kind == "LBEFORE" || kind == "LAFTER") {
      check(l != nullptr, "Buffer outside leaf");
      in >> raw;
      (kind == "LBEFORE" ? l->before : l->after) = decode(raw);
    } else if (kind == "STORE") {
      check(c != nullptr, "Store outside case");
      auto ip = hex(in), at = hex(in), n = hex(in), before = hex(in),
           after = hex(in);
      c->stores.push_back({ip, at, n, before, after});
    } else if (kind == "ENDCASE")
      c = nullptr;
    else if (kind == "ENDLEAF")
      l = nullptr;
    else
      throw std::runtime_error("Unknown selection fixture row: " + kind);
  }
  return out;
}
SourceAnimationGraphMemory memory(const Fixture &f) {
  SourceAnimationGraphMemory g;
  g.graph_address = [&](std::uint32_t d) {
    auto i = f.graphs.find(d);
    check(i != f.graphs.end(), "Missing literal graph identity");
    return i->second;
  };
  g.read = [&](std::uint32_t at, std::size_t n) -> std::span<const std::byte> {
    for (const auto &[p, b] : f.memory) {
      if (at >= p && std::uint64_t(at) - p <= b.size() &&
          n <= b.size() - (std::uint64_t(at) - p))
        return std::span<const std::byte>(b).subspan(at - p, n);
    }
    throw std::runtime_error("Source graph read outside retained backing");
  };
  return g;
}
void pure(const Fixture &f, const SourceAnimationGraphMemory &g,
          std::map<std::string, unsigned> &counts) {
  for (const auto &line : f.pure) {
    std::istringstream in(line);
    std::string kind, a, b, c, d;
    in >> kind;
    ++counts[kind];
    if (kind == "LOOKUP") {
      auto bank = hex(in), key = hex(in), stride = hex(in), expected = hex(in);
      check(find_source_animation_record(bank, key, stride, g) == expected,
            "Original binary search differs");
    } else if (kind == "TREE") {
      auto graph = hex(in), mode = hex(in), wc = hex(in), weapon = hex(in),
           expected = hex(in);
      check(find_source_animation_weapon(graph, mode, wc, weapon, g) ==
                expected,
            "Original full tree lookup differs");
    } else if (kind == "NORMAL") {
      in >> a;
      SourceAnimationIdentifiers ids;
      for (auto &v : ids)
        v = hex(in);
      in >> b;
      check(normalize_source_animation_request(decode(a), ids, g) ==
                words<4>(decode(b)),
            "Original normalization differs");
    } else if (kind == "NEXT") {
      in >> a >> b >> c >> d;
      auto expected = hex(in);
      auto state = decode(a);
      auto outputs = words<3>(decode(b));
      check(next_source_animation_weapon(state, outputs, g) == expected,
            "Original iterator return differs");
      check(state == decode(c), "Original complete iterator state differs");
      check(outputs == words<3>(decode(d)),
            "Original retained iterator outputs differ");
    }
  }
  bool missing = false;
  try {
    find_source_animation_record(0, 0, 8, g);
  } catch (const std::runtime_error &) {
    missing = true;
  }
  check(missing, "Missing graph backing fabricated a result");
  auto tiny = g;
  const std::array<std::byte, 3> truncated{};
  tiny.read = [&](std::uint32_t, std::size_t) {
    return std::span<const std::byte>(truncated);
  };
  bool bounds = false;
  try {
    find_source_animation_record(0, 0, 8, tiny);
  } catch (const std::runtime_error &) {
    bounds = true;
  }
  check(bounds, "Truncated graph backing admitted");
}
void kernels(const Fixture &f, unsigned &leaves, unsigned &stores,
             const std::function<const UnitAnimationDefinition &(
                 std::uint32_t, std::uint32_t)> &owned = {}) {
  for (const auto &c : f.cases) {
    auto m = c.before;
    check(m.size() == 0x90 && c.after.size() == 0x90,
          "Full original manager missing");
    std::vector<UnitAnimationSelectionStore> observed;
    auto observer = [&](const UnitAnimationSelectionStore &s) {
      observed.push_back(s);
    };
    UnitAnimationSelectionServices s{};
    s.one_bits = f.constants.at(0x465030);
    s.zero_bits = f.constants.at(0x465048);
    s.frame_subtractor_bits = f.constants.at(0x46504c);
    std::size_t next = 0;
    auto leaf =
        [&](std::uint32_t e, std::uint32_t off, std::vector<std::uint32_t> args,
            std::span<std::byte> manager) -> std::vector<std::uint32_t> {
      check(next < c.leaves.size(), "Unexpected native selection service");
      const auto &v = c.leaves[next++];
      check(v.entry == e && v.offset == off && v.args == args,
            "Source selection service identity/ABI/order differs at case " +
                std::to_string(c.id));
      check(std::equal(manager.begin(), manager.end(), v.before.begin(),
                       v.before.end()),
            "Source complete boundary entry manager differs at case " +
                std::to_string(c.id));
      std::copy(v.after.begin(), v.after.end(), manager.begin());
      ++leaves;
      return v.output;
    };
    s.definition =
        owned ? owned
              : [&](std::uint32_t graph,
                    std::uint32_t packed) -> const UnitAnimationDefinition & {
      auto i = f.definitions.find({graph, packed});
      check(i != f.definitions.end(),
            "Missing actual original definition result");
      return i->second;
    };
    s.resolve_request = [&](std::span<std::byte> manager,
                            SourceAnimationIdentifiers ids,
                            std::uint32_t flags) {
      auto args = std::vector<std::uint32_t>(ids.begin(), ids.end());
      args.push_back(flags);
      auto out = leaf(0x1cfde0, 0, args, manager);
      check(out.size() == 10, "Resolve packet width differs");
      UnitAnimationResolvedRequest r;
      std::copy_n(out.begin(), 4, r.requested.begin());
      std::copy_n(out.begin() + 4, 4, r.resolved.begin());
      r.packed = out[8];
      r.admitted = out[9] != 0;
      return r;
    };
    s.secondary = [&](std::span<std::byte> manager, std::uint32_t mode,
                      std::uint32_t activity) {
      auto out = leaf(0x1d0790, 0, {mode, activity}, manager);
      check(out.size() == 2, "Secondary packet width differs");
      return UnitAnimationSecondarySelection{out[0], std::uint8_t(out[1])};
    };
    s.third = [&](std::span<std::byte> manager,
                  SourceAnimationIdentifiers ids) {
      auto out = leaf(0x1d06c0, 0, {ids.begin(), ids.end()}, manager);
      check(out.size() == 2, "Third packet width differs");
      return UnitAnimationThirdSelection{out[0], out[1]};
    };
    s.variant = [&](std::span<std::byte> manager, std::size_t offset,
                    std::uint32_t packed) {
      auto out = leaf(0x1ceda0, std::uint32_t(offset), {packed}, manager);
      check(out.size() == 1, "Variant packet width differs");
      return out[0];
    };
    s.clamp_frame = [&](std::span<std::byte> manager, std::size_t offset,
                        std::uint32_t frame) {
      leaf(0x1c9f50, std::uint32_t(offset), {frame}, manager);
    };
    s.root_transform_delta = [&](std::span<std::byte> manager) {
      leaf(0x1d1a00, 0, {}, manager);
    };
    s.resources = [&](std::span<std::byte> manager,
                      std::array<std::uint32_t, 3> ids, std::uint32_t a,
                      std::uint32_t b) {
      leaf(0x1d1b70, 0, {ids[0], ids[1], ids[2], a, b}, manager);
    };
    bool result = false;
    if (c.entry == 0x1d0a60) {
      check(c.args.size() == 6, "Selector arguments missing");
      UnitAnimationSelectionRequest request{
          {c.args[0], c.args[1], c.args[2], c.args[3]}, c.args[4], c.args[5]};
      result = select_source_unit_animation(m, request, s, observer);
      check(result == (c.result != 0), "Defined original selector AL differs");
    } else if (c.entry == 0x1d0590)
      reset_source_unit_animation_channels(m, s.one_bits, observer);
    else if (c.entry == 0x1c9e60) {
      check(c.args.size() == 8, "Install arguments missing");
      const auto &a = c.args;
      result = install_source_unit_animation_channel(m, a[0], a[1], a[2], a[3],
                                                     a[4], a[5], a[6], a[7],
                                                     s.one_bits, observer);
      check(result == (c.result != 0), "Defined original install AL differs");
    } else
      throw std::runtime_error("Unexpected kernel entry");
    check(next == c.leaves.size(),
          "Original required selection service skipped");
    check(m == c.after,
          "Full original manager differs at case " + std::to_string(c.id));
    if (observed != c.stores) {
      for (std::size_t i = 0; i < std::min(observed.size(), c.stores.size());
           ++i)
        if (observed[i] != c.stores[i]) {
          std::cerr << "Store " << i << " case " << c.id << " observedIP "
                    << std::hex << observed[i].instruction << " sourceIP "
                    << c.stores[i].instruction << '\n';
          break;
        }
      throw std::runtime_error(
          "Ordered original selection stores differ at case " +
          std::to_string(c.id));
    }
    stores += unsigned(observed.size());
    if (c.entry == 0x1d0a60) {
      auto blocked = c.before;
      auto absent = s;
      absent.resolve_request = {};
      bool stopped = false;
      try {
        select_source_unit_animation(
            blocked,
            {{c.args[0], c.args[1], c.args[2], c.args[3]},
             c.args[4],
             c.args[5]},
            absent);
      } catch (const std::runtime_error &) {
        stopped = true;
      }
      check(stopped,
            "Missing original graph/resource owner fabricated success");
    }
  }
}
#if HALO2_SELECTION_CACHE_TESTS
void owned_cache(const Fixture &f, const char *map) {
  Cache cache(map);
  std::map<std::uint32_t, UnitAnimationGraphMetadata> graphs;
  for (const auto &tag : cache.tags())
    if (tag.group == "jmad" && tag.address)
      graphs.emplace(tag.datum, cache.load_source_unit_animation_graph_metadata(
                                    tag.datum));
  check(graphs.size() == f.graphs.size(), "Owned authored graph count differs");
  SourceAnimationGraphMemory g;
  g.graph_address = [&](std::uint32_t datum) {
    return graphs.at(datum).source_address();
  };
  g.read = [&](std::uint32_t address,
               std::size_t bytes) -> std::span<const std::byte> {
    for (const auto &[_, graph] : graphs)
      if (address >= graph.source_address() &&
          std::uint64_t(address) - graph.source_address() <= graph.size() &&
          bytes <=
              graph.size() - (std::uint64_t(address) - graph.source_address()))
        return graph.bytes_at(address, bytes);
    throw std::runtime_error(
        "Pointer escapes actual Cache-owned graph payloads");
  };
  for (const auto &[datum, address] : f.graphs)
    check(g.graph_address(datum) == address,
          "Cache-owned original graph token differs");
  for (const auto &[address, literal] : f.memory) {
    const auto actual = g.read(address, literal.size());
    check(std::equal(actual.begin(), actual.end(), literal.begin(),
                     literal.end()),
          "Cache-owned graph differs from actual original lookup backing");
  }
  std::map<std::pair<std::uint32_t, std::uint32_t>, UnitAnimationDefinition>
      definitions;
  for (const auto &[key, original] : f.definitions) {
    auto actual =
        cache.load_source_unit_animation_definition(key.first, key.second);
    check(actual.frame_count == original.frame_count &&
              actual.authored_flags == original.authored_flags,
          "Cache-owned actual selector definition fields differ");
    definitions.emplace(key, std::move(actual));
  }
  const auto definition =
      [&](std::uint32_t graph,
          std::uint32_t packed) -> const UnitAnimationDefinition & {
    return definitions.at({graph, packed});
  };
  std::map<std::string, unsigned> counts;
  pure(f, g, counts);
  unsigned leaves = 0, stores = 0;
  kernels(f, leaves, stores, definition);
  std::cout << "Actual Cache-owned graphs=" << graphs.size()
            << " retainedSpans=" << f.memory.size()
            << " stableDefinitions=" << definitions.size()
            << " selectionCases=" << f.cases.size()
            << " explicitOriginalServices=" << leaves
            << " orderedStores=" << stores;
  for (const auto &[name, n] : counts)
    std::cout << ' ' << name << '=' << n;
  std::cout << '\n';
}
#endif
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3,
          "Usage: unit_animation_selection_tests original-fixtures.tsv [map]");
    if (!std::ifstream(argv[1])) {
      std::cout << "Original ignored selection fixture unavailable\n";
      return 77;
    }
    auto f = load(argv[1]);
    std::map<std::string, unsigned> counts;
    pure(f, memory(f), counts);
    unsigned leaves = 0, stores = 0;
    kernels(f, leaves, stores);
    std::cout << "Original selection cases=" << f.cases.size()
              << " requiredServices=" << leaves << " orderedStores=" << stores;
    for (const auto &[name, n] : counts)
      std::cout << ' ' << name << '=' << n;
    std::cout << '\n';
    if (argc == 3) {
#if HALO2_SELECTION_CACHE_TESTS
      owned_cache(f, argv[2]);
#else
      throw std::runtime_error(
          "Optional Cache-owned mode requires CMake target");
#endif
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
