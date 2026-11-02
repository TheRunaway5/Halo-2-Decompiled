#pragma once
#include "halo2/content.hpp"
#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace halo2 {
// A prepared native data asset, not a live tag/voice or an Xbox cache. All
// literal ordinals below remain in the original map's index space. A runtime
// binder must allocate/rebase real owners before publishing these records.
class PreparedOriginalSoundBank final {
public:
    struct Chunk {
        std::uint32_t offset{},bytes{},sector_bytes{},source_flags{};
        std::optional<std::array<std::byte,12>> literal;
    };
    struct Permutation {
        std::string name;
        std::array<std::byte,16> literal;
        std::optional<std::uint32_t> import_name_id;
        std::vector<Chunk> chunks;
    };
    struct PitchRange {
        std::string name;
        std::optional<std::uint32_t> import_name_id;
        std::array<std::byte,12> literal;
        std::array<std::byte,10> interval;
        // Unavailable storage is distinct from an authored all-unused bank.
        std::optional<std::vector<std::byte>> used;
        std::vector<Permutation> permutations;
    };
    struct Promotion {
        // Original pointers and mutable initial fields remain literal here.
        // The runtime binder must rebase both nested block owners.
        std::array<std::byte,28> literal;
        std::vector<std::array<std::byte,16>> rules;
        std::vector<std::array<std::byte,4>> counters;
    };
    struct Origin {
        std::string map,map_sha256;
        TagHandle sound,gestalt,classes;
        friend bool operator==(const Origin&,const Origin&)=default;
    };
    struct Metadata {
        std::string name;
        std::array<std::byte,20> sound,scale;
        std::array<std::byte,56> playback;
        std::array<std::byte,92> sound_class;
        std::vector<PitchRange> pitch_ranges;
        std::optional<Promotion> promotion;
        // Native table construction needs the full original namespace and
        // complete class owner. Older offline V3 banks may omit these; they
        // remain readable but cannot supply a runtime binding.
        std::optional<Origin> origin;
        std::optional<std::array<std::array<std::byte,92>,54>> sound_classes;
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
    std::vector<std::vector<EncodedSound>> encoded_;
};
}
