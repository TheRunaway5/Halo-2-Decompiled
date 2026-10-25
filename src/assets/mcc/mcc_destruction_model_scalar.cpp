#include "halo2/mcc_destruction_model_scalar.hpp"

namespace halo2 {
namespace {
std::uint32_t add(std::uint32_t address,std::uint32_t offset) {
    if(address>0xffffffffu-offset)throw FormatError("Destruction model scalar address overflow");
    return address+offset;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)throw FormatError("Truncated destruction model scalar word");
    std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[offset+n])<<(n*8);
    return value;
}
}
std::optional<MccGameplayUnitScalarReceipt> convert_mcc_destruction_model_scalars(
    TagGroup group,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& read) {
    if(group!=tag_groups::model || name!="objects\\vehicles\\h_turret_ap\\h_turret_ap")return {};
    if(!read)throw FormatError("Destruction model scalar conversion requires owned metadata");
    MccGameplayUnitScalarReceipt receipt;
    receipt.status=MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized;
    const auto view=[&](std::uint32_t address,std::size_t size){
        if(!size || size-1>0xffffffffu-address)
            throw FormatError("Destruction model scalar extent overflow");
        auto result=read(address,size);
        if(result.size()!=size)throw FormatError("Destruction model scalar reader returned a short view");
        return result;
    };
    const auto only=[&](std::uint32_t descriptor,std::size_t stride)->std::optional<std::uint32_t>{
        const auto block=view(descriptor,8);
        if(word(block,0)!=1)return {};
        const auto address=word(block,4);
        if(!address || address==0xffffffffu)
            throw FormatError("Destruction model scalar block has no owned metadata");
        (void)view(address,stride);return address;
    };
    const auto damage=only(add(root,0x60u),0x100u);if(!damage)return receipt;
    const auto section=only(add(*damage,0xbcu),0x38u);if(!section)return receipt;
    const auto response=only(add(*section,0xcu),0x50u);if(!response)return receipt;
    const auto words=view(*response,0x50u);
    // Authenticated authored tuples and map hashes:
    // tools/mcc-destruction-model-scalar-profile.json. Restore the literal
    // original flag word without inferring a meaning for MCC's added bit25.
    // Every other reviewed response scalar participates in this guard.
    constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,7> guards{{
        {0,0},{8,0},{0x20,4},{0x2c,0},{0x30,0x65666665u},{0x44,0},{0x4c,0}}};
    for(const auto [offset,expected]:guards)if(word(words,offset)!=expected)return receipt;
    const auto flags=word(words,4);
    auto &field=receipt.fields[0];field.address=add(*response,4);field.words=1;
    field.before[0]=field.after[0]=flags;receipt.field_count=1;
    if(flags==0){receipt.status=MccGameplayUnitScalarStatus::AlreadyEquivalent;return receipt;}
    if(flags!=0x02000000u)return receipt;
    auto destination=view(field.address,4);
    for(auto &byte:destination)byte=std::byte{};
    field.after[0]=0;receipt.status=MccGameplayUnitScalarStatus::Converted;
    return receipt;
}
}
