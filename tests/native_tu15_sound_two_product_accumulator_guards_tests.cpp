#include "halo2/native_tu15_sound_two_product_accumulator.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_two_product_accumulator;
int main(){
    unsigned checked=0;
    for(unsigned test=0;test<30;test++){
        Controls c{Word24(0x54),AddressingMode::linear,AddressingMode::linear,AddressingMode::linear,AddressingMode::linear};
        auto parameters=authored_parameters();State state{Word24(2)};
        std::array<Word24,33> primary{},secondary{},output{};std::array<ArithmeticTrace,32> trace{};
        std::span<const Word24,32> a(primary.data(),32),b(secondary.data(),32);std::span<Word24,32> out(output.data(),32);
        if(test<7){constexpr unsigned masks[]={1u<<10,1u<<11,3u<<10,1u<<17,1u<<14,1u<<12,1u<<18};c.entry_status=Word24(c.entry_status.bits|masks[test]);}
        if(test==7)c.primary=AddressingMode::modulo;
        if(test==8)c.primary=AddressingMode::reverse_carry;
        if(test==9)c.secondary=AddressingMode::modulo;
        if(test==10)c.secondary=AddressingMode::reverse_carry;
        if(test==11)c.output=AddressingMode::modulo;
        if(test==12)c.output=AddressingMode::reverse_carry;
        if(test==13)b=std::span<const Word24,32>(primary.data(),32);
        if(test==14)b=std::span<const Word24,32>(primary.data()+1,32);
        if(test==15)out=std::span<Word24,32>(primary.data(),32);
        if(test==16)out=std::span<Word24,32>(primary.data()+1,32);
        if(test==17)out=std::span<Word24,32>(secondary.data(),32);
        if(test==18)out=std::span<Word24,32>(secondary.data()+1,32);
        if(test==19)c.entry_status.bits=0x1000000;
        if(test==20)parameters.secondary_gain.bits=0x1000000;
        if(test==21)state.flags.bits=0x1000000;
        if(test==22)primary[31].bits=0x1000000;
        if(test==23)output[31].bits=0x1000000;
        if(test>=24&&test<27)c.entry_status=Word24(c.entry_status.bits|(1u<<(test==24?13:test==25?15:16)));
        if(test>=27)c.state_relative=static_cast<AddressingMode>(test==29?99:test-26);
        const auto old_p=parameters;const auto old_s=state;const auto old_primary=primary;const auto old_secondary=secondary;const auto old_output=output;const auto old_trace=trace;
        bool rejected=false;try{process(c,parameters,state,a,b,out,trace);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected||parameters!=old_p||state!=old_s||primary!=old_primary||secondary!=old_secondary||output!=old_output||trace!=old_trace)throw std::runtime_error("Invalid entry changed caller objects");
        checked++;
    }
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<checked<<"}\n";
}
