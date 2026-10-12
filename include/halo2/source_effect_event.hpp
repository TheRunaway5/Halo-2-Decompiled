#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>

namespace halo2 {
// Retail default.xbe 1782A0/179190/1792B0. The caller owns the live effect
// datum, definition, random word and downstream part/particle/retire services.
// These are event clocks, not an effect allocator or attachment constructor.
struct SourceEffectEventDefinition {
    float skip{},delay_low{},delay_high{},duration_low{},duration_high{};
};
struct SourceEffectEventState {
    std::uint16_t flags{};                 // effect body+02
    std::int16_t event{},previous=-1;       // +5C/+5E
    float elapsed{},target{},field_68{};   // +60/+64/+68
};
struct SourceEffectEventServices {
    std::function<void()> synchronize;    // 17ADD0
    std::function<void()> begin_parts;    // 17A380
    std::function<void(float,float)> advance_particles; // 179020
    std::function<void()> after_particles; // 179FB0
    std::function<bool()> retire;         // 177610; true means datum freed
};
inline long double source_effect_event_fraction(std::uint32_t &random) {
    random=random*1664525u+1013904223u;
    // 259D0 and179190 retain the x87 product until the store/comparison.
    return static_cast<long double>(random>>16)*static_cast<long double>(1.f/65535.f);
}
inline float source_effect_event_range(std::uint32_t &random,float low,float high) {
    if(low==high)return low;
    const auto fraction=source_effect_event_fraction(random);
    return float((static_cast<long double>(high)-low)*fraction+low);
}
inline bool source_effect_event_select(SourceEffectEventState &state,
    std::span<const SourceEffectEventDefinition> events,int event,
    std::uint32_t &random,const SourceEffectEventServices &services) {
    if(event<0 || std::size_t(event)>=events.size())return false;
    const auto &d=events[event];
    state.flags&=~1u;state.event=std::int16_t(event);state.elapsed=0;
    state.target=source_effect_event_range(random,d.delay_low,d.delay_high);
    services.synchronize();
    return true;
}
inline int source_effect_event_next(const SourceEffectEventState &state,
    std::span<const SourceEffectEventDefinition> events,std::int16_t loop_start,
    std::uint32_t &random) {
    const bool repeat=state.flags&0x202u;
    const int loop=repeat && loop_start==-1?0:loop_start;
    int event=state.event;
    for(;;) {
        event=repeat && event==int(events.size())-1 && loop!=-1?loop:event+1;
        if(event>=int(events.size()))return event;
        if(event<0)throw std::runtime_error("Effect loop escapes its event bank");
        const auto skip=events[event].skip;
        if(!(skip>0) || !(source_effect_event_fraction(random)<skip))return event;
    }
}
inline bool source_effect_event_advance(SourceEffectEventState &state,
    std::span<const SourceEffectEventDefinition> events,std::int16_t loop_start,
    float seconds,std::uint32_t &random,const SourceEffectEventServices &services) {
    if(state.flags&0x40u)return false;
    for(unsigned iteration=0;seconds>=0 && !(state.flags&4u) && iteration<8;++iteration) {
        if(state.event<0 || std::size_t(state.event)>=events.size())
            throw std::runtime_error("Effect clock has no selected authored event");
        const float remaining=state.target-state.elapsed;
        const float before=state.elapsed;
        const float interval=remaining<=seconds?remaining:seconds;
        const bool complete=!(seconds<remaining);
        if(complete){state.elapsed=state.target;seconds=seconds-remaining;}
        else{state.elapsed=state.elapsed+seconds;seconds=-1.f;}
        if(state.flags&1u) {
            if(!(state.flags&8u))services.advance_particles(before,interval);
            if(!(state.flags&8u))services.after_particles();
            if(complete) {
                const int next=source_effect_event_next(state,events,loop_start,random);
                if(next>=int(events.size())) {
                    if(services.retire())return true;
                    break;
                }
                source_effect_event_select(state,events,next,random,services);
            }
        }else if(complete) {
            state.flags|=1u;state.elapsed=0;state.field_68=-1.f;
            const auto &d=events[state.event];
            state.target=source_effect_event_range(random,d.duration_low,d.duration_high);
            if(!(state.flags&8u))services.begin_parts();
            services.synchronize();state.previous=state.event;
        }
    }
    state.flags&=~0x200u;
    return false;
}
} // namespace halo2
