#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void rejects(F action,const char* message){bool caught=false;try{action();}catch(const FormatError&){caught=true;}require(caught,message);}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply actual MCC intro and independent raw residency census");
    if(!*argv[1])return 77;
    nlohmann::json fixture;std::ifstream(argv[2])>>fixture;
    require(fixture.at("schema")=="H2_MCC_INTRO_VEHICLE_HUD_RESIDENCY_V1" &&
        fixture.at("source_map_sha256")=="f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8","Changed raw MCC HUD authority");
    Cache cache(argv[1]);require(cache.is_mcc(),"HUD residency must use imported MCC");
    unsigned pruned{},resident{};
    for(const auto& row:fixture.at("rows")) {
        const auto weapon_id=row.at("weapon").at("datum").get<std::uint32_t>();
        const auto hud_id=row.at("hud").at("datum").get<std::uint32_t>();
        const auto weapon=cache.query_tag({.handle=TagHandle{weapon_id},.group=tag_groups::weapon,.require_address=true});
        const auto hud=cache.query_tag({.handle=TagHandle{hud_id},.group=TagGroup{"nhdt"}});
        require(weapon && hud && weapon->name==row.at("weapon").at("name").get<std::string>() &&
            hud->name==row.at("hud").at("name").get<std::string>(),"HUD residency lost actual full typed owners");
        require(Bytes(cache.tag_payload(weapon->handle())).u32(0x2b4)==hud_id,"Authored vehicle weapon HUD reference changed");
        const bool expected=row.at("resident");
        require(cache.resident_hud_definition(hud_id)==expected,"Actual MCC HUD residency differs from raw map census");
        if(expected)++resident;else {++pruned;require(hud->size==0,"Pruned cinematic HUD acquired supplied content");}
        rejects([&]{cache.resident_hud_definition(hud_id^0x10000u);},"Wrong HUD salt admitted");
        rejects([&]{cache.resident_hud_definition(weapon_id);},"Wrong HUD group admitted");
    }
    require(pruned==1 && resident==2 && !cache.resident_hud_definition(null_datum),"Incomplete cinematic HUD census");
    std::cout<<"PASS actual pruned/resident MCC weapon HUD ownership and salt/group negatives\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
