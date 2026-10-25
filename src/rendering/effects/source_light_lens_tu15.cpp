#include "halo2/source_light_lens_tu15.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
std::vector<SourceLightLensRequestTu15> source_standalone_light_lenses_tu15(
    const SourceEffectLightView &view) {
    std::vector<SourceLightLensRequestTu15> out;
    for(const auto identity:view.visible) {
        const auto at=std::find_if(view.records.begin(),view.records.end(),
            [&](const auto &r){return r.runtime.identity==identity;});
        if(at==view.records.end() || !at->definition)
            throw FormatError("Visible light lens lost its current runtime/tag owner");
        const auto &r=at->runtime;const auto &d=*at->definition;
        if(r.attached_node!=-1 || r.owner!=0xffffffffu)continue;
        // Whole31500: node NONE has local priority zero. Flag20 therefore
        // rejects and flag40 permits. C5370 independently checks active bit1.
        if((d.flags&0x20u) || !(r.flags&2u) || d.lens==0xffffffffu)continue;
        volatile float brightness=1.f-r.lens_fade;
        out.push_back({identity,d.lens,r.position,r.forward,r.state.diffuse,brightness,r.state.control});
    }
    return out;
}
void source_request_light_lens_tu15(SourceLensFrameWorldTu15 world,
    const SourceLightLensRequestTu15 &r) {
    if(r.identity==0xffffffffu || r.lens==0xffffffffu)
        throw FormatError("Standalone light lens lacks its current light/tag identity");
    auto call=world.prepare_source_call();const auto caller=call.gp[4];
    const auto storage=caller-0x100u;
    const std::array vectors{r.position,r.forward,r.color};
    world.write_source_call_input(storage,std::as_bytes(std::span(vectors)));
    call.gp[4]=caller-0x3000u;call.gp[1]=r.lens;call.gp[7]=storage+12;
    const std::array<std::uint32_t,10> args{0,0,2,r.identity&65535u,0,storage,storage+24,
        std::bit_cast<std::uint32_t>(r.brightness),std::bit_cast<std::uint32_t>(r.opacity),0x3f800000u};
    world.write_source_call_input(call.gp[4],std::as_bytes(std::span(args)));
    world.run_actual(0x2db80u,call);
}
}
