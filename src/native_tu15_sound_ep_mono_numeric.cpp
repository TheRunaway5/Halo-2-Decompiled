#include "halo2/native_tu15_sound_ep_mono_numeric.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2::tu15_ep_mono_numeric {
namespace {
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
std::int64_t signed24(Word24 w){return w.bits&0x800000?std::int64_t(w.bits)-0x1000000:w.bits;}
std::int64_t wrap56(std::int64_t value){const auto raw=std::uint64_t(value)&mask56;return raw&(std::uint64_t{1}<<55)?std::int64_t(raw)-(std::int64_t{1}<<56):std::int64_t(raw);}
std::int64_t shift(std::int64_t value,unsigned n){const auto divisor=std::int64_t{1}<<n;return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
Word24 transfer(std::int64_t a){return Word24(std::uint32_t(std::clamp(shift(a,24),std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffff);}
std::int64_t multiply(Word24 a,Word24 b){return 2*signed24(a)*signed24(b);}
std::int64_t round(std::int64_t a){
 auto raw=std::uint64_t(wrap56(a+0x800000))&mask56;
 if(!(raw&0xffffff))raw&=~(std::uint64_t{1}<<24);
 return wrap56(std::int64_t(raw&~std::uint64_t{0xffffff}));
}
void valid(Word24 w){if(w.bits>0xffffff)throw std::invalid_argument("Mono input exceeds24bits");}
std::int64_t magnitude(std::int64_t v){return v<0?-v:v;}
}
Result process(const Controls&controls,const Parameters&parameters,const Inputs&inputs){
 valid(controls.entry_status);valid(parameters.scale_enabled);valid(parameters.scale);
 // All eight incoming CCR bits are admitted. In particular, source input
 // transfer can set sticky S(bit7); the body never tests it numerically.
 if(controls.entry_status.bits&~0xffu)throw std::invalid_argument("Mono numeric entry status outside proved profile");
 for(auto mode:controls.address_modes)if(mode!=AddressingMode::linear)throw std::invalid_argument("Mono numeric addressing outside proved profile");
 for(const auto&channel:inputs)for(auto sample:channel)valid(sample);
 Result result{};
 const std::array<std::uint32_t,5> authored{0x400000,0x5a8279,0x400000,0x2d413c,0x2d413c};
 for(unsigned i=0;i<5;i++){
  result.coefficients[i]=Word24(authored[i]);
  if(parameters.scale_enabled.bits)result.coefficients[i]=transfer(round(multiply(result.coefficients[i],parameters.scale)));
 }
 for(unsigned sample=0;sample<256;sample++){
  auto accumulator=multiply(inputs[0][sample],result.coefficients[0]);
  for(unsigned channel=1;channel<5;channel++)accumulator=wrap56(accumulator+multiply(inputs[channel][sample],result.coefficients[channel]));
  // P036A/P036B perform two source transfers of the same rounded A.
  const auto rounded=round(accumulator);
  result.output[0][sample]=transfer(rounded);result.output[1][sample]=transfer(rounded);
 }
 for(unsigned output=0;output<2;output++){
  std::int64_t peak=0,energy=0;
  for(auto sample:result.output[output]){
   const auto value=signed24(sample);
   energy=wrap56(energy+multiply(sample,sample));
   if(magnitude(peak)<magnitude(value))peak=value;
  }
  result.peak[output]=transfer(magnitude(peak)*(std::int64_t{1}<<24));
  const auto shifted_energy=shift(energy,8);
  // Original short immediate #40 is aligned to B1's high byte (400000).
  // Original short-immediate encoding at P037B sets B1=400000 before P037E.
  std::int64_t a=0,b=std::int64_t{0x400000}<<24;
  for(unsigned bit=0;bit<23;bit++){
   // Parallel A->X1 observes the accumulator before ADD B,A. The later
   // MOVE X1,A restores its limited24-bit value and preserves ADD Y,A flags.
   const auto previous=transfer(a);a=wrap56(a+b);const auto trial=transfer(a);
   const auto residual=wrap56(-multiply(trial,trial)+shifted_energy);
   a=signed24(previous)*(std::int64_t{1}<<24);
   if(residual>=0)a=wrap56(a+b); // exact IFGE ADD, no CCR update
   b=shift(b,1);
  }
  result.energy_root[output]=transfer(a);
 }
 return result;
}
std::array<std::byte,1024> pack_interleaved_s16(const Result&result){
 for(const auto&channel:result.output)for(auto sample:channel)valid(sample);
 std::array<std::byte,1024> output;
 for(unsigned i=0;i<256;i++)for(unsigned channel=0;channel<2;channel++){
  const auto word=result.output[channel][i].bits>>8;const auto offset=(2*i+channel)*2;
  output[offset]=std::byte(word&255);output[offset+1]=std::byte(word>>8);
 }
 return output;
}
}
