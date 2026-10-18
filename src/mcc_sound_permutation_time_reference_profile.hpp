#pragma once
// Private fixture seam; normal callers use only the immutable checked policy.
#include "halo2/mcc_sound_permutation_time_reference.hpp"
namespace halo2::mcc_sound_permutation_time_detail {
struct Owner {
    TagHandle handle;
    std::uint32_t root{};
    std::string path;
    MccSoundVanillaDigest metadata;
};
struct Policy {
    std::span<const Owner> owners;
    // Sorted unique namedKey32 + actual encoded payload SHA32 + MCC U16 +
    // original U16, little endian. The domain is distinct from snd!+0x10.
    std::span<const std::byte> records;
};
MccSoundVanillaDigest named_key(std::string_view,std::string_view,std::string_view);
MccSoundPermutationTimePlan build(const MccSoundVanillaSource&,
    const MccSoundVanillaSource& fingerprint_source,
    const MccSoundRuntimePlan&, const Policy&);
// Explicit private native Cache class domain; never inferred by generic Source.
MccSoundPermutationTimePlan build_class_qualified(const MccSoundVanillaSource&,
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&,const Policy&);
MccSoundPermutationTimePlan plan_class_qualified(const MccSoundVanillaSource&,
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&);
// Checked in-memory staging only. Performs all provider reads and all guards
// before changing any byte in the owned, unpublished native plan.
void apply(MccSoundRuntimePlan&,const MccSoundRuntimeSource&,
    const MccSoundPermutationTimePlan&);
} // namespace halo2::mcc_sound_permutation_time_detail
