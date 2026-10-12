#include "halo2/unit_alternate_request.hpp"
#ifdef HALO2_ALTERNATE_REQUEST_CACHE
#include "halo2/original_data.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
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
void check(bool b, const std::string &s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t word(std::istringstream &r) {
  std::uint32_t v;
  check(bool(r >> std::hex >> v), "Incomplete alternate fixture");
  return v;
}
Bytes decode(const std::string &s) {
  check(s.size() % 2 == 0, "Incomplete original bytes");
  Bytes b(s.size() / 2);
  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = std::byte(std::stoul(s.substr(i * 2, 2), nullptr, 16));
  return b;
}
std::uint32_t get(std::span<const std::byte> b, std::size_t p, unsigned n = 4) {
  check(p <= b.size() && n <= b.size() - p, "Truncated original field");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (8 * i);
  return v;
}
struct Tag {
  std::uint32_t address;
  Bytes raw;
};
struct Event {
  std::string kind;
  std::vector<std::uint32_t> words;
  friend bool operator==(const Event &, const Event &) = default;
};
struct Case {
  std::string id;
  std::uint32_t entry, result;
  std::vector<std::uint32_t> args;
  State before, after;
  std::map<std::string, std::size_t> locations;
  std::vector<std::array<std::uint32_t, 2>> resources;
  std::vector<AnimationResolutionStore> stores;
  std::vector<Event> events;
};
struct Fixture {
  std::map<std::uint32_t, Tag> tags;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream in(path);
  check(bool(in), "No original alternate fixture");
  Fixture f;
  Case *c = nullptr;
  for (std::string line; std::getline(in, line);) {
    std::istringstream r(line);
    std::string kind;
    r >> kind;
    if (kind.empty() || kind[0] == '#')
      continue;
    if (kind == "TAG") {
      auto d = word(r), p = word(r);
      std::string raw;
      r >> raw;
      f.tags.emplace(d, Tag{p, decode(raw)});
    } else if (kind == "CASE") {
      f.cases.emplace_back();
      c = &f.cases.back();
      r >> c->id;
      c->entry = word(r);
      c->result = word(r);
      auto n = word(r);
      while (n--)
        c->args.push_back(word(r));
    } else if (kind == "LOCATION") {
      std::string name;
      r >> name;
      c->locations.emplace(name, word(r));
    } else if (kind == "BEFORE" || kind == "AFTER") {
      std::string name, raw;
      r >> name >> raw;
      (kind == "BEFORE" ? c->before : c->after).emplace(name, decode(raw));
    } else if (kind == "RESOURCE") {
      auto d = word(r), p = word(r);
      c->resources.push_back({d, p});
    } else if (kind == "STORE") {
      auto ip = word(r);
      std::string k;
      r >> k;
      c->stores.push_back({ip,
                           k == "P" ? AnimationResolutionOutput::packed
                                    : AnimationResolutionOutput::resolved,
                           word(r), word(r), word(r), word(r)});
    } else if (kind == "EVENT") {
      Event e;
      r >> e.kind;
      if (e.kind == "STORE") {
        auto ip = word(r);
        std::string k;
        r >> k;
        e.words = {ip, k == "P" ? 1u : 2u, word(r), word(r), word(r), word(r)};
      } else if (e.kind == "EXIT") {
        for (int i = 0; i < 6; ++i)
          e.words.push_back(word(r));
        std::string raw;
        r >> raw;
        e.words.push_back(get(decode(raw), 0));
      } else
        for (std::string v; r >> v;)
          e.words.push_back(std::stoul(v, nullptr, 16));
      c->events.push_back(std::move(e));
    } else
      check(kind == "ENDCASE", "Unknown alternate fixture record");
  }
  return f;
}
SourceAnimationGraphMemory literal(const Fixture &f) {
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
            throw std::runtime_error("Unowned graph backing");
          }};
}
template <class F> void rejects(F &&fn) {
  bool caught = false;
  try {
    fn();
  } catch (const std::exception &) {
    caught = true;
  }
  check(caught, "Missing actual owner became successful");
}
void verify(const Fixture &f, const SourceAnimationGraphMemory &memory,
            const char *label) {
  std::size_t stores_total = 0, resource_total = 0, positive = 0, calls = 0;
  for (const auto &c : f.cases) {
    auto state = c.before;
    auto visible = [&](const std::string &name) -> std::span<std::byte> {
      if (c.locations.contains(name + "_U"))
        return std::span<std::byte>(state.at("U"))
            .subspan(c.locations.at(name + "_U"), state.at(name).size());
      return state.at(name);
    };
    auto compare = [&](const State &expected) {
      for (const auto &[k, b] : expected) {
        auto got = visible(k);
        check(got.size() == b.size() &&
                  std::equal(got.begin(), got.end(), b.begin()),
              c.id + " Complete retained buffer differs: " + k);
      }
    };
    compare(c.before);
    std::vector<Event> events;
    std::vector<AnimationResolutionStore> stores;
    std::size_t next_resource = 0;
    auto emit = [&](std::string kind, std::vector<std::uint32_t> w) {
      events.push_back({std::move(kind), std::move(w)});
    };
    SourceAnimationResourceServices s{};
    s.state = [&] {
      auto g = visible("G");
      return std::array<std::uint8_t, 2>{std::uint8_t(get(g, 0, 1)),
                                         std::uint8_t(get(g, 1, 1))};
    };
    s.request = [&](std::uint32_t d, std::uint8_t demand) {
      check(next_resource < c.resources.size() &&
                c.resources[next_resource++] ==
                    std::array<std::uint32_t, 2>{d, demand},
            c.id + " Ordered resource demand differs");
      emit("RESOURCE", {d, demand});
      SourceAnimationDemandServices lower{};
      lower.bindings.active_byte = 0x518411;
      lower.memory.read = [&](std::uint32_t p,
                              std::size_t n) -> std::span<const std::byte> {
        auto g = visible("G");
        check(p >= 0x518410 && std::uint64_t(p) - 0x518410 <= g.size() &&
                  n <= g.size() - (p - 0x518410),
              "Actual resource queue/residency owner unavailable");
        return g.subspan(p - 0x518410, n);
      };
      request_source_animation_resource(d, demand, lower);
    };
    if (c.entry == 0x1df670) {
      SourceAnimationIdentifiers ids{c.args[1], c.args[2], c.args[3],
                                     c.args[4]};
      std::array<std::span<std::byte>, 3> optional{};
      for (unsigned i = 0; i < 3; ++i)
        if (state.contains("O" + std::to_string(i)))
          optional[i] = visible("O" + std::to_string(i));
      lookup_source_alternate_animation_request(
          c.args[0], ids, visible("P"), optional, memory, s,
          [&](const AnimationResolutionStore &v) {
            stores.push_back(v);
            emit("STORE",
                 {v.instruction,
                  v.output == AnimationResolutionOutput::packed ? 1u : 2u,
                  std::uint32_t(v.offset), v.bytes, v.before, v.after});
          });
      positive += (get(visible("P"), 0) >> 16) != 65535;
      rejects([&] {
        lookup_source_alternate_animation_request(
            c.args[0], ids, visible("P").first(2), optional, memory, s);
      });
      if (!c.resources.empty()) {
        auto missing = s;
        missing.request = {};
        auto output = c.before.at("P");
        rejects([&] {
          lookup_source_alternate_animation_request(c.args[0], ids, output,
                                                    optional, memory, missing);
        });
      }
    } else {
      auto available = source_unit_alternate_animation_available(
          visible("M"), memory, s,
          [&](const SourceAlternateAvailabilityCall &v) {
            std::vector<std::uint32_t> w{v.instruction, v.graph_address};
            w.insert(w.end(), v.identifiers.begin(), v.identifiers.end());
            if (v.packed) {
              w.push_back(*v.packed);
              emit("EXIT", w);
            } else
              emit("ENTER", w);
            ++calls;
          });
      check(unsigned(available) == c.result,
            c.id + " Defined availabilityAL differs");
      auto missing = memory;
      missing.graph_address = {};
      rejects([&] {
        source_unit_alternate_animation_available(visible("M"), missing, s);
      });
    }
    compare(c.after);
    check(stores == c.stores, c.id + " Ordered full output stores differ");
    check(next_resource == c.resources.size(),
          c.id + " Missing original resource demand");
    if (events != c.events) {
      std::size_t n = 0;
      while (n < std::min(events.size(), c.events.size()) &&
             events[n] == c.events[n])
        ++n;
      throw std::runtime_error(c.id +
                               " Combined callback/store order differs at " +
                               std::to_string(n));
    }
    stores_total += stores.size();
    resource_total += next_resource;
  }
  std::cout << label << " cases=" << f.cases.size()
            << " positiveRequests=" << positive
            << " outputStores=" << stores_total
            << " resourceCalls=" << resource_total
            << " availabilityBoundaries=" << calls << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3 || argc == 4,
          "Supply fixture, optional Lockout cache and verified updateimage");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    auto f = load(argv[1]);
    check(f.cases.size() == 26, "Expected26 complete original cases");
    verify(f, literal(f), "literal");
#ifdef HALO2_ALTERNATE_REQUEST_CACHE
    if (argc >= 3) {
      auto cache = std::make_shared<Cache>(argv[2]);
      UnitAnimationMetadataStore store(cache);
      auto memory = store.graph_memory();
      for (const auto &[d, t] : f.tags) {
        check(memory.graph_address(d) == t.address,
              "Cache graph token differs");
        auto b = memory.read(t.address, t.raw.size());
        check(std::equal(b.begin(), b.end(), t.raw.begin()),
              "Actual full Cache graph bytes differ");
      }
      if (argc == 4) {
        auto image = OriginalData::load_xbe(argv[3]);
        check(image->image == OriginalData::Image::MapPack11 ||
                  image->image == OriginalData::Image::TitleUpdate15,
              "Unverified alternate-owner image");
      }
      verify(f, memory,
             argc == 4 ? "actualCache+verifiedImage" : "actualCache");
    }
#else
    check(argc == 2, "Cache mode not compiled");
#endif
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
