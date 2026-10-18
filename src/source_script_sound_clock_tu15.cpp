#include "halo2/source_script_sound_clock_tu15.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::int32_t signed_word(std::uint32_t bits){return std::bit_cast<std::int32_t>(bits);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
    std::fenv_t environment;unsigned mxcsr;
    explicit FloatingPointScope(const SoundDriverFloatingPointContext&context):mxcsr(_mm_getcsr()){
        if((context.x87_control_word&0x3fu)!=0x3fu ||
           (context.mxcsr&0x1f80u)!=0x1f80u || (context.mxcsr&0xffff0000u))
            throw std::runtime_error("Unmasked original script sound clock exceptions unavailable");
        if(std::fegetenv(&environment)!=0)throw std::runtime_error("Original script sound clock environment unavailable");
        _mm_setcsr(context.mxcsr);
        asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
    }
    ~FloatingPointScope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
};
float scalar(std::uint32_t bits){return std::bit_cast<float>(bits);}
float integer(std::int32_t value){return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),value));}
float multiply(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
std::int32_t spill_integer(float value){std::int32_t result;asm volatile("flds %1\n\tfistpl %0":"=m"(result):"m"(value):"st","memory");return result;}
std::int32_t duration_ticks(std::uint32_t duration,const SourceScriptSoundClockTimeTu15&time){return spill_integer(multiply(integer(time.tick_rate),scalar(duration)));}
bool random_at_least_skip(std::uint16_t random,float skip){
    const std::int32_t draw=random;const float unit=scalar(0x37800080u);unsigned char result;
    //21FC54..70 retains the original80-bit random product and comparison.
    asm volatile("fildl %1\n\tfmuls %2\n\tflds %3\n\tfxch %%st(1)\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tsetae %0"
        :"=qm"(result):"m"(draw),"m"(unit),"m"(skip):"st","st(1)","cc","memory");
    return result;
}
std::int32_t explicit_remaining(bool matched,std::uint32_t start,std::int32_t input,const SourceScriptSoundClockTimeTu15&time){
    if(!matched)return 0;
    //18D11A..13A retains both MULSS operations before the FISTP spill.
    const auto ticks=spill_integer(multiply(multiply(integer(input),scalar(0x3d088889u)),integer(time.tick_rate)));
    const auto remaining=signed_word(start-time.tick_bits+std::uint32_t(ticks));
    return remaining>0?remaining:0;
}
#else
[[noreturn]] void unavailable(){throw std::runtime_error("Original x87/SSE script sound clock backend unavailable");}
#endif
} // namespace

std::int32_t source_script_sound_explicit_remaining_tu15(bool matched,std::uint32_t start,std::int32_t input,const SourceScriptSoundClockTimeTu15&time,const SoundDriverFloatingPointContext&fp){
    //18D0E0 returns before every FP instruction when no full datum matches.
    if(!matched)return 0;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);return explicit_remaining(matched,start,input,time);
#else
    (void)matched;(void)start;(void)input;(void)time;(void)fp;unavailable();
#endif
}
std::int32_t source_script_sound_stored_remaining_tu15(bool matched,std::uint32_t start,std::int32_t maximum_ms,const SourceScriptSoundClockTimeTu15&query,const SourceScriptSoundClockTimeTu15&returned,const SoundDriverFloatingPointContext&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);
    const auto original30=spill_integer(multiply(multiply(integer(maximum_ms),scalar(0x3a83126fu)),30.f));
    const auto remaining=explicit_remaining(matched,start,original30,query);
    return spill_integer(multiply(multiply(integer(remaining),scalar(returned.delta_time_bits)),30.f));
#else
    (void)matched;(void)start;(void)maximum_ms;(void)query;(void)returned;(void)fp;unavailable();
#endif
}
std::int32_t source_script_sound_language_remaining_tu15(bool matched,std::uint32_t start,std::uint32_t duration,const SourceScriptSoundClockTimeTu15&time,const SoundDriverFloatingPointContext&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);float remaining=0;
    if(matched){const auto ticks=duration_ticks(duration,time);remaining=multiply(integer(signed_word(start-time.tick_bits+std::uint32_t(ticks))),scalar(time.delta_time_bits));if(!(remaining>0.f))remaining=0;}
    return spill_integer(multiply(remaining,30.f));
#else
    (void)matched;(void)start;(void)duration;(void)time;(void)fp;unavailable();
#endif
}
bool source_script_sound_clock_expired_tu15(std::uint32_t start,std::uint32_t duration,const SourceScriptSoundClockTimeTu15&time,const SoundDriverFloatingPointContext&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);return signed_word(start+std::uint32_t(duration_ticks(duration,time)))<=signed_word(time.tick_bits);
#else
    (void)start;(void)duration;(void)time;(void)fp;unavailable();
#endif
}

std::optional<unsigned> source_script_sound_clock_admit_tu15(
    SourceScriptSoundClockRowsTu15&rows,std::uint32_t sound,
    const SourceScriptSoundClockPolicyTu15&policy,const SoundDriverFloatingPointContext&fp){
    std::optional<unsigned> candidate,match;
    for(unsigned i=0;i<rows.size();++i){
        const auto &row=rows[i];
        if(row.sound==sound){
            if(row.predicate==0){
                if(!policy.request_stop)throw std::runtime_error("Original script sound stop owner unavailable");
                policy.request_stop(i);
            }
            match=i;
        }else if(!candidate){
            if(row.sound==0xffffffffu)candidate=i;
            else{
                if(!policy.selected_seconds || !policy.time)
                    throw std::runtime_error("Original script sound duration/time owner unavailable");
                const auto seconds=policy.selected_seconds(i,row.sound,row.selection);
                //18CD36 loads the live clock only AFTER213690 has returned.
                const auto time=policy.time();
                if(source_script_sound_clock_expired_tu15(row.start_tick_bits,seconds,time,fp))candidate=i;
            }
        }
    }
    const auto selected=match?match:candidate;
    if(!selected)return {};
    if(!policy.select || !policy.time)
        throw std::runtime_error("Original script sound selection/time owner unavailable");
    auto &row=rows[*selected];
    policy.select(sound,row.selection);
    const auto time=policy.time();
    row.start_tick_bits=time.tick_bits;row.sound=sound;row.predicate=0;
    return selected;
}

