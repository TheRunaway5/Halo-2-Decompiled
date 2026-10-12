#include "halo2/spatial_registry.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value, const std::string &context) {
  if (!value) throw std::runtime_error("Original spatial registry differs: " + context);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  std::vector<std::byte> out; check(s.size() % 2 == 0, "hex extent");
  for (std::size_t i = 0; i < s.size(); i += 2) out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
struct State {
  std::map<std::uint32_t, std::vector<std::byte>> bodies;
  std::map<std::string, std::vector<std::byte>> other;
  std::map<std::string, std::string> aliases;
  friend bool operator==(const State &, const State &) = default;
};
void synchronize_aliases(State &state) {
  for (const auto &[alias, owner] : state.aliases) {
    auto &to = state.other.at(alias);
    const auto &from = state.other.at(owner);
    check(to.size() == from.size(), "original physical alias extent");
    std::copy(from.begin(), from.end(), to.begin());
  }
}
struct Packet { std::vector<std::byte> before, after; };
struct Cluster {
  unsigned before, after; std::int16_t cluster, maximum; float radius;
  std::int32_t result; std::vector<std::byte> center, scratch_before, scratch_after;
};
struct Call {
  std::string label; std::uint32_t entry, object; unsigned registry, before, after;
  float radius, zero;
  std::map<std::string, Packet> packets;
  std::vector<Cluster> clusters;
  std::vector<halo2::SourceSpatialStore> stores;
};
template<std::size_t N> std::span<std::byte, N> fixed(std::span<std::byte> raw) {
  check(raw.size() == N, "literal fixed extent"); return std::span<std::byte, N>(raw.data(), N);
}
void replace(State &to, const State &from) {
  check(to.bodies.size() == from.bodies.size() && to.other.size() == from.other.size(), "literal cluster producer roster");
  auto copy = [&](auto &target, const auto &source) {
    check(target.size() == source.size(), "literal cluster producer storage extent");
    std::copy(source.begin(), source.end(), target.begin());
  };
  for (auto &[key, value] : to.bodies) copy(value, from.bodies.at(key));
  for (auto &[key, value] : to.other) copy(value, from.other.at(key));
}
void run(const Call &call, const std::map<unsigned, State> &states, unsigned &leaves, unsigned &writes) {
  auto state = states.at(call.before); auto &body = state.bodies.at(call.object);
  const auto prefix = "registries/" + std::to_string(call.registry) + "/";
  auto pool = [&](const char *kind) -> halo2::SourceSpatialPoolStorage {
    const auto key = prefix + kind + "/";
    return {fixed<0x4c>(state.other.at(key + "header4c_hex")), state.other.at(key + "data_hex"), state.other.at(key + "bitmap_hex")};
  };
  halo2::SourceSpatialRegistryStorage storage{state.other.at(prefix + "heads800_hex"), pool("forward"), pool("reverse"),
      std::span<std::byte, 4>(body.data() + 0x60, 4)};
  std::vector<halo2::SourceSpatialStore> stores;
  const auto observe = [&](const auto &event) {
    stores.push_back(event);
    synchronize_aliases(state);
  };
  if (call.entry == 0x1ce830)
    halo2::source_unregister_spatial_object(storage, call.object, observe);
  else {
    check(call.entry == 0x1ce650 && call.clusters.size() == 1, call.label + " measured registry/cluster producer roster");
    auto payload = call.packets.at("payload").before;
    auto overflow = call.packets.at("overflow").before;
    auto scratch = call.clusters[0].scratch_before;
    auto center = std::span<const std::byte, 12>(body.data() + 0x40, 12);
    auto location = std::span<const std::byte, 8>(body.data() + 0x28, 8);
    check(std::equal(center.begin(), center.end(), call.packets.at("center").before.begin()), call.label + " actual center packet");
    check(std::equal(location.begin(), location.end(), call.packets.at("location").before.begin()), call.label + " actual location packet");
    auto &visit = state.other.at("source_cluster_visit_globals/prefix8_hex");
    const halo2::SourceSpatialRegistration input{call.object, center, call.radius, call.zero, location, payload,
        fixed<8>(visit), fixed<512>(scratch), fixed<1>(overflow)};
    unsigned cursor = 0;
    halo2::source_register_spatial_object(storage, input, [&](auto cluster, auto actual_center, auto radius, auto maximum, auto output) {
      check(cursor < call.clusters.size(), call.label + " extra cluster producer");
      const auto &leaf = call.clusters[cursor++];
      check(state == states.at(leaf.before), call.label + " complete state at cluster producer entry");
      check(cluster == leaf.cluster && maximum == leaf.maximum &&
            std::bit_cast<std::uint32_t>(radius) == std::bit_cast<std::uint32_t>(leaf.radius), call.label + " original cluster scalar ABI");
      check(std::equal(actual_center.begin(), actual_center.end(), leaf.center.begin()), call.label + " complete cluster center");
      check(std::equal(output.begin(), output.end(), leaf.scratch_before.begin()), call.label + " literal source uninitialized scratch");
      std::copy(leaf.scratch_after.begin(), leaf.scratch_after.end(), output.begin());
      replace(state, states.at(leaf.after)); ++leaves; return leaf.result;
    }, observe);
    check(cursor == call.clusters.size(), call.label + " complete explicit cluster consumption");
    check(overflow == call.packets.at("overflow").after && payload == call.packets.at("payload").after &&
          scratch == call.clusters[0].scratch_after, call.label + " complete source packet results");
    check(std::equal(center.begin(), center.end(), call.packets.at("center").after.begin()) &&
          std::equal(location.begin(), location.end(), call.packets.at("location").after.begin()), call.label + " unchanged center/location packets");
  }
  for (const auto &[key, actual] : state.other) {
    const auto &expected = states.at(call.after).other.at(key);
    check(actual.size() == expected.size(), call.label + " final raw extent " + key);
    for (unsigned i = 0; i < actual.size(); ++i)
      check(actual[i] == expected[i], call.label + " final raw " + key + " byte " + std::to_string(i));
  }
  check(state == states.at(call.after), call.label + " complete final body/header/pool/bitmap/registry/world raw buffers");
  check(stores.size() == call.stores.size(), call.label + " exact ordered native store count");
  for (unsigned i = 0; i < stores.size(); ++i)
    check(stores[i] == call.stores[i], call.label + " exact ordered native write " + std::to_string(i));
  writes += unsigned(stores.size());
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original spatial registry fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); std::map<unsigned, State> states; std::vector<Call> calls;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line); std::vector<std::string> r;
      for (std::string s; stream >> s;) r.push_back(s); if (r.empty()) continue;
      if (r[0] == "B") states[number(r[1])].bodies.emplace(number(r[2]), bytes(r[3]));
      else if (r[0] == "E") states[number(r[1])].other.emplace(r[2], bytes(r[3]));
      else if (r[0] == "A") {
        check(r.size() == 4, "physical alias row extent");
        auto &state = states[number(r[1])];
        check(state.other.at(r[2]) == state.other.at(r[3]), "literal original physical alias bytes");
        check(state.aliases.emplace(r[2], r[3]).second, "unique physical alias");
      }
      else if (r[0] == "C") {
        check(r.size() == 10 && r[9] == "literal_source", "core row extent/authority");
        calls.push_back({r[1], number(r[2]), number(r[3]), number(r[4]), number(r[5]), number(r[6]),
            std::bit_cast<float>(number(r[7])), std::bit_cast<float>(number(r[8])), {}, {}, {}});
      } else if (r[0] == "P") calls.back().packets.emplace(r[1], Packet{bytes(r[2]), bytes(r[3])});
      else if (r[0] == "K") calls.back().clusters.push_back({number(r[1]), number(r[2]), std::int16_t(number(r[3])), std::int16_t(number(r[4])), std::bit_cast<float>(number(r[5])), std::bit_cast<std::int32_t>(number(r[6])), bytes(r[7]), bytes(r[8]), bytes(r[9])});
      else if (r[0] == "W") calls.back().stores.push_back({halo2::SourceSpatialTarget(number(r[1])), number(r[2]), number(r[3]), bytes(r[4]), bytes(r[5])});
      else throw std::runtime_error("Unknown original spatial registry fixture row");
    }
    check(calls.size() == 4, "complete original unsorted registry roster");
    unsigned leaves = 0, writes = 0;
    for (const auto &call : calls) run(call, states, leaves, writes);
    std::cout << calls.size() << " original registry calls/full stored states matched with " << writes
              << " ordered native writes and " << leaves << " explicit original cluster walks\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
