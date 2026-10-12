#include "halo2/original_data.hpp"
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F function, const char *message) {
  bool rejected = false;
  try { function(); } catch (const halo2::FormatError &) { rejected = true; }
  check(rejected, message);
}
struct Temporary {
  std::filesystem::path path;
  Temporary() : path(std::filesystem::temp_directory_path() /
      ("halo2-original-data-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
    if (!std::filesystem::create_directory(path))
      throw std::runtime_error("cannot create disposable original-data test directory");
  }
  ~Temporary() { std::error_code error; std::filesystem::remove_all(path,error); }
};
void write(const std::filesystem::path &path, const std::vector<char> &bytes) {
  std::ofstream stream(path, std::ios::binary);
  if (!stream.write(bytes.data(), bytes.size()))
    throw std::runtime_error("cannot write disposable original-data fixture");
}
void same_tables(const halo2::OriginalData &a, const halo2::OriginalData &b) {
  check(a.fade_curve == b.fade_curve, "original fade tables differ");
  check(a.collision_projection == b.collision_projection,
        "original collision projection tables differ");
  for (unsigned i = 0; i < a.weapon_directions.size(); ++i) {
    const auto x = a.weapon_directions[i], y = b.weapon_directions[i];
    check(std::bit_cast<std::uint32_t>(x.x) == std::bit_cast<std::uint32_t>(y.x) &&
          std::bit_cast<std::uint32_t>(x.y) == std::bit_cast<std::uint32_t>(y.y) &&
          std::bit_cast<std::uint32_t>(x.z) == std::bit_cast<std::uint32_t>(y.z),
          "original direction-table decode mismatch");
  }
}
void balance_words(const halo2::OriginalData &data) {
  check(data.multiplayer_balance_constants.has_value(), "update constants absent");
  // Frozen original 76-byte concatenation, independent of native tuning code.
  std::uint64_t hash = 0xcbf29ce484222325ull;
  for (const auto word : *data.multiplayer_balance_constants)
    for (unsigned shift = 0; shift < 32; shift += 8)
      hash = (hash ^ ((word >> shift) & 255)) * 0x100000001b3ull;
  check(hash == 0x488cd915e8c2e81full, "original balance words/order differ");
}
std::uint64_t word_hash(std::span<const std::uint32_t> words) {
  std::uint64_t hash = 0xcbf29ce484222325ull;
  for (const auto word : words)
    for (unsigned shift = 0; shift < 32; shift += 8)
      hash = (hash ^ ((word >> shift) & 255)) * 0x100000001b3ull;
  return hash;
}
void netgame_words(const halo2::OriginalData &data) {
  check(data.multiplayer_netgame_postprocess_constants.has_value(), "netgame source constants absent");
  const auto &c = *data.multiplayer_netgame_postprocess_constants;
  const auto &b = c.beaver_creek_targets;
  check(b[0][2] == b[1][2], "shared original Beaver Creek Z operand differs");
  const std::array<std::uint32_t,7> original11_order{
      b[0][2], c.distance_squared_threshold, c.beaver_creek_x_delta,
      b[0][0], b[0][1], b[1][0], b[1][1]};
  check(word_hash(original11_order) == 0xd15c8d0c31afae08ull,
        "original Beaver Creek source words/semantic mapping differ");
  if (data.image == halo2::OriginalData::Image::MapPack11) {
    check(!c.relic, "1.1 received invented Relic constants");
  } else {
    check(data.image == halo2::OriginalData::Image::TitleUpdate15 && c.relic.has_value(),
          "1.5 Relic source constants absent");
    const auto &r = *c.relic;
    const std::array<std::uint32_t,14> original15_order{
        b[0][2], c.distance_squared_threshold, c.beaver_creek_x_delta,
        b[0][1], b[0][0], b[1][1], b[1][0],
        r.targets[0][0], r.z_delta, r.targets[0][1], r.targets[0][2],
        r.targets[1][0], r.targets[1][1], r.targets[1][2]};
    check(word_hash(original15_order) == 0xd08abe82ba07a4baull,
          "original 1.5 netgame source words/semantic mapping differ");
  }
}
std::vector<char> read(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot read original-data test source");
  return std::vector<char>((std::istreambuf_iterator<char>(stream)), {});
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 3 && argc != 4)
      throw std::runtime_error("supply base and 1.1 executable paths, optionally 1.5");
    Temporary temporary;
    rejects([&] { halo2::OriginalData::load_xbe(temporary.path / "missing.xbe"); },
            "missing original data silently substituted");
    write(temporary.path / "bad.xbe", std::vector<char>(512, 0));
    rejects([&] { halo2::OriginalData::load_xbe(temporary.path / "bad.xbe"); },
            "invalid original signature accepted");
    write(temporary.path / "short.xbe", std::vector<char>(8, 0));
    rejects([&] { halo2::OriginalData::load_xbe(temporary.path / "short.xbe"); },
            "truncated original data accepted");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) ||
        (argc == 4 && !std::filesystem::exists(argv[3])))
      return 77;
    const auto base = halo2::OriginalData::load_xbe(argv[1]);
    const auto update = halo2::OriginalData::load_xbe(argv[2]);
    check(base->image == halo2::OriginalData::Image::Base &&
          update->image == halo2::OriginalData::Image::MapPack11, "image provenance mismatch");
    same_tables(*base, *update);
    check(!base->multiplayer_balance_constants, "base executable received a patch");
    check(!base->multiplayer_netgame_postprocess_constants, "base executable received a netgame patch");
    check(!base->full_scoreboard_constants, "base executable received unverified scoreboard addresses");
    balance_words(*update);
    netgame_words(*update);
    check(base->fade_curve.front() == 0 && base->fade_curve.back() == 255,
          "authored fade endpoint decode mismatch");
    if (argc == 4) {
      const auto latest = halo2::OriginalData::load_xbe(argv[3]);
      check(latest->image == halo2::OriginalData::Image::TitleUpdate15 &&
            latest->source == std::filesystem::path(argv[3]), "1.5 image provenance mismatch");
      same_tables(*update, *latest);
      balance_words(*latest);
      netgame_words(*latest);
      check(latest->multiplayer_balance_constants == update->multiplayer_balance_constants,
            "original update constant arrays differ");
      const auto original = read(argv[3]);
      // Exact raw offsets from the separately frozen original source evidence.
      for (auto offset : {std::size_t(0x104), std::size_t(0x4472e8), std::size_t(0x465d74),
                          std::size_t(0x4462a0), std::size_t(0x465fbc)}) {
        auto changed = original;
        changed.at(offset) ^= 1;
        write(temporary.path / "changed15.xbe", changed);
        rejects([&] { halo2::OriginalData::load_xbe(temporary.path / "changed15.xbe"); },
                "changed 1.5 header/table/constant source accepted");
      }
    }
    auto bytes = read(argv[1]);
    bytes.at(0x104) ^= 1; // Controlled corruption of the supported image header.
    write(temporary.path / "changed.xbe", bytes);
    rejects([&] { halo2::OriginalData::load_xbe(temporary.path / "changed.xbe"); },
            "changed original data accepted");
    std::cout << "Original tables and source balance words verified; absent/corrupt sources rejected"
              << (argc == 4 ? " (base, 1.1, 1.5)\n" : " (base, 1.1)\n");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
