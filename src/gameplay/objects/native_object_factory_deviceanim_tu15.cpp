// Native device animation owners (track "deviceanim"). Each owner replicates
// its TU1.5 routine; the oracle cases live in
// tools/native_object_factory_cases_deviceanim.py and the harness kernels in
// tests/native_object_factory_deviceanim_kernels.cpp. Animation-graph
// routines shared with the animation manager come from track "weapon".
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_object_factory_units_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2::native_object_factory_tu15::device_animation {
using namespace detail;
namespace wa=weapon_animation;
namespace {
namespace sse=native_campaign_script_fp;
constexpr std::uint32_t device_types=0x380u;  // machine, control, light fixture
constexpr std::uint32_t epsilon_bits=0x38d1b717u;      // [0x46A0EC] 1.0e-4
constexpr std::uint32_t seconds_per_tick=0x3d088889u;  // [0x46A17C]
constexpr std::uint32_t ticks_per_second=0x41f00000u;  // [0x46A20C] 30.0
constexpr std::uint32_t quarter=0x3e800000u,two=0x40000000u;  // [0x46A130], [0x46A11C]
constexpr std::uint32_t identity_orientation=0x474d38u;
constexpr std::uint32_t game_time=0x51df54u;

// Original stack locals that other owners address. Reads and writes inside
// the frame stay private; everything else goes through to the world memory
// in order. Nested frames take the next range (below track nodebuild's
// 0xFFFE0000 frame).
class FrameMemory final : public M {
public:
  static constexpr std::uint32_t size=0x1000u;
  explicit FrameMemory(M& world):world_(world),bytes_(size) {
    const auto* parent=dynamic_cast<const FrameMemory*>(&world);
    base_=parent?parent->base_+size:0xfffc0000u;
  }
  std::uint32_t allocate(std::uint32_t bytes) {
    const auto at=base_+top_;top_+=(bytes+15u)&~15u;
    if(top_>size)refuse("10CF80","native frame locals beyond 4 KiB");
    return at;
  }
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    if(local(address,output.size())){std::copy_n(bytes_.begin()+(address-base_),output.size(),output.begin());return;}
    world_.read(address,output);
  }
  void write(std::uint32_t address,std::span<const std::byte> input)override {
    if(local(address,input.size())){std::copy(input.begin(),input.end(),bytes_.begin()+(address-base_));return;}
    world_.write(address,input);
  }
private:
  bool local(std::uint32_t address,std::size_t bytes)const{return address>=base_ && address-base_+bytes<=size;}
  M& world_;
  std::vector<std::byte> bytes_;
  std::uint32_t base_=0,top_=0;
};

// SSE steps (bits in and out) in the original's destination, source order.
std::uint32_t mul(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return bits(sse::mulss(f32(a),f32(b),mx));}
std::uint32_t add(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return bits(sse::addss(f32(a),f32(b),mx));}
std::uint32_t sub(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return bits(sse::subss(f32(a),f32(b),mx));}
std::uint32_t div(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return bits(sse::divss(f32(a),f32(b),mx));}
bool above(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return sse::sse_compared(a,mx)>sse::sse_compared(b,mx);}
bool equal(std::uint32_t a,std::uint32_t b,std::uint32_t mx){return sse::sse_compared(a,mx)==sse::sse_compared(b,mx);}

#if HALO2_NATIVE_SCRIPT_X86_FP
std::uint16_t x87(const Fp& fp){return std::uint16_t(fp.x87_control_word|0x3fu);}
// FLD a; FMUL b; FSTP.
std::uint32_t x87_mul(std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float x=f32(a),y=f32(b);float r{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(r),"+m"(saved):"m"(mode),"m"(x),"m"(y):"st");
  return bits(r);
}
// FLD a; FSQRT; FDIV b or FMUL b; FSTP.
std::uint32_t x87_root_div(std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float x=f32(a),y=f32(b);float r{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsqrt\n\tfdivs %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(r),"+m"(saved):"m"(mode),"m"(x),"m"(y):"st");
  return bits(r);
}
std::uint32_t x87_root_mul(std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float x=f32(a),y=f32(b);float r{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsqrt\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(r),"+m"(saved):"m"(mode),"m"(x),"m"(y):"st");
  return bits(r);
}
// FLD a; FSUB b; FABS; FLD eps; FCOMIP; FSTP: eps above |a - b|.
bool x87_close(std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float x=f32(a),y=f32(b),e=f32(epsilon_bits);unsigned char r{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsubs %4\n\tfabs\n\tflds %5\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %0\n\tfldcw %1"
      :"=qm"(r),"+m"(saved):"m"(mode),"m"(x),"m"(y),"m"(e):"st","st(1)","cc");
  return r;
}
// 1D4170: FLD seconds; FMUL 30; FST frame; FABS; FLD eps; FCOMIP: the frame
// and whether eps is above |frame|.
std::pair<std::uint32_t,bool> x87_frame(std::uint32_t seconds,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float t=f32(seconds),k=f32(ticks_per_second),e=f32(epsilon_bits);
  float frame{};unsigned char still{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\tflds %4\n\tfmuls %5\n\tfsts %0\n\tfabs\n\tflds %6\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %1\n\tfldcw %2"
      :"=m"(frame),"=qm"(still),"+m"(saved):"m"(mode),"m"(t),"m"(k),"m"(e):"st","st(1)","cc");
  return {bits(frame),still!=0};
}
// 30BD0: x87 length of (x, y, z) stored as a float, and eps above |length|.
std::pair<std::uint32_t,bool> x87_length(std::uint32_t x,std::uint32_t y,std::uint32_t z,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float a=f32(x),b=f32(y),c=f32(z),e=f32(epsilon_bits);
  float length{};unsigned char small{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\tflds %6\n\tflds %5\n\tflds %4\n\t"
      "fld %%st(0)\n\tfmul %%st(1), %%st\n\tfld %%st(2)\n\tfmul %%st(3), %%st\n\tfaddp %%st, %%st(1)\n\t"
      "fld %%st(3)\n\tfmul %%st(4), %%st\n\tfaddp %%st, %%st(1)\n\tfsqrt\n\tfstp %%st(3)\n\t"
      "fstp %%st(0)\n\tfstp %%st(0)\n\tfsts %0\n\tfabs\n\tflds %7\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %1\n\tfldcw %2"
      :"=m"(length),"=qm"(small),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c),"m"(e)
      :"st","st(1)","st(2)","st(3)","st(4)","cc");
  return {bits(length),small!=0};
}
// 144A60: FLD a; [FSUB b; FSUB c;] FADD 1; FSQRT; FMUL 0.5; FST.
std::uint32_t x87_half_root(std::uint32_t a,const std::uint32_t* b,const std::uint32_t* c,const Fp& fp) {
  const auto mode=x87(fp);std::uint16_t saved{};const float x=f32(a),one_f=1.f,half=.5f;float r{};
  if(!b) {
    asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfadds %4\n\tfsqrt\n\tfmuls %5\n\tfstps %0\n\tfldcw %1"
        :"=m"(r),"+m"(saved):"m"(mode),"m"(x),"m"(one_f),"m"(half):"st");
  } else {
    const float y=f32(*b),z=f32(*c);
    asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsubs %4\n\tfsubs %5\n\tfadds %6\n\tfsqrt\n\t"
        "fmuls %7\n\tfstps %0\n\tfldcw %1"
        :"=m"(r),"+m"(saved):"m"(mode),"m"(x),"m"(y),"m"(z),"m"(one_f),"m"(half):"st");
  }
  return bits(r);
}
// RSQRTPS under the live MXCSR (the host's approximation, as the original runs).
std::array<std::uint32_t,4> rsqrt4(const std::array<std::uint32_t,4>& v,std::uint32_t mxcsr) {
  const auto mode=sse::sse_control(mxcsr);std::uint32_t saved{};
  __m128 x=_mm_castsi128_ps(_mm_set_epi32(int(v[3]),int(v[2]),int(v[1]),int(v[0])));
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\trsqrtps %0, %0\n\tldmxcsr %1":"+x"(x),"+m"(saved):"m"(mode));
  alignas(16) std::array<std::uint32_t,4> out{};
  _mm_store_ps(reinterpret_cast<float*>(out.data()),x);
  return out;
}
#else
[[noreturn]] void no_x87(){throw std::runtime_error("Exact native TU1.5 device animation x87 backend unavailable");}
std::uint32_t x87_mul(std::uint32_t,std::uint32_t,const Fp&){no_x87();}
std::uint32_t x87_root_div(std::uint32_t,std::uint32_t,const Fp&){no_x87();}
std::uint32_t x87_root_mul(std::uint32_t,std::uint32_t,const Fp&){no_x87();}
bool x87_close(std::uint32_t,std::uint32_t,const Fp&){no_x87();}
std::pair<std::uint32_t,bool> x87_frame(std::uint32_t,const Fp&){no_x87();}
std::pair<std::uint32_t,bool> x87_length(std::uint32_t,std::uint32_t,std::uint32_t,const Fp&){no_x87();}
std::uint32_t x87_half_root(std::uint32_t,const std::uint32_t*,const std::uint32_t*,const Fp&){no_x87();}
std::array<std::uint32_t,4> rsqrt4(const std::array<std::uint32_t,4>&,std::uint32_t){no_x87();}
#endif
bool missing(std::uint32_t handle){return std::uint16_t(handle>>16)==0xffffu;}
void mark_for_update(M& m,std::uint32_t object) {
  NativeCampaignScriptBank bank(m);
  native_campaign_script_tu15::object_mark_for_update(bank,object);
}
// 10D870/10D790 share one body; only the channel offset differs.
bool device_start_channel(M& m,std::uint32_t object,std::uint32_t name,std::uint32_t channel,const Fp& fp) {
  if(object==none)return false;
  const auto body=object_body(m,object);
  if(r16(m,body+0x12au)==0xffffu)return false;
  const auto manager=body+sx16(r16(m,body+0x12au));
  const auto record=body+channel;
  bool started=start_named_animation(m,manager,record,0x40u,name,fp);
  if(!started && r32(m,manager+0x68u)!=none) {
    FrameWord handle;
    overlay_lookup(m,tag_data(m,r32(m,manager+0x68u)),name,r32(m,manager+0x70u),r32(m,manager+0x74u),
        r32(m,manager+0x78u),nullptr,nullptr,nullptr,handle);
    if(!missing(handle()) && r32(m,manager+0x68u)!=none
        && wa::channel_start(m,record,r32(m,manager+0x68u),0x40u,handle(),name,0xffu,0xffu,1))
      started=true;
  }
  mark_for_update(m,object);
  return started;
}
}

