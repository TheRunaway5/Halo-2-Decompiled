#include "halo2/bloom.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {
CinematicBloomParameters cinematic_bloom_parameters(float threshold,float strength,
    bool cinematic,const CinematicBloomOverride &override) {
    if(cinematic && override.enabled) {
        if(override.threshold!=-1.f)threshold=override.threshold;
        if(override.brightness!=-1.f)strength=override.brightness;
    }
    return {std::clamp(threshold,0.f,1.f),std::clamp(strength,0.f,1.f)};
}
MenuBloom menu_bloom(float threshold,float strength) {
    MenuBloom out;
    const auto byte=[](float v){return std::uint32_t(std::nearbyint(double(std::clamp(v,0.f,1.f))*255.));};
    auto &t=out.threshold.words;
    t[0]=0x18201920;t[1]=0x1a201b20;t[2]=0x1c201d20;t[3]=0x1c110000;
    t[8]=0x000f000d;t[9]=0xc110000;t[13]=0x20000000|byte(threshold)*0x010101;
    t[26]=0x30c00;t[27]=0x30d00;t[28]=0x30c00;t[29]=0x10d00;
    t[34]=0x08200920;t[35]=0x0a200b20;t[36]=0x0c200d20;t[37]=0x0c200140;
    t[45]=0x30c00;t[46]=0x30d00;t[47]=0x30c00;t[48]=0x20d00;t[53]=0x11004;t[54]=0x8421;
    auto &b=out.blur.words;
    b[0]=0x18201920;b[1]=0x1a201b20;b[2]=0x1c121d12;b[8]=0xc;b[9]=0x1c00;
    b[20]=byte((.4f+1.f)*.5f)<<24;
    b[26]=0x30c00;b[27]=0x30d00;b[28]=0xc00;
    b[34]=0x08200920;b[35]=0x0a200b20;b[36]=0x0c200d20;
    b[45]=0x30c00;b[46]=0x30d00;b[47]=0x30c00;b[53]=0x11003;b[54]=0x8421;
    auto &c=out.composite.words;
    c[8]=0xc;c[10]=0xff000000|byte(strength)*0x010101;c[34]=0x08011800;c[45]=0x10c00;c[53]=0x11001;c[54]=1;
    out.radii[0]=.5f;float step=1.f;
    const float passes=std::bit_cast<float>(0x407f7ceeu);
    for(unsigned i=0;i<4;++i) {out.radii[i+1]=std::clamp(passes-float(i),0.f,1.f)*step;step=(.75f+.5f)*step;}
    return out;
}
}
extern "C" void halo2_menu_bloom(float threshold,float strength,std::uint32_t *words,float *radii) {
    const auto bloom=halo2::menu_bloom(threshold,strength);
    std::copy(bloom.threshold.words.begin(),bloom.threshold.words.end(),words);
    std::copy(bloom.blur.words.begin(),bloom.blur.words.end(),words+60);
    std::copy(bloom.composite.words.begin(),bloom.composite.words.end(),words+120);
    std::copy(bloom.radii.begin(),bloom.radii.end(),radii);
}
