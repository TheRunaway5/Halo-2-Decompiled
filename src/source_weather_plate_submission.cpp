#include "halo2/source_weather_plate_submission.hpp"
#include <bit>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <stdexcept>

#if defined(__FAST_MATH__)
#error "Original weather plate submission requires strict arithmetic"
#endif
static_assert(FLT_EVAL_METHOD==0);
namespace halo2 {
namespace {
float finite(float value) {
    if(!std::isfinite(value))throw std::invalid_argument("nonfinite weather plate submission input/intermediate");
    return value;
}
float read(std::span<const std::byte> bytes,unsigned at) {
    float value;std::memcpy(&value,bytes.data()+at,4);return finite(value);
}
float clamp_source(float value) {
    finite(value);if(value<0.f)return 0.f;if(value>1.f)return 1.f;return value;
}
}
SourceWeatherPlateSubmission source_weather_plate_submission(const SourceWeatherPlateSubmissionInput &in) {
    if(in.plate.size()!=0x3a8 || in.slot<0 || in.slot>3)
        throw std::invalid_argument("weather plate submission requires a complete current plate and admitted view slot");
    if(in.rectangle[2]<in.rectangle[0] || in.rectangle[3]<in.rectangle[1])
        throw std::invalid_argument("weather plate submission requires an ordered source rectangle");
    const unsigned view=unsigned(in.slot)*0xb8;
    SourceWeatherPlateSubmission out;
    out.caller.depth_start=read(in.plate,0x80);out.caller.depth_end=read(in.plate,0x84);
    out.caller.selection_near=in.selection[0];out.caller.selection_far=in.selection[1];
    out.caller.projection_near=in.projection[0];out.caller.projection_far=in.projection[1];
    out.caller.source_fpcw=in.source_fpcw;out.caller.source_mxcsr=in.source_mxcsr;
    // Original 37A60 leaves three stack padding bytes undefined. Only byte0
    // is consumed by 3A5C0; zero padding is not an inferred source stack value.
    out.caller.compositor.words[0]=1;
    const float gain=finite(read(in.plate,view+0x14c)*read(in.plate,view+0x148));
    for(unsigned layer=0;layer<3;++layer) {
        const float scale=read(in.plate,0x48+layer*4);
        out.scroll[layer]={finite(read(in.plate,view+0x11c+layer*8)*scale),
            finite(read(in.plate,view+0x120+layer*8)*scale),0,0};
        out.caller.compositor.words[2+layer*3]=std::bit_cast<std::uint32_t>(
            clamp_source(finite(read(in.plate,view+0xec+layer*4)*gain)));
        out.caller.compositor.words[10+layer]=std::bit_cast<std::uint32_t>(
            clamp_source(read(in.plate,0x98+layer*4)));
    }
    out.result=source_weather_plate_caller_profile(out.caller);
    if(out.result.profile==SourceWeatherPlateCallerProfile::NoDraw)return out;
    const auto zw=*out.result.projected_zw;
    const float left=float(in.rectangle[1]),top=float(in.rectangle[0]);
    const float width=float(int(in.rectangle[3])-int(in.rectangle[1]));
    const float height=float(int(in.rectangle[2])-int(in.rectangle[0]));
    const std::array<float,2> x{left-.5f,(width+left)-.5f};
    const std::array<float,2> y{top-.5f,(height+top)-.5f};
    for(unsigned vertex=0;vertex<4;++vertex) {
        const unsigned right=vertex&1,bottom=vertex>>1;
        auto &v=out.vertices[vertex].registers;
        v[0]={x[right],y[bottom],zw[0],zw[1]};v[1]={x[right],y[bottom],0,1};
        for(unsigned layer=0;layer<3;++layer) {
            const float positive=read(in.plate,view+0x134+layer*4);
            const float negative=finite(positive*-1.f);
            v[layer+2]={right?positive:negative,bottom?positive:negative,0,1};
        }
    }
    return out;
}
}
