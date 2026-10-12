#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_shadow_filter.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
float word(std::istream &input) {
    std::string text;check(bool(input>>text)&&text.size()==8,"invalid shadow source word");
    std::size_t used;auto value=std::stoul(text,&used,16);
    check(used==8&&value<=0xfffffffful,"shadow source word overflow");
    return std::bit_cast<float>(std::uint32_t(value));
}
template<class F>void rejects(F &&fn){bool caught=false;try{fn();}catch(const FormatError&){caught=true;}check(caught,"invalid shadow plan was admitted");}
struct Bank {std::array<ShaderVector,192> constants;ShaderVector conversion;bool receiver{};};
constexpr unsigned outputs[]{0,3,4,9,10,11,12};
}
int main(){try {
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    const auto path=root/"assets/disc/maps/zanzibar.map";
    if(!std::filesystem::exists(path))return 77;
    std::ifstream filter_fixture(root/"tests/fixtures/retail_shadow_filter.bin",std::ios::binary);
    std::array<std::uint32_t,2> filter_header{};
    check(bool(filter_fixture.read(reinterpret_cast<char*>(filter_header.data()),sizeof(filter_header)))&&filter_header==std::array<std::uint32_t,2>{0x46533248,1},"original filter fixture missing");
    for(const auto &pass:source_shadow_filter_passes()) {
        std::array<std::uint32_t,2> targets{};std::array<std::uint32_t,60> program{};std::array<ShaderVector,4> taps{};
        check(bool(filter_fixture.read(reinterpret_cast<char*>(targets.data()),sizeof(targets))),"filter targets truncated");
        check(bool(filter_fixture.read(reinterpret_cast<char*>(program.data()),sizeof(program))),"filter program truncated");
        check(bool(filter_fixture.read(reinterpret_cast<char*>(taps.data()),sizeof(taps))),"filter taps truncated");
        check(targets==std::array<std::uint32_t,2>{pass.source_target,pass.destination_target}&&program==pass.program.words&&taps==pass.taps,"original222A0 filter producer differs");
    }
    Cache cache(path);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
        return t.group=="shad"&&t.name=="objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod";
    });check(tag!=cache.tags().end(),"authored shadow material missing");
    const auto material=cache.load_material(tag->datum);
    auto casters=prepare_shadow_material(cache,material,SourceShadowPhase::Generate,true,false,.75f);
    auto receivers=prepare_shadow_material(cache,material,SourceShadowPhase::Apply,true,false,.75f);
    check(casters.size()==1&&receivers.size()==1,"source shadow pass cardinality differs");
    const std::array<SourceShadowPlanMaterial,2> resident{{{6,&casters[0]},{7,&receivers[0]}}};
    std::array<Vertex,3> generation{},application{};
    for(auto &v:generation)v.material=6;
    for(auto &v:application)v.material=7;
    auto caster=source_shadow_plan_draw(6,casters[0],generation,{0,true});
    auto receiver=source_shadow_plan_draw(7,receivers[0],application,{0,true});
    SourceShadowPlanRoot owner;owner.casters={&caster,1};owner.receivers={&receiver,1};
    SourceShadowPlan plan;plan.roots={&owner,1};
    auto stats=validate_source_shadow_plan(plan,resident);
    check(stats.roots==1&&stats.caster_draws==1&&stats.receiver_draws==1&&stats.vertices==6,"shadow plan order/count differs");
    rejects([&]{auto bad=resident;bad[1].material_slot=6;validate_source_shadow_plan(plan,bad);});
    rejects([&]{auto bad=resident;bad[1].material_slot=8;validate_source_shadow_plan(plan,bad);});
    rejects([&]{caster.raster.reset();try{validate_source_shadow_plan(plan,resident);}catch(...){caster.raster=SourceShadowRasterInput{0,true};throw;}});
    rejects([&]{caster.raster->cull=123;try{validate_source_shadow_plan(plan,resident);}catch(...){caster.raster->cull=0;throw;}});
    rejects([&]{const auto saved=caster.program.words[54];caster.program.words[54]=2;try{validate_source_shadow_plan(plan,resident);}catch(...){caster.program.words[54]=saved;throw;}});
    rejects([&]{generation[1].material=7;try{validate_source_shadow_plan(plan,resident);}catch(...){generation[1].material=6;throw;}});
    rejects([&]{owner.uniforms.texture_width=256;try{validate_source_shadow_plan(plan,resident);}catch(...){owner.uniforms.texture_width=128;throw;}});
    rejects([&]{auto bad=receivers[0];bad.external_targets={0,10,0,0};validate_source_shadow_plan_material(bad,SourceShadowPhase::Apply);});
    rejects([&]{auto bad=receivers[0];bad.material.forward_raster.depth_enable=0;validate_source_shadow_plan_material(bad,SourceShadowPhase::Apply);});
    rejects([&]{auto bad=receivers[0];bad.material.vertex_declaration.reset();validate_source_shadow_plan_material(bad,SourceShadowPhase::Apply);});
    rejects([&]{auto bad=receivers[0];bad.material.vertex_declaration->semantics={0,3};validate_source_shadow_plan_material(bad,SourceShadowPhase::Apply);});
    rejects([&]{auto bad=receivers[0];bad.material.world_vertex_input_transform=SourceWorldVertexInputTransform::NormalizeSkinNormal;validate_source_shadow_plan_material(bad,SourceShadowPhase::Apply);});
    auto absent=owner;absent.receivers={};SourceShadowPlan empty;empty.roots={&absent,1};
    check(validate_source_shadow_plan(empty,resident).roots==0,"single missing group was submitted");
    std::ifstream file(root/"tests/fixtures/retail_shadow_vertex.tsv");check(bool(file),"original shadow fixture missing");
    std::map<unsigned,Bank> banks;std::string line;unsigned words=0,samples=0;
    const auto vp=decode_vertex_program(receivers[0].material.vertex_program);
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream input(line);char kind;unsigned event,index;check(bool(input>>kind>>event>>index),"truncated source fixture");
        if(kind=='B') {
            unsigned ordinal;check(bool(input>>ordinal),"missing source class");
            banks[event].receiver=index!=0;
            for(unsigned a=0;a<3;++a)banks[event].conversion[a]=word(input);
        } else if(kind=='C') {
            check(index<192,"source register outside192");
            for(auto &v:banks[event].constants[index])v=word(input);
        } else if(kind=='V' && banks.at(event).receiver) {
            const auto &bank=banks.at(event);VertexProgramState state;
            for(auto &attribute:state.attributes)for(auto &v:attribute)v=word(input);
            std::array<ShaderVector,7> expected;
            for(auto &out:expected)for(auto &v:out)v=word(input);
            std::copy_n(bank.constants.begin(),12,plan.camera.begin());
            std::copy_n(bank.constants.begin()+31,6,owner.uniforms.projection.begin());
            std::copy_n(bank.constants.begin()+40,9,owner.uniforms.constants.begin());
            receiver.constants[0]=state.attributes;
            std::copy_n(bank.constants.begin()+18,16,receiver.constants[1].begin());
            std::fill(receiver.constants[1].begin()+13,receiver.constants[1].end(),ShaderVector{});
            receiver.program.words[43]=0x76543210u; // Completed caller binding must survive.
            const auto bound=source_shadow_plan_bindings(plan,owner,receiver,resident[1]);
            check(bound.program.words[43]==0x76543210u&&bound.program.fragment_offsets==receiver.program.fragment_offsets,"shadow hook overwrote completed pixel binding");
            check(bound.external_targets[0]==10&&bound.globals.first_constant==34&&bound.globals.constants.size()==15&&!bound.globals.prt_inputs,"shadow GPU bank metadata differs");
            const auto merged=source_shadow_plan_vertex_bank(bound);
            const auto packed=pack_vertex_program_bank(merged);
            check(packed.range==std::array<std::uint32_t,4>{0,49,0,1},"shadow camera override bank metadata differs");
            check(std::equal(bound.camera.begin(),bound.camera.end(),packed.constants.begin())&&
                  std::equal(bound.constants[1].begin(),bound.constants[1].end(),packed.constants.begin()+18)&&
                  std::equal(bound.globals.constants.begin(),bound.globals.constants.end(),packed.constants.begin()+34),"shadow merged bank changed actual completed constants");
            check(std::all_of(packed.constants.begin()+12,packed.constants.begin()+18,[](const auto &v){return v==ShaderVector{};})&&
                  std::all_of(packed.constants.begin()+37,packed.constants.begin()+40,[](const auto &v){return v==ShaderVector{};}),"unconsumed shadow storage gap was inferred");
            rejects([&]{auto bad=bound;bad.globals.first_constant=35;source_shadow_plan_vertex_bank(bad);});
            rejects([&]{auto bad=bound;bad.globals.constants.pop_back();source_shadow_plan_vertex_bank(bad);});
            rejects([&]{auto bad=bound;bad.globals.prt_inputs=true;source_shadow_plan_vertex_bank(bad);});
            rejects([&]{auto bad=bound;bad.globals.camera_inputs=true;source_shadow_plan_vertex_bank(bad);});
            std::copy(bound.camera.begin(),bound.camera.end(),state.constants.begin());
            std::copy(bound.constants[1].begin(),bound.constants[1].end(),state.constants.begin()+18);
            std::copy(bound.globals.constants.begin(),bound.globals.constants.end(),state.constants.begin()+34);
            execute_vertex_program(vp,state);
            for(unsigned o=0;o<7;++o) {
                auto actual=state.outputs[outputs[o]];
                if(o==0) {
                    for(unsigned a=0;a<2;++a)actual[a]=((std::trunc(actual[a]*16.f)/16.f)*2.f-bank.conversion[a])/bank.conversion[a];
                    actual[2]/=bank.conversion[2];
                    for(unsigned a=0;a<3;++a)actual[a]*=actual[3];
                }
                for(unsigned a=0;a<4;++a) {
                    check(std::bit_cast<std::uint32_t>(actual[a])==std::bit_cast<std::uint32_t>(expected[o][a]),"shadow plan changed a literal original receiver output");
                    ++words;
                }
            }
            auto retained=bound;
            owner.uniforms.constants[0][0]+=1;
            const auto changed=source_shadow_plan_bindings(plan,owner,receiver,resident[1]);
            check(retained.globals.constants[6]!=changed.globals.constants[6],"root constant snapshots alias each other");
            ++samples;
        }
    }
    check(samples==10&&words==280,"original receiver plan coverage differs");
    std::cout<<words<<" original receiver output words bit-exact through explicit shadow plan; resident/order/lifetime rejection controls passed\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
