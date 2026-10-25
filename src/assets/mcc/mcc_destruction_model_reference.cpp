#include "halo2/mcc_destruction_model_reference.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t add(std::uint32_t address,std::uint32_t offset) {
    if(address>0xffffffffu-offset)throw FormatError("Destruction reference address overflow");
    return address+offset;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)throw FormatError("Truncated destruction reference word");
    std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[offset+n])<<(8*n);
    return value;
}
}
std::optional<MccGameplayReferenceReceipt> convert_mcc_destruction_model_references(
    TagGroup group,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess &access) {
    if(group!=tag_groups::model || name!=
        "scenarios\\objects\\flood\\flood_floor_clump_large\\flood_floor_clump_large")return {};
    if(!access.bytes)throw FormatError("Destruction reference requires owned metadata");
    MccGameplayReferenceReceipt receipt;receipt.profile="tu15-flood-clump-null-response-effect";
    receipt.status=MccGameplayReferenceStatus::AuthoredTupleUnrecognized;
    const auto view=[&](std::uint32_t address,std::size_t size){
        if(!size || size-1>0xffffffffu-address)throw FormatError("Destruction reference extent overflow");
        auto bytes=access.bytes(address,size);
        if(bytes.size()!=size)throw FormatError("Destruction reference reader returned a short view");
        return bytes;
    };
    const auto only=[&](std::uint32_t descriptor,std::size_t size)->std::optional<std::uint32_t>{
        const auto bytes=view(descriptor,8);if(word(bytes,0)!=1)return {};
        const auto address=word(bytes,4);
        if(!address || address==0xffffffffu)throw FormatError("Destruction reference block has no owner");
        (void)view(address,size);return address;
    };
    const auto info=only(add(root,0x60),0xf8);if(!info)return receipt;
    const auto section=only(add(*info,0xbc),0x38);if(!section)return receipt;
    const auto response=only(add(*section,0xc),0x50);if(!response)return receipt;
    // Exact scalars and all response words except the local effect handle.
    // The paired original/MCC map owners are authenticated in the profile.
    constexpr std::array<std::pair<unsigned,std::uint32_t>,15> info_words{{
        {0,0},{8,0},{0x28,0x3f800000},{0x2c,0},{0x30,0},{0x34,0},{0x38,0x3f800000},
        {0x8c,0},{0x94,0},{0x98,0},{0x9c,0},{0xa0,0},{0xcc,0xffffffff},{0xd0,0x447a0000},{0xd4,0}}};
    const auto info_bytes=view(*info,0xf8);
    for(const auto [offset,value]:info_words)if(word(info_bytes,offset)!=value)return receipt;
    constexpr std::array<std::pair<unsigned,std::uint32_t>,11> section_words{{
        {4,0},{8,0x3f800000},{0x14,0},{0x18,0},{0x1c,0},{0x20,0},{0x24,0},
        {0x28,0},{0x2c,0},{0x30,0},{0x34,0xffff}}};
    const auto section_bytes=view(*section,0x38);
    for(const auto [offset,value]:section_words)if(word(section_bytes,offset)!=value)return receipt;
    if(word(section_bytes,0)!=0x05001561 && word(section_bytes,0)!=0x05000a1c)return receipt;
    constexpr std::array<std::uint32_t,20> response_words{{
        0,0x80,0,0x65666665,0,0x6a707421,0xffffffff,0,0xffff0004,0,0,0,
        0x65666665,0xffffffff,0,0,0,0,0,0}};
    const auto response_bytes=view(*response,0x50);
    for(unsigned n=0;n<20;++n)if(n!=4 && word(response_bytes,n*4)!=response_words[n])return receipt;
    const std::array<std::pair<std::uint32_t,std::size_t>,4> ranges{{
        {root,0x84},{*info,0xf8},{*section,0x38},{*response,0x50}}};
    for(unsigned a=0;a<ranges.size();++a)for(unsigned b=a+1;b<ranges.size();++b)
        if(std::uint64_t(ranges[a].first)<std::uint64_t(ranges[b].first)+ranges[b].second &&
           std::uint64_t(ranges[b].first)<std::uint64_t(ranges[a].first)+ranges[a].second)
            throw FormatError("Flood-clump authored metadata blocks overlap");
    auto &field=receipt.fields[0];receipt.field_count=1;field.address=add(*response,0xc);
    field.before={tag_groups::effect,TagHandle{word(response_bytes,0x10)}};field.after=field.before;
    field.required_group=tag_groups::effect;field.required_name="";
    if(field.before.handle.is_null()) {
        receipt.status=MccGameplayReferenceStatus::AlreadyEquivalent;return receipt;
    }
    if(!access.resolve)throw FormatError("Destruction reference requires its full local tag identity owner");
    const auto identity=access.resolve(field.before.handle);
    if(!identity){receipt.status=MccGameplayReferenceStatus::RequiredDependencyUnavailable;return receipt;}
    if(identity->handle!=field.before.handle || identity->group!=tag_groups::effect || identity->name!=
        "effects\\objects\\characters\\flood_infection\\body_destroyed")return receipt;
    // Keep the original effe group; only its literal null handle differs.
    auto destination=view(add(*response,0x10),4);
    for(auto &byte:destination)byte=std::byte{0xff};
    field.after.handle=TagHandle{0xffffffff};receipt.status=MccGameplayReferenceStatus::Converted;
    return receipt;
}
}
