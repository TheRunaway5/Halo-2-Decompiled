#include "halo2/mcc_effect_parameter_function.hpp"
#include <iostream>
#include <xmmintrin.h>
using namespace halo2::tu15_effect_function;
std::uint16_t cw(){std::uint16_t x;asm volatile("fnstcw %0":"=m"(x));return x;}
std::uint16_t sw(){std::uint16_t x;asm volatile("fnstsw %0":"=m"(x));return x;}
int main(){unsigned count=0;unsigned supplied,flags;CubicParameterValues function{};Inputs inputs;SpeakerGeometry geometry;std::int32_t speaker;while(std::cin>>supplied>>function.input>>function.range_input>>function.period_bits>>function.lower_bits>>function.upper_bits>>flags>>speaker){function.flags=std::uint8_t(flags);for(auto&p:function.polynomials)std::cin>>p.cubic>>p.quadratic>>p.linear>>p.constant;for(auto&w:inputs.words)std::cin>>w;for(auto&position:geometry.directions)for(auto&w:position)std::cin>>w;const auto before_cw=cw(),before_sw=sw();const auto before_mx=_mm_getcsr();try{auto result=evaluate_cubic_values(function,inputs,speaker,&geometry,{std::uint16_t(supplied),0x1f80});if(cw()!=before_cw||sw()!=before_sw||_mm_getcsr()!=before_mx){std::cerr<<"case "<<count<<" cw "<<cw()<<"/"<<before_cw<<" sw "<<sw()<<"/"<<before_sw<<" mx "<<_mm_getcsr()<<"/"<<before_mx<<"\n";return 2;}std::cout<<result.fraction_bits<<' '<<result.value_bits<<'\n';}catch(const halo2::FormatError&){if(cw()!=before_cw||sw()!=before_sw||_mm_getcsr()!=before_mx)return 3;std::cout<<"rejected\n";}++count;}}
