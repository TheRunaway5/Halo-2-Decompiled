#include "halo2/mcc_destruction_model_contract.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t add(std::uint32_t address,std::uint32_t offset) {
    if(address>0xffffffffu-offset)throw FormatError("Destruction contract address overflow");
    return address+offset;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)throw FormatError("Truncated destruction contract word");
    std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[offset+n])<<(8*n);
    return value;
}
}
std::optional<MccDestructionModelContractReceipt> convert_mcc_destruction_model_contract(
    TagGroup group,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& read) {
    if(group!=tag_groups::model || name!="objects\\characters\\hunter\\hunter")return {};
    if(!read)throw FormatError("Destruction contract requires owned metadata");
    MccDestructionModelContractReceipt receipt;
    const auto view=[&](std::uint32_t address,std::size_t size){
        if(!size || size-1>0xffffffffu-address)throw FormatError("Destruction contract extent overflow");
        auto bytes=read(address,size);
        if(bytes.size()!=size)throw FormatError("Destruction contract reader returned a short view");
        return bytes;
    };
    const auto block=[&](std::uint32_t descriptor,unsigned count,std::size_t size)->std::optional<std::uint32_t>{
        const auto bytes=view(descriptor,8);if(word(bytes,0)!=count)return {};
        const auto address=word(bytes,4);
        if(!address || address==0xffffffffu)throw FormatError("Destruction contract block has no owned metadata");
        (void)view(address,size);return address;
    };
    const auto damage=block(add(root,0x60),1,0xf8);if(!damage)return receipt;
    const auto sections=block(add(*damage,0xbc),2,0x70);if(!sections)return receipt;
    // All reviewed info/section scalars participate, including both sections.
    // Authored source owners and exact donor words are pinned in the profile.
    constexpr std::array<std::pair<unsigned,std::uint32_t>,15> info{{
        {0,0},{8,0},{0x28,0x42c80000},{0x2c,0},{0x30,0},{0x34,0},{0x38,0x3f800000},
        {0x8c,0x41a00000},{0x94,0},{0x98,0x41200000},{0x9c,0x40000000},
        {0xa0,0},{0xcc,0x005f0094},{0xd0,0x3f000000},{0xd4,0}}};
    for(const auto [offset,value]:info)if(word(view(*damage,0xf8),offset)!=value)return receipt;
    constexpr std::array<std::pair<unsigned,std::uint32_t>,10> section{{
        {8,0x3f800000},{0x14,0},{0x18,0},{0x1c,0},{0x20,0},{0x24,0},
        {0x28,0},{0x2c,0},{0x30,0},{0x34,0x0000ffff}}};
    const auto bytes=view(*sections,0x70);
    constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,6> names{{
        {0x05002152,0x0a0013fa},{0x050020d0,0x0a001a41},{0x05001a10,0x0a00148f},
        {0x050016b5,0x0a000f75},{0x05000ddf,0x0a001a8c},{0x0500186f,0x0a00112c}}};
    if(word(bytes,0)!=0x040000bdu)return receipt;
    const auto named=std::ranges::find_if(names,[&](const auto &pair){return pair.first==word(bytes,0x38);});
    if(named==names.end())return receipt;
    for(const auto base:{0u,0x38u})for(const auto [offset,value]:section)
        if(word(bytes,base+offset)!=value)return receipt;
    if(word(bytes,0x38+4)!=0 || word(bytes,0x38+0xc)!=0 || word(bytes,0x38+0x10)!=0)return receipt;
    const auto flags=word(bytes,4),count=word(bytes,0xc),pointer=word(bytes,0x10);
    receipt.addresses={add(*sections,4),add(*sections,0xc),add(*sections,0x10)};
    receipt.before=receipt.after={flags,count,pointer};
    if(flags==0 && count==0 && pointer==0) {
        receipt.status=MccDestructionModelContractStatus::AlreadyEquivalent;return receipt;
    }
    if(flags!=2 || count!=1)return receipt;
    if(!pointer || pointer==0xffffffffu)throw FormatError("Hunter response has no owned metadata");
    const auto response=view(pointer,0x50);
    constexpr std::array<std::uint32_t,20> expected{{
        0,0,0,0x65666665,0xffffffff,0x6a707421,0xffffffff,0,0xffff0004,0,0,0,
        0x65666665,0xffffffff,0,0,0,0,0,0}};
    for(unsigned n=0;n<20;++n)if(n!=16 && word(response,n*4)!=expected[n])return receipt;
    if(word(response,0x40)!=named->second)return receipt;
    const std::array<std::pair<std::uint32_t,std::size_t>,4> ranges{{
        {root,0x84},{*damage,0xf8},{*sections,0x70},{pointer,0x50}}};
    for(unsigned a=0;a<ranges.size();++a)for(unsigned b=a+1;b<ranges.size();++b)
        if(std::uint64_t(ranges[a].first)<std::uint64_t(ranges[b].first)+ranges[b].second &&
           std::uint64_t(ranges[b].first)<std::uint64_t(ranges[a].first)+ranges[a].second)
            throw FormatError("Hunter authored metadata blocks overlap");
    // Validate every destination before the first write. Reads must remain
    // stable, and the original/donor response storage itself is never edited.
    std::array<std::span<std::byte>,3> destinations;
    for(unsigned n=0;n<3;++n)destinations[n]=view(receipt.addresses[n],4);
    for(auto destination:destinations)for(auto &byte:destination)byte=std::byte{};
    receipt.after={0,0,0};receipt.status=MccDestructionModelContractStatus::Converted;
    return receipt;
}
}
