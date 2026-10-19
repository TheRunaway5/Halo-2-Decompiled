#include "halo2/cache.hpp"
#include "halo2/source_weapon_material_input.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
unsigned checks{},digit_cases{},controls{};
void check(bool value,const char* message) {
    ++checks;if(!value)throw FormatError(message);
}
void matches(const std::optional<FunctionValue>& actual,const Json& expected) {
    check(actual.has_value(),"Original ammunition digit producer is unresolved");
    check(std::bit_cast<std::uint32_t>(actual->value)==expected[0].get<std::uint32_t>() &&
        actual->enabled==bool(expected[1].get<unsigned>()),"Ammunition digit differs from unchanged1046C0");
    ++digit_cases;
}
void digits(const Json& fixture,const SourceWeaponMaterialNames& names,
    const std::array<std::uint32_t,2>& selectors) {
    ShaderFunctionTables tables{};
    std::array<SourceWeaponMagazineDefinition,2> definitions{{{36,36,108},{60,60,240}}};
    std::array<SourceWeaponMagazineState,2> states{{{0,123},{1234,-45}}};
    for(unsigned word=0;word<65536;++word) {
        const auto loaded=std::bit_cast<std::int16_t>(std::uint16_t(word));
        states[0].loaded=loaded;
        // Unlike normalized ammunition, digits do not read capacity/reserve.
        definitions[0].maximum_loaded=loaded;
        const auto& row=fixture["cycle"][loaded<0?0:unsigned(loaded)%100];
        for(unsigned digit=0;digit<2;++digit)
            matches(source_weapon_material_input(selectors[digit],{},
                std::span(definitions).first(1),std::span(states).first(1),{},tables,0.f,{},names),row[digit]);
    }
    for(const auto& control:fixture["controls"]) {
        const auto count=control["magazines"].get<unsigned>();
        states[0].loaded=control["loaded"].get<std::int16_t>();
        states[1].loaded=control["secondary_loaded"].get<std::int16_t>();
        for(unsigned digit=0;digit<2;++digit)
            matches(source_weapon_material_input(selectors[digit],{},
                std::span(definitions).first(count),std::span(states).first(count),{},tables,0.f,{},names),control["results"][digit]);
    }
    check(!source_weapon_material_input(selectors[0],{},std::span(definitions).first(1),{},
        {},tables,0.f,{},names),"Unbound magazine state manufactured a digit");++controls;
}
}
int main(int argc,char** argv)try {
    check(argc==2 || argc==4,"Supply original digit fixture and optional actual MCC map/small report");
    std::ifstream input(argv[1]);check(bool(input),"Original digit fixture missing");
    const auto fixture=Json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["entry"]==0x1046c0u && fixture["signed_counts_verified"]==65536 &&
        fixture["positive_period"]==100 && fixture["all_negative_counts_equal_zero"]==true,
        "Digit fixture authority/domain differs");
    SourceWeaponMaterialNames original;
    digits(fixture,original,{0x1700056bu,0x1700056cu});
    unsigned namespaces=1;
    if(argc==4) {
        Cache cache(argv[2]);check(cache.is_mcc(),"Material namespace acceptance requires actual MCC");
        const auto names=SourceWeaponMaterialNames::from_cache(cache);
        check(cache.string_id(0x17000577u)=="primary_ammunition_ones" &&
            cache.string_id(0x17000578u)=="primary_ammunition_tens","MCC authored digit names differ");
        digits(fixture,names,{0x17000577u,0x17000578u});++namespaces;
        ShaderFunctionTables tables{};SourceWeaponShaderState state{};
        state.heat=.5f;state.power_fraction=.25f;
        const auto value=[&](unsigned id,std::span<const ObjectFunction> functions={},
            std::optional<FunctionValue> charge={}) {
            return source_weapon_material_input(id,state,{}, {},functions,tables,0.f,charge,names);
        };
        const auto required=[&](unsigned id,std::span<const ObjectFunction> functions={},
            std::optional<FunctionValue> charge={}) {
            const auto result=value(id,functions,charge);
            check(result.has_value(),"Expected MCC builtin or authored function is unresolved");
            return *result;
        };
        check(required(0x030005b2u).value==1.f && required(0x030005b2u).enabled,"MCC one is not active1");
        check(required(0x040005b3u).value==0.f && !required(0x040005b3u).enabled,"MCC zero lost inactive0");
        check(required(0x04000573u).value==.5f && required(0x03000583u).value==.25f,
            "MCC heat/age inputs lost their original state producers");
        check(required(0x09000574u).value==source_weapon_turned_on(state).value,"MCC turned_on differs");
        check(!value(0x0f000580u),"Absent charged producer was manufactured");
        check(required(0x0f000580u,{},FunctionValue{.375f,true}).value==.375f,"MCC charged producer lost its state");
        check(!value(0xdeadbeefu) && !value(0x03000573u) && !value(0x04000567u),
            "Foreign/length-mismatched/original-namespace ID became an MCC builtin");
        std::array<ObjectFunction,2> aliases{};
        aliases[0].output=0x0a00fe01u;aliases[0].input=0x04000573u;
        aliases[0].enable=0x040005b3u;
        check(required(aliases[0].output,aliases).value==0.f && !required(aliases[0].output,aliases).enabled,
            "MCC inactive authored enable gate was treated as an override");
        aliases[0].enable=0x030005b2u;
        check(required(aliases[0].output,aliases).value==.5f,"MCC active enable gate lost authored heat");
        // A local output can equal an Xbox builtin token. Its authored ID
        // must remain local while builtin inputs use the MCC namespace.
        aliases[0].scale=0x04000567u;aliases[1].output=0x04000567u;aliases[1].input=0x03000583u;
        check(required(aliases[0].output,aliases).value==.125f &&
            required(aliases[1].output,aliases).value==.25f,"Authored output/scale IDs were remapped as builtins");
        aliases[1].input=0xdeadbeefu;
        check(!value(aliases[0].output,aliases),"Missing authored scale producer became zero");
        controls+=12;
    }
    Json report{{"passed",true},{"original_xbe_sha256",fixture["original_xbe_sha256"]},
        {"signed_counts_per_namespace",65536},{"namespaces",namespaces},{"digit_cases",digit_cases},
        {"checks",checks},{"controls",controls},{"runtime_cache_source",argc==4?"MCC":"none"},
        {"authored_function_ids_preserved",true},{"source_results_supplied",false},{"full_weapon_parity",false}};
    if(argc==4){std::ofstream out(argv[3]);check(bool(out),"Cannot save small material-input report");out<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
