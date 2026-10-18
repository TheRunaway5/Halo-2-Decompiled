#pragma once
#include "halo2/tags/types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace halo2 {
// matg Sound Globals: MCC adds the remastered-class reference before the
// Legacy Sound Classes reference. The original row is exactly MCC bytes8..2b
// (0x24 bytes), including the unchanged full salted gestalt datum at +0x20.
// Cache/PCM ownership keeps the original 0x2c donor row separately.
std::array<std::byte,0x24> project_mcc_sound_globals(std::span<const std::byte>);
enum class MccSoundLayoutNeed {
    NativePermutationOwner, NativeChunkOwner, NativeExtraInfoOwner,
    NativeEncodedSampleExtent, ForeignRuntimeCacheHandle, NativeCodecDuration,
    // Retain the previous public enumerator's numeric value/ABI. The
    // authenticated original21F590 operands prove this extent is encoded
    // bytes (Xbox ADPCM uses64/36), rather than decoded PCM bytes/samples.
    NativeDecodedSampleExtent=NativeEncodedSampleExtent
};
class MccSoundLayoutRequirement final : public std::runtime_error {
public:
    MccSoundLayoutRequirement(MccSoundLayoutNeed,const char*);
    MccSoundLayoutNeed need() const {return need_;}
private:
    MccSoundLayoutNeed need_;
};
struct MccSoundCodecLayout {
    std::uint8_t sample_rate{},encoding{},compression{};
};
struct MccSoundDefinitionLayout {
    std::array<std::byte,0x14> definition;
    // Donor-only additions stay explicit; the Cache/PCM decoder continues
    // owning the complete original MCC tag and its codec/resource directory.
    std::array<std::byte,8> mcc_extension;
    MccSoundCodecLayout codec;
    std::uint8_t codec_index{};
    std::uint16_t remastered_sound_index{};
    // Describes original hardware admission, not native PCM capability or
    // device activation. No codec value or readiness flag is forged.
    bool original_adpcm_admission() const;
    // 21F590 indexes original rate/encoding/compression operand arrays. MCC
    // extensions require the real PCM backend's format/duration calculation.
    void require_original_duration_domain() const;
};
// Exact snd! field-layout projection: retain authored low10 flags, remove
// MCC route bits10/11, and move the four verified compiled predicates12..15
// to original10..13. This does not restore separate authored flag changes.
// codecs is the owning ugh!'s complete three-byte codec bank, not an
// arbitrary same-named shared gestalt. Cache retains the raw MCC header.
MccSoundDefinitionLayout project_mcc_sound_definition(
    std::span<const std::byte> sound,std::span<const std::byte> codecs);
// sncl's first 0x5c bytes are identical named authored fields; MCC's 0x78
// stride must be compacted explicitly. No floating-point arithmetic occurs.
std::vector<std::byte> project_mcc_sound_class_records(std::span<const std::byte>);
std::array<std::byte,8> project_mcc_sound_class_root(
    std::span<const std::byte>,std::uint32_t native_record_address);

struct MccSoundResourceBlocks {
    std::optional<TagBlock> permutations,chunks,extra_info;
};
// Build the original 0x58 root only after the changed resource tables have
// real declared native owners. source_language_chunks is the selected MCC
// language's eight-byte chunk descriptor. Common descriptor pointer values
// remain serialized source addresses for the delivery planner to relocate.
// This is metadata conversion only; it does not activate hardware/streaming.
std::array<std::byte,0x58> project_mcc_sound_gestalt_root(
    std::span<const std::byte> gestalt,
    std::span<const std::byte> source_language_chunks,
    const MccSoundResourceBlocks&);
// The original SampleSize is encoded bytes, consumed with codec-specific
// original conversion operands. MCC localized info instead stores decoded
// PCM/container bytes. Supply the checked sum of the selected chunks' actual
// encoded extents; never copy the localized PCM size or coerce a codec.
// The original eight authored permutation bytes stay exact.
std::array<std::byte,16> project_mcc_sound_permutation(
    std::span<const std::byte> permutation,
    std::span<const std::byte> localized_info,
    std::optional<std::uint32_t> native_encoded_extent);
struct MccSoundChunkLayout {
    std::array<std::byte,12> chunk;
    std::uint32_t decoded_bytes{};
    bool reset_after{};
};
// Resource file offsets remain truthful MCC DAT offsets. They require the
// retained MCC resource owner, not an Xbox map async-read provider. Opus bit29
// is retained separately and cannot become part of original lower30 size.
MccSoundChunkLayout project_mcc_sound_chunk(std::span<const std::byte>);
} // namespace halo2
