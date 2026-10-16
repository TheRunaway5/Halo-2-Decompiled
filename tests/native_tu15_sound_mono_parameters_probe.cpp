#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include <bit>
#include <iostream>
using namespace halo2::tu15_mono_parameter_conversion;
int main(){std::uint32_t kind,cw;while(std::cin>>kind>>cw){const auto mode=static_cast<FpMode>(cw);if(kind==0){std::array<std::uint32_t,7> bits;for(auto&v:bits)std::cin>>v;const auto p=convert_bounded_values(bits,mode);std::cout<<p.gain.bits;for(const auto&f:{p.before_clip,p.after_clip})for(auto v:{f.current_input,f.previous_input,f.older_input,f.previous_output,f.older_output})std::cout<<' '<<v.bits;std::cout<<'\n';}else{std::uint64_t bits;std::cin>>bits;std::cout<<quantize_finite_fraction(std::bit_cast<double>(bits),mode).bits<<'\n';}}}
