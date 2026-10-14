#include "halo2/source_campaign_coop.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;
using O=SourcePhysicsContactRecordObserver;
constexpr auto none=0xffffffffu;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
  std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(n));
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return v;
}
std::int32_t si(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int16_t sh(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t v) {
  SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(n);s.written.resize(n);m.read(p,s.before);
  for(unsigned i=0;i<n;++i)s.written[i]=std::byte(v>>(i*8));m.write(p,s.written);if(o)o(s);
}
std::uint32_t body(const M&m,std::uint32_t pool,std::uint32_t datum) {
  return r(m,r(m,pool+0x44)+(datum&0xffffu)*12+8);
}
std::uint32_t valid_body(const M&m,std::uint32_t pool,std::uint32_t datum,std::uint32_t mask) {
  if(datum==none || std::int32_t(datum&0xffffu)>=si(r(m,pool+0x38)))return 0;
  const auto header=r(m,pool+0x44)+(datum&0xffffu)*r(m,pool+0x24);
  const auto salt=r(m,header,2);
  if(!salt || salt!=(datum>>16) || !(mask&(1u<<(r(m,header+3,1)&31u))))return 0;
  return r(m,header+8);
}
template<class F>void each_player(const M&m,std::uint32_t pool,F fn) {
  const auto high_water=si(r(m,pool+0x38));
  for(std::int32_t i=0;i<high_water;++i) {
    if(!(r(m,r(m,pool+0x48)+(std::uint32_t(i)>>5)*4)&(1u<<(i&31))))continue;
    const auto row=r(m,pool+0x44)+std::uint32_t(i)*r(m,pool+0x24);
    fn(row,(r(m,row,2)<<16)|std::uint32_t(i));
  }
}
template<class F,class... A>bool required(const F&fn,const char*name,A... args) {
  if(!fn)throw std::runtime_error(name);return fn(args...);
}
struct Scope {
  unsigned mx=_mm_getcsr();std::fenv_t env;
  explicit Scope(SourcePhysicsFactoryFloatingPoint fp) {
    if((fp.x87_control_word&0x3f)!=0x3f || (fp.mxcsr&0x1f80)!=0x1f80)
      throw std::runtime_error("Cooperative respawn requires masked original FP exceptions");
    std::fegetenv(&env);_mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
#else
    std::fesetround(std::array{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}[(fp.x87_control_word>>10)&3]);
#endif
  }
  ~Scope(){std::fesetenv(&env);_mm_setcsr(mx);}
};
}
bool source_campaign_coop_respawn_allowed(std::int32_t game_type,
    std::int16_t difficulty,bool override) {
  return game_type!=1 || (!override && difficulty!=3);
}
void source_campaign_coop_request_respawn(M&m,const SourceCampaignCoopContext&c,
    float delay,SourcePhysicsFactoryFloatingPoint fp,const O&o) {
  if(r(m,c.users+0x9e,1))return;
  Scope scope(fp);
  w(m,o,0x14e4d8,c.users+0x9e,1,1);w(m,o,0x14e4df,c.users+0x9f,1,0);
  volatile float value=float(sh(r(m,c.clock+2,2)))*delay;
  std::int32_t ticks;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1; fistpl %0":"=m"(ticks):"m"(value):"st");
#else
  ticks=std::int32_t(std::nearbyint(value));
#endif
  w(m,o,0x14e506,c.users+0xa0,4,std::bit_cast<std::uint32_t>(ticks));
}
void source_campaign_coop_update_respawn(M&m,const SourceCampaignCoopContext&c,
    const SourceCampaignCoopRespawnServices&s,const O&o) {
  const auto is_cinematic=[&]{return c.cinematic && r(m,c.cinematic+5,1);};
  const auto timer=si(r(m,c.users+0xa0));
  w(m,o,0x14e527,c.users+0x9c,2,0);
  if(timer>0 && !is_cinematic())w(m,o,0x14e53e,c.users+0xa0,4,std::uint32_t(timer-1));
  if(!r(m,c.users+0x9e,1))return;
  const bool cinematic=is_cinematic();
  if(cinematic)w(m,o,0x14e55d,c.users+0x9c,2,2);
  else if(!r(m,c.users+0x9f,1)) {
    if(si(r(m,c.users+0xa0))>0)w(m,o,0x14e57c,c.users+0x9c,2,3);
    else if(r(m,c.game+0x1121,1))w(m,o,0x14e596,c.users+0x9c,2,8);
    else if(required(s.action_unsafe,"Required originalCD5C0 cooperative safety owner unavailable"))
      w(m,o,0x14e5af,c.users+0x9c,2,4);
    else if(required(s.combat_unsafe,"Required original1CE060 cooperative combat owner unavailable"))
      w(m,o,0x14e5c3,c.users+0x9c,2,5);
  }
  if(r(m,c.users+0x9c,2))return;
  auto anchor=none;Vec3 position{};
  each_player(m,c.players,[&](auto row,auto) {
    const auto unit=r(m,row+0x2c);if(unit==none)return;
    auto root=unit;
    while(r(m,body(m,c.objects,root)+0x14)!=none)root=r(m,body(m,c.objects,root)+0x14);
    bool unsafe=false;
    if(root==unit) {
      if(valid_body(m,c.objects,unit,1))
        unsafe=required(s.biped_unsafe,"Required originalE5680 cooperative biped owner unavailable",unit);
    } else if(const auto vehicle=valid_body(m,c.objects,root,2))unsafe=r(m,vehicle+0x34c,1)!=0;
    if(unsafe){w(m,o,0x14e6da,c.users+0x9c,2,6);return;}
    anchor=unit;const auto object=body(m,c.objects,unit);
    position={std::bit_cast<float>(r(m,object+0x30)),std::bit_cast<float>(r(m,object+0x34)),
              std::bit_cast<float>(r(m,object+0x38))};
  });
  if(anchor==none)return;
  bool failed=false;
  each_player(m,c.players,[&](auto row,auto datum) {
    const auto flags=r(m,row+2,2);
    if((flags&2u) || !(flags&1u) || r(m,row+0x2c)!=none)return;
    if(!required(s.spawn,"Required original1509C0 cooperative birth owner unavailable",datum) ||
       !required(s.place,"Required original1500B0 cooperative placement owner unavailable",datum,anchor,position))
      failed=true;
  });
  if(!failed)w(m,o,0x14e765,c.users+0x9e,1,0);
}
} // namespace halo2