void find_named_animation(const M& m,std::uint32_t graph,FrameWord& out,std::uint32_t name) {
  // 1E5360 is track units' animation_graph_find; its one store is the result.
  out.set(animation_graph_find(m,graph,name));
}

void overlay_lookup(const M& m,std::uint32_t graph,std::uint32_t key3,std::uint32_t r0,std::uint32_t r1,std::uint32_t r2,
    FrameWord* out0,FrameWord* out1,FrameWord* out2,FrameWord& result) {
  if(r1==none){result.set(none);return;}
  wa::ModeIterator it{FrameWord(graph),FrameWord(r0),FrameWord(r1),FrameWord(r2),FrameWord(0),FrameWord(0),FrameWord(0)};
  FrameWord k0(none),k1(none),k2(none);
  std::uint32_t found=none;
  while(const auto row=wa::mode_iterate(m,it,k0,k1,k2)) {
    found=none;
    if(const auto entry=wa::tag_block_search(m,row+0xcu,key3,8))found=r32(m,entry+4u);
    if(!missing(found))break;
  }
  if(out0)out0->set(k0());
  if(out1)out1->set(k1());
  if(out2)out2->set(k2());
  if(!missing(found)) {
    wa::graph_require_resident(m,graph,found);
    wa::graph_prefetch(m,graph,r0,r1,r2,true,false);
    wa::graph_prefetch(m,graph,k0(),k1(),k2(),true,false);
  }
  result.set(found);
}

