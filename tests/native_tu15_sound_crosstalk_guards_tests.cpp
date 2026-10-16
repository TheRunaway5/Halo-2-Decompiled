#include "halo2/native_tu15_sound_crosstalk.hpp"
#include <iostream>
#include <stdexcept>

using namespace halo2::tu15_sound_crosstalk;
struct Harness {
    Controls control{Word24(0),AddressingMode::linear,AddressingMode::linear,AddressingMode::linear};
    State state;
    std::array<std::array<Word24,32>,4> inputs,outputs;
    std::array<ArithmeticTrace,64> trace;
    std::array<Word24,65> partial;
    int alias_left=-1,alias_right=-1;
    bool partial_input_output{},partial_inputs{},partial_outputs{};
    Harness() {
        state.flags=Word24(3);state.coefficients={Word24(0x7645a1),Word24(0xf88313)};
        for(auto& pair:state.pairs) {
            pair.cursor=10;
            for(auto& history:pair.histories)for(auto& word:history)word=Word24(0x123456);
            pair.previous={Word24(0xfedcba),Word24(0x654321)};
        }
        for(auto& input:inputs)for(unsigned i=0;i<32;i++)input[i]=Word24(i+1);
        for(auto& output:outputs)for(unsigned i=0;i<32;i++)output[i]=Word24(0xffff00+i);
        for(auto& sample:trace)for(auto& value:sample.values)value=0x1234567890;
        for(auto& word:partial)word=Word24(0x987654);
    }
    Word24* buffer(unsigned index){return index<4?inputs[index].data():outputs[index-4].data();}
};
static void require(bool condition){if(!condition)throw std::runtime_error("Admission guard mutated caller data or accepted an unsupported input");}
static unsigned rejections{};
template<class Setup>void reject(Setup setup) {
    Harness h;setup(h);
    const auto state=h.state;
    const auto inputs=h.inputs,outputs=h.outputs;
    const auto trace=h.trace;
    const auto partial=h.partial;
    std::array<Input,4> in{h.inputs[0],h.inputs[1],h.inputs[2],h.inputs[3]};
    std::array<Output,4> out{h.outputs[0],h.outputs[1],h.outputs[2],h.outputs[3]};
    if(h.alias_right>=0) {
        if(h.alias_right<4)in[h.alias_right]=Input(h.buffer(h.alias_left),32);
        else out[h.alias_right-4]=Output(h.buffer(h.alias_left),32);
    }
    if(h.partial_input_output){in[0]=Input(h.partial.data(),32);out[3]=Output(h.partial.data()+1,32);}
    if(h.partial_inputs){in[0]=Input(h.partial.data(),32);in[3]=Input(h.partial.data()+1,32);}
    if(h.partial_outputs){out[0]=Output(h.partial.data(),32);out[3]=Output(h.partial.data()+1,32);}
    bool rejected=false;
    try{(void)process(h.control,h.state,in,out,h.trace);}catch(const std::invalid_argument&){rejected=true;}
    require(rejected&&h.state==state&&h.inputs==inputs&&h.outputs==outputs&&h.trace==trace&&h.partial==partial);
    ++rejections;
}
int main() {
    for(unsigned bit:{10,11,12,13,14,15,16,17,18,20})reject([=](Harness& h){h.control.entry_status=Word24(1u<<bit);});
    reject([](Harness& h){h.control.entry_status.bits=0x1000000;});
    for(unsigned mode:{1,2,99}) {
        reject([=](Harness& h){h.control.state_relative=static_cast<AddressingMode>(mode);});
        reject([=](Harness& h){h.control.first_output=static_cast<AddressingMode>(mode);});
        reject([=](Harness& h){h.control.second_output=static_cast<AddressingMode>(mode);});
    }
    reject([](Harness& h){h.state.flags.bits=0x1000000;});
    for(unsigned i=0;i<2;i++)reject([=](Harness& h){h.state.coefficients[i].bits=0x1000000;});
    for(unsigned pair=0;pair<2;pair++) {
        for(unsigned cursor:{11u,12u,0xffffffffu})reject([=](Harness& h){h.state.pairs[pair].cursor=cursor;});
        for(unsigned channel=0;channel<2;channel++) {
            for(unsigned word=0;word<11;word++)reject([=](Harness& h){h.state.pairs[pair].histories[channel][word].bits=0x1000000;});
            reject([=](Harness& h){h.state.pairs[pair].previous[channel].bits=0x1000000;});
        }
    }
    for(unsigned buffer=0;buffer<8;buffer++)for(unsigned word:{0,15,31})
        reject([=](Harness& h){h.buffer(buffer)[word].bits=0x1000000;});
    for(unsigned right=1;right<8;right++)for(unsigned left=0;left<right;left++)
        reject([=](Harness& h){h.alias_left=static_cast<int>(left);h.alias_right=static_cast<int>(right);});
    reject([](Harness& h){h.partial_input_output=true;});
    reject([](Harness& h){h.partial_inputs=true;});
    reject([](Harness& h){h.partial_outputs=true;});
    require(rejections==132);
    std::cout<<"{\"passed\":true,\"rejections_before_mutation\":"<<rejections<<"}\n";
}
