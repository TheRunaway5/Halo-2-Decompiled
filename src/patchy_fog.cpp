#include "halo2/atmosphere.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
float saturated(float v) {return std::clamp(v,0.f,1.f);}
std::uint32_t byte(float v) {return std::uint32_t(std::nearbyint(double(v)*255.));}
std::uint32_t rgb(const ShaderVector &v) {return byte(v[0])<<16|byte(v[1])<<8|byte(v[2]);}
std::uint32_t argb(const ShaderVector &v) {return byte(v[0])<<24|byte(v[1])<<16|byte(v[2])<<8|byte(v[3]);}
}
std::array<ShaderVector,18> patchy_fog_combiner_constants(const PixelProgram &program) {
    std::array<ShaderVector,18> out;
    // pgraph_glsl_set_psh_uniform_values / pgraph_argb_pack32_to_rgba_float:
    // paired C0/C1 stage constants, followed by the two final constants.
    for(unsigned i=0;i<out.size();++i) {
        const auto color=program.words[i<16?10+i/2+(i%2)*8:43+i-16];
        out[i]={float((color>>16)&255)/255.f,float((color>>8)&255)/255.f,
                float(color&255)/255.f,float(color>>24)/255.f};
    }
    return out;
}
PatchyFogQuad patchy_fog_noise_quad(const PatchyFogFrame &frame,float width,float height,
    float near_clip,float far_clip) {
    PatchyFogQuad out;
    const float scale=far_clip/(far_clip-near_clip);
    const float depth=std::clamp((scale-near_clip*scale)*16777215.f,0.f,16777215.f);
    const float w=std::clamp((1.f/far_clip)*16777215.f,0.f,16777215.f);
    // This color-only pass reads only clipRange.y in the original vertex
    // output conversion. Other clip/depth fields are not consumed here.
    out.clip_range={0,16777215.f,0,0};
    // The supported title capture has both inline color alphas equal to one
    // at every corner. They still interpolate: replacing fragment D0/D1
    // alpha with a literal one loses the original perspective rounding.
    out.surface={width,height,1,1};
    constexpr std::array<std::array<float,2>,4> corners{{{0,0},{0,1},{1,1},{1,0}}};
    for(unsigned corner=0;corner<4;++corner) {
        const auto t=corners[corner];
        out.attributes[corner]={width*t[0],height*t[1],depth,w};
        const float x=t[0]*2.f-1.f,y=t[1]*2.f-1.f;
        for(unsigned layer=0;layer<4;++layer) {
            const auto &r=frame.rows[layer*2],&s=frame.rows[layer*2+1];
            out.attributes[4+layer*4+corner]={
                x*r[0]-y*r[1]*frame.vertical+r[3],
                x*s[0]-y*s[1]*frame.vertical+s[3],0,1};
        }
    }
    return out;
}
PatchyFogQuad patchy_fog_direct_quad(const PatchyFog &fog,float width,float height,
    float noise_width,float noise_height,float near_clip,float far_clip) {
    PatchyFogQuad out;
    // In the captured title direct branch, 27520 uses g_509400's default
    // g_485ae0 (24-bit maximum) for 350E0's depth argument.
    constexpr float x=16777215.f;
    const float scale=far_clip/(far_clip-near_clip);
    const float depth=std::clamp(((scale*x-near_clip*scale)/x)*16777215.f,0.f,16777215.f);
    const float w=std::clamp((x/far_clip)*16777215.f,0.f,16777215.f);
    const float inverse=1.f/std::max(.0001f,fog.distance[1]-fog.distance[0]);
    const ShaderVector gradient{far_clip*inverse,0,0.f-fog.distance[0]*inverse,0};
    out.clip_range={0,16777215.f,0,0};out.surface={width,height,1,1};
    constexpr std::array<std::array<float,2>,4> corners{{{0,0},{0,1},{1,1},{1,0}}};
    for(unsigned i=0;i<4;++i) {
        const auto t=corners[i];
        out.attributes[i]={width*t[0],height*t[1],depth,w};
        out.attributes[4+i]={width*t[0],height*t[1],0,1};
        out.attributes[8+i]=out.attributes[12+i]=gradient;
        out.attributes[16+i]={noise_width*t[0],noise_height*t[1],0,1};
    }
    return out;
}
PatchyFogFrame patchy_fog_frame(const PatchyFog &fog,PatchyFogState &s,
    std::uint32_t &random,const Transform &view,float forward_z,float fov,float height_over_width,float time) {
    PatchyFogFrame out;
    const auto random_pair=[&](unsigned layer) {
        random=random*1664525u+1013904223u;
        random=random*1664525u+1013904223u;
        const float v=float(random>>16)*.000015259021893143654f;
        s.offsets[layer]={v,v}; // Original advances twice, then uses the same high word twice.
    };
    if(!s.initialized) {
        s={};for(unsigned i=0;i<4;++i)random_pair(i);
        s.previous=view;s.initialized=1;
    }
    s.time=time; // 28580: velocity maximum is zero in the retail title fpch.
    Transform wind;wind.position=fog.constant_wind;
    const auto delta=compose_transform(view,compose_transform(wind,inverse_transform(s.previous)));
    s.previous=view;
    const float elevation=float((std::cos(double(forward_z)*double(3.1415927410125732f))+1.)*.5);
    out.vertical=fog.vertical_up*(1.f-elevation)+fog.vertical_forward*elevation;
    const float opacity=fog.opacity_up*(1.f-elevation)+elevation;
    const float interval=(fog.distance[1]-fog.distance[0])*.25f;
    const float inverse=1.f/std::max(.0001f,fog.distance[1]-fog.distance[0]);
    const float half_extent=float((double(fog.noise_scale)*.5)/
        (std::tan(double(fov)*.5)*double(std::max(.0001f,fog.distance[1]))*double(height_over_width)));
    const double rotation=double(s.rotation)+std::atan2(double(delta.basis[0].y),double(delta.basis[0].x))*double(fog.rotation);
    s.rotation=float(rotation);
    const float cosine=float(std::cos(rotation)),sine=float(std::sin(rotation));
    s.fraction-=(fog.zoom/std::max(.0001f,interval))*delta.position.z;
    const float crossed=float(std::floor(double(s.fraction)));
    s.fraction-=crossed;
    s.layer=unsigned((int(s.layer)-int(crossed))%4+4)%4;
    if(crossed>0)random_pair(s.layer);
    else if(crossed<0)random_pair((s.layer+3)%4);
    auto &weights=out.weights;
    for(unsigned i=0;i<4;++i) {
        const unsigned layer=(s.layer+i)%4;
        const float distance=(float(i)+s.fraction)*interval+fog.distance[0];
        const float normalized=(distance-fog.distance[0])*inverse;
        // 28213/2821B center in binary32 before2823A loads that value
        // into x87 for the two powers. Keeping this subtraction in double
        // changes the retained source weight, even when packed RGB agrees.
        const float centered=normalized*2.f-1.f;
        const float weight=float(std::pow(1.-std::pow(std::abs(double(centered)),3.),2.)*double(opacity));
        float zero=0,back=0.f-distance;
        if(delta.scale!=1.f){zero=delta.scale*0.f;back=delta.scale*back;}
        const float x=((delta.basis[2].x*back+delta.basis[1].x*zero)+delta.basis[0].x*zero)+delta.position.x;
        const float y=((delta.basis[2].y*back+delta.basis[1].y*zero)+delta.basis[0].y*zero)+delta.position.y;
        s.offsets[layer][0]-=(((y*sine*out.vertical+x*cosine)*fog.strafe)*half_extent)*.5f;
        s.offsets[layer][1]-=(((y*cosine*out.vertical-x*sine)*fog.strafe)*half_extent)*.5f;
        weights[layer]={0,saturated(weight),0,saturated(normalized)};
        const float scale=(fog.noise_scale/std::max(.0001f,fog.distance[1]))*distance;
        const float cx=scale*cosine,sx=scale*sine;
        out.rows[layer*2]={cx*.5f,sx*height_over_width*.5f,0,s.offsets[layer][0]};
        out.rows[layer*2+1]={sx*-.5f,cx*height_over_width*.5f,0,s.offsets[layer][1]};
    }
    auto &w=out.noise.words;
    w[8]=0x0b01000c;w[9]=0x800;w[10]=rgb(weights[0]);w[18]=rgb(weights[1]);
    w[11]=rgb(weights[2]);w[19]=rgb(weights[3]);w[12]=0xff0000;w[20]=0xff00;
    w[34]=0x08010902;w[35]=0x0a010b02;w[36]=0x09010a02;
    w[43]=0xff;w[45]=0x3089;w[46]=0x30ab;w[47]=0xc00;w[53]=0x11003;w[54]=0x8421;
    auto &c=out.cached.words;
    c[0]=0x1a20b120;c[2]=0x2c2d0000;c[3]=0x24251a1a;c[4]=0x14150000;c[8]=0xc;
    c[10]=argb({weights[0][3],weights[1][3],weights[2][3],weights[3][3]});
    c[12]=0xff0000;c[20]=0xff00;
    const auto d0=byte(saturated(fog.density[0])),d1=byte(saturated(fog.density[1]));
    c[14]=d0*0x01010101;c[22]=d1*0x01010101;
    c[26]=0x10c00;c[28]=0x40;c[29]=0x58;c[30]=0xc0;
    c[34]=0x0a20a120;c[35]=0x0b0c1b1c;c[36]=0x010c020c;c[38]=0x0a010a02;c[39]=0x1c0c3c0d;
    c[45]=0x10c00;c[46]=0xcd;c[47]=0x3045;c[49]=0xcd;c[50]=0xc00;c[53]=0x11006;c[54]=0xa621;c[55]=0x44;
    out.direct=out.cached;auto &d=out.direct.words;
    d[8]=0xc010000;d[9]=0xc00;d[43]=rgb({fog.color.x,fog.color.y,fog.color.z,0});
    return out;
}
}
extern "C" void halo2_patchy_fog(const float *parameters,const float *sky,const float *input,
    std::uint32_t *random,float *state,float *output,std::uint32_t *programs) {
    halo2::PatchyFog fog{{sky[0],sky[1],sky[2]},{sky[6],sky[7]},{sky[8],sky[9]},
        std::bit_cast<std::uint32_t>(parameters[0]),parameters[1],parameters[2],parameters[3],parameters[4],
        parameters[7],parameters[8],parameters[9],parameters[10],{parameters[17],parameters[18],parameters[19]},{}};
    std::array<float,34> sw;std::copy_n(state,34,sw.begin());
    static_assert(sizeof(halo2::PatchyFogState)==sizeof(sw));
    auto s=std::bit_cast<halo2::PatchyFogState>(sw);
    std::array<float,13> vw;std::copy_n(input,13,vw.begin());
    const auto f=halo2::patchy_fog_frame(fog,s,*random,std::bit_cast<halo2::Transform>(vw),input[13],input[14],input[17]/input[16],input[15]);
    sw=std::bit_cast<std::array<float,34>>(s);std::copy(sw.begin(),sw.end(),state);
    for(unsigned i=0;i<4;++i) {
        output[i*14]=output[i*14+1]=0;
        std::copy(f.weights[i].begin(),f.weights[i].end(),output+i*14+2);
        std::copy(f.rows[i*2].begin(),f.rows[i*2].end(),output+i*14+6);
        std::copy(f.rows[i*2+1].begin(),f.rows[i*2+1].end(),output+i*14+10);
    }
    output[56]=f.vertical;
    std::copy(f.noise.words.begin(),f.noise.words.end(),programs);
    std::copy(f.cached.words.begin(),f.cached.words.end(),programs+60);
    std::copy(f.direct.words.begin(),f.direct.words.end(),programs+120);
}
