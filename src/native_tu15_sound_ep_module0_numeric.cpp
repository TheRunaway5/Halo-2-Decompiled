#include "halo2/native_tu15_sound_ep_module0_numeric.hpp"
#include "native_tu15_sound_ep_module0_literals.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2::tu15_ep_module0_numeric {
namespace {
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
std::int64_t signed24(Word24 w){return w.bits&0x800000?std::int64_t(w.bits)-0x1000000:w.bits;}
std::int64_t wrap56(std::int64_t value){const auto raw=std::uint64_t(value)&mask56;return raw&(std::uint64_t{1}<<55)?std::int64_t(raw)-(std::int64_t{1}<<56):std::int64_t(raw);}
std::int64_t shift(std::int64_t value,unsigned n){const auto divisor=std::int64_t{1}<<n;return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
Word24 transfer(std::int64_t a){return Word24(std::uint32_t(std::clamp(shift(a,24),std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffff);}
std::int64_t multiply(Word24 a,Word24 b){return 2*signed24(a)*signed24(b);}
std::int64_t round(std::int64_t a){auto raw=std::uint64_t(wrap56(a+0x800000))&mask56;if(!(raw&0xffffff))raw&=~(std::uint64_t{1}<<24);return wrap56(std::int64_t(raw&~std::uint64_t{0xffffff}));}
void valid(Word24 w){if(w.bits>0xffffff)throw std::invalid_argument("Module0 value exceeds24bits");}
template<class Sections>void validate_sections(const Sections&sections){for(const auto&section:sections){for(auto sample:section.samples)valid(sample);valid(section.residual_high);valid(section.residual_low);}}
std::int64_t retained_residual(const SectionHistory&h){const auto raw=(std::uint64_t(h.residual_high.bits)<<24)|h.residual_low.bits;return raw&(std::uint64_t{1}<<47)?std::int64_t(raw)-(std::int64_t{1}<<48):std::int64_t(raw);}
template<std::size_t Count>void filter(std::array<Word24,256>&channel,std::array<SectionHistory,Count>&sections,std::size_t coefficient_base){
    for(std::size_t section=0;section<Count;section++){
        auto&h=sections[section];const auto at=coefficient_base+5*section;
        const auto c0=Word24(literals::coefficients[at]),c1=Word24(literals::coefficients[at+1]),c2=Word24(literals::coefficients[at+2]),c3=Word24(literals::coefficients[at+3]),c4=Word24(literals::coefficients[at+4]);
        auto previous_output=h.samples[0],older_output=h.samples[1],previous_input=h.samples[2],older_input=h.samples[3];
        auto residual=retained_residual(h);
        for(auto&sample:channel){
            const auto input=sample;
            auto a=-multiply(c0,previous_output);
            a=wrap56(a-multiply(c0,previous_output));
            a=wrap56(a-multiply(c1,older_output));
            a=wrap56(a+multiply(c2,input));
            a=wrap56(a+multiply(c3,previous_input));
            a=wrap56(a+multiply(c3,previous_input));
            a=wrap56(a+multiply(c4,older_input));
            a=wrap56(a+residual);
            const auto output=transfer(a);
            residual=wrap56(a-signed24(output)*(std::int64_t{1}<<24));
            older_output=previous_output;previous_output=output;older_input=previous_input;previous_input=input;sample=output;
        }
        h.samples={previous_output,older_output,previous_input,older_input};
        const auto raw=std::uint64_t(residual);h.residual_high=Word24(std::uint32_t(raw>>24)&0xffffff);h.residual_low=Word24(std::uint32_t(raw)&0xffffff);
    }
}
std::int64_t magnitude(std::int64_t v){return v<0?-v:v;}
void diagnose(Result&result){
    for(unsigned output=0;output<2;output++){
        std::int64_t peak=0,energy=0;
        for(auto sample:result.output[output]){const auto value=signed24(sample);energy=wrap56(energy+multiply(sample,sample));if(magnitude(peak)<magnitude(value))peak=value;}
        result.peak[output]=transfer(magnitude(peak)*(std::int64_t{1}<<24));
        const auto shifted_energy=shift(energy,8);std::int64_t a=0,b=std::int64_t{0x400000}<<24;
        for(unsigned bit=0;bit<23;bit++){const auto previous=transfer(a);a=wrap56(a+b);const auto trial=transfer(a);const auto residual=wrap56(-multiply(trial,trial)+shifted_energy);a=signed24(previous)*(std::int64_t{1}<<24);if(residual>=0)a=wrap56(a+b);b=shift(b,1);}
        result.energy_root[output]=transfer(a);
    }
}
}
Result process(const Controls&controls,const Parameters&parameters,const Inputs&inputs,State&state){
    valid(controls.entry_status);valid(parameters.initialization_requested);valid(parameters.gain_level);
    if(controls.entry_status.bits&~0xffu)throw std::invalid_argument("Module0 entry status outside proved profile");
    for(auto mode:controls.address_modes)if(mode!=AddressingMode::linear)throw std::invalid_argument("Module0 addressing outside proved profile");
    for(const auto&channel:inputs)for(auto sample:channel)valid(sample);
    validate_sections(state.left);validate_sections(state.right);validate_sections(state.surround);
    State next=parameters.initialization_requested.bits?State{}:state;
    Result result{};
    // P03D4 clears the complete accumulator, then writes config word7 to
    // A1 alone. Its extension is zero even when the parameter high bit is set.
    const auto level=std::clamp(parameters.gain_level.bits,1u,31u);
    result.scale_coefficient=Word24(literals::gain_levels[level-1]);
    Inputs scaled=inputs;
    for(auto&channel:scaled)for(auto&sample:channel)sample=transfer(round(multiply(result.scale_coefficient,sample)));
    const auto diagonal=Word24(0x5a827a);
    std::array<Word24,256> surround{};
    for(unsigned i=0;i<256;i++){
        surround[i]=transfer(round(wrap56(multiply(diagonal,scaled[3][i])+multiply(diagonal,scaled[4][i]))));
        result.output[0][i]=transfer(round(wrap56(signed24(scaled[0][i])*(std::int64_t{1}<<24)+multiply(diagonal,scaled[1][i]))));
        result.output[1][i]=transfer(round(wrap56(signed24(scaled[2][i])*(std::int64_t{1}<<24)+multiply(diagonal,scaled[1][i]))));
    }
    filter(result.output[0],next.left,0);filter(result.output[1],next.right,0);filter(surround,next.surround,20);
    for(unsigned i=0;i<256;i++){
        result.output[0][i]=transfer(round(wrap56(signed24(result.output[0][i])*(std::int64_t{1}<<24)+multiply(diagonal,surround[i]))));
        result.output[1][i]=transfer(round(wrap56(signed24(result.output[1][i])*(std::int64_t{1}<<24)-multiply(diagonal,surround[i]))));
    }
    diagnose(result);state=next;return result;
}
std::array<std::byte,1024> pack_interleaved_s16(const Result&result){
    for(const auto&channel:result.output)for(auto sample:channel)valid(sample);
    std::array<std::byte,1024> output;
    for(unsigned i=0;i<256;i++)for(unsigned channel=0;channel<2;channel++){const auto word=result.output[channel][i].bits>>8;const auto offset=(2*i+channel)*2;output[offset]=std::byte(word&255);output[offset+1]=std::byte(word>>8);}
    return output;
}
}