bool start_named_animation(M& m,std::uint32_t manager,std::uint32_t record,std::uint32_t flags,std::uint32_t name,const Fp& fp) {
  if(r32(m,manager+0x68u)==none)return false;
  FrameWord handle;
  find_named_animation(m,tag_data(m,r32(m,manager+0x68u)),handle,name);
  if(missing(handle()))return false;
  return wa::manager_start_channel(m,manager,flags,record,handle(),name,0xffu,0xffu,0,fp);
}

bool device_start_overlay_channel(M& m,std::uint32_t object,std::uint32_t name,const Fp& fp) {
  return device_start_channel(m,object,name,0x1acu,fp);
}
bool device_start_position_channel(M& m,std::uint32_t object,std::uint32_t name,const Fp& fp) {
  return device_start_channel(m,object,name,0x18cu,fp);
}

bool device_set_overlay_track(M& m,std::uint32_t device,std::uint32_t name,const Fp& fp) {
  if(device==none)return false;
  {
    NativeCampaignScriptBank bank(m);
    if(!native_campaign_script_tu15::object_try_and_get_and_verify_type(bank,device,device_types))return false;
  }
  const auto body=object_body(m,device);
  if(r16(m,body+0x12au)==0xffffu)return false;
  for(const auto offset:{0x16cu,0x170u,0x174u,0x178u,0x184u,0x17cu,0x180u})w32(m,body+offset,0);
  bool started=device_start_overlay_channel(m,device,name,fp);
  if(!started)device_start_overlay_channel(m,device,0x5000081u,fp);
  mark_for_update(m,device);
  const auto flags=r32(m,body+0x12cu);
  w32(m,body+0x12cu,started?flags|0x10u:flags&~0x10u);
  return started;
}

std::uint32_t normalize3(M& m,std::uint32_t v,const Fp& fp) {
  const auto mx=fp.mxcsr;
  const auto [length,small]=x87_length(r32(m,v),r32(m,v+4u),r32(m,v+8u),fp);
  if(small)return 0;
  const auto x=r32(m,v);
  const auto inverse=div(one,length,mx);
  w32(m,v,mul(x,inverse,mx));
  const auto y=mul(inverse,r32(m,v+4u),mx),z=mul(inverse,r32(m,v+8u),mx);
  w32(m,v+4u,y);w32(m,v+8u,z);
  return length;
}

