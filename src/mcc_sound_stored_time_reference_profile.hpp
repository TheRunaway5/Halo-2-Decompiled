#pragma once
// Private fixture seam. Normal loading uses the immutable generated policy.
#include "halo2/mcc_sound_stored_time_reference.hpp"
namespace halo2::mcc_sound_stored_time_detail {
struct Owner {
    TagHandle handle;
    std::uint32_t root{};
    std::string path;
    MccSoundVanillaDigest metadata;
};
struct Policy {
    std::span<const Owner> owners;
    // Sorted unique little-endian records: pathKey32 + signed milliseconds4;
    // and existing timing namedKey32 + actual MCC encoded-payload SHA32.
    std::span<const std::byte> values,payloads;
};
MccSoundVanillaDigest path_key(std::string_view);
MccSoundStoredTimePlan build(const MccSoundVanillaSource&,
    const MccSoundVanillaSource& fingerprint_source,
    const MccSoundRuntimePlan&,const Policy&);
MccSoundStoredTimePlan build_class_qualified(const MccSoundVanillaSource&,
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&,const Policy&);
MccSoundStoredTimePlan plan_class_qualified(const MccSoundVanillaSource&,
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&);
} // namespace halo2::mcc_sound_stored_time_detail
