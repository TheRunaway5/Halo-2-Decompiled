#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_weapon_material_input.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const Material &material){
    std::vector<std::uint32_t> result(material.forward_program.words.begin(),material.forward_program.words.end());
    for(const auto &bank:material.program_constants)for(const auto &vector:bank)
        for(float value:vector)result.push_back(std::bit_cast<std::uint32_t>(value));
    return result;
}
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply original map,builtin fixture,small output report");
    std::ifstream input(argv[2]);if(!input)return 77;
    Json fixture;input>>fixture;
    check(fixture["original_xbe_sha256"]=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original material reference");
    Cache cache(argv[1]);const auto tables=cache.shader_function_tables();
    const auto tag=[&](const std::string &name,const char *group)->const Tag& {
        const auto found=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &row){return row.name==name && row.group==group;});
        check(found!=cache.tags().end(),"Original weapon material tag missing");return *found;
    };
    unsigned builtin_cases=0,alias_cases=0;Json aliases=Json::array();
    for(const auto &row:fixture["cases"]) {
        const auto result=source_weapon_material_input(row["name"],{}, {}, {}, {},*tables,0.f);
        check(result && row["recognized"]==1,"Original builtin was not recognized");
        check(std::bit_cast<std::uint32_t>(result->value)==row["value_bits"] && result->enabled==(row["enabled"].get<unsigned>()!=0),"Weapon material builtin differs from complete TU1.5 handler");
        ++builtin_cases;
    }
    unsigned original_alias_cases=0;
    for(const auto &row:fixture["alias_cases"]) {
        const auto definition=cache.load_source_world_weapon_definition(tag(row["weapon"],"weap").datum);
        check(definition.functions.at(row["function"].get<unsigned>()).output==row["name"],"Original alias metadata changed");
        SourceWeaponShaderState original_state{};
        original_state.heat=std::bit_cast<float>(row["heat_bits"].get<std::uint32_t>());
        original_state.power_fraction=std::bit_cast<float>(row["age_bits"].get<std::uint32_t>());
        std::vector<SourceWeaponMagazineState> magazines;
        if(!definition.magazines.empty())magazines.push_back({row["loaded"].get<std::int16_t>(),0});
        const auto value=source_weapon_material_input(row["name"],original_state,definition.magazines,
            magazines,definition.functions,*tables,0.f);
        check(value && std::bit_cast<std::uint32_t>(value->value)==row["value_bits"] &&
            value->enabled==(row["enabled"].get<unsigned>()!=0),"Authored weapon alias differs from complete original TU1.5 composition");
        ++original_alias_cases;
    }
    check(original_alias_cases==25,"Complete original alias cases missing");
    for(const auto *path:{"objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol",
            "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle",
            "objects\\weapons\\rifle\\brute_plasma_rifle\\brute_plasma_rifle"}) {
        const auto functions=cache.load_object_functions(tag(path,"weap").datum);
        for(const auto &function:functions)if(function.input==0x04000567u) {
            check(function.flags==0 || function.flags==4,"Actual heat alias flags changed");
            check(function.enable==0 || function.enable==0x03000577u,"Actual heat alias gate changed");
            SourceWeaponShaderState state{};state.heat=.5f;
            auto value=source_weapon_material_input(function.output,state,{}, {},functions,*tables,0.f);
            check(value.has_value(),"Actual heat alias producer was not admitted");
            const bool enabled=!function.enable;
            check(value->value==(enabled?.5f:0.f) && value->enabled==enabled,"Actual age gate was treated as an interpolated override");
            state.power_fraction=.25f;
            value=source_weapon_material_input(function.output,state,{}, {},functions,*tables,0.f);
            check(value && value->value==.5f && value->enabled,"Actual authored heat alias lost its producer");
            state.heat=0;
            value=source_weapon_material_input(function.output,state,{}, {},functions,*tables,0.f);
            check(value && value->value==0 && value->enabled==bool(function.flags&4u),"Actual always-active zero or inactive zero was lost");
            aliases.push_back({{"weapon",path},{"name",function.output},{"flags",function.flags},{"gate",function.enable}});
            alias_cases+=3;
        }
    }
    check(alias_cases>=9,"Actual authored heat aliases were not exercised");
    SourceWeaponShaderState state{};state.heat=.5f;state.power_fraction=.25f;
    const auto &rifle=tag("objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle","weap");
    const auto functions=cache.load_object_functions(rifle.datum);
    check(!source_weapon_material_input(0x0f000574u,state,{}, {},functions,*tables,0.f),"Unbound primary-trigger state was invented");
    const auto trigger=source_weapon_primary_charge(1,15,1.f,1.f/30.f);
    const auto charged=source_weapon_material_input(0x0f000574u,state,{}, {},functions,*tables,0.f,trigger);
    check(charged && charged->value==trigger.value && charged->enabled==trigger.enabled,"Actual primary-trigger value was not published");
    check(!source_weapon_material_input(0xdeadbeefu,state,{}, {},functions,*tables,0.f),"Unknown material producer was invented");
    auto material=cache.load_material(tag("objects\\weapons\\rifle\\plasma_rifle\\shaders\\plasma_rifle_gauge","shad").datum);
    check(prepare_dynamic_material(cache,material,{},true,0.f),"Actual plasma-rifle gauge pass was not admitted");
    auto legacy=material,source=material;unsigned one_queries=0;
    const ShaderInput legacy_input=[&](auto name){return source_weapon_shader_input(name,state).value_or(0.f);};
    const ShaderInput source_input=[&](auto name){
        if(!name)return 0.f;
        if(name==0x030005a6u)++one_queries;
        const auto result=source_weapon_material_input(name,state,{}, {},functions,*tables,0.f);
        return result?result->value:0.f;
    };
    check(animate_forward_material(legacy,.25,legacy_input) && animate_forward_material(source,.25,source_input),"Actual weapon gauge animation was not evaluated");
    const bool changed=words(legacy)!=words(source);
    Json report{{"passed",one_queries>0 && changed},{"original_builtin_cases",builtin_cases},
        {"complete_original_authored_alias_cases",original_alias_cases},
        {"actual_heat_alias_cases",alias_cases},{"aliases",aliases},{"actual_gauge_one_queries",one_queries},
        {"actual_gauge_program_or_constants_changed",changed},{"original_callee_results_supplied",false},
        {"rendered_Xbox_parity_verified",false}};
    const auto &pass=material.passes.at(material.forward_pass_index);
    report["gauge_reference_vectors"]={{"shader",material.source_shader_datum},
        {"template_pass",pass.template_pass},{"implementation",pass.implementation},
        {"shader_switches",material.shader_switches},{"fragment_offsets",pass.program.fragment_offsets},
        {"seconds",.25},{"heat_bits",std::bit_cast<std::uint32_t>(state.heat)},
        {"age_bits",std::bit_cast<std::uint32_t>(state.power_fraction)},
        {"initial",words(material)},{"legacy",words(legacy)},{"native",words(source)}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    check(one_queries>0,"Actual gauge did not consume its authored one input");
    check(changed,"Actual gauge pass did not respond to the corrected builtin");
    std::cout<<"PASS original TU1.5 builtins,actual heat/age gates and plasma-rifle gauge constants\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
