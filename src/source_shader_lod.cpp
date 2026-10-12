#include "halo2/source_shader_lod.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_view_projection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

namespace halo2 {
namespace {
std::optional<float> original_nan(float a,float b) {
    const auto x=std::bit_cast<std::uint32_t>(a),y=std::bit_cast<std::uint32_t>(b);
    const auto nan=[](std::uint32_t v){return (v&0x7fffffffu)>0x7f800000u;};
    const bool nx=nan(x),ny=nan(y);
    if(!nx && !ny)return {};
    // Original execution's SSE NaN selection: a quiet operand precedes a
    // signaling operand; equal classes select greater significand, positive
    // on a tie. Preserve the measured result across host compiler rewrites.
    const auto qx=x|0x00400000u,qy=y|0x00400000u;
    if(!nx)return std::bit_cast<float>(qy);
    if(!ny)return std::bit_cast<float>(qx);
    if(bool(x&0x00400000u)!=bool(y&0x00400000u))
        return std::bit_cast<float>((x&0x00400000u)?qx:qy);
    return std::bit_cast<float>((qx<<1)>(qy<<1)?qx:(qx<<1)<(qy<<1)?qy:std::min(qx,qy));
}
float mul(float a,float b){if(auto n=original_nan(a,b))return *n;const volatile float r=a*b;return r;}
float div(float a,float b){if(auto n=original_nan(a,b))return *n;const volatile float r=a/b;return r;}
float add(float a,float b){if(auto n=original_nan(a,b))return *n;const volatile float r=a+b;return r;}
float sub(float a,float b){if(auto n=original_nan(a,b))return *n;const volatile float r=a-b;return r;}
float depth(float d) {
    if(!(d>=0))d=sub(0,d);
    const auto minimum=std::bit_cast<float>(0x3dcccccdu);
    return d>minimum?d:minimum;
}
}
SourceShaderLodView source_shader_lod_view(std::int16_t top,std::int16_t bottom,
    bool present,bool auxiliary) {
    const float height=float(std::int32_t(bottom)-std::int32_t(top));
    const float twice=div(2,height),zero=div(0,height),once=div(1,height),four=div(4,height);
    SourceShaderLodView view{{once,zero,zero,twice,four,zero,once}};
    if(present && auxiliary)view.multipliers[6]=mul(once,2);
    return view;
}
float source_shader_lod_player_multiplier(unsigned players) {
    return source_geometry_lod_policy(players).effect_scale;
}
SourceShaderCoverageCamera source_shader_coverage_camera(const SourceViewProjectionBank &bank) {
    return {{bank.inverse_view.basis[0].z,bank.inverse_view.basis[1].z,
        bank.inverse_view.basis[2].z,bank.inverse_view.position.z},bank.viewport_half_metrics.y};
}
float source_shader_cluster_coverage(const std::array<float,6> &bounds,const SourceShaderCoverageCamera &camera) {
    const Vec3 center{mul(add(bounds[0],bounds[1]),.5f),mul(add(bounds[2],bounds[3]),.5f),mul(add(bounds[4],bounds[5]),.5f)};
    const double x=sub(bounds[1],center.x),y=sub(bounds[3],center.y),z=sub(bounds[5],center.z);
    const float radius=float(std::sqrt((x*x+y*y)+z*z));
    const auto &p=camera.inverse_view_z;
    const float distance=depth(add(add(add(mul(p[2],center.z),mul(p[0],center.x)),mul(p[1],center.y)),p[3]));
    return mul(mul(div(camera.viewport_height_metric,distance),radius),2);
}
float source_shader_instance_coverage(Vec3 center,float radius,const SourceShaderCoverageCamera &camera) {
    const auto &p=camera.inverse_view_z;
    const float distance=depth(add(add(add(mul(center.y,p[1]),mul(center.z,p[2])),mul(center.x,p[0])),p[3]));
    float coverage=mul(mul(div(radius,distance),camera.viewport_height_metric),2);
    if(std::bit_cast<float>(0x3e051eb8u)>radius)coverage=mul(coverage,1.75f);
    return coverage;
}
std::uint8_t source_shader_instance_alpha(float coverage) {
    float value=mul(sub(coverage,25),std::bit_cast<float>(0x3e124925u));
    if(value<0)value=0;
    else if(value>1)value=1;
    // The original spills the clamped SSE scalar, then multiplies by255 in
    // x87 PC53 before the complete322B0C truncation. NaN produces AL0.
    return std::isfinite(value)?std::uint8_t(double(value)*255):0;
}
SourceShaderStipple source_shader_instance_stipple(std::uint8_t alpha) {
    const float fraction=mul(float(alpha),std::bit_cast<float>(0x3b808081u));
    const float value=mul(fraction,16);
    unsigned level=unsigned(value);
    const float residual=sub(value,float(level));
    if(residual>.5f || (residual==.5f && (level&1)))++level;
    constexpr std::array<std::array<unsigned,2>,16> positions{{
        {0,0},{2,2},{0,2},{2,0},{1,1},{3,3},{3,1},{1,3},
        {0,1},{2,3},{3,0},{1,2},{1,0},{3,2},{0,3},{2,1}}};
    SourceShaderStipple result;result.level=std::uint8_t(level);
    for(unsigned y=0;y<32;y+=4)for(unsigned x=0;x<32;x+=4)
        for(unsigned i=0;i<level;++i)
            result.rows[y+positions[i][1]]|=1u<<(x+positions[i][0]);
    return result;
}
bool source_shader_layer_stipple(unsigned layer,std::int32_t type) {
    if(layer>=25)throw std::invalid_argument("source shader layer outside44370 table");
    if(type>=4 && type<=7)return false;
    return layer==1 || layer==2 || layer==3 || layer==15 || layer==18;
}
float source_shader_object_coverage(Vec3 center,float radius,float multiplier,const SourceShaderCoverageCamera &camera) {
    const auto &p=camera.inverse_view_z;
    const float distance=depth(add(add(add(mul(center.z,p[2]),mul(center.y,p[1])),mul(center.x,p[0])),p[3]));
    return mul(mul(mul(div(camera.viewport_height_metric,distance),radius),2),multiplier);
}
std::uint8_t source_shader_coverage_byte(float coverage) {
    const float value=mul(coverage,.25f);
    const auto invalid=std::numeric_limits<std::int32_t>::min();
    const std::int32_t truncated=std::isfinite(value) && value>=-2147483648.f && value<2147483648.f?
        std::int32_t(value):invalid;
    const bool correction=value<0 && float(truncated)!=value;
    const auto floored=std::bit_cast<std::int32_t>(std::uint32_t(truncated)-unsigned(correction));
    if(floored<0)return 0;
    if(floored>255)return 255;
    return std::uint8_t(floored);
}
float source_shader_part_metric(std::uint8_t value,float scale,float players) {
    return mul(mul(mul(float(value),scale),players),4);
}
SourceShaderLodChoice source_shader_lod_choice(std::span<const float> thresholds,
    unsigned bias,float metric,const SourceShaderLodView &view,bool modifier) {
    if(bias>=view.multipliers.size() || thresholds.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))
        throw std::invalid_argument("source shader LOD bank or bias outside source range");
    if(modifier)metric=mul(metric,2);
    std::int32_t selected=0;
    if(bias!=5) {
        const float coverage=mul(view.multipliers[bias],metric);
        for(const float threshold:thresholds) {
            if(coverage>=threshold)break;
            ++selected;
        }
    }
    const auto count=std::int32_t(thresholds.size());
    if(selected<0)return {0,count>0};
    if(selected>count-1)selected=count-1;
    return {selected,selected<count};
}
}