void basis_matrix(M& m,std::uint32_t out,std::uint32_t position,std::uint32_t forward,std::uint32_t up,const Fp& fp) {
  const auto mx=fp.mxcsr;
  w32(m,out,one);
  w32(m,out+4u,r32(m,forward));w32(m,out+8u,r32(m,forward+4u));w32(m,out+0xcu,r32(m,forward+8u));
  auto x0=r32(m,forward+4u),x1=r32(m,forward);
  x1=mul(x1,r32(m,up+4u),mx);x0=mul(x0,r32(m,up),mx);
  auto x2=mul(r32(m,forward+8u),r32(m,up),mx);
  const auto x3=mul(r32(m,up+8u),r32(m,forward+4u),mx);
  x0=sub(x0,x1,mx);
  x1=mul(r32(m,up+8u),r32(m,forward),mx);
  x1=sub(x1,x2,mx);
  x2=mul(r32(m,forward+8u),r32(m,up+4u),mx);
  w32(m,out+0x18u,x0);w32(m,out+0x14u,x1);
  x2=sub(x2,x3,mx);
  w32(m,out+0x10u,x2);
  w32(m,out+0x1cu,r32(m,up));w32(m,out+0x20u,r32(m,up+4u));w32(m,out+0x24u,r32(m,up+8u));
  w32(m,out+0x28u,0);w32(m,out+0x2cu,0);w32(m,out+0x30u,0);
  w32(m,out+0x28u,r32(m,position));w32(m,out+0x2cu,r32(m,position+4u));w32(m,out+0x30u,r32(m,position+8u));
}

void matrix_inverse(M& m,std::uint32_t out,std::uint32_t in,const Fp& fp) {
  const auto mx=fp.mxcsr;
  if(equal(r32(m,in),0,mx)){for(std::uint32_t n=0;n<13u;++n)w32(m,out+4u*n,0);return;}
  const auto scale=r32(m,in);
  const bool unit=equal(scale,one,mx);
  auto t0=sub(0,r32(m,in+0x28u),mx),t1=sub(0,r32(m,in+0x2cu),mx),t2=sub(0,r32(m,in+0x30u),mx);
  if(!unit) {
    const auto inverse=div(one,scale,mx);
    const auto x=mul(inverse,t0,mx);
    w32(m,out,inverse);
    t1=mul(inverse,t1,mx);t2=mul(inverse,t2,mx);t0=x;
  } else w32(m,out,one);
  w32(m,out+4u,r32(m,in+4u));
  auto e=r32(m,in+0x14u);auto x5=r32(m,out+4u);w32(m,out+0x14u,e);
  w32(m,out+0x24u,r32(m,in+0x24u));
  auto x2=r32(m,in+0x10u);e=r32(m,in+8u);w32(m,out+8u,x2);w32(m,out+0x10u,e);
  x2=r32(m,in+0x1cu);e=r32(m,in+0xcu);x5=mul(x5,t0,mx);w32(m,out+0x1cu,e);
  auto x4=mul(r32(m,out+0x1cu),t2,mx);x4=add(x4,x5,mx);x5=r32(m,out+0x10u);
  w32(m,out+0xcu,x2);
  x2=r32(m,in+0x20u);const auto c=r32(m,in+0x18u);
  x5=mul(x5,t1,mx);x4=add(x4,x5,mx);x5=r32(m,out+8u);
  w32(m,out+0x28u,x4);
  x5=mul(x5,t0,mx);w32(m,out+0x20u,c);x4=mul(r32(m,out+0x20u),t2,mx);x4=add(x4,x5,mx);
  x5=mul(t1,r32(m,out+0x14u),mx);x4=add(x4,x5,mx);w32(m,out+0x2cu,x4);
  x4=mul(r32(m,out+0xcu),t0,mx);auto x0=r32(m,out+0x24u);w32(m,out+0x18u,x2);x0=mul(x0,t2,mx);x4=add(x4,x0,mx);
  x2=mul(x2,t1,mx);x4=add(x4,x2,mx);w32(m,out+0x30u,x4);
}

void rotation_quaternion(M& m,std::uint32_t out,std::uint32_t r,const Fp& fp) {
  const auto mx=fp.mxcsr;
  const auto trace=add(add(r32(m,r+0x10u),r32(m,r),mx),r32(m,r+0x20u),mx);
  if(above(trace,0,mx)) {
    const auto w=x87_half_root(trace,nullptr,nullptr,fp);
    const auto k=div(quarter,w,mx);
    w32(m,out+0xcu,w);
    w32(m,out,mul(sub(r32(m,r+0x14u),r32(m,r+0x1cu),mx),k,mx));
    w32(m,out+4u,mul(sub(r32(m,r+0x18u),r32(m,r+8u),mx),k,mx));
    w32(m,out+8u,mul(sub(r32(m,r+4u),r32(m,r+0xcu),mx),k,mx));
  } else {
    std::uint32_t i=0;
    if(above(r32(m,r+0x10u),r32(m,r),mx))i=1;
    if(above(r32(m,r+0x20u),r32(m,r+(i<<4)),mx))i=2;
    const auto j=(i+1u)%3u,k=(i+2u)%3u;
    const auto dj=r32(m,r+(j<<4)),dk=r32(m,r+(k<<4));
    const auto v=x87_half_root(r32(m,r+(i<<4)),&dj,&dk,fp);
    const auto f=div(quarter,v,mx);
    w32(m,out+i*4u,v);
    w32(m,out+0xcu,mul(sub(r32(m,r+(j*3u+k)*4u),r32(m,r+(k*3u+j)*4u),mx),f,mx));
    w32(m,out+j*4u,mul(add(r32(m,r+(i*3u+j)*4u),r32(m,r+(j*3u+i)*4u),mx),f,mx));
    w32(m,out+k*4u,mul(add(r32(m,r+(i*3u+k)*4u),r32(m,r+(k*3u+i)*4u),mx),f,mx));
  }
  if(above(0,r32(m,out+0xcu),mx)) {
    w32(m,out+0xcu,sub(0,r32(m,out+0xcu),mx));
    w32(m,out,sub(0,r32(m,out),mx));
    const auto y=sub(0,r32(m,out+4u),mx),z=sub(0,r32(m,out+8u),mx);
    w32(m,out+4u,y);w32(m,out+8u,z);
  }
}

