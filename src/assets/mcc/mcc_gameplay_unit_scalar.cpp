#include "halo2/mcc_gameplay_unit_scalar.hpp"
#include <algorithm>

namespace halo2 {
namespace {
struct Field {
    std::uint32_t descriptor,count,index,stride,offset,words;
    std::array<std::uint32_t,3> donor,target;
};
struct Profile {TagGroup group;std::string_view name;std::array<Field,4> fields;std::uint8_t count;};
// Exact named tuples and all 38 original/MCC map hashes accompany the
// compiled table in tools/mcc-gameplay-unit-scalar-profiles.json. Each donor
// tuple was checked in every matching context before inclusion. In contrast,
// Gauss flyby and Relic's global CTF distance fail that context test and are
// excluded. This restores authored data, without assigning unverified seat
// flag meanings or claiming complete collision/body or visual equivalence.
// The Cache physics converter already packs embedded sphere records to0x80;
// original source consumers use those strides, not MCC's donor0xA0 records.
#include "assets/mcc/mcc_gameplay_unit_scalar_profiles.inc"
std::uint32_t add(std::uint32_t a,std::uint64_t b) {
    if(b>0xffffffffu-a)throw FormatError("Unit scalar source address overflow");
    return a+std::uint32_t(b);
}
}
std::optional<MccGameplayUnitScalarReceipt> convert_mcc_gameplay_unit_scalars(
    TagGroup group,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& read) {
    const auto profile=std::ranges::find_if(unit_profiles,[&](const auto& p){return p.group==group && p.name==name;});
    if(profile==unit_profiles.end())return {};
    if(!read)throw FormatError("Unit scalar conversion requires checked owned metadata");
    const auto view=[&](std::uint32_t address,std::size_t size) {
        auto result=read(address,size);
        if(result.size()!=size)throw FormatError("Unit scalar reader returned a short view");
        return result;
    };
    MccGameplayUnitScalarReceipt receipt;std::array<std::span<std::byte>,4> views;
    bool donor=true,target=true;
    for(unsigned n=0;n<profile->count;++n) {
        const auto& field=profile->fields[n];auto owner=root;
        if(field.descriptor!=0xffffffffu) {
            const Bytes block(view(add(root,field.descriptor),8));
            if(block.u32(0)!=field.count) {
                receipt.status=MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized;return receipt;
            }
            if(!block.u32(4) || block.u32(4)==0xffffffffu)
                throw FormatError("Unit scalar block has no owned metadata");
            const auto extent=std::uint64_t(field.count)*field.stride;
            (void)add(block.u32(4),extent-1);
            (void)view(block.u32(4),std::size_t(extent));
            owner=add(block.u32(4),std::uint64_t(field.index)*field.stride);
        }
        auto& result=receipt.fields[n];result.address=add(owner,field.offset);result.words=std::uint8_t(field.words);
        views[n]=view(result.address,field.words*4);const Bytes bytes(views[n]);
        for(unsigned w=0;w<field.words;++w) {
            result.before[w]=result.after[w]=bytes.u32(w*4);
            donor&=result.before[w]==field.donor[w];target&=result.before[w]==field.target[w];
        }
    }
    receipt.field_count=profile->count;
    if(target){receipt.status=MccGameplayUnitScalarStatus::AlreadyEquivalent;return receipt;}
    if(!donor){receipt.status=MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized;return receipt;}
    for(unsigned n=0;n<profile->count;++n) {
        const auto& field=profile->fields[n];
        for(unsigned w=0;w<field.words;++w) {
            const auto value=field.target[w];receipt.fields[n].after[w]=value;
            for(unsigned b=0;b<4;++b)views[n][w*4+b]=std::byte(value>>(b*8));
        }
    }
    receipt.status=MccGameplayUnitScalarStatus::Converted;return receipt;
}
} // namespace halo2
