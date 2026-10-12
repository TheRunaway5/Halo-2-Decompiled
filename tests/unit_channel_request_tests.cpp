#include "halo2/animation_variant.hpp"
#include "halo2/unit_channel_request.hpp"
#ifdef HALO2_CHANNEL_REQUEST_CACHE
#include "halo2/original_data.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

namespace {
using namespace halo2;
using Bytes = std::vector<std::byte>;
using State = std::map<std::string, Bytes>;
using Store = UnitAnimationSelectionStore;
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
std::uint32_t word(std::istringstream &row) {
  std::uint32_t v;
  check(bool(row >> std::hex >> v), "Incomplete original request fixture");
  return v;
}
Bytes decode(const std::string &raw) {
  if (raw == "-")
    return {};
  check(raw.size() % 2 == 0, "Incomplete original buffer");
  Bytes b(raw.size() / 2);
  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = std::byte(std::stoul(raw.substr(i * 2, 2), nullptr, 16));
  return b;
}
std::uint32_t get(std::span<const std::byte> b, std::size_t p, unsigned n = 4) {
  check(p <= b.size() && n <= b.size() - p, "Incomplete original field");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (i * 8);
  return v;
}
void put(std::span<std::byte> b, std::size_t p, std::uint32_t v) {
  check(p <= b.size() && 4 <= b.size() - p, "Incomplete retained field");
  for (unsigned i = 0; i < 4; ++i)
    b[p + i] = std::byte((v >> (i * 8)) & 255);
}
struct Tag {
  std::uint32_t address;
  Bytes raw;
};
struct Lower {
  std::uint32_t entry, result{};
  std::vector<std::uint32_t> args;
  State before, after;
};
struct Event {
  std::string kind;
  std::vector<std::uint32_t> words;
  friend bool operator==(const Event &, const Event &) = default;
};
struct Case {
  std::string id;
  std::uint32_t entry, result, fpcw, mxcsr;
  std::vector<std::uint32_t> args;
  State before, after;
  std::map<std::string, std::size_t> locations;
  Bytes clock;
  std::vector<Lower> lower;
  std::vector<Store> stores;
  std::vector<std::array<std::uint32_t, 2>> resources;
  std::vector<Event> events;
};
struct Fixture {
  std::map<std::uint32_t, Tag> tags;
  std::map<std::pair<std::uint32_t, std::uint32_t>, UnitAnimationDefinition>
      definitions;
  std::array<std::uint32_t, 5> constants;
  std::vector<Case> cases;
};
UnitAnimationDefinition definition(std::istringstream &row) {
  std::array<Bytes, 6> b;
  for (auto &v : b) {
    std::string raw;
    check(bool(row >> raw), "Incomplete source definition");
    v = decode(raw);
  }
  UnitAnimationDefinition d;
  d.frame_count =
      std::bit_cast<std::int16_t>(std::uint16_t(get(b[0], 0x14, 2)));
  d.loop_frame = std::bit_cast<std::int16_t>(std::uint16_t(get(b[0], 0x2e, 2)));
  d.authored_flags = std::uint8_t(get(b[0], 0x18, 1));
  auto short_at = [](const Bytes &v, std::size_t p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(get(v, p, 2)));
  };
  for (std::size_t p = 0; p < b[1].size(); p += 4)
    d.timing.push_back({short_at(b[1], p), short_at(b[1], p + 2)});
  for (std::size_t p = 0; p < b[2].size(); p += 8)
    d.sounds.push_back(
        {short_at(b[2], p), short_at(b[2], p + 2), get(b[2], p + 4)});
  for (std::size_t p = 0; p < b[3].size(); p += 4)
    d.effects.push_back({short_at(b[3], p), short_at(b[3], p + 2)});
  for (std::size_t p = 0; p < b[4].size(); p += 12)
    d.sound_references.push_back(
        {get(b[4], p + 4), std::uint16_t(get(b[4], p + 8, 2))});
  for (std::size_t p = 0; p < b[5].size(); p += 12)
    d.effect_references.push_back(
        {get(b[5], p + 4), std::uint16_t(get(b[5], p + 8, 2))});
  return d;
}
Fixture load(const char *path) {
  std::ifstream file(path);
  check(bool(file), "Original request fixture missing");
  Fixture f;
  Case *c = nullptr;
  Lower *l = nullptr;
  for (std::string line; std::getline(file, line);) {
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind.empty() || kind[0] == '#')
      continue;
    auto bytes = [&] {
      std::string raw;
      check(bool(row >> raw), "Missing raw buffer");
      return decode(raw);
    };
    if (kind == "TAG") {
      auto d = word(row), a = word(row);
      f.tags.emplace(d, Tag{a, bytes()});
    } else if (kind == "DEF") {
      auto d = word(row), p = word(row);
      f.definitions.emplace(std::pair{d, p}, definition(row));
    } else if (kind == "CONSTANTS") {
      for (auto &v : f.constants)
        v = word(row);
    } else if (kind == "CASE") {
      f.cases.emplace_back();
      c = &f.cases.back();
      row >> c->id;
      c->entry = word(row);
      c->result = word(row);
      auto n = word(row);
      while (n--)
        c->args.push_back(word(row));
    } else if (kind == "CLOCK") {
      c->clock = bytes();
      c->fpcw = word(row);
      c->mxcsr = word(row);
    } else if (kind == "LOCATION") {
      std::string name;
      row >> name;
      c->locations.emplace(name, word(row));
    } else if (kind == "BEFORE" || kind == "AFTER" || kind == "LBEFORE" ||
               kind == "LAFTER") {
      std::string name;
      row >> name;
      auto &state = kind == "BEFORE"    ? c->before
                    : kind == "AFTER"   ? c->after
                    : kind == "LBEFORE" ? l->before
                                        : l->after;
      state.emplace(name, bytes());
    } else if (kind == "LOWER") {
      c->lower.emplace_back();
      l = &c->lower.back();
      l->entry = word(row);
      auto n = word(row);
      while (n--)
        l->args.push_back(word(row));
    } else if (kind == "RESULT") {
      l->result = get(bytes(), 0);
    } else if (kind == "RESOURCE") {
      auto d = word(row), p = word(row);
      c->resources.push_back({d, p});
    } else if (kind == "STORE") {
      c->stores.push_back(
          {word(row), word(row), word(row), word(row), word(row)});
    } else if (kind == "EVENT") {
      Event e;
      row >> e.kind;
      for (std::string v; row >> v;)
        e.words.push_back(std::stoul(v, nullptr, 16));
      c->events.push_back(std::move(e));
    } else
      check(kind == "ENDCASE" || kind == "ENDLOWER",
            "Unknown fixture record: " + kind);
  }
  return f;
}
SourceAnimationGraphMemory literal_memory(const Fixture &f) {
  return {[&](std::uint32_t d) { return f.tags.at(d).address; },
          [&](std::uint32_t p, std::size_t n) -> std::span<const std::byte> {
            for (const auto &[d, t] : f.tags) {
              (void)d;
              if (p >= t.address) {
                auto off = std::uint64_t(p) - t.address;
                if (off <= t.raw.size() && n <= t.raw.size() - off)
                  return std::span<const std::byte>(t.raw).subspan(
                      std::size_t(off), n);
              }
            }
            throw std::runtime_error("Unowned original graph range");
          }};
}
template <class F> void rejects(F &&fn) {
  bool caught = false;
  try {
    fn();
  } catch (const std::exception &) {
    caught = true;
  }
  check(caught, "Missing owner became successful");
}
void verify(const Fixture &f, const SourceAnimationGraphMemory &memory,
            const std::function<const UnitAnimationDefinition &(
                std::uint32_t, std::uint32_t)> &definitions,
            const std::array<std::uint32_t, 5> &constants, const char *label) {
  std::size_t total_stores = 0, total_lower = 0, total_resources = 0;
  for (const auto &c : f.cases) {
    auto state = c.before;
    std::function<std::span<std::byte>(const std::string &)> visible;
    visible = [&](const std::string &name) -> std::span<std::byte> {
      for (const auto &parent : {std::string("U"), std::string("M")}) {
        auto it = c.locations.find(name + "_" + parent);
        if (it != c.locations.end())
          return visible(parent).subspan(it->second, state.at(name).size());
      }
      return state.at(name);
    };
    auto compare = [&](const State &expected) {
      for (const auto &[name, b] : expected) {
        auto got = visible(name);
        check(got.size() == b.size() &&
                  std::equal(got.begin(), got.end(), b.begin()),
              c.id + " full buffer differs: " + name);
      }
    };
    compare(c.before);
    check(c.clock.size() == 12 && (c.mxcsr & 0xffc0) == 0x1f80 &&
              (c.fpcw & 0xc00) == 0,
          "Unsupported measured clock/FPU provider");
    std::vector<Store> stores;
    std::vector<Event> events;
    std::size_t next_lower = 0, next_resource = 0;
    auto emit = [&](std::string kind, std::vector<std::uint32_t> words) {
      events.push_back({std::move(kind), std::move(words)});
    };
    SourceAnimationResourceServices resources;
    resources.state = [&] {
      auto g = visible("G");
      return std::array<std::uint8_t, 2>{std::uint8_t(get(g, 0, 1)),
                                         std::uint8_t(get(g, 1, 1))};
    };
    resources.request = [&](std::uint32_t d, std::uint8_t demand) {
      const auto resource_index = next_resource++;
      if (resource_index >= c.resources.size() ||
          c.resources[resource_index] !=
              std::array<std::uint32_t, 2>{d, demand}) {
        std::ostringstream message;
        message << c.id << " ordered resource request differs at "
                << resource_index << " actual " << std::hex << d << ":"
                << unsigned(demand);
        if (resource_index < c.resources.size())
          message << " expected " << c.resources[resource_index][0] << ":"
                  << c.resources[resource_index][1];
        throw std::runtime_error(message.str());
      }
      emit("RESOURCE", {d, demand});
      SourceAnimationDemandServices required{};
      required.bindings.active_byte = 0x518411;
      required.memory.read = [&](std::uint32_t address,
                                 std::size_t n) -> std::span<const std::byte> {
        auto g = visible("G");
        check(address >= 0x518410 &&
                  std::uint64_t(address) - 0x518410 <= g.size() &&
                  n <= g.size() - (address - 0x518410),
              "Live resource queue/cache backing remains unavailable");
        return g.subspan(address - 0x518410, n);
      };
      // Execute complete124E40 against the actual recorded inactive
      // global byte. No allocator/queue/IO success callback is supplied.
      request_source_animation_resource(d, demand, required);
    };
    auto enter = [&](std::uint32_t entry) -> const Lower & {
      check(next_lower < c.lower.size(), c.id + " Extra lower owner");
      const auto &l = c.lower[next_lower++];
      check(l.entry == entry, c.id + " Lower owner order differs");
      emit("ENTER", {entry});
      compare(l.before);
      return l;
    };
    auto exit = [&](const Lower &l) {
      compare(l.after);
      emit("EXIT", {l.entry});
    };
    SourceUnitChannelRequestServices s{};
    s.graphs = memory;
    s.resources = resources;
    s.one_bits = constants[0];
    s.variant = [&](std::span<std::byte> m, std::uint32_t packed) {
      const auto &l = enter(0x1ceda0);
      check(l.args == std::vector<std::uint32_t>{packed},
            c.id + " Variant ABI differs");
      auto r = visible("R");
      std::array<std::uint32_t, 2> rng{get(r, 0), get(r, 4)};
      AnimationVariantServices v{
          memory,
          {constants[0], constants[3], constants[4]},
          [&](std::uint32_t graph, std::uint32_t p) {
            demand_source_animation(graph, p, memory, resources);
          },
          [&](const AnimationVariantRngStore &v) {
            put(r, 4 * v.stream, v.after);
            emit("RNG", {v.instruction, v.stream, v.before, v.after});
          }};
      const auto selected = select_source_animation_variant(m, packed, rng, v);
      put(r, 0, rng[0]);
      put(r, 4, rng[1]);
      check(selected == l.result, c.id + " Variant result differs");
      exit(l);
      return selected;
    };
    s.clamp_frame = [&](std::span<std::byte> ch, std::uint32_t frame) {
      const auto &l = enter(0x1c9f50);
      check(l.args == std::vector<std::uint32_t>{frame},
            c.id + " Clamp ABI differs");
      UnitAnimationServices a{};
      a.tick_seconds = std::bit_cast<float>(get(c.clock, 4));
      a.definition = definitions;
      clamp_source_unit_animation_frame(ch, std::bit_cast<float>(frame), a);
      exit(l);
    };
    auto observer = [&](const Store &v) {
      stores.push_back(v);
      emit("STORE", {v.instruction, std::uint32_t(v.offset), v.bytes, v.before,
                     v.after});
    };
    bool result;
    if (c.entry == 0x1cec20)
      result = update_source_unit_channel_request(
          visible("M"), visible("C"), c.args.at(0), c.args.at(1), s, observer);
    else if (c.entry == 0x1cee00)
      result = install_source_unit_requested_channel(
          visible("M"), visible("C"), c.args.at(0), c.args.at(1), c.args.at(2),
          c.args.at(3), c.args.at(4), c.args.at(5), s, observer);
    else
      throw std::runtime_error("Unknown direct owner");
    check(unsigned(result) == c.result, c.id + " DefinedAL differs");
    compare(c.after);
    check(next_lower == c.lower.size() && next_resource == c.resources.size(),
          c.id + " Missing required lower invocation");
    check(stores == c.stores, c.id + " Ordered direct/install stores differ");
    if (events != c.events) {
      std::size_t n = 0;
      while (n < std::min(events.size(), c.events.size()) &&
             events[n] == c.events[n])
        ++n;
      throw std::runtime_error(c.id +
                               " Combined callback/store ordering differs at " +
                               std::to_string(n));
    }
    total_stores += stores.size();
    total_lower += next_lower;
    total_resources += next_resource;
    // Only host boundary checks; no new source-admitted channel or readiness.
    rejects([&] {
      update_source_unit_channel_request(visible("M"), visible("C").first(3),
                                         c.args[0], c.args[1], s);
    });
    if (!c.lower.empty() && c.entry == 0x1cee00) {
      auto restored = c.before;
      auto m = restored.at("M"), ch = restored.at("C");
      auto missing = s;
      missing.variant = {};
      if (c.args[2] & 0x10)
        rejects([&] {
          install_source_unit_requested_channel(m, ch, c.args[0], c.args[1],
                                                c.args[2], c.args[3], c.args[4],
                                                c.args[5], missing);
        });
    }
  }
  std::cout << label << " cases=" << f.cases.size()
            << " completeLowerCalls=" << total_lower
            << " orderedResourceCalls=" << total_resources
            << " orderedStores=" << total_stores << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3 || argc == 4,
          "Supply source fixture, optional Lockout cache and verified image");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    auto f = load(argv[1]);
    check(f.cases.size() == 26, "Expected26 actual complete source calls");
    auto defs = [&](std::uint32_t d,
                    std::uint32_t p) -> const UnitAnimationDefinition & {
      return f.definitions.at({d, p});
    };
    verify(f, literal_memory(f), defs, f.constants, "literal");
#ifdef HALO2_CHANNEL_REQUEST_CACHE
    if (argc >= 3) {
      auto cache = std::make_shared<Cache>(argv[2]);
      UnitAnimationMetadataStore store(cache);
      auto memory = store.graph_memory();
      for (const auto &[d, t] : f.tags) {
        check(memory.graph_address(d) == t.address,
              "Actual Cache graph identity differs");
        auto b = memory.read(t.address, t.raw.size());
        check(std::equal(b.begin(), b.end(), t.raw.begin()),
              "Actual Cache graph payload differs");
      }
      auto constants = f.constants;
      if (argc == 4) {
        auto image = OriginalData::load_xbe(argv[3]);
        check(bool(image->unit_animation_constants),
              "Missing verified source constants");
        const auto &c = *image->unit_animation_constants;
        constants = {c.one_bits, c.zero_bits, c.frame_subtractor_bits,
                     c.rng_scale_bits, c.unsigned_conversion_correction_bits};
        check(constants == f.constants, "Imported executable operands differ");
      }
      verify(f, memory, store.definition_provider(), constants,
             argc == 4 ? "actualCache+image" : "actualCache");
    }
#else
    check(argc == 2, "Actual Cache mode not compiled");
#endif
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
