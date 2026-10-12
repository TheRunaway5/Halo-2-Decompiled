#include "halo2/audio.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <tuple>

namespace {
void check(bool passed, const char *message) {
  if (!passed)
    throw halo2::FormatError(message);
}
std::uint64_t fnv(std::span<const std::byte> bytes) {
  std::uint64_t hash = 0xcbf29ce484222325ull;
  for (auto byte : bytes)
    hash = (hash ^ std::to_integer<unsigned>(byte)) * 0x100000001b3ull;
  return hash;
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 3, "Supply original resource fixture and owned project root");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    const std::filesystem::path root(argv[2]);
    std::ifstream fixture(argv[1]);
    std::map<std::filesystem::path, std::unique_ptr<halo2::Cache>> caches;
    std::set<std::tuple<std::uint64_t, std::size_t, unsigned, unsigned, unsigned, unsigned>> decoded;
    unsigned cases = 0;
    std::uint64_t decoded_frames = 0;
    for (std::string line; std::getline(fixture, line);) {
      std::istringstream row(line);
      std::string kind, path, datum, hash;
      unsigned pitch, permutation, codec, channels, rate, sound_class;
      std::uint32_t samples;
      std::size_t byte_count;
      check(bool(row >> kind >> path >> datum >> pitch >> permutation >> codec >>
                 channels >> rate >> sound_class >> samples >> byte_count >> hash) && kind == "S",
            "Incomplete original sound resource fixture");
      const auto map = root / path;
      if (!std::filesystem::exists(map))
        return 77;
      if (!caches.contains(map))
        caches[map] = std::make_unique<halo2::Cache>(map, root / "assets/disc/maps");
      const auto encoded = caches.at(map)->load_sound(std::stoul(datum, nullptr, 16),
                                                      pitch, permutation);
      const auto actual_hash = fnv(encoded.data);
      check(encoded.codec == codec && encoded.channels == channels &&
                encoded.sample_rate == rate && encoded.samples == samples &&
                encoded.sound_class == sound_class &&
                encoded.data.size() == byte_count &&
                actual_hash == std::stoull(hash, nullptr, 16),
            "Native sound import differs from independent original resources");
      // One decode per identical original encoded payload. This validates PC
      // sample availability, not original Xbox ADPCM hardware sample parity,
      // sound selection/admission, listener gains or live playback routing.
      if (decoded.emplace(actual_hash, byte_count, codec, channels, rate, sound_class).second) {
        const auto pcm = halo2::decode_sound(encoded);
        check(pcm.frames() > 0 && pcm.rate == 48000 &&
                  pcm.sound_class == encoded.sound_class,
              "Original sound resources did not produce a usable PC sample");
        bool nonzero = false;
        for (float sample : pcm.stereo) {
          check(std::isfinite(sample), "Nonfinite PC decoded sound sample");
          nonzero |= sample != 0;
        }
        check(nonzero, "Original first-person sample decoded to silence");
        decoded_frames += pcm.frames();
      }
      ++cases;
    }
    check(!caches.empty() && cases != 0, "No actual multiplayer sound resources");
    std::cout << "ownedMaps=" << caches.size() << " originalEncodedCases=" << cases
              << " uniqueDecodedResources=" << decoded.size()
              << " pcDecodedStereoFrames=" << decoded_frames
              << " schedulerSelection=unverified liveRouting=separate\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
