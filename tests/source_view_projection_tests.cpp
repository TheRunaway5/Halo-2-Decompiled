#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
std::array<std::uint32_t,76> result(const std::array<std::uint32_t,32>& words,bool first_person) {
    const auto value=[&](unsigned i){return std::bit_cast<float>(words[i]);};
    SourceViewProjectionInput in;
    in.position={value(0),value(1),value(2)};
    in.forward={value(3),value(4),value(5)};in.up={value(6),value(7),value(8)};
    in.near_distance=value(9);in.far_distance=value(10);
    for(unsigned i=0;i<4;++i)in.oblique_near_plane[i]=value(11+i);
    for(unsigned i=0;i<16;++i)in.projection_program[i]=value(15+i);
    const auto bank=source_view_projection(in);
    const auto constants=source_view_shader_constants(bank,value(31),first_person);
    const auto b=std::bit_cast<std::array<std::uint32_t,48>>(bank);
    const auto c=std::bit_cast<std::array<std::uint32_t,28>>(constants);
    std::array<std::uint32_t,76> output;
    std::copy(b.begin(),b.end(),output.begin());std::copy(c.begin(),c.end(),output.begin()+48);
    return output;
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open("tests/fixtures/retail_source_view_projection.tsv");require(bool(file),"missing original view/projection fixture");}
    auto& input=probe?std::cin:file;unsigned count=0,first_person;
    while(input>>first_person) {
        require(first_person<=1,"invalid original view first-person flag");
        std::array<std::uint32_t,32> words;for(auto& w:words)input>>w;
        require(bool(input),"truncated original view/projection input");
        const auto actual=result(words,bool(first_person));
        if(probe){for(unsigned i=0;i<actual.size();++i)std::cout<<(i?" ":"")<<actual[i];std::cout<<'\n';}
        else {std::array<std::uint32_t,76> expected;for(auto& w:expected)input>>w;
            require(bool(input),"truncated original view/projection output");
            require(actual==expected,"original render view/projection bank or shader constants differ");}
        ++count;
    }
    require(input.eof(),"invalid original view/projection fixture");
    if(!probe){require(count==466,"incomplete original view/projection fixture");std::cout<<count<<" original render view/projection and shader bank fixtures passed\n";}
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
