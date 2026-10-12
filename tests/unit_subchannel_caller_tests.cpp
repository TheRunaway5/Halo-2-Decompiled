#include "halo2/unit_subchannel_caller.hpp"
#ifdef HALO2_SUBCHANNEL_CALLER_CACHE
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
  check(bool(r >> std::hex >> v), "Incomplete original caller fixture");
  return v;
}
Bytes decode(const std::string &s) {
  check(s.size() % 2 == 0, "Incomplete source bytes");
  Bytes b(s.size() / 2);
  for (std::size_t i = 0; i < b.size(); ++i)
    b[i] = std::byte(std::stoul(s.substr(2 * i, 2), nullptr, 16));
  return b;
}
struct Tag {
  std::uint32_t address;
  Bytes raw;
};
struct Case {
  std::string id;
  std::uint32_t unit, capacity;
  State before, after;
  std::vector<SourceUnitSubchannelCall> calls;
};
struct Fixture {
  std::uint32_t one;
  std::map<std::uint32_t, Tag> tags;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream in(path);
  check(bool(in), "No actual caller fixture");
  Fixture f{};
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
    } else if (kind == "ONE")
      f.one = word(r);
    else if (kind == "CASE") {
      f.cases.emplace_back();
      c = &f.cases.back();
      r >> c->id;
      c->unit = word(r);
      c->capacity = word(r);
    } else if (kind == "BEFORE" || kind == "AFTER") {
      std::string name, raw;
      r >> name >> raw;
      (kind == "BEFORE" ? c->before : c->after).emplace(name, decode(raw));
    } else if (kind == "CALL") {
      c->calls.push_back({word(r), word(r), word(r), word(r), word(r), word(r),
                          bool(word(r)), bool(word(r))});
    } else
      check(kind == "ENDCASE", "Unknown caller fixture record");
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
  check(caught, "Missing source body owner became successful");
}
void verify(const Fixture &f, SourceAnimationGraphMemory graphs,
            std::uint32_t one, const char *label) {
  std::size_t call_count = 0;
  for (const auto &c : f.cases) {
    auto state = c.before;
    check(state.at("POOL").size() == std::size_t(c.capacity) * 12,
          "Actual full pool capacity differs");
    UnitConstructorObjectPool pool{};
    pool.headers = state.at("POOL");
    pool.capacity = c.capacity;
    pool.body = [&](std::uint16_t index) -> std::span<std::byte> {
      check(index == std::uint16_t(c.unit), "Unowned source body index");
      return state.at("U");
    };
    auto selected_header =
        pool.headers.subspan(std::size_t(c.unit & 65535) * 12, 12);
    check(std::equal(selected_header.begin(), selected_header.end(),
                     state.at("H").begin()),
          "Actual pool/header alias differs");
    SourceUnitChannelRequestServices s{};
    s.one_bits = one;
    s.graphs = graphs;
    // OriginalNONE channels avoid all resource/RNG/clock callbacks. Their
    // absence is retained as a required-owner stop if an admitted branch ever
    // reaches it.
    std::vector<SourceUnitSubchannelCall> calls;
    std::size_t stores = 0;
    update_source_unit_subchannels(
        c.unit, pool, s,
        [&](const SourceUnitSubchannelCall &v) { calls.push_back(v); },
        [&](const UnitAnimationSelectionStore &) { ++stores; });
    check(calls == c.calls, c.id + " Complete call ABI/order differs");
    check(stores == 0, c.id + " SourceNONE caller unexpectedly stored");
    check(state == c.after,
          c.id + " Complete header/pool/body/global state differs");
    call_count += calls.size();
    auto missing = pool;
    missing.body = {};
    rejects([&] { update_source_unit_subchannels(c.unit, missing, s); });
    missing = pool;
    missing.headers = pool.headers.first(3);
    rejects([&] { update_source_unit_subchannels(c.unit, missing, s); });
  }
  std::cout << label << " cases=" << f.cases.size()
            << " orderedCallBoundaries=" << call_count << " originalStores=0\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2 || argc == 3 || argc == 4,
          "Supply source caller fixture, optional Lockout cache and verified "
          "image");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    auto f = load(argv[1]);
    check(f.cases.size() == 2, "Expected2 unchanged original callers");
    SourceAnimationGraphMemory literal{
        [&](std::uint32_t d) { return f.tags.at(d).address; },
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
          throw std::runtime_error("Missing original graph backing");
        }};
    verify(f, literal, f.one, "literal");
#ifdef HALO2_SUBCHANNEL_CALLER_CACHE
    if (argc >= 3) {
      auto cache = std::make_shared<Cache>(argv[2]);
      UnitAnimationMetadataStore store(cache);
      auto g = store.graph_memory();
      for (const auto &[d, t] : f.tags) {
        check(g.graph_address(d) == t.address, "Cache graph identity differs");
        auto b = g.read(t.address, t.raw.size());
        check(std::equal(b.begin(), b.end(), t.raw.begin()),
              "Actual graph bytes differ");
      }
      auto one = f.one;
      if (argc == 4) {
        auto original = OriginalData::load_xbe(argv[3]);
        check(bool(original->unit_animation_constants),
              "Missing verified image operands");
        one = original->unit_animation_constants->one_bits;
        check(one == f.one, "Original one bits differ");
      }
      verify(f, g, one, argc == 4 ? "actualCache+image" : "actualCache");
    }
#else
    check(argc == 2, "Cache comparison not compiled");
#endif
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
