#include "halo2/native_tu15_sound_chorus.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace halo2::tu15_sound_chorus;
static unsigned number(){unsigned v;if(!(std::cin>>v))throw std::runtime_error("Missing coupled numeric fixture field");return v;}
static Word24 word(){return Word24(number());}
struct Fixture {Controls c{};Parameters p{};Word24 flags;unsigned cursor{},end{},different{};std::array<Word24,32>audio{},modulation{},output{};std::array<Word24,401>history{};};
static void emit(const Fixture&f,const State&s,std::span<const Word24,401>h,const std::array<SampleTrace,32>&t){
    std::cout<<"{\"flags\":"<<s.flags.bits<<",\"gain\":"<<f.p.gain.bits<<",\"depth\":"<<f.p.depth.bits<<",\"cursor\":"<<s.active->cursor<<",\"cached_end\":"<<s.active->cached_end<<",\"output\":[";
    for(unsigned i=0;i<32;i++)std::cout<<(i?",":"")<<f.output[i].bits;
    std::cout<<"],\"history\":[";for(unsigned i=0;i<401;i++)std::cout<<(i?",":"")<<h[i].bits;
    std::cout<<"],\"trace\":[";for(unsigned i=0;i<32;i++){auto q=t[i];std::cout<<(i?",":"")<<'['<<q.delay_before_correction<<','<<q.delay_after_correction<<','<<q.history_index<<','<<q.delayed_word.bits<<','<<q.output_accumulator<<','<<q.next_cursor<<']';}
    std::cout<<"]}";
}
int main(){try{
    auto fixtures=std::make_unique<std::array<Fixture,4>>();
    for(auto&f:*fixtures){
        f.c.entry_status=word();f.c.live_begin_offset=number();f.c.live_span=number();
        for(auto&mode:f.c.addressing){unsigned n=number();if(n>2)throw std::invalid_argument("Unknown mode");mode=static_cast<AddressingMode>(n);}
        {auto mode=number();if(mode>2)throw std::invalid_argument("Unknown state-relative mode");f.c.state_relative=static_cast<AddressingMode>(mode);}
        f.p={word(),word()};f.flags=word();f.cursor=number();f.end=number();f.different=number();if(f.different>1)throw std::invalid_argument("Unknown declarations");
        for(auto&w:f.audio)w=word();
        for(auto&w:f.modulation)w=word();
        for(auto&w:f.output)w=word();
        for(auto&w:f.history)w=word();
    }
    auto owner=std::make_unique<std::array<Word24,1601>>();
    for(unsigned j=0;j<4;j++){
        if((*fixtures)[j].modulation!=(*fixtures)[0].modulation)throw std::runtime_error("Original common modulation mismatch");
        for(unsigned i=j?1:0;i<401;i++)(*owner)[400*j+i]=(*fixtures)[j].history[i];
    }
    std::cout<<"{\"history_owner_address\":"<<reinterpret_cast<std::uintptr_t>(owner.get())<<",\"common_modulation_address\":"<<reinterpret_cast<std::uintptr_t>((*fixtures)[0].modulation.data())<<",\"view_addresses\":[";
    for(unsigned j=0;j<4;j++)std::cout<<(j?",":"")<<reinterpret_cast<std::uintptr_t>(owner->data()+j*400);
    std::cout<<"],\"results\":[";
    for(unsigned j=0;j<4;j++){
        auto&f=(*fixtures)[j];std::span<Word24,401>history(owner->data()+400*j,401);
        for(unsigned i=0;i<401;i++)if(history[i]!=f.history[i])throw std::runtime_error("Native prior shared endpoint differs from original begin");
        Bindings actual{f.audio,(*fixtures)[0].modulation,f.output};State state{f.flags,std::nullopt};
        if(f.flags.bits&1u)state.active=ActiveState{actual,f.cursor,f.end};
        std::array<Word24,32>alternate_audio{},alternate_modulation{},alternate_output{};alternate_output.fill(Word24(0x33));
        Bindings declared=f.different?Bindings{alternate_audio,alternate_modulation,alternate_output}:actual;
        std::array<SampleTrace,32>trace{};process(f.c,f.p,state,declared,history,trace);
        for(auto w:alternate_output)if(w.bits!=0x33)throw std::runtime_error("Warm ignored output mutated");
        if(j)std::cout<<',';
        emit(f,state,history,trace);
    }
    std::cout<<"],\"final_owner\":[";for(unsigned i=0;i<1601;i++)std::cout<<(i?",":"")<<(*owner)[i].bits;std::cout<<"]}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
