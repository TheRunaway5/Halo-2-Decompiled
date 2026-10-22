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
unsigned cases{},aliases{};
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
void domain(const Json& fixture,const SourceWeaponMaterialNames& names,unsigned selector) {
    ShaderFunctionTables tables{};
    const auto value=[&](unsigned object) {
        const auto result=source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names,object);
        check(result.has_value(),"Original object-owned random_constant is unresolved");
        ++cases;return *result;
    };
    check(!source_weapon_material_input(selector,{}, {}, {}, {},tables,0.f,{},names),
        "Missing object owner manufactured a random material constant");
    for(const auto& row:fixture["vectors"]) {
        const auto result=value(row[0]);
        check(std::bit_cast<unsigned>(result.value)==row[1].get<unsigned>() &&
            result.enabled==bool(row[2].get<unsigned>()),"Salted random constant differs from unchanged BEC50");
    }
    std::vector<unsigned char> bytes;bytes.reserve(65536*9);
    for(unsigned salt=0;salt<65536;++salt) {
        const unsigned object=salt<<16;const auto result=value(object);
        for(auto word:{object,std::bit_cast<unsigned>(result.value)})
            for(unsigned n=0;n<4;++n)bytes.push_back(static_cast<unsigned char>(word>>(n*8)));
        bytes.push_back(result.enabled);
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream hex;hex<<std::hex<<std::setfill('0');
    for(auto byte:hash)hex<<std::setw(2)<<unsigned(byte);
    check(hex.str()==fixture["exhaustive_salt_results_sha256"].get<std::string>(),"Complete native salt domain differs from original execution");
}
}
int main(int argc,char** argv)try {
    check(argc==2 || argc==4,"Supply original random fixture and optional MCC map/report");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture["generic_entry"]==0x10de40u && fixture["base_owner"]==0xbec50u &&
        fixture["salt_values_verified"]==65536 && fixture["scale_bits"]==0x37800080u,
        "Random constant fixture authority differs");
    domain(fixture,{},0x0f0005aau);unsigned namespaces=1;
    if(argc==4) {
        Cache cache(argv[2]);check(cache.is_mcc(),"Random material acceptance requires actual MCC");
        const auto names=SourceWeaponMaterialNames::from_cache(cache);domain(fixture,names,0x0f0005b6u);++namespaces;
        ShaderFunctionTables tables{};
        for(const auto& tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true}))
            if(tag.name=="objects\\weapons\\multiplayer\\assault_bomb\\assault_bomb") {
                const auto functions=cache.load_object_functions(tag.datum);
                for(const auto& function:functions)if(function.input==0x0f0005b6u) {
                    check(cache.string_id(function.output)=="scrawl","Actual bomb random alias changed identity");
                    for(const auto& row:fixture["vectors"]) {
                        const auto value=source_weapon_material_input(function.output,{}, {}, {},functions,tables,
                            0.f,{},names,row[0].get<unsigned>());
                        check(value && std::bit_cast<unsigned>(value->value)==row[1].get<unsigned>() &&
                            value->enabled==bool(row[2].get<unsigned>()),"Actual MCC bomb scrawl lost object-owned random input");
                        ++aliases;
                    }
                }
            }
        check(aliases>0,"MCC random acceptance did not exercise an actual bomb scrawl function");
        check(!source_weapon_material_input(0x0f0005aau,{}, {}, {}, {},tables,0.f,{},names,0x12340000u),
            "Foreign Xbox random token became an MCC builtin");
    }
    Json report{{"passed",true},{"runtime_cache_source",argc==4?"MCC":"none"},{"namespaces",namespaces},
        {"native_random_cases",cases},{"actual_mcc_scrawl_cases",aliases},{"full_object_salt_preserved",true},
        {"missing_owner_remains_unbound",true},{"full_weapon_parity",false}};
    if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
