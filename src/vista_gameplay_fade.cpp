#include "halo2/vista_gameplay_fade.hpp"
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
  explicit Scope(const VistaLoadingFloatingPoint& f):mx_(_mm_getcsr()) {
    if((f.x87_control_word&0x3f)!=0x3f || (f.x87_control_word&0x300)==0x100 ||
        (f.mxcsr&0x1f80)!=0x1f80 || (f.mxcsr&0xffff0000))throw FormatError("Unsupported Vista gameplay fade FP context");
    if(std::fegetenv(&saved_))throw FormatError("Vista gameplay fade FP environment unavailable");
    _mm_setcsr(f.mxcsr);asm volatile("fldcw %0"::"m"(f.x87_control_word):"memory");
  }
  ~Scope(){std::fesetenv(&saved_);_mm_setcsr(mx_);}
};
float f(std::uint32_t v){return std::bit_cast<float>(v);}
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float mul(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float integer(std::int32_t v){return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),v));}
bool above(float a,float b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc");return v;}
bool below_equal(float a,float b){unsigned char v;asm volatile("comiss %2,%1; setbe %0":"=qm"(v):"x"(a),"x"(b):"cc");return v;}
std::int32_t to_integer(float v){std::int32_t out;asm volatile("flds %1; fistpl %0":"=m"(out):"m"(v):"st");return out;}
std::uint32_t alpha_color(float alpha){const float scale=255.f;std::int32_t v;asm volatile("flds %1; fmuls %2; fistpl %0":"=m"(v):"m"(alpha),"m"(scale):"st");return std::bit_cast<std::uint32_t>(v)<<24;}
#endif
}
void vista_gameplay_fade_activate(VistaGameplayFadeState& s,unsigned user){
  if(user>=4)throw FormatError("Vista gameplay fade activation requires actual output user0..3");
  s.opacity_bits[user]=0x3f800000;s.activation_mask|=std::uint8_t(1u<<user);
}
void vista_gameplay_fade_tick(VistaGameplayFadeState& s,const VistaGameplayFadeTick& in,
    const VistaLoadingFloatingPoint& fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!in.engine_present)return; // Whole47590F admission, before consumed tail.
  Scope scope(fp);
  if(in.phase==1 && (in.playback || in.pending_phase==1))s.phase_counter=0;
  else s.phase_counter=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(s.phase_counter)+1));
  for(unsigned user=0;user<4;++user){
    if(!in.users[user] || !in.users[user]->player_present)continue;
    const auto& p=*in.users[user];bool increase=s.phase_counter>int(in.tick_rate)*4;
    if(!increase && !p.unit_present && p.respawn_countdown<=0){
      if(!p.active_player)throw FormatError("Missing Vista gameplay fade46C54F active-player owner");
      increase=p.active_player();
    }
    if(increase){
      const auto v=add(div(1.f,integer(in.tick_rate)),f(s.opacity_bits[user]));
      s.opacity_bits[user]=std::bit_cast<std::uint32_t>(above(1.f,v)?v:1.f);continue;
    }
    if((s.activation_mask&(1u<<user)) || !p.unit_present){
      if(p.unit_present)continue;
      if(!p.active_player)throw FormatError("Missing Vista gameplay fade46C54F active-player owner");
      if(p.active_player())continue;
      const auto delta=std::bit_cast<std::int32_t>(in.tick-in.round_start_tick);
      if((delta<0?0:delta)<=in.tick_rate)continue;
    }
    if(std::bit_cast<std::int32_t>(in.tick)<=to_integer(mul(integer(in.tick_rate),1.5f)))continue;
    const auto v=sub(f(s.opacity_bits[user]),div(3.f,integer(in.tick_rate)));
    s.opacity_bits[user]=std::bit_cast<std::uint32_t>(below_equal(0.f,v)?v:0.f);
  }
#else
  (void)s;(void)in;(void)fp;throw FormatError("Vista gameplay fade SSE backend unavailable");
#endif
}
std::optional<std::uint32_t> vista_gameplay_fade_render(VistaGameplayFadeState& s,
    const VistaGameplayFadeRender& in,const VistaLoadingFloatingPoint& fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!in.engine_present || in.cubemap_capture_in_progress)return std::nullopt;
  if(in.user!=0xffffffffu && in.user>=4)throw FormatError("Vista gameplay fade render requires actual output user0..3 orNONE");
  Scope scope(fp);
  if(in.user!=0xffffffffu){const auto u=in.user;const auto bit=std::uint8_t(1u<<u);
    if(s.activation_mask&bit){
      if(in.unresolved_world_geometry_count>0){s.rendered_frames[u]=std::uint8_t(s.rendered_frames[u]+1);
        if(int(s.rendered_frames[u])>to_integer(mul(integer(in.tick_rate),3.f)))s.activation_mask&=std::uint8_t(~bit);
      }else s.activation_mask&=std::uint8_t(~bit);
    }else s.rendered_frames[u]=0;
  }
  const float opacity=in.user==0xffffffffu?1.f:f(s.opacity_bits[in.user]);
  if(below_equal(opacity,0.f))return std::nullopt;
  return alpha_color(opacity);
#else
  (void)s;(void)in;(void)fp;throw FormatError("Vista gameplay fade SSE backend unavailable");
#endif
}
} // namespace halo2
