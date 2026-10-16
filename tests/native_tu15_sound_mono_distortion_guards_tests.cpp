#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_mono_distortion;
int main(){
    const Controls standard{ScalingMode::none,false,false,false,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear};
    unsigned checked=0;
    const auto rejects_without_mutation=[&](auto action){
        auto p=authored_initial_parameters();State s{};std::array<Word24,33> block{};
        const auto before_p=p;const auto before_s=s;const auto before_block=block;
        bool rejected=false;try{action(p,s,block);}catch(const std::invalid_argument&){rejected=true;}
        // Compare named values and all input/output words, including the case
        // where validation sees a public Word24 mutated after construction.
        if(!rejected||p!=before_p||s!=before_s||block!=before_block)
            throw std::runtime_error("Guard failed before-mutation contract");
        checked++;
    };
    for(unsigned mode=0;mode<13;mode++) {
        auto c=standard;
        if(mode<3)c.entry_scaling=static_cast<ScalingMode>(mode+1);
        if(mode==3)c.arithmetic_saturation=true;
        if(mode==4)c.sixteen_bit_arithmetic=true;
        if(mode==5)c.double_precision_multiply=true;
        if(mode==6)c.coefficients=AddressingMode::modulo;
        if(mode==7)c.input=AddressingMode::reverse_carry;
        if(mode==8)c.output=AddressingMode::modulo;
        if(mode==9)c.history=AddressingMode::reverse_carry;
        if(mode>=10)c.state_relative=static_cast<AddressingMode>(mode==12?99:mode-9);
        rejects_without_mutation([&](Parameters& p,State& s,auto& b){process(c,p,s,std::span<const Word24,32>(b.data(),32),std::span<Word24,32>(b.data(),32));});
    }
    rejects_without_mutation([&](Parameters& p,State& s,auto& b){process(standard,p,s,std::span<const Word24,32>(b.data(),32),std::span<Word24,32>(b.data()+1,32));});
    // For invalid-word tests mutate the input before taking the invariant
    // snapshots, then check every named audio field and all words unchanged.
    for(unsigned target=0;target<5;target++) {
        auto p=authored_initial_parameters();State s{};std::array<Word24,32> input{},output{};
        if(target==0)p.gain.bits=0x1000000;
        if(target==1)s.before_clip.previous_input.bits=0x1000000;
        if(target==2)s.flags.bits=0x1000000;
        if(target==3)input[31].bits=0x1000000;
        if(target==4)output[31].bits=0x1000000;
        const auto old_p=p;const auto old_s=s;const auto old_input=input;const auto old_output=output;
        bool rejected=false;try{process(standard,p,s,input,output);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected||p!=old_p||s!=old_s||input!=old_input||output!=old_output)
            throw std::runtime_error("Invalid word changed caller state");
        checked++;
    }
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<checked<<"}\n";
}
