#include "halo2/native_tu15_sound_chorus.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace halo2::tu15_sound_chorus;
static unsigned number(){unsigned v;if(!(std::cin>>v))throw std::runtime_error("Missing explicit fixture field");return v;}
static Word24 word(){return Word24(number());}
int main(){try{
    Controls c{word(),{},AddressingMode::linear,number(),number()};
    // Header order is status,livebegin,livespan,five data address modes,state-relative M5,gain,depth,
    // flags,cursor,cachedend,declared_changed, then audio/mod/output/history.
    for(auto& mode:c.addressing){auto v=number();if(v>2)throw std::invalid_argument("Unknown mode");mode=static_cast<AddressingMode>(v);}
    {auto mode=number();if(mode>2)throw std::invalid_argument("Unknown state-relative mode");c.state_relative=static_cast<AddressingMode>(mode);}
    auto p=std::make_unique<Parameters>(Parameters{word(),word()});auto flags=word();auto cursor=number(),end=number(),different=number();if(different>1)throw std::invalid_argument("Unknown declaration fixture");
    auto audio=std::make_unique<std::array<Word24,32>>(),modulation=std::make_unique<std::array<Word24,32>>(),output=std::make_unique<std::array<Word24,32>>();auto history=std::make_unique<std::array<Word24,401>>();
    for(auto&w:*audio)w=word();
    for(auto&w:*modulation)w=word();
    for(auto&w:*output)w=word();
    for(auto&w:*history)w=word();
    Bindings actual{*audio,*modulation,*output};std::array<Word24,32> alternate_audio{},alternate_modulation{},alternate_output{};alternate_audio.fill(Word24(0x11));alternate_modulation.fill(Word24(0x22));alternate_output.fill(Word24(0x33));
    Bindings declared=different?Bindings{alternate_audio,alternate_modulation,alternate_output}:actual;
    auto s=std::make_unique<State>(State{flags,std::nullopt});if(flags.bits&1u)s->active=ActiveState{actual,cursor,end};
    auto trace=std::make_unique<std::array<SampleTrace,32>>();process(c,*p,*s,declared,*history,*trace);
    bool ignored=true;for(auto w:alternate_output)ignored&=w.bits==0x33;
    std::cout<<"{\"flags\":"<<s->flags.bits<<",\"gain\":"<<p->gain.bits<<",\"depth\":"<<p->depth.bits<<",\"cursor\":"<<s->active->cursor<<",\"cached_end\":"<<s->active->cached_end<<",\"warm_alternate_output_unchanged\":"<<(ignored?"true":"false")<<",\"output\":[";
    for(unsigned i=0;i<32;i++)std::cout<<(i?",":"")<<(*output)[i].bits;
    std::cout<<"],\"history\":[";
    for(unsigned i=0;i<401;i++)std::cout<<(i?",":"")<<(*history)[i].bits;
    std::cout<<"],\"trace\":[";
    for(unsigned i=0;i<32;i++){auto t=(*trace)[i];std::cout<<(i?",":"")<<'['<<t.delay_before_correction<<','<<t.delay_after_correction<<','<<t.history_index<<','<<t.delayed_word.bits<<','<<t.output_accumulator<<','<<t.next_cursor<<']';}
    std::cout<<"],\"native_addresses\":["<<reinterpret_cast<std::uintptr_t>(p.get())<<','<<reinterpret_cast<std::uintptr_t>(s.get())<<','<<reinterpret_cast<std::uintptr_t>(audio.get())<<','<<reinterpret_cast<std::uintptr_t>(modulation.get())<<','<<reinterpret_cast<std::uintptr_t>(output.get())<<','<<reinterpret_cast<std::uintptr_t>(history.get())<<','<<reinterpret_cast<std::uintptr_t>(trace.get())<<"]}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