void matrix_orientation(M& m,std::uint32_t out,std::uint32_t matrix,const Fp& fp) {
  rotation_quaternion(m,out,matrix+4u,fp);
  w32(m,out+0x10u,r32(m,matrix+0x28u));w32(m,out+0x14u,r32(m,matrix+0x2cu));w32(m,out+0x18u,r32(m,matrix+0x30u));
  w32(m,out+0x1cu,r32(m,matrix));
}

void quaternion_product(M& m,std::uint32_t out,std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mx=fp.mxcsr;
  // An operand equal to `out` is first copied (it would be overwritten).
  std::array<std::uint32_t,4> p{},q{};
  for(std::uint32_t n=0;n<4u;++n){p[n]=r32(m,a+4u*n);q[n]=r32(m,b+4u*n);}
  auto x=add(add(mul(q[3],p[0],mx),mul(q[2],p[1],mx),mx),mul(p[3],q[0],mx),mx);
  w32(m,out,sub(x,mul(q[1],p[2],mx),mx));
  x=add(add(mul(p[2],q[0],mx),mul(q[3],p[1],mx),mx),mul(p[3],q[1],mx),mx);
  w32(m,out+4u,sub(x,mul(p[0],q[2],mx),mx));
  x=add(add(mul(q[3],p[2],mx),mul(q[1],p[0],mx),mx),mul(p[3],q[2],mx),mx);
  w32(m,out+8u,sub(x,mul(q[0],p[1],mx),mx));
  x=sub(sub(sub(mul(q[3],p[3],mx),mul(p[0],q[0],mx),mx),mul(q[1],p[1],mx),mx),mul(p[2],q[2],mx),mx);
  w32(m,out+0xcu,x);
}

void orientation_compose(M& m,std::uint32_t out,std::uint32_t a,std::uint32_t b,const Fp& fp) {
  const auto mx=fp.mxcsr;
  // 120570 copies an operand equal to `out` into its frame before writing.
  FrameMemory frame(m);
  if(a==out || b==out) {
    const auto copy=frame.allocate(0x20);
    for(std::uint32_t n=0;n<8u;++n)w32(frame,copy+4u*n,r32(m,out+4u*n));
    if(a==out)a=copy;
    if(b==out)b=copy;
  }
  quaternion_product(frame,out,a,b,fp);
  const auto A=[&](std::uint32_t o){return r32(frame,a+o);};
  const auto B=[&](std::uint32_t o){return r32(frame,b+o);};
  auto x0=mul(A(4),B(0x14),mx);
  auto x3=mul(B(0x10),A(0),mx);
  auto x1=A(0xc);
  auto x4=A(0xc);
  auto x6=mul(A(4),B(0x10),mx);
  auto x7=mul(B(0x14),A(8),mx);
  x0=add(x0,x3,mx);
  x3=mul(B(0x18),A(8),mx);
  x4=mul(x4,two,mx);
  x0=add(x0,x3,mx);
  x3=mul(A(0),B(0x18),mx);
  x0=mul(x0,two,mx);
  x1=mul(x1,x1,mx);x1=mul(x1,two,mx);
  auto x2=mul(B(0x10),A(8),mx);
  x2=sub(x2,x3,mx);
  x3=mul(A(0),B(0x14),mx);
  x3=sub(x3,x6,mx);
  x6=mul(A(4),B(0x18),mx);
  x6=sub(x6,x7,mx);
  x7=B(0x10);
  x6=mul(x6,x4,mx);
  x1=sub(x1,one,mx);
  x7=mul(x7,x1,mx);
  x6=add(x6,x7,mx);
  x7=mul(A(0),x0,mx);
  x6=add(x6,x7,mx);
  w32(frame,out+0x10u,x6);
  x6=A(4);
  x2=mul(x2,x4,mx);
  x6=mul(x6,x0,mx);
  x6=add(x6,x2,mx);
  x2=mul(x1,B(0x14),mx);
  x6=add(x6,x2,mx);
  w32(frame,out+0x14u,x6);
  x2=B(0x18);
  x0=mul(x0,A(8),mx);
  x2=mul(x2,x1,mx);
  x3=mul(x3,x4,mx);
  x2=add(x2,x3,mx);
  x2=add(x2,x0,mx);
  w32(frame,out+0x18u,x2);
  const auto scale=A(0x1c);
  if(!equal(scale,one,mx)) {
    for(const auto o:{0x10u,0x14u,0x18u})w32(frame,out+o,mul(r32(frame,out+o),scale,mx));
    w32(frame,out+0x1cu,mul(B(0x1c),A(0x1c),mx));
  } else w32(frame,out+0x1cu,B(0x1c));
  for(const auto o:{0x10u,0x14u,0x18u})w32(frame,out+o,add(A(o),r32(frame,out+o),mx));
}

