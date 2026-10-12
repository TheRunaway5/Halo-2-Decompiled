#include "halo2/cache.hpp"
#include "halo2/unit_animation.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
using Packet = std::vector<std::byte>;
void check(bool ok, const std::string &why) {
  if (!ok)
    throw halo2::FormatError("Original animation definition differs: " + why);
}
std::uint32_t number(const std::string &s) {
  return std::stoul(s, nullptr, 16);
}
Packet packet(const std::string &s) {
  if (s == "-")
    return {};
  check(s.size() % 2 == 0, "fixture hex extent");
  Packet out;
  for (std::size_t i = 0; i < s.size(); i += 2)
    out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
struct Counts {
  unsigned definitions{}, inherited{}, timing{}, sounds{}, effects{},
      sound_refs{}, effect_refs{}, negative_loop{}, graphs{}, graph_boundaries{};
};
void compare_graph(const halo2::Cache &cache, std::istringstream &row,
                   Counts &counts) {
  std::string graph, address, raw;
  std::size_t size;
  check(bool(row >> graph >> address >> size >> raw), "complete original graph");
  const auto g = cache.load_source_unit_animation_graph_metadata(number(graph));
  const auto expected = packet(raw);
  check(g.graph() == number(graph) && g.source_address() == number(address) &&
            g.size() == size && expected.size() == size,
        "exact graph identity, original virtual token and extent");
  const auto full = g.bytes_at(g.source_address(), size);
  check(std::equal(full.begin(), full.end(), expected.begin(), expected.end()) &&
            std::equal(g.header().begin(), g.header().end(), expected.begin()),
        "full authored graph banks and opaque pointer words");
  check(g.bytes_at(g.source_address() + std::uint32_t(size), 0).empty(),
        "zero-size end of graph is representable");
  auto rejects_pointer = [&](std::uint32_t p, std::size_t n) {
    bool stopped = false;
    try {
      (void)g.bytes_at(p, n);
    } catch (const std::runtime_error &) {
      stopped = true;
    }
    check(stopped, "escaped virtual token must not become a host pointer");
    ++counts.graph_boundaries;
  };
  rejects_pointer(g.source_address() - 1, 1);
  rejects_pointer(g.source_address() + std::uint32_t(size), 1);
  rejects_pointer(g.source_address(), size + 1);
  ++counts.graphs;
}
void compare(const halo2::Cache &cache, std::istringstream &row, Counts &counts) {
  std::string graph, packed, raw, timing, sounds, effects, sound_refs, effect_refs;
  check(bool(row >> graph >> packed >> raw >> timing >> sounds >> effects >>
             sound_refs >> effect_refs), "complete source fixture");
  const auto d = cache.load_source_unit_animation_definition(number(graph),
                                                             number(packed));
  const auto r = packet(raw), t = packet(timing), s = packet(sounds),
             e = packet(effects), sr = packet(sound_refs), er = packet(effect_refs);
  const halo2::Bytes rb(r), tb(t), sb(s), eb(e), srb(sr), erb(er);
  check(r.size() == 0x6c && d.frame_count == std::int16_t(rb.u16(0x14)) &&
            d.loop_frame == std::int16_t(rb.u16(0x2e)) &&
            d.authored_flags == std::to_integer<unsigned>(r[0x18]),
        graph + ":" + packed + " signed frame/loop/flag words");
  check(t.size() == d.timing.size() * 4 && s.size() == d.sounds.size() * 8 &&
            e.size() == d.effects.size() * 4 &&
            sr.size() == d.sound_references.size() * 12 &&
            er.size() == d.effect_references.size() * 12,
        graph + ":" + packed + " all authored record counts");
  for (std::size_t i = 0; i < d.timing.size(); ++i)
    check(d.timing[i].type == std::int16_t(tb.u16(i * 4)) &&
              d.timing[i].frame == std::int16_t(tb.u16(i * 4 + 2)),
          "ordered timing record");
  for (std::size_t i = 0; i < d.sounds.size(); ++i)
    check(d.sounds[i].reference == std::int16_t(sb.u16(i * 8)) &&
              d.sounds[i].frame == std::int16_t(sb.u16(i * 8 + 2)) &&
              d.sounds[i].marker == sb.u32(i * 8 + 4),
          "ordered sound record including marker");
  for (std::size_t i = 0; i < d.effects.size(); ++i)
    check(d.effects[i].reference == std::int16_t(eb.u16(i * 4)) &&
              d.effects[i].frame == std::int16_t(eb.u16(i * 4 + 2)),
          "ordered effect record");
  for (std::size_t i = 0; i < d.sound_references.size(); ++i)
    check(d.sound_references[i].tag == srb.u32(i * 12 + 4) &&
              d.sound_references[i].flags == srb.u16(i * 12 + 8),
          "resolved graph sound tag and all flags");
  for (std::size_t i = 0; i < d.effect_references.size(); ++i)
    check(d.effect_references[i].tag == erb.u32(i * 12 + 4) &&
              d.effect_references[i].flags == erb.u16(i * 12 + 8),
          "resolved graph effect tag and all flags");
  ++counts.definitions;
  counts.inherited += (number(packed) & 0xffffu) != 0xffffu;
  counts.timing += d.timing.size();
  counts.sounds += d.sounds.size();
  counts.effects += d.effects.size();
  counts.sound_refs += d.sound_references.size();
  counts.effect_refs += d.effect_references.size();
  counts.negative_loop += d.loop_frame < 0;
}
void rejects(const halo2::Cache &cache, std::uint32_t graph,
             std::uint32_t packed) {
  bool stopped = false;
  try {
    (void)cache.load_source_unit_animation_definition(graph, packed);
  } catch (const halo2::FormatError &) {
    stopped = true;
  }
  check(stopped, "unavailable metadata must stop at its owner boundary");
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc >= 3, "supply original map and original definition fixture(s)");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    const halo2::Cache cache(argv[1]);
    Counts counts;
    std::uint32_t known_graph = 0xffffffffu;
    for (int i = 2; i < argc; ++i) {
      std::ifstream f(argv[i]);
      for (std::string line; std::getline(f, line);) {
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        if (kind == "G") {
          compare_graph(cache, row, counts);
          continue;
        }
        if (kind != "D")
          continue;
        if (known_graph == 0xffffffffu) {
          std::istringstream first(line);
          std::string graph;
          first >> kind >> graph;
          known_graph = number(graph);
        }
        compare(cache, row, counts);
      }
    }
    check(counts.definitions != 0, "original definitions are present");
    rejects(cache, 0xffffffffu, 0x0000ffffu);
    rejects(cache, known_graph, 0xffffffffu);
    rejects(cache, known_graph, 0x8000ffffu);
    rejects(cache, known_graph, 0x0000fffeu);
    rejects(cache, known_graph, 0x00007fffu);
    std::cout << "definitions=" << counts.definitions
              << " inherited=" << counts.inherited
              << " timing=" << counts.timing << " sounds=" << counts.sounds
              << " effects=" << counts.effects
              << " sound_references=" << counts.sound_refs
              << " effect_references=" << counts.effect_refs
              << " preserved_negative_loops=" << counts.negative_loop
              << " unavailable_owner_checks=5 graphs=" << counts.graphs
              << " graph_pointer_boundaries=" << counts.graph_boundaries
              << '\n';
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
