#include "halo2/item_nodes.hpp"
#include "halo2/item_bounds.hpp"
#include <algorithm>
#include <array>
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
  if (!value) throw std::runtime_error("Original item node/bounds differs: " + context);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  std::vector<std::byte> out; check(s.size() % 2 == 0, "hex extent");
  for (std::size_t i = 0; i < s.size(); i += 2) out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
template<class T> T read(const auto &bytes, std::size_t offset) {
  check(offset + sizeof(T) <= bytes.size(), "literal read extent");
  T value; std::memcpy(&value, bytes.data() + offset, sizeof value); return value;
}
struct State {
  std::map<std::uint32_t, std::vector<std::byte>> bodies;
  std::map<std::string, std::vector<std::byte>> other;
  friend bool operator==(const State &, const State &) = default;
};
struct Tag { std::uint32_t address; std::vector<std::byte> raw; };
struct Call {
  std::string label; std::uint32_t entry, datum; unsigned before, after, result;
  float one, zero, move_limit; std::array<std::uint32_t, 4> sign_mask;
  std::vector<std::pair<std::uint32_t, std::uint32_t>> reciprocal;
  std::vector<std::uint32_t> callbacks;
  std::vector<halo2::SourceItemNodeStore> stores;
};
void run(const Call &call, const std::map<unsigned, State> &states,
    const std::map<std::uint32_t, Tag> &tags, unsigned &writes, unsigned &reciprocals) {
  auto state = states.at(call.before); auto &body = state.bodies.at(call.datum);
  const auto &definition = tags.at(read<std::uint32_t>(body, 0)).raw;
  std::vector<halo2::SourceItemNodeStore> stores;
  unsigned cursor = 0;
  if (call.entry == 0xbdf80) {
    const auto &model = tags.at(read<std::uint32_t>(definition, 0x38)).raw;
    const auto &render = tags.at(read<std::uint32_t>(model, 4));
    const auto count = read<std::uint32_t>(render.raw, 0x48);
    const auto address = read<std::uint32_t>(render.raw, 0x4c);
    check(address >= render.address && std::uint64_t(address - render.address) + count * 0x60 <= render.raw.size(),
          call.label + " complete actual render node backing");
    auto &fallback = state.other.at("source_fallback_node_bank/raw_hex");
    auto &lazy = state.other.at("source_quaternion_lazy_constants/raw8_hex");
    check(lazy.size() == 8, call.label + " full source lazy buffer");
    halo2::SourceNodeMatrixInputs input{call.one, call.sign_mask,
        std::span<std::byte, 8>(lazy.data(), 8), [&](float value) {
          check(cursor < call.reciprocal.size(), call.label + " extra source reciprocal request");
          const auto [before, after] = call.reciprocal[cursor++];
          check(std::bit_cast<std::uint32_t>(value) == before, call.label + " exact source RCPSS input");
          return std::bit_cast<float>(after);
        }};
    halo2::SourceItemNodesStorage storage{body, definition, model, render.raw,
        std::span<const std::byte>(render.raw).subspan(address - render.address, count * 0x60), fallback};
    halo2::source_item_current_nodes(storage, input, call.callbacks,
        [&](const auto &event) { stores.push_back(event); });
  } else {
    check(call.entry == 0xbede0, call.label + " actual producer entry");
    const auto result = halo2::source_item_bounds(body, definition,
        {call.one, call.zero, call.move_limit}, [&](const auto &event) { stores.push_back({false, event}); });
    check(unsigned(result) == (call.result & 0xffu), call.label + " original defined AL");
  }
  check(cursor == call.reciprocal.size(), call.label + " complete source reciprocal consumption");
  check(state.bodies.size() == states.at(call.after).bodies.size(), call.label + " source body roster");
  for (const auto &[datum, actual] : state.bodies) {
    const auto &expected = states.at(call.after).bodies.at(datum);
    check(actual.size() == expected.size(), call.label + " complete source body extent");
    for (unsigned i = 0; i < actual.size(); ++i)
      check(actual[i] == expected[i], call.label + " full body datum " + std::to_string(datum) + " byte " + std::to_string(i));
  }
  check(state.other == states.at(call.after).other, call.label + " complete other source raw storage");
  check(stores.size() == call.stores.size(), call.label + " exact original ordered store count");
  for (unsigned i = 0; i < stores.size(); ++i)
    check(stores[i] == call.stores[i], call.label + " exact original ordered write " + std::to_string(i));
  writes += unsigned(stores.size()); reciprocals += cursor;
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original item node/bounds fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); std::map<unsigned, State> states;
    std::map<std::uint32_t, Tag> tags; std::vector<Call> calls;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line); std::vector<std::string> r;
      for (std::string s; stream >> s;) r.push_back(s); if (r.empty()) continue;
      if (r[0] == "B") states[number(r[1])].bodies.emplace(number(r[2]), bytes(r[3]));
      else if (r[0] == "E") states[number(r[1])].other.emplace(r[2], bytes(r[3]));
      else if (r[0] == "D") tags.emplace(number(r[1]), Tag{number(r[2]), bytes(r[3])});
      else if (r[0] == "C") {
        check(r.size() == 12 && r[11] == "literal_source", "call row extent/authority");
        Call c{r[1], number(r[2]), number(r[3]), number(r[4]), number(r[5]), number(r[6]),
            std::bit_cast<float>(number(r[7])), std::bit_cast<float>(number(r[8])),
            std::bit_cast<float>(number(r[9])), {}, {}, {}, {}};
        const auto mask = bytes(r[10]); check(mask.size() == 16, "source sign-mask extent");
        for (unsigned i = 0; i < 4; ++i) c.sign_mask[i] = read<std::uint32_t>(mask, i * 4);
        calls.push_back(std::move(c));
      } else if (r[0] == "R") calls.back().reciprocal.emplace_back(number(r[1]), number(r[2]));
      else if (r[0] == "K") calls.back().callbacks.push_back(number(r[1]));
      else if (r[0] == "W") calls.back().stores.push_back({r[1] == "F", {number(r[2]), number(r[3]), bytes(r[4]), bytes(r[5])}});
      else throw std::runtime_error("Unknown original item node/bounds fixture row");
    }
    check(calls.size() == 8 && tags.size() == 6, "complete measured node/bounds roster");
    unsigned writes = 0, reciprocal = 0;
    for (const auto &call : calls) run(call, states, tags, writes, reciprocal);
    std::cout << calls.size() << " original node/bounds calls matched complete raw states with "
              << writes << " ordered body/fallback writes and " << reciprocal << " original RCPSS results\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