std::uint32_t graph_node_index(const M& m,std::uint32_t manager,std::uint32_t name) {
  const auto graph=tag_data(m,r32(m,manager+0x68u));
  const auto count=s32(r32(m,graph+0xcu));
  auto row=r32(m,graph+0x10u);
  for(std::int32_t n=0;n<count;++n,row+=0x20u)if(r32(m,row)==name)return std::uint32_t(n);
  return none;
}

std::uint32_t codec_entry(const M& m,std::uint32_t manager,std::uint32_t handle) {
  if(missing(handle) || std::uint16_t(handle)==0xffffu)return 0;
  return animation_codec_entry(m,tag_data(m,r32(m,manager+0x68u)),handle);
}

bool static_pose(M& m,std::uint32_t graph,std::uint32_t handle,std::uint32_t out,const Fp& fp) {
  const auto mx=fp.mxcsr;
  // 1E5B30: the owner graph and animation row of the handle.
  std::uint32_t owner=graph;
  if(std::uint16_t(handle)!=0xffffu) {
    owner=0;
    const auto child=sx16(std::uint16_t(handle));
    if(s32(child)<s32(r32(m,graph+0x4cu)))
      if(const auto tag=r32(m,(child<<5)+r32(m,graph+0x50u)+4u);tag!=none)owner=tag_data(m,tag);
  }
  const auto index=std::uint32_t(std::int32_t(handle)>>16);
  const auto row=index==none?0u:index*0x6cu+r32(m,owner+0x30u);
  const auto pose=r16(m,row+0x2cu);
  if(pose==0xffffu) {
    const auto identity=r32(m,identity_orientation);
    for(std::uint32_t n=0;n<8u;++n)w32(m,out+4u*n,r32(m,identity+4u*n));
    return false;
  }
  const auto entry=sx16(pose)*24u+r32(m,owner+0xb8u);
  std::array<std::uint32_t,4> q{},square{},sum{};
  for(std::uint32_t n=0;n<4u;++n)q[n]=bits(float(std::int16_t(r16(m,entry+2u*n))));
  for(std::uint32_t n=0;n<4u;++n)square[n]=mul(q[n],q[n],mx);
  // MULPS, SHUFPS 0x4E + ADDPS, SHUFPS 0x11 + ADDPS, RSQRTPS, MULPS.
  std::array<std::uint32_t,4> half{};
  for(std::uint32_t n=0;n<4u;++n)half[n]=add(square[n],square[(n+2u)&3u],mx);
  constexpr std::array<std::uint32_t,4> pick{1u,0u,1u,0u};
  for(std::uint32_t n=0;n<4u;++n)sum[n]=add(half[n],half[pick[n]],mx);
  const auto r=rsqrt4(sum,mx);
  std::array<std::byte,16> packed{};
  for(std::uint32_t n=0;n<4u;++n) {
    const auto v=mul(q[n],r[n],mx);
    for(std::uint32_t k=0;k<4u;++k)packed[4u*n+k]=std::byte(v>>(8u*k));
  }
  m.write(out,packed);  // one MOVAPS store
  w32(m,out+0x10u,r32(m,entry+8u));w32(m,out+0x14u,r32(m,entry+0xcu));w32(m,out+0x18u,r32(m,entry+0x10u));
  w32(m,out+0x1cu,r32(m,entry+0x14u));
  wa::graph_require_resident(m,graph,handle);
  return true;
}

void sample_orientation(M& m,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds,std::uint32_t out,
    const Fp& fp,const CodecService& codec) {
  const auto [frame,still]=x87_frame(seconds,fp);
  if(still && static_pose(m,tag_data(m,r32(m,manager+0x68u)),handle,out,fp))return;
  const auto graph=tag_data(m,r32(m,manager+0x68u));
  const auto entry=codec_entry(m,manager,handle);
  if(!codec)refuse("283A90","animation codec (track nodebuild)");
  codec(m,graph,handle,frame,entry,out);
}

void sample_root(M& m,std::uint32_t out,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds,
    const Fp& fp,const CodecService& codec) {
  FrameMemory frame(m);
  const auto orientation=frame.allocate(0x20);
  const auto identity=r32(m,identity_orientation);
  for(std::uint32_t n=0;n<8u;++n)w32(frame,orientation+4u*n,r32(m,identity+4u*n));
  sample_orientation(frame,manager,handle,seconds,orientation,fp,codec);
  quaternion_rotation(frame,orientation,out+4u,fp.mxcsr);
  w32(frame,out+0x28u,0);w32(frame,out+0x2cu,0);w32(frame,out+0x30u,0);
  w32(frame,out+0x28u,r32(frame,orientation+0x10u));w32(frame,out+0x2cu,r32(frame,orientation+0x14u));
  w32(frame,out,one);w32(frame,out+0x30u,r32(frame,orientation+0x18u));
}

