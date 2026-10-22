#include "halo2/source_shadow_submission.hpp"
#include <bit>
#include <cmath>
#include <set>

namespace halo2 {
namespace {
const SourceShadowOwnerState &owner(std::span<const SourceShadowOwnerState> objects,std::uint32_t identity) {
    const SourceShadowOwnerState *result=nullptr;
    for(const auto &o:objects)if(o.identity==identity){if(result)throw FormatError("duplicate shadow owner full handle");result=&o;}
    if(!result)throw FormatError("source shadow owner full handle unresolved");
    if(identity==0xffffffffu || result->object_type>12 || result->header_kind>12)
        throw FormatError("shadow owner outside admitted original object classes");
    return *result;
}
float camo(const SourceShadowOwnerState &o) {
    if(!o.camouflage || !std::isfinite(*o.camouflage))throw FormatError("source shadow unit camouflage unresolved");
    return *o.camouflage;
}
std::uint32_t root_owner(std::span<const SourceShadowOwnerState> objects,std::uint32_t handle) {
    std::set<std::uint32_t> seen;
    for(;;){if(!seen.insert(handle).second)throw FormatError("cyclic source shadow parent links");const auto&o=owner(objects,handle);if(o.parent==0xffffffffu)return handle;handle=o.parent;}
}
SourceShadowOwnerFade fade(std::span<const SourceShadowOwnerState> objects,std::uint32_t handle,
                          const SourceShadowClock &clock,std::set<std::uint32_t> &active) {
    if(!active.insert(handle).second)throw FormatError("cyclic shadow fade root fallback");
    const auto&o=owner(objects,handle);
    const bool admitted=!(o.object_flags&0x10000u) && (o.definition_mode==2 ||
        (o.definition_mode==0 && o.object_type<2));
    if(admitted && !o.model_fade)throw FormatError("source shadow4BAF0 byte unresolved");
    const bool dying=(o.lod_flags_10a&4)!=0;
    float elapsed=0;
    if(admitted && o.object_type==0 && dying) {
        if(!std::isfinite(clock.seconds_per_tick))throw FormatError("source shadow death clock unresolved");
        const auto ticks=std::bit_cast<std::int32_t>(std::uint32_t(clock.tick)-std::uint32_t(o.death_tick));
        elapsed=float(ticks)*clock.seconds_per_tick;
    }
    // Skip only the final header factor here. A first camouflage factor of
    // zero does not trigger original3D7F0's parent fallback.
    auto base=source_shadow_root_fade({o.definition_mode,o.object_type,2,o.model_fade.value_or(0),
        bool(o.object_flags&0x10000u),dying,elapsed,admitted && o.object_type==0?camo(o):0.f});
    if(!base.eligible && o.parent!=0xffffffffu) {
        const auto inherited=fade(objects,root_owner(objects,handle),clock,active);
        base={inherited.eligible,inherited.fade};
    }
    if(o.header_kind<2) {
        const float value=camo(o);const float clamped=value<0?0.f:value>1?1.f:value;
        base.fade=(1.f-clamped)*base.fade;if(base.fade<=0)base.eligible=false;
    }
    active.erase(handle);return {base.eligible,base.fade,o.object_type==12};
}
}
SourceShadowOwnerFade source_shadow_owner_fade(std::span<const SourceShadowOwnerState> objects,
    std::uint32_t handle,const SourceShadowClock &clock) {
    std::set<std::uint32_t> active;return fade(objects,handle,clock,active);
}
std::vector<ShadowCasterSphere> source_shadow_owner_hierarchy(
    std::span<const SourceShadowOwnerState> objects,std::uint32_t root) {
    std::vector<ShadowCasterSphere> result;std::set<std::uint32_t> seen;
    std::function<void(std::uint32_t)> visit=[&](std::uint32_t handle){
        while(handle!=0xffffffffu) {
            if(!seen.insert(handle).second)throw FormatError("cyclic/repeated source shadow hierarchy owner");
            const auto&o=owner(objects,handle);result.push_back(o.sphere);
            if(o.sibling!=0xffffffffu)visit(o.sibling);
            handle=o.child;
        }
    };visit(root);return result;
}
bool source_shadow_owner_blocked(std::span<const SourceShadowOwnerState> objects,std::uint32_t handle) {
    std::set<std::uint32_t> seen;
    while(handle!=0xffffffffu) {
        if(!seen.insert(handle).second)throw FormatError("cyclic source shadow blocking parents");
        const auto&o=owner(objects,handle);if((o.header_flags&16) || (o.object_flags&1))return true;handle=o.parent;
    }return false;
}
std::vector<SourceShadowCandidate> source_shadow_candidates(
    std::span<const SourceShadowOwnerState> objects,std::span<const SourceShadowCollectorEntry> collector,
    const SourceShadowCandidateContext &context) {
    if(collector.size()>32767 || !context.priority)throw FormatError("source shadow collector/priority unresolved");
    std::vector<SourceShadowCandidate> result;
    for(const auto &entry:collector) {
        if(context.mode!=0 && !(context.mode==1 && (entry.flags&0x4000)) &&
           !(context.mode==2 && (entry.flags&0x8000)))continue;
        const auto state=source_shadow_owner_fade(objects,entry.owner,context.clock);
        if(!state.eligible || result.size()>=32)continue;
        if(result.size()>=context.retained_fourth_flag_bytes.size())throw FormatError("original shadow candidate scratch flag3 unresolved");
        const auto&o=owner(objects,entry.owner);const auto flags=std::uint32_t(context.priority(entry.owner))|
            (std::uint32_t(context.retained_fourth_flag_bytes[result.size()])<<24);
        result.push_back({flags,entry.owner,std::bit_cast<std::int32_t>(entry.owner),state.fade,
            source_shadow_coverage(o.sphere.center,o.sphere.radius,context.depth_plane,context.pixel_scale)});
    }return result;
}
}
