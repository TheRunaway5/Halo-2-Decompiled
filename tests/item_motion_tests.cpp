#include "halo2/item_motion.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value, const std::string &context) {
  if (!value) throw std::runtime_error("Original item motion differs: " + context);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size() % 2 == 0, "hex extent");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < s.size(); i += 2) out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
template<std::size_t N> std::array<std::byte, N> buffer(const std::string &s) {
  const auto raw = bytes(s); check(raw.size() == N, "source fixed buffer extent");
  std::array<std::byte, N> result; std::copy(raw.begin(), raw.end(), result.begin()); return result;
}
struct State {
  std::map<std::uint32_t, std::vector<std::byte>> bodies;
  std::map<std::string, std::vector<std::byte>> other;
  friend bool operator==(const State &, const State &) = default;
};
struct Packet { std::vector<std::byte> before, after; };
struct Callback {
  std::uint32_t entry; unsigned before, after; std::uint32_t result;
  std::map<std::string, Packet> packets;
};
struct Motion {
  std::string label; std::uint32_t entry, datum; std::uint8_t type;
  unsigned before, after; std::uint32_t result;
  std::array<float, 8> values;
  std::uint16_t bank;
  std::array<std::byte, 0x5c> scratch;
  std::vector<Callback> callbacks;
  std::vector<halo2::SourceItemMotionStore> stores;
};
void replace(State &state, const State &source) {
  auto copy = [&](auto &to, const auto &from) {
    check(to.size() == from.size(), "literal leaf buffer extent");
    std::copy(from.begin(), from.end(), to.begin());
  };
  check(state.bodies.size() == source.bodies.size() && state.other.size() == source.other.size(),
        "literal leaf record roster");
  for (auto &[key, value] : state.bodies) copy(value, source.bodies.at(key));
  for (auto &[key, value] : state.other) copy(value, source.other.at(key));
}
void run(const Motion &motion, const std::map<unsigned, State> &states,
         const std::map<std::uint32_t, std::vector<std::byte>> &definitions,
         unsigned &leaves, unsigned &stores) {
  auto state = states.at(motion.before);
  auto scratch = motion.scratch;
  auto &body = state.bodies.at(motion.datum);
  const auto definition = number([&] {
    std::string s; constexpr char h[] = "0123456789abcdef";
    for (int i = 3; i >= 0; --i) { const auto b = std::to_integer<unsigned>(body[i]); s += h[b >> 4]; s += h[b & 15]; }
    return s;
  }());
  const halo2::SourceItemMotionStorage storage{motion.datum, motion.type, body, definitions.at(definition)};
  const auto &v = motion.values;
  halo2::SourceItemMotionInputs input{v[0], v[1], v[2], v[3], v[4], {v[5], v[6], v[7]}, motion.bank, scratch};
  std::size_t cursor = 0;
  auto enter = [&](std::uint32_t entry) -> const Callback & {
    check(cursor < motion.callbacks.size(), motion.label + " extra leaf");
    const auto &leaf = motion.callbacks[cursor++];
    check(leaf.entry == entry, motion.label + " leaf order");
    check(state == states.at(leaf.before), motion.label + " full raw state at leaf entry");
    return leaf;
  };
  auto packet = [&](const Callback &leaf, const char *field, auto raw) {
    const auto &expected = leaf.packets.at(field).before;
    check(std::equal(raw.begin(), raw.end(), expected.begin(), expected.end()),
          motion.label + " source ABI packet " + field);
  };
  auto finish = [&](const Callback &leaf) { replace(state, states.at(leaf.after)); ++leaves; };
  halo2::SourceItemMotionServices services;
  services.make_moving = [&](auto datum) {
    check(datum == motion.datum, motion.label + " moving full datum"); finish(enter(0xbc910));
  };
  services.cast = [&](auto mask, auto start, auto delta, auto ignored, auto second, auto out) {
    const auto &leaf = enter(0x16b240);
    check(mask == 0x24909c0du && ignored == 0xffffffffu && second == 0xffffffffu,
          motion.label + " exact outer arguments");
    packet(leaf, "stack1_start", start); packet(leaf, "stack2_delta", delta);
    packet(leaf, "stack5_result5c", out);
    const auto &after = leaf.packets.at("stack5_result5c").after;
    std::copy(after.begin(), after.end(), out.begin()); finish(leaf);
    return (leaf.result & 0xffu) != 0;
  };
  services.root_transform = [&](auto datum, auto point, auto location) {
    check(datum == motion.datum, motion.label + " root full datum");
    const auto &leaf = enter(0xb8490);
    packet(leaf, "eax_point", point); packet(leaf, "stack1_location8", location); finish(leaf);
  };
  services.contact_effect = [&](auto datum, auto linear, auto contact, auto point) {
    check(datum == motion.datum, motion.label + " effect full datum");
    const auto &leaf = enter(0xfaf20);
    packet(leaf, "ecx_velocity", linear); packet(leaf, "esi_contact5c", contact);
    packet(leaf, "stack1_point", point); finish(leaf);
  };
  services.contact_transform = [&](auto datum, auto contact_point, auto normal, auto point) {
    check(datum == motion.datum && normal.data() == contact_point.data() + 0x20,
          motion.label + " actual transform full datum/contact packet pointers");
    const auto &leaf = enter(0xfbbe0);
    // Frozen field name preserves its earlier literal over-wide observation;
    // original FBBE0/143860 instructions prove this is point input, not matrix.
    const auto &window = leaf.packets.at("eax_matrix34_output");
    const auto backing = std::span<const std::byte>(contact_point.data(), 0x34);
    packet(leaf, "eax_matrix34_output", backing); packet(leaf, "edi_normal", normal);
    packet(leaf, "stack1_point", point);
    check(window.before == window.after, motion.label + " complete unchanged source input window");
    const auto &after = leaf.packets.at("stack1_point").after;
    std::copy(after.begin(), after.end(), point.begin());
    check(std::equal(point.begin(), point.end(), after.begin()), motion.label + " actual adjusted point output");
    check(std::equal(normal.begin(), normal.end(), leaf.packets.at("edi_normal").after.begin()),
          motion.label + " unchanged source normal input"); finish(leaf);
  };
  services.network_dirty = [&](auto, auto) { throw std::runtime_error("Unexecuted source item network branch"); };
  std::vector<halo2::SourceItemMotionStore> observed;
  const auto observe = [&](const auto &event) { observed.push_back(event); };
  if (motion.entry == 0xf9b10)
    check(unsigned(halo2::source_item_update(storage, input, services, observe)) == (motion.result & 0xff),
          motion.label + " original defined AL");
  else { check(motion.entry == 0xf9ea0, motion.label + " actual core entry"); halo2::source_item_motion(storage, input, services, observe); }
  check(cursor == motion.callbacks.size(), motion.label + " complete explicit producer consumption");
  check(state == states.at(motion.after), motion.label + " complete final raw owning buffers");
  check(observed == motion.stores, motion.label + " ordered body stores, including unchanged writes");
  stores += unsigned(observed.size());
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original item-motion fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); std::map<unsigned, State> states;
    std::map<std::uint32_t, std::vector<std::byte>> definitions; std::vector<Motion> motions;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line); std::vector<std::string> r;
      for (std::string s; stream >> s;) r.push_back(s); if (r.empty()) continue;
      if (r[0] == "B") states[number(r[1])].bodies.emplace(number(r[2]), bytes(r[3]));
      else if (r[0] == "E") states[number(r[1])].other.emplace(r[2], bytes(r[3]));
      else if (r[0] == "D") definitions.emplace(number(r[1]), bytes(r[2]));
      else if (r[0] == "M") {
        check(r.size() == 19, "motion row extent");
        Motion m{r[1], number(r[2]), number(r[3]), std::uint8_t(number(r[4])), number(r[5]), number(r[6]), number(r[7]), {}, std::uint16_t(number(r[16])), buffer<0x5c>(r[17]), {}, {}};
        for (unsigned i = 0; i < 8; ++i) m.values[i] = std::bit_cast<float>(number(r[8 + i]));
        check(r[18] == "literal_source", "motion authority marker"); motions.push_back(std::move(m));
      } else if (r[0] == "K") motions.back().callbacks.push_back({number(r[1]), number(r[2]), number(r[3]), number(r[4]), {}});
      else if (r[0] == "P") motions.back().callbacks.back().packets.emplace(r[1], Packet{bytes(r[2]), bytes(r[3])});
      else if (r[0] == "W") motions.back().stores.push_back({number(r[1]), number(r[2]), bytes(r[3]), bytes(r[4])});
      else throw std::runtime_error("Unknown item-motion fixture row");
    }
    check(motions.size() == 6 && states.size() == 29 && definitions.size() == 2,
          "complete independently measured motion suite");
    unsigned leaves = 0, stores = 0;
    for (const auto &motion : motions) run(motion, states, definitions, leaves, stores);
    std::cout << motions.size() << " original item core calls/full raw states matched with "
              << leaves << " explicit original-output leaves and " << stores << " ordered body stores\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
