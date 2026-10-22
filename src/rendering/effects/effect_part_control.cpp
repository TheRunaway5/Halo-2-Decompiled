#include "halo2/effect_part_control.hpp"
namespace halo2 {
float source_effect_part_control(std::uint32_t flags_a,std::uint32_t flags_b,float a,float b) {
    float value=(flags_a&0x20u)?a:1.f;
    if(flags_b&0x20u)value=b*value;
    return value;
}
}
