#include "halo2/source_object_primitive.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
using Input=std::array<std::uint32_t,10>;
using Output=std::array<std::uint32_t,5>;
Output execute(const Input& w) {
    SourceObjectPrimitiveInput in;
    in.header_flags=std::uint8_t(w[0]);in.object_flags=w[1];
    in.type=std::bit_cast<std::int8_t>(std::uint8_t(w[2]));
    in.unit_flags_10a=std::uint8_t(w[3]);in.unit_flags_348=std::uint8_t(w[4]);
    in.prop_flags_12c=w[5];
    in.primitive_sphere={{std::bit_cast<float>(w[6]),std::bit_cast<float>(w[7]),
                         std::bit_cast<float>(w[8])},std::bit_cast<float>(w[9])};
    const auto out=source_object_registration_primitive(in);
    return {std::uint32_t(std::uint16_t(out.type))|(std::uint32_t(out.flags)<<16),
        std::bit_cast<std::uint32_t>(out.center.x),std::bit_cast<std::uint32_t>(out.center.y),
        std::bit_cast<std::uint32_t>(out.center.z),std::bit_cast<std::uint32_t>(out.radius)};
}
template<std::size_t N> std::array<std::uint32_t,N> words(std::string_view line) {
    std::istringstream input{std::string(line)};std::array<std::uint32_t,N> result;
    for(auto& w:result)if(!(input>>std::hex>>w))throw FormatError("short source primitive fixture");
    std::string extra;if(input>>extra)throw FormatError("extra source primitive fixture word");
    return result;
}
void print(const Output& w) {
    for(std::size_t i=0;i<w.size();++i)std::cout<<(i?" ":"")<<std::hex<<w[i];
    std::cout<<'\n';
}
}
int main(int argc,char** argv) {try {
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::string line;while(std::getline(std::cin,line))print(execute(words<10>(line)));
        return 0;
    }
    if(argc!=2)throw FormatError("source primitive test requires its original fixture path");
    std::ifstream file(argv[1]);if(!file)throw FormatError("missing source primitive fixtures");
    std::string line;unsigned cases=0,noncollidable=0,unit=0,prop=0;
    while(std::getline(file,line)) {
        if(line.empty() || line.front()=='#')continue;
        const auto split=line.find('|');if(split==std::string::npos)throw FormatError("source primitive fixture separator missing");
        const auto in=words<10>(std::string_view(line).substr(0,split));
        const auto expected=words<5>(std::string_view(line).substr(split+1));
        if(execute(in)!=expected)throw FormatError("source B88E0 primitive words differ at case "+std::to_string(cases));
        ++cases;noncollidable+=!(in[1]&512);unit+=in[2]==0;prop+=in[2]==11;
    }
    if(!file.eof() || cases<1000 || noncollidable<100 || unit<20 || prop<415)
        throw FormatError("incomplete original primitive fixture coverage");
    std::cout<<cases<<" original B88E0/16B430 primitive fixtures passed; "
             <<cases*5<<" exact output words\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
