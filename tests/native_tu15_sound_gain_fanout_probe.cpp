#include "halo2/native_tu15_sound_gain_fanout.hpp"
#include <iostream>
#include <memory>
using namespace halo2::tu15_sound_gain_fanout;
static unsigned number(){unsigned v;if(!(std::cin>>v))throw std::runtime_error("Missing fixture field");return v;}
static Word24 word(){return Word24(number());}
int main(){
    try {
        const auto scaling=number(),sm=number(),sa=number(),dm=number();
        const auto input_mode=number(),table_mode=number(),gain_mode=number(),output_mode=number(),state_relative_mode=number();
        if(scaling>3||sm>1||sa>1||dm>1||input_mode>2||table_mode>2||gain_mode>2||output_mode>2||state_relative_mode>2)throw std::invalid_argument("Invalid controls");
        const Controls controls{static_cast<ScalingMode>(scaling),bool(sm),bool(sa),bool(dm),static_cast<AddressingMode>(input_mode),static_cast<AddressingMode>(table_mode),static_cast<AddressingMode>(gain_mode),static_cast<AddressingMode>(output_mode),static_cast<AddressingMode>(state_relative_mode)};
        auto parameters=std::make_unique<Parameters>();for(auto& g:parameters->gains)g=word();
        auto state=std::make_unique<State>();state->flags=word();const auto selector=number();if(selector>1)throw std::invalid_argument("Unknown gain source");if(selector)state->published_gain_source=GainSource::sequence_begin;
        auto input=std::make_unique<std::array<Word24,32>>();for(auto& w:*input)w=word();
        auto output=std::make_unique<std::array<std::array<Word24,32>,5>>();for(auto& block:*output)for(auto& w:block)w=word();
        const std::array<Output,8> routes{(*output)[0],(*output)[1],(*output)[2],(*output)[3],(*output)[4],(*output)[4],(*output)[4],(*output)[4]};
        process(controls,*parameters,*state,*input,routes);
        std::cout<<"{\"flags\":"<<state->flags.bits<<",\"published_gain_source\":"<<int(state->published_gain_source.has_value())<<",\"gains\":[";
        for(unsigned i=0;i<8;i++)std::cout<<(i?",":"")<<parameters->gains[i].bits;
        std::cout<<"],\"destination_words\":[";for(unsigned i=0;i<160;i++)std::cout<<(i?",":"")<<(*output)[i/32][i%32].bits;
        std::cout<<"],\"native_addresses\":["<<reinterpret_cast<std::uintptr_t>(parameters.get())<<','<<reinterpret_cast<std::uintptr_t>(state.get())<<','<<reinterpret_cast<std::uintptr_t>(input.get())<<','<<reinterpret_cast<std::uintptr_t>(output.get())<<"]}\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
