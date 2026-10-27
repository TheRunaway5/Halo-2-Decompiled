#include "halo2/atmosphere.hpp"
#include "halo2/bloom.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/cinematic_crossfade.hpp"
#include "halo2/light_volume.hpp"
#include "halo2/popup_blur.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <iostream>

using namespace halo2;
int main() {
    nlohmann::json rows=nlohmann::json::array();
    const auto emit=[&](const char* family,const PixelProgram& program,nlohmann::json arguments) {
        rows.push_back({{"family",family},{"words",program.words},{"arguments",arguments}});
    };
    MenuAtmosphere fog;
    fog.atmospheric={{.25f,.5f,.75f},.5f,0,1024};
    fog.planar.plane={0,0,1,0};fog.planar.color={.5f,.25f,.75f};
    fog.planar.maximum_density=.5f;fog.planar.opaque_depth=fog.planar.opaque_distance=1;
    for(unsigned power=0;power<7;++power) {
        fog.atmospheric.opaque=std::ldexp(1024.f,-int(power));
        emit("fog",menu_fog_composite(fog,{0,0,1}).program,{{"mode",1},{"power",power}});
    }
    fog.atmospheric.opaque=1;
    emit("fog",menu_fog_composite(fog,{0,0,1}).program,{{"mode","lookup fallback"}});
    emit("fog",menu_fog_draw(fog,{0,0,-.5f})->program,{{"mode","far-only eye"}});
    const auto bloom=menu_bloom(.37f,.63f);
    emit("bloom",bloom.threshold,{{"pass","threshold"}});
    emit("bloom",bloom.blur,{{"pass","blur"}});
    emit("bloom",bloom.composite,{{"pass","composite"}});
    emit("bloom",frontend_popup_blur(.5f).program,{{"pass","popup lower-channel blur"}});
    PatchyFog patchy{};patchy.color={.25f,.5f,.75f};patchy.distance={1,512};patchy.density={.25f,.75f};
    patchy.flags=1;patchy.noise_scale=32;patchy.vertical_up=patchy.vertical_forward=patchy.opacity_up=1;
    patchy.noise.resize(1);Transform view;PatchyFogState state;std::uint32_t random=0x54553135;
    const auto frame=patchy_fog_frame(patchy,state,random,view,0,1,.75f,0);
    emit("patchy-noise",frame.noise,{{"mode","original title"}});
    emit("patchy-composite",frame.cached,{{"mode","cached"}});
    emit("patchy-composite",frame.direct,{{"mode","direct"}});
    const auto bsp=[&](unsigned flags,float start,float end) {
        auto input=patchy;input.flags=flags;input.distance={start,end};
        BspPatchyDefinition definition;definition.fog=input;
        PatchyFogState motion;std::uint32_t rng=0x54553135;
        return bsp_patchy_fog_frame(definition,input,motion,rng,view,0,1,.75f,0,1024,.015625f);
    };
    for(unsigned flags=0;flags<2;++flags) {
        const auto input=bsp(flags,-1,512);
        emit("bsp-patchy-noise",input.shading.noise,{{"flags",flags},{"mode","fallback"}});
        emit("bsp-patchy-composite",input.shading.direct,{{"flags",flags},{"mode","direct fallback"}});
        emit("bsp-patchy-composite",input.shading.cached,{{"flags",flags},{"mode","cached fallback"}});
    }
    for(unsigned low_byte=0;low_byte<2;++low_byte)for(unsigned power=0;power<8;++power) {
        const float end=std::ldexp(1024.f,-int(power)-(low_byte?8:0));
        const auto input=bsp(0,0,end);
        if(!input.quantized_composite)throw std::runtime_error("A complete native patchy mode2 control variant was not produced");
        emit("bsp-patchy-composite",input.shading.direct,{{"mode",2},{"low_byte",low_byte},{"power",power},{"cached",false}});
        emit("bsp-patchy-composite",input.shading.cached,{{"mode",2},{"low_byte",low_byte},{"power",power},{"cached",true}});
    }
    emit("cinematic-crossfade",*cinematic_crossfade_program(.5f,1),{{"alpha",.5f},{"strength",1}});
    emit("light-volume",light_volume_program(),{{"source","original fixed light volume"}});
    std::cout<<rows.dump()<<'\n';
}
