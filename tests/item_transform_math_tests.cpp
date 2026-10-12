#include "halo2/item_transform_math.hpp"
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
  if (!value) throw std::runtime_error("Original item matrix differs: " + context);
}
std::uint32_t number(const std::string &value) { return std::stoul(value, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &value) {
  check(value.size() % 2 == 0, "hex extent"); std::vector<std::byte> result;
  for (std::size_t i = 0; i < value.size(); i += 2)
    result.push_back(std::byte(number(value.substr(i, 2))));
  return result;
}
template<std::size_t N> std::array<std::byte, N> buffer(const std::string &value) {
  const auto raw = bytes(value); check(raw.size() == N, "fixed raw extent");
  std::array<std::byte, N> result; std::copy(raw.begin(), raw.end(), result.begin()); return result;
}
struct Packet { std::string field; std::uint32_t address; std::vector<std::byte> before, after; };
struct Call {
  std::string label; std::uint32_t entry; float one, scale;
  std::uint32_t handedness;
  std::array<std::uint32_t, 4> sign_mask;
  std::array<std::byte, 8> lazy_before, lazy_after;
  std::vector<Packet> packets;
  std::vector<std::pair<std::uint32_t, std::uint32_t>> reciprocal;
  std::vector<halo2::SourceMatrixMathStore> stores;
};
struct Range { std::uint32_t start, end; std::vector<std::byte> data; std::vector<bool> defined; };
void run(const Call &call, unsigned &store_count, unsigned &reciprocals, unsigned &aliases) {
  std::vector<Range> ranges;
  std::vector<Packet> sorted = call.packets;
  std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.address < b.address; });
  for (const auto &packet : sorted) {
    if (!packet.address) continue;
    const auto end = packet.address + std::uint32_t(packet.before.size());
    if (ranges.empty() || ranges.back().end < packet.address) ranges.push_back({packet.address, end, {}, {}});
    else ranges.back().end = std::max(ranges.back().end, end);
  }
  for (auto &range : ranges) { range.data.resize(range.end - range.start); range.defined.resize(range.data.size()); }
  auto memory = [&](std::uint32_t address, std::size_t size) -> std::span<std::byte> {
    for (auto &range : ranges)
      if (range.start <= address && std::uint64_t(address) + size <= range.end)
        return std::span(range.data).subspan(address - range.start, size);
    throw std::runtime_error("Missing literal source pointer backing");
  };
  for (const auto &packet : call.packets) {
    if (!packet.address) continue;
    for (auto &range : ranges) if (range.start <= packet.address && packet.address < range.end) {
      for (unsigned i = 0; i < packet.before.size(); ++i) {
        const auto offset = packet.address - range.start + i;
        check(!range.defined[offset] || range.data[offset] == packet.before[i], call.label + " literal alias input");
        range.data[offset] = packet.before[i]; range.defined[offset] = true;
      }
      break;
    }
  }
  auto find = [&](const char *field) -> const Packet & {
    const auto it = std::find_if(call.packets.begin(), call.packets.end(),
        [&](const auto &packet) { return packet.field == field; });
    check(it != call.packets.end(), call.label + " required source packet"); return *it;
  };
  auto packet = [&]<std::size_t N>(const char *field) -> std::span<std::byte, N> {
    const auto &p = find(field); check(p.address && p.before.size() == N, call.label + " packet extent");
    const auto raw = memory(p.address, N); return std::span<std::byte, N>(raw.data(), N);
  };
  auto lazy = call.lazy_before;
  std::vector<halo2::SourceMatrixMathStore> stores;
  const auto observe = [&](const auto &store) { stores.push_back(store); };
  unsigned cursor = 0;
  halo2::SourceNodeMatrixInputs input{call.one, call.sign_mask, lazy, [&](float value) {
    check(cursor < call.reciprocal.size(), call.label + " extra RCPSS request");
    const auto [before, after] = call.reciprocal[cursor++];
    check(std::bit_cast<std::uint32_t>(value) == before, call.label + " actual RCPSS input");
    return std::bit_cast<float>(after);
  }};
  if (call.entry == 0x143960)
    halo2::source_item_node_matrix(packet.template operator()<0x20>("s1_node_record20"),
        packet.template operator()<0x34>("s0_matrix34_output"), input, observe);
  else if (call.entry == 0x1441d0) {
    const auto &out = find("stack0_matrix34_output");
    if (find("ecx_matrix34_left").address == out.address || find("edx_matrix34_right").address == out.address) ++aliases;
    halo2::source_compose_item_matrix(packet.template operator()<0x34>("ecx_matrix34_left"),
        packet.template operator()<0x34>("edx_matrix34_right"),
        packet.template operator()<0x34>("stack0_matrix34_output"), observe);
  } else {
    check(call.entry == 0xbeb30 && !find("s3_parent_matrix34").address, call.label + " measured parent-NONE branch");
    halo2::source_item_root_matrix(packet.template operator()<12>("eax_up12"),
        packet.template operator()<12>("ecx_forward12"), packet.template operator()<12>("s0_root12"),
        call.scale, call.handedness, call.one, packet.template operator()<0x34>("s4_matrix34_output"), observe);
  }
  check(cursor == call.reciprocal.size(), call.label + " complete reciprocal boundary consumption");
  check(lazy == call.lazy_after, call.label + " complete lazy-global state");
  for (const auto &p : call.packets) if (p.address) {
    const auto actual = memory(p.address, p.after.size());
    for (unsigned i = 0; i < actual.size(); ++i)
      check(actual[i] == p.after[i], call.label + " full packet " + p.field + " byte " + std::to_string(i));
  }
  check(stores.size() == call.stores.size(), call.label + " original ordered store count");
  for (unsigned i = 0; i < stores.size(); ++i)
    check(stores[i] == call.stores[i], call.label + " exact original ordered store " + std::to_string(i));
  store_count += unsigned(stores.size()); reciprocals += cursor;
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original item matrix fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); std::vector<Call> calls;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line); std::vector<std::string> r;
      for (std::string value; stream >> value;) r.push_back(value); if (r.empty()) continue;
      if (r[0] == "C") {
        check(r.size() == 10 && r[9] == "literal_source", "call row extent/authority");
        Call call{r[1], number(r[2]), std::bit_cast<float>(number(r[3])), std::bit_cast<float>(number(r[7])), number(r[8]), {}, buffer<8>(r[5]), buffer<8>(r[6]), {}, {}, {}};
        const auto mask = buffer<16>(r[4]);
        for (unsigned i = 0; i < 4; ++i) {
          call.sign_mask[i] = 0;
          for (unsigned b = 0; b < 4; ++b) call.sign_mask[i] |= std::to_integer<unsigned>(mask[i * 4 + b]) << (b * 8);
        }
        calls.push_back(std::move(call));
      } else if (r[0] == "P") calls.back().packets.push_back({r[1], number(r[2]), r[3] == "-" ? std::vector<std::byte>{} : bytes(r[3]), r[4] == "-" ? std::vector<std::byte>{} : bytes(r[4])});
      else if (r[0] == "R") calls.back().reciprocal.emplace_back(number(r[1]), number(r[2]));
      else if (r[0] == "W") calls.back().stores.push_back({number(r[1]), number(r[2]), bytes(r[3]), bytes(r[4])});
      else throw std::runtime_error("Unknown original item matrix fixture row");
    }
    check(calls.size() == 46, "complete independently measured math-call roster");
    unsigned stores = 0, reciprocals = 0, aliases = 0;
    for (const auto &call : calls) run(call, stores, reciprocals, aliases);
    std::cout << calls.size() << " original node/composition/root math calls matched, " << stores
              << " ordered matrix writes, " << aliases << " in-place compositions and "
              << reciprocals << " explicit original RCPSS results\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