void sample_root_velocity(M& m,std::uint32_t out,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds,
    std::uint32_t rate,const Fp& fp,const CodecService& codec) {
  const auto mx=fp.mxcsr;
  if(!above(seconds,seconds_per_tick,mx))seconds=seconds_per_tick;
  FrameMemory frame(m);
  const auto before=frame.allocate(0x20),after=frame.allocate(0x20);
  const auto identity=r32(m,identity_orientation);
  for(std::uint32_t n=0;n<8u;++n)w32(frame,before+4u*n,r32(m,identity+4u*n));
  for(std::uint32_t n=0;n<8u;++n)w32(frame,after+4u*n,r32(m,identity+4u*n));
  sample_orientation(frame,manager,handle,sub(seconds,seconds_per_tick,mx),before,fp,codec);
  sample_orientation(frame,manager,handle,seconds,after,fp,codec);
  const auto scale=mul(rate,ticks_per_second,mx);
  for(std::uint32_t k=0;k<3u;++k)
    w32(frame,out+4u*k,mul(sub(r32(frame,after+0x10u+4u*k),r32(frame,before+0x10u+4u*k),mx),scale,mx));
}

std::uint32_t channel_seconds(const M& m,std::uint32_t record,const Fp& fp) {
  if(r16(m,record+6u)==0xffffu)return 0;
  const auto animation=wa::graph_animation(m,tag_data(m,r32(m,record)),r32(m,record+4u));
  return mul(bits(sse::cvtsi2ss(std::int16_t(r16(m,animation+0x14u)),fp.mxcsr)),seconds_per_tick,fp.mxcsr);
}

bool object_has_node_orientations(const M& m,std::uint32_t object) {
  return r16(m,object_body(m,object)+0x112u)!=0xffffu;
}

