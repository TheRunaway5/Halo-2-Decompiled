#include "halo2/source_game_time.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::int32_t rounded_integer(float value) {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
    std::int32_t result;
    asm volatile("flds %1; fistpl %0" : "=m"(result) : "m"(value) : "st");
    return result;
#else
    // FISTP observes x87 rounding, independently of MXCSR. The portable path
    // requires the caller's C floating-point rounding mode to agree with x87.
    const double result=std::rint(double(value));
    if(!(result>=-2147483648. && result<2147483648.))return INT32_MIN;
    return static_cast<std::int32_t>(result);
#endif
}
std::int32_t truncated_integer(float value) {
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    return _mm_cvtt_ss2si(_mm_set_ss(value));
#else
    if(!(value>=-2147483648.f && double(value)<2147483648.))return INT32_MIN;
    return static_cast<std::int32_t>(value);
#endif
}
std::int32_t add(std::int32_t value,std::uint32_t amount) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+amount);
}
float ramp_bound(float value) {
    if(.2f>value)return .2f;
    if(value>5.f)return 5.f;
    return value;
}
}
void source_game_time_initialize(SourceGameTime &time,std::int16_t configured) {
    time={};time.tick_rate=configured;time.seconds_per_tick=1.f/float(configured);
    time.scale=1.f;time.active=1;
}
void source_game_time_speed(SourceGameTime &time,std::int16_t configured,float speed) {
    if(!(speed>.01f))speed=.01f;
    const auto ticks=rounded_integer(float(configured)/speed);
    time.seconds_per_tick=1.f/float(ticks);
    time.tick_rate=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ticks));
    time.scale=float(configured)/float(ticks);
}
void source_game_time_speed_ramp(SourceGameTime &time,std::int16_t configured,
    float initial,float final,float duration) {
    initial=ramp_bound(initial);final=ramp_bound(final);
    if(duration>0.f) {
        time.speed_final=final;time.speed_timer=0.f;time.speed_duration=duration;
        time.speed_initial=initial;source_game_time_speed(time,configured,initial);
    } else source_game_time_speed(time,configured,final);
}
SourceGameTimeStep source_game_time_advance(SourceGameTime *&time,
    const SourceGameTimeContext &context,float seconds,const SourceGameTimeNetworkBudget &network) {
    if(!time)throw std::invalid_argument("original game time owner is absent");
    const bool network_mode=context.simulation_mode>=2 && context.simulation_mode<=5;
    SourceGameTimeStep step;float leftover=0.f;
    if(context.options_present && context.game_active) {
        if(time->speed_duration>0.f) {
            time->speed_timer=time->speed_timer+seconds;
            if(time->speed_duration>time->speed_timer) {
                const float t=time->speed_timer/time->speed_duration;
                const float initial=(1.f-t)*time->speed_initial;
                const float final=time->speed_final*t;
                source_game_time_speed(*time,context.configured_tick_rate,initial+final);
            } else {
                source_game_time_speed(*time,context.configured_tick_rate,time->speed_final);
                time->speed_duration=0.f;
            }
        }
        if(!time->paused && time->scale>0.f) {
            bool synchronous=false;std::int32_t maximum;
            if(network_mode) {
                if(!network)throw std::invalid_argument("original 680C0 network budget is absent");
                const auto budget=network();
                if(!budget.time)throw std::invalid_argument("original network budget lost game time owner");
                time=budget.time;maximum=budget.ticks;synchronous=budget.synchronous;
            } else maximum=std::int32_t(time->tick_rate)*5;
            if(!synchronous && !context.unlimited_ticks) {
                float scale=1.f;
                if(!(scale>time->scale)) {
                    scale=5.f;
                    if(!(time->scale>scale))scale=time->scale;
                }
                const auto limit=rounded_integer(scale*2.f);
                if(maximum>limit)maximum=limit;
            }
            step.elapsed=time->scale*seconds;
            const float ticks=float(time->tick_rate)*step.elapsed;
            const float accumulated=ticks+time->leftover_ticks;
            const auto count=truncated_integer(accumulated);
            const bool retain=count<=maximum && (!synchronous ||
                (add(count,7)>=maximum && add(count,1)==maximum));
            if(retain) {step.ticks=count;leftover=accumulated-float(count);}
            else {step.elapsed=float(maximum)*time->seconds_per_tick;step.ticks=maximum;}
        }
    }
    time->leftover_ticks=leftover;return step;
}
std::int32_t source_game_time_consume(SourceGameTime *&time,SourceGameTimeStep &step,
    const std::function<bool()> &stop,const std::function<void()> &tick) {
    const auto requested=step.ticks;
    if(requested<=0)return 0;
    if(!stop || !tick)throw std::invalid_argument("original tick consumers are absent");
    std::int32_t consumed=0;
    while(consumed<requested) {
        if(stop()) {
            if(!consumed)step.elapsed=0.f;
            else {
                if(!time)throw std::invalid_argument("original tick consumer lost game time owner");
                const float skipped=float(requested-consumed)*time->seconds_per_tick;
                const float elapsed=step.elapsed-skipped;
                step.elapsed=elapsed>0.f?elapsed:0.f;
            }
            break;
        }
        tick();++consumed;
    }
    return consumed;
}
void source_world_time_accumulate(double &realtime,float elapsed) {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
    asm volatile("flds %1; faddl %0; fstpl %0" : "+m"(realtime) : "m"(elapsed) : "st");
#else
    realtime=double(elapsed)+realtime;
#endif
}
}
