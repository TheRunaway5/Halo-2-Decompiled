#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace halo2 {
class AuthorityArchiveImage;
struct AuthorityArchiveDecodeLimits {
    std::uint64_t encoded_bytes{},decoded_bytes{},allocated_bytes{},container_elements{},fixed_native_bytes{},working_bytes{};
};
struct AuthorityArchiveValue {
    std::uint8_t tag{};std::uint64_t bits{},count{};std::size_t next{};
};
struct AuthorityArchiveParseStats {
    std::uint64_t raw_bytes{};std::size_t nodes{};std::uint64_t metadata_bytes{};std::size_t deepest{};
    std::uint64_t working_limit{},working_reserved{},working_peak{},native_claimed{};
};
// Owns the immutable Rust value tree and either decompressed Rust backing or
// a shared immutable AuthorityArchiveImage. Rust indexes never retain a slice.
// Borrowed spans
// below stay inside the immediate schema materializer/hash call and expire
// with this owner. They are not inputs to a second byte decoder. Claims are
// synchronized in Rust; parsing/materialization never invokes native callbacks.
class ParsedAuthorityArchive {
public:
    ~ParsedAuthorityArchive();
    AuthorityArchiveValue node(std::size_t)const;
    std::span<const std::byte> name(std::size_t)const;
    std::span<const std::byte> data(std::size_t)const;
    std::span<const std::byte> raw()const;
    std::array<std::byte,32> expected_hash()const;
    std::size_t size()const;
    bool claim(std::uint64_t count,std::uint64_t extent,bool elements)const;
    AuthorityArchiveParseStats stats()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit ParsedAuthorityArchive(std::unique_ptr<Impl>);
    friend std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive(
        std::span<const std::byte>,AuthorityArchiveDecodeLimits);
    friend std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_raw(
        std::span<const std::byte>,std::array<std::byte,32>,AuthorityArchiveDecodeLimits);
    friend std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_image(
        const AuthorityArchiveImage&,AuthorityArchiveDecodeLimits);
};
// Failure is terminal for this archive. Successful syntax parsing grants no
// native schema, checksum, sender, world-import or callback authority.
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive(
    std::span<const std::byte>,AuthorityArchiveDecodeLimits);
// Expanded delta images still require the Rust grammar and allocation policy.
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_raw(
    std::span<const std::byte>,std::array<std::byte,32>,AuthorityArchiveDecodeLimits);
// The closed image already owns checksum-verified immutable canonical bytes.
// Retain that exact owner while Rust checks all grammar/ranges and budgets,
// without a second byte bank. This still grants no schema/native authority.
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_image(
    const AuthorityArchiveImage&,AuthorityArchiveDecodeLimits);
}
