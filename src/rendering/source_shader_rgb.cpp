#include "halo2/source_shader_rgb.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
void validate(std::uint16_t control) {
    const auto precision=(control>>8)&3u;
    if((control&0x3fu)!=0x3fu || precision<2)
        throw std::runtime_error("Shader conversion requires admitted masked 53/64-bit source control");
}
std::uint32_t integer(double value,std::uint16_t control) {
    if(!std::isfinite(value))throw std::runtime_error("Shader conversion requires finite supplied input");
    double rounded;
    switch((control>>10)&3u) {
    case 1:rounded=std::floor(value);break;
    case 2:rounded=std::ceil(value);break;
    case 3:rounded=std::trunc(value);break;
    default: {
        const auto lower=std::floor(value),fraction=value-lower;
        rounded=fraction<.5?lower:fraction>.5?lower+1.0:
            std::fmod(lower,2.0)!=0.0?lower+1.0:lower;
        break;
    }
    }
    // Captured masked FISTP conversions outside signed32 produce 80000000.
    if(rounded<double(std::numeric_limits<std::int32_t>::min()) ||
       rounded>double(std::numeric_limits<std::int32_t>::max()))return 0x80000000u;
    return std::uint32_t(std::int32_t(rounded));
}
std::uint32_t channel(float input,std::uint16_t control) {
    // A binary32 significand multiplied by integer 255 fits exactly within
    // binary53. Source 53/64-bit x87 precision therefore retains this product.
    return integer(double(input)*255.0,control)&255u;
}
}
std::uint32_t source_shader_rgb(const std::array<float,3> &rgb,std::uint16_t control) {
    validate(control);
    return channel(rgb[0],control)<<16|channel(rgb[1],control)<<8|channel(rgb[2],control);
}
std::uint32_t source_shader_rgb_state_word(std::uint32_t previous,
    const std::array<float,3> &rgb,std::uint16_t control) {
    return (previous&0xff000000u)|source_shader_rgb(rgb,control);
}
std::uint32_t source_shader_integer(float value,std::uint16_t control) {
    validate(control);return integer(value,control);
}
std::uint32_t source_shader_alpha_state_word(std::uint32_t previous,
    float value,std::uint16_t control) {
    validate(control);return (previous&0x00ffffffu)|(channel(value,control)<<24);
}
}