bool device_set_position_track(M& world,std::uint32_t device,std::uint32_t name,std::uint32_t seconds,const Fp& fp,
    const PositionTrackServices& services) {
  if(device==none)return false;
  std::uint32_t body=0;
  {
    NativeCampaignScriptBank bank(world);
    body=native_campaign_script_tu15::object_try_and_get_and_verify_type(bank,device,device_types);
  }
  if(!body)return false;
  const auto model=r32(world,tag_data(world,r32(world,body))+0x38u);
  if(model==none)return false;
  const auto render=r32(world,tag_data(world,model)+4u);
  if(render==none)return false;
  (void)tag_data(world,render);  // 10CFF1: read, then passed unused to 1D4200
  if(!object_has_animation(world,device))return false;
  const auto mx=fp.mxcsr;
  FrameMemory m(world);
  const auto manager=body+sx16(r16(m,body+0x12au));
  const auto basis=m.allocate(0x34),root=m.allocate(0x34),first=m.allocate(0x34),last=m.allocate(0x34);
  const auto inverse=m.allocate(0x34),delta=m.allocate(0x34),orientation=m.allocate(0x20);
  const auto forward=m.allocate(12),up=m.allocate(12);
  bool sampled=false;
  std::uint32_t speed=0;
  basis_matrix(m,basis,body+0x64u,body+0x70u,body+0x7cu,fp);
  for(std::uint32_t n=0;n<13u;++n)w32(m,root+4u*n,r32(m,basis+4u*n));
  if(r8(m,body+0x12cu)&8u) {
    const auto at=x87_mul(r32(m,body+0x1a8u),seconds_per_tick,fp);
    const auto handle=r32(m,body+0x190u);
    sample_root(m,last,manager,handle,0,fp,services.codec);
    sample_root(m,first,manager,handle,at,fp,services.codec);
    if(const auto node=graph_node_index(m,manager,0xd000533u);node!=none) {
      const auto matrix=object_node_matrix(m,device,node);
      for(std::uint32_t n=0;n<13u;++n)w32(m,root+4u*n,r32(m,matrix+4u*n));
    } else {
      matrix_inverse(m,inverse,last,fp);
      matrix_product(m,first,inverse,delta,mx);
      matrix_product(m,root,delta,root,mx);
    }
    const auto tick=r32(m,r32(m,game_time)+4u);
    sampled=true;
    if(above(tick,0,mx)) {
      const auto earlier=sub(at,tick,mx);
      if(above(earlier,0,mx)) {
        sample_root(m,last,manager,handle,earlier,fp,services.codec);
        const auto dy=sub(r32(m,first+0x2cu),r32(m,last+0x2cu),mx);
        const auto dx=sub(r32(m,first+0x28u),r32(m,last+0x28u),mx);
        const auto dz=sub(r32(m,first+0x30u),r32(m,last+0x30u),mx);
        const auto squared=add(add(mul(dz,dz,mx),mul(dy,dy,mx),mx),mul(dx,dx,mx),mx);
        if(above(squared,0,mx))speed=x87_root_div(squared,tick,fp);
      }
    }
  }
  for(const auto offset:{0x14cu,0x150u,0x154u,0x158u,0x164u,0x15cu,0x160u,0x140u,0x144u})w32(m,body+offset,0);
  if(sampled && r32(m,body+0x14u)==none) {
    w32(m,forward+8u,r32(m,root+0xcu));w32(m,forward,r32(m,root+4u));w32(m,forward+4u,r32(m,root+8u));
    w32(m,up+8u,r32(m,root+0x24u));w32(m,up,r32(m,root+0x1cu));w32(m,up+4u,r32(m,root+0x20u));
    normalize3(m,forward,fp);
    normalize3(m,up,fp);
    const std::array<std::pair<std::uint32_t,std::uint32_t>,9> pairs{{
        {basis+0x28u,root+0x28u},{basis+0x2cu,root+0x2cu},{basis+0x30u,root+0x30u},
        {basis+4u,forward},{basis+8u,forward+4u},{basis+0xcu,forward+8u},
        {basis+0x1cu,up},{basis+0x20u,up+4u},{basis+0x24u,up+8u}}};
    bool moved=false;
    for(const auto& [a,b]:pairs)if(!x87_close(r32(m,a),r32(m,b),fp)){moved=true;break;}
    if(moved) {
      if(!services.set_position)refuse("B95D0","object set position service (track placement)");
      services.set_position(m,device,root+0x28u,forward,up);
      if(object_has_node_orientations(m,device)) {
        const auto moved_body=object_body(m,device);
        const auto nodes=moved_body+sx16(r16(m,moved_body+0x112u));
        const auto previous=moved_body+sx16(r16(m,moved_body+0x10eu));
        matrix_orientation(m,orientation,basis,fp);
        orientation_compose(m,nodes,orientation,nodes,fp);
        orientation_compose(m,previous,orientation,previous,fp);
        basis_matrix(m,delta,body+0x64u,body+0x70u,body+0x7cu,fp);
        matrix_inverse(m,inverse,delta,fp);
        matrix_orientation(m,orientation,inverse,fp);
        orientation_compose(m,nodes,orientation,nodes,fp);
        orientation_compose(m,previous,orientation,previous,fp);
      }
    }
  }
  if(above(seconds,0,mx) && object_has_node_orientations(m,device)) {
    UnitFp unit{};unit.control_word=fp.x87_control_word;unit.mxcsr=mx;
    object_interpolation_start(m,device,seconds,unit);
  }
  bool started=device_start_position_channel(m,device,name,fp);
  if(!started)device_start_position_channel(m,device,0x8000080u,fp);
  else if(above(speed,0,mx)) {
    const auto handle=r32(m,body+0x190u);
    const auto length=channel_seconds(m,body+0x18cu,fp);
    if(above(length,0,mx)) {
      sample_root(m,basis,manager,handle,0,fp,services.codec);
      sample_root(m,first,manager,handle,length,fp,services.codec);
      const auto dy=sub(r32(m,first+0x2cu),r32(m,basis+0x2cu),mx);
      const auto dx=sub(r32(m,first+0x28u),r32(m,basis+0x28u),mx);
      const auto dz=sub(r32(m,first+0x30u),r32(m,basis+0x30u),mx);
      const auto squared=add(add(mul(dz,dz,mx),mul(dy,dy,mx),mx),mul(dx,dx,mx),mx);
      if(above(squared,0,mx))w32(m,body+0x150u,x87_root_mul(squared,speed,fp));
    }
  }
  mark_for_update(m,device);
  if(!services.rebuild_nodes)refuse("BF050","object node rebuild service (track nodebuild)");
  services.rebuild_nodes(m,device);
  const auto flags=r32(m,body+0x12cu)|4u;
  w32(m,body+0x12cu,flags);
  w32(m,body+0x12cu,started?flags|8u:flags&~8u);
  return started;
}

bool device_set_position_track(M& m,std::uint32_t device,std::uint32_t name,std::uint32_t seconds,const Fp& fp) {
  PositionTrackServices services;
  const auto frame_fp=[&]{NativeTu15PhysicsFrameFp f{};f.control_word=fp.x87_control_word;f.mxcsr=fp.mxcsr;return f;};
  services.rebuild_nodes=[&](M& memory,std::uint32_t object){auto f=frame_fp();object_rebuild_nodes(memory,object,f);};
  services.set_position=[&](M& memory,std::uint32_t object,std::uint32_t position,std::uint32_t forward,std::uint32_t up) {
    auto f=frame_fp();
    PlacementServices placement{[&](std::uint32_t moved){auto g=frame_fp();object_rebuild_nodes(memory,moved,g);}};
    object_set_position(memory,object,position,forward,up,0,0,f,placement);
  };
  return device_set_position_track(m,device,name,seconds,fp,services);
}
}
