#pragma once
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include <string_view>

namespace halo2 {
enum class MccSoundClassReferenceStatus {
    UnrecognizedSource, CandidateMismatch, Converted, AlreadyEquivalent
};
enum class MccSoundClassReferenceWordKind { Float32, TwoUnsigned16, TwoFlags16 };
struct MccSoundClassReferenceWord {
    std::uint8_t class_index{},offset{};
    MccSoundClassReferenceWordKind kind{};
    std::uint32_t before_bits{},after_bits{};
    std::string_view class_name,field_name;
};
struct MccSoundClassReferenceProjection {
    MccSoundClassReferenceStatus status{};
    TagHandle owner;
    std::string source_bank_sha256;
    std::vector<std::byte> native_records;
    std::array<MccSoundClassReferenceWord,48> words{};
    std::uint8_t changed_words{},restored_named_rows{};
    // These repurposed slots have no proven corresponding original named
    // class. Their COMPLETE MCC rows remain untouched, including native fields.
    std::array<std::uint8_t,6> unresolved_classes{15,16,17,39,40,41};
};
// Pure finite authoring projection. Authenticate the complete actual matg ->
// singular44-byte SoundGlobals Legacy sncl route, its full salted directory
// identity and complete54x120-byte source bank. Candidate must be the WHOLE
// truthful54x92-byte layout preimage or this profile's WHOLE postimage.
// Only48 proved words in25 original-named rows change in the owned result.
// Neither callback/source data nor candidate changes, even on failure. Unknown,
// custom and partial candidates have explicit status and an empty result bank.
// Tag identities/strings and candidate are owned before copying callables;
// all are retained before reads. Namespace/candidate containers may reset
// from a callable copy constructor; its wrapper may reset during a read.
// The wrapper must remain valid through its own copy. Callbacks must expose
// stable immutable raw MCC views for the entire call.
MccSoundClassReferenceProjection project_mcc_sound_class_reference(
    const MccSoundRuntimeSource&,TagHandle legacy_owner,
    std::span<const std::byte> candidate_native_records);
} // namespace halo2
