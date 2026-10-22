#include "halo2/vista_widget_animation_consumer.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <unordered_set>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
#if defined(__i386__) || defined(__x86_64__)
std::int16_t signed_word(std::uint32_t v) {return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int32_t signed_dword(std::uint32_t v) {return std::bit_cast<std::int32_t>(v);}
__m128 value(std::uint32_t v) {return _mm_set_ss(std::bit_cast<float>(v));}
std::uint32_t bits(__m128 v) {return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
#endif
}
void vista_widget_animation_update(std::uint32_t widget,std::uint32_t now,
    const VistaWidgetAnimationConsumerServices& s) {
#if defined(__i386__) || defined(__x86_64__)
  if(!s.read || !s.write)throw FormatError("Vista animation update lacks its source registry");
  const auto r=[&](std::uint32_t off,std::size_t n=4){return s.read(widget+off,n);};
  const auto w=[&](std::uint32_t off,std::uint32_t v,std::size_t n=4){s.write(widget+off,v,n);};
  bool reactivate=false,dismiss=false;
  const auto endpoint=[&] {
    const auto flags=r(0x42,2);const auto root=r(4)==0;
    if((flags&2) && root){dismiss=true;return;}
    reactivate=root && (flags&3);
  };
  const auto positive_count=signed_word(r(0x3c,2))>0;
  const auto positive_duration=positive_count && signed_dword(r(0x54))>0;
  const auto start=positive_duration?r(0x50):0;
  if(positive_duration && start<=s.read(0xd71900u,4)) {
    const auto count=signed_word(r(0x3c,2));
    const auto duration=signed_dword(r(0x54));
    if(count==1)throw FormatError("Original Vista animation IDIV count1 fault");
    const auto step=std::uint32_t(duration/(std::int32_t(count)-1));
    auto end=start+step;auto elapsed=now-start;
    std::unordered_set<std::uint32_t> zero_step_states;
    while(now>=end) {
      auto direction=r(0x40,2);const auto current=r(0x3e,2);
      if(!step && !zero_step_states.insert((direction<<16)|current).second)
        throw FormatError("Original Vista animation zero-step loop repeats without progress");
      auto next=std::uint16_t(direction+current);
      if(signed_word(next)<0) {
        endpoint();if(dismiss)break;
        const auto mode=r(0x48);
        if(mode==0 || mode==2)next=std::uint16_t(current);
        else if(mode==1)next=std::uint16_t(r(0x3c,2)-1);
        else if(mode==3){direction=std::uint16_t(0-direction);w(0x40,direction,2);next=std::uint16_t(current);}
      } else if(next==r(0x3c,2)) {
        endpoint();if(dismiss)break;
        const auto mode=r(0x48);
        if(mode==0)next=std::uint16_t(current);
        else if(mode==1)next=0;
        else if(mode==2 || mode==3){direction=std::uint16_t(0-direction);w(0x40,direction,2);next=std::uint16_t(current);}
      }
      if(reactivate)break;
      if(!(r(0x42,1)&8))w(0x3e,next,2);
      elapsed-=step;end+=step;w(0x50,now);
    }
    if(!dismiss) {
      auto direction=r(0x40,2);auto current=r(0x3e,2);
      auto next=std::uint16_t(current+direction);
      if(signed_word(next)<0) {
        const auto mode=r(0x48);
        if(mode==0 || mode==2)next=std::uint16_t(current);
        else if(mode==1){current=std::uint16_t(r(0x3c,2)-1);w(0x3e,current,2);next=std::uint16_t(current-1);}
        else if(mode==3){direction=std::uint16_t(0-direction);w(0x40,direction,2);next=std::uint16_t(current+direction);}
      } else if(next==r(0x3c,2)) {
        const auto mode=r(0x48);
        if(mode==0)next=std::uint16_t(current);
        else if(mode==1){w(0x3e,0,2);next=1;}
        else if(mode==2 || mode==3){direction=std::uint16_t(0-direction);w(0x40,direction,2);next=std::uint16_t(current+direction);}
      }
      if(signed_dword(elapsed)<1)elapsed=1;
      if((_mm_getcsr()&0x1f80u)!=0x1f80u)
        throw FormatError("Unmasked Vista animation SSE exceptions are not admitted");
      const auto frames=r(0x38);
      const auto to=frames+std::uint32_t(std::int32_t(signed_word(next))*20);
      current=r(0x3e,2);
      const auto from=frames+std::uint32_t(std::int32_t(signed_word(current))*20);
      auto y=value(s.read(to+12,4));auto z=value(s.read(to+16,4));
      y=_mm_sub_ss(y,value(s.read(from+12,4)));z=_mm_sub_ss(z,value(s.read(from+16,4)));
      const auto ratio=_mm_div_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),signed_dword(elapsed)),
                                _mm_cvtsi32_ss(_mm_setzero_ps(),signed_dword(step)));
      auto x=_mm_sub_ss(value(s.read(to+8,4)),value(s.read(from+8,4)));
      y=_mm_mul_ss(y,ratio);x=_mm_mul_ss(x,ratio);x=_mm_add_ss(x,value(s.read(from+8,4)));w(0x58,bits(x));
      y=_mm_add_ss(y,value(s.read(from+12,4)));w(0x5c,bits(y));
      z=_mm_mul_ss(z,ratio);z=_mm_add_ss(z,value(s.read(from+16,4)));w(0x60,bits(z));
      auto alpha=value(s.read(to+4,4));const auto first_alpha=value(s.read(from+4,4));
      alpha=_mm_sub_ss(alpha,first_alpha);alpha=_mm_mul_ss(alpha,ratio);alpha=_mm_add_ss(alpha,first_alpha);w(0x64,bits(alpha));
    }
  } else {
    const auto flags=r(0x42,2);const auto root=r(4)==0;
    dismiss=(flags&2) && root;reactivate=root && (flags&3);
  }
  if(dismiss) {
    auto root=r(0x10);
    while(root) {const auto parent=s.read(root+0x10,4);if(!parent)break;root=parent;}
    if(!root && r(4)==0)root=widget;
    if(root==widget) {
      if(!s.dismiss)throw FormatError("Vista animation root dismissal requires its source service");
      s.dismiss(widget);return;
    }
  }
  if(reactivate) {
    if(!s.activate)throw FormatError("Vista animation endpoint requires its source activation service");
    s.activate(widget,4);
  }
#else
  (void)widget;(void)now;(void)s;
  throw FormatError("Original Vista animation SSE backend unavailable");
#endif
}
void vista_widget_animation_ambient_update(std::uint32_t widget,
    const VistaWidgetAnimationConsumerServices& s) {
  if(!s.read)throw FormatError("Vista ambient update lacks its source registry");
  const auto now=s.read(0xd71900u,4);
  vista_widget_animation_update(widget,now,s);
  auto child=s.read(widget+0x14,4);
  while(child) {
    if(!s.update_child)throw FormatError("Vista ambient update requires actual child virtual+0C");
    s.update_child(child);child=s.read(child+0x18,4);
  }
}
} // namespace halo2
