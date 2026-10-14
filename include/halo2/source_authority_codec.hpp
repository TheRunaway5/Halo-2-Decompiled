#pragma once
#include "halo2/source_match_authority.hpp"

namespace halo2 {
struct SourceAuthorityCodecLimits {
    std::uint64_t encoded_bytes=512ull*1024*1024;
    std::uint64_t decoded_bytes=1024ull*1024*1024;
    // Decoded dynamic values; decompression and fixed/default owner storage
    // have separate decoded-size/compile-time bounds, rather than an RSS cap.
    std::uint64_t allocated_bytes=1024ull*1024*1024;
    std::uint64_t container_elements=1'000'000;
};
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
}
