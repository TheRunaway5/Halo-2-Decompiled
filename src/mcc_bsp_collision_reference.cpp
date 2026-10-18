#include "halo2/mcc_bsp_collision_reference.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <openssl/evp.h>

namespace halo2 {
namespace {
// Owned paired maps: MCC SHA-256 395ad7bccefdb3d50950dc911e8adeee770a75e6ad109c3979d6a641fd648475;
// original 7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4.
// Serialization: main0, definition0..42; for each of eight blocks, LE u32
// definition index (main=0), block offset, count, then the entire raw payload.
// Pointers are deliberately excluded so relocation retains the same identity.
constexpr std::array<unsigned,8> strides{8,16,4,4,16,8,12,16};
constexpr std::array<std::array<std::uint32_t,8>,44> counts{{
    {{15395,6500,6485,21297,10897,12434,23473,10835}},
    {{69,34,45,90,131,107,200,95}},
    {{32,27,25,42,29,46,90,46}},
    {{11,9,9,11,5,13,25,14}},
    {{11,10,9,11,4,14,28,16}},
    {{27,19,21,35,38,42,78,38}},
    {{7,7,7,7,0,7,15,10}},
    {{47,20,30,58,61,58,112,46}},
    {{35,15,31,35,0,35,75,50}},
    {{77,30,62,99,82,93,183,96}},
    {{127,77,82,176,234,217,423,210}},
    {{77,30,62,99,82,93,183,96}},
    {{127,77,82,176,237,217,423,210}},
    {{42,32,37,57,10,42,90,60}},
    {{45,22,30,62,2,30,58,32}},
    {{6,6,6,6,0,6,12,8}},
    {{9,8,8,9,2,10,20,12}},
    {{6,6,6,6,4,10,20,12}},
    {{18,14,15,26,8,20,39,21}},
    {{38,22,29,49,4,24,38,16}},
    {{82,40,60,102,2,42,65,25}},
    {{82,40,60,102,2,42,65,25}},
    {{6,6,6,6,0,6,12,8}},
    {{25,18,26,118,11,38,59,15}},
    {{37,21,28,51,6,24,38,16}},
    {{45,25,46,191,5,52,79,19}},
    {{23,14,19,29,0,14,28,16}},
    {{30,18,24,39,0,18,36,20}},
    {{37,21,28,51,6,24,38,16}},
    {{25,18,26,118,11,38,59,15}},
    {{30,18,24,39,0,18,36,20}},
    {{23,14,19,29,0,14,28,16}},
    {{30,18,24,39,0,18,36,20}},
    {{30,18,24,39,0,18,36,20}},
    {{45,25,46,191,5,52,79,19}},
    {{115,61,78,174,14,68,107,41}},
    {{9,8,8,9,2,10,20,12}},
    {{6,6,6,6,0,6,12,8}},
    {{9,8,8,9,2,10,20,12}},
    {{179,79,124,285,18,95,148,65}},
    {{10,10,9,13,10,19,35,18}},
    {{16,16,16,16,0,16,30,16}},
    {{33,19,26,45,0,19,33,16}},
    {{31,18,25,40,0,18,36,20}},
}};
struct Scalar {unsigned owner,field,component;std::uint32_t before,after;};
constexpr std::array<Scalar,17> scalars{{
    {0,1,3,0x43361d71u,0x43361d55u},
    {0,4,0,0x3f6893b3u,0x3f6893b2u},
    {0,4,1,0xbed5f3f4u,0xbed5f3f7u},
    {0,4,2,0x43ecbe56u,0x43ecbe54u},
    {0,7,0,0xc4201873u,0xc420186bu},
    {0,7,1,0x436f1fbeu,0x436f1fadu},
    {0,7,2,0xc14446dcu,0xc144470cu},
    {1,1,0,0x3efc6111u,0x3efc610bu},
    {1,1,1,0x3f5ebceau,0x3f5ebcebu},
    {1,1,2,0xb845de6cu,0xb845de71u},
    {1,1,3,0xbe314db6u,0xbe314da1u},
    {1,4,0,0x3f66ac73u,0x3f66ac75u},
    {1,4,1,0xbede0ac6u,0xbede0abfu},
    {1,4,2,0xbef57e46u,0xbef57e53u},
    {1,7,0,0x3de7ae9bu,0x3de7ae8fu},
    {1,7,1,0xbe86ab58u,0xbe86ab4au},
    {1,7,2,0x3fa284e0u,0x3fa284d0u},
}};
constexpr auto hex_digest(std::string_view hex) {
    std::array<unsigned char,32> result{};
    const auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(unsigned i=0;i<32;++i)result[i]=static_cast<unsigned char>(nibble(hex[i*2])*16+nibble(hex[i*2+1]));
    return result;
}
constexpr auto preimage=hex_digest("ae3206ce3d6639889f3a65e569fa885d1f19fc0385a3c7816a5fac3b184100a3");
constexpr auto postimage=hex_digest("766867e8913fa32f3a833acdb29e4f012d77e73327958c0593cb1e091d7f8b2a");
std::uint32_t add(std::uint32_t base,std::size_t offset) {
    if(offset>std::numeric_limits<std::uint32_t>::max()-base)
        throw FormatError("MCC collision reference address overflows its source token");
    return base+static_cast<std::uint32_t>(offset);
}
void word(std::span<std::byte> data,unsigned offset,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)data[offset+i]=std::byte(value>>(8*i));
}
struct Payload {std::uint32_t address{};std::span<std::byte> bytes;};
using Payloads=std::array<std::array<Payload,8>,44>;
struct Region {std::uint64_t token,host,size;};
bool disjoint(std::vector<Region> regions,bool host) {
    std::ranges::sort(regions,{},[&](const auto& r){return host?r.host:r.token;});
    for(std::size_t i=1;i<regions.size();++i) {
        const auto& a=regions[i-1];const auto& b=regions[i];
        if((host?a.host:a.token)+a.size>(host?b.host:b.token))return false;
    }
    return true;
}
std::array<unsigned char,32> digest(const Payloads& payloads,bool proposed) {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),EVP_MD_CTX_free);
    if(!context || EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)!=1)
        throw FormatError("Cannot initialize MCC collision reference digest");
    const auto update=[&](std::span<const std::byte> data) {
        if(!data.empty() && EVP_DigestUpdate(context.get(),data.data(),data.size())!=1)
            throw FormatError("Cannot hash MCC collision reference payload");
    };
    for(unsigned owner=0;owner<counts.size();++owner)for(unsigned field=0;field<8;++field) {
        std::array<std::byte,12> header{};
        word(header,0,owner?owner-1:0);word(header,4,field*8);word(header,8,counts[owner][field]);
        update(header);
        const auto data=payloads[owner][field].bytes;
        const bool changed=proposed && std::ranges::any_of(scalars,[&](const auto& s){return s.owner==owner && s.field==field;});
        if(!changed){update(data);continue;}
        // Every proven replacement is in the first 16-byte row. Stage the
        // prospective digest without changing any owner before all guards pass.
        std::array<std::byte,16> first{};std::ranges::copy(data.first(16),first.begin());
        for(const auto& s:scalars)if(s.owner==owner && s.field==field)word(first,s.component*4,s.after);
        update(first);update(data.subspan(16));
    }
    std::array<unsigned char,32> result{};unsigned size{};
    if(EVP_DigestFinal_ex(context.get(),result.data(),&size)!=1 || size!=result.size())
        throw FormatError("Cannot finish MCC collision reference digest");
    return result;
}
} // namespace
std::optional<MccBspCollisionReferenceReceipt> convert_mcc_bsp_collision_reference(
    TagGroup group,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& bytes) {
    if(group!=tag_groups::scenario_structure_bsp || name!="scenarios\\solo\\03a_oldmombasa\\earthcity_3")return {};
    if(!bytes)throw FormatError("MCC collision reference conversion requires checked bank access");
    const auto view=[&](std::uint32_t address,std::size_t size) {
        if(!address || address==0xffffffffu || size>0x100000000ull-address)
            throw FormatError("MCC collision reference has an invalid metadata owner");
        auto result=bytes(address,size);
        if(result.size()!=size)throw FormatError("MCC collision reference reader returned a short view");
        return result;
    };
    MccBspCollisionReferenceReceipt receipt{MccGameplayReferenceStatus::AuthoredTupleUnrecognized};
    std::vector<Region> regions;
    const auto owner_view=[&](std::uint32_t address,std::size_t size) {
        auto data=view(address,size);const auto host=reinterpret_cast<std::uintptr_t>(data.data());
        if(!host || size>std::numeric_limits<std::uintptr_t>::max()-host)
            throw FormatError("MCC collision reference has an invalid host view");
        regions.push_back({address,host,size});return data;
    };
    const Bytes header(owner_view(root,0x23c));
    if(header.u32(8)!=0x12130f01u || header.u32(0x14)!=1 ||
        header.u32(0x138)!=43 || header.u32(0x140)!=243)return receipt;
    const auto main_address=header.u32(0x18),definitions_address=header.u32(0x13c);
    const Bytes main(owner_view(main_address,64));
    const Bytes definitions(owner_view(definitions_address,43*200));
    // Protect instance names, bounds, policies and transforms from a payload
    // pointer that aliases metadata. Their authored values are never copied.
    (void)owner_view(header.u32(0x144),243*88);
    if(definitions.u32(0x5c)!=0x36138b96u)return receipt;
    Payloads payloads{};
    for(unsigned owner=0;owner<counts.size();++owner)for(unsigned field=0;field<8;++field) {
        const auto at=owner?(owner-1)*200+0x70+field*8:field*8;
        const auto& record=owner?definitions:main;
        if(record.u32(at)!=counts[owner][field])return receipt;
        if(!counts[owner][field])continue;
        auto& p=payloads[owner][field];p.address=record.u32(at+4);
        p.bytes=owner_view(p.address,counts[owner][field]*strides[field]);
    }
    if(!disjoint(regions,false) || !disjoint(regions,true))return receipt;
    bool donor=true,equivalent=true;
    for(unsigned i=0;i<scalars.size();++i) {
        const auto& s=scalars[i];const auto& p=payloads[s.owner][s.field];
        const auto value=Bytes(p.bytes).u32(s.component*4);
        donor&=value==s.before;equivalent&=value==s.after;
        receipt.fields[i]={add(p.address,s.component*4),s.before,s.after};
    }
    receipt.field_count=static_cast<std::uint8_t>(scalars.size());
    if(!donor && !equivalent)return receipt;
    const auto actual=digest(payloads,false);
    if(equivalent) {
        if(actual==postimage)receipt.status=MccGameplayReferenceStatus::AlreadyEquivalent;
        return receipt;
    }
    if(actual!=preimage || digest(payloads,true)!=postimage)return receipt;
    for(const auto& s:scalars)word(payloads[s.owner][s.field].bytes,s.component*4,s.after);
    receipt.status=MccGameplayReferenceStatus::Converted;return receipt;
}
} // namespace halo2
