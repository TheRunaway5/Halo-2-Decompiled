#include "halo2/unit_constructor_tail.hpp"
#ifdef HALO2_CONSTRUCTOR_TAIL_CACHE
#include "halo2/original_data.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

namespace {
void check(bool v, const std::string &s) {
  if (!v)
    throw std::runtime_error(s);
}
std::uint32_t num(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> decode(const std::string &s) {
  check(s.size() % 2 == 0, "Incomplete literal source buffer");
  std::vector<std::byte> b(s.size() / 2);
  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = std::byte(num(s.substr(i * 2, 2)));
  return b;
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  check(at <= b.size() && n <= b.size() - at, "Test-owned buffer extent");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
template <class F> void rejects(F &&f) {
  bool caught = false;
  try {
    f();
  } catch (const std::exception &) {
    caught = true;
  }
  check(caught, "Missing source owner did not stop");
}
using State = std::map<std::string, std::vector<std::byte>>;
using Store = halo2::UnitConstructorTailStore;
struct Tag {
  std::uint32_t address;
  std::vector<std::byte> raw;
};
struct Leaf {
  halo2::UnitConstructorTailCall call;
  unsigned result;
  State before, after;
};
struct Case {
  std::string label;
  std::uint32_t entry, unit, capacity, result;
  std::vector<std::uint32_t> args;
  State before, after;
  std::vector<Leaf> leaves;
  std::vector<Store> stores;
  std::optional<std::uint8_t> gate;
};
void run(const Case &c, const halo2::UnitConstructorTailConstants &constants,
         const halo2::UnitConstructorTailMetadata &tags,
         const halo2::SourceAnimationGraphMemory &graphs, unsigned &callbacks,
         unsigned &store_count) {
  auto state = c.before;
  std::vector<std::byte> headers(std::size_t(c.capacity) * 12);
  const auto hat = std::size_t(c.unit & 65535) * 12;
  if (state.contains("H"))
    std::copy(state.at("H").begin(), state.at("H").end(),
              headers.begin() + hat);
  auto visible = [&](const std::string &name) -> std::span<const std::byte> {
    if (name == "H")
      return std::span<const std::byte>(headers).subspan(hat, 12);
    if (name == "M") {
      auto at = read(state.at("U"), 0x12a, 2);
      return std::span<const std::byte>(state.at("U")).subspan(at, 0x90);
    }
    return state.at(name);
  };
  auto compare = [&](const State &expected, const std::string &where) {
    for (const auto &[name, b] : expected) {
      auto v = visible(name);
      check(v.size() == b.size() && std::equal(v.begin(), v.end(), b.begin()),
            c.label + " " + where + " " + name);
    }
  };
  auto apply = [&](const State &after) {
    for (const auto &[name, b] : after) {
      if (name == "H")
        std::copy(b.begin(), b.end(), headers.begin() + hat);
      else if (name == "M") {
        auto at = read(state.at("U"), 0x12a, 2);
        check(b.size() == 0x90, "Original manager extent");
        std::copy(b.begin(), b.end(), state.at("U").begin() + at);
      } else {
        check(state.at(name).size() == b.size(),
              "Fixture callback cannot relocate retained constructor backing");
        std::copy(b.begin(), b.end(), state.at(name).begin());
      }
    }
  };
  halo2::UnitConstructorTailServices s{};
  s.constants = constants;
  s.tags = tags;
  s.graphs = graphs;
  s.objects.headers = headers;
  s.objects.capacity = c.capacity;
  s.objects.body = [&](std::uint16_t i) -> std::span<std::byte> {
    check(i == std::uint16_t(c.unit), "Original low16 object pool identity");
    return state.at("U");
  };
  unsigned gate_reads = 0;
  s.player_transition_gate = [&] {
    check(c.gate.has_value(), "Unobserved runtime gate read");
    ++gate_reads;
    return *c.gate;
  };
  std::size_t next = 0;
  auto callback = [&](const halo2::UnitConstructorTailCall &packet) {
    check(next < c.leaves.size(), c.label + " Extra lower callback");
    const auto &l = c.leaves[next++];
    check(packet == l.call,
          c.label + " Exact callback ABI/order " + std::to_string(next));
    compare(l.before, "lower entry " + std::to_string(next));
    // Test-only actual original output. This does not implement or default the
    // lower resource/subchannel/root-transform/action-cache owner in
    // production.
    apply(l.after);
    ++callbacks;
    return std::uint8_t(l.result);
  };
  s.invoke = [&](std::span<std::byte> manager,
                 const halo2::UnitConstructorTailCall &packet) {
    auto m = read(state.at("U"), 0x12a, 2);
    check(manager.data() == state.at("U").data() + m && manager.size() == 0x90,
          "Actual retained manager span");
    return callback(packet);
  };
  s.resource = [&](std::uint32_t graph, std::uint32_t packed) {
    callback({0x1e13c0, c.unit, 0, false, {graph, packed, 0, 0, 0}, 2});
  };
  std::vector<Store> stores;
  auto observer = [&](const Store &v) { stores.push_back(v); };
  std::uint32_t result = c.result;
  switch (c.entry) {
  case 0x110800:
    halo2::reset_source_unit_subchannels(c.unit, s, observer);
    break;
  case 0x1100e0:
    halo2::reset_source_unit_named_animation(c.unit, s, observer);
    break;
  case 0x10f380:
    halo2::reset_source_unit_aim_animation(c.unit, s, observer);
    break;
  case 0x115030:
    halo2::reset_source_unit_action_cache(c.unit, s, observer);
    break;
  case 0x110a20:
    halo2::initialize_source_unit_extension(c.unit, s, observer);
    break;
  case 0x116680:
    halo2::initialize_source_unit_variant_metadata(c.unit, c.args.at(0), s,
                                                   observer);
    break;
  case 0x110bf0:
    result = halo2::bind_source_unit_activity(c.unit,
                                              {c.args[0], c.args[1], c.args[2],
                                               c.args[3], c.args[4], c.args[5],
                                               std::uint8_t(c.args[6])},
                                              s, observer);
    break;
  case 0x16f510:
    result = halo2::find_source_model_node(c.args[0], c.args[1], tags);
    break;
  case 0x1e0aa0:
    halo2::find_source_named_animation(c.args[0], c.args[1], state.at("P"), s,
                                       observer);
    break;
  case 0x114f70:
    result = halo2::source_unit_transition_duration(
        c.args[0], c.args[1], std::uint8_t(c.args[2]), std::uint8_t(c.args[3]),
        c.args[4], constants);
    break;
  default:
    throw std::runtime_error("Unimplemented original case");
  }
  check(result == c.result, c.label + " Defined return differs");
  compare(c.after, "complete output");
  check(next == c.leaves.size(), c.label + " Missing lower callbacks");
  check(gate_reads == unsigned(c.gate.has_value()),
        c.label + " Runtime gate ownership");
  if (stores != c.stores) {
    auto n = std::min(stores.size(), c.stores.size());
    std::size_t i = 0;
    while (i < n && stores[i] == c.stores[i])
      ++i;
    std::ostringstream why;
    why << c.label << " Ordered stores differ at " << i << " sizes "
        << stores.size() << "/" << c.stores.size();
    if (i < stores.size())
      why << " actual " << std::hex << stores[i].instruction << ":"
          << stores[i].offset;
    if (i < c.stores.size())
      why << " expected " << std::hex << c.stores[i].instruction << ":"
          << c.stores[i].offset;
    throw std::runtime_error(why.str());
  }
  store_count += stores.size();
  // Host stop checks reuse actual source-created records; no manufactured
  // constructor bytes, component offsets or successful lower results.
  if (c.entry == 0x110bf0) {
    auto absent = s;
    absent.invoke = {};
    rejects([&] {
      (void)halo2::bind_source_unit_activity(c.unit,
                                             {c.args[0], c.args[1], c.args[2],
                                              c.args[3], c.args[4], c.args[5],
                                              std::uint8_t(c.args[6])},
                                             absent);
    });
  }
  if (c.entry == 0x110800) {
    auto absent = s;
    absent.objects.headers = {};
    rejects([&] { halo2::reset_source_unit_subchannels(c.unit, absent); });
  }
  if (c.entry == 0x16f510) {
    auto absent = tags;
    absent.read = {};
    rejects([&] {
      (void)halo2::find_source_model_node(c.args[0], c.args[1], absent);
    });
  }
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3 || argc == 4,
          "Supply constructor fixture, optional Lockout cache and selected "
          "update image");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    std::ifstream in(argv[1]);
    std::map<std::uint32_t, Tag> packets;
    std::vector<Case> cases;
    halo2::UnitConstructorTailConstants constants{};
    Case *current = nullptr;
    Leaf *leaf = nullptr;
    for (std::string line; std::getline(in, line);) {
      std::istringstream row(line);
      std::string kind;
      row >> kind;
      auto word = [&]() {
        std::string s;
        check(bool(row >> s), "Incomplete literal fixture row");
        return num(s);
      };
      if (kind == "TAG") {
        auto datum = word(), at = word();
        std::string raw;
        row >> raw;
        packets.emplace(datum, Tag{at, decode(raw)});
      } else if (kind == "CONSTANTS") {
        constants.one_bits = word();
        constants.zero_bits = word();
        constants.normal_blend_bits = word();
        constants.special_blend_bits = word();
        for (auto &p : constants.activity_pairs) {
          p[0] = word();
          p[1] = word();
        }
      } else if (kind == "CASE") {
        cases.emplace_back();
        current = &cases.back();
        row >> current->label;
        current->entry = word();
        current->unit = word();
        current->capacity = word();
        current->result = word();
        auto n = word();
        for (unsigned i = 0; i < n; ++i)
          current->args.push_back(word());
      } else if (kind == "BEFORE" || kind == "AFTER" || kind == "LBEFORE" ||
                 kind == "LAFTER") {
        std::string name, raw;
        row >> name >> raw;
        auto &target = kind == "BEFORE"    ? current->before
                       : kind == "AFTER"   ? current->after
                       : kind == "LBEFORE" ? leaf->before
                                           : leaf->after;
        target.emplace(name, decode(raw));
      } else if (kind == "LEAF") {
        current->leaves.emplace_back();
        leaf = &current->leaves.back();
        leaf->call.entry = word();
        leaf->call.unit = word();
        leaf->call.eax = word();
        leaf->call.eax_input = word() != 0;
        leaf->result = word();
        leaf->call.count = word();
        check(leaf->call.count <= 5, "Oversized lower packet");
        for (unsigned i = 0; i < leaf->call.count; ++i)
          leaf->call.arguments[i] = word();
      } else if (kind == "STORE") {
        Store s{};
        s.instruction = word();
        std::string b;
        row >> b;
        check(b == "U" || b == "P", "Unknown source store buffer");
        s.buffer = b == "U" ? halo2::UnitConstructorTailBuffer::Unit
                            : halo2::UnitConstructorTailBuffer::Packed;
        s.offset = word();
        s.bytes = word();
        s.before = word();
        s.after = word();
        current->stores.push_back(s);
      } else if (kind == "GATE")
        current->gate = std::uint8_t(word());
    }
    halo2::UnitConstructorTailMetadata tags;
    tags.address = [&](std::uint32_t d) { return packets.at(d).address; };
    tags.read = [&](std::uint32_t at,
                    std::size_t n) -> std::span<const std::byte> {
      const Tag *owner = nullptr;
      for (const auto &[datum, t] : packets)
        if (at >= t.address && std::uint64_t(at) - t.address <= t.raw.size() &&
            n <= t.raw.size() - (std::uint64_t(at) - t.address)) {
          check(!owner, "Ambiguous literal source ownership");
          owner = &t;
        }
      check(owner, "Missing literal source backing");
      return std::span<const std::byte>(owner->raw)
          .subspan(at - owner->address, n);
    };
    halo2::SourceAnimationGraphMemory graphs{tags.address, tags.read};
    bool actual_cache = false;
#ifdef HALO2_CONSTRUCTOR_TAIL_CACHE
    if (argc >= 3) {
      auto cache = std::make_shared<halo2::Cache>(argv[2]);
      halo2::SourceTagMetadataStore store(cache, {"bipd", "hlmt", "udlg"});
      auto p = store.providers();
      tags = {p.address, p.read};
      halo2::UnitAnimationMetadataStore animations(cache);
      graphs = animations.graph_memory();
      actual_cache = true;
    }
    if (argc == 4) {
      auto original = halo2::OriginalData::load_xbe(argv[3]);
      check(original->unit_constructor_tail_constants.has_value(),
            "Unverified image cannot supply constructor operands");
      const auto &owned = *original->unit_constructor_tail_constants;
      check(owned.one_bits == constants.one_bits &&
                owned.zero_bits == constants.zero_bits &&
                owned.normal_blend_bits == constants.normal_blend_bits &&
                owned.special_blend_bits == constants.special_blend_bits &&
                owned.activity_pairs == constants.activity_pairs,
            "Selected original image operands differ from owning source");
      constants = owned;
    }
#else
    check(argc == 2,
          "Actual cache mode requires the registered provider build");
#endif
    unsigned callbacks = 0, stores = 0;
    for (const auto &c : cases) {
      try {
        run(c, constants, tags, graphs, callbacks, stores);
      } catch (const std::exception &e) {
        throw std::runtime_error("Original case " + c.label + ": " + e.what());
      }
    }
    check(cases.size() == 14, "Incomplete owning source case coverage");
    std::cout << "PASS " << cases.size() << " complete source buffers, "
              << callbacks << " required original callbacks, " << stores
              << " ordered stores; "
              << (actual_cache ? "actual Cache-owned cross-tag/graph providers"
                               : "literal ignored source packets")
              << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
