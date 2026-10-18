#pragma once
#include "halo2/mcc_sound_tag_layout.hpp"
#include <functional>
#include <memory>

namespace halo2 {
class Cache;
struct MccSoundVanillaFlagPlan;
// Source tokens and full salted tag identities only. Readers must return the
// exact requested raw MCC bytes; no pointer in this interface is a host address.
struct MccSoundRuntimeSource {
    std::span<const Tag> tags;
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
    std::function<TagHandle(TagHandle)> gestalt_owner;
    // Unchanged nested CustomPlayback/Promotion/EnglishExtraInfo structures
    // use existing fully relocated metadata allocations (identical schemas).
    std::function<std::uint32_t(std::uint32_t,std::size_t)> relocate;
};
struct MccSoundRuntimePatch {
    TagHandle owner;
    std::uint32_t source_address{};
    std::vector<std::byte> before,after;
};
enum class MccSoundDataFile { Neutral,English,Remastered };
struct MccSoundRuntimeChunk {
    TagHandle gestalt;
    std::uint32_t source_address{},native_address{},source_index{};
    std::uint32_t file_offset{},encoded_bytes{},decoded_bytes{};
    bool reset_after{};
};
struct MccSoundRuntimeDefinition {
    TagHandle sound,gestalt;
    MccSoundCodecLayout codec;
    MccSoundDataFile file{};
    std::uint16_t remastered_index{};
    std::array<std::byte,8> mcc_extension;
    std::uint32_t source_root{},native_root{};
    // Complete original MCC header flags, including its DAT/playback-route
    // bits. Native consumers receive the separately normalized TU1.5 word.
    std::uint16_t source_flags{},native_flags{};
};
struct MccSoundRuntimeGestalt {
    TagHandle owner;
    // Ordered original root descriptors; the bases are row ordinals in each
    // merged bank. Runtime permutation flags use a byte base, not bit base.
    std::array<std::uint32_t,11> first{},count{};
};
struct MccSoundRuntimeFlagUser {
    TagHandle sound,mcc_class,native_class;
    std::uint8_t class_index{};
    std::uint16_t mcc_flags{},native_flags{};
};
struct MccSoundRuntimeUnusedFlag {
    TagHandle gestalt;
    std::uint32_t range_index{};
    std::uint16_t encoded_index{};
    // Both the actual MCC-selected and projected Legacy native class have
    // bit5 set for EVERY user. Original21FF20/21FFB6 therefore skip the
    // encoded field completely. It is preserved, not given fake backing.
    std::vector<MccSoundRuntimeFlagUser> users;
};
struct MccSoundRuntimePlan {
    std::uint32_t address{};
    std::vector<std::byte> bytes;
    std::array<TagBlock,11> blocks;
    std::vector<MccSoundRuntimePatch> patches;
    std::vector<MccSoundRuntimeGestalt> gestalts;
    std::vector<MccSoundRuntimeChunk> chunks;
    std::vector<MccSoundRuntimeDefinition> sounds;
    std::vector<MccSoundRuntimeUnusedFlag> unused_flag_indices;
    // Metadata is complete, but original21F590 cannot index its fixed arrays
    // for these codecs. Real PCM duration/streaming remains a separate owner.
    std::vector<TagHandle> native_pcm_duration;
    // Cache-aware finite authored corrections retain complete per-owner
    // authentication/custom/unmatched status. Pure Source loading remains
    // generic and leaves this empty; raw Cache/source_flags are unchanged.
    std::shared_ptr<const MccSoundVanillaFlagPlan> authored_flags;
};
// Pure, staged conversion. No Cache/raw DAT bytes or live runtime bank change.
// Caller atomically validates all before bytes/target extents, appends bytes,
// then commits patches. Publish neither roots nor partial banks on failure.
// This does not create voices, allocate hardware resources, or make serialized
// NONE chunk handles ready. DAT routes remain explicit. The Cache overload
// authenticates encoded resources and restores qualified TU1.5 stored times;
// the generic Source overload retains its original metadata-only contract.
MccSoundRuntimePlan plan_mcc_sound_runtime_delivery(
    const MccSoundRuntimeSource&,std::uint32_t native_bank_address);
MccSoundRuntimePlan plan_mcc_sound_runtime_delivery(
    const Cache&,std::uint32_t native_bank_address,
    const std::function<std::uint32_t(std::uint32_t,std::size_t)>& relocate);
} // namespace halo2
