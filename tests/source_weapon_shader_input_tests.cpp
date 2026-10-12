#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_weapon_shader_input.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint32_t> words(const Material &m){
    std::vector<std::uint32_t> out(m.forward_program.words.begin(),m.forward_program.words.end());
    for(const auto&bank:m.program_constants)for(const auto&v:bank)
        for(float c:v)out.push_back(std::bit_cast<std::uint32_t>(c));
    return out;
}
struct Result {FunctionValue input;std::vector<std::uint32_t> baseline,animated;};
Result evaluate(const Cache&c,std::span<const std::uint32_t> row){
    require(row.size()==6,"weapon shader input row size");
    const SourceWeaponShaderState state{row[1],row[2],std::bit_cast<float>(row[3])};
    const double seconds=std::bit_cast<double>(std::uint64_t(row[4])|(std::uint64_t(row[5])<<32));
    auto m=c.load_material(row[0]);
    require(prepare_dynamic_material(c,m,{},true),"actual energy blade material admission");
    require(m.forward_pass_index==0 && m.forward_vertex==1 && m.forward_blend==3,
            "actual energy blade authored pass selection");
    require(m.animation.overlays.size()==24 && m.animation.overlays[23].range==0x09000568u,
            "actual energy blade turned_on animation metadata");
    Result out;out.input=source_weapon_turned_on(state);out.baseline=words(m);
    const auto input=[&](std::uint32_t name){return source_weapon_shader_input(name,state).value_or(0.f);};
    require(animate_forward_material(m,seconds,input),"actual energy blade animation path");
    out.animated=words(m);
    // Retained shader state must be rebuilt from the authored pass each time.
    m.forward_program.words.fill(0xfeedface);
    for(auto&bank:m.program_constants)for(auto&v:bank)v.fill(-23);
    animate_forward_material(m,seconds,input);
    require(words(m)==out.animated,"energy blade animation retained stale bindings");
    return out;
}
}
int main(int argc,char**argv){try{
    require(argc==3,"weapon shader input requires maps and fixture/--probe");
    Cache cache(std::filesystem::path(argv[1])/"zanzibar.map");
    const bool probe=std::string(argv[2])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[2]);require(bool(file),"weapon shader input fixture missing");}
    auto&stream=probe?std::cin:file;std::string line;unsigned cases=0;bool on=false,off=false,responds=false;
    while(std::getline(stream,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream in(line);in>>std::hex;std::array<std::uint32_t,6> row{};
        for(auto&w:row)require(bool(in>>w),"weapon shader input fixture truncated");
        auto result=evaluate(cache,row);
        std::vector<std::uint32_t> actual{std::bit_cast<std::uint32_t>(result.input.value),result.input.enabled};
        if(probe)actual.insert(actual.end(),result.baseline.begin(),result.baseline.end());
        actual.insert(actual.end(),result.animated.begin(),result.animated.end());
        if(probe){for(auto w:actual)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
        else{
            for(auto w:actual){std::uint32_t expected;require(bool(in>>expected),"weapon shader input expected bank truncated");
                require(w==expected,"weapon shader input/original animation mismatch");}
            std::string extra;require(!(in>>extra),"weapon shader input expected bank trailing words");
            on|=result.input.enabled;off|=!result.input.enabled;
            auto opposite=row;opposite[1]=0;opposite[2]=0;opposite[3]=result.input.enabled?0x3f800000u:0;
            responds|=evaluate(cache,opposite).animated!=result.animated;
        }
        ++cases;
    }
    require(!source_weapon_shader_input(0xdeadbeef,{}),"unknown weapon shader input admitted");
    if(!probe){require(on&&off&&responds,"weapon shader input controls did not respond");
        std::cout<<cases<<" source weapon input/actual authored animation cases exact\n";}
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