std::optional<unsigned> source_script_sound_clock_find_first_tu15(
    const SourceScriptSoundClockRowsTu15&rows,std::uint32_t sound){
    if(sound!=0xffffffffu)
        for(unsigned i=0;i<rows.size();++i)if(rows[i].sound==sound)return i;
    return {};
}

std::optional<unsigned> source_script_sound_clock_stop_first_tu15(
    SourceScriptSoundClockRowsTu15&rows,std::uint32_t sound,
    const std::function<void(unsigned)>&request_stop){
    const auto match=source_script_sound_clock_find_first_tu15(rows,sound);
    if(match){
        if(!request_stop)throw std::runtime_error("Original script sound stop owner unavailable");
        request_stop(*match);
        rows[*match].start_tick_bits=0xffffffffu;rows[*match].sound=0xffffffffu;
    }
    return match;
}

void source_script_sound_clock_select_tu15(
    const SourceScriptSoundClockSelectionInputTu15&input,
    std::span<std::uint32_t>flags,std::uint32_t&random,
    SourceScriptSoundClockSelectionTu15&selection,const SoundDriverFloatingPointContext&fp,
    const SourceScriptSoundClockRandomStoreTu15&store){
    if(input.sound==0xffffffffu || input.pitch_range_count!=1){selection={0xff,0xff};return;}
    const auto count=input.permutation_count;
    if(count<1 || count>31)throw std::runtime_error("Original script sound permutation domain unavailable");
    if(count==1){selection.permutation=0;selection.pitch_range=0;return;}
    const auto advance=[&](std::uint32_t instruction){
        const auto before=random;random=random*0x19660du+0x3c6ef35fu;
        if(store)store(instruction,before,random);
        return std::uint16_t(random>>16);
    };
    if(!input.prefix_bit){
        selection.permutation=std::uint8_t((std::uint32_t(advance(0x2133eau))*std::uint32_t(count))>>16);
        selection.pitch_range=0;return;
    }
    if(input.skip_fractions.size()!=std::size_t(count))
        throw std::runtime_error("Original script sound skip-fraction owner unavailable");
    const auto width=unsigned(std::bit_width(std::uint32_t(count-1)));
    const auto base=std::uint32_t(*input.prefix_bit);
    if(base+std::uint32_t(count)+width>std::uint64_t(flags.size())*32)
        throw std::runtime_error("Original script sound prefix owner unavailable");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);
    const auto bit=[&](unsigned offset){const auto n=base+offset;return bool(flags[n/32]&(1u<<(n%32)));};
    const auto put=[&](unsigned offset,bool value){const auto n=base+offset;const auto mask=1u<<(n%32);auto &word=flags[n/32];word=value?word|mask:word&~mask;};
    std::uint32_t used=0,last=0;
    for(unsigned i=0;i<unsigned(count);++i)if(bit(i))used|=1u<<i;
    for(unsigned i=0;i<width;++i)if(bit(unsigned(count)+i))last|=1u<<i;
    unsigned selected,retries=0;
    const auto mask=(1u<<unsigned(count))-1;
    for(;;){
        selected=(std::uint32_t(advance(0x21fb81u))*std::uint32_t(count))>>16;
        if((~used&mask)==0){
            used=0;
            if(count>1 && std::uint16_t(last)!=0xffffu)used=1u<<(last&31);
            last=0xffffffffu;
        }
        const auto chosen=1u<<(selected&31);
        if(used&chosen)continue;
        used|=chosen;
        auto skip=multiply(integer(input.skip_fractions[selected]),scalar(0x37800080u));
        if(input.game_kind==1 && input.skip_control && skip>=.5f)
            skip=_mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(1.f),_mm_set_ss(skip)));
        if(random_at_least_skip(advance(0x21fc49u),skip) || ++retries==unsigned(count)*2)break;
    }
    selection.permutation=std::uint8_t(selected);
    //21358F..645 stores the selected last bits, then the updated used bits.
    for(unsigned i=0;i<width;++i)put(unsigned(count)+i,(selected&(1u<<i))!=0);
    for(unsigned i=0;i<unsigned(count);++i)put(i,(used&(1u<<i))!=0);
    selection.pitch_range=0;
#else
    (void)flags;(void)fp;unavailable();
#endif
}
} // namespace halo2

namespace halo2 {
std::int32_t source_script_sound_clock_hs_explicit_tail_tu15(
    std::int32_t game_ticks,std::uint32_t fresh_delta_time_bits,
    const SoundDriverFloatingPointContext&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    FloatingPointScope scope(fp);
    return spill_integer(multiply(multiply(integer(game_ticks),scalar(fresh_delta_time_bits)),30.f));
#else
    (void)game_ticks;(void)fresh_delta_time_bits;(void)fp;unavailable();
#endif
}
} // namespace halo2
