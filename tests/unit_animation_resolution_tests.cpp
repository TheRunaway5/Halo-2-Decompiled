#include "halo2/unit_animation_resolution.hpp"
#if HALO2_RESOLUTION_CACHE_TESTS
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using Bytes = std::vector<std::byte>;
void check(bool ok, const std::string &why) {
  if (!ok)
    throw std::runtime_error(why);
}
std::uint32_t hex(std::istringstream &in) {
  std::uint32_t v;
  check(bool(in >> std::hex >> v), "Missing fixture word");
  return v;
}
Bytes decode(const std::string &raw) {
  check(raw.size() % 2 == 0, "Partial original byte buffer");
  Bytes b;
  for (std::size_t i = 0; i < raw.size(); i += 2)
    b.push_back(std::byte(std::stoul(raw.substr(i, 2), nullptr, 16)));
  return b;
}
struct Event {
  bool state;
  std::uint32_t descriptor, demand, source_case;
  friend bool operator==(const Event &, const Event &) = default;
};
struct Service {
  std::uint32_t entry, datum, pool;
  std::map<std::uint32_t, Bytes> before, after;
};
struct Case {
  std::uint32_t id, entry, result;
  std::vector<std::uint32_t> args;
  Bytes manager, gates, descriptor_before, descriptor_after;
  std::array<Bytes, 3> before, after;
  std::vector<Event> events;
  std::vector<AnimationResolutionStore> stores;
  std::map<std::uint32_t, Bytes> runtime_before, runtime_after;
  std::vector<Service> services;
  std::vector<AnimationResourceStore> resource_stores;
  bool has_queued_result = false;
  std::uint32_t queued_result = 0;
};
struct Fixture {
  std::map<std::uint32_t, std::uint32_t> graphs;
  std::map<std::uint32_t, Bytes> memory;
  std::vector<Case> cases;
};
Fixture load(const char *file) {
  std::ifstream in(file);
  check(bool(in), "Cannot open ignored original resolution fixture");
  Fixture f;
  Case *c = nullptr;
  Service *service = nullptr;
  for (std::string line; std::getline(in, line);) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream row(line);
    std::string kind, raw;
    row >> kind;
    if (kind == "MEM") {
      auto at = hex(row);
      row >> raw;
      f.memory.emplace(at, decode(raw));
    } else if (kind == "GRAPH") {
      auto datum = hex(row), at = hex(row);
      f.graphs.emplace(datum, at);
    } else if (kind == "CASE") {
      Case next;
      next.id = hex(row);
      next.entry = hex(row);
      next.result = hex(row);
      auto n = hex(row);
      while (n--)
        next.args.push_back(hex(row));
      f.cases.push_back(std::move(next));
      c = &f.cases.back();
    } else if (kind == "END")
      c = nullptr;
    else {
      check(c != nullptr, "Fixture row outside case");
      if (kind == "MANAGER" || kind == "GATES" || kind == "DESCRIPTOR_BEFORE" ||
          kind == "DESCRIPTOR_AFTER") {
        row >> raw;
        auto b = decode(raw);
        if (kind == "MANAGER")
          c->manager = b;
        if (kind == "GATES")
          c->gates = b;
        if (kind == "DESCRIPTOR_BEFORE")
          c->descriptor_before = b;
        if (kind == "DESCRIPTOR_AFTER")
          c->descriptor_after = b;
      } else if (kind == "BEFORE" || kind == "AFTER") {
        std::string name;
        row >> name >> raw;
        const unsigned field = name == "requested"  ? 0
                               : name == "resolved" ? 1
                                                    : 2;
        (kind == "BEFORE" ? c->before : c->after)[field] = decode(raw);
      } else if (kind == "STATE")
        c->events.push_back({true, 0, 0, 0});
      else if (kind == "DEMAND") {
        auto id = hex(row), descriptor = hex(row), demand = hex(row);
        c->events.push_back({false, descriptor, demand, id});
      } else if (kind == "STORE") {
        auto ip = hex(row), field = hex(row), at = hex(row), n = hex(row),
             before = hex(row), after = hex(row);
        check(field <= 2, "Unknown output field");
        c->stores.push_back(
            {ip, AnimationResolutionOutput(field), at, n, before, after});
      } else if (kind == "ALLOC" || kind == "SCHEDULE") {
        Service next{};
        next.entry = kind == "ALLOC" ? 0x16d2c0 : 0x125120;
        if (kind == "ALLOC") {
          next.datum = hex(row);
          next.pool = hex(row);
        }
        c->services.push_back(std::move(next));
        service = &c->services.back();
      } else if (kind == "RBEFORE" || kind == "RAFTER" || kind == "ABEF" ||
                 kind == "AAFT" || kind == "SBEF" || kind == "SAFT") {
        auto at = hex(row);
        row >> raw;
        auto b = decode(raw);
        if (kind == "RBEFORE")
          c->runtime_before.emplace(at, b);
        else if (kind == "RAFTER")
          c->runtime_after.emplace(at, b);
        else {
          check(service != nullptr, "Service buffer outside lower owner");
          (kind == "ABEF" || kind == "SBEF" ? service->before : service->after)
              .emplace(at, b);
        }
      } else if (kind == "QUEUED_RESULT") {
        c->has_queued_result = true;
        c->queued_result = hex(row);
      } else if (kind == "RSTORE") {
        auto ip = hex(row), at = hex(row), before = hex(row), after = hex(row);
        c->resource_stores.push_back({ip, at, before, after});
      } else
        throw std::runtime_error("Unknown resolution fixture row " + kind);
    }
  }
  return f;
}
SourceAnimationGraphMemory memory(const Fixture &f) {
  SourceAnimationGraphMemory g;
  g.graph_address = [&](std::uint32_t d) {
    auto it = f.graphs.find(d);
    check(it != f.graphs.end(), "Missing original graph identity");
    return it->second;
  };
  g.read = [&](std::uint32_t p, std::size_t n) -> std::span<const std::byte> {
    for (const auto &[base, b] : f.memory)
      if (p >= base && std::uint64_t(p) - base <= b.size() &&
          n <= b.size() - (std::uint64_t(p) - base))
        return std::span<const std::byte>(b).subspan(p - base, n);
    throw std::runtime_error("Missing original graph/resource backing");
  };
  return g;
}
template <class F> void rejects(F &&f, const char *why) {
  bool rejected = false;
  try {
    f();
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  check(rejected, why);
}
void resource_case(const Case &c, const SourceAnimationGraphMemory &g) {
  auto current = c.runtime_before;
  SourceAnimationDemandServices resource{};
  resource.bindings = {0x518411, 0x4eb11c, 0x4eb124, 0x5662cc, 0x46fcb4};
  resource.memory.read = [&](std::uint32_t p,
                             std::size_t n) -> std::span<const std::byte> {
    for (const auto &[base, b] : current)
      if (p >= base && std::uint64_t(p) - base <= b.size() &&
          n <= b.size() - (std::uint64_t(p) - base))
        return std::span<const std::byte>(b).subspan(p - base, n);
    if (p >= 0x518410 && std::uint64_t(p) - 0x518410 <= 2 &&
        n <= 2 - (std::uint64_t(p) - 0x518410))
      return std::span<const std::byte>(c.gates).subspan(p - 0x518410, n);
    return g.read(p, n);
  };
  resource.memory.write = [&](std::uint32_t p,
                              std::span<const std::byte> value) {
    for (auto &[base, b] : current)
      if (p >= base && std::uint64_t(p) - base <= b.size() &&
          value.size() <= b.size() - (std::uint64_t(p) - base)) {
        std::copy(value.begin(), value.end(), b.begin() + (p - base));
        return;
      }
    throw std::runtime_error(
        "Resource write escapes actual mutable runtime backing");
  };
  std::size_t next = 0;
  auto service = [&](std::uint32_t entry) -> const Service & {
    check(next < c.services.size(), "Unexpected resource lower owner");
    const auto &s = c.services[next++];
    check(s.entry == entry, "Resource allocation/scheduler order differs");
    check(current == s.before,
          "Full runtime differs before actual lower service");
    current = s.after;
    return s;
  };
  if (!c.services.empty()) {
    resource.allocate_queued = [&](std::uint32_t requested_pool) {
      const auto &s = service(0x16d2c0);
      check(requested_pool == s.pool, "Original allocator pool input differs");
      return SourceAnimationQueuedAllocation{s.datum, s.pool};
    };
    resource.schedule = [&] { service(0x125120); };
  }
  const auto before = g.read(c.args[0], 20);
  check(std::equal(before.begin(), before.end(), c.descriptor_before.begin(),
                   c.descriptor_before.end()),
        "Actual complete descriptor differs");
  if (c.has_queued_result)
    check(find_source_queued_animation(c.args[0], resource) == c.queued_result,
          "Original full salted queued lookup differs");
  std::vector<AnimationResourceStore> writes;
  request_source_animation_resource(
      c.args[0], std::uint8_t(c.args[1]), resource,
      [&](const AnimationResourceStore &st) { writes.push_back(st); });
  check(next == c.services.size(), "Actual allocation/scheduler was omitted");
  check(current == c.runtime_after,
        "Complete resource runtime differs after124e40");
  check(writes == c.resource_stores, "Exact resource direct stores differ");
  const auto after = g.read(c.args[0], 20);
  check(std::equal(after.begin(), after.end(), c.descriptor_after.begin(),
                   c.descriptor_after.end()),
        "Original descriptor after124e40 differs");
}
void cases(const Fixture &f, const SourceAnimationGraphMemory &g,
           const char *mode) {
  std::size_t stores = 0, demands = 0, states = 0;
  std::map<std::uint32_t, unsigned> counts;
  for (const auto &c : f.cases) {
    ++counts[c.entry];
    auto out = c.before;
    std::vector<Event> events;
    std::vector<AnimationResolutionStore> observed;
    check(c.gates.size() == 2, "Missing original runtime gate bytes");
    SourceAnimationDemandServices resource{};
    resource.bindings = {0x518411, 0x4eb11c, 0x4eb124, 0x5662cc, 0x46fcb4};
    resource.memory.read = [&](std::uint32_t p, std::size_t n) {
      if (p >= 0x518410 && std::uint64_t(p) - 0x518410 <= 2 &&
          n <= 2 - (std::uint64_t(p) - 0x518410))
        return std::span<const std::byte>(c.gates).subspan(p - 0x518410, n);
      return g.read(p, n);
    };
    SourceAnimationResourceServices services;
    services.state = [&] {
      events.push_back({true, 0, 0, 0});
      return std::array<std::uint8_t, 2>{
          std::to_integer<std::uint8_t>(c.gates[0]),
          std::to_integer<std::uint8_t>(c.gates[1])};
    };
    services.request = [&](std::uint32_t p, std::uint8_t demand) {
      check(events.size() < c.events.size(), "Extra resource demand");
      const auto id = c.events[events.size()].source_case;
      events.push_back({false, p, demand, id});
      auto found = std::find_if(f.cases.begin(), f.cases.end(),
                                [&](const Case &v) { return v.id == id; });
      check(found != f.cases.end() && found->entry == 0x124e40 &&
                found->args[0] == p && found->args[1] == demand,
            "Actual resource request identity differs");
      resource_case(*found, g);
    };
    auto observer = [&](const AnimationResolutionStore &st) {
      observed.push_back(st);
    };
    if (c.entry == 0x1cfde0) {
      check(c.args.size() == 5, "Missing full1cfde0 arguments");
      SourceAnimationIdentifiers ids{c.args[0], c.args[1], c.args[2],
                                     c.args[3]};
      const bool result = resolve_source_animation_request(
          c.manager, ids, c.args[4], {out[0], out[1], out[2]}, g, services,
          observer);
      check(result == bool(c.result), "Original1cfde0 definedAL differs");
      check(out == c.after, "Original complete1cfde0 buffers differ");
    } else if (c.entry == 0x1deb60) {
      check(c.args.size() == 6, "Missing full1deb60 arguments");
      SourceAnimationIdentifiers ids{c.args[1], c.args[2], c.args[3],
                                     c.args[4]};
      std::array<std::span<std::byte>, 3> optional{};
      for (unsigned i = 0; i < 3; ++i)
        if (c.args[5] & (1u << i))
          optional[i] = std::span<std::byte>(out[1]).subspan(i * 4, 4);
      lookup_source_animation_request(c.args[0], ids, out[2], optional, g,
                                      services, observer);
      check(out == c.after, "Original complete1deb60 buffers differ");
    } else if (c.entry == 0x1e13c0)
      demand_source_animation(c.args[0], c.args[1], g, services);
    else if (c.entry == 0x124e40) {
      resource_case(c, g);
    } else
      throw std::runtime_error("Unknown native resource owner");
    check(events == c.events, "Ordered actual resource calls differ in case " +
                                  std::to_string(c.id));
    check(observed == c.stores,
          "Ordered original output stores differ in case " +
              std::to_string(c.id));
    for (auto e : events)
      e.state ? ++states : ++demands;
    stores += observed.size();
  }
  std::cout << mode << " resolution=" << counts[0x1cfde0]
            << " lookup=" << counts[0x1deb60]
            << " animationDemand=" << counts[0x1e13c0]
            << " resourceDemand=" << counts[0x124e40] << " states=" << states
            << " orderedDemands=" << demands << " orderedStores=" << stores
            << '\n';
}
void boundaries(const Fixture &f) {
  const auto g = memory(f);
  SourceAnimationResourceServices absent{};
  rejects([&] { demand_source_animation(0, 0xffffffff, g, absent); },
          "Missing resource state fabricated a disabled gate");
  SourceAnimationDemandServices missing{};
  rejects([&] { request_source_animation_resource(0, 1, missing); },
          "Missing resource backing fabricated no demand");
  const auto &c = f.cases.front();
  SourceAnimationResourceServices actual;
  actual.state = [&] {
    return std::array<std::uint8_t, 2>{
        std::to_integer<std::uint8_t>(c.gates[0]),
        std::to_integer<std::uint8_t>(c.gates[1])};
  };
  // Keep the same actual request. Any demand reached before an absent scheduler
  // must be a hard boundary; it cannot be silently dropped just because the
  // original underlying gate happened to be disabled in this fixture.
  auto b = c.before;
  SourceAnimationIdentifiers ids{c.args[0], c.args[1], c.args[2], c.args[3]};
  rejects(
      [&] {
        resolve_source_animation_request(c.manager, ids, c.args[4],
                                         {b[0], b[1], b[2]}, g, actual);
      },
      "Missing124e40 owner silently dropped demand");
  auto malformed = g;
  const std::array<std::byte, 3> tiny{};
  malformed.read = [&](std::uint32_t, std::size_t) {
    return std::span<const std::byte>(tiny);
  };
  b = c.before;
  rejects(
      [&] {
        resolve_source_animation_request(c.manager, ids, c.args[4],
                                         {b[0], b[1], b[2]}, malformed, actual);
      },
      "Truncated original graph admitted a result");
  b = c.before;
  rejects(
      [&] {
        resolve_source_animation_request(c.manager, ids, c.args[4],
                                         {b[0], b[0], b[2]}, g, actual);
      },
      "Unowned output aliasing silently admitted");
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3,
          "Usage: unit_animation_resolution_tests fixtures.tsv [actual map]");
    if (!std::ifstream(argv[1]))
      return 77;
    const auto f = load(argv[1]);
    cases(f, memory(f), "Literal");
    boundaries(f);
    if (argc == 3) {
#if HALO2_RESOLUTION_CACHE_TESTS
      auto cache = std::make_shared<Cache>(argv[2]);
      SourceAnimationGraphMemory g;
      std::weak_ptr<const Cache> retained = cache;
      {
        UnitAnimationMetadataStore store(cache);
        check(store.graph_count() == f.graphs.size(),
              "Actual graph inventory differs");
        g = store.graph_memory();
        cache.reset();
      }
      check(!retained.expired(), "Real graph callbacks lost ownership");
      for (const auto &[datum, p] : f.graphs) {
        check(g.graph_address(datum) == p, "Actual graph identity differs");
        const auto &b = f.memory.at(p);
        const auto actual = g.read(p, b.size());
        check(std::equal(actual.begin(), actual.end(), b.begin(), b.end()),
              "Actual complete graph payload differs");
      }
      cases(f, g, "Cache-owned");
#else
      throw std::runtime_error(
          "Cache-owned mode requires registered CMake target");
#endif
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
