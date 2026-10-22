#pragma once
// Implementation/fixture seam only. Normal loading always uses the immutable
// generated reference catalog; callers cannot supply it through Cache APIs.
#include "halo2/mcc_sound_vanilla_reference.hpp"
namespace halo2::mcc_sound_reference_detail {
struct Permutation {
    unsigned range{},permutation{},global_range{},global_permutation{};
    std::uint16_t range_import{},permutation_import{};
    std::uint32_t range_sid{},permutation_sid{};
    std::string range_name,permutation_name;
    MccSoundVanillaPayload payload;
};
struct Sound {
    TagHandle handle;
    std::uint32_t root{};
    std::string path;
    std::array<std::byte,28> header;
    std::uint16_t before{},after{};
    std::vector<Permutation> permutations;
};
struct Owner {
    TagHandle handle;
    std::uint32_t root{};
    std::string path;
    MccSoundVanillaDigest metadata;
    std::vector<Sound> sounds;
};
MccSoundVanillaDigest metadata_digest(const MccSoundVanillaSource&,TagHandle);
// Authenticate the moved matg sound row against its retained raw allocation.
// Only the directory identity used for source fingerprinting is replaced;
// no active or retained metadata bytes are changed.
std::vector<Tag> raw_globals_namespace(const MccSoundVanillaSource&,std::span<const Tag> retained);
MccSoundVanillaFlagPlan plan(const MccSoundVanillaSource&,std::span<const Owner>);
} // namespace halo2::mcc_sound_reference_detail
