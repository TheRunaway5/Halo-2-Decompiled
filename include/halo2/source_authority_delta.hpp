#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace halo2 {
using AuthorityArchiveDigest=std::array<std::byte,32>;
struct AuthorityDeltaLimits {
    std::uint64_t encoded_bytes=64ull*1024*1024;
    std::uint64_t decoded_bytes=1024ull*1024*1024;
    std::uint64_t patch_bytes=1024ull*1024*1024;
    std::uint64_t commands=1'000'000;
};
// Immutable canonical named-field archive bytes, before compression. This
// owner validates the source-codec envelope/checksum, not the state schema or
// the world. The executor still decodes and prepares every reconstructed cut.
// These hashes detect corruption/base mismatch; authenticate and bind the
// outer payload to the room/epoch/membership through the peer transfer owner.
class AuthorityArchiveImage {
public:
    static AuthorityArchiveImage read(std::span<const std::byte>,AuthorityDeltaLimits = {});
    AuthorityArchiveDigest digest()const;
    std::uint64_t decoded_size()const;
    std::vector<std::byte> encode(AuthorityDeltaLimits = {})const;
private:
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
