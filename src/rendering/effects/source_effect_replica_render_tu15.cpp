#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/effect_part_control.hpp"
#include "halo2/kernels.hpp"
#include <cmath>

namespace halo2 {
float source_effect_part_lens_opacity_tu15(const SourceEffectPartLensControlsTu15 &part,
    SourceEffectControls controls,const SourceEffectEventState &clock,const ShaderFunctionTables &tables) {
    if(!std::isfinite(clock.elapsed)||!std::isfinite(clock.target))
        throw FormatError("Effect lens requires its finite original event clock");
    float opacity=source_effect_part_control(part.scale_a,part.scale_b,controls.a,controls.b);
    if(clock.target!=0.f) {
        volatile float ratio=clock.elapsed/clock.target;
        volatile float remaining=1.f-ratio;
        float clamped=remaining;if(0.f>clamped)clamped=0.f;else if(clamped>1.f)clamped=1.f;
        volatile float phase=1.f-clamped;
        if(part.fade_curve>=tables.transition.size())throw FormatError("Effect lens transition escapes its original table bank");
        const float faded=part.fade_curve?fade_curve_fraction(phase,tables.transition[part.fade_curve]):float(phase);
        volatile float inverse=1.f-faded;
        volatile float result=inverse*opacity;opacity=result;
    }
    return opacity;
}
}
