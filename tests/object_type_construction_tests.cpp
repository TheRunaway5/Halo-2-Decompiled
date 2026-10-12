#include "halo2/object_type_construction.hpp"
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
using namespace halo2;
using Bytes = std::vector<std::byte>;
using State = std::map<std::string, Bytes>;
void check(bool p, const std::string &m) {
  if (!p)
    throw std::runtime_error(m);
}
std::uint32_t word(std::istringstream &r) {
  std::uint32_t v;
  check(bool(r >> std::hex >> v), "Truncated source fixture");
  return v;
}
Bytes decode(const std::string &s) {
  check(s.size() % 2 == 0, "Truncated original byte string");
  Bytes b(s.size() / 2);
  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = std::byte(std::stoul(s.substr(2 * i, 2), nullptr, 16));
  return b;
}
std::uint32_t read(std::span<const std::byte> b, std::size_t p,
                   unsigned n = 4) {
  check(p <= b.size() && n <= b.size() - p, "Fixture backing truncated");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[p + i]) << (8 * i);
  return v;
}
struct Read {
  std::uint32_t address;
  Bytes bytes;
  bool operator==(const Read &) const = default;
};
struct Call {
  std::uint32_t entry, al;
  State before, after;
};
struct Case {
  std::string id;
  std::uint32_t entry, unit, definition, eax;
  State before, after;
  std::vector<Call> calls;
  std::vector<Read> reads;
};
struct Fixture {
  std::uint32_t bank;
  std::map<std::uint32_t, Bytes> memory;
  std::map<std::int16_t, std::uint32_t> groups;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream in(path);
  check(bool(in), "Missing original constructor fixture");
  Fixture f{};
  Case *c = nullptr;
  Call *call = nullptr;
  for (std::string line; std::getline(in, line);) {
    std::istringstream r(line);
    std::string kind;
    r >> kind;
    if (kind.empty() || kind[0] == '#')
      continue;
    if (kind == "BANK")
      f.bank = word(r);
    else if (kind == "MEM") {
      auto p = word(r);
      std::string raw;
      r >> raw;
      f.memory.emplace(p, decode(raw));
    } else if (kind == "GROUP") {
      auto index = std::bit_cast<std::int16_t>(std::uint16_t(word(r)));
      f.groups.emplace(index, word(r));
    } else if (kind == "CASE") {
      f.cases.emplace_back();
      c = &f.cases.back();
      r >> c->id;
      c->entry = word(r);
      c->unit = word(r);
      c->definition = word(r);
      c->eax = word(r);
    } else if (kind == "CALL") {
      check(c, "CALL outside fixture case");
      c->calls.push_back({word(r), word(r), {}, {}});
      call = &c->calls.back();
    } else if (kind == "BEFORE" || kind == "AFTER" || kind == "CALLBEFORE" ||
               kind == "CALLAFTER") {
      check(c, "State outside fixture case");
      std::string name, raw;
      r >> name >> raw;
      auto *s = kind == "BEFORE"       ? &c->before
                : kind == "AFTER"      ? &c->after
                : kind == "CALLBEFORE" ? &call->before
                                       : &call->after;
      s->emplace(name, decode(raw));
    } else if (kind == "READ") {
      auto p = word(r);
      std::string raw;
      r >> raw;
      c->reads.push_back({p, decode(raw)});
    } else
      check(kind == "ENDCALL" || kind == "ENDCASE",
            "Unknown original fixture row");
  }
  return f;
}
template <class F> void rejects(F &&fn) {
  bool caught = false;
  try {
    fn();
  } catch (const std::exception &) {
    caught = true;
  }
  check(caught, "Missing constructor ownership became success");
}
void apply(State &state, const State &after) {
  for (const auto &[name, b] : after) {
    auto &target = state.at(name);
    if (target.size() == b.size())
      std::copy(b.begin(), b.end(), target.begin());
    else {
      check(name.starts_with("U"),
            "Only actual body-growth boundary may change extent");
      target = b;
    }
  }
  check(state == after, "Lower original callback output backing differs");
}
void verify(const Fixture &f) {
  std::size_t callbacks = 0, reads = 0, whole = 0;
  check(f.cases.size() == 17,
        "Expected7 wrappers and10 actual authored group queries");
  for (const auto &c : f.cases) {
    auto state = c.before;
    std::vector<Read> actual_reads;
    std::size_t at = 0;
    auto raw = [&](std::uint32_t p,
                   std::size_t n) -> std::span<const std::byte> {
      for (const auto &[base, b] : f.memory)
        if (p >= base && std::uint64_t(p) - base <= b.size() &&
            n <= b.size() - (p - base))
          return std::span<const std::byte>(b).subspan(p - base, n);
      throw std::runtime_error("Unowned type metadata range");
    };
    auto observed = [&](std::uint32_t p,
                        std::size_t n) -> std::span<const std::byte> {
      auto b = raw(p, n);
      actual_reads.push_back({p, Bytes(b.begin(), b.end())});
      return b;
    };
    SourceObjectConstructionServices services{};
    services.metadata.read = observed;
    services.metadata.type_address = [&](std::int16_t type) {
      check(type >= 0 && type < 13,
            "Original type bank index outside owned bank");
      return read(observed(f.bank + std::uint32_t(type) * 4, 4), 0);
    };
    services.metadata.tag_group = [&](std::int16_t index) {
      return f.groups.at(index);
    };
    const auto capacity = read(state.at("P"), 0x20);
    services.objects.headers = state.at("K");
    services.objects.capacity = capacity;
    services.objects.body = [&](std::uint16_t index) -> std::span<std::byte> {
      return state.at("U" + std::to_string(index));
    };
    services.invoke =
        [&](const SourceObjectConstructionCall &v) -> std::uint8_t {
      check(at < c.calls.size(), "Unexpected constructor callback");
      const auto &e = c.calls[at++];
      check(v.entry == e.entry, "Actual original callback/order differs");
      const bool constructing = c.entry == 0x10cc20;
      check(v.callback == (constructing ? ObjectConstructionCallback::Construct
                                        : ObjectConstructionCallback::Prepare),
            "Wrong original callback kind");
      check(v.unit == (constructing ? c.unit : 0xffffffffu),
            "Full incoming unit identity differs");
      check(v.request.data() == state.at("D").data() &&
                v.request.size() == state.at("D").size(),
            "C4 request backing identity changed");
      if (constructing)
        check(v.failure.data() == state.at("F").data() && v.failure.size() == 1,
              "Failure byte backing changed");
      else
        check(v.failure.empty(), "Preparation gained failure output");
      check(state == e.before, "Complete original callback input differs");
      // Test-only explicit unchanged-original lower boundary. The production
      // wrapper has no replay/default provider: real lower constructors
      // required.
      apply(state, e.after);
      return std::uint8_t(e.al);
    };
    if (c.entry == 0x10ca30)
      check(source_object_definition_type(c.definition, services.metadata) ==
                std::bit_cast<std::int32_t>(c.eax),
            "Original13-group query result differs");
    else if (c.entry == 0x10cbd0)
      prepare_source_object_type(state.at("D"), services);
    else
      check(construct_source_object_type(c.unit, state.at("D"), state.at("F"),
                                         services) == bool(c.eax & 255),
            "Original constructor AL differs");
    check(at == c.calls.size(), "Omitted original callback");
    check(state == c.after,
          "Complete original pool/header/world/body/request/failure buffers "
          "differ");
    check(actual_reads == c.reads,
          "Literal original metadata read order/width/value differs");
    callbacks += at;
    reads += actual_reads.size();
    for (const auto &[name, b] : state) {
      (void)name;
      whole += b.size();
    }
    auto missing = services;
    missing.metadata.read = {};
    rejects(
        [&] { source_object_definition_type(c.definition, missing.metadata); });
    missing = services;
    missing.metadata.type_address = {};
    rejects(
        [&] { source_object_definition_type(c.definition, missing.metadata); });
    missing = services;
    missing.metadata.tag_group = {};
    rejects(
        [&] { source_object_definition_type(c.definition, missing.metadata); });
    if (c.entry == 0x10cc20) {
      missing = services;
      missing.objects.headers = missing.objects.headers.first(3);
      rejects([&] {
        construct_source_object_type(c.unit, state.at("D"), state.at("F"),
                                     missing);
      });
      missing = services;
      missing.objects.body = {};
      rejects([&] {
        construct_source_object_type(c.unit, state.at("D"), state.at("F"),
                                     missing);
      });
      rejects([&] {
        construct_source_object_type(c.unit, state.at("D"), {}, services);
      });
      rejects([&] {
        construct_source_object_type(
            c.unit, std::span<std::byte>(state.at("D")).first(4), state.at("F"),
            services);
      });
      missing = services;
      missing.invoke = {};
      rejects([&] {
        construct_source_object_type(c.unit, state.at("D"), state.at("F"),
                                     missing);
      });
    }
  }
  std::cout << "sourceObjectConstruction cases=" << f.cases.size()
            << " requiredOriginalCallbackBoundaries=" << callbacks
            << " orderedMetadataReads=" << reads
            << " wholeOutputBytes=" << whole << " wrapperStores=0\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply literal original construction fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    verify(load(argv[1]));
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
