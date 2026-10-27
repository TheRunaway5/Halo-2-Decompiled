#include "halo2/pstencil/geometry.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool b,const char* why){if(!b)throw FormatError(why);}
float real(std::istream& input) {
    std::string text;require(bool(input>>text)&&text.size()==8,"invalid original float word");
    std::size_t consumed{};const auto word=std::stoul(text,&consumed,16);require(consumed==8,"invalid original float word");
    return std::bit_cast<float>(std::uint32_t(word));
}
}
int main(int argc,char** argv){try {
    require(argc==3,"provide the current Xbox cache and retained original world-position fixture");
    Cache cache(argv[1]);const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& tag) {
        return tag.group=="vrtx" && tag.name.ends_with("\\lightaccum_sphere_diffuse");
    });require(tag!=cache.tags().end(),"original point-light vertex family absent");
    struct Bank {unsigned classification{};std::array<ShaderVector,192> constants{};unsigned count{};};
    std::map<unsigned,Bank> banks;std::set<unsigned> classes;std::ifstream input(argv[2]);std::string line;unsigned samples{},words{},negative{};
    require(bool(input),"original world-position fixture missing");
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;std::istringstream row(line);char kind;row>>kind;
        if(kind=='N' || kind=='S')continue;
        unsigned event,index;row>>event>>index;
        if(kind=='B')banks[event].classification=index;
        else if(kind=='C') {
            auto& bank=banks.at(event);require(index==bank.count && index<192,"incomplete original constant bank");
            for(auto& value:bank.constants[index])value=real(row);++bank.count;
        } else if(kind=='V') {
            const auto& bank=banks.at(event);require(bank.count==192,"original constants absent");
            VertexProgramState state;state.constants=bank.constants;
            for(auto& attribute:state.attributes)for(auto& value:attribute)value=real(row);
            ShaderVector expected;for(auto& value:expected)value=real(row);
            const auto program=decode_vertex_program(cache.load_vertex_shader_code(tag->datum,bank.classification));
            pstencil::execute_native_position_prefix(program,state);
            for(unsigned axis=0;axis<3;++axis) {
                require(std::bit_cast<std::uint32_t>(state.outputs[9][axis])==std::bit_cast<std::uint32_t>(expected[axis]),
                    "pStencil position prefix differs from retained original posed world position");++words;
            }
            negative+=state.attributes[0]!=expected;classes.insert(bank.classification);++samples;
        }
    }
    require(samples==14 && classes.size()==4 && words==42 && negative==14,"original position coverage or negative controls changed");
    std::cout<<"PASS: 14 actual original samples across four captured vertex classes; 42 exact world-position words; 14 local-position negative controls\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
