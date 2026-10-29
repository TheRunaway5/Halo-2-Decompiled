#include "halo2/native_campaign_ai_orders_camouflage_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);}
void dirty(SourcePhysicsContactMemory &m,std::uint32_t entity) {
  const auto session=read(m,0x4dc04cu);
  const auto mode=read(m,session+8u);
  if(mode!=4u && mode!=5u)return;
  if(mode==5u)return;
  const auto state=read(m,session+4u);
  const auto entities=read(m,state+0x20a4u);
  const auto index=entity&0x3ffu;
  if(!(read(m,entities+index*8u+0x44u,1)&4u))return;
  const auto target=state+0x2098u+index*36u+0x14u+0xcu;
  write(m,target,read(m,target)|0x800000u);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  NativeCampaignAiOrdersFp &fp_;std::uint32_t saved_;
public:
  explicit Environment(NativeCampaignAiOrdersFp &fp):fp_(fp){if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("AI order camouflage needs masked source SSE policy");saved_=_mm_getcsr();_mm_setcsr(fp.mxcsr);}
  ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(saved_);}
};
std::uint32_t multiply(std::uint32_t a,std::uint32_t b){auto left=_mm_set_ss(std::bit_cast<float>(a));const auto right=_mm_set_ss(std::bit_cast<float>(b));asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(left));}
bool control(SourcePhysicsContactMemory &m,std::uint32_t actor,bool enabled){
  const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u);
  const auto unit=read(m,rows+(actor&0xffffu)*0x888u+0x18u);
  if(unit==0xffffffffu)return false;
  std::uint32_t objects,speed,body,offset=(unit&0xffffu)*12u;
  if(enabled){
    objects=read(m,0x4ecbd0u);const auto units=read(m,objects+0x44u);
    speed=read(m,0x46a130u);body=read(m,units+offset+8u);
    write(m,body+0x134u,read(m,body+0x134u)|8u);write(m,body+0x2b8u,speed);
  } else {
    const auto supplied=read(m,0x46a0d0u);
    objects=read(m,0x4ecbd0u);
    speed=multiply(supplied,read(m,0x46a130u));
    const auto units=read(m,objects+0x44u);body=read(m,units+offset+8u);
    auto flags=read(m,body+0x134u)&~8u;write(m,body+0x134u,flags);
    flags&=~16u;write(m,body+0x134u,flags);write(m,body+0x2b8u,speed);
  }
  const auto current_units=read(m,objects+0x44u);
  const auto current_body=read(m,current_units+offset+8u);
  const auto entity=read(m,current_body+0xd4u);
  if(entity!=0xffffffffu)dirty(m,entity);
  return true;
}
#endif
}
bool native_campaign_ai_orders_camouflage_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t actor,bool enabled,NativeCampaignAiOrdersFp &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp);return control(m,actor,enabled);
#else
  (void)m;(void)actor;(void)enabled;(void)fp;throw std::runtime_error("Native AI order camouflage has no qualified source SSE policy on this architecture");
#endif
}
}
