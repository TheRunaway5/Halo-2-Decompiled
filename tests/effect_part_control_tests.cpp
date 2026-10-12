#include "halo2/effect_part_control.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("Supply original effect-part control fixture");
    std::ifstream input(argv[1],std::ios::binary);
    const auto word=[&]() {std::uint32_t v;input.read(reinterpret_cast<char*>(&v),4);if(!input)throw std::runtime_error("Truncated part control fixture");return v;};
    if(word()!=0x31504345)throw std::runtime_error("Invalid original part control fixture");
    const auto cases=word();if(cases!=2048)throw std::runtime_error("Original part control coverage missing");
    for(unsigned i=0;i<cases;++i) {
        const auto a=word(),b=word(),x=word(),y=word(),expected=word();
        const auto got=std::bit_cast<std::uint32_t>(halo2::source_effect_part_control(a,b,std::bit_cast<float>(x),std::bit_cast<float>(y)));
        if(got!=expected)throw std::runtime_error("Original part control differs at case "+std::to_string(i));
    }
    if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Extra original part control input");
    std::cout<<"PASS "<<cases<<" original effect-part A/B control words\n";return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
