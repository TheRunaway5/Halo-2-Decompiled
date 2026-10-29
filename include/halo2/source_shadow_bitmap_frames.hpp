#pragma once
#include "halo2/cache.hpp"
#include "halo2/bitmap_frame_animation.hpp"

namespace halo2 {
// The fixed shadow route currently owns bitmap-frame selection only. Bitmap
// properties without a selected vertex remap do not change its constant bank.
// Constant curves may carry a zero input without consuming an animation clock.
inline std::array<std::uint32_t,4> source_shadow_bitmap_frames(
    const Material& material,const ShaderInput& input) {
    const auto& animation=material.animation;
    if(animation.overlays.empty())return material.texture_frames;
    const auto& pass=material.passes.at(material.forward_pass_index);
    if(pass.template_pass>=animation.passes.size())
        throw FormatError("shadow scene animation pass escapes authored metadata");
    const auto range=animation.passes[pass.template_pass];
    if(range.first()>animation.implementations.size() ||
       range.count()>animation.implementations.size()-range.first() || pass.implementation>=range.count())
        throw FormatError("shadow scene animation implementation escapes authored metadata");
    const auto& implementation=animation.implementations[range.first()+pass.implementation];
    if(implementation.pixel_constants.count() || implementation.vertex_constants.count())
        throw FormatError("shadow scene constant animation lacks its live function owner");
    const auto mappings=implementation.bitmap_transforms;
    if(mappings.first()>animation.mappings.size() || mappings.count()>animation.mappings.size()-mappings.first())
        throw FormatError("shadow scene bitmap mappings escape authored metadata");
    for(const auto& mapping:std::span(animation.mappings).subspan(mappings.first(),mappings.count())) {
        if((mapping.destination&255u)>=8 || mapping.source>=animation.parameters.size())
            throw FormatError("shadow scene bitmap property escapes authored metadata");
        const auto references=animation.parameters[mapping.source];
        if(references.first()>animation.references.size() || references.count()>animation.references.size()-references.first())
            throw FormatError("shadow scene bitmap references escape authored metadata");
        for(const auto& reference:std::span(animation.references).subspan(references.first(),references.count())) {
            if(reference.transform<0 || reference.transform>=10 || reference.function<0 ||
               std::size_t(reference.function)>=animation.overlays.size())
                throw FormatError("shadow scene bitmap function escapes authored metadata");
            const auto& function=animation.overlays[reference.function];
            if(!function.input) {
                if(function.curve.empty())throw FormatError("shadow scene bitmap function has no authored curve");
                const auto type=std::to_integer<unsigned>(function.curve[0]);
                // These original function kinds return a fraction independent
                // of their clock input (1 uses only its optional range).
                if(type!=1 && type!=6 && type!=8)
                    throw FormatError("shadow scene bitmap animation requires its actual clock owner");
            }
        }
    }
    // Zero is unused by the admitted clock-independent functions above. Live
    // object inputs and ranges still come from the caller's original BCB70.
    return animate_bitmap_frames(bitmap_frame_animations(material),0.,input,material.texture_frames);
}
}
