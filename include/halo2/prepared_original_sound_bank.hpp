#pragma once
#include "halo2/content.hpp"
#include <array>
#include <filesystem>
#include <memory>
#include <string_view>

namespace halo2 {
// A prepared native data asset, not a live tag/voice or an Xbox cache. All
// literal ordinals below remain in the original map's index space. A runtime
// binder must allocate/rebase real owners before publishing these records.
class PreparedOriginalSoundBank final {
public:
    struct Chunk {
        std::uint32_t offset{},bytes{},sector_bytes{},source_flags{};
    };
    struct Permutation {
        std::string name;
        std::array<std::byte,16> literal;
        std::vector<Chunk> chunks;
    };
    struct Metadata {
        std::string name,pitch_name;
        std::array<std::byte,20> sound,scale;
        std::array<std::byte,56> playback;
        std::array<std::byte,92> sound_class;
        std::array<std::byte,12> range;
        std::array<std::byte,10> interval;
        // V1 omitted this owner. Empty means unavailable, not all unused.
        // V2 retains the exact original initial permutation-state bytes.
        std::vector<std::byte> used;
        std::vector<Permutation> permutations;
    };
    // The expected manifest digest comes from the admitting prepared-asset
    // owner. Never derive it from the same untrusted manifest being loaded.
    static std::shared_ptr<const PreparedOriginalSoundBank> load(
        const std::filesystem::path& directory,std::string_view expected_manifest_sha256);
    const Metadata& metadata() const {return metadata_;}
    const EncodedSound& encoded(unsigned pitch,unsigned permutation) const;
    std::span<const std::byte> sectors() const {return sectors_;}
private:
    Metadata metadata_;
    std::vector<std::byte> sectors_;
    std::vector<EncodedSound> encoded_;
};
}
