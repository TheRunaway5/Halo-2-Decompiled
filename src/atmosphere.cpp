#include "halo2/atmosphere.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
std::array<ShaderVector,6> menu_fog_constants(
    const MenuAtmosphere &fog, Vec3 eye, Vec3 forward) {
    constexpr float minimum_distance = .0001f; // retail 0045DBDC
    const float distance =
        (eye.z*forward.z + eye.y*forward.y) + eye.x*forward.x;
    const auto &plane=fog.planar;
    const float height=((eye.z*plane.plane[2]+eye.y*plane.plane[1])+
        eye.x*plane.plane[0])-plane.plane[3];
    const auto snapped=[](float value) {
        return value<.0001f?0.f:value>.9999f?1.f:value;
    };
    // 001305D0: view-dependent eye offset and default atmospheric/planar
    // transition interval (0045DF04/08), followed by 00130DE0 type 5.
    const float offset=0.f-(plane.eye_offset/plane.opaque_distance)*std::max(0.f,height);
    const bool submerged=snapped(0.f-height/plane.opaque_depth)==1.f &&
        snapped((height-1023.f)/(1024.f-1023.f))==0.f;
    const float secondary_start=submerged?offset:0.f;
    const float secondary_end=submerged?plane.opaque_distance:0.f;
    const float range=1.f/std::max(minimum_distance,
        fog.atmospheric.opaque-fog.atmospheric.start);
    const float plane_distance=1.f/std::max(minimum_distance,plane.opaque_distance);
    const float plane_depth=1.f/std::max(minimum_distance,plane.opaque_depth);
    const float secondary_range=1.f/std::max(minimum_distance,secondary_end-secondary_start);
    return {{{forward.x*range,forward.y*range,forward.z*range,
               0.f-(fog.atmospheric.start+distance)*range},
             {0.f-plane.plane[0]*plane_depth,0.f-plane.plane[1]*plane_depth,
               0.f-plane.plane[2]*plane_depth,plane.plane[3]*plane_depth},
             {forward.x*plane_distance,forward.y*plane_distance,forward.z*plane_distance,
               0.f-(offset+distance)*plane_distance},
             {std::clamp(0.f-height*plane_depth,0.f,1.f),0,
               fog.atmospheric.maximum_density,submerged?0.f:plane.maximum_density},
             {forward.x*secondary_range,forward.y*secondary_range,forward.z*secondary_range,
               0.f-(distance+secondary_start)*secondary_range},
             {submerged?plane.maximum_density:0.f,0,0,0}}};
}

MenuFogExterns menu_fog_externs(const MenuAtmosphere &fog, Vec3 eye) {
    MenuFogExterns out;
    const auto &p=fog.planar;
    const float height=((eye.z*p.plane[2]+eye.y*p.plane[1])+eye.x*p.plane[0])-p.plane[3];
    const auto snapped=[](float v){return v<.0001f?0.f:v>.9999f?1.f:v;};
    const float raw_fraction=snapped(0.f-height/p.opaque_depth);
    const float raw_transition=snapped((height-1023.f)/(1024.f-1023.f));
    const bool submerged=raw_fraction==1.f && raw_transition==0.f;
    // 130DE0 moves the plane to secondary fog, clears the planar-eye
    // fraction and sets the transition to one in classification 5.
    const float fraction=submerged?0.f:raw_fraction;
    const float transition=submerged?1.f:raw_transition;
    const float planar=submerged?0.f:p.maximum_density;
    const float secondary=submerged?p.maximum_density:0.f;
    auto &s=out.scalars;
    s[24]=fog.atmospheric.maximum_density;s[25]=transition;
    s[26]=(1.f-transition)*planar;s[27]=transition*s[24];
    s[28]=secondary;s[29]=0;s[30]=secondary;s[31]=0;
    s[33]=planar;s[34]=fraction;s[35]=(1.f-fraction)*planar;s[36]=fraction*planar;
    const auto rgb=[](Vec3 color,float density=1.f){return ShaderVector{
        color.x*density,color.y*density,color.z*density,0};};
    auto &c=out.colors;
    c[25]=rgb(fog.atmospheric.color);c[26]=rgb(fog.atmospheric.color,s[24]);
    c[27]=submerged?rgb(p.color):ShaderVector{};
    c[28]=submerged?rgb(p.color,secondary):ShaderVector{};
    c[31]=rgb(p.color);c[32]=rgb(p.color,planar);
    return out;
}
PixelProgram menu_planar_fog_program(const MaterialPass &pass,const MenuAtmosphere &fog,Vec3 eye) {
    auto program=pass.program;
    const auto externs=menu_fog_externs(fog,eye);
    remap_pixel_externs(program,externs.scalars,externs.colors,pass.pixel_externs,pass.fragments,{});
    return program;
}

}
extern "C" int halo2_menu_fog_constants(
    const float *eye,const float *forward,const float *parameters,float *output) {
    if(!eye || !forward || !parameters || !output)return 0;
    const halo2::MenuAtmosphere atmosphere{{{},parameters[2],parameters[0],parameters[1]},
        {{parameters[3],parameters[4],parameters[5],parameters[6]},
         {},parameters[7],parameters[8],parameters[9],parameters[10]}};
    const auto constants=halo2::menu_fog_constants(atmosphere,
        {eye[0],eye[1],eye[2]},{forward[0],forward[1],forward[2]});
    for(unsigned i=0;i<6;++i)for(unsigned c=0;c<4;++c)
        output[i*4+c]=constants[i][c];
    return 1;
}
extern "C" int halo2_menu_fog_externs(const float *parameters,const float *eye,float *scalars,float *colors) {
    if(!parameters || !eye || !scalars || !colors)return 0;
    const halo2::MenuAtmosphere fog{{{parameters[0],parameters[1],parameters[2]},parameters[3],parameters[4],parameters[5]},
        {{parameters[6],parameters[7],parameters[8],parameters[9]},
         {parameters[10],parameters[11],parameters[12]},parameters[13],parameters[14],parameters[15],parameters[16]}};
    const auto values=halo2::menu_fog_externs(fog,{eye[0],eye[1],eye[2]});
    std::copy(values.scalars.begin(),values.scalars.end(),scalars);
    for(unsigned i=0;i<42;++i)std::copy(values.colors[i].begin(),values.colors[i].end(),colors+i*4);
    return 1;
}
extern "C" int halo2_menu_planar_fog_program(const float *parameters,const float *eye,
    const std::uint32_t *words,const std::uint32_t *remaps,unsigned count,std::uint32_t *output) {
    if(!parameters || !eye || !words || !remaps || !output || count>128)return 0;
    try {
        halo2::MaterialPass pass;
        std::copy_n(words,60,pass.program.words.begin());
        for(unsigned i=0;i<count;++i)pass.pixel_externs.push_back({std::uint16_t(remaps[i]),
            std::uint8_t(remaps[i]>>16),std::uint8_t(remaps[i]>>24)});
        const halo2::MenuAtmosphere fog{{{parameters[0],parameters[1],parameters[2]},parameters[3],parameters[4],parameters[5]},
            {{parameters[6],parameters[7],parameters[8],parameters[9]},
             {parameters[10],parameters[11],parameters[12]},parameters[13],parameters[14],parameters[15],parameters[16]}};
        const auto result=halo2::menu_planar_fog_program(pass,fog,{eye[0],eye[1],eye[2]});
        std::copy(result.words.begin(),result.words.end(),output);
        return 1;
    }catch(const halo2::FormatError &){return 0;}
}
