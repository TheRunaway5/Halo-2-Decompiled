#include "halo2/source_effect_lighting_colors_tu15.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
bool source_effect_lighting_colors_tu15(SourceEffectColors&colors,
    SourceEffectLightingParametersTu15*parameters,bool probe,
    const SourceEffectLightingServicesTu15&services) {
    bool completed{};
    if(parameters&&(parameters->flags&8u)) {
        //17E481..17E48D: caller's cached pair, in source A/B order.
        colors.a=parameters->colors.a;colors.b=parameters->colors.b;
        completed=true;
    } else {
        if(!services.render_cache)
            throw FormatError("17E460 requires its completed native root render-cache owner");
        if(const auto sample=services.render_cache()) {
            //17E4CF..17E4DB: +2C lightmap -> effect+104, +28 diffuse -> +108.
            // Packed words are copied without clamping, unpacking or alpha edits.
            colors.a=sample->lightmap_rgb;colors.b=sample->diffuse_rgb;
            completed=true;
        }
    }
    if(probe&&!completed) {
        if(!services.probe)
            throw FormatError("17E460 probe requires native17DCE0/17BA50/D4F90 owners");
        SourceEffectLightingProbeTu15 input;
        if(parameters)input.reference=parameters->probe_reference;
        if(const auto result=services.probe(input)) {
            colors.a=result->a;colors.b=result->b;completed=true;
        }
    }
    //17E59A..17E5C8 reloads the request flags after the producer calls.
    if(parameters&&!(parameters->flags&8u)&&completed) {
        parameters->flags|=8u;
        parameters->colors.a=colors.a;parameters->colors.b=colors.b;
    }
    return completed;
}
} // namespace halo2
