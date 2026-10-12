#include "halo2/unit_animation_pose.hpp"
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
using Bytes = std::vector<std::byte>;
void check(bool v, const std::string &s) {
  if (!v)
    throw std::runtime_error(s);
}
std::uint32_t num(const std::string &s) { return std::stoul(s, nullptr, 16); }
Bytes bytes(const std::string &s) {
  Bytes b;
  if (s == "-")
    return b;
  for (std::size_t i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(num(s.substr(i, 2))));
  return b;
}
struct Definition {
  Bytes raw, masks;
};
struct Case {
  std::string name, kind, mask_kind;
  std::int16_t frequency{};
  bool dc{}, dd{};
  unsigned nodes{}, decode_nodes{}, interpolate{};
  std::uint32_t weight{}, graph{}, packed{}, frame{}, decode_weight{},
      mapping{};
  halo2::UnitAnimationLocalContext context;
  Bytes packet, mask, before, after, decode_before, decode_after, decode_mask;
};
halo2::UnitAnimationMaskOwner kind(const std::string &s) {
  if (s == "None")
    return halo2::UnitAnimationMaskOwner::None;
  if (s == "Caller")
    return halo2::UnitAnimationMaskOwner::Caller;
  if (s == "Graph5C")
    return halo2::UnitAnimationMaskOwner::Graph5C;
  if (s == "Graph7C")
    return halo2::UnitAnimationMaskOwner::Graph7C;
  check(s == "Intersection", "unknown source mask pointer owner");
  return halo2::UnitAnimationMaskOwner::Intersection;
}
void run(
    const Case &c,
    const std::map<std::pair<std::uint32_t, std::uint32_t>, Definition> &defs) {
  Bytes output = c.before;
  unsigned calls = 0;
  halo2::UnitAnimationLocalPoseServices s;
  s.tick_frequency = c.frequency;
  s.layer_synchronization = c.dc;
  s.third_channel_override = c.dd;
  s.definition6c = [&](std::uint32_t graph,
                       std::uint32_t packed) -> std::span<const std::byte> {
    return defs.at({graph, packed}).raw;
  };
  s.graph_masks = [&](std::uint32_t graph) -> std::span<const std::byte> {
    return defs.at({graph, c.packed}).masks;
  };
  s.decode = [&](const halo2::UnitAnimationLocalSample &r,
                 std::span<std::byte> out) {
    ++calls;
    check(r.graph == c.graph && r.packed == c.packed &&
              r.inheritance_mapping == c.mapping &&
              r.node_count == c.decode_nodes,
          c.name + " graph/packed/map/node delivery");
    check(std::bit_cast<std::uint32_t>(r.frame) == c.frame &&
              std::bit_cast<std::uint32_t>(r.weight) == c.decode_weight,
          c.name + " exact frame/weight words");
    check(unsigned(r.interpolate) == c.interpolate &&
              r.mask_owner == kind(c.mask_kind),
          c.name + " actual fractional admission/mask owner");
    check(std::equal(r.node_mask.begin(), r.node_mask.end(),
                     c.decode_mask.begin(), c.decode_mask.end()),
          c.name + " full source node mask");
    check(std::equal(out.begin(), out.end(), c.decode_before.begin(),
                     c.decode_before.end()),
          c.name + " entire local bank before codec");
    check(out.size() == c.decode_after.size(), "codec output extent");
    std::copy(c.decode_after.begin(), c.decode_after.end(), out.begin());
  };
  auto invoke = [&] {
    if (c.kind == "M")
      halo2::build_source_unit_animation_locals(c.packet, c.context,
                                                std::bit_cast<float>(c.weight),
                                                c.mask, c.nodes, output, s);
    else
      halo2::sample_source_unit_animation_locals(
          c.packet, std::bit_cast<float>(c.weight), c.mask, c.nodes, output, s);
  };
  invoke();
  check(calls == 1 && output == c.after,
        c.name + " complete source local-bank output");
  output = c.before;
  s.decode = {};
  bool stopped = false;
  try {
    invoke();
  } catch (const std::runtime_error &) {
    stopped = true;
  }
  check(stopped && output == c.before,
        c.name + " unresolved packed decoder is a hard boundary");
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original local-pose fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream f(argv[1]);
    std::vector<Case> cases;
    std::map<std::pair<std::uint32_t, std::uint32_t>, Definition> defs;
    for (std::string line; std::getline(f, line);) {
      std::istringstream s(line);
      std::vector<std::string> r;
      for (std::string q; s >> q;)
        r.push_back(q);
      if (r.empty())
        continue;
      if (r[0] == "D") {
        check(r.size() == 5, "definition row");
        defs.emplace(std::pair(num(r[1]), num(r[2])),
                     Definition{bytes(r[3]), bytes(r[4])});
      } else if (r[0] == "C") {
        check(r.size() == 11, "pose owner row");
        Case c;
        c.name = r[1];
        c.kind = r[2];
        c.frequency = std::int16_t(std::stoi(r[3]));
        c.dc = r[4] != "0";
        c.dd = r[5] != "0";
        c.weight = num(r[6]);
        c.nodes = std::stoul(r[7]);
        c.context = {num(r[8]), num(r[9]), num(r[10])};
        cases.push_back(std::move(c));
      } else if (r[0] == "K") {
        auto &c = cases.back();
        check(r.size() == 9, "decoder delivery row");
        c.graph = num(r[1]);
        c.packed = num(r[2]);
        c.decode_nodes = std::stoul(r[3]);
        c.frame = num(r[4]);
        c.decode_weight = num(r[5]);
        c.mapping = num(r[6]);
        c.interpolate = std::stoul(r[7]);
        c.mask_kind = r[8];
      } else {
        auto &c = cases.back();
        auto b = bytes(r.at(1));
        if (r[0] == "B")
          c.packet = std::move(b);
        else if (r[0] == "N")
          c.mask = std::move(b);
        else if (r[0] == "L")
          c.before = std::move(b);
        else if (r[0] == "F")
          c.after = std::move(b);
        else if (r[0] == "I")
          c.decode_before = std::move(b);
        else if (r[0] == "O")
          c.decode_after = std::move(b);
        else if (r[0] == "A")
          c.decode_mask = std::move(b);
        else
          throw std::runtime_error("Unknown local-pose fixture row");
      }
    }
    check(cases.size() == 23, "complete original local-pose suite");
    for (const auto &c : cases)
      run(c, defs);
    std::cout << cases.size()
              << " original local-pose owners, full decoder packets/banks and "
                 "unavailable decoder stops matched\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
