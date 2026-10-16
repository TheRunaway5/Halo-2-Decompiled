#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include <iostream>
#include <memory>
using namespace halo2::tu15_sound_mono_distortion;
static Word24 word(){std::uint32_t v;if(!(std::cin>>v))throw std::runtime_error("Missing typed fixture word");return Word24(v);}
static FilterCoefficients coefficients(){return {word(),word(),word(),word(),word()};}
static FilterHistory history(){return {word(),word(),word(),word()};}
int main(){
    try {
        unsigned scaling,saturation,sixteen,double_precision,coefficient_mode,input_mode,output_mode,history_mode,state_relative_mode,disjoint;
        if(!(std::cin>>scaling>>saturation>>sixteen>>double_precision>>coefficient_mode>>input_mode>>output_mode>>history_mode>>state_relative_mode>>disjoint))throw std::runtime_error("Missing explicit original modes");
        if(scaling>3||saturation>1||sixteen>1||double_precision>1||coefficient_mode>2||input_mode>2||output_mode>2||history_mode>2||state_relative_mode>2||disjoint>1)throw std::runtime_error("Invalid control field");
        const Controls controls{static_cast<ScalingMode>(scaling),bool(saturation),bool(sixteen),bool(double_precision),static_cast<AddressingMode>(coefficient_mode),static_cast<AddressingMode>(input_mode),static_cast<AddressingMode>(output_mode),static_cast<AddressingMode>(history_mode),static_cast<AddressingMode>(state_relative_mode)};
        auto parameters=std::make_unique<Parameters>(Parameters{word(),coefficients(),coefficients()});
        auto state=std::make_unique<State>();state->flags=word();state->last_dry_word=word();state->before_clip=history();state->after_clip=history();
        auto input=std::make_unique<std::array<Word24,32>>();for(auto& w:*input)w=word();
        auto output=std::make_unique<std::array<Word24,32>>();for(auto& w:*output)w=word();
        if(!disjoint && *input!=*output)throw std::runtime_error("Aliased fixture has unequal input/output words");
        auto* destination=disjoint?output.get():input.get();
        process(controls,*parameters,*state,*input,*destination);
        const auto print_coefficients=[](const FilterCoefficients& c){std::cout<<c.current_input.bits<<','<<c.previous_input.bits<<','<<c.older_input.bits<<','<<c.previous_output.bits<<','<<c.older_output.bits;};
        std::cout<<"{\"parameters\":["<<parameters->gain.bits<<',';print_coefficients(parameters->before_clip);std::cout<<',';print_coefficients(parameters->after_clip);std::cout<<"],\"native_parameters_address\":"<<reinterpret_cast<std::uintptr_t>(parameters.get())<<",\"native_state_address\":"<<reinterpret_cast<std::uintptr_t>(state.get())<<",\"native_input_address\":"<<reinterpret_cast<std::uintptr_t>(input.get())<<",\"native_output_address\":"<<reinterpret_cast<std::uintptr_t>(destination)<<",\"gain\":"<<parameters->gain.bits<<",\"flags\":"<<state->flags.bits<<",\"last_dry_word\":"<<state->last_dry_word.bits<<",\"histories\":[";
        const auto print_history=[](const FilterHistory& h){std::cout<<h.previous_input.bits<<','<<h.older_input.bits<<','<<h.previous_output.bits<<','<<h.older_output.bits;};print_history(state->before_clip);std::cout<<',';print_history(state->after_clip);std::cout<<"],\"output_words\":[";for(unsigned i=0;i<32;i++)std::cout<<(i?",":"")<<(*destination)[i].bits;std::cout<<"]}\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
