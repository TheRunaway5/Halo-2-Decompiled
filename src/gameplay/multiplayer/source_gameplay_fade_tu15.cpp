#include "halo2/source_gameplay_fade_tu15.hpp"
#include "halo2/source_player_lives.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cfenv>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Scope {
  std::fenv_t saved_;unsigned mx_;
public:
  explicit Scope(SourcePhysicsFactoryFloatingPoint f):mx_(_mm_getcsr()) {
    if((f.x87_control_word&0x3f)!=0x3f||(f.x87_control_word&0x300)==0x100||
        (f.mxcsr&0x1f80)!=0x1f80||(f.mxcsr&0xffff0000))throw FormatError("Unsupported TU1.5 gameplay fade FP context");
    if(std::fegetenv(&saved_))throw FormatError("TU1.5 gameplay fade FP environment unavailable");
    _mm_setcsr(f.mxcsr);asm volatile("fldcw %0"::"m"(f.x87_control_word):"memory");
  }
  ~Scope(){std::fesetenv(&saved_);_mm_setcsr(mx_);}
};
float f(std::uint32_t word){return std::bit_cast<float>(word);}
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float integer(std::int32_t a){return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),a));}
bool above(float a,float b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc");return v;}
bool below_equal(float a,float b){unsigned char v;asm volatile("comiss %2,%1; setbe %0":"=qm"(v):"x"(a),"x"(b):"cc");return v;}
std::int32_t round_integer(float value){std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(value):"st");return out;}
bool active(const SourceGameplayFadePlayerTu15 &p,std::int32_t limit){
  return source_player_respawn_allowed(limit,p.flags,p.membership,p.current_unit,p.lives);
}
#endif
}
void source_gameplay_fade_activate_tu15(SourceGameplayFadeStateTu15 &state,unsigned view){
  if(view>=4)throw FormatError("TU1.5 viewport activation requires an actual local view0..3");
  state.opacity_bits[view]=0x3f800000u;state.activation_mask|=std::uint8_t(1u<<view);
}
void source_gameplay_fade_tick_tu15(SourceGameplayFadeStateTu15 &state,const SourceGameplayFadeTickTu15 &in,
    SourcePhysicsFactoryFloatingPoint fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!in.engine_present)return;
  Scope scope(fp);
  if(in.phase==1&&(in.raw_mode==4||in.pending_phase==1))state.phase_counter=0;
  else state.phase_counter=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(state.phase_counter)+1u));
  for(unsigned view=0;view<4;++view){const auto &p=in.users[view];if(!p.present)continue;
    bool increase=int(in.rate)*4<state.phase_counter;
    if(!increase&&p.current_unit==0xffffffffu&&p.respawn_seconds<1)increase=active(p,in.life_limit);
    if(increase){const auto value=add(div(1.f,integer(in.rate)),f(state.opacity_bits[view]));
      state.opacity_bits[view]=std::bit_cast<std::uint32_t>(above(1.f,value)?value:1.f);continue;}
    if((state.activation_mask&(1u<<view))||p.current_unit==0xffffffffu){
      if(p.current_unit!=0xffffffffu||active(p,in.life_limit))continue;
      const auto delta=std::bit_cast<std::int32_t>(in.tick-in.round_start_tick);
      if((delta<0?0:delta)<=in.rate)continue;
    }
    // Authenticated literal449FD4=1.5,46A0E4=3,46A0D0=1. SSE
    // operations and original x87 FISTP preserve their actual rounding order.
    if(std::bit_cast<std::int32_t>(in.tick)<=round_integer(mul(integer(in.rate),1.5f)))continue;
    const auto value=sub(f(state.opacity_bits[view]),div(3.f,integer(in.rate)));
    state.opacity_bits[view]=std::bit_cast<std::uint32_t>(below_equal(0.f,value)?value:0.f);
  }
#else
  (void)state;(void)in;(void)fp;throw FormatError("TU1.5 gameplay fade SSE backend unavailable");
#endif
}
std::optional<float> source_gameplay_fade_render_tu15(SourceGameplayFadeStateTu15 &state,
    const SourceGameplayFadeRenderTu15 &in,SourcePhysicsFactoryFloatingPoint fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!in.engine_present)return {};
  if(in.actual_view!=0xffffffffu&&in.actual_view>=4)throw FormatError("TU1.5 fade draw requires actual view0..3 orNONE");
  Scope scope(fp);
  if(in.actual_view!=0xffffffffu){const auto view=in.actual_view;const auto bit=std::uint8_t(1u<<view);
    if(state.activation_mask&bit){
      if(in.unresolved_world_geometry<1)state.activation_mask&=std::uint8_t(~bit);
      else {state.rendered_frames[view]=std::uint8_t(state.rendered_frames[view]+1);
        if(round_integer(mul(integer(in.rate),3.f))<int(state.rendered_frames[view]))
          state.activation_mask&=std::uint8_t(~bit);}
    }else state.rendered_frames[view]=0;
  }
  const float opacity=in.actual_view==0xffffffffu?1.f:f(state.opacity_bits[in.actual_view]);
  if(below_equal(opacity,0.f))return {};
  return opacity;
#else
  (void)state;(void)in;(void)fp;throw FormatError("TU1.5 gameplay fade SSE backend unavailable");
#endif
}
}
