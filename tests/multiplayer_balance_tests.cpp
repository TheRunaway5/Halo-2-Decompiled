#include "halo2/multiplayer_balance.hpp"
#include "halo2/original_data.hpp"
#include "halo2/content.hpp"
#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input || input.tellg() < 0) throw FormatError("missing balance test input");
  std::vector<std::byte> bytes(static_cast<std::size_t>(input.tellg()));
  input.seekg(0);
  if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
    throw FormatError("short balance test input");
  return bytes;
}
void check(bool condition, const char* reason) { if (!condition) throw FormatError(reason); }
bool same(float a, float b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
void consumer_check(const std::filesystem::path& map, const std::filesystem::path& processed,
                    const std::filesystem::path& native_output,
                    const std::shared_ptr<const OriginalData>& source, std::size_t stores) {
  const auto* shared_override = std::getenv("HALO2_TEST_BALANCE_SHARED_MAPS");
  const auto shared = shared_override ? std::filesystem::path(shared_override) : map.parent_path();
  const Cache native(map, shared, source);
  // This reference consumes the unchanged original emulator's expected buffer
  // in differential runs. It imports ordinary disc tables and adds no patch.
  const Cache expected(processed, shared);
  const auto content = native.load_content(0, true), reference = expected.load_content(0, true);
  check(native.multiplayer_balance_writes() == stores, "live Cache omitted original postprocess");
  check(content.original_data == source, "live content lost selected original data source");
  check(content.weapons.size() == reference.weapons.size(), "processed weapon inventory differs");
  for (const auto& [id, weapon] : content.weapons) {
    const auto& other = reference.weapons.at(id);
    check(same(weapon.damage_min, other.damage_min) && same(weapon.damage_max, other.damage_max) &&
          same(weapon.firing_error.normal_low, other.firing_error.normal_low) &&
          same(weapon.firing_error.normal_high, other.firing_error.normal_high) &&
          bool(weapon.damage_amount) == bool(other.damage_amount), "live gameplay values differ from processed source");
    if (weapon.damage_amount)
      check(same(weapon.damage_amount->lower, other.damage_amount->lower) &&
            same(weapon.damage_amount->upper_minimum, other.damage_amount->upper_minimum) &&
            same(weapon.damage_amount->upper_maximum, other.damage_amount->upper_maximum),
            "live damage endpoints differ from processed source");
  }
  // The host compatibility identity must distinguish updated and raw definitions,
  // while accepting identical executable bytes at a different local path.
  const auto copy = native_output.string() + ".xbe";
  check(!std::filesystem::exists(copy), "private test executable copy already exists");
  std::filesystem::copy_file(source->source, copy);
  try {
    const auto raw = multiplayer_fingerprint(map, shared, 0, 0);
    const auto selected = multiplayer_fingerprint(map, shared, 0, 0, source->source);
    check(raw != selected, "LAN admitted raw and updated balance definitions together");
    check(selected == multiplayer_fingerprint(map, shared, 0, 0, copy),
          "LAN update identity depends on local file path");
    auto rules = slayer_rules({"Balance compatibility", 0, {}});
    rules.weapon = native.slayer_starting_weapon(0);
    MultiplayerMatch match(content, 0, native.spawns(), rules);
    LanHost host(match, 0, selected);
    const auto address = "127.0.0.1:" + std::to_string(host.port());
    std::array<Controls, multiplayer_capacity> controls{};
    LanClient wrong(address, raw, 1);
    for (unsigned i = 0; i < 64 && wrong.error().empty(); ++i) {
      wrong.send(std::array<Controls, 1>{}); host.receive(controls); wrong.receive();
    }
    check(!wrong.ready() && !wrong.error().empty() && host.peers() == 0,
          "actual UDP join admitted unprocessed balance data");
    LanClient correct(address, selected, 1);
    for (unsigned i = 0; i < 64 && !correct.ready(); ++i) {
      correct.send(std::array<Controls, 1>{}); host.receive(controls);
      host.broadcast(); correct.receive();
    }
    check(correct.ready() && correct.slots().size() == 1 && host.peers() == 1,
          "actual UDP join rejected matching balance data");
  } catch (...) { std::filesystem::remove(copy); throw; }
  std::filesystem::remove(copy);
}
} // namespace
int main(int argc, char** argv) {
  try {
    if (argc < 4 || argc > 7 || argc == 6)
      throw FormatError("Supply raw cache, update XBE, original store fixture (or -), optional output and --content ORIGINAL_OUTPUT");
    const bool expect_bounds_stop = std::string(argv[3]) == "--bounds-stop";
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) ||
        (std::string(argv[3]) != "-" && !expect_bounds_stop && !std::filesystem::exists(argv[3]))) return 77;
    const auto source = OriginalData::load_xbe(argv[2]);
    check(bool(source->multiplayer_balance_constants), "selected image has no verified balance routine");
    auto bytes = read_file(argv[1]);
    const Bytes data(bytes);
    const auto index = data.u32(16), count = data.u32(index + 24);
    const auto header_magic = std::int64_t(data.u32(index)) - (std::int64_t(index) + 32);
    const auto table = std::size_t(std::int64_t(data.u32(index + 8)) - header_magic);
    const auto magic = std::int64_t(data.u32(table + 8)) - (std::int64_t(index) + data.u32(20));
    const auto scenario = data.u32(table + std::size_t(data.u32(index + 12) & 0xffff) * 16 + 8);
    const auto globals = data.u32(table + std::size_t(data.u32(index + 16) & 0xffff) * 16 + 8);
    std::vector<MultiplayerBalanceStore> trace;
    MultiplayerBalanceResult result;
    bool bounds_stopped = false;
    try {
      result = apply_multiplayer_balance({bytes, magic, table, count, scenario, globals},
          *source->multiplayer_balance_constants,
          source->image == OriginalData::Image::TitleUpdate15 ? 0x13e0 : 0, &trace);
    } catch (const FormatError&) {
      if (!expect_bounds_stop) throw;
      bounds_stopped = true;
    }
    check(bounds_stopped == expect_bounds_stop, "original invalid reference unexpectedly returned normally");
    if (std::string(argv[3]) != "-" && !expect_bounds_stop) {
      std::ifstream input(argv[3]);
      std::string line;
      std::size_t row = 0;
      for (; std::getline(input, line);) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        MultiplayerBalanceStore expected;
        check(bool(fields >> std::hex >> expected.instruction >> expected.file_offset >> expected.before >> expected.after),
              "malformed original store fixture");
        check(row < trace.size(), "native omitted original metadata store");
        const auto& actual = trace[row++];
        check(actual.instruction == expected.instruction && actual.file_offset == expected.file_offset &&
              actual.before == expected.before && actual.after == expected.after, "native store differs from original instructions");
      }
      check(row == trace.size(), "native emitted additional metadata stores");
    }
    if (argc >= 5) {
      std::ofstream output(argv[4], std::ios::binary);
      if (!output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()))
        throw FormatError("could not write private processed test cache");
    }
    if (argc == 7) {
      check(std::string(argv[5]) == "--content", "unknown balance test option");
      consumer_check(argv[1], argv[6], argv[4], source, result.stores);
    }
    if (bounds_stopped) std::cout << "{\"returned\":false,\"host_bounds_rejected\":true,\"stores\":[";
    else std::cout << "{\"admitted\":" << (result.admitted ? "true" : "false") << ",\"stores\":[";
    for (std::size_t i = 0; i < trace.size(); ++i) {
      if (i) std::cout << ',';
      const auto& row = trace[i];
      std::cout << '[' << row.instruction << ',' << row.file_offset << ',' << row.before << ',' << row.after << ']';
    }
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
