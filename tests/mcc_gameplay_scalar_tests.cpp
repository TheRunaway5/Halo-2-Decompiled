#include "halo2/mcc_gameplay_scalar.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
struct Field {std::uint32_t offset,donor,target;unsigned bytes;};
struct Case {TagGroup group;std::string_view name;std::vector<Field> fields;};
const std::array cases{
    Case{tag_groups::weapon,"objects\\weapons\\melee\\energy_blade\\energy_blade",
        {{0x1b0,0x3ed6dfe4,0x3ed6dfe3,4},{0x1b4,0x3e76f125,0x3e76f125,4}}},
    Case{tag_groups::weapon,"objects\\weapons\\melee\\energy_blade\\energy_blade_useless",
        {{0x1b0,0x3eb30fe9,0x3eb30fe9,4},{0x1b4,0x3e4dc8f4,0x3e4dc8f5,4}}},
    Case{tag_groups::weapon,"objects\\weapons\\support_high\\flak_cannon\\flak_cannon",
        {{0x22c,0,2,2},{0x230,0,0x3e800000,4},{0x234,0,0x3e800000,4}}},
    Case{tag_groups::projectile,"objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet",
        {{0x128,0x2a,0x24,1}}},
    Case{tag_groups::weapon,"objects\\weapons\\multiplayer\\ball\\ball",{{0x12c,0x00340058,0x003400d8,4}}},
    Case{tag_groups::weapon,"objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle",{{0x12c,0x00000020,0x00004020,4}}},
    Case{tag_groups::weapon,"objects\\weapons\\rifle\\beam_rifle\\beam_rifle",{{0x12c,0x00000820,0x00004820,4}}},
    Case{tag_groups::weapon,"objects\\weapons\\rifle\\beam_rifle\\beam_rifle_noplayer",{{0x12c,0x20000820,0x20004820,4}}},
    Case{tag_groups::damage_effect,"globals\\collision",{{0x0c,4,0,4}}},
    Case{tag_groups::damage_effect,"objects\\weapons\\damage_effects\\slice_melee",{{0x0c,5,1,4}}},
    Case{tag_groups::damage_effect,"effects\\scenarios\\objects\\solo\\spacestation\\boarding_craft_shake",
        {{0x0c,6,2,4}}}
};
void require(bool value,const char* why){if(!value)throw FormatError(why);}
void put(std::span<std::byte> payload,const Field& field,std::uint32_t value) {
    Bytes(payload).require(field.offset,field.bytes);
    for(unsigned n=0;n<field.bytes;++n)payload[field.offset+n]=std::byte(value>>(n*8));
}
std::vector<std::byte> payload(const Case& c) {
    std::vector<std::byte> result(0x400);
    for(unsigned n=0;n<result.size();++n)result[n]=std::byte(n*17u+0xa7u);
    for(const auto& field:c.fields)put(result,field,field.donor);
    return result;
}
void synthetic() {
    for(const auto& c:cases) {
        auto bytes=payload(c),expected=bytes;
        for(const auto& field:c.fields)put(expected,field,field.target);
        const auto changed=convert_mcc_gameplay_scalar(c.group,c.name,bytes);
        require(changed && changed->status==MccGameplayScalarStatus::Converted,"Known donor tuple was not converted");
        require(bytes==expected,"Scalar conversion changed another authored byte or used inexact target bits");
        require(changed->field_count==c.fields.size(),"Scalar receipt omitted a guard field");
        for(unsigned n=0;n<changed->field_count;++n) {
            const auto& actual=changed->fields[n];const auto& field=c.fields[n];
            require(actual.offset==field.offset && actual.bytes==field.bytes &&
                actual.before==field.donor && actual.after==field.target,"Scalar receipt changed field width or exact bits");
        }
        const auto repeat=convert_mcc_gameplay_scalar(c.group,c.name,bytes);
        require(repeat && repeat->status==MccGameplayScalarStatus::AlreadyEquivalent && bytes==expected,
            "Already-equivalent scalar tuple was not idempotent");
        for(unsigned n=0;n<c.fields.size();++n) {
            auto modified=payload(c);modified[c.fields[n].offset]^=std::byte{0x10};
            const auto authored=modified;
            const auto mismatch=convert_mcc_gameplay_scalar(c.group,c.name,modified);
            require(mismatch && mismatch->status==MccGameplayScalarStatus::AuthoredTupleUnrecognized && modified==authored,
                "Mod-authored scalar tuple was overwritten instead of reported");
        }
        auto wrong_group=payload(c),wrong_name=wrong_group;
        require(!convert_mcc_gameplay_scalar(tag_groups::bitmap,c.name,wrong_group) && wrong_group==payload(c),
            "Scalar conversion reinterpreted another tag group");
        require(!convert_mcc_gameplay_scalar(c.group,std::string(c.name)+"_custom",wrong_name) && wrong_name==payload(c),
            "Scalar conversion changed a custom named definition");
        auto truncated=payload(c);
        truncated.resize(c.fields.back().offset+c.fields.back().bytes-1u);
        const auto before=truncated;bool rejected=false;
        try{(void)convert_mcc_gameplay_scalar(c.group,c.name,truncated);}catch(const FormatError&){rejected=true;}
        require(rejected && truncated==before,"Truncated scalar definition was partly changed");
    }
    const auto& flak=cases[2];auto partial=payload(flak);put(partial,flak.fields[0],2);
    const auto before=partial;const auto mismatch=convert_mcc_gameplay_scalar(flak.group,flak.name,partial);
    require(mismatch && mismatch->status==MccGameplayScalarStatus::AuthoredTupleUnrecognized && partial==before,
        "Partial authored tuple was completed without evidence");
    const auto& collision=cases[8];
    auto other_flags=payload(collision);put(other_flags,collision.fields[0],0x80000004u);
    const auto authored_flags=other_flags;
    const auto preserved=convert_mcc_gameplay_scalar(tag_groups::damage_effect,collision.name,other_flags);
    require(preserved && preserved->status==MccGameplayScalarStatus::AuthoredTupleUnrecognized && other_flags==authored_flags,
        "Damage profile cleared an authored flag word outside its exact retail tuple");
    require(!convert_mcc_gameplay_scalar(tag_groups::damage_effect,"custom\\unlisted_damage",other_flags) &&
        other_flags==authored_flags,"Damage profile applied a blanket bit clear to an unknown path");
    std::cout<<"PASS "<<cases.size()<<" exact scalar tuples, serialized widths, byte preservation, authored guards and idempotence\n";
}
void actual(const std::filesystem::path& reference,const std::filesystem::path& donor) {
    const Cache xbox(reference,{},OriginalData::native_tu15()),mcc(donor);
    require(!xbox.is_mcc() && mcc.is_mcc(),"Scalar integration requires actual Xbox and MCC maps");
    unsigned matched{},fields{};
    for(const auto& c:cases) {
        const auto select=[&](const Cache& cache)->const Tag* {
            const auto found=std::ranges::find_if(cache.tags(),[&](const auto& tag){
                return !tag.handle().is_null() && tag.address && tag.size &&
                    tag.type()==c.group && tag.name==c.name;
            });
            return found==cache.tags().end()?nullptr:&*found;
        };
        const auto* expected=select(xbox);const auto* converted=select(mcc);
        if(!expected || !converted)continue;
        for(const auto& field:c.fields) {
            require(std::ranges::equal(xbox.read_tag_bytes(expected->address+field.offset,field.bytes),
                mcc.read_tag_bytes(converted->address+field.offset,field.bytes)),
                "MCC cache did not publish the exact Xbox TU1.5 scalar bytes");
            ++fields;
        }
        auto bytes=mcc.read_tag_bytes(converted->address,converted->size);
        std::vector<std::byte> copy(bytes.begin(),bytes.end());
        const auto receipt=convert_mcc_gameplay_scalar(c.group,c.name,copy);
        require(receipt && receipt->status==MccGameplayScalarStatus::AlreadyEquivalent &&
            std::ranges::equal(bytes,copy),"Published MCC scalar profile is not idempotent");
        ++matched;
    }
    require(matched!=0,"No source-backed scalar profile exists in this map pair");
    std::cout<<"PASS "<<mcc.name()<<": "<<matched<<" actual vanilla scalar definitions / "<<fields<<" exact reference fields\n";
}
}
int main(int argc,char** argv)try {
    synthetic();
    if(argc==3)actual(argv[1],argv[2]);
    else if(argc!=1)throw halo2::FormatError("usage: mcc_gameplay_scalar_tests [XBOX.map MCC.map]");
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
