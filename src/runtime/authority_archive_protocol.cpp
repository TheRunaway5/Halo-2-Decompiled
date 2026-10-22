#include "halo2/authority_archive_protocol.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <stdexcept>

namespace halo2 {
namespace {
std::span<const std::byte> bytes(rust::Slice<const std::uint8_t> value) {
    return {reinterpret_cast<const std::byte*>(value.data()),value.size()};
}
}
struct ParsedAuthorityArchive::Impl {
    rust::Box<online_protocol_bridge::Archive> value;
    explicit Impl(rust::Box<online_protocol_bridge::Archive> v):value(std::move(v)){}
};
ParsedAuthorityArchive::ParsedAuthorityArchive(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
ParsedAuthorityArchive::~ParsedAuthorityArchive()=default;
AuthorityArchiveValue ParsedAuthorityArchive::node(std::size_t index)const {
    const auto n=impl_->value->archive_node(index);
    if(!n.valid)throw std::runtime_error("Authority archive node is out of range");
    return {n.tag,n.bits,n.count,n.next};
}
std::span<const std::byte> ParsedAuthorityArchive::name(std::size_t index)const {return bytes(impl_->value->archive_name(index));}
std::span<const std::byte> ParsedAuthorityArchive::data(std::size_t index)const {return bytes(impl_->value->archive_data(index));}
std::span<const std::byte> ParsedAuthorityArchive::raw()const {return bytes(impl_->value->archive_raw());}
std::array<std::byte,32> ParsedAuthorityArchive::expected_hash()const {
    std::array<std::byte,32> result{};const auto source=impl_->value->archive_hash();
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte(source[i]);return result;
}
std::size_t ParsedAuthorityArchive::size()const {return impl_->value->archive_len();}
bool ParsedAuthorityArchive::claim(std::uint64_t n,std::uint64_t extent,bool elements)const {return impl_->value->archive_claim(n,extent,elements);}
AuthorityArchiveParseStats ParsedAuthorityArchive::stats()const {
    const auto s=impl_->value->archive_stats();
    if(!s.valid)throw std::runtime_error("Authority archive budget is unavailable");
    return {s.raw_bytes,s.nodes,s.metadata_bytes,s.deepest,s.working_limit,s.working_reserved,s.working_peak,s.native_claimed};
}
std::unique_ptr<ParsedAuthorityArchive> parse_authority_archive(std::span<const std::byte> packet,AuthorityArchiveDecodeLimits l) {
    try {
        auto value=online_protocol_bridge::parse_authority_archive(
            rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(packet.data()),packet.size()),
            {l.encoded_bytes,l.decoded_bytes,l.allocated_bytes,l.container_elements,l.fixed_native_bytes});
        auto impl=std::make_unique<ParsedAuthorityArchive::Impl>(std::move(value));
        return std::unique_ptr<ParsedAuthorityArchive>(new ParsedAuthorityArchive(std::move(impl)));
    }catch(const rust::Error &){return {};}
}
}
