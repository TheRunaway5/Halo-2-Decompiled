#include "halo2/audio.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace {
void check(bool value, const char *message) {
  if (!value)
    throw halo2::FormatError(message);
}
std::uint64_t fnv(std::span<const std::byte> bytes) {
  std::uint64_t value = 0xcbf29ce484222325ull;
  for (auto byte : bytes)
    value = (value ^ std::to_integer<unsigned>(byte)) * 0x100000001b3ull;
  return value;
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 3 || argc == 4, "Supply literal sound fixture, owned project root and optional MCC shared directory");
    const bool mcc = argc == 4;
    if (!std::filesystem::exists(argv[1]))
      return 77;
    const std::filesystem::path root(argv[2]);
    std::ifstream fixture(argv[1]);
    std::filesystem::path current_map;
    std::unique_ptr<halo2::Cache> cache;
    std::map<std::filesystem::path, std::ifstream> files;
    using ResourceKey = std::tuple<std::string, std::size_t, std::size_t>;
    std::map<ResourceKey, std::vector<std::byte>> resources;
    std::set<std::filesystem::path> maps;
    std::set<std::tuple<std::uint64_t, std::size_t, unsigned, unsigned, unsigned, unsigned>> decoded;
    std::size_t cases = 0, decode_failures = 0, silent_resources = 0;
    std::uint64_t checked_bytes = 0, decoded_frames = 0;
    for (std::string line; std::getline(fixture, line);) {
      std::istringstream row(line);
      std::string kind, path, datum;
      unsigned pitch, permutation, codec, channels, rate, sound_class, chunk_count;
      std::uint32_t samples;
      check(bool(row >> kind >> std::quoted(path) >> datum >> pitch >> permutation >> codec >>
                 channels >> rate >> sound_class >> samples >> chunk_count) && kind == "S",
            "Incomplete authored whole-map sound fixture");
      const auto map = root / path;
      if (!std::filesystem::exists(map))
        return 77;
      if (map != current_map) {
        try {
          cache = std::make_unique<halo2::Cache>(map, mcc ? std::filesystem::path(argv[3]) : root / "assets/disc/maps");
        } catch(const std::exception& error) {
          throw halo2::FormatError("Native cache construction failed for " + map.string() + ": " + error.what());
        }
        check(cache->is_mcc() == mcc, "Literal fixture resource namespace differs from its cache format");
        current_map = map;
        maps.insert(map);
      }
      std::vector<std::byte> expected;
      for (unsigned n = 0; n < chunk_count; ++n) {
        check(bool(std::getline(fixture, line)), "Missing original sound chunk descriptor");
        std::istringstream chunk(line);
        std::string source, pointer, size_flags;
        std::size_t offset, size;
        check(bool(chunk >> kind >> std::quoted(source) >> offset >> size >> pointer >> size_flags) && kind == "C",
              "Incomplete original sound chunk descriptor");
        check((std::stoul(pointer, nullptr, 16) & (mcc ? 0xffffffffu : 0x3fffffffu)) == offset &&
                  (std::stoul(size_flags, nullptr, 16) & (mcc ? 0x1fffffffu : 0x3fffffffu)) == size,
              "Literal original sound descriptor disagrees with owned-file range");
        const ResourceKey key{source, offset, size};
        auto found = resources.find(key);
        if (found == resources.end()) {
          const auto file = root / source;
          if (!std::filesystem::exists(file))
            return 77;
          if (!files.contains(file))
            files.try_emplace(file, file, std::ios::binary);
          auto &stream = files.at(file);
          check(bool(stream), "Cannot open original sound resource file");
          stream.seekg(std::streamoff(offset));
          std::vector<std::byte> bytes(size);
          stream.read(reinterpret_cast<char *>(bytes.data()), std::streamsize(size));
          check(std::size_t(stream.gcount()) == size, "Original sound resource read was incomplete");
          found = resources.emplace(key, std::move(bytes)).first;
        }
        expected.insert(expected.end(), found->second.begin(), found->second.end());
      }
      const auto encoded = [&] {
        try { return cache->load_sound(std::stoul(datum, nullptr, 16), pitch, permutation); }
        catch(const std::exception& error) {
          throw halo2::FormatError("Native sound load failed map=" + map.string() + " sound=" + datum +
              " pitch=" + std::to_string(pitch) + " permutation=" + std::to_string(permutation) + ": " + error.what());
        }
      }();
      check(encoded.codec == codec && encoded.channels == channels &&
                encoded.sample_rate == rate && encoded.sound_class == sound_class &&
                encoded.samples == samples && encoded.data == expected,
            "Native encoded sound differs from independent original metadata/file bytes");
      checked_bytes += expected.size();
      ++cases;
      // Deduplicate only after the full native payload equals the actual file
      // bytes. PC sample usability does not establish Xbox decoder, scheduler,
      // listener, voice arbitration or live sound-event parity.
      if (decoded.emplace(fnv(expected), expected.size(), codec, channels, rate, sound_class).second) {
        try {
          const auto pcm = halo2::decode_sound(encoded);
          check(pcm.frames() > 0 && pcm.rate == 48000 && pcm.sound_class == sound_class,
                "Original payload did not decode to a usable PC sample");
          bool nonzero = false;
          for (float sample : pcm.stereo) {
            check(std::isfinite(sample), "Original payload decoded to a nonfinite PC sample");
            nonzero |= sample != 0;
          }
          silent_resources += !nonzero;
          decoded_frames += pcm.frames();
        } catch (const std::exception &e) {
          ++decode_failures;
          std::cerr << "PC decode failed map=" << path << " sound=" << datum
                    << " name=" << encoded.name << " pitch=" << pitch
                    << " permutation=" << permutation << ": " << e.what() << '\n';
        }
      }
    }
    check(cases > 0 && !maps.empty(), "No authored whole-map sound cases");
    std::cout << (mcc ? "runtimeMccMaps=" : "ownedMaps=") << maps.size()
              << (mcc ? " literalEncodedCases=" : " originalEncodedCases=") << cases
              << " exactEncodedBytes=" << checked_bytes << " originalChunkRanges=" << resources.size()
              << " uniqueDecodedResources=" << decoded.size() << " decodeFailures=" << decode_failures
              << " silentResources=" << silent_resources << " pcDecodedStereoFrames=" << decoded_frames
              << " schedulerSelection=unverified liveRouting=separate\n";
    return decode_failures ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
