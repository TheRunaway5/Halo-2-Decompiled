#include "halo2/source_loop_track_control_tu15.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
float source_loop_track_gain_control_tu15(float current,float target,float rate,float elapsed) {
    if(.0001f>std::fabs(static_cast<long double>(rate)))return target;
    const float step=elapsed*rate,delta=target-current,lower=0.f-step;
    const float selected=lower>delta?lower:delta>step?step:delta;
    return selected+current;
}
namespace {
float fraction(float a,float b,float c,bool flag) {
    if(!(.0001f>std::fabs(static_cast<long double>(a)-b))) {
        const float difference=c-a,range=b-a,value=difference/range;
        return value<0?0:value>1?1:value;
    }
    return ((a>c)==flag)?0.f:1.f;
}
float layer_db(float t) {
    // Original2201C0 ->220060 curve2: linear power between -64 and0dB.
    // Preserve the single stores between each arithmetic operation.
    const float lower=float(std::exp(double(-64.f*.05f)*double(2.3025851f)));
    const float upper=1.f,blend=float(std::sqrt(double(t)));
    const float range=upper-lower,product=range*blend,value=product+lower;
    const float selected=value<0?0:value>1?1:value;
    const float db=value>0?float(20.f*std::log10(double(selected))):-64.f;
    return db< -64.f?-64.f:db>0?0.f:db;
}
}
SourceLoopPitchLayerTu15 source_loop_pitch_layer_tu15(
    const std::array<std::int16_t,5>& interval,float pitch,bool layered) {
    const float bend_lower=interval[1],bend_upper=interval[2];
    const auto bounded=bend_lower>pitch?bend_lower:pitch>bend_upper?bend_upper:pitch;
    SourceLoopPitchLayerTu15 result{bounded==pitch,0};
    if(layered) {
        const float low=interval[3],high=interval[4];
        if(!(.0001f>std::fabs(static_cast<long double>(pitch)-low)) && low>pitch)
            result.gain_db=layer_db(fraction(bend_lower,low,pitch,true));
        else if(!(.0001f>std::fabs(static_cast<long double>(pitch)-high)) && pitch>high)
            result.gain_db=layer_db(fraction(bend_upper,high,pitch,false));
    }
    return result;
}
}
