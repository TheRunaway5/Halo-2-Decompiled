#include "halo2/atmosphere.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
namespace {
float saturated(float v) { return std::clamp(v,0.f,1.f); }
float snapped(float v) { return v<.0001f?0.f:v>.9999f?1.f:v; }
std::uint32_t byte(float v) {
    // 00131E50/00131FC0 multiply on x87 and round to nearest-even.
    return std::uint32_t(std::nearbyint(double(saturated(v))*255.0));
}
std::uint32_t packed(float a, Vec3 rgb) {
    return byte(a)<<24 | byte(rgb.x)<<16 | byte(rgb.y)<<8 | byte(rgb.z);
}
Vec3 premultiplied(Vec3 v,float a) {
    return {saturated(v.x*a),saturated(v.y*a),saturated(v.z*a)};
}
}
MenuFogComposite menu_fog_composite(const MenuAtmosphere &fog,Vec3 eye,float far) {
    MenuFogComposite result;
    auto &w=result.program.words;
    const auto &p=fog.planar;
    const float height=((eye.z*p.plane[2]+eye.y*p.plane[1])+eye.x*p.plane[0])-p.plane[3];
    const float fraction=snapped(0.f-height/p.opaque_depth);
    const float transition=snapped((height-1023.f)/(1024.f-1023.f));
    const bool submerged=fraction==1.f && transition==0.f;
    const float eye_opacity=snapped(1.f-saturated(1.f-fraction*p.maximum_density));
    const auto eye_rgb=premultiplied(p.color,saturated(fraction*p.maximum_density));
    const auto primary_rgb=premultiplied(fog.atmospheric.color,fog.atmospheric.maximum_density);
    // 0003A5C0 quantizes the two normalized endpoints before calculating
    // its reciprocal. Mode 1 rejects reciprocals above 64 with these flags.
    const float start=float(std::floor(double((1.f/far)*fog.atmospheric.start*65536.f)))*.0000152587890625f;
    const float end=float(std::floor(double((1.f/far)*fog.atmospheric.opaque*65536.f)))*.0000152587890625f;
    const float reciprocal=1.f/(end-start);
    if(!submerged && end>start && reciprocal<=64.f && start>=0.f) {
        unsigned power=0;
        while(float(1u<<power)<reciprocal)++power;
        const auto q=std::uint32_t(std::floor(double(start*65536.f)));
        const auto low=std::min(q&255u,255u),high=std::min(q>>8,255u);
        w[0]=0x08200000;w[1]=0x04209140;w[2]=0x18201140;w[3]=0xdd202df1;
        w[4]=0x1d201d00|(power>=4?0x20:0);w[5]=0x1d201220;
        w[6]=0x1d120c02;w[7]=0x11141d05;w[8]=0x0114000f;w[9]=0x1d041c00;
        w[10]=0x00ff0000;w[11]=low<<24;w[12]=high<<24;w[13]=0x01000000;
        w[16]=packed(fog.atmospheric.maximum_density,primary_rgb);
        w[17]=packed(eye_opacity,eye_rgb);w[18]=0x0000ff00;
        const auto coefficient=[](float v) {return std::uint32_t(std::clamp(std::floor(double(v)),0.,255.));};
        w[24]=coefficient(float(double(reciprocal)*256./double(1u<<power)))<<24 |
              coefficient(reciprocal*.5f);
        w[26]=0x40;w[27]=0xc00;w[28]=0xd00;w[29]=0xd00;
        const auto scale=[](unsigned exponent,unsigned low_boundary,unsigned high_boundary) {
            return exponent>=high_boundary?0x20u:exponent>=low_boundary?0x10u:0u;
        };
        w[30]=scale(power,2,3)<<12|0xd00;
        w[31]=scale(power,5,6)<<12|0xd00;
        w[32]=(power>=1?0x10u:0u)<<12|0xd00;w[33]=0xc00;
        w[34]=0x08010802;w[35]=0x04201140;w[36]=0x2020;w[37]=0xcc202d20;
        w[40]=0x01341134;w[43]=w[17];
        w[45]=0x3045;w[46]=0xc00;w[47]=0x4d00;w[48]=0xc00;w[51]=0x45;
        w[53]=0x11108;w[54]=1;
    } else {
        const float secondary_density=submerged?p.maximum_density:0.f;
        // 25E50 reads state+0x11c, not the classification at +0x118.
        // The bounded title state leaves this interpolation input at zero.
        const float blend=0.f;
        const auto secondary_rgb=premultiplied(p.color,secondary_density);
        w[0]=0x111a120a;w[1]=0x110a121a;w[2]=0x3c3d0820;w[3]=0x3d3c1d11;
        w[8]=0xc;w[9]=0x1c00;
        w[10]=packed(saturated((1.f-blend)*secondary_density),primary_rgb);
        w[11]=byte(fog.atmospheric.maximum_density)<<24;
        w[13]=packed(eye_opacity,eye_rgb);
        w[18]=packed(saturated(fog.atmospheric.maximum_density*blend),secondary_rgb);
        w[19]=byte(secondary_density)<<24;
        w[26]=w[27]=w[28]=0xcd;w[29]=0xc00;
        w[34]=0x010a021a;w[35]=0x0c3c0d3d;w[37]=0x3d0c1d01;
        w[45]=0xcd;w[46]=w[48]=0xc00;w[53]=0x11004;w[54]=0x2621;w[55]=0x44;
        const float secondary_start=submerged?0.f-(p.eye_offset/p.opaque_distance)*std::max(0.f,height):0.f;
        const float secondary_end=submerged?p.opaque_distance:0.f;
        const float primary_range=1.f/std::max(.0001f,fog.atmospheric.opaque-fog.atmospheric.start);
        const float secondary_range=1.f/std::max(.0001f,secondary_end-secondary_start);
        result.lookup_coordinates={{{far*primary_range,0,0.f-fog.atmospheric.start*primary_range,0},
            {far*secondary_range,0,0.f-secondary_start*secondary_range,0}}};
    }
    return result;
}
std::optional<MenuFogComposite> menu_fog_draw(const MenuAtmosphere &fog,Vec3 eye,float far) {
    const auto &p=fog.planar;
    const float height=((eye.z*p.plane[2]+eye.y*p.plane[1])+eye.x*p.plane[0])-p.plane[3];
    const float fraction=snapped(0.f-height/p.opaque_depth);
    const float transition=snapped((height-1023.f)/(1024.f-1023.f));
    if(fraction==1.f && transition==0.f)return menu_fog_composite(fog,eye,far);
    // 1301C0's eye color precedes 130DE0's classification. 26520 packs
    // that premultiplied color through 131F40, then draws at maximum W.
    const float opacity=snapped(1.f-saturated(1.f-fraction*p.maximum_density));
    if(opacity<=0.f)return std::nullopt; // 0045DBD8 is exactly zero.
    MenuFogComposite result;
    auto &w=result.program.words;
    w[8]=0x01200000;w[9]=0x1100;
    w[43]=packed(opacity,premultiplied(p.color,saturated(fraction*p.maximum_density)));
    w[53]=0x11001;
    return result;
}
}
extern "C" int halo2_menu_fog_composite(const float *parameters,const float *eye,
                                        std::uint32_t *output,float *coordinates) {
    if(!parameters || !eye || !output || !coordinates)return 0;
    const halo2::MenuAtmosphere fog{{{parameters[0],parameters[1],parameters[2]},parameters[3],parameters[4],parameters[5]},
        {{parameters[6],parameters[7],parameters[8],parameters[9]},
         {parameters[10],parameters[11],parameters[12]},parameters[13],parameters[14],parameters[15],parameters[16]}};
    const auto result=halo2::menu_fog_composite(fog,{eye[0],eye[1],eye[2]});
    std::copy(result.program.words.begin(),result.program.words.end(),output);
    for(unsigned i=0;i<2;++i)std::copy(result.lookup_coordinates[i].begin(),result.lookup_coordinates[i].end(),coordinates+i*4);
    return 1;
}
extern "C" int halo2_menu_fog_draw(const float *parameters,const float *eye,
                                   std::uint32_t *output,float *coordinates) {
    if(!parameters || !eye || !output || !coordinates)return -1;
    const halo2::MenuAtmosphere fog{{{parameters[0],parameters[1],parameters[2]},parameters[3],parameters[4],parameters[5]},
        {{parameters[6],parameters[7],parameters[8],parameters[9]},
         {parameters[10],parameters[11],parameters[12]},parameters[13],parameters[14],parameters[15],parameters[16]}};
    const auto result=halo2::menu_fog_draw(fog,{eye[0],eye[1],eye[2]});
    if(!result)return 0;
    std::copy(result->program.words.begin(),result->program.words.end(),output);
    for(unsigned i=0;i<2;++i)std::copy(result->lookup_coordinates[i].begin(),result->lookup_coordinates[i].end(),coordinates+i*4);
    return 1;
}
