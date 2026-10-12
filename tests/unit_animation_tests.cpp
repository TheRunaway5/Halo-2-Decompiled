#include "halo2/unit_animation.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

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
  check(s.size() % 2 == 0, "hex extent");
  for (std::size_t i = 0; i < s.size(); i += 2)
    b.push_back(std::byte(num(s.substr(i, 2))));
  return b;
}
std::uint32_t get(std::span<const std::byte> b, std::size_t off,
                  unsigned n = 4) {
  check(off + n <= b.size(), "source metadata extent");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[off + i])) << (i * 8);
  return v;
}
void copy(std::span<std::byte> a, std::span<const std::byte> b) {
  check(a.size() == b.size(), "source callback packet extent");
  std::copy(b.begin(), b.end(), a.begin());
}
struct State {
  Bytes manager, channel;
};
struct Boundary {
  char type{};
  std::uint32_t first{}, second{};
  Bytes event;
  State before, after;
};
struct Case {
  std::string name;
  std::uint32_t entry{}, dt{}, frame{}, result{}, nodes{};
  int offset{};
  bool feature{};
  halo2::UnitAnimationCallback callback;
  State before, after;
  std::vector<Boundary> calls;
};
halo2::UnitAnimationDefinition definition(const std::vector<std::string> &r) {
  auto raw = bytes(r[3]), timing = bytes(r[4]), sounds = bytes(r[5]),
       effects = bytes(r[6]), sr = bytes(r[7]), er = bytes(r[8]);
  halo2::UnitAnimationDefinition d;
  d.frame_count = std::int16_t(get(raw, 0x14, 2));
  d.loop_frame = std::int16_t(get(raw, 0x2e, 2));
  d.authored_flags = std::uint8_t(get(raw, 0x18, 1));
  for (std::size_t i = 0; i < timing.size(); i += 4)
    d.timing.push_back(
        {std::int16_t(get(timing, i, 2)), std::int16_t(get(timing, i + 2, 2))});
  for (std::size_t i = 0; i < sounds.size(); i += 8)
    d.sounds.push_back({std::int16_t(get(sounds, i, 2)),
                        std::int16_t(get(sounds, i + 2, 2)),
                        get(sounds, i + 4)});
  for (std::size_t i = 0; i < effects.size(); i += 4)
    d.effects.push_back({std::int16_t(get(effects, i, 2)),
                         std::int16_t(get(effects, i + 2, 2))});
  for (std::size_t i = 0; i < sr.size(); i += 12)
    d.sound_references.push_back(
        {get(sr, i + 4), std::uint16_t(get(sr, i + 8, 2))});
  for (std::size_t i = 0; i < er.size(); i += 12)
    d.effect_references.push_back(
        {get(er, i + 4), std::uint16_t(get(er, i + 8, 2))});
  return d;
}
void run(const Case &c,
         const std::map<std::pair<std::uint32_t, std::uint32_t>,
                        halo2::UnitAnimationDefinition> &definitions,
         unsigned &callbacks) {
  auto state = c.before;
  std::size_t next = 0;
  auto channel = [&]() -> std::span<std::byte> {
    return c.offset >= 0
               ? std::span(state.manager).subspan(std::size_t(c.offset), 32)
               : std::span(state.channel);
  };
  if (c.offset >= 0)
    check(Bytes(channel().begin(), channel().end()) == c.before.channel,
          c.name + " actual channel alias");
  auto boundary = [&](char type, std::span<std::byte> ch) -> const Boundary & {
    check(next < c.calls.size(), c.name + " extra callback");
    const auto &k = c.calls[next++];
    check(k.type == type, c.name + " callback order");
    check(state.manager == k.before.manager,
          c.name + " full manager at callback");
    if (!k.before.channel.empty())
      check(std::equal(ch.begin(), ch.end(), k.before.channel.begin(),
                       k.before.channel.end()),
            c.name + " full channel at callback");
    copy(state.manager, k.after.manager);
    if (!k.after.channel.empty())
      copy(ch, k.after.channel);
    ++callbacks;
    return k;
  };
  halo2::UnitAnimationServices s;
  s.tick_seconds = std::bit_cast<float>(c.dt);
  s.layer_synchronization = c.feature;
  s.definition =
      [&](std::uint32_t graph,
          std::uint32_t packed) -> const halo2::UnitAnimationDefinition & {
    return definitions.at({graph, packed});
  };
  s.event = [&](halo2::UnitAnimationCallback cb, float frame,
                const std::array<std::byte, 20> &event, std::span<std::byte>,
                std::span<std::byte> ch) {
    const auto &k = boundary('E', ch);
    check(cb.identity == c.callback.identity && cb.context == k.first,
          c.name + " actual callback identity/context");
    check(std::bit_cast<std::uint32_t>(frame) == k.second &&
              std::equal(event.begin(), event.end(), k.event.begin(),
                         k.event.end()),
          c.name + " actual event packet/frame");
  };
  s.select_variant = [&](std::span<std::byte>, std::uint32_t old) {
    const auto &k = boundary('V', channel());
    check(old == k.first, c.name + " variant input");
    return k.second;
  };
  s.save_local_transition = [&](std::span<std::byte>, float seconds,
                                unsigned nodes, std::span<std::byte>,
                                std::span<const std::byte>) {
    const auto &k = boundary('T', {});
    check(std::bit_cast<std::uint32_t>(seconds) == k.first && nodes == k.second,
          c.name + " local transition inputs");
  };
  auto invoke = [&](const halo2::UnitAnimationServices &services) {
    unsigned result = 0;
    switch (c.entry) {
    case 0x1cf100:
      result = halo2::update_source_unit_animation_manager(
          state.manager, c.nodes, {}, {}, c.callback, services);
      break;
    case 0x1ca2b0:
      halo2::advance_source_unit_animation_channel(channel(), state.manager,
                                                   c.callback, services);
      break;
    case 0x1ca090:
      halo2::set_source_unit_animation_frame(
          channel(), std::bit_cast<float>(c.frame), state.manager, c.callback,
          services);
      break;
    case 0x1c9f50:
      halo2::clamp_source_unit_animation_frame(
          channel(), std::bit_cast<float>(c.frame), services);
      break;
    case 0x1d0ed0:
      result = halo2::advance_source_unit_animation_transitions(state.manager);
      break;
    default:
      throw std::runtime_error("unrecognized original clock root");
    }
    return result;
  };
  const auto result = invoke(s);
  check(next == c.calls.size(), c.name + " all callbacks consumed");
  check(state.manager == c.after.manager, c.name + " complete manager output");
  if (!c.after.channel.empty())
    check(std::equal(channel().begin(), channel().end(),
                     c.after.channel.begin(), c.after.channel.end()),
          c.name + " complete channel output");
  if (c.entry == 0x1cf100 || c.entry == 0x1d0ed0)
    check(result == c.result, c.name + " source AL");
  if (!c.calls.empty()) {
    state = c.before;
    next = 0;
    auto unavailable = s;
    const auto &first = c.calls.front();
    if (first.type == 'E')
      unavailable.event = {};
    else if (first.type == 'V')
      unavailable.select_variant = {};
    else
      unavailable.save_local_transition = {};
    bool stopped = false;
    try {
      invoke(unavailable);
    } catch (const std::runtime_error &) {
      stopped = true;
    }
    check(stopped && next == 0 && state.manager == first.before.manager,
          c.name + " unavailable producer stops at original owning boundary");
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply independent original animation fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream f(argv[1]);
    std::map<std::pair<std::uint32_t, std::uint32_t>,
             halo2::UnitAnimationDefinition>
        definitions;
    std::vector<Case> cases;
    for (std::string line; std::getline(f, line);) {
      std::istringstream s(line);
      std::vector<std::string> r;
      for (std::string q; s >> q;)
        r.push_back(q);
      if (r.empty())
        continue;
      if (r[0] == "D") {
        check(r.size() == 9, "definition row");
        definitions.emplace(std::pair(num(r[1]), num(r[2])), definition(r));
      } else if (r[0] == "C") {
        check(r.size() == 11, "case row");
        Case c;
        c.name = r[1];
        c.entry = num(r[2]);
        c.offset = std::stoi(r[3]);
        c.dt = num(r[4]);
        c.feature = r[5] != "0";
        c.callback = {num(r[6]), num(r[7])};
        c.frame = num(r[8]);
        c.nodes = std::stoul(r[9]);
        c.result = std::stoul(r[10]);
        cases.push_back(std::move(c));
      } else if (r[0] == "K") {
        check(r.size() >= 4, "boundary row");
        Boundary k;
        k.type = r[1][0];
        k.first = num(r[2]);
        k.second = num(r[3]);
        if (k.type == 'E')
          k.event = bytes(r.at(4));
        cases.back().calls.push_back(std::move(k));
      } else if (r[0] == "B" || r[0] == "F" || r[0] == "I" || r[0] == "O") {
        auto &c = cases.back();
        auto *st = r[0] == "B"   ? &c.before
                   : r[0] == "F" ? &c.after
                   : r[0] == "I" ? &c.calls.back().before
                                 : &c.calls.back().after;
        (r[1] == "manager" ? st->manager : st->channel) = bytes(r.at(2));
      } else
        throw std::runtime_error("Unknown fixture row");
    }
    unsigned callbacks = 0;
    for (const auto &c : cases)
      run(c, definitions, callbacks);
    check(!cases.empty(), "No original animation cases");
    std::cout << cases.size() << " original manager/channel roots and "
              << callbacks << " source callback packets matched\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
