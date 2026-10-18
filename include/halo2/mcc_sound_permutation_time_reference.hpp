#pragma once
#include "halo2/mcc_sound_vanilla_reference.hpp"

namespace halo2 {
enum class MccSoundPermutationTimeStatus {
    Authenticated, AlreadyEquivalent, NoOriginalReference, UnknownOwner,
    CustomOwner, EncodedResourceMismatch, UnsafeIncomingReference,
    InvalidNativeProjection
};
struct MccSoundPermutationTimeUser {
    TagHandle sound;
    std::uint32_t source_root{}, native_root{};
    std::uint32_t range{}, permutation{}, source_range{}, source_permutation{};
    std::array<std::byte,28> source_header;
    std::array<std::byte,20> native_header;
};
struct MccSoundPermutationTimeResult {
    TagHandle sound, gestalt;
    std::uint32_t range{}, permutation{}, source_permutation{};
    MccSoundPermutationTimeStatus status{MccSoundPermutationTimeStatus::NoOriginalReference};
    std::string reason;
};
struct MccSoundPermutationTimeCorrection {
    TagHandle gestalt;
    std::uint32_t source_address{}, native_address{}, source_index{}, native_index{};
    std::uint16_t source_ms{}, original_ms{};
    // Complete source and canonical native row guards. Only native +6..7
    // changes; the source row, ordinals, encoded extent and PCM stay intact.
    std::array<std::byte,16> source_before, native_before;
    std::array<std::byte,2> before, after;
    std::vector<MccSoundPermutationTimeUser> users;
};
class MccSoundPermutationTimePlan final {
public:
    std::span<const MccSoundPermutationTimeCorrection> corrections() const noexcept;
    std::span<const MccSoundPermutationTimeResult> permutations() const noexcept;
    std::span<const MccSoundVanillaOwnerResult> owners() const noexcept;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundPermutationTimePlan(std::shared_ptr<const Impl>);
    friend struct MccSoundPermutationTimeAccess;
};
// Pure finite TU1.5 authored U16 milliseconds for original 21F670. Exact
// complete raw owners, every known sound's actual encoded resources, named
// identities and every incoming user qualify each native row. No PCM-derived
// value, name-only match, ordinal rewrite, metadata write or voice admission.
// Native input must be the complete generic/vanilla-flag base projection or
// its complete authenticated permutation-time result; partial sets reject.
MccSoundPermutationTimePlan plan_mcc_sound_permutation_times(
    const MccSoundVanillaSource&, const MccSoundRuntimePlan&);
MccSoundPermutationTimePlan plan_mcc_sound_permutation_times(
    const Cache&, const MccSoundRuntimePlan&,
    const std::function<std::uint32_t(std::uint32_t,std::size_t)>& relocate);
} // namespace halo2
