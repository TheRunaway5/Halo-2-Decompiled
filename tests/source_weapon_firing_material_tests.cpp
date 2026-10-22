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
unsigned cases{};
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
void domain(const Json &fixture,const SourceWeaponMaterialNames &names,bool mcc) {
    std::vector<unsigned char> bytes;
    const ShaderFunctionTables tables{};
    for(const auto &count:fixture["barrel_counts"])
    for(const auto &pair:fixture["tick_pairs"])
    for(const auto &primary:fixture["raw_values"])
    for(const auto &secondary:fixture["raw_values"])
    for(const auto &selector:fixture["selectors"]) {
        const auto original=selector.get<std::uint32_t>();
        const auto authored=mcc?(original==0x0e000572u?0x0e00057eu:0x1000057fu):original;
        const SourceWeaponFiringMaterialState state{std::bit_cast<std::int32_t>(count.get<std::uint32_t>()),
            pair[0].get<std::uint32_t>(),pair[1].get<std::uint32_t>(),
            {std::bit_cast<float>(primary.get<std::uint32_t>()),std::bit_cast<float>(secondary.get<std::uint32_t>())}};
        const auto value=source_weapon_material_input(authored,{}, {}, {}, {},tables,0.f,{},names,
            0x12340000u,{},state);
        check(value.has_value(),"Declared firing input lost its original producer");
        for(const auto word:{count.get<std::uint32_t>(),state.current_tick,state.last_fired_tick,
            primary.get<std::uint32_t>(),secondary.get<std::uint32_t>(),original,std::bit_cast<std::uint32_t>(value->value)})
            for(unsigned n=0;n<4;++n)bytes.push_back(word>>(n*8));
        bytes.push_back(value->enabled);++cases;
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream text;text<<std::hex<<std::setfill('0');for(const auto byte:hash)text<<std::setw(2)<<unsigned(byte);
    check(text.str()==fixture["results_sha256"].get<std::string>(),"Firing material differs from complete unchanged TU1.5 dispatch");
    const auto selector=mcc?0x0e00057eu:0x0e000572u;
    check(!source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,0x12340000u),
        "Missing actual firing state manufactured a result");
    check(!source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,{}, {},SourceWeaponFiringMaterialState{}),
        "Missing full object identity manufactured a firing result");
}
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==4,"Supply original firing fixture and optional MCC map/report");
#ifdef HALO2_FIRING_REFERENCE_ONLY
    check(argc==2,"Reference-only component cannot claim MCC cache acceptance");
#endif
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["entry"]==0x10de40u && fixture["weapon_callback"]==0x1046c0u &&
        fixture["barrel_count_offset"]==0x2d0u && fixture["current_tick_pointer"]==0x51df54u &&
        fixture["current_tick_offset"]==8 && fixture["last_fired_offset"]==0x248u &&
        fixture["barrel_value_offsets"]==Json::array({0x1b4u,0x1e8u}) && fixture["cases"]==13520,
        "Firing reference authority differs");
    domain(fixture,{},false);unsigned namespaces=1,mcc_inputs{};
#ifndef HALO2_FIRING_REFERENCE_ONLY
    if(argc==4) {
        Cache cache(argv[2]);check(cache.is_mcc(),"Firing material acceptance requires prepared MCC content");
        const auto names=SourceWeaponMaterialNames::from_cache(cache);domain(fixture,names,true);++namespaces;
        const ShaderFunctionTables tables{};
        for(const auto &tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true}))
            for(const auto &function:cache.load_object_functions(tag.datum))
                if(function.input==0x0e00057eu || function.input==0x1000057fu) {
                    const bool secondary=function.input==0x1000057fu;
                    check(cache.string_id(function.input)==(secondary?"secondary_firing":"primary_firing"),
                        "MCC authored firing selector changed identity");
                    const SourceWeaponFiringMaterialState state{2,17,16,{.25f,.5f}};
                    const auto value=source_weapon_material_input(function.input,{}, {}, {}, {},tables,0.f,{},names,
                        0x12340000u,{},state);
                    check(value && value->enabled && value->value==(secondary?.5f:.25f),
                        "Actual MCC authored input selected another barrel's value");++mcc_inputs;
                }
        check(mcc_inputs>0,"No actual MCC firing material input was exercised");
        check(!source_weapon_material_input(0x0e000572u,{}, {}, {}, {},tables,0.f,{},names,0x12340000u,{},SourceWeaponFiringMaterialState{}),
            "Foreign Xbox firing token became an MCC builtin");
    }
#endif
    const Json report{{"passed",true},{"runtime_cache_source",argc==4?"MCC":"none"},{"namespaces",namespaces},
        {"original_matched_firing_cases",cases},{"actual_mcc_authored_inputs",mcc_inputs},
        {"barrel_and_tick_values_are_declared_component_inputs",true},
        {"natural_weapon_firing_or_rendering_verified",false},{"full_weapon_parity",false}};
    if(argc==4) {std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
