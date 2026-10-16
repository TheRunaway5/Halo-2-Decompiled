#include "halo2/native_tu15_sound_controller_arithmetic.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_controller_arithmetic;
int main(){try{
    unsigned rejected=0;
    auto test=[&](auto alter){auto p=authored_parameters();State s{Word24(0xa0),Word24(3)};std::array<Word24,32> storage;for(unsigned n=0;n<32;n++)storage[n]=Word24(n+1);
        Controls c{Word24(0),AddressingMode::linear,AddressingMode::linear,AddressingMode::linear};alter(c,p,s,storage);const auto before_s=s;const auto before_p=p;const auto before_storage=storage;bool caught=false;
        try{(void)process(c,p,s,storage);}catch(const std::invalid_argument&){caught=true;}
        if(!caught||s!=before_s||p!=before_p||storage!=before_storage)throw std::runtime_error("Invalid controller request changed caller data");
        ++rejected;};
    for(unsigned bit=10;bit<24;bit++)test([&](auto& c,auto&,auto&,auto&){c.entry_status.bits=1u<<bit;});
    test([](auto& c,auto&,auto&,auto&){c.entry_status.bits=0x1000000;});
    for(unsigned slot=0;slot<3;slot++)for(auto value:{AddressingMode::modulo,AddressingMode::reverse_carry,static_cast<AddressingMode>(99)})test([&](auto& c,auto&,auto&,auto&){if(slot==0)c.storage_read=value;if(slot==1)c.storage_write=value;if(slot==2)c.state_relative=value;});
    test([](auto&,auto& p,auto&,auto&){p.threshold.bits=0x1000000;});test([](auto&,auto& p,auto&,auto&){p.gain.bits=0x1000000;});
    test([](auto&,auto&,auto& s,auto&){s.flags.bits=0x1000000;});test([](auto&,auto&,auto& s,auto&){s.counter.bits=0x1000000;});
    test([](auto&,auto&,auto&,auto& b){b.front().bits=0x1000000;});test([](auto&,auto&,auto&,auto& b){b.back().bits=0x1000000;});
    bool word_rejected=false;try{(void)Word24(0x1000000);}catch(const std::invalid_argument&){word_rejected=true;}if(!word_rejected)throw std::runtime_error("Invalid typed word accepted");++rejected;
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<rejected<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
