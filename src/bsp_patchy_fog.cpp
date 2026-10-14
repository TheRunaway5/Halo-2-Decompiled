#include "halo2/bsp_atmosphere.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
float saturated(float v) {return std::clamp(v,0.f,1.f);}
std::uint32_t byte(float v) {return std::uint32_t(std::nearbyint(double(saturated(v))*255.));}
std::uint32_t rgb(const ShaderVector &v) {return byte(v[0])<<16|byte(v[1])<<8|byte(v[2]);}
std::uint32_t next_random(std::uint32_t &random) {return random=random*1664525u+1013904223u;}
std::optional<PixelProgram> quantized_patchy(const PatchyFog &fog,bool cached,float far,
    float *minimum_distance=nullptr) {
    // 3A5C0 mode 2, called by 26E50 only for the shared-layer-depth path.
    float start=(1.f/far)*fog.distance[0],end=(1.f/far)*fog.distance[1];
    const bool low_byte=end*256.f<=1.f;
    if(low_byte) {start*=256.f;end*=256.f;}
    start=float(std::floor(double(start*65536.f)))*.0000152587890625f;
    end=float(std::floor(double(end*65536.f)))*.0000152587890625f;
    if(end<=start || start<0)return {};
    float reciprocal=1.f/(end-start);
    if(reciprocal>128.f) {if(low_byte)return {};reciprocal=128.f;}
    unsigned power=0;while(float(1u<<power)<reciprocal)++power;
    const unsigned endpoint=std::uint32_t(std::floor(double(start*65536.f)));
    PixelProgram result;auto &w=result.words;
    w[1]=(low_byte?0x05209140u:0x04209140u);
    w[2]=(low_byte?0x04201140u:0x18201140u);w[3]=0xdd202df1;
    w[4]=0x18011120;w[5]=0x0d200d00|(power>=7?0x20:0);
    w[6]=0x1d120c02;w[7]=0x1d0d0d20;
    w[8]=cached?0x1d:0x1d010000;w[9]=cached?0:0x1d00;
    w[10]=0xff0000;w[11]=std::min(endpoint&255u,255u)<<24;
    w[12]=std::min(endpoint>>8,255u)<<24;w[13]=0x01000000;
    w[14]=low_byte?0x7f0000ff:0;w[15]=byte(fog.density[0])<<24;
    w[18]=0xff00;w[23]=byte(fog.density[1])<<24;
    const auto coefficient=[](float v){return std::uint32_t(std::clamp(std::floor(double(v)),0.,255.));};
    w[24]=coefficient(float(double(reciprocal)*256./double(1u<<power)))<<24|coefficient(reciprocal*.5f);
    w[27]=0xc00;w[28]=w[29]=0xd00;w[30]=0xc00;
    const auto scale=[](unsigned p,unsigned low,unsigned high){return p>=high?0x20u:p>=low?0x10u:0u;};
    w[31]=scale(power,5,6)<<12|0xd00;w[32]=(power>=1?0x10u:0u)<<12|0xd00;w[33]=0x4d00;
    w[34]=0x08010802;w[35]=(low_byte?0x05201140u:0x04201140u);
    w[36]=0x2020;w[37]=0xcc202d20;w[38]=0x1d201d00|(power>=4?0x20:0);
    w[39]=0x39111912;w[43]=cached?0:rgb({fog.color.x,fog.color.y,fog.color.z,0});
    w[45]=0x3045;w[46]=0xc00;w[47]=0x4d00;w[48]=0xc00;
    w[49]=scale(power,2,3)<<12|0xd00;w[50]=0xd00;w[53]=0x11108;w[54]=0x21;
    // 3B830..3B840 reports the quantized range start to 27520, which
    // places a LESSEQUAL, non-writing W-depth quad at that distance.
    if(minimum_distance) {
        *minimum_distance=far*start;
        if(low_byte)*minimum_distance*=.00390625f;
    }
    return result;
}
}
BspPatchyFrame bsp_patchy_fog_frame(const BspPatchyDefinition &definition,const PatchyFog &sample,
    PatchyFogState &state,std::uint32_t &random,const Transform &view,float forward_z,
    float fov,float height_over_width,float time,float far,float near) {
    if(sample.noise.empty())throw FormatError("patchy fog has no noise bitmaps");
    if(!state.initialized) {
        state={};
        for(auto &offset:state.offsets) {
            next_random(random);next_random(random);
            const float value=float(random>>16)*.000015259021893143654f;
            offset={value,value};
        }
        state.previous=view;state.initialized=1;
    }
    // 28580: smooth direction/speed once per rendered frame, then generate
    // four unsynchronized random values when the authored wind period expires.
    auto &wind=state.wind;const float elapsed=time-state.time;
    if(definition.wind_velocity[1]>0) {
        const float weight=definition.wind_acceleration,remaining=1.f-weight;
        wind[0]=wind[2]*weight+wind[0]*remaining;
        wind[1]=wind[3]*weight+wind[1]*remaining;
        float magnitude=float(std::sqrt(double(wind[0])*wind[0]+double(wind[1])*wind[1]));
        if(std::abs(magnitude)<.0001f)magnitude=0;
        else {const float inverse=1.f/magnitude;wind[0]*=inverse;wind[1]*=inverse;}
        if(magnitude==0) {wind[0]=1;wind[1]=0;}
        wind[4]=remaining*wind[4]+weight*wind[5];
        if(time-wind[6]>=wind[7]) {
            const double unit=double(next_random(random)>>16)*double(.000015259021893143654f);
            const float perpendicular=float(std::pow(unit,1.-double(definition.wind_perpendicular)));
            const float sign=(next_random(random)&0x80000000u)?-1.f:1.f;
            const float side=sign*perpendicular,forward=1.f-perpendicular;
            wind[2]=wind[0]*forward+side*(0.f-wind[1]);
            wind[3]=wind[1]*forward+wind[0]*side;
            const float target_magnitude=float(std::sqrt(double(wind[3])*wind[3]+double(wind[2])*wind[2]));
            if(std::abs(target_magnitude)>=.0001f) {
                const float inverse=1.f/target_magnitude;wind[2]*=inverse;wind[3]*=inverse;
            }
            if(target_magnitude==0) {wind[2]=1;wind[3]=0;}
            const auto range=[&](const std::array<float,2> &r) {
                const double unit=double(next_random(random)>>16)*double(.000015259021893143654f);
                return float((double(r[1])-r[0])*unit+r[0]);
            };
            wind[5]=range(definition.wind_velocity);wind[6]=time;wind[7]=range(definition.wind_period);
        }
    }
    auto fog=sample;
    fog.constant_wind=fog.constant_wind+Vec3{(wind[0]*wind[4])*elapsed,(wind[1]*wind[4])*elapsed,0};
    BspPatchyFrame result;
    result.shading=patchy_fog_frame(fog,state,random,view,forward_z,fov,height_over_width,time);
    constexpr std::array<float,4> phases{0,.7135000228881836f,.34220001101493835f,.579800009727478f};
    const unsigned count=unsigned(fog.noise.size());
    for(unsigned i=0;i<4;++i) {
        if(fog.animation_period<=0)result.bitmaps[i]=i%count;
        else {
            const float frame=time/fog.animation_period+float(count)*phases[i];
            const float integral=float(std::floor(double(frame)));
            result.bitmaps[i]=std::max(0,int(integral)%int(count));
            const float fraction=frame-integral;
            result.interpolation[i]=saturated(fraction);
        }
        if(fog.animation_period!=0) {
            const float f=result.interpolation[i],back=1.f-f;
            auto &weights=result.shading.weights[i];const float weight=weights[1];
            weights[0]=saturated((back*back)*weight);
            weights[1]=saturated(((f*2.f)*back)*weight);
            weights[2]=saturated((f*f)*weight);
        }
    }
    auto &noise=result.shading.noise.words;
    noise[10]=rgb(result.shading.weights[0]);noise[18]=rgb(result.shading.weights[1]);
    noise[11]=rgb(result.shading.weights[2]);noise[19]=rgb(result.shading.weights[3]);
    if(!(fog.flags&1)) {
        noise[8]=0x2c;noise[9]=0x2c00;noise[12]=noise[20]=noise[43]=0;
        noise[36]=0x28292a2b;noise[37]=0x0c0d0000;noise[47]=0xcd;noise[48]=0xc0;noise[53]=0x11004;
        float minimum_distance{};
        if(const auto cached=quantized_patchy(fog,true,far,&minimum_distance)) {
            result.shading.cached=*cached;result.shading.direct=*quantized_patchy(fog,false,far);
            result.quantized_composite=true;
            // 3B850..3B864 disables Z when the near clip reaches the start.
            if(near<minimum_distance)
                result.composite_minimum_depth=std::clamp((minimum_distance/far)*16777215.f,
                    0.f,16777215.f);
        }
    }
    return result;
}
} // namespace halo2
// Original-instruction differential ABI; runtime owns the typed state above.
extern "C" void halo2_bsp_patchy_fog(const float *parameters,const float *sky,const float *input,
    std::uint32_t *random,float *state,float *output,std::uint32_t *programs) {
    halo2::PatchyFog fog{{sky[0],sky[1],sky[2]},{sky[6],sky[7]},{sky[8],sky[9]},
        std::bit_cast<std::uint32_t>(parameters[0]),parameters[1],parameters[2],parameters[3],parameters[4],
        parameters[7],parameters[8],parameters[9],parameters[10],{parameters[17],parameters[18],parameters[19]},
        std::vector<halo2::Texture>(unsigned(input[18]))};
    const halo2::BspPatchyDefinition definition{0,fog,{parameters[11],parameters[12]},
        {parameters[13],parameters[14]},parameters[15],parameters[16]};
    std::array<float,34> words;std::copy_n(state,34,words.begin());
    auto native_state=std::bit_cast<halo2::PatchyFogState>(words);
    std::array<float,13> transform;std::copy_n(input,13,transform.begin());
    const auto frame=halo2::bsp_patchy_fog_frame(definition,fog,native_state,*random,
        std::bit_cast<halo2::Transform>(transform),input[13],input[14],input[17]/input[16],input[15],input[19]);
    words=std::bit_cast<std::array<float,34>>(native_state);std::copy(words.begin(),words.end(),state);
    for(unsigned i=0;i<4;++i) {
        output[i*14]=std::bit_cast<float>(frame.bitmaps[i]);output[i*14+1]=frame.interpolation[i];
        std::copy(frame.shading.weights[i].begin(),frame.shading.weights[i].end(),output+i*14+2);
        std::copy(frame.shading.rows[i*2].begin(),frame.shading.rows[i*2].end(),output+i*14+6);
        std::copy(frame.shading.rows[i*2+1].begin(),frame.shading.rows[i*2+1].end(),output+i*14+10);
    }
    output[56]=frame.shading.vertical;
    std::copy(frame.shading.noise.words.begin(),frame.shading.noise.words.end(),programs);
    std::copy(frame.shading.cached.words.begin(),frame.shading.cached.words.end(),programs+60);
    std::copy(frame.shading.direct.words.begin(),frame.shading.direct.words.end(),programs+120);
}
