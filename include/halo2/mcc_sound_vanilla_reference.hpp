#pragma once
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include <string>

namespace halo2 {
using MccSoundVanillaDigest=std::array<std::byte,32>;
struct MccSoundVanillaPacket {
    std::uint32_t bytes{},pcm_bytes{};
    bool reset_after{};
    auto operator<=>(const MccSoundVanillaPacket&) const = default;
};
struct MccSoundVanillaPayload {
    MccSoundVanillaDigest sha256{};
    std::uint64_t encoded_bytes{};
    std::uint32_t codec{},sample_rate{},channels{},samples{},sound_class{};
    std::vector<MccSoundVanillaPacket> packets;
    auto operator<=>(const MccSoundVanillaPayload&) const = default;
};
struct MccSoundVanillaSource {
    MccSoundRuntimeSource metadata;
    std::function<std::string(std::uint32_t)> string_id;
    // Hash the ACTUAL Cache-loaded encoded bytes and preserve packet/format
    // identity. This is neither a readiness callback nor a PCM substitute.
    std::function<MccSoundVanillaPayload(TagHandle,unsigned,unsigned)> payload;
};
enum class MccSoundVanillaOwnerStatus { Unmatched,Custom,Authenticated };
struct MccSoundVanillaOwnerResult {
    TagHandle gestalt;
    MccSoundVanillaOwnerStatus status{};
    std::string reason;
};
struct MccSoundVanillaFlagCorrection {
    TagHandle sound,gestalt;
    std::uint32_t source_root{};
    std::uint16_t source_flags{},native_before{},native_after{};
};
struct MccSoundVanillaFlagPlan {
    std::vector<MccSoundVanillaFlagCorrection> corrections;
    std::vector<MccSoundVanillaOwnerResult> owners;
};
// Pure staged corrections: only exact finite authenticated retail owners
// qualify. No donor/reference files are opened and no Cache bytes change.
// Modified/unknown owners remain explicit and receive no partial correction.
MccSoundVanillaFlagPlan plan_mcc_sound_vanilla_flags(const MccSoundVanillaSource&);
MccSoundVanillaFlagPlan plan_mcc_sound_vanilla_flags(const Cache&);
} // namespace halo2
