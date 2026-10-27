#include "halo2/cache.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/damage_feedback_names.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
ObjectFunction identity(std::uint32_t output,std::uint32_t input){
    ObjectFunction result;result.output=output;result.input=input;result.curve.resize(20);
    result.curve[10]=std::byte{0x80};result.curve[11]=std::byte{0x3f};return result;
}
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply actual MCC map, original map and bounded JSON report");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[1]);check(cache.is_mcc(),"Damage namespace regression requires actual MCC");
    const auto names=DamageFeedbackNames::from_cache(cache);
    const auto elite=cache.query_tag({.group=tag_groups::biped,.name="objects\\characters\\elite\\elite_mp"});
    check(bool(elite),"Actual MCC Elite multiplayer definition is missing");
    const auto functions=cache.load_object_functions(elite->datum);
    const auto burnt=std::find_if(functions.begin(),functions.end(),[&](const auto &function){
        return cache.string_id(function.output)=="elite_burnt";
    });
    check(burnt!=functions.end() && cache.string_id(burnt->input)=="one","Actual Elite burn input differs");
    const auto tables=cache.shader_function_tables();DamageFeedbackState feedback;
    nlohmann::json rows=nlohmann::json::array();bool passed=true;
    for(bool dead:{false,true}) {
        const auto value=damage_feedback_shader_input(feedback,dead?0.f:1.f,dead?0.f:1.f,dead,
            functions,*tables,burnt->output,0,names);
        rows.push_back({{"dead",dead},{"input",burnt->input},{"output",burnt->output},{"value",value}});
        passed&=std::bit_cast<std::uint32_t>(value)==0x3f800000u;
    }
    std::cout<<rows.dump()<<'\n';check(passed,"Actual MCC healthy/dead Elite lost its authored constant-one burn input");
    constexpr std::array<std::uint32_t,8> original{
        0x030005a6u,0x040005a7u,0x0500055bu,0x0d000558u,
        0x0f000559u,0x13000556u,0x15000557u,0x1800055au};
    constexpr std::array<std::uint32_t,8> mcc{
        0x030005b2u,0x040005b3u,0x05000567u,0x0d000564u,
        0x0f000565u,0x13000562u,0x15000563u,0x18000566u};
    Cache original_cache(argv[2]);const auto original_names=DamageFeedbackNames::from_cache(original_cache);
    feedback.body.visible=.25f;feedback.shield.visible=.75f;
    unsigned builtin_cases{},alias_cases{};
    for(bool dead:{false,true})for(unsigned i=0;i<mcc.size();++i){
        check(names.builtin(mcc[i])==original[i] && original_names.builtin(original[i])==original[i],
            "Damage feedback primary-table pair differs");
        const auto expected=damage_feedback_shader_input(feedback,.375f,1.5f,dead,{},*tables,original[i],0);
        const auto actual=damage_feedback_shader_input(feedback,.375f,1.5f,dead,{},*tables,mcc[i],0,names);
        const auto retained=damage_feedback_shader_input(feedback,.375f,1.5f,dead,{},*tables,original[i],0,original_names);
        check(same(actual,expected) && same(retained,expected),"Mapped vitality/recent-damage input differs from original producer");
        ++builtin_cases;
    }
    const auto evaluate=[&](std::uint32_t name,std::span<const ObjectFunction> aliases){
        return damage_feedback_shader_input(feedback,.375f,1.5f,false,aliases,*tables,name,0,names);
    };
    std::array<ObjectFunction,2> aliases{identity(0x0800fe01u,mcc[3]),identity(original[0],mcc[6])};
    aliases[0].enable=mcc[1];check(evaluate(aliases[0].output,aliases)==0,"Inactive MCC zero gate became an override");++alias_cases;
    aliases[0].enable=mcc[0];check(same(evaluate(aliases[0].output,aliases),.375f),"Active MCC one gate lost vitality");++alias_cases;
    for(auto token:original){
        check(!names.builtin(token),"Original token was admitted in the MCC builtin namespace");
        aliases[1].output=token;aliases[0].scale=token;
        check(same(evaluate(token,aliases),.75f) && same(evaluate(aliases[0].output,aliases),.28125f),
            "MCC authored output/scale collided with an original builtin token");++alias_cases;
    }
    check(!names.builtin(0x040005b2u) && !names.builtin(0xdeadbeefu),"Length-mismatched or unknown builtin was normalized");
    std::ofstream(argv[3])<<nlohmann::json{{"passed",true},{"cases",rows},{"builtin_cases",builtin_cases},
        {"alias_gate_scale_controls",alias_cases},{"authored_ids_preserved",true}}.dump(2)<<'\n';
    std::cout<<"PASS actual MCC Elite healthy/dead exports, "<<builtin_cases<<" paired builtin cases and "<<alias_cases<<" alias/gate/scale controls\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
