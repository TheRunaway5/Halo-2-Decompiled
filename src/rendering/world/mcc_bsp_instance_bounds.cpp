#include "halo2/mcc_bsp_instance_bounds.hpp"
#include <algorithm>
#include <limits>
#include <openssl/evp.h>

namespace halo2 {
namespace {
constexpr auto hex_digest(std::string_view hex){
    std::array<unsigned char,32> result{};
    const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(unsigned i=0;i<32;++i)result[i]=static_cast<unsigned char>(nibble(hex[i*2])*16+nibble(hex[i*2+1]));
    return result;
}
#include "rendering/world/mcc_bsp_instance_bounds_profile.inc"
void word(std::span<std::byte> data,unsigned at,std::uint32_t value){for(unsigned i=0;i<4;++i)data[at+i]=std::byte(value>>(8*i));}
std::array<unsigned char,32> digest(std::span<const std::byte> bytes){
    std::array<unsigned char,32> result{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),result.data(),&size,EVP_sha256(),nullptr)!=1||size!=result.size())
        throw FormatError("Cannot authenticate MCC stored instance bounds");
    return result;
}
struct Region{std::uint64_t token,host,size;};
bool disjoint(std::array<Region,3> regions,bool host){
    std::ranges::sort(regions,{},[&](const auto&r){return host?r.host:r.token;});
    for(unsigned i=1;i<regions.size();++i){const auto&a=regions[i-1];const auto&b=regions[i];if((host?a.host:a.token)+a.size>(host?b.host:b.token))return false;}
    return true;
}
} // namespace
std::optional<MccBspInstanceBoundsReceipt> convert_mcc_bsp_instance_bounds(
    const Tag& owner,const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& access){
    if(owner.type()!=tag_groups::scenario_structure_bsp||owner.name!=owner_name)return {};
    MccBspInstanceBoundsReceipt result{MccGameplayReferenceStatus::AuthoredTupleUnrecognized};
    if(owner.datum!=owner_datum||owner.address!=owner_address||owner.size!=owner_bytes)return result;
    if(!access)throw FormatError("MCC stored instance bounds require checked metadata access");
    std::array<Region,3> regions{};unsigned region=0;
    const auto view=[&](std::uint32_t at,std::size_t size){
        if(at<owner.address||std::uint64_t(at)+size>std::uint64_t(owner.address)+owner.size)
            throw FormatError("MCC stored bounds escape their exact typed allocation");
        auto data=access(at,size);
        if(data.size()!=size)throw FormatError("MCC stored bounds reader returned a short view");
        const auto host=reinterpret_cast<std::uintptr_t>(data.data());
        if(!host||size>std::numeric_limits<std::uintptr_t>::max()-host)
            throw FormatError("MCC stored bounds have an invalid host view");
        regions[region++]={at,host,size};return data;
    };
    const auto root=view(owner.address,0x23c);const Bytes header(root);
    // The complete fixed root also authenticates all pointer/count words. No
    // pointer normalization, empty-slot masking or partial owner is accepted.
    if(header.u32(8)!=0x12130f01u||header.u32(0x138)!=43||header.u32(0x140)!=243||digest(root)!=root_sha)return result;
    const auto definitions=view(header.u32(0x13c),43*200);
    const auto instances=view(header.u32(0x144),243*88);
    if(!disjoint(regions,false)||!disjoint(regions,true))return result;
    if(Bytes(definitions).u32(0x5c)!=0x36138b96u||digest(definitions)!=definitions_sha)return result;
    const Bytes rows(instances);bool donor_transform=true,converted_transform=true;
    for(unsigned i=0;i<13;++i){donor_transform&=rows.u32(i*4)==transform_before[i];converted_transform&=rows.u32(i*4)==transform_after[i];}
    if(!donor_transform&&!converted_transform)return result;
    bool pre=true,post=true;
    for(const auto& s:scalars){const auto offset=s.instance*88+60+s.component*4;
        if(rows.u16(s.instance*88+52)!=s.definition)return result;
        pre&=rows.u32(offset)==s.before;post&=rows.u32(offset)==s.after;
    }
    if(!pre&&!post)return result;
    // Normalize only the two authenticated transform tuples to their exact
    // preimage while hashing every instance byte. The supplied transform is
    // never modified. The before/post sphere banks must match in their entirety.
    std::vector<std::byte> canonical(instances.begin(),instances.end());
    for(unsigned i=0;i<13;++i)word(canonical,i*4,transform_before[i]);
    if(digest(canonical)!=(pre?instance_pre_sha:instance_post_sha))return result;
    if(pre){for(const auto& s:scalars)word(canonical,s.instance*88+60+s.component*4,s.after);
        if(digest(canonical)!=instance_post_sha)return result;}
    for(unsigned i=0;i<scalars.size();++i){const auto&s=scalars[i];result.fields[i]={header.u32(0x144)+s.instance*88+60+s.component*4,s.before,s.after,s.instance,s.component};}
    result.field_count=static_cast<std::uint8_t>(scalars.size());
    if(post){result.status=MccGameplayReferenceStatus::AlreadyEquivalent;return result;}
    // All views, full-bank fingerprints, exact scalar preimages and prospective
    // postimage are validated before the first store. Preserve every other byte.
    for(const auto& s:scalars)word(instances,s.instance*88+60+s.component*4,s.after);
    result.status=MccGameplayReferenceStatus::Converted;return result;
}
} // namespace halo2
