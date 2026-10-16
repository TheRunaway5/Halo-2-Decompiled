#include "halo2/native_tu15_sound_gain_fanout.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_gain_fanout;
int main(){
    const Controls standard{ScalingMode::none,false,false,false,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear};
    unsigned checked=0;
    for(unsigned test=0;test<21;test++) {
        auto control=standard;auto parameters=authored_parameters();State state{};
        std::array<Word24,33> input{};std::array<std::array<Word24,33>,5> output{};
        std::array<Output,8> routes{Output(output[0].data(),32),Output(output[1].data(),32),Output(output[2].data(),32),Output(output[3].data(),32),Output(output[4].data(),32),Output(output[4].data(),32),Output(output[4].data(),32),Output(output[4].data(),32)};
        if(test<3)control.entry_scaling=static_cast<ScalingMode>(test+1);
        if(test==3)control.arithmetic_saturation=true;
        if(test==4)control.sixteen_bit_arithmetic=true;
        if(test==5)control.double_precision_multiply=true;
        if(test==6)control.input=AddressingMode::modulo;
        if(test==7)control.destination_table=AddressingMode::reverse_carry;
        if(test==8)control.gain_sequence=AddressingMode::modulo;
        if(test==9)control.output=AddressingMode::reverse_carry;
        if(test==10)routes[0]=Output(input.data(),32);
        if(test==11)routes[0]=Output(input.data()+1,32);
        if(test==12)routes[1]=Output(output[0].data()+1,32);
        if(test==13)state.flags.bits=0x1000000;
        if(test==14)parameters.gains[7].bits=0x1000000;
        if(test==15)input[31].bits=0x1000000;
        if(test==16)output[4][31].bits=0x1000000;
        if(test==17)state.published_gain_source=static_cast<GainSource>(1);
        if(test>=18)control.state_relative=static_cast<AddressingMode>(test==20?99:test-17);
        const auto old_state=state;const auto old_parameters=parameters;const auto old_input=input;const auto old_output=output;
        bool rejected=false;try{process(control,parameters,state,Input(input.data(),32),routes);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected||state!=old_state||parameters!=old_parameters||input!=old_input||output!=old_output)throw std::runtime_error("Invalid fixture mutated caller objects");
        checked++;
    }
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<checked<<"}\n";
}
