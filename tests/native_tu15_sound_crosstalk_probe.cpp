#include "halo2/native_tu15_sound_crosstalk.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace halo2::tu15_sound_crosstalk;
static unsigned number(){unsigned value;if(!(std::cin>>value))throw std::runtime_error("Missing explicit numerical input");return value;}
static Word24 word(){return Word24(number());}
int main(){try{
    const auto status=word();const unsigned state_relative=number(),first=number(),second=number();
    if(state_relative>2||first>2||second>2)throw std::invalid_argument("Unknown address mode");
    const Controls control{status,static_cast<AddressingMode>(state_relative),static_cast<AddressingMode>(first),static_cast<AddressingMode>(second)};
    auto state=std::make_unique<State>();state->flags=word();
    for(auto& coefficient:state->coefficients)coefficient=word();
    for(auto& pair:state->pairs) {
        pair.cursor=number();
        for(unsigned channel=0;channel<2;channel++) {
            for(auto& history:pair.histories[channel])history=word();
            pair.previous[channel]=word();
        }
    }
    auto inputs=std::make_unique<std::array<std::array<Word24,32>,4>>();
    auto outputs=std::make_unique<std::array<std::array<Word24,32>,4>>();
    for(auto& input:*inputs)for(auto& value:input)value=word();
    for(auto& output:*outputs)for(auto& value:output)value=word();
    auto trace=std::make_unique<std::array<ArithmeticTrace,64>>();
    const std::array<Input,4> input_views{(*inputs)[0],(*inputs)[1],(*inputs)[2],(*inputs)[3]};
    const std::array<Output,4> output_views{(*outputs)[0],(*outputs)[1],(*outputs)[2],(*outputs)[3]};
    const auto result=process(control,*state,input_views,output_views,*trace);
    std::cout<<"{\"flags\":"<<state->flags.bits<<",\"coefficients\":["<<state->coefficients[0].bits<<','<<state->coefficients[1].bits<<"],\"pairs\":[";
    for(unsigned index=0;index<2;index++) {
        const auto& pair=state->pairs[index];std::cout<<(index?",":"")<<"{\"cursor\":"<<pair.cursor<<",\"words\":[";
        for(unsigned channel=0;channel<2;channel++) {
            for(unsigned j=0;j<11;j++)std::cout<<(channel||j?",":"")<<pair.histories[channel][j].bits;
            std::cout<<','<<pair.previous[channel].bits;
        }
        std::cout<<"]}";
    }
    std::cout<<"],\"outputs\":[";
    for(unsigned channel=0;channel<4;channel++)for(unsigned i=0;i<32;i++)std::cout<<(channel||i?",":"")<<(*outputs)[channel][i].bits;
    std::cout<<"],\"arithmetic\":[";
    for(unsigned i=0;i<64;i++) {
        std::cout<<(i?",":"")<<'[';
        for(unsigned j=0;j<8;j++)std::cout<<(j?",":"")<<(*trace)[i].values[j];
        std::cout<<']';
    }
    std::cout<<"],\"exit_arithmetic_saturation\":"<<(result.exit_arithmetic_saturation?"true":"false")<<",\"native_addresses\":["<<reinterpret_cast<std::uintptr_t>(state.get())<<','<<reinterpret_cast<std::uintptr_t>(inputs.get())<<','<<reinterpret_cast<std::uintptr_t>(outputs.get())<<','<<reinterpret_cast<std::uintptr_t>(trace.get())<<"]}\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
