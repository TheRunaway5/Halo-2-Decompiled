#include "halo2/native_tu15_sound_word24.hpp"
#include "halo2/native_tu15_sound_gain_fanout.hpp"
#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include "halo2/native_tu15_sound_two_product_accumulator.hpp"
#include "halo2/native_tu15_sound_chorus.hpp"
#include "halo2/native_tu15_sound_crosstalk.hpp"
#include "halo2/native_tu15_sound_controller_arithmetic.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
namespace gain=halo2::tu15_sound_gain_fanout;
namespace chorus=halo2::tu15_sound_chorus;
namespace cross=halo2::tu15_sound_crosstalk;
namespace controller=halo2::tu15_sound_controller_arithmetic;
using Word=halo2::tu15_sound::Word24;
static_assert(std::is_same_v<Word,gain::Word24>);
static_assert(std::is_same_v<Word,halo2::tu15_sound_mono_distortion::Word24>);
static_assert(std::is_same_v<Word,halo2::tu15_sound_two_product_accumulator::Word24>);
static_assert(std::is_same_v<Word,chorus::Word24>);
static_assert(std::is_same_v<Word,cross::Word24>);
static_assert(std::is_same_v<Word,controller::Word24>);
static_assert(!std::is_same_v<Word,std::uint32_t>);
namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
std::int64_t signed_word(Word value){return value.bits&0x800000u?std::int64_t(value.bits)-0x1000000:std::int64_t(value.bits);}
Word limited(std::int64_t value){return Word(std::uint32_t(std::clamp(value,std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu);}
struct Graph {
    std::array<Word,32> input{},modulation{},unused_audio{},unused_modulation{},unused_output{};
    std::array<std::array<Word,32>,5> routed{};
    std::array<std::array<Word,32>,4> chorus_output{},output{};
    std::array<Word,1601> history{},expected_history{};
    gain::State gain_state{};
    std::array<chorus::State,4> chorus_state{};
    cross::State cross_state{};
    controller::State controller_state{};
    std::array<std::array<chorus::SampleTrace,32>,4> chorus_trace{};
    std::array<cross::ArithmeticTrace,64> cross_trace{};
};
}
int main(){try{
    auto graph=std::make_unique<Graph>();auto& g=*graph;
    require(reinterpret_cast<std::uintptr_t>(graph.get())>0xffffffffu,"shared owner does not exceed32-bit address space");
    gain::Parameters gp{};for(auto& value:gp.gains)value=Word(0x400000);
    const gain::Controls gc{gain::ScalingMode::none,false,false,false,gain::AddressingMode::linear,gain::AddressingMode::linear,gain::AddressingMode::linear,gain::AddressingMode::linear,gain::AddressingMode::linear};
    std::array<gain::Output,8> routes{g.routed[0],g.routed[1],g.routed[2],g.routed[3],g.routed[4],g.routed[4],g.routed[4],g.routed[4]};
    const chorus::Controls hc{Word(0),{chorus::AddressingMode::linear,chorus::AddressingMode::linear,chorus::AddressingMode::linear,chorus::AddressingMode::linear,chorus::AddressingMode::linear},chorus::AddressingMode::linear,0,400};
    const chorus::Parameters hp{Word(0),Word(0)};
    const cross::Controls xc{Word(0),cross::AddressingMode::linear,cross::AddressingMode::linear,cross::AddressingMode::linear};
    const controller::Controls cc{Word(0),controller::AddressingMode::linear,controller::AddressingMode::linear,controller::AddressingMode::linear};
    const controller::Parameters cp{Word(1),Word(0)};
    std::array<cross::Input,4> cross_inputs{g.chorus_output[0],g.chorus_output[1],g.chorus_output[2],g.chorus_output[3]};
    std::array<cross::Output,4> cross_outputs{g.output[0],g.output[1],g.output[2],g.output[3]};
    constexpr std::array<std::uint32_t,8> pattern{0,2,0xfffffe,0x7ffffe,0x800000,0x400000,0xc00000,0x123456};
    unsigned cache_checks=0,rejects=0;
    for(unsigned frame=0;frame<26;frame++){
        for(unsigned j=0;j<32;j++)g.input[j]=Word(pattern[(j+frame)%pattern.size()]);
        gain::process(gc,gp,g.gain_state,g.input,routes);
        for(const auto& route:g.routed)for(unsigned j=0;j<32;j++)require(route[j]==limited(signed_word(g.input[j])/2),"fanout shared owner differs");
        for(unsigned channel=0;channel<4;channel++){
            chorus::Bindings declared{g.routed[channel],g.modulation,g.chorus_output[channel]};
            if(frame)declared={g.unused_audio,g.unused_modulation,g.unused_output};
            if(frame==1&&channel==0){
                const auto state=g.chorus_state[0];const auto history=g.history;const auto output=g.chorus_output;const auto trace=g.chorus_trace;
                const auto saved=g.routed[0][0];g.routed[0][0].bits=0x1000000;bool threw=false;
                try{chorus::process(hc,hp,g.chorus_state[0],declared,std::span<Word,401>(g.history.data(),401),g.chorus_trace[0]);}catch(const std::invalid_argument&){threw=true;}
                require(threw&&g.history==history&&g.chorus_output==output&&g.chorus_trace==trace,"shared mutated word was not rejected before chorus mutation");
                require(g.chorus_state[0].flags==state.flags&&g.chorus_state[0].active->cursor==state.active->cursor&&g.chorus_state[0].active->bindings.audio.data()==state.active->bindings.audio.data(),"chorus guard changed cached owner");
                g.routed[0][0]=saved;++rejects;
            }
            chorus::process(hc,hp,g.chorus_state[channel],declared,std::span<Word,401>(g.history.data()+channel*400,401),g.chorus_trace[channel]);
            unsigned cursor=(frame*32)%401;
            for(unsigned j=0;j<32;j++){
                const auto& trace=g.chorus_trace[channel][j];const auto prior=g.expected_history[channel*400+cursor];
                require(trace.history_index==cursor&&trace.delayed_word==prior,"shared inclusive history read differs");
                require(trace.output_accumulator==signed_word(g.routed[channel][j])*(std::int64_t{1}<<24),"zero-gain bridge arithmetic differs");
                g.expected_history[channel*400+cursor]=g.routed[channel][j];cursor=(cursor+1)%401;
                require(trace.next_cursor==cursor&&g.chorus_output[channel][j]==g.routed[channel][j],"chorus shared output differs");
            }
            require(g.history==g.expected_history,"inclusive shared endpoint ownership differs");
            const auto& active=*g.chorus_state[channel].active;
            require(active.cursor==cursor&&active.bindings.audio.data()==g.routed[channel].data()&&active.bindings.output.data()==g.chorus_output[channel].data(),"warm cached owner identity changed");
            if(frame)++cache_checks;
        }
        if(frame==1){
            const auto state=g.cross_state;const auto output=g.output;const auto trace=g.cross_trace;
            const auto saved=g.chorus_output[1][3];g.chorus_output[1][3].bits=0x1000000;bool threw=false;
            try{(void)cross::process(xc,g.cross_state,cross_inputs,cross_outputs,g.cross_trace);}catch(const std::invalid_argument&){threw=true;}
            require(threw&&g.cross_state==state&&g.output==output&&g.cross_trace==trace,"shared mutated word was not rejected before crosstalk mutation");
            g.chorus_output[1][3]=saved;++rejects;
        }
        const auto prior=g.output;const auto result=cross::process(xc,g.cross_state,cross_inputs,cross_outputs,g.cross_trace);
        require(!result.exit_arithmetic_saturation,"crosstalk did not clear owned SM exit");
        for(unsigned channel=0;channel<4;channel++)for(unsigned j=0;j<32;j++)require(g.output[channel][j]==limited(signed_word(prior[channel][j])+signed_word(g.chorus_output[channel][j])),"cross-module retained output differs");
        for(const auto& pair:g.cross_state.pairs)require(pair.cursor==((frame+1)*32)%11,"crosstalk retained cursor differs");
        const auto emitted=controller::process(cc,cp,g.controller_state,g.output[0]);
        require(emitted.produced_count==32&&g.controller_state.counter==Word(1),"controller did not consume shared owner");
        for(unsigned j=0;j<32;j++)require(g.output[0][j]==Word(0)&&emitted.emissions[j].input_index==j,"controller shared output packing differs");
        require(std::all_of(g.unused_output.begin(),g.unused_output.end(),[](Word value){return value.bits==0;}),"warm declared replacement output was used");
    }
    require(cache_checks==100&&rejects==2,"focused owner coverage incomplete");
    std::cout<<"{\"passed\":true,\"persistent_frames\":26,\"fanout_calls\":26,\"chorus_calls\":104,\"crosstalk_calls\":26,\"controller_calls\":26,\"cached_binding_checks\":100,\"inclusive_shared_endpoint_checks\":104,\"cross_module_bad_word_rejections\":2,\"native_owners_above_4GiB\":true,\"authored_full_graph_or_physical_audio_proof\":false}\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
