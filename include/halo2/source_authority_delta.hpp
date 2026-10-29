#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include "halo2/authority_network_limits.hpp"
#include "halo2/source_shadow_archive_limits.hpp"

namespace halo2 {
using AuthorityArchiveDigest=std::array<std::byte,32>;
struct AuthorityDeltaLimits {
    std::uint64_t encoded_bytes=64ull*1024*1024;
    std::uint64_t decoded_bytes=authority_network_decoded_bytes;
    std::uint64_t patch_bytes=authority_network_decoded_bytes;
    std::uint64_t commands=1'000'000;
    std::uint64_t working_bytes=4*authority_network_decoded_bytes;
};
inline AuthorityDeltaLimits source_authority_network_delta_limits(){
    return {authority_network_encoded_bytes,authority_network_decoded_bytes,authority_network_decoded_bytes,
        1'000'000,4*authority_network_decoded_bytes};
}
inline AuthorityDeltaLimits source_shadow_archive_delta_limits(){
    return {source_shadow_archive_encoded_bytes,source_shadow_archive_decoded_bytes,source_shadow_archive_decoded_bytes,
        1'000'000,source_shadow_archive_delta_bytes};
}
// Immutable canonical named-field archive bytes, before compression. This
// owner validates the source-codec envelope/checksum, not the state schema or
// the world. The executor still decodes and prepares every reconstructed cut.
// These hashes detect corruption/base mismatch; authenticate and bind the
// outer payload to the room/epoch/membership through the peer transfer owner.
class AuthorityArchiveImage {
public:
    static AuthorityArchiveImage read(std::span<const std::byte>,AuthorityDeltaLimits = {});
    // Seal a complete canonical byte bank without a compression round trip.
    // Copy even a moved public input so prior mutable aliases cannot change
    // the checksum-verified image. Charge both input and sealed byte banks.
    // Schema/native admission is still required, exactly as for read().
    static AuthorityArchiveImage from_raw(std::vector<std::byte>,AuthorityDeltaLimits = {});
    AuthorityArchiveDigest digest()const;
    std::uint64_t decoded_size()const;
    std::span<const std::byte> raw()const;
    std::vector<std::byte> encode(AuthorityDeltaLimits = {})const;
private:
    friend struct SourceAuthorityArchiveBuilderAccess;
    // Only the codec's fresh private Writer may transfer an unaliased buffer.
    static AuthorityArchiveImage from_unaliased_raw(std::vector<std::byte>,AuthorityDeltaLimits);
    struct Data;
    explicit AuthorityArchiveImage(std::shared_ptr<const Data>);
    std::shared_ptr<const Data> data_;
    friend std::vector<std::byte> encode_authority_delta(const AuthorityArchiveImage &,
        const AuthorityArchiveImage &,AuthorityDeltaLimits);
    friend AuthorityArchiveImage apply_authority_delta(const AuthorityArchiveImage &,
        std::span<const std::byte>,AuthorityDeltaLimits);
};
// Copy/literal instructions refer only to the exact retained base image. They
// preserve every byte, including unknown source regions, rather than selecting
// gameplay fields or replaying an incomplete set of inputs. Images are shared
// immutable values; caller owns cache lifetime and agreed cut/epoch selection.
std::vector<std::byte> encode_authority_delta(const AuthorityArchiveImage &base,
    const AuthorityArchiveImage &next,AuthorityDeltaLimits = {});
AuthorityArchiveImage apply_authority_delta(const AuthorityArchiveImage &base,
    std::span<const std::byte>,AuthorityDeltaLimits = {});
}
