#pragma once
#include "halo2/source_match_authority.hpp"
#include "halo2/authority_network_limits.hpp"

namespace halo2 {
struct SourceAuthorityCodecLimits {
    std::uint64_t encoded_bytes=512ull*1024*1024;
    std::uint64_t decoded_bytes=1024ull*1024*1024;
    // Decoded dynamic values; decompression and fixed/default owner storage
    // have separate decoded-size/compile-time bounds, rather than an RSS cap.
    std::uint64_t allocated_bytes=1024ull*1024*1024;
    std::uint64_t container_elements=1'000'000;
    std::uint64_t working_bytes{}; // Zero preserves the offline additive budget.
    std::uint64_t staged_bytes{}; // Zero preserves offline native-owner policy.
};
struct SourceAuthorityCodecStats {
    std::uint64_t raw_bytes{},metadata_bytes{},native_claimed{},working_peak{},staged_claimed{};
};
inline SourceAuthorityCodecLimits source_authority_network_codec_limits(){
    return {authority_network_encoded_bytes,authority_network_decoded_bytes,authority_network_native_bytes,
        1'000'000,authority_network_parser_bytes,authority_network_staged_bytes};
}
// Owning, named-field, little-endian native format; never an Xbox save record
// or a memcpy of C++ layout. Source32 addresses remain data tokens. Process
// pointers/borrowed spans and unsupported owners are rejected, never omitted.
// The envelope checksum detects corruption, not peer authority. The session
// must authenticate and generation/membership-bind a payload before decoding,
// then use SourceMatchAuthority preparation to validate the complete world.
std::vector<std::byte> encode_source_authority(const SourceMatchAuthority::Checkpoint &,
    SourceAuthorityCodecLimits = {});
SourceMatchAuthority::Checkpoint decode_source_authority(std::span<const std::byte>,
    SourceAuthorityCodecLimits = {});
SourceMatchAuthority::Checkpoint decode_source_authority_raw(std::span<const std::byte>,
    std::array<std::byte,32>,SourceAuthorityCodecLimits,SourceAuthorityCodecStats *stats=nullptr);
}
