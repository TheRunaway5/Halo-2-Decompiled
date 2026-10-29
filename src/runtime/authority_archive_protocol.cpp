#include "halo2/authority_archive_protocol.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <optional>
#include <stdexcept>

namespace halo2 {
namespace {
std::span<const std::byte> bytes(rust::Slice<const std::uint8_t> value) {
    return {reinterpret_cast<const std::byte*>(value.data()),value.size()};
}
std::span<const std::byte> indexed_bytes(std::span<const std::byte> raw,
    const online_protocol_bridge::ArchiveRange &range) {
    if(!range.valid)return {};
    if(range.offset>raw.size() || range.bytes>raw.size()-range.offset)
        throw std::runtime_error("Authority archive index escaped its retained immutable image");
    return raw.subspan(range.offset,range.bytes);
}
}
struct ParsedAuthorityArchive::Impl {
    std::optional<rust::Box<online_protocol_bridge::Archive>> value;
    std::optional<AuthorityArchiveImage> image;
    std::optional<rust::Box<online_protocol_bridge::ArchiveIndex>> index;
    explicit Impl(rust::Box<online_protocol_bridge::Archive> v):value(std::move(v)){}
    Impl(AuthorityArchiveImage owned,rust::Box<online_protocol_bridge::ArchiveIndex> nodes)
        :image(std::move(owned)),index(std::move(nodes)){
        // index_raw also reserves the two small C++ owners and conservative
        // allocator metadata, separately from its retained raw/index bytes.
        static_assert(sizeof(Impl)+sizeof(ParsedAuthorityArchive)+64<=128);
    }
};
ParsedAuthorityArchive::ParsedAuthorityArchive(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
ParsedAuthorityArchive::~ParsedAuthorityArchive()=default;
AuthorityArchiveValue ParsedAuthorityArchive::node(std::size_t index)const {
    const auto n=impl_->index?(*impl_->index)->archive_index_node(index):(*impl_->value)->archive_node(index);
    if(!n.valid)throw std::runtime_error("Authority archive node is out of range");
    return {n.tag,n.bits,n.count,n.next};
}
std::span<const std::byte> ParsedAuthorityArchive::name(std::size_t index)const {
    return impl_->index?indexed_bytes(impl_->image->raw(),(*impl_->index)->archive_index_name(index)):
        bytes((*impl_->value)->archive_name(index));
}
std::span<const std::byte> ParsedAuthorityArchive::data(std::size_t index)const {
    return impl_->index?indexed_bytes(impl_->image->raw(),(*impl_->index)->archive_index_data(index)):
        bytes((*impl_->value)->archive_data(index));
}
std::span<const std::byte> ParsedAuthorityArchive::raw()const {
    return impl_->image?impl_->image->raw():bytes((*impl_->value)->archive_raw());
}
std::array<std::byte,32> ParsedAuthorityArchive::expected_hash()const {
    std::array<std::byte,32> result{};const auto source=impl_->index?(*impl_->index)->archive_index_hash():(*impl_->value)->archive_hash();
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte(source[i]);return result;
}
std::size_t ParsedAuthorityArchive::size()const {return impl_->index?(*impl_->index)->archive_index_len():(*impl_->value)->archive_len();}
bool ParsedAuthorityArchive::claim(std::uint64_t n,std::uint64_t extent,bool elements)const {
    return impl_->index?(*impl_->index)->archive_index_claim(n,extent,elements):(*impl_->value)->archive_claim(n,extent,elements);
}
AuthorityArchiveParseStats ParsedAuthorityArchive::stats()const {
    const auto s=impl_->index?(*impl_->index)->archive_index_stats():(*impl_->value)->archive_stats();
    if(!s.valid)throw std::runtime_error("Authority archive budget is unavailable");
    return {s.raw_bytes,s.nodes,s.metadata_bytes,s.deepest,s.working_limit,s.working_reserved,s.working_peak,s.native_claimed};
}
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive(std::span<const std::byte> packet,AuthorityArchiveDecodeLimits l) {
    try {
        auto value=online_protocol_bridge::parse_authority_archive(
            rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(packet.data()),packet.size()),
            {l.encoded_bytes,l.decoded_bytes,l.allocated_bytes,l.container_elements,l.fixed_native_bytes,l.working_bytes});
        auto impl=std::make_unique<ParsedAuthorityArchive::Impl>(std::move(value));
        return std::unique_ptr<ParsedAuthorityArchive>(new ParsedAuthorityArchive(std::move(impl)));
    }catch(const rust::Error &){return {};}
}
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_raw(std::span<const std::byte> raw,
    std::array<std::byte,32> hash,AuthorityArchiveDecodeLimits l) {
    try {
        std::array<std::uint8_t,32> digest{};for(unsigned i=0;i<32;++i)digest[i]=std::to_integer<std::uint8_t>(hash[i]);
        auto value=online_protocol_bridge::parse_authority_archive_raw(
            rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(raw.data()),raw.size()),digest,
            {l.encoded_bytes,l.decoded_bytes,l.allocated_bytes,l.container_elements,l.fixed_native_bytes,l.working_bytes});
        auto impl=std::make_unique<ParsedAuthorityArchive::Impl>(std::move(value));
        return std::unique_ptr<ParsedAuthorityArchive>(new ParsedAuthorityArchive(std::move(impl)));
    }catch(const rust::Error &){return {};}
}
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive_image(const AuthorityArchiveImage &image,
    AuthorityArchiveDecodeLimits l) {
    try {
        // Retain first, before exposing a temporary read-only slice to Rust.
        // The returned Rust index contains only owned offsets and metadata.
        auto retained=image;const auto raw=retained.raw();
        const auto hash=retained.digest();std::array<std::uint8_t,32> digest{};
        for(unsigned i=0;i<32;++i)digest[i]=std::to_integer<std::uint8_t>(hash[i]);
        auto index=online_protocol_bridge::index_authority_archive_raw(
            rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(raw.data()),raw.size()),digest,
            {l.encoded_bytes,l.decoded_bytes,l.allocated_bytes,l.container_elements,l.fixed_native_bytes,l.working_bytes});
        auto impl=std::make_unique<ParsedAuthorityArchive::Impl>(std::move(retained),std::move(index));
        return std::unique_ptr<ParsedAuthorityArchive>(new ParsedAuthorityArchive(std::move(impl)));
    }catch(const rust::Error &){return {};}
}
}
