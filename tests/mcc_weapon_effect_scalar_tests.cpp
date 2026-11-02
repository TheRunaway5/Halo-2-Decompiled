#include "halo2/mcc_gameplay_scalar.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool ok,const char *why){if(!ok)throw FormatError(why);}
void put(std::span<std::byte> bytes,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8));
}
}
int main(int argc,char **argv)try {
    need(argc==2 || argc==4,"Supply profile manifest and optional original/MCC reference maps");
    std::ifstream in(argv[1]);const auto manifest=nlohmann::json::parse(in);
    need(manifest.at("schema")=="H2_MCC_TU15_WEAPON_EFFECT_FLAGS_V1" &&
        manifest.at("effect_paths_audited")==70 && manifest.at("profiles").size()==16,
        "Authored weapon effect reference scope changed");
    unsigned profiles{},new_profiles{},actual{};
    for(const auto &profile:manifest.at("profiles")) {
        const auto name=profile.at("name").get<std::string>();
        const auto donor=std::uint32_t(std::stoul(profile.at("donor_bits").get<std::string>(),nullptr,16));
        const auto target=std::uint32_t(std::stoul(profile.at("target_bits").get<std::string>(),nullptr,16));
        need((donor^target)==4,"Reference changed more than the original event RNG selector");
        std::vector<std::byte> bytes(0x1c);
        for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(n*17+0xa7);
        put(bytes,donor);const auto original=bytes;auto expected=bytes;put(expected,target);
        const auto changed=convert_mcc_gameplay_scalar(tag_groups::effect,name,bytes);
        need(changed && changed->status==MccGameplayScalarStatus::Converted && bytes==expected,
            "Native weapon effect conversion changed another byte or selected the wrong RNG stream");
        const auto again=convert_mcc_gameplay_scalar(tag_groups::effect,name,bytes);
        need(again && again->status==MccGameplayScalarStatus::AlreadyEquivalent && bytes==expected,
            "Native weapon effect conversion is not idempotent");
        auto authored=original;put(authored,donor^0x80000000u);const auto before=authored;
        const auto custom=convert_mcc_gameplay_scalar(tag_groups::effect,name,authored);
        need(custom && custom->status==MccGameplayScalarStatus::AuthoredTupleUnrecognized && authored==before,
            "Native weapon effect conversion overwrote unrecognized authored flags");
        need(!convert_mcc_gameplay_scalar(tag_groups::effect,name+"_custom",authored) && authored==before,
            "Native weapon effect conversion changed a custom path");
        need(!convert_mcc_gameplay_scalar(tag_groups::bitmap,name,authored) && authored==before,
            "Native weapon effect conversion crossed tag groups");
        auto short_bytes=original;short_bytes.resize(3);bool rejected{};
        try{(void)convert_mcc_gameplay_scalar(tag_groups::effect,name,short_bytes);}catch(const FormatError&){rejected=true;}
        need(rejected && short_bytes==std::vector<std::byte>(original.begin(),original.begin()+3),
            "Truncated weapon effect metadata was partly written");
        ++profiles;if(!profile.at("existing_profile").get<bool>())++new_profiles;
    }
    if(argc==4) {
        Cache original(argv[2],{},OriginalData::native_tu15()),mcc(argv[3]);
        need(!original.is_mcc() && mcc.is_mcc(),"Actual flags need original references and MCC runtime metadata");
        for(const auto &profile:manifest.at("profiles")) {
            const auto name=profile.at("name").get<std::string>();
            const auto *left=original.query_tag({.group=tag_groups::effect,.name=name,.require_address=true});
            const auto *right=mcc.query_tag({.group=tag_groups::effect,.name=name,.require_address=true});
            if(!left || !right)continue;
            const auto a=original.load_source_tag_metadata(left->datum),b=mcc.load_source_tag_metadata(right->datum);
            const auto expected=std::uint32_t(std::stoul(profile.at("target_bits").get<std::string>(),nullptr,16));
            need(Bytes(a.bytes_at(a.source_address(),4)).u32(0)==expected &&
                Bytes(b.bytes_at(b.source_address(),4)).u32(0)==expected,
                "Actual converted MCC weapon effect RNG selector differs from original metadata");++actual;
        }
        need(actual>=8,"Actual weapon effect reference coverage is incomplete");
    }
    need(new_profiles==12,"Unexpected new weapon effect profile count");
    std::cout<<"PASS "<<profiles<<" exact event RNG tuples ("<<new_profiles<<" new), authored guards; "
        <<actual<<" actual original/MCC metadata comparisons\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
