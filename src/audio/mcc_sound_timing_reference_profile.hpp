#pragma once
// Fixture/implementation seam; normal loading uses the immutable catalog.
#include "halo2/mcc_sound_timing_reference.hpp"
namespace halo2::mcc_sound_permutation_time_detail {struct Policy;}
namespace halo2::mcc_sound_timing_detail {
struct TimingOwner {
    TagHandle handle;
    std::uint32_t root{};
    std::string path;
    MccSoundVanillaDigest metadata;
};
struct Policy {
    std::span<const TimingOwner> owners;
    // Sorted, unique 81-byte records, explicitly little endian.
    std::span<const std::byte> records;
    std::span<const MccSoundVanillaDigest> unavailable_pitch;
};
MccSoundVanillaDigest named_key(std::string_view,std::string_view,std::string_view);
MccSoundTimingPlan build(const MccSoundVanillaSource&,
    const MccSoundVanillaSource& fingerprint_source,
    const MccSoundRuntimePlan&,const Policy&);
// Synthetic exact finite permutation policy only; normal loading reuses the
// same immutable canonical proof already required by expected_projection.
MccSoundTimingPlan build_with_permutation_times(const MccSoundVanillaSource&,
    const MccSoundVanillaSource&,const MccSoundRuntimePlan&,const Policy&,
    const mcc_sound_permutation_time_detail::Policy&);
// Actual raw Source/class-qualified cohort. Uses the same finite production
// policies and expected-projection implementation as Cache loading, while
// retaining the generic Source stored-time contract and diagnostics.
MccSoundTimingPlan build_class_qualified(const MccSoundVanillaSource&,
    const MccSoundRuntimePlan&);
} // namespace halo2::mcc_sound_timing_detail
