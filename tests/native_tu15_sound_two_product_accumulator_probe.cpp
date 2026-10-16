#include "halo2/native_tu15_sound_two_product_accumulator.hpp"
#include <iostream>
#include <memory>
using namespace halo2::tu15_sound_two_product_accumulator;
static unsigned number(){unsigned v;if(!(std::cin>>v))throw std::runtime_error("Missing explicit fixture field");return v;}
static Word24 word(){return Word24(number());}
int main(){try{
    const auto status=word();unsigned modes[4];for(auto& mode:modes){mode=number();if(mode>2)throw std::invalid_argument("Unknown address mode");}
    const Controls controls{status,static_cast<AddressingMode>(modes[0]),static_cast<AddressingMode>(modes[1]),static_cast<AddressingMode>(modes[2]),static_cast<AddressingMode>(modes[3])};
    auto parameters=std::make_unique<Parameters>(Parameters{word(),word()});auto state=std::make_unique<State>(State{word()});
    auto primary=std::make_unique<std::array<Word24,32>>(),secondary=std::make_unique<std::array<Word24,32>>(),output=std::make_unique<std::array<Word24,32>>();
    for(auto& w:*primary)w=word();
    for(auto& w:*secondary)w=word();
    for(auto& w:*output)w=word();
    auto trace=std::make_unique<std::array<ArithmeticTrace,32>>();const auto result=process(controls,*parameters,*state,*primary,*secondary,*output,*trace);
    std::cout<<"{\"flags\":"<<state->flags.bits<<",\"gains\":["<<parameters->primary_gain.bits<<','<<parameters->secondary_gain.bits<<"],\"captured_status_operand\":"<<result.captured_status_operand.bits<<",\"exit_arithmetic_saturation\":"<<(result.exit_arithmetic_saturation?"true":"false")<<",\"output\":[";
    for(unsigned i=0;i<32;i++)std::cout<<(i?",":"")<<(*output)[i].bits;
    std::cout<<"],\"arithmetic\":[";for(unsigned i=0;i<32;i++){const auto& t=(*trace)[i];std::cout<<(i?",":"")<<'['<<t.primary_product<<','<<t.first_sum<<','<<t.secondary_product<<','<<t.final_sum<<']';}
    std::cout<<"],\"native_addresses\":["<<reinterpret_cast<std::uintptr_t>(parameters.get())<<','<<reinterpret_cast<std::uintptr_t>(state.get())<<','<<reinterpret_cast<std::uintptr_t>(primary.get())<<','<<reinterpret_cast<std::uintptr_t>(secondary.get())<<','<<reinterpret_cast<std::uintptr_t>(output.get())<<','<<reinterpret_cast<std::uintptr_t>(trace.get())<<"]}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
