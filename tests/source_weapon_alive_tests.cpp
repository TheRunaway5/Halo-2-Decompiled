#include "halo2/cache.hpp"
#include "halo2/source_weapon_material_input.hpp"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sstream>

using namespace halo2;
namespace {
using Json=nlohmann::json;
unsigned cases{},mcc_inputs{};
void check(bool value,const char* message){if(!value)throw FormatError(message);}
void domain(const Json& fixture,const SourceWeaponMaterialNames& names,unsigned selector) {
    ShaderFunctionTables tables{};std::vector<unsigned char> bytes;bytes.reserve(65536*7);
    for(unsigned flags=0;flags<65536;++flags) {
        const auto value=source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,
            0x12340000u,std::uint16_t(flags));
        check(value.has_value(),"Original alive producer is unavailable with declared owner flags");
        const auto& expected=fixture["result_period"][flags%8];const auto bits=std::bit_cast<unsigned>(value->value);
        check(bits==expected[0].get<unsigned>() && value->enabled==bool(expected[1].get<unsigned>()),
            "Alive input differs from complete unchanged10DE40/BEC50 execution");
        bytes.push_back(flags);bytes.push_back(flags>>8);
        for(unsigned n=0;n<4;++n)bytes.push_back(bits>>(8*n));bytes.push_back(value->enabled);++cases;
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream text;text<<std::hex<<std::setfill('0');for(auto byte:hash)text<<std::setw(2)<<unsigned(byte);
    check(text.str()==fixture["exhaustive_results_sha256"].get<std::string>(),"Complete alive flag domain differs from original execution");
    check(!source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,0x12340000u),
        "Missing actual object flags manufactured an alive result");
    check(!source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,{},std::uint16_t(0)),
        "Missing object identity manufactured an alive result");
}
}
int main(int argc,char** argv)try {
    check(argc==2 || argc==4,"Supply original alive fixture and optional MCC map/report");
#ifdef HALO2_ALIVE_REFERENCE_ONLY
    check(argc==2,"Reference-only component cannot claim MCC cache acceptance");
#endif
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["generic_entry"]==0x10de40u && fixture["base_owner"]==0xbec50u &&
        fixture["selector"]==0x0500055bu && fixture["flag_offset"]==0x10au && fixture["flag_words_verified"]==65536,
        "Alive reference authority differs");
    domain(fixture,{},0x0500055bu);unsigned namespaces=1;
#ifndef HALO2_ALIVE_REFERENCE_ONLY
    if(argc==4) {
        Cache cache(argv[2]);check(cache.is_mcc(),"Alive material acceptance requires an actual MCC cache");
        const auto names=SourceWeaponMaterialNames::from_cache(cache);domain(fixture,names,0x05000567u);++namespaces;
        ShaderFunctionTables tables{};
        for(const auto& tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true}))
            for(const auto& function:cache.load_object_functions(tag.datum))if(function.input==0x05000567u) {
                check(cache.string_id(function.input)=="alive","Authored MCC alive input changed identity");
                for(unsigned flags=0;flags<8;++flags) {
                    const auto value=source_weapon_material_input(function.input,{}, {}, {}, {},tables,0.f,{},names,
                        0x12340000u,std::uint16_t(flags));
                    const auto& expected=fixture["result_period"][flags];
                    check(value && std::bit_cast<unsigned>(value->value)==expected[0].get<unsigned>() &&
                        value->enabled==bool(expected[1].get<unsigned>()),"Actual authored MCC input lost original alive producer");
                    ++mcc_inputs;
                }
            }
        check(mcc_inputs>0,"MCC fixture did not exercise an authored weapon alive input");
        check(!source_weapon_material_input(0x0500055bu,{}, {}, {}, {},tables,0.f,{},names,0x12340000u,std::uint16_t(0)),
            "Foreign Xbox alive token became an MCC builtin");
    }
#endif
    Json report{{"passed",true},{"runtime_cache_source",argc==4?"MCC":"none"},{"namespaces",namespaces},
        {"original_matched_alive_cases",cases},{"actual_mcc_authored_inputs",mcc_inputs},
        {"flags_are_declared_component_inputs",true},{"natural_weapon_lifetime_or_rendering_verified",false},
        {"missing_owner_or_flags_remain_unbound",true},{"full_weapon_parity",false}};
    if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
