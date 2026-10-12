#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
struct EncodedSound;
struct PcmSound;
struct VistaAudioPacket {
  std::size_t offset{}; // absolute offset in the supplied serialized chunk
  std::span<const std::byte> bytes;
};
struct VistaSerializedMedia {
  std::array<std::byte,16> major_type{},subtype{},format_type{};
  std::uint32_t fixed_size_samples{},temporal_compression{},sample_size{};
  std::uint16_t format_tag{},channels{},block_align{},bits_per_sample{};
  std::uint32_t sample_rate{},average_bytes_per_second{};
  std::span<const std::byte> format,extra,payload;
  std::vector<VistaAudioPacket> packets;
};

// Exact serialized audio/WaveFormatEx GUID identity, never ASF sniffing. It
// intentionally recognizes malformed lengths too, so those fail in the parser
// instead of falling through to the Xbox container decoder.
bool is_vista_serialized_audio(std::span<const std::byte>);

// Source 4F7404 structural validation and 4F7355 packet boundaries. Spans borrow
// input; caller retains it. GUID/codec/channel acceptance belongs to the decoder.
// Zero packet size with unread payload rejects explicitly: the original stalls.
// Input larger than a source32-bit chunk rejects instead of narrowing.
VistaSerializedMedia parse_vista_serialized_media(std::span<const std::byte>);

// Bounded native FFmpeg decoder for the observed WMA2 serialized-audio family.
// Metadata must agree with the chunk, and source packet framing is preserved.
// Produces native48k stereo PCM; this is not a DMO PCM/mixer-equivalence claim.
PcmSound decode_vista_serialized_sound(const EncodedSound&);
} // namespace halo2
