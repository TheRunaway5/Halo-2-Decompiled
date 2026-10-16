#include "halo2/native_tu15_sound_ring_effect.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
using namespace halo2::tu15_sound_ring_effect;
namespace {
constexpr std::array<unsigned,14> sizes{6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000};
constexpr std::array<unsigned,31> route{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,5,6,7,8,9,10,11,12,13};
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
struct Harness {
    State state;
    Workspace workspace{};
    std::array<Word,64> input{};
    std::array<std::array<Word,64>,4> output{};
    Controls controls{};
    ArithmeticTrace trace{};
    std::span<Word,32> input_view{input.data(),32};
    std::array<std::span<Word,32>,4> output_view{
        std::span<Word,32>{output[0].data(),32},std::span<Word,32>{output[1].data(),32},
        std::span<Word,32>{output[2].data(),32},std::span<Word,32>{output[3].data(),32}};
    Harness(){state.flags=3;controls.modifiers.fill(0xffffffu);trace.values.fill(0xffeedd);trace.count=std::numeric_limits<Word>::max();for(unsigned i=0;i<14;i++)state.rings[i].words.resize(sizes[i]);}
    std::vector<Word> snapshot()const{
        std::vector<Word> words;
        auto one=[&](auto value){words.push_back(Word(value));};
        auto range=[&](const auto& values){for(auto value:values)one(value);};
        one(state.flags);for(const auto& ring:state.rings){one(ring.words.size());one(ring.write_byte_cursor);range(ring.words);}for(const auto& tap:state.taps)one(tap.read_byte_cursor);
        range(state.delays);range(state.reflection_gains);one(state.cross_gain);one(state.mono_gain);range(state.target_gains);range(state.current_gains);one(state.output_mode);
        for(const auto& filter:state.filters){one(filter.previous);one(filter.feedback);one(filter.feedforward);}range(state.diffusion_gains);range(state.wet_gains);range(state.cluster_taps);range(state.dormant_biquad_coefficients);
        const auto& fade=state.fade;one(fade.pending_mask);one(fade.sample_count);one(fade.step);one(fade.amplitude);one(fade.selected_read);one(fade.selected_write);one(fade.write_stride);one(fade.tap_cursor);range(fade.adjusted_taps);one(fade.bindings_initialized);one(fade.tap_owner);one(state.smoothing_count);
        for(const auto& frame:workspace)range(frame);
        range(input);
        for(const auto& channel:output)range(channel);
        one(controls.entry_status);range(controls.modifiers);range(trace.values);one(trace.count);return words;
    }
    void run(){process(state,workspace,input_view,output_view,controls,&trace);}
};
unsigned rejected{};
template<class Mutate>void reject(Mutate mutate){
    auto h=std::make_unique<Harness>();mutate(*h);const auto before=h->snapshot();bool threw=false;
    try{h->run();}catch(const std::invalid_argument&){threw=true;}
    require(threw,"unsupported ownership/cursor/value was admitted");require(h->snapshot()==before,"rejection mutated caller data");++rejected;
}
}
int main(){try{
    for(unsigned ring=0;ring<14;ring++){
        for(int delta:{-1,1})reject([=](Harness& h){h.state.rings[ring].words.resize(sizes[ring]+delta);});
        reject([=](Harness& h){h.state.rings[ring].words.clear();});
        for(Word cursor:{Word(sizes[ring]*4),Word(sizes[ring]*4+16),Word(1),Word(sizes[ring]*4-4),std::numeric_limits<Word>::max()})reject([=](Harness& h){h.state.rings[ring].write_byte_cursor=cursor;});
        for(unsigned position:{0u,sizes[ring]/2,sizes[ring]-1})reject([=](Harness& h){h.state.rings[ring].words[position]=0x1000000;});
    }
    for(unsigned tap=0;tap<31;tap++)for(Word cursor:{Word(sizes[route[tap]]*4),Word(sizes[route[tap]]*4+16),Word(1),Word(sizes[route[tap]]*4-4),std::numeric_limits<Word>::max()})reject([=](Harness& h){h.state.taps[tap].read_byte_cursor=cursor;});
    // Use actual live Word array ranges for exact and partial owner overlaps.
    for(unsigned owner=0;owner<14;owner++)for(unsigned channel=0;channel<5;channel++)for(unsigned displacement:{0u,1u})reject([=](Harness& h){
        Word* p=h.state.rings[owner].words.data();
        std::span<Word,32> alias(p+displacement,32);if(channel==0)h.input_view=alias;else h.output_view[channel-1]=alias;
    });
    for(unsigned first=0;first<5;first++)for(unsigned second=first+1;second<5;second++)for(unsigned displacement:{0u,1u})reject([=](Harness& h){
        Word* p=first==0?h.input.data():h.output[first-1].data();
        // Backing arrays contain64 words, so the shifted32-word range is valid.
        std::span<Word,32> alias(p+displacement,32);h.output_view[second-1]=alias;
    });
    for(unsigned channel=0;channel<5;channel++)for(unsigned displacement:{0u,1u})reject([=](Harness& h){std::span<Word,32> alias(h.trace.values.data()+displacement,32);if(channel==0)h.input_view=alias;else h.output_view[channel-1]=alias;});
    for(auto member:{&State::flags,&State::cross_gain,&State::mono_gain,&State::output_mode,&State::smoothing_count})reject([=](Harness& h){h.state.*member=0x1000000;});
    auto array_words=[](auto member,unsigned count){for(unsigned j=0;j<count;j++)reject([=](Harness& h){(h.state.*member)[j]=0x1000000;});};
    array_words(&State::delays,19);array_words(&State::reflection_gains,8);array_words(&State::target_gains,6);array_words(&State::current_gains,6);array_words(&State::diffusion_gains,7);array_words(&State::wet_gains,4);array_words(&State::cluster_taps,12);array_words(&State::dormant_biquad_coefficients,10);
    for(unsigned j=0;j<10;j++)for(auto member:{&OnePole::previous,&OnePole::feedback,&OnePole::feedforward})reject([=](Harness& h){h.state.filters[j].*member=0x1000000;});
    for(auto member:{&Fade::pending_mask,&Fade::sample_count,&Fade::step,&Fade::amplitude})reject([=](Harness& h){h.state.fade.*member=0x1000000;});
    for(unsigned j=0;j<8;j++)reject([=](Harness& h){h.state.fade.adjusted_taps[j]=0x1000000;});
    for(unsigned block=0;block<34;block++)for(unsigned j=0;j<16;j++)reject([=](Harness& h){h.workspace[block][j]=0x1000000;});
    for(unsigned j=0;j<32;j++){reject([=](Harness& h){h.input[j]=0x1000000;});for(unsigned channel=0;channel<4;channel++)reject([=](Harness& h){h.output[channel][j]=0x1000000;});}
    for(unsigned bit:{10u,11u,12u,13u,14u,15u,16u,17u,18u,20u,21u})reject([=](Harness& h){h.controls.entry_status=Word{1}<<bit;});
    reject([](Harness& h){h.controls.entry_status=0x1000000;});
    for(unsigned j=0;j<8;j++)for(Word mode:{0u,31u,65534u,65535u})reject([=](Harness& h){h.controls.modifiers[j]=mode;});
    for(unsigned j=0;j<19;j++)reject([=](Harness& h){h.state.delays[j]=0x800000;});
    for(unsigned j=0;j<12;j++)reject([=](Harness& h){h.state.cluster_taps[j]=0x800000;});
    reject([](Harness& h){h.state.fade.pending_mask=3;});
    reject([](Harness& h){h.state.fade.sample_count=1;});
    reject([](Harness& h){h.state.fade.sample_count=0x110;});
    reject([](Harness& h){h.state.fade.selected_read=31;});
    reject([](Harness& h){h.state.fade.selected_write=14;});
    reject([](Harness& h){h.state.fade.write_stride=2;});
    reject([](Harness& h){h.state.fade.tap_cursor=20;});
    reject([](Harness& h){h.state.fade.pending_mask=2;h.state.fade.selected_read=30;h.state.fade.bindings_initialized=true;});
    reject([](Harness& h){h.state.fade.pending_mask=2;h.state.fade.selected_write=13;h.state.fade.write_stride=1;h.state.fade.bindings_initialized=true;});
    reject([](Harness& h){h.state.fade.tap_owner=static_cast<TapOwner>(2);});
    reject([](Harness& h){h.state.fade.tap_owner=TapOwner::adjusted;h.state.fade.tap_cursor=9;});
    for(unsigned count:{16u,128u})reject([=](Harness& h){h.state.fade.pending_mask=1;h.state.fade.sample_count=count;});
    reject([](Harness& h){h.state.fade.pending_mask=2;});
    reject([](Harness& h){h.state.fade.pending_mask=1;h.state.fade.sample_count=128;h.state.fade.bindings_initialized=true;h.state.fade.tap_cursor=19;});
    reject([](Harness& h){h.state.fade.pending_mask=1;h.state.fade.sample_count=128;h.state.fade.bindings_initialized=true;h.state.fade.tap_owner=TapOwner::adjusted;h.state.fade.tap_cursor=8;});
    std::array<unsigned,6> trace_counts{};
    for(unsigned profile=0;profile<6;profile++){auto h=std::make_unique<Harness>();if(profile==1)h->state.flags=2;if(profile==2||profile==3)h->state.output_mode=4;if(profile==3){h->state.fade.pending_mask=1;h->state.fade.sample_count=16;h->state.fade.step=0xff0000;h->state.fade.amplitude=0x7f0000;h->state.fade.bindings_initialized=true;}if(profile==4){h->state.fade.pending_mask=1;h->state.fade.sample_count=256;}if(profile==5)h->state.fade.pending_mask=1u<<19;h->run();require(reinterpret_cast<std::uintptr_t>(h.get())>0xffffffffu,"native owners unexpectedly fit 32 bits");trace_counts[profile]=h->trace.count;require(h->trace.count<=h->trace.values.size(),"trace overflow");}
    require(trace_counts==std::array<unsigned,6>{880,880,976,992,880,880},"unexpected fixed trace emission bound");
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<rejected<<",\"nominal_calls\":6,\"Release_NDEBUG\":true,\"native_owners_above_4GiB\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
