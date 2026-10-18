#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
// TU1.5 183AF0 (vector3d_pack in the original source): clamp, multiply in
// single precision, then round down. GPU normalization is a separate step.
inline std::uint32_t pack_bsp_incident_tu15(Vec3 value) {
    const auto axis=[](float component,float scale,std::uint32_t mask) {
        if(!std::isfinite(component))throw FormatError("nonfinite MCC incident direction");
        const float product=std::clamp(component,-1.f,1.f)*scale;
        return std::uint32_t(std::int32_t(std::floor(product)))&mask;
    };
    return axis(value.x,1023.5f,0x7ffu)|(axis(value.y,1023.5f,0x7ffu)<<11)|
        (axis(value.z,511.5f,0x3ffu)<<22);
}
// Canonical inverse of D2DC0's signed SHORT2 midpoint representation. This
// recovers the authored Xbox codes from the paired MCC FLOAT2 streams. Values
// outside that representable domain require an explicit format conversion.
inline std::int16_t pack_bsp_lightmap_uv_tu15(float value) {
    if(!std::isfinite(value)||value<-1.f||value>1.f)
        throw FormatError("MCC lightmap UV outside Xbox SHORT2 domain");
    const float product=value*32767.5f;
    return std::int16_t(std::int32_t(std::floor(product)));
}
// Xbox NORMPACKED3: three signed normalized components (11, 11, 10).
// The bucket's incident direction stream is the same declaration as the
// cluster incident stream; its length carries authored directional strength.
inline Vec3 decode_bsp_incident(std::uint32_t packed) {
    auto axis=[](std::uint32_t value,unsigned bits) {
        const auto sign=1u<<(bits-1),mask=(1u<<bits)-1;
        const auto integer=std::int32_t((value&mask)^sign)-std::int32_t(sign);
        // The supplied NV2A GPU reference divides the signed code directly,
        // including the minimum negative encoding. Do not clamp that code;
        // retail declares X_D3DVSDT_NORMPACKED3 (0x35790 -> datatype 0x16).
        return float(integer)/float(sign-1);
    };
    return {axis(packed,11),axis(packed>>11,11),axis(packed>>22,10)};
}

// Retail D3630 reads format 47 as consecutive R, G, B bytes and multiplies
// each by the original 1/255 constant. This is not a BGRA bitmap palette.
inline Vec3 decode_bsp_radiance(const std::array<std::uint8_t,3>& rgb) {
    constexpr float scale=0.003921568859368563f;
    return {float(rgb[0])*scale,float(rgb[1])*scale,float(rgb[2])*scale};
}
inline std::size_t bsp_incident_count(const BspLightingBucket& bucket) {
    return bucket.incident_vectors.empty()?bucket.incident.size():bucket.incident_vectors.size();
}
inline Vec3 bsp_incident(const BspLightingBucket& bucket,std::size_t index) {
    return bucket.incident_vectors.empty()?decode_bsp_incident(bucket.incident.at(index)):bucket.incident_vectors.at(index);
}
} // namespace halo2
