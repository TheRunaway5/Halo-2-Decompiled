#include "halo2/source_weather_plate_caller.hpp"
#include <algorithm>
#include <bit>
#include <cfloat>
#include <cmath>
#include <limits>
#include <stdexcept>

#if defined(__FAST_MATH__)
#error "Original weather plate caller requires strict arithmetic"
#endif
static_assert(FLT_EVAL_METHOD==0,"Original caller SSE operations require binary32 evaluation");
static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits==24);

namespace halo2 {
namespace {
float finite(float value) {
    if(!std::isfinite(value))
        throw std::invalid_argument("unsupported nonfinite source weather caller projection");
    return value;
}
}
SourceWeatherPlateCallerResult source_weather_plate_caller_profile(
    const SourceWeatherPlateCallerInput &in) {
    const auto &record=in.compositor.words;
    if((record[0]&255u)!=1u)
        throw std::invalid_argument("weather caller requires original alpha compositor flag1");
    for(unsigned index:{1u,3u,4u,6u,7u,9u})if(record[index]!=0)
        throw std::invalid_argument("weather caller requires original positive-zero X/Z compositor lanes");
    for(unsigned index:{2u,5u,8u,10u,11u,12u}) {
        const float value=std::bit_cast<float>(record[index]);
        if(!std::isfinite(value)||value<0.f||value>1.f)
            throw std::invalid_argument("weather caller alpha/tint outside source clamped compositor profile");
    }
    finite(in.projection_near);finite(in.projection_far);
    SourceWeatherPlateProgramInput producer;
    producer.compositor=in.compositor;
    producer.depth_start=in.depth_start;producer.depth_end=in.depth_end;
    producer.near_clip=in.selection_near;producer.far_clip=in.selection_far;
    producer.mode=3;producer.center=0;producer.anchor_end=0;producer.anchor_start=1;
    producer.skip_state=1;producer.depth_requested=true;
    producer.source_fpcw=in.source_fpcw;producer.source_mxcsr=in.source_mxcsr;
    // This call also enforces source/host nearest, PC53, gradual arithmetic.
    // It owns no caller output bank and publishes nothing on rejection.
    const auto generated=source_weather_plate_program(producer);
    if(!generated.program)return {};
    if(!generated.depth)
        throw std::logic_error("original caller requested depth was not produced");
    const float depth=*generated.depth;
    // Each local is the separately rounded result of one original SSE
    // instruction. Build this translation unit without contraction/fast-math.
    const float span=finite(in.projection_far-in.projection_near);
    const float scale=finite(in.projection_far/span);
    const float scaled_depth=finite(scale*depth);
    const float scaled_near=finite(scale*in.projection_near);
    const float numerator=finite(scaled_depth-scaled_near);
    const float divided_z=finite(numerator/depth);
    const float raw_z=finite(divided_z*16777215.f);
    const float divided_w=finite(depth/in.projection_far);
    const float raw_w=finite(divided_w*16777215.f);
    const auto profile=depth>in.selection_near
        ?SourceWeatherPlateCallerProfile::AboveNearWDepth
        :SourceWeatherPlateCallerProfile::NearDepthDisabledUnsupported;
    return {profile,generated.program,generated.depth,
        std::array<float,2>{std::clamp(raw_z,0.f,16777215.f),std::clamp(raw_w,0.f,16777215.f)}};
}
}
