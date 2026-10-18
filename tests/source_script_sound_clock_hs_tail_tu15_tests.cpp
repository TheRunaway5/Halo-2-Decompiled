#include "halo2/source_script_sound_clock_tu15.hpp"
#include <bit>
#include <cfenv>
#include <iostream>
#include <stdexcept>
#include <string>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace {
using namespace halo2;
struct Case {
    std::uint32_t source_case,game_ticks,fresh_delta,x87_control,mxcsr;
    std::int32_t expected;
};
constexpr Case cases[]={
#include "source_script_sound_clock_hs_tail_tu15_cases.inc"
};
void need(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
template<class F>void rejects(F&&f,const char*why){bool threw=false;try{f();}catch(const std::runtime_error&){threw=true;}need(threw,why);}
void original_tail(){
    unsigned distinct=0;
    for(const auto&c:cases){
        const auto ticks=std::bit_cast<std::int32_t>(c.game_ticks);
        const auto actual=source_script_sound_clock_hs_explicit_tail_tu15(ticks,c.fresh_delta,{std::uint16_t(c.x87_control),c.mxcsr});
        if(actual!=c.expected)throw std::runtime_error("Original HS explicit tail case "+std::to_string(c.source_case));
        distinct+=ticks!=actual;
    }
    need(distinct==7424,"HS query GAME ticks were conflated with returned30Hz count");
}
void fresh_phase(){
    const SoundDriverFloatingPointContext fp{0x027f,0x1f80};
    const SourceScriptSoundClockTimeTu15 query{100,60,0x3c888889};
    const auto game_ticks=source_script_sound_explicit_remaining_tu15(true,100,30,query,fp);
    need(game_ticks==60,"Original explicit query did not return GAME ticks");
    need(source_script_sound_clock_hs_explicit_tail_tu15(game_ticks,0x3c888889,fp)==30,"Same-phase60Hz tail");
    need(source_script_sound_clock_hs_explicit_tail_tu15(game_ticks,0x3d088889,fp)==60,"Fresh post-query delta was replaced with earlier query delta");
    need(source_script_sound_clock_hs_explicit_tail_tu15(0,0x7f800000,fp)==std::int32_t(0x80000000u),"Zero GAME ticks bypassed original FP tail");
}
void context(){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto old_round=std::fegetround();const auto old_mxcsr=_mm_getcsr();std::fenv_t old_environment;
    need(std::fegetenv(&old_environment)==0,"Caller FP environment unavailable");
    std::fesetround(FE_DOWNWARD);std::feraiseexcept(FE_INVALID|FE_INEXACT);const auto before_exceptions=std::fetestexcept(FE_ALL_EXCEPT);std::uint16_t before_cw;asm volatile("fnstcw %0":"=m"(before_cw));const auto before_mxcsr=_mm_getcsr();
    source_script_sound_clock_hs_explicit_tail_tu15(16777217,0x3d088889,{0x0e7f,0x7f80});
    std::uint16_t after_cw;asm volatile("fnstcw %0":"=m"(after_cw));
    const bool restored=before_cw==after_cw&&before_mxcsr==_mm_getcsr()&&before_exceptions==std::fetestexcept(FE_ALL_EXCEPT)&&std::fegetround()==FE_DOWNWARD;
    std::fesetenv(&old_environment);_mm_setcsr(old_mxcsr);std::fesetround(old_round);need(restored,"Pure HS tail changed caller FP environment");
#endif
    rejects([]{source_script_sound_clock_hs_explicit_tail_tu15(1,0x3d088889,{0x027e,0x1f80});},"Unmasked x87 admitted");
    rejects([]{source_script_sound_clock_hs_explicit_tail_tu15(1,0x3d088889,{0x027f,0x1f00});},"Unmasked SSE admitted");
    rejects([]{source_script_sound_clock_hs_explicit_tail_tu15(1,0x3d088889,{0x027f,0x11f80});},"Reserved MXCSR admitted");
}
}
int main(){try{original_tail();fresh_phase();context();std::cout<<"TU1.5 two-argument HS fresh-delta numeric tail: "<<std::size(cases)<<" genuine CPU cases and phase/environment controls PASS\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
