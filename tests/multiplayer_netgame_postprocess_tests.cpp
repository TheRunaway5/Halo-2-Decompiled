#include "halo2/multiplayer_netgame_postprocess.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool condition, const char* reason) {
  if (!condition) throw FormatError(reason);
}
std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input || input.tellg() < 0) throw FormatError("missing netgame differential input");
  std::vector<std::byte> bytes(static_cast<std::size_t>(input.tellg()));
  input.seekg(0);
  if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
    throw FormatError("short netgame differential input");
  return bytes;
}
std::uint32_t read32(std::span<const std::byte> bytes, std::size_t at) {
  check(at <= bytes.size() && bytes.size() - at >= 4, "truncated differential input");
  std::uint32_t out = 0;
  for (unsigned i = 0; i < 4; ++i)
    out |= std::to_integer<std::uint32_t>(bytes[at + i]) << (i * 8);
  return out;
}
void write32(std::span<std::byte> bytes, std::size_t at, std::uint32_t word) {
  check(at <= bytes.size() && bytes.size() - at >= 4, "invalid host-bound fixture write");
  for (unsigned i = 0; i < 4; ++i) bytes[at + i] = std::byte((word >> (i * 8)) & 255);
}
template<class F> void rejects(F function, const char* reason) {
  bool rejected = false;
  try { function(); } catch (const FormatError&) { rejected = true; }
  check(rejected, reason);
}
void compare_trace(const MultiplayerNetgamePostprocessTrace& trace,
                   const std::filesystem::path& fixture) {
  std::ifstream input(fixture);
  check(bool(input), "missing original netgame store/access fixture");
  std::string line;
  std::size_t stores = 0, protection = 0;
  for (; std::getline(input, line);) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream fields(line);
    char kind{};
    fields >> kind >> std::hex;
    if (kind == 'S') {
      MultiplayerNetgamePostprocessStore expected;
      check(bool(fields >> expected.instruction >> expected.file_offset >> expected.before >> expected.after),
            "malformed original netgame store fixture");
      check(stores < trace.stores.size(), "native omitted original netgame store");
      const auto& actual = trace.stores[stores++];
      check(actual.instruction == expected.instruction && actual.file_offset == expected.file_offset &&
            actual.before == expected.before && actual.after == expected.after,
            "native netgame store/order differs from original");
    } else if (kind == 'P') {
      MultiplayerNetgamePostprocessProtection expected;
      check(bool(fields >> expected.instruction >> expected.address >> expected.byte_count >>
            expected.protection >> expected.stores_completed), "malformed original access fixture");
      check(protection < trace.protection_calls.size(), "native omitted original access call");
      const auto& actual = trace.protection_calls[protection++];
      check(actual.instruction == expected.instruction && actual.address == expected.address &&
            actual.byte_count == expected.byte_count && actual.protection == expected.protection &&
            actual.stores_completed == expected.stores_completed, "native access call/order differs from original");
    } else throw FormatError("unknown original netgame fixture row");
  }
  check(stores == trace.stores.size() && protection == trace.protection_calls.size(),
        "native added original netgame stores/access calls");
}
} // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 5 && argc != 6)
      throw FormatError("Supply raw cache, update XBE, original netgame buffer, original trace fixture, optional native output");
    for (unsigned i = 1; i <= 4; ++i)
      if (!std::filesystem::exists(argv[i])) return 77;
    const auto source = OriginalData::load_xbe(argv[2]);
    check(bool(source->multiplayer_netgame_postprocess_constants), "selected image has no netgame postprocess constants");
    const auto image = source->image == OriginalData::Image::TitleUpdate15
        ? MultiplayerNetgamePostprocessImage::TitleUpdate15 : MultiplayerNetgamePostprocessImage::MapPack11;
    auto bytes = read_file(argv[1]);
    const auto raw = bytes;
    const auto index = read32(raw, 16), count = read32(raw, index + 24);
    const auto header_magic = std::int64_t(read32(raw, index)) - (std::int64_t(index) + 32);
    const auto table_value = std::int64_t(read32(raw, index + 8)) - header_magic;
    check(table_value >= 0 && std::uint64_t(table_value) <= raw.size(), "invalid source tag table");
    const auto table = static_cast<std::size_t>(table_value);
    const auto magic = std::int64_t(read32(raw, table + 8)) - (std::int64_t(index) + read32(raw, 20));
    const auto scenario_index = read32(raw, index + 12) & 0xffff;
    check(scenario_index < count, "missing source scenario");
    const auto scenario = read32(raw, table + std::size_t(scenario_index) * 16 + 8);
    const auto scenario_value = std::int64_t(scenario) - magic;
    check(scenario_value >= 0 && std::uint64_t(scenario_value) <= raw.size(), "invalid source scenario pointer");
    const auto scenario_offset = static_cast<std::size_t>(scenario_value);
    const auto flags_count = read32(raw, scenario_offset + 0x118);
    const auto flags_value = std::int64_t(read32(raw, scenario_offset + 0x11c)) - magic;
    check(flags_value >= 0 && std::uint64_t(flags_value) <= raw.size(), "invalid source netgame pointer");
    const auto flags_offset = static_cast<std::size_t>(flags_value);
    const auto flags_size = std::uint64_t(flags_count) * 0x20;
    check(flags_size <= raw.size() - flags_offset, "truncated authored netgame block");
    const auto expected_flags = read_file(argv[3]);
    check(expected_flags.size() == flags_size, "original netgame buffer extent differs");
    auto expected = raw;
    std::copy(expected_flags.begin(), expected_flags.end(), expected.begin() + flags_offset);
    MultiplayerNetgamePostprocessTrace trace;
    const auto result = apply_multiplayer_netgame_postprocess({bytes, magic, scenario},
        *source->multiplayer_netgame_postprocess_constants, image, &trace);
    compare_trace(trace, argv[4]);
    check(bytes == expected, "complete native cache differs from original netgame output");
    check(result.stores == trace.stores.size(), "netgame store count differs from trace");

    // These controlled cases validate the host bounds/early-return boundary;
    // they are not represented as additional original executable runs.
    auto malformed = raw;
    write32(malformed, scenario_offset + 0x39c, 0xffffffff);
    rejects([&] { apply_multiplayer_netgame_postprocess({malformed, magic, scenario},
        *source->multiplayer_netgame_postprocess_constants, image); }, "invalid level pointer escaped host bounds");
    malformed[scenario_offset + 0x10] = std::byte{0};
    malformed[scenario_offset + 0x11] = std::byte{0};
    check(!apply_multiplayer_netgame_postprocess({malformed, magic, scenario},
        *source->multiplayer_netgame_postprocess_constants, image).multiplayer,
        "non-multiplayer early return read invalid level pointer");
    rejects([&] { apply_multiplayer_netgame_postprocess(
        {std::span<std::byte>(malformed).first(scenario_offset + 0x11), magic, scenario},
        *source->multiplayer_netgame_postprocess_constants, image); }, "truncated scenario word escaped host bounds");
    if (argc == 6) {
      std::ofstream output(argv[5], std::ios::binary);
      check(bool(output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size())),
            "could not save private native netgame output");
    }
    std::cout << "{\"map_id\":" << result.map_id << ",\"multiplayer\":"
              << (result.multiplayer ? "true" : "false") << ",\"recognized_map\":"
              << (result.recognized_map ? "true" : "false") << ",\"stores\":" << result.stores
              << ",\"protection_calls\":" << trace.protection_calls.size() << "}\n";
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
