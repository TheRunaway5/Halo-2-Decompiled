#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace halo2::tu15_sound_mono_distortion {
namespace {
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
std::int64_t signed_word(Word24 w) {
    return (w.bits&0x800000u)?std::int64_t(w.bits)-0x1000000:std::int64_t(w.bits);
}
std::int64_t wrap56(std::int64_t v) {
    const auto bits=std::uint64_t(v)&mask56;
    return (bits&(std::uint64_t{1}<<55))?std::int64_t(bits)-std::int64_t(std::uint64_t{1}<<56):std::int64_t(bits);
}
std::int64_t product(Word24 a,Word24 b) {
    // DSP56300 fractional MPY: signed 24 x 24 product aligned with doubled LSB.
    return signed_word(a)*signed_word(b)*2;
}
std::int64_t add(std::int64_t a,std::int64_t b) {return wrap56(a+b);}
std::int64_t floor_shift(std::int64_t v,unsigned shift) {
    const auto d=std::int64_t{1}<<shift;
    return v>=0?v/d:-((-v+d-1)/d);
}
Word24 transfer(std::int64_t accumulator,bool scaling_up) {
    // Data limiter, DSP56300FM data ALU/scaling. P01A9/P01CD select S1;
    // P01C0/P01DE clear it. No RND/MACR instruction occurs in this record.
    const auto value=floor_shift(accumulator,scaling_up?23:24);
    const auto limited=std::clamp(value,std::int64_t{-0x800000},std::int64_t{0x7fffff});
    return Word24(std::uint32_t(limited)&0xffffffu);
}
std::int64_t filter(Word24 input,const FilterCoefficients& c,FilterHistory& h) {
    // P01AF..01B2 / P01D0..01D3, including original parallel operand timing
    // and REP3: current, previousInput, olderInput, previousOutput,
    // olderOutput. Preserve full 56-bit accumulator through every addition.
    auto a=product(c.current_input,input);
    a=add(a,product(c.previous_input,h.previous_input));
    a=add(a,product(c.older_input,h.older_input));
    a=add(a,product(c.previous_output,h.previous_output));
    a=add(a,product(c.older_output,h.older_output));
    // P01B8..01BF / P01D6..01DD; histories receive S1-limited transfer,
    // while the accumulator remains unrounded for the later final store.
    const auto result=transfer(a,true);
    h.older_input=h.previous_input;h.previous_input=input;
    h.older_output=h.previous_output;h.previous_output=result;
    return a;
}
void validate(Word24 w){if(w.bits>0xffffffu)throw std::invalid_argument("Mutated Word24 exceeds 24 bits");}
void validate(const FilterCoefficients& c) {
    validate(c.current_input);validate(c.previous_input);validate(c.older_input);validate(c.previous_output);validate(c.older_output);
}
void validate(const FilterHistory& h) {validate(h.previous_input);validate(h.older_input);validate(h.previous_output);validate(h.older_output);}
}
void process(const Controls& control,Parameters& p,State& s,std::span<const Word24,32> input,std::span<Word24,32> output) {
    // This candidate admits the actual traced entry modes only. DSP56300
    // alternate SA/SM modes and S0 scaling-down are separate arithmetic modes.
    if(control.entry_scaling!=ScalingMode::none || control.arithmetic_saturation || control.sixteen_bit_arithmetic || control.double_precision_multiply)
        throw std::invalid_argument("Original arithmetic mode is outside recovered domain");
    if(control.state_relative!=AddressingMode::linear)throw std::invalid_argument("Unsupported original state-relative addressing mode");
    for(auto mode:{control.coefficients,control.input,control.output,control.history})
        if(mode!=AddressingMode::linear)throw std::invalid_argument("Original address mode is outside recovered domain");
    validate(p.gain);validate(p.before_clip);validate(p.after_clip);validate(s.flags);validate(s.last_dry_word);validate(s.before_clip);validate(s.after_clip);
    for(auto w:input)validate(w);
    for(auto w:output)validate(w);
    const auto ib=reinterpret_cast<std::uintptr_t>(input.data()),ob=reinterpret_cast<std::uintptr_t>(output.data());
    constexpr auto bytes=sizeof(Word24)*32;
    if(ib>std::numeric_limits<std::uintptr_t>::max()-bytes||ob>std::numeric_limits<std::uintptr_t>::max()-bytes)throw std::invalid_argument("Native span address overflow");
    if(ib!=ob&&ib<ob+bytes&&ob<ib+bytes)throw std::invalid_argument("Partial native span overlap is outside recovered domain");
    // P0198..019E. The literal first-call gain intentionally replaces both the
    // authored initial gain and a converter update supplied before first run.
    if(!(s.flags.bits&1u)){s.flags=Word24(s.flags.bits|1u);p.gain=Word24(0x370000u);}
    for(std::size_t i=0;i<32;i++) { // P01A7 DOR32
        const Word24 source=input[i],dry=output[i];
        auto a=filter(source,p.before_clip,s.before_clip);
        s.last_dry_word=dry; // P01B3..01B4, before aliased output overwrite
        const auto pre=transfer(a,true); // P01B5
        // P01C1..01C6: transfer to A, arithmetic shift 4, limited data transfer
        // into X1, fractional gain multiply, then normal limited transfer.
        const auto shifted=wrap56(signed_word(pre)*(std::int64_t{1}<<28));
        const auto clipped=transfer(shifted,false);
        const auto gained=transfer(product(clipped,p.gain),false);
        output[i]=gained;
        a=filter(gained,p.after_clip,s.after_clip);
        if(s.flags.bits&2u)a=add(a,signed_word(dry)*(std::int64_t{1}<<24)); // P01DF..01E3
        output[i]=transfer(a,false); // P01E4, after clearing S1
    }
}
Parameters authored_initial_parameters() {
    const FilterCoefficients c{Word24(0x400000),Word24(0xad7746),Word24(0x23434c),Word24(0x5288b9),Word24(0xdcbcb3)};
    return Parameters{Word24(3000000),c,c};
}
} // namespace halo2::tu15_sound_mono_distortion
