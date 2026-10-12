#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
void admitted(const SourceSceneryIdlePhaseDefinition &d,float step) {
    if(d.graph==0xffffffffu || d.animation==0xffffffffu || d.frames<=1 ||
       d.loop_frame!=0 || d.previous_sibling!=-1 || d.next_sibling!=-1 ||
       d.event_counts!=std::array<std::uint32_t,3>{})
        throw FormatError("SCEN idle phase requires its stable event-free source record");
    if(!std::isfinite(step) || step<0)
        throw FormatError("SCEN idle phase requires the actual finite simulation clock step");
}
float increment(float step,float rate) {
    // Original mulss,mulss; retain the intermediate binary32 rounding.
    const volatile float scaled=step*rate;
    return scaled*30.f;
}
float endpoint(const SourceSceneryIdlePhaseDefinition &d) {
    return float(d.frames)-std::bit_cast<float>(0x38d1b717u);
}
void store_phase(SourceSceneryIdlePhaseState &s,const SourceSceneryIdlePhaseDefinition &d,
                 float frame,float step) {
    const auto end=endpoint(d);
    s.frame=std::clamp(frame,0.f,end);
    if((s.playback_flags&1u) && !(s.status&9u)) {
        const volatile float next=increment(step,s.playback_rate)+s.frame;
        if(next>=end)s.status|=2u;
        else s.status&=0xfdu;
    }
    // Original1C69B0 has no events in the admitted record.
    s.next_events=0;
}
}
SourceSceneryIdlePhaseState source_scenery_idle_phase_initial(
    const SourceSceneryIdlePhaseDefinition &d,float step) {
    admitted(d,step);
    SourceSceneryIdlePhaseState result;
    store_phase(result,d,0,step);
    return result;
}
void source_scenery_idle_phase_step(SourceSceneryIdlePhaseState &s,
    const SourceSceneryIdlePhaseDefinition &d,float step) {
    admitted(d,step);
    if(!std::isfinite(s.frame) || !std::isfinite(s.playback_rate))
        throw FormatError("SCEN idle controller phase/rate is not finite");
    // 1C68C0 clears these before delay/pause admission.
    s.crossed_events=0;s.next_events=0;
    if(s.delay){--s.delay;return;}
    if(!(s.playback_flags&1u) || (s.status&9u))return;
    const volatile float advanced=increment(step,s.playback_rate)+s.frame;
    float frame=std::max(float(advanced),0.f);
    s.status&=0xf1u;
    const auto end=endpoint(d);
    if(frame>=end) {
        if(!(s.playback_flags&2u)){s.status|=8u;frame=end;}
        else {
            // The real1CB3B0/1DD630 call preserves the binding when siblings
            // are NONE. It resets rate only with playback bits5 and3 set.
            if((s.playback_flags&0x20u) && (s.playback_flags&8u))s.playback_rate=1;
            frame=frame-end; // authored loop frame is exactly zero
            s.status|=4u;
        }
    }
    store_phase(s,d,frame,step);
}
SourceSceneryIdlePhaseDefinition Cache::load_source_scenery_idle_phase_definition(
    const AnimationClip &clip) const {
    const auto tag=query_tag({.handle=TagHandle{clip.source_graph}, .group=tag_groups::model_animation_graph, .require_address=true});
    if(!tag)throw FormatError("SCEN idle phase source graph missing");
    const Bytes data(storage_);const auto root=metadata_offset(tag->address,0x34);
    const auto count=data.u32(root+0x2c);
    if(clip.source_index>=count || count>65536)
        throw FormatError("SCEN idle phase source index outside graph");
    const auto at=metadata_offset(data.u32(root+0x30),std::size_t(count)*0x6c)+
        std::size_t(clip.source_index)*0x6c;
    SourceSceneryIdlePhaseDefinition d;
    d.graph=clip.source_graph;d.animation=clip.source_index;
    d.frames=std::int16_t(data.u16(at+0x14));d.loop_frame=std::int16_t(data.u16(at+0x2e));
    d.previous_sibling=std::int16_t(data.u16(at+0x30));d.next_sibling=std::int16_t(data.u16(at+0x32));
    d.event_counts={data.u32(at+0x4c),data.u32(at+0x54),data.u32(at+0x5c)};
    if(clip.frame_count!=unsigned(d.frames) || clip.loop_frame!=unsigned(d.loop_frame) ||
       clip.frame_event_count!=d.event_counts[0] || !clip.frame_events.empty())
        throw FormatError("SCEN idle phase source record differs from the initialized clip");
    admitted(d,0);
    return d;
}
} // namespace halo2
