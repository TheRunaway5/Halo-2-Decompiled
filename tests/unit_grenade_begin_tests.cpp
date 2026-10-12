#include "halo2/unit_grenade_begin.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using Bytes = std::vector<std::byte>;
using Bodies = std::map<std::uint32_t, Bytes>;
void check(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t hex(std::istringstream &r) {
  std::uint32_t v;
  check(bool(r >> std::hex >> v), "Missing original fixture word");
  return v;
}
Bytes bytes(const std::string &s) {
  check(s.size() % 2 == 0, "Partial fixture bytes");
  Bytes b;
  for (std::size_t i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(std::stoul(s.substr(i, 2), nullptr, 16)));
  return b;
}
struct Leaf {
  GrenadeBeginLeaf call;
  GrenadeBeginLeafResult result;
  Bytes output;
  Bodies before, after;
};
struct Case {
  std::uint32_t unit, result;
  Bytes request, request_before;
  Bodies before, after;
  std::vector<Leaf> leaves;
  std::vector<GrenadeBeginStore> stores, request_stores;
};
struct Fixture {
  std::uint32_t zero;
  std::map<int, Bytes> definitions;
  std::vector<Case> cases;
};
Fixture load(const char *name) {
  std::ifstream f(name);
  check(bool(f), "Missing ignored original begin fixture");
  Fixture result{};
  Case *c = nullptr;
  Leaf *l = nullptr;
  for (std::string line; std::getline(f, line);) {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream row(line);
    std::string kind, raw;
    row >> kind;
    if (kind == "ZERO")
      result.zero = hex(row);
    else if (kind == "DEFINITION") {
      auto index = hex(row);
      row >> raw;
      result.definitions.emplace(index, bytes(raw));
    } else if (kind == "CASE") {
      Case value{};
      value.unit = hex(row);
      value.result = hex(row);
      result.cases.push_back(std::move(value));
      c = &result.cases.back();
    } else if (kind == "END_CASE")
      c = nullptr;
    else {
      check(c != nullptr, "Fixture record outside case");
      if (kind == "REQUEST" || kind == "REQUEST_BEFORE") {
        row >> raw;
        (kind == "REQUEST" ? c->request : c->request_before) = bytes(raw);
      } else if (kind == "BEFORE" || kind == "AFTER" || kind == "LBEFORE" ||
                 kind == "LAFTER") {
        auto datum = hex(row);
        row >> raw;
        Bodies *target;
        if (kind == "BEFORE")
          target = &c->before;
        else if (kind == "AFTER")
          target = &c->after;
        else {
          check(l != nullptr, "Service state outside leaf");
          target = kind == "LBEFORE" ? &l->before : &l->after;
        }
        target->emplace(datum, bytes(raw));
      } else if (kind == "LEAF") {
        Leaf value{};
        value.call.entry = hex(row);
        value.call.count = hex(row);
        check(value.call.count <= 6, "Too many owner operands");
        for (unsigned i = 0; i < value.call.count; ++i)
          value.call.arguments[i] = hex(row);
        value.result.eax = hex(row);
        value.result.xmm0_bits = hex(row);
        row >> raw;
        if (raw != "-")
          value.output = bytes(raw);
        c->leaves.push_back(std::move(value));
        l = &c->leaves.back();
      } else if (kind == "END_LEAF")
        l = nullptr;
      else if (kind == "STORE" || kind == "REQUEST_STORE") {
        GrenadeBeginStore value{};
        value.instruction = hex(row);
        value.datum = kind == "STORE" ? c->unit : 0xffffffffu;
        value.buffer = kind == "STORE" ? GrenadeBeginBuffer::action
                                       : GrenadeBeginBuffer::request;
        value.offset = hex(row);
        value.bytes = hex(row);
        value.before = hex(row);
        value.after = hex(row);
        (kind == "STORE" ? c->stores : c->request_stores).push_back(value);
      } else
        check(false, "Unknown original fixture record");
    }
  }
  return result;
}
void apply(Bodies &b, const Bodies &after) {
  check(b.size() == after.size(),
        "Unprovided object lifecycle in begin service");
  for (const auto &[datum, raw] : after) {
    auto &out = b.at(datum);
    check(out.size() == raw.size(), "Source owner storage changed extent");
    std::copy(raw.begin(), raw.end(), out.begin());
  }
}
void run(const Fixture &f) {
  unsigned callbacks = 0, stores = 0, packet_stores = 0, admitted = 0;
  for (const auto &c : f.cases) {
    Bodies bodies = c.before;
    std::vector<GrenadeBeginStore> observed, constructed;
    auto packet = c.request_before;
    construct_source_grenade_begin_request(
        packet, [&](auto s) { constructed.push_back(s); });
    check(packet == c.request,
          "Complete original request constructor mismatch");
    check(constructed == c.request_stores,
          "Original request store order mismatch");
    packet_stores += constructed.size();
    SourceGrenadeBeginServices s{};
    s.zero_bits = f.zero;
    s.unit_body = [&](auto datum) -> std::span<std::byte> {
      return bodies.at(datum);
    };
    s.grenade_definition = [&](std::int8_t type) -> std::span<const std::byte> {
      return f.definitions.at(type);
    };
    // No parent/local-player service is admitted by these source-produced
    // cases.
    unsigned next = 0;
    s.invoke = [&](const auto &call, std::span<std::byte> output,
                   std::span<const std::byte> point,
                   std::span<const std::byte> direction) {
      check(next < c.leaves.size(), "Unexpected begin callback");
      const auto &expected = c.leaves[next++];
      check(call == expected.call,
            "Original begin callback arguments/order mismatch");
      check(bodies == expected.before,
            "Complete original before-service object buffers mismatch");
      check(point.empty() && direction.empty(),
            "Unobserved immediate release vectors");
      check(output.size() == expected.output.size(),
            "Original owner output extent mismatch");
      std::copy(expected.output.begin(), expected.output.end(), output.begin());
      apply(bodies, expected.after);
      ++callbacks;
      return expected.result;
    };
    const auto actual = begin_source_grenade(
        c.unit, packet, s, [&](auto st) { observed.push_back(st); });
    check(actual == (c.result != 0), "Original defined AL mismatch");
    check(next == c.leaves.size(), "Skipped original begin callback");
    check(bodies == c.after, "Complete original final object buffers mismatch");
    check(observed == c.stores, "Complete direct begin store order mismatch");
    stores += observed.size();
    admitted += actual;
  }
  check(f.cases.size() == 3 && callbacks == 22 && stores == 6 &&
            packet_stores == 27 && admitted == 1,
        "Incomplete owning begin suite");
  // A required lower owner is unavailable, not successful/false by default.
  const auto &c = f.cases.front();
  Bodies b = c.before;
  SourceGrenadeBeginServices absent{};
  absent.zero_bits = f.zero;
  absent.unit_body = [&](auto datum) -> std::span<std::byte> {
    return b.at(datum);
  };
  absent.grenade_definition =
      [&](std::int8_t type) -> std::span<const std::byte> {
    return f.definitions.at(type);
  };
  bool stopped = false;
  try {
    begin_source_grenade(c.unit, c.request, absent);
  } catch (const std::runtime_error &) {
    stopped = true;
  }
  check(stopped && b == c.before,
        "Unavailable begin owner mutated/admitted state");
  stopped = false;
  try {
    source_grenade_begin_action_storage(
        std::span<std::byte>(b.at(c.unit)).first(0x346));
  } catch (const std::runtime_error &) {
    stopped = true;
  }
  check(stopped, "Truncated unit component admitted");
  std::cout << "Original full begin cases=" << f.cases.size()
            << " admitted=" << admitted << " callbacks=" << callbacks
            << " directStores=" << stores << " requestStores=" << packet_stores
            << "\n";
}
} // namespace
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  if (!std::filesystem::exists(argv[1]))
    return 77;
  try {
    run(load(argv[1]));
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
