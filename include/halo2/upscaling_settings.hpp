#pragma once
#include <algorithm>
#include <cstdint>
#include <utility>
namespace halo2 {
enum class Upscaler : std::uint8_t { Off, FSR31, DLSS45 };
enum class UpscalingQuality : std::uint8_t { NativeAA, UltraQuality, Quality, Balanced, Performance, UltraPerformance };
constexpr const char* upscaler_name(Upscaler v) {
    switch(v) { case Upscaler::Off:return "Off";case Upscaler::FSR31:return "FSR 3.1";case Upscaler::DLSS45:return "DLSS 4.5"; }
    return "Unknown";
}
constexpr const char* upscaling_quality_name(UpscalingQuality v,Upscaler method) {
    switch(v) {
    case UpscalingQuality::NativeAA:return method==Upscaler::DLSS45?"DLAA":"Native AA";
    case UpscalingQuality::UltraQuality:return "Ultra Quality";
    case UpscalingQuality::Quality:return "Quality";
    case UpscalingQuality::Balanced:return "Balanced";
    case UpscalingQuality::Performance:return "Performance";
    case UpscalingQuality::UltraPerformance:return "Ultra Performance";
    }
    return "Unknown";
}
constexpr bool valid_upscaling(Upscaler method,UpscalingQuality quality) {
    return unsigned(method)<=unsigned(Upscaler::DLSS45) && unsigned(quality)<=unsigned(UpscalingQuality::UltraPerformance);
}
// Input pixels per output axis. Ultra Quality is the donor's custom 1/1.3
// scale; DLSS executes it through Quality's dynamic-resolution input range.
constexpr std::pair<unsigned,unsigned> upscaling_render_size(unsigned w,unsigned h,Upscaler method,UpscalingQuality quality) {
    if(method==Upscaler::Off)return {w,h};
    constexpr unsigned numerator[]{100,100,2,10,1,1},denominator[]{100,130,3,17,2,3};
    const auto q=std::min(unsigned(quality),5u);
    return {std::max(1u,unsigned(std::uint64_t(w)*numerator[q]/denominator[q])),
            std::max(1u,unsigned(std::uint64_t(h)*numerator[q]/denominator[q]))};
}
}
