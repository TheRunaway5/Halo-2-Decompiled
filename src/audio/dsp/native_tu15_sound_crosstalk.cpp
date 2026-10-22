#include "halo2/native_tu15_sound_crosstalk.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace halo2::tu15_sound_crosstalk {
namespace {
void validate(Word24 word) {
    if(word.bits>0xffffffu)throw std::invalid_argument("Mutated data word exceeds24bits");
}
std::int64_t signed_word(Word24 word) {
    return word.bits&0x800000u?std::int64_t(word.bits)-0x1000000:std::int64_t(word.bits);
}
std::int64_t saturate48(std::int64_t value) {
    return std::clamp(value,-(std::int64_t{1}<<47),(std::int64_t{1}<<47)-1);
}
std::int64_t product(Word24 left,Word24 right) {
    return 2*signed_word(left)*signed_word(right);
}
Word24 transfer(std::int64_t value) {
    constexpr std::int64_t divisor=std::int64_t{1}<<24;
    const auto floored=value>=0?value/divisor:-((-value+divisor-1)/divisor);
    return Word24(std::uint32_t(std::clamp(floored,std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu);
}
std::uintptr_t address(const Word24* pointer) {
    const auto value=reinterpret_cast<std::uintptr_t>(pointer);
    if(!value||value>std::numeric_limits<std::uintptr_t>::max()-32*sizeof(Word24))
        throw std::invalid_argument("Invalid native buffer extent");
    return value;
}
bool overlaps(std::uintptr_t left,std::uintptr_t right) {
    constexpr auto bytes=32*sizeof(Word24);
    return left<right+bytes&&right<left+bytes;
}
}
Result process(const Controls& control,State& state,const std::array<Input,4>& inputs,
               const std::array<Output,4>& outputs,std::span<ArithmeticTrace,64> trace) {
    validate(control.entry_status);
    constexpr auto unsupported=(3u<<10)|(1u<<12)|(1u<<13)|(1u<<14)|(1u<<15)|
        (1u<<16)|(1u<<17)|(1u<<18)|(1u<<20);
    if(control.entry_status.bits&unsupported)
        throw std::invalid_argument("Unsupported original entry mode or loop context");
    if(control.state_relative!=AddressingMode::linear||
       control.first_output!=AddressingMode::linear||control.second_output!=AddressingMode::linear)
        throw std::invalid_argument("State-relative or output addressing mode is outside the recovered domain");
    validate(state.flags);
    for(auto coefficient:state.coefficients)validate(coefficient);
    for(const auto& pair:state.pairs) {
        if(pair.cursor>=11)throw std::invalid_argument("History cursor is outside its owned eleven-word ring");
        for(const auto& history:pair.histories)for(auto word:history)validate(word);
        for(auto word:pair.previous)validate(word);
    }
    std::array<std::uintptr_t,8> buffers;
    for(unsigned i=0;i<4;i++){buffers[i]=address(inputs[i].data());buffers[4+i]=address(outputs[i].data());}
    for(unsigned i=0;i<8;i++)for(unsigned prior=0;prior<i;prior++)
        if(overlaps(buffers[i],buffers[prior]))throw std::invalid_argument("Overlapping spans are outside the recovered graph");
    for(auto input:inputs)for(auto word:input)validate(word);
    for(auto output:outputs)for(auto word:output)validate(word);

    if(!(state.flags.bits&1u)) { // P0CEF..0CF8
        state.flags=Word24(state.flags.bits|1u);
        state.coefficients={Word24(0x7645a1),Word24(0xf88313)};
    }
    for(unsigned pair_index=0;pair_index<2;pair_index++) {
        auto& pair=state.pairs[pair_index];
        for(unsigned sample=0;sample<32;sample++) {
            auto& values=trace[32*pair_index+sample].values;
            const auto cursor=pair.cursor;
            auto b=saturate48(product(state.coefficients[0],pair.previous[0])); // P0D39
            values[0]=b;
            // MAC saturates its combined result; its product is not separately
            // clamped before addition to the already saturated accumulator.
            b=saturate48(b+product(state.coefficients[1],pair.histories[0][cursor])); // P0D3B
            values[1]=b;
            pair.previous[0]=transfer(b); // P0D3C
            auto a=saturate48(product(state.coefficients[0],pair.previous[1])); // P0D3E
            values[2]=a;
            a=saturate48(a+product(state.coefficients[1],pair.histories[1][cursor])); // P0D40
            values[3]=a;
            pair.previous[1]=transfer(a); // P0D41
            const auto first=inputs[2*pair_index][sample],second=inputs[2*pair_index+1][sample];
            a=saturate48(a+signed_word(first)*(std::int64_t{1}<<24)); // P0D44
            values[4]=a;
            b=saturate48(b+signed_word(second)*(std::int64_t{1}<<24)); // P0D45
            values[5]=b;
            pair.histories[0][cursor]=transfer(a); // P0D46
            pair.histories[1][cursor]=transfer(b); // P0D47
            if(!(state.flags.bits&2u)) { // P0D49..0D4E; histories remain updated
                a=signed_word(first)*(std::int64_t{1}<<24);
                b=signed_word(second)*(std::int64_t{1}<<24);
            }
            a=saturate48(a+signed_word(outputs[2*pair_index][sample])*(std::int64_t{1}<<24)); // P0D51
            values[6]=a;
            b=saturate48(b+signed_word(outputs[2*pair_index+1][sample])*(std::int64_t{1}<<24)); // P0D52
            values[7]=b;
            outputs[2*pair_index][sample]=transfer(a); // P0D53
            outputs[2*pair_index+1][sample]=transfer(b); // P0D54
            pair.cursor=cursor+1<11?cursor+1:0; // P0D55..0D5C
        }
    }
    return {false}; // Source clears SM at P0D08 after both pair helpers.
}
} // namespace halo2::tu15_sound_crosstalk
