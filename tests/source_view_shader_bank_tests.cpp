#include "halo2/source_view_projection.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::array<std::uint32_t,48> result(std::istream &input) {
    unsigned foreground,flags,mode;
    std::array<std::uint32_t,48> raw;
    std::array<std::uint32_t,9> values;
    input>>foreground;
    for(auto& word:raw)input>>word;
    for(auto& word:values)input>>word;
    input>>flags>>mode;
    require(bool(input) && foreground<=1 && flags<=3,"invalid original full shader-bank input");
    const auto value=[&](unsigned i){return std::bit_cast<float>(values[i]);};
    const auto bank=std::bit_cast<SourceViewProjectionBank>(raw);
    return std::bit_cast<std::array<std::uint32_t,48>>(source_view_shader_bank(
        bank,value(0),{value(1),value(2),value(3)},
        {value(4),value(5),value(6),value(7),value(8),bool(flags&1),bool(flags&2),mode},
        bool(foreground)));
}
}
int main(int argc,char **argv){try {
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    require(probe || argc==2,"expected original full shader-bank fixture or --probe");
    std::ifstream file;
    if(!probe){file.open(argv[1]);require(bool(file),"missing original full shader-bank fixture");}
    auto& input=probe?std::cin:file;
    std::string line;unsigned cases=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);
        const auto actual=result(row);
        if(probe) {
            for(auto word:actual)std::cout<<word<<' ';
            std::cout<<'\n';
        } else {
            std::array<std::uint32_t,48> expected;
            for(auto& word:expected)row>>word;
            require(bool(row),"truncated original full shader-bank outputs");
            require(actual==expected,"original complete c0..11 bank differs");
        }
        std::string extra;require(!(row>>extra),"trailing full shader-bank input");
        ++cases;
    }
    require(cases>0,"empty original full shader-bank fixture");
    if(!probe)std::cout<<cases<<" complete original c0..11 banks / "<<cases*48<<" words exact\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
