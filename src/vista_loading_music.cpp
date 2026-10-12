#include "halo2/vista_loading_music.hpp"

#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;

void loading_desired(VistaLoadingMusicState& state,const VistaLoadingMusicServices& services) {
  const auto request=services.loading_request();
  const auto map_state=request?services.map_state(services.path_conversion(*request)):3u;
  switch(map_state) {
  case 0:case 3:state.desired=1;break;
  case 1: {
    const auto shared=services.shared_ui();
    const auto duration=shared?shared->silence_ms:0u;
    if(services.estimate(*request)<=std::bit_cast<std::int32_t>(duration))state.desired=0;
    break;
  }
  case 2:state.desired=0;break;
  default:break;
  }
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FloatingPointScope {
  std::fenv_t saved_;
  unsigned mxcsr_;
public:
  explicit FloatingPointScope(const VistaLoadingFloatingPoint& context):mxcsr_(_mm_getcsr()) {
    if((context.x87_control_word&0x3f)!=0x3f ||
       (context.x87_control_word&0x300)==0x100 ||
       (context.mxcsr&0x1f80)!=0x1f80 || (context.mxcsr&0xffff0000))
      throw std::runtime_error("Original Vista music FP exception/precision context unsupported");
    if(std::fegetenv(&saved_)!=0)
      throw std::runtime_error("Original Vista music FP context unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope() { std::fesetenv(&saved_);_mm_setcsr(mxcsr_); }
};
float unsigned_integer(std::uint32_t value) {
  const auto signed_value=std::bit_cast<std::int32_t>(value);
  const float correction=std::bit_cast<float>(0x4f800000u);
  float result;
  if(signed_value<0)
    asm volatile("fildl %1; fadds %2; fstps %0":"=m"(result):"m"(signed_value),"m"(correction):"st");
  else
    asm volatile("fildl %1; fstps %0":"=m"(result):"m"(signed_value):"st");
  return result;
}
float signed_integer(std::uint32_t value) {
  return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(value)));
}
bool above(float a,float b) {
  unsigned char result;
  asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc");
  return result;
}
#endif
}

void vista_loading_music_initialize(VistaLoadingMusicState& state) {
  state.applied=state.requested=state.desired=state.previous_ui_clock=0;
  state.sound_handle=none;
}

void vista_loading_music_desired(VistaLoadingMusicState& state,
    std::optional<std::uint16_t> scenario_kind,const VistaLoadingMusicServices& services) {
  if(!scenario_kind || *scenario_kind!=2) { state.desired=0;return; }
  switch(services.current_screen()) {
  case 6:case 9:state.desired=1;break;
  case 13:case 14:case 186:case 247:state.desired=0;break;
  case 209:loading_desired(state,services);break;
  default:break;
  }
}

void vista_loading_music_update(VistaLoadingMusicState& state,
    std::optional<std::uint16_t> scenario_kind,const VistaLoadingMusicServices& services) {
  vista_loading_music_desired(state,scenario_kind,services);
  state.requested=services.audio_allowed()?state.desired:0;
  if(state.applied==state.requested)return;
  const auto now=services.ui_clock();
  if(state.previous_ui_clock && std::bit_cast<std::int32_t>(now-state.previous_ui_clock)<=6000)return;
  if(state.requested==1) {
    const auto shared=services.shared_ui();
    if(shared && shared->music_datum!=none)
      state.sound_handle=services.play(shared->music_datum,none,0x3f800000);
  } else if(state.sound_handle!=none) {
    services.stop(state.sound_handle);
    state.sound_handle=none;
    state.stopped_network_clock=services.network_clock();
  }
  state.applied=state.requested;
  state.previous_ui_clock=now;
}

bool vista_loading_music_ready(const VistaLoadingMusicState& state,
    const VistaLoadingMusicServices& services) {
  if(state.desired || state.sound_handle!=none)return false;
  const auto shared=services.shared_ui();
  const auto duration=shared?shared->silence_ms:0u;
  return duration<services.network_clock()-state.stopped_network_clock;
}

std::uint32_t vista_loading_music_secondary_bits(const VistaLoadingMusicState& state,
    const VistaLoadingMusicServices& services,const VistaLoadingFloatingPoint& context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  if(state.desired || state.sound_handle!=none)return 0;
  const auto shared=services.shared_ui();
  const float duration=signed_integer(shared?shared->silence_ms:0u);
  const float elapsed=unsigned_integer(services.network_clock()-state.stopped_network_clock);
  if(!above(duration,1.0f))return 0;
  float result=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(elapsed),_mm_set_ss(duration)));
  if(above(0.0f,result))result=0.0f;
  else if(above(result,1.0f))result=1.0f;
  return std::bit_cast<std::uint32_t>(result);
#else
  (void)state;(void)services;(void)context;
  throw std::runtime_error("Original Vista music x87/SSE backend unavailable");
#endif
}
} // namespace halo2
