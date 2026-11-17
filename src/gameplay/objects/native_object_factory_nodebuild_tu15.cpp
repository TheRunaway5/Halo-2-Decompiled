// Native object node-matrix rebuild (track "nodebuild"): BF050 and the
// routines it executes, replicated over the live world memory. The oracle
// cases are tools/native_object_factory_cases_nodebuild.py, the harness
// kernels tests/native_object_factory_nodebuild_kernels.cpp.
#include "halo2/native_object_factory_nodebuild_tu15.hpp"
#include "halo2/native_object_factory_nodes_tu15.hpp"
#include "halo2/native_object_factory_placement_tu15.hpp"
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/native_object_factory_units_tu15.hpp"
#include "halo2/native_object_factory_attachments_tu15.hpp"
#include "halo2/native_object_factory_modelstate_tu15.hpp"
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "halo2/native_tu15_page_allocator.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>
#include <cstdio>
#include <string>
#include <vector>

namespace halo2::native_object_factory_tu15 {
using namespace detail;
namespace {
namespace sse=native_campaign_script_fp;
using u32=std::uint32_t;
using u16=std::uint16_t;

std::string hex(u32 value){char text[16];std::snprintf(text,sizeof text,"%X",value);return text;}

// Literal TU1.5 .rdata constants.
constexpr u32 zero_f=0u;                  // [0x46A0E8] 0.0f
constexpr u32 epsilon_f=0x38d1b717u;      // [0x46A0EC] 1e-4f
constexpr u32 sphere_moved_f=0x3c23d70bu; // [0x46A560] 0.01f
constexpr u32 height_scale_f=0xbe000000u; // [0x46A410] -0.125f
constexpr u32 ramp_rate_f=0x40d55555u;    // [0x46A314] 6.6666665f
constexpr u32 sign_bit=0x80000000u;       // [0x45F490] lanes 0 and 3
constexpr u32 two_f=0x40000000u,three_f=0x40400000u,six_f=0x40c00000u; // [0x46A11C] [0x46A0E4] [0x46A10C]
constexpr u32 ten_f=0x41200000u,fifteen_f=0x41700000u;                // [0x46A110] [0x46A108]
constexpr u32 blend_limit_f=0x3f7ff972u,blend_scale_f=0x3f80068eu;    // [0x46A188] [0x46A184]
constexpr u32 type_table=0x474b90u;       // object type definitions
constexpr u32 render_only_mask=0x56eb8cu; // BF0C0 static node mask (+0x20 flag at 0x56EBAC)
constexpr u32 render_only_flag=0x56ebacu;
constexpr u32 orientation_scratch=0x4e8bc0u;
constexpr u32 identity_orientation=0x474d38u;
constexpr u32 epsilon_flag=0x56e684u,epsilon_value=0x56e680u;

// RCPSS under the live MXCSR: its approximation is the host instruction's.
float rcpss(float value,u32 mxcsr) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::sse_control(mxcsr);u32 saved{};float result=0.f;
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\trcpss %3, %0\n\tldmxcsr %1"
      :"+x"(result),"+m"(saved):"m"(mode),"x"(value));
  return result;
#else
  (void)value;(void)mxcsr;
  refuse("144CF0","RCPSS on a non-x86 host");
#endif
}

// Native stand-in for the original stack locals that routines pass by
// pointer: they live in a private extent above every world address, so
// aliasing and store order are the original's while only world stores
// reach memory.write().
// A Frame opened while another is live on the thread (a public owner
// called by another track on a FrameMemory, such as placement's BAD60 ->
// 145560) allocates above the enclosing frame's locals and forwards every
// lower address of the extent to its memory, so the enclosing locals stay
// reachable through it.
class Frame {
public:
  static constexpr u32 base=0xfffe0000u,size=0x4000u;
  Frame(M& m,u32 mxcsr,std::uint16_t x87=0x027fu):m_(m),own_{x87,0,mxcsr},fp_(own_){enter();}
  // The caller's FP controls: callees in other tracks may change them.
  Frame(M& m,NodeRebuildFp& fp):m_(m),fp_(fp){enter();}
  ~Frame(){innermost()=outer_;}
  Frame(const Frame&)=delete;
  Frame& operator=(const Frame&)=delete;
  M& world()const{return m_;}
  NodeRebuildFp& fp()const{return fp_;}
  u32 mxcsr()const{return fp_.mxcsr;}
  std::uint16_t x87()const{return fp_.control_word;}
  u32 r32(u32 a)const{return local(a,4)?load(a,4):detail::r32(m_,a);}
  u32 r16(u32 a)const{return local(a,2)?load(a,2):detail::r16(m_,a);}
  u32 r8(u32 a)const{return local(a,1)?load(a,1):detail::r8(m_,a);}
  void w32(u32 a,u32 v){if(local(a,4))save(a,v,4);else detail::w32(m_,a,v);}
  void w16(u32 a,u32 v){if(local(a,2))save(a,v,2);else detail::w16(m_,a,std::uint16_t(v));}
  void w8(u32 a,u32 v){if(local(a,1))save(a,v,1);else detail::w8(m_,a,std::uint8_t(v));}
  // One 8-byte publication (MOVLPS/MOVHPS).
  void w64(u32 a,u32 low,u32 high) {
    if(local(a,8)){save(a,low,4);save(a+4u,high,4);return;}
    const std::array b{std::byte(low),std::byte(low>>8),std::byte(low>>16),std::byte(low>>24),
                       std::byte(high),std::byte(high>>8),std::byte(high>>16),std::byte(high>>24)};
    m_.write(a,b);
  }
  u32 allocate(u32 bytes) {
    const auto at=base+top_;
    top_+=(bytes+3u)&~3u;
    if(top_>size)refuse("BF0C0","native frame locals beyond 16 KiB");
    return at;
  }
  // One 16-byte publication (MOVAPS).
  void w128(u32 a,const std::array<u32,4>& v) {
    if(local(a,16)){for(u32 k=0;k<4;++k)save(a+4u*k,v[k],4);return;}
    std::array<std::byte,16> b{};
    for(u32 k=0;k<16;++k)b[k]=std::byte(v[k/4u]>>(8u*(k%4u)));
    m_.write(a,b);
  }
  u32 mark()const{return top_;}
  void release(u32 mark){top_=mark;}
  // Scalar SSE steps (bits in, bits out); operand order is the original's
  // destination, source (NaN payload selection).
  u32 mul(u32 a,u32 b)const{return bits(sse::mulss(f32(a),f32(b),mxcsr()));}
  u32 add(u32 a,u32 b)const{return bits(sse::addss(f32(a),f32(b),mxcsr()));}
  u32 sub(u32 a,u32 b)const{return bits(sse::subss(f32(a),f32(b),mxcsr()));}
  u32 div(u32 a,u32 b)const{return bits(sse::divss(f32(a),f32(b),mxcsr()));}
  u32 from_int(u32 a)const{return bits(sse::cvtsi2ss(s32(a),mxcsr()));}
  u32 rcp(u32 a)const{return bits(rcpss(f32(a),mxcsr()));}
  // UCOMISS + LAHF/TEST AH,44h/JNP: equal and ordered (DAZ operands).
  bool equal(u32 a,u32 b)const{return sse::sse_compared(a,mxcsr())==sse::sse_compared(b,mxcsr());}
  // COMISS + JA: strictly greater and ordered.
  bool greater(u32 a,u32 b)const{return sse::sse_compared(a,mxcsr())>sse::sse_compared(b,mxcsr());}
  // COMISS + JAE: greater or equal and ordered.
  bool at_least(u32 a,u32 b)const{return sse::sse_compared(a,mxcsr())>=sse::sse_compared(b,mxcsr());}
private:
  static Frame*& innermost(){thread_local Frame* frame=nullptr;return frame;}
  void enter(){outer_=innermost();if(outer_)floor_=top_=outer_->top_;innermost()=this;}
  bool local(u32 a,u32 n)const {
    if(a<base+floor_)return false;
    if(a-base+n>size)refuse("BF0C0","native frame access past its locals");
    return true;
  }
  u32 load(u32 a,u32 n)const{u32 v=0;for(u32 k=0;k<n;++k)v|=u32(locals_[a-base+k])<<(8u*k);return v;}
  void save(u32 a,u32 v,u32 n){for(u32 k=0;k<n;++k)locals_[a-base+k]=std::uint8_t(v>>(8u*k));}
  M& m_;
  NodeRebuildFp own_{};
  NodeRebuildFp& fp_;
  Frame* outer_=nullptr;
  u32 floor_=0;
  u32 top_=0;
  std::array<std::uint8_t,size> locals_{};
};
class Locals {
public:
  explicit Locals(Frame& f):f_(f),mark_(f.mark()){}
  ~Locals(){f_.release(mark_);}
  Locals(const Locals&)=delete;
  Locals& operator=(const Locals&)=delete;
  u32 operator()(u32 bytes){return f_.allocate(bytes);}
private:
  Frame& f_;
  u32 mark_;
};

// The Frame as memory for callees of other tracks: its private extent
// reaches the Frame locals, every other address the world (in order and
// with the callee's widths).
class FrameMemory final : public M {
public:
  explicit FrameMemory(Frame& f):f_(f){}
  void read(u32 a,std::span<std::byte> out)const override {
    if(a<Frame::base){f_.world().read(a,out);return;}
    for(std::size_t k=0;k<out.size();++k)out[k]=std::byte(f_.r8(a+u32(k)));
  }
  void write(u32 a,std::span<const std::byte> in)override {
    if(a<Frame::base){f_.world().write(a,in);return;}
    for(std::size_t k=0;k<in.size();++k)f_.w8(a+u32(k),u32(in[k]));
  }
private:
  Frame& f_;
};

// REP MOVSD: ascending word copy, one read then one store per word.
void copy_words(Frame& f,u32 to,u32 from,u32 words){for(u32 n=0;n<words;++n)f.w32(to+4u*n,f.r32(from+4u*n));}

// 145560. A is read whole before the first store; each B row is read around
// the preceding row's store exactly as the original orders them.
u32 product(Frame& f,u32 a,u32 b,u32 out) {
  const u32 as=f.r32(a),bs=f.r32(b);
  const u32 bpx=f.r32(b+0x28u),bpy=f.r32(b+0x2cu),bpz=f.r32(b+0x30u);
  const u32 scale=f.mul(bs,as);
  std::array<u32,3> af,al,au,ap;
  for(u32 k=0;k<3;++k){af[k]=f.r32(a+4u+4u*k);al[k]=f.r32(a+0x10u+4u*k);au[k]=f.r32(a+0x1cu+4u*k);}
  for(u32 k=0;k<3;++k)ap[k]=f.r32(a+0x28u+4u*k);
  std::array<u32,3> row;
  for(u32 k=0;k<3;++k)
    row[k]=f.add(f.mul(f.add(f.add(f.mul(bpx,af[k]),f.mul(bpy,al[k])),f.mul(bpz,au[k])),as),ap[k]);
  // Up/left/forward: x*f + (y*l + z*u); z is read before the previous store.
  u32 z=f.r32(b+0x24u);
  f.w32(out+0x28u,row[0]);f.w64(out+0x2cu,row[1],row[2]);
  const auto basis=[&](u32 at,u32 next_z,u32 store_at) {
    const u32 y=f.r32(b+at+4u),x=f.r32(b+at);
    for(u32 k=0;k<3;++k)row[k]=f.add(f.mul(x,af[k]),f.add(f.mul(y,al[k]),f.mul(z,au[k])));
    if(next_z)z=f.r32(b+next_z);
    f.w32(out+store_at,row[0]);f.w64(out+store_at+4u,row[1],row[2]);
  };
  basis(0x1cu,0x18u,0x1cu);
  basis(0x10u,0x0cu,0x10u);
  basis(0x04u,0,0x04u);
  f.w32(out,scale);
  return out;
}

// 144CF0.
void orientation(Frame& f,u32 out,u32 q) {
  if(!(f.r8(epsilon_flag)&1u)) {
    f.w32(epsilon_flag,f.r32(epsilon_flag)|1u);
    f.w32(epsilon_value,epsilon_f);
  }
  const u32 i=f.r32(q),j=f.r32(q+4u),k=f.r32(q+8u),w=f.r32(q+0xcu);
  const u32 n=f.add(f.add(f.mul(i,i),f.mul(k,k)),f.add(f.mul(j,j),f.mul(w,w)));
  if(!f.greater(n,f.r32(epsilon_value))) {
    f.w64(out+4u,one,0);f.w64(out+0xcu,0,0);f.w64(out+0x14u,one,0);f.w64(out+0x1cu,0,0);f.w32(out+0x24u,one);
  } else {
    const u32 r=f.rcp(n);
    const u32 s=[&]{const u32 twice=f.sub(f.add(r,r),f.mul(f.mul(n,r),r));return f.add(twice,twice);}();
    const std::array qs{f.mul(i,s),f.mul(j,s),f.mul(k,s),f.mul(w,s)};
    std::array<u32,4> I,W,J;
    for(u32 n2=0;n2<4;++n2){I[n2]=f.mul(i,qs[n2]);W[n2]=f.mul(w,qs[n2]);J[n2]=f.mul(j,qs[n2]);}
    const u32 kk=f.mul(qs[2],f.r32(q+8u));
    const std::array X{f.add(I[1],W[2]^sign_bit),f.add(I[2],W[1]),f.add(I[1],W[2]),f.add(I[2],W[1]^sign_bit)};
    f.w64(out+8u,X[2],X[3]);
    f.w32(out+0x10u,X[0]);
    f.w64(out+0x18u,X[0],X[1]);
    const u32 a=f.sub(one,kk);
    f.w32(out+4u,f.sub(a,J[1]));
    f.w32(out+0x14u,f.sub(a,I[0]));
    f.w32(out+0x24u,f.sub(f.sub(one,I[0]),J[1]));
    f.w32(out+0x20u,f.sub(J[2],W[0]));
    f.w32(out+0x18u,f.add(J[2],W[0]));
  }
  f.w32(out,f.r32(q+0x1cu));
  const u32 tx=f.r32(q+0x10u),ty=f.r32(q+0x14u);
  f.w64(out+0x28u,tx,ty);
  f.w32(out+0x30u,f.r32(q+0x18u));
}

// BFC70: locals S/M' (13 words), T (translation) and R (rotation).
void root_matrix(Frame& f,u32 up,u32 fwd,bool parent_mirrored,u32 position,u32 scale,u32 mirror,u32 parent,u32 out) {
  Locals locals(f);
  const u32 S=locals(0x34),T=locals(0x34),R=locals(0x34);
  const u32 lx=f.sub(f.mul(f.r32(up+4u),f.r32(fwd+8u)),f.mul(f.r32(fwd+4u),f.r32(up+8u)));
  const u32 xz=f.mul(f.r32(up),f.r32(fwd+8u));
  const u32 xy=f.mul(f.r32(fwd),f.r32(up+4u));
  const u32 ly=f.sub(f.mul(f.r32(fwd),f.r32(up+8u)),xz);
  const u32 lz=f.sub(f.mul(f.r32(fwd+4u),f.r32(up)),xy);
  f.w32(T+0x28u,f.r32(position));f.w32(T+0x30u,f.r32(position+8u));f.w32(T+0x2cu,f.r32(position+4u));
  f.w32(R+4u,f.r32(fwd));f.w32(R+0xcu,f.r32(fwd+8u));
  const u32 fy=f.r32(fwd+4u),ux=f.r32(up),uy=f.r32(up+4u);
  f.w32(R+0x24u,f.r32(up+8u));
  for(const auto [at,v]:std::array<std::array<u32,2>,10>{{{0,one},{4,one},{8,0},{0xc,0},{0x10,0},{0x14,one},{0x18,0},
      {0x1c,0},{0x20,0},{0x24,one}}})f.w32(T+at,v);
  f.w32(R,one);f.w32(R+8u,fy);f.w32(R+0x10u,lx);f.w32(R+0x14u,ly);f.w32(R+0x18u,lz);
  f.w32(R+0x1cu,ux);f.w32(R+0x20u,uy);f.w32(R+0x28u,0);f.w32(R+0x2cu,0);f.w32(R+0x30u,0);
  if(mirror&0xffu){f.w32(R+0x14u,f.sub(zero_f,ly));f.w32(R+0x10u,f.sub(zero_f,lx));f.w32(R+0x18u,f.sub(zero_f,lz));}
  if(!f.equal(scale,one)) {
    const std::array<u32,13> s{scale,one,0,0,0,one,0,0,0,one,0,0,0};
    for(u32 n=0;n<13;++n)f.w32(S+4u*n,s[n]);
    product(f,R,S,R);
  }
  u32 a=T;
  if(parent) {
    a=parent;
    if(!f.equal(f.r32(parent),one) || parent_mirrored) {
      copy_words(f,S,parent,13);
      const u32 ps=f.r32(S);
      if(!f.equal(ps,one)) {
        for(const u32 at:{0x28u,0x2cu,0x30u})f.w32(T+at,f.mul(f.r32(T+at),ps));
        f.w32(S,one);
      }
      if(parent_mirrored)for(const u32 at:{0x10u,0x14u,0x18u})f.w32(S+at,f.sub(zero_f,f.r32(S+at)));
      a=S;
    }
    product(f,a,T,out);
    a=out;
  }
  product(f,a,R,out);
}

// 1707A0.
void default_orientations(Frame& f,u32 model,u32 orientations) {
  if(s32(f.r32(model+0x48u))<=0)return;
  for(u32 n=0;;) {
    const u32 record=f.r32(model+0x4cu)+n*0x60u,to=orientations+n*0x20u;
    for(u32 k=0;k<4;++k)f.w32(to+4u*k,f.r32(record+0x18u+4u*k));
    for(u32 k=0;k<3;++k)f.w32(to+0x10u+4u*k,f.r32(record+0xcu+4u*k));
    f.w32(to+0x1cu,one);
    ++n;
    if(s32(n)>=s32(f.r32(model+0x48u)))break;
  }
}

// BCDF0 (object_try_and_get_and_verify_type) with its datum stride and salt.
// Kept local: the existing native_tu15_object_body_bcdf0 is not part of the
// oracle harness link.
u32 object_verified_body(const M& m,u32 object,u32 mask) {
  if(object==none)return 0;
  const u32 pool=r32(m,object_pool),index=object&0xffffu;
  if(s32(index)>=s32(r32(m,pool+0x38u)))return 0;
  const u32 row=r32(m,pool+0x24u)*index+r32(m,pool+0x44u);
  const u16 salt=r16(m,row);
  if(!salt || sx16(salt)!=u32(s32(object)>>16))return 0;
  if(!(mask&(1u<<(r8(m,row+3u)&31u))))return 0;
  return r32(m,row+8u);
}

// 1154A0 / 10F440: `owner` is the attaching object handle.
void attached_parent_hook(Frame& f,u32 rows,u32 owner,u32 nodes) {
  if(owner==none || !object_verified_body(f.world(),owner,none))return;
  const u32 body=f.r32(rows+(owner&0xffffu)*12u+8u);
  product(f,sx16(std::uint16_t(f.r16(body+0x116u)))+body,nodes,nodes);
}
void unit_hook(Frame& f,u32 object,u32 nodes) {
  const u32 rows=f.r32(f.r32(object_pool)+0x44u);
  const u32 body=f.r32(rows+(object&0xffffu)*12u+8u);
  attached_parent_hook(f,rows,f.r32(sx16(std::uint16_t(f.r16(body+0x33eu)))+body+0x28u),nodes);
}
void scenery_hook(Frame& f,u32 object,u32 nodes) {
  const u32 rows=f.r32(f.r32(object_pool)+0x44u);
  const u32 body=f.r32(rows+(object&0xffffu)*12u+8u);
  attached_parent_hook(f,rows,f.r32(body+0x138u),nodes);
}
// 10E180.
void type_node_hooks(Frame& f,u32 object,u32 nodes) {
  const u32 body=object_body(f.world(),object);
  const u32 definition=f.r32(type_table+sx8(std::uint8_t(f.r8(body+0xaau)))*4u);
  for(u32 level=0;f.r32(definition+0x84u+level*4u);level=sx16(std::uint16_t(level+1u))) {
    const u32 hook=f.r32(f.r32(definition+0x84u+level*4u)+0x70u);
    if(!hook)continue;
    if(hook==0x1154a0u)unit_hook(f,object,nodes);
    else if(hook==0x10f440u)scenery_hook(f,object,nodes);
    else refuse(hex(hook).c_str(),"type node matrix hook");
  }
}

// 1F4920: 1 - (ticks since +5C) * seconds-per-tick * 6.6666665, clamped.
u32 recovery_ramp(Frame& f,u32 at) {
  const u32 start=f.r32(at+0x5cu);
  if(start==none)return 0;
  const u32 time=f.r32(0x51df54u);
  const u32 elapsed=bits(sse::cvtsi2ss(s32(f.r32(time+8u)-start),f.mxcsr()));
  const u32 x=f.sub(one,f.mul(f.mul(elapsed,f.r32(time+4u)),ramp_rate_f));
  if(f.greater(zero_f,x))return 0;
  if(f.greater(x,one))return one;
  return x;
}
// E3BC0.
u32 physics_height(Frame& f,u32 object) {
  const u32 body=object_body(f.world(),object);
  if(f.r8(body+0x34bu)!=1u || f.r8(body+0x3dcu)!=3u)return 0;
  return f.mul(recovery_ramp(f,body+0x3ecu),height_scale_f);
}

// 1E5AF0 (EAX animation index, EDX graph): resource-backed animation row.
void animation_resource(const M& m,u32 graph,u32 index) {
  const u32 animation=index==none?0:r32(m,graph+0x30u)+index*0x6cu;
  if(!(r8(m,animation+0x16u)&0x10u))return;
  const u32 row=r32(m,animation+0x24u)*20u+r32(m,graph+0xb0u);
  if(r8(m,row+0x10u))refuse("126200","animation resource admission");
}
// 1E3270: inherited graph `index` of `graph`, or 0.
u32 inherited_graph(const M& m,u32 graph,u32 index) {
  if(s32(index)>=s32(r32(m,graph+0x4cu)))return 0;
  const u32 tag=r32(m,r32(m,graph+0x50u)+index*32u+4u);
  return tag==none?0:tag_data(m,tag);
}
// 1E5C80: request the resources of an animation and its chain.
void animation_request(const M& m,u32 graph,u32 id) {
  if(!r8(m,0x51df20u) || !r8(m,0x51df21u))return;
  const u32 high=sx16(std::uint16_t(id>>16));
  if(high==none)return;
  u32 g=graph;
  if(std::uint16_t(id)!=0xffffu)g=inherited_graph(m,graph,sx16(std::uint16_t(id)));
  const auto row=[&](u32 index){return index==none?0u:r32(m,g+0x30u)+index*0x6cu;};
  u32 current=row(high);
  const u16 first=r16(m,current+0x30u);
  if(first!=0xffffu)current=row(sx16(first));
  animation_resource(m,g,high);
  while(r16(m,current+0x32u)!=0xffffu) {
    const u32 next=row(sx16(r16(m,current+0x32u)));
    if(r8(m,next+0x16u)&0x10u) {
      const u32 resource=r32(m,next+0x24u)*20u+r32(m,g+0xb0u);
      if(r8(m,resource+0x10u))refuse("126200","animation resource admission");
    }
    current=row(sx16(r16(m,current+0x32u)));
  }
}
// 1E3150.
u32 animation_lookup(const M& m,u32 graph,u32 id) {
  const u16 high=std::uint16_t(id>>16),low=std::uint16_t(id);
  if(high==0xffffu)return 0;
  u32 animation=0;
  if(low==0xffffu) {
    animation=r32(m,graph+0x30u)+sx16(high)*0x6cu;
  } else {
    u32 inherited=0;
    if(s32(sx16(low))<s32(r32(m,graph+0x4cu))) {
      const u32 tag=r32(m,r32(m,graph+0x50u)+sx16(low)*32u+4u);
      if(tag!=none)inherited=tag_data(m,tag);
    }
    animation=r32(m,inherited+0x30u)+sx16(high)*0x6cu;
  }
  if(animation)animation_request(m,graph,id);
  return animation;
}
u32 manager_animation(const M& m,u32 manager) {
  if(r16(m,manager+6u)==0xffffu)return 0;
  return animation_lookup(m,tag_data(m,r32(m,manager)),r32(m,manager+4u));
}
bool manager_valid(const M& m,u32 manager) {
  return r32(m,manager+0x68u)!=none && r32(m,manager)!=none && r16(m,manager+6u)!=0xffffu;
}


// ---- Animation codec per-track decoders (0x48C33C table entries) ----
// State block 0x510F30: +14 frame, +18 next frame, +1C fraction, +30 codec
// data, +64/+68/+6C rotation/translation/scale track, +70 output orientation.
constexpr u32 codec_state=0x510f30u;
using V4=std::array<u32,4>;
__m128 vector(const V4& v){__m128 r;std::memcpy(&r,v.data(),16);return r;}
V4 lanes(__m128 v){V4 r;std::memcpy(r.data(),&v,16);return r;}
#if HALO2_NATIVE_SCRIPT_X86_FP
#define HALO2_NODEBUILD_PACKED(name,op) \
V4 name(const V4& a,const V4& b,u32 mxcsr) { \
  __m128 x=vector(a);const __m128 y=vector(b);const auto mode=sse::sse_control(mxcsr);u32 saved{}; \
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\t" op " %3, %0\n\tldmxcsr %1":"+x"(x),"+m"(saved):"m"(mode),"x"(y)); \
  return lanes(x); \
}
HALO2_NODEBUILD_PACKED(mulps,"mulps")
HALO2_NODEBUILD_PACKED(addps,"addps")
HALO2_NODEBUILD_PACKED(subps,"subps")
HALO2_NODEBUILD_PACKED(minps,"minps")
HALO2_NODEBUILD_PACKED(maxps,"maxps")
#undef HALO2_NODEBUILD_PACKED
u32 sqrtss(u32 value,u32 mxcsr) {
  float x=f32(value);const auto mode=sse::sse_control(mxcsr);u32 saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\tsqrtss %0, %0\n\tldmxcsr %1":"+x"(x),"+m"(saved):"m"(mode));
  return bits(x);
}
V4 rsqrtps(const V4& a,u32 mxcsr) {
  const __m128 x=vector(a);__m128 r=x;const auto mode=sse::sse_control(mxcsr);u32 saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\trsqrtps %3, %0\n\tldmxcsr %1":"=&x"(r),"+m"(saved):"m"(mode),"x"(x));
  return lanes(r);
}
#else
[[noreturn]] V4 packed_refused(){refuse("296180","packed SSE codec on a non-x86 host");}
V4 mulps(const V4&,const V4&,u32){packed_refused();}
V4 addps(const V4&,const V4&,u32){packed_refused();}
V4 subps(const V4&,const V4&,u32){packed_refused();}
V4 minps(const V4&,const V4&,u32){packed_refused();}
V4 maxps(const V4&,const V4&,u32){packed_refused();}
u32 sqrtss(u32,u32){packed_refused();}
V4 rsqrtps(const V4&,u32){packed_refused();}
#endif
V4 broadcast(u32 v){return {v,v,v,v};}
// MOVQ + PUNPCKLWD/PUNPCKHWD + PSRAD 16 + CVTPI2PS: four int16 lanes.
V4 dequantized(Frame& f,u32 at) {
  const u32 low=f.r32(at),high=f.r32(at+4u);
  return {bits(float(std::int16_t(low))),bits(float(std::int16_t(low>>16))),
          bits(float(std::int16_t(high))),bits(float(std::int16_t(high>>16)))};
}
// MULPS q,q; ADDPS with SHUFPS 4E; ADDPS with SHUFPS 11; RSQRTPS; MULPS.
V4 normalized(Frame& f,const V4& q) {
  const V4 square=mulps(q,q,f.mxcsr());
  const V4 pairs=addps(square,{square[2],square[3],square[0],square[1]},f.mxcsr());
  const V4 total=addps(pairs,{pairs[1],pairs[0],pairs[1],pairs[0]},f.mxcsr());
  return mulps(q,rsqrtps(total,f.mxcsr()),f.mxcsr());
}
// A sparse track's key frame `n`: bytes, or words for the 2972xx/2974xx
// decoders.
u32 key_frame(Frame& f,u32 keys,u32 n,u32 width){return width==2u?f.r16(keys+n*2u):f.r8(keys+n);}
// Binary search of a sparse track's key frames (CMOVBE/CMOVA on the
// unsigned key versus the codec frame): the last key at or before it.
u32 sparse_key(Frame& f,u32 keys,u32 count,u32 width=1u) {
  u32 low=0,high=count;
  if(s32(count)>1)
    do {
      const u32 middle=u32(s32(high+low)>>1);
      if(key_frame(f,keys,middle,width)<=f.r32(codec_state+0x14u))low=middle;else high=middle;
    } while(s32(high)>s32(low+1u));
  return low;
}
// 296AD0 (byte keys) / 297270 (word keys): sparse quantized rotation keys.
// Between two keys the quaternions are weighted by the integer frame
// distances (next - frame, frame - key) and renormalized.
void sparse_rotation(Frame& f,u32 width) {
  const u32 data=f.r32(codec_state+0x30u);
  const u32 packed=f.r32(data+f.r32(codec_state+0x64u)*4u+0x30u);
  const u32 first=packed>>12,count=packed&0xfffu;
  const u32 values=f.r32(data+0x20u)+first*8u+data;
  const u32 keys=f.r32(data+0x14u)+first*width+data;
  const u32 frame=f.r32(codec_state+0x14u);
  const u32 key=sparse_key(f,keys,count,width);
  const u32 at=key_frame(f,keys,key,width);
  const u32 row=values+key*8u;
  if(frame==at) {
    f.w128(f.r32(codec_state+0x70u),normalized(f,dequantized(f,row)));
    return;
  }
  const u32 next=key_frame(f,keys,key+1u,width);
  const u32 out=f.r32(codec_state+0x70u);
  const u32 toward_next=f.from_int(next-frame),since=f.from_int(frame-at);
  const V4 a=dequantized(f,row),b=dequantized(f,row+8u);
  const V4 q=addps(mulps(a,broadcast(toward_next),f.mxcsr()),mulps(b,broadcast(since),f.mxcsr()),f.mxcsr());
  f.w128(out,normalized(f,q));
}
// 296700: forced sparse byte-key rotation: between two keys the
// quaternions are weighted by the float distances to frame + fraction
// (next - t, t - key) and renormalized; the last key is used alone.
void sparse_rotation_forced(Frame& f) {
  const u32 data=f.r32(codec_state+0x30u);
  const u32 packed=f.r32(data+f.r32(codec_state+0x64u)*4u+0x30u);
  const u32 first=packed>>12,count=packed&0xfffu;
  const u32 values=f.r32(data+0x20u)+first*8u+data;
  const u32 keys=f.r32(data+0x14u)+first+data;
  const u32 key=sparse_key(f,keys,count,1u);
  const u32 out=f.r32(codec_state+0x70u);
  const u32 row=values+key*8u;
  if(s32(key+1u)>=s32(count)){f.w128(out,normalized(f,dequantized(f,row)));return;}
  const u32 next=f.r8(keys+key+1u);
  const u32 at=f.add(f.from_int(f.r32(codec_state+0x14u)),f.r32(codec_state+0x1cu));
  const u32 toward_next=f.sub(f.from_int(next),at);
  const u32 since=f.sub(at,f.from_int(f.r8(keys+key)));
  const V4 a=dequantized(f,row),b=dequantized(f,row+8u);
  const V4 q=addps(mulps(a,broadcast(toward_next),f.mxcsr()),mulps(b,broadcast(since),f.mxcsr()),f.mxcsr());
  f.w128(out,normalized(f,q));
}
// 296890 (translation, 12-byte values) / 2969E0 (scale, 4-byte values):
// forced sparse byte keys. The key at or before the frame is copied; before
// the last key its next value is blended in by (frame + fraction - key) *
// RCPSS(next key - key) (every next translation lane read before the first
// blended store).
void sparse_forced(Frame& f,bool scale) {
  const u32 data=f.r32(codec_state+0x30u);
  const u32 packed=f.r32(f.r32(data+(scale?0x10u:0xcu))+f.r32(codec_state+(scale?0x6cu:0x68u))*4u+data);
  const u32 count=packed&0xfffu,first=packed>>12;
  const u32 keys=f.r32(data+(scale?0x1cu:0x18u))+first+data;
  const u32 width=scale?4u:12u;
  const u32 values=f.r32(data+(scale?0x28u:0x24u))+first*width+data;
  const u32 key=sparse_key(f,keys,count,1u);
  const u32 row=values+key*width;
  const u32 out=f.r32(codec_state+0x70u)+(scale?0x1cu:0x10u);
  for(u32 k=0;k<width/4u;++k)f.w32(out+4u*k,f.r32(row+4u*k));
  if(s32(key+1u)>=s32(count))return;
  const u32 at=f.r8(keys+key),next=f.r8(keys+key+1u);
  const u32 t=f.sub(f.add(f.from_int(f.r32(codec_state+0x14u)),f.r32(codec_state+0x1cu)),f.from_int(at));
  const u32 step=f.rcp(f.from_int(next-at));
  if(scale) {
    const u32 v=f.r32(row+4u),o=f.r32(out);
    f.w32(out,f.add(f.mul(f.sub(v,o),f.mul(step,t)),o));
    return;
  }
  const u32 weight=f.mul(step,t);
  const std::array<u32,3> v{f.r32(row+0xcu),f.r32(row+0x10u),f.r32(row+0x14u)};
  for(u32 k=0;k<3u;++k)f.w32(out+4u*k,f.add(f.mul(f.sub(v[k],f.r32(out+4u*k)),weight),f.r32(out+4u*k)));
}
void codec_track(Frame& f,u32 decoder) {
  const u32 data=f.r32(codec_state+0x30u);
  switch(decoder) {
  case 0x296700u:sparse_rotation_forced(f);return;
  case 0x296890u:sparse_forced(f,false);return;
  case 0x2969e0u:sparse_forced(f,true);return;
  case 0x296180u: {  // rotation, one quantized key
    const u32 at=data+f.r32(codec_state+0x64u)*8u+0x20u;
    const u32 out=f.r32(codec_state+0x70u);
    f.w128(out,normalized(f,dequantized(f,at)));
    return;
  }
  case 0x296680u: {  // rotation, quantized key at the frame
    const u32 track=f.r32(data+0x14u)*f.r32(codec_state+0x64u)+data;
    const u32 out=f.r32(codec_state+0x70u);
    f.w128(out,normalized(f,dequantized(f,track+f.r32(codec_state+0x14u)*8u+0x20u)));
    return;
  }
  case 0x296450u: {  // rotation, quantized keys blended at the fraction
    const u32 base=f.r32(data+0x14u)*f.r32(codec_state+0x64u)+data+0x20u;
    const u32 out=f.r32(codec_state+0x70u),t=f.r32(codec_state+0x1cu);
    const u32 next=base+f.r32(codec_state+0x18u)*8u,current=base+f.r32(codec_state+0x14u)*8u;
    const u32 keep=f.sub(one,t);
    const V4 a=dequantized(f,current),b=dequantized(f,next);
    const V4 q=addps(mulps(a,broadcast(keep),f.mxcsr()),mulps(b,broadcast(t),f.mxcsr()),f.mxcsr());
    f.w128(out,normalized(f,q));
    return;
  }
  case 0x296240u: {  // rotation, float keys blended at the fraction
    const u32 base=f.r32(data+0x14u)*f.r32(codec_state+0x64u)+data+0x20u;
    const u32 out=f.r32(codec_state+0x70u),t=f.r32(codec_state+0x1cu);
    const u32 next=f.r32(codec_state+0x18u)*16u+base,current=f.r32(codec_state+0x14u)*16u+base;
    V4 a,b;
    for(u32 k=0;k<4;++k){a[k]=f.r32(current+4u*k);b[k]=f.r32(next+4u*k);}
    const V4 q=addps(mulps(subps(b,a,f.mxcsr()),broadcast(t),f.mxcsr()),a,f.mxcsr());
    f.w128(out,normalized(f,q));
    return;
  }
  case 0x2963d0u: {  // rotation, float key at the frame (copied)
    const u32 at=(f.r32(codec_state+0x14u)+2u)*16u+f.r32(data+0x14u)*f.r32(codec_state+0x64u)+data;
    const u32 out=f.r32(codec_state+0x70u);
    for(u32 k=0;k<4;++k)f.w32(out+4u*k,f.r32(at+4u*k));
    return;
  }
  case 0x2961f0u: {  // translation, one key
    const u32 at=f.r32(codec_state+0x68u)*12u+f.r32(data+0xcu)+data;
    const u32 out=f.r32(codec_state+0x70u)+0x10u;
    for(u32 k=0;k<3;++k)f.w32(out+4u*k,f.r32(at+4u*k));
    return;
  }
  case 0x296410u: {  // translation, key at the frame
    const u32 at=f.r32(data+0x18u)*f.r32(codec_state+0x68u)+f.r32(codec_state+0x14u)*12u+f.r32(data+0xcu)+data;
    const u32 out=f.r32(codec_state+0x70u)+0x10u;
    for(u32 k=0;k<3;++k)f.w32(out+4u*k,f.r32(at+4u*k));
    return;
  }
  case 0x296540u:    // translation, float keys blended at the fraction
  case 0x2962f0u: {  // (format 8's forced translation: its own statics)
    // Both keys are staged in .bss statics first (the current key at +C, the
    // next one at +0), then each lane is (next - current) * t + current,
    // re-read from them after the previous lane's store.
    const u32 statics=decoder==0x296540u?0x56e688u:0x56e6a0u;
    const u32 track=f.r32(data+0x18u)*f.r32(codec_state+0x68u)+f.r32(data+0xcu)+data;
    const u32 t=f.r32(codec_state+0x1cu);
    const u32 current=track+f.r32(codec_state+0x14u)*12u;
    for(u32 k=0;k<3;++k)f.w32(statics+0xcu+4u*k,f.r32(current+4u*k));
    const u32 next=track+f.r32(codec_state+0x18u)*12u;
    f.w32(statics,f.r32(next));
    const u32 out=f.r32(codec_state+0x70u)+0x10u;
    for(u32 k=1;k<3;++k)f.w32(statics+4u*k,f.r32(next+4u*k));
    for(u32 k=0;k<3;++k) {
      const u32 a=f.r32(statics+0xcu+4u*k),b=f.r32(statics+4u*k);
      f.w32(out+4u*k,f.add(f.mul(f.sub(b,a),t),a));
    }
    return;
  }
  case 0x296620u: {  // scale, keys blended at the fraction
    const u32 track=f.r32(data+0x1cu)*f.r32(codec_state+0x6cu)+f.r32(data+0x10u)+data;
    const u32 a=f.r32(track+f.r32(codec_state+0x14u)*4u),b=f.r32(track+f.r32(codec_state+0x18u)*4u);
    const u32 out=f.r32(codec_state+0x70u);
    f.w32(out+0x1cu,f.add(f.mul(f.sub(b,a),f.r32(codec_state+0x1cu)),a));
    return;
  }
  case 0x296220u:  // scale, one key
    f.w32(f.r32(codec_state+0x70u)+0x1cu,f.r32(f.r32(data+0x10u)+f.r32(codec_state+0x6cu)*4u+data));
    return;
  case 0x2cec60u:  // scale, key at the frame
    f.w32(f.r32(codec_state+0x70u)+0x1cu,
        f.r32(f.r32(data+0x1cu)*f.r32(codec_state+0x6cu)+f.r32(codec_state+0x14u)*4u+f.r32(data+0x10u)+data));
    return;
  case 0x296ad0u:  // rotation, sparse byte keys
  case 0x297270u:  // rotation, sparse word keys
    sparse_rotation(f,decoder==0x297270u?2u:1u);
    return;
  case 0x296db0u:    // scale, sparse keys
  case 0x296c60u:    // translation, sparse keys
  case 0x297400u: {  // translation, sparse word keys
    const bool scale=decoder==0x296db0u;
    const u32 key_width=decoder==0x297400u?2u:1u;
    const u32 packed=f.r32(f.r32(data+(scale?0x10u:0xcu))+f.r32(codec_state+(scale?0x6cu:0x68u))*4u+data);
    const u32 count=packed&0xfffu,first=packed>>12;
    const u32 keys=f.r32(data+(scale?0x1cu:0x18u))+first*key_width+data;
    const u32 values=scale?f.r32(data+0x28u)+first*4u+data:f.r32(data+0x24u)+first*12u+data;
    const u32 key=sparse_key(f,keys,count,key_width);
    const u32 width=scale?4u:12u,row=values+key*width;
    const u32 out=f.r32(codec_state+0x70u)+(scale?0x1cu:0x10u);
    for(u32 k=0;k<width/4u;++k)f.w32(out+4u*k,f.r32(row+4u*k));
    if(s32(key+1u)>=s32(count))return;
    const u32 at=key_frame(f,keys,key,key_width),frame=f.r32(codec_state+0x14u);
    if(frame==at)return;
    const u32 since=f.sub(f.from_int(frame),f.from_int(at));
    const u32 step=f.rcp(f.from_int(key_frame(f,keys,key+1u,key_width)-at));
    if(scale) {
      const u32 next=f.r32(row+4u),value=f.r32(out);
      f.w32(out,f.add(f.mul(f.sub(next,value),f.mul(step,since)),value));
    } else {
      const u32 weight=f.mul(step,since);
      const u32 a=f.r32(row+0xcu),b=f.r32(row+0x10u),c=f.r32(row+0x14u);
      for(const auto [lane,v]:std::array<std::array<u32,2>,3>{{{0,a},{4,b},{8,c}}})
        f.w32(out+lane,f.add(f.mul(f.sub(v,f.r32(out+lane)),weight),f.r32(out+lane)));
    }
    return;
  }
  default:
    refuse(hex(decoder).c_str(),"animation codec track decoder");
  }
}

// ---- Codec frame decoders (283570 and the per-format loops) ----
// State block fields (offsets from 0x510F30): +00 kind, +04 mode, +08 force
// byte, +0C node count, +10 orientation count, +14/+18 frame and next frame,
// +1C fraction, +20 weight, +24 animation, +28 inherited entry, +2C node
// mask, +30 codec data, +34/+38/+3C track decoders, +40/+44/+48 flag
// streams, +4C orientations, +50 second-frame flag byte, +54..+60 second
// frame, +64/+68/+6C track counters, +70 decoder output. Kinds 2 and 3
// decode into the orientation at 0x510F10 and then layer it onto the
// output.
constexpr u32 codec_temp=0x510f10u;
u32 state(Frame& f,u32 offset){return f.r32(codec_state+offset);}
void set_state(Frame& f,u32 offset,u32 value){f.w32(codec_state+offset,value);}
void decode_track(Frame& f,u32 pass){codec_track(f,state(f,0x34u+4u*pass));}
void count_track(Frame& f,u32 pass){set_state(f,0x64u+4u*pass,state(f,0x64u+4u*pass)+1u);}
bool node_bit(Frame& f,u32 mask,u32 node){return (f.r32(mask+u32(s32(node)>>5)*4u)>>(node&31u))&1u;}
// One pass over a flag stream: a zero byte skips eight nodes, else each of
// its (up to count) nodes takes its bit, lowest first.
template<class Zero,class Node> void flag_pass(Frame& f,u32 stream,u32 count,Zero zero,Node node) {
  for(u32 n=0;s32(n)<s32(count);) {
    u32 flags=f.r8(stream++);
    if(!flags){zero();n+=8u;continue;}
    const u32 end=s32(n+8u)<=s32(count)?n+8u:count;
    for(;s32(n)<s32(end);++n,flags>>=1)node(n,(flags&1u)!=0);
  }
}
// The inherited entry's remap (+0C int16 per node) of `node`, masked by the
// entry's node mask (+14), the orientation count and (`masked`) the codec
// node mask at the remapped index.
bool remapped(Frame& f,u32 node,bool masked,u32& out,u32* index=nullptr) {
  const u32 entry=state(f,0x28u);
  if(!node_bit(f,f.r32(entry+0x14u),node))return false;
  const u32 remap=sx16(std::uint16_t(f.r16(f.r32(entry+0xcu)+node*2u)));
  if(s32(remap)<0 || s32(remap)>=s32(state(f,0x10u)))return false;
  if(masked && !node_bit(f,state(f,0x2cu),remap))return false;
  out=state(f,0x4cu)+(remap<<5);
  if(index)*index=remap;
  return true;
}
// SHUFPS with both sources given.
V4 shuffle(const V4& a,const V4& b,u32 imm){return {a[imm&3u],a[(imm>>2)&3u],b[(imm>>4)&3u],b[(imm>>6)&3u]};}
V4 sign_lane3(V4 v){v[3]^=sign_bit;return v;}  // XORPS with {0,0,0,0x80000000}
V4 load128(Frame& f,u32 at){return {f.r32(at),f.r32(at+4u),f.r32(at+8u),f.r32(at+0xcu)};}
// Quaternion product out = a * b (the shared SHUFPS sequence; XMM0 = a,
// XMM1 = b).
void quaternion_compose(Frame& f,u32 out,u32 at_a,u32 at_b) {
  const u32 c=f.mxcsr();
  const V4 a=load128(f,at_a),b=load128(f,at_b);
  V4 x0=shuffle(a,a,0x24u),x3=shuffle(a,a,0xffu),x7=shuffle(b,b,0x3fu);
  x3=mulps(x3,b,c);
  x7=mulps(x7,x0,c);
  x0=shuffle(x0,x0,0x49u);
  x7=sign_lane3(x7);
  V4 x6=shuffle(b,b,0x52u);
  x3=addps(x3,x7,c);
  x6=mulps(x6,x0,c);
  x0=shuffle(x0,x0,0x49u);
  x6=sign_lane3(x6);
  const V4 x1=shuffle(b,b,0x89u);
  x3=addps(x3,x6,c);
  x0=mulps(x0,x1,c);
  f.w128(out,subps(x3,x0,c));
}
// out = out * decoded.
void compose(Frame& f,u32 out){quaternion_compose(f,out,out,codec_temp);}
// Kind 3: scale the decoded rotation toward identity by the weight (+20);
// w moves toward copysign(1, w).
void weight_rotation(Frame& f) {
  const u32 w3=f.r32(codec_temp+0xcu),x=f.r32(codec_temp);
  const u32 weight=state(f,0x20u);
  f.w32(codec_temp,f.mul(x,weight));
  const u32 y=f.mul(f.r32(codec_temp+4u),weight);
  const u32 unit=one|(w3&sign_bit);
  f.w32(codec_temp+4u,y);
  const u32 z=f.mul(f.r32(codec_temp+8u),weight);
  const u32 w=f.add(f.mul(weight,f.sub(w3,unit)),unit);
  f.w32(codec_temp+8u,z);
  f.w32(codec_temp+0xcu,w);
}
// Kinds 2 and 3: layer the decoded track onto `out`. Kind 2 (`blend`
// false) adds translations and multiplies scales; the masked loop
// (28ADA0) takes decoded-first operands, the unmasked one (28B5F0)
// output-first. Kind 3 weights them first.
void layer(Frame& f,u32 pass,u32 out,bool masked,bool blend) {
  if(pass==0u) {
    if(blend)weight_rotation(f);
    compose(f,out);
    return;
  }
  const u32 weight=state(f,0x20u);
  if(pass==1u) {
    for(u32 k=0;k<3;++k) {
      const u32 d=codec_temp+0x10u+4u*k,o=out+0x10u+4u*k;
      if(blend)f.w32(o,f.add(f.mul(weight,f.r32(d)),f.r32(o)));
      else f.w32(o,masked?f.add(f.r32(d),f.r32(o)):f.add(f.r32(o),f.r32(d)));
    }
    return;
  }
  const u32 d=codec_temp+0x1cu,o=out+0x1cu;
  if(blend)f.w32(o,f.mul(f.add(f.mul(f.sub(f.r32(d),one),weight),one),f.r32(o)));
  else f.w32(o,masked?f.mul(f.r32(d),f.r32(o)):f.mul(f.r32(o),f.r32(d)));
}
// RSQRTSS under the live MXCSR (defined below).
u32 rsqrtss(u32 value,u32 mxcsr);
// The sign-aligned normalized lerp of the shared kind-1 / second-frame
// sequence: p = first * second, the sign of (p0+p2)+(p1+p3) flips second,
// r = (second - first) * w + first, then r * RSQRTSS((r0r0+r2r2)+(r1r1+r3r3))
// as one 16-byte store to `out`.
void nlerp(Frame& f,u32 out,u32 at_first,u32 at_second,u32 w) {
  const V4 q1=load128(f,at_first),q2=load128(f,at_second);
  V4 p;
  for(u32 k=0;k<4;++k)p[k]=f.mul(q1[k],q2[k]);
  const u32 sign=f.add(f.add(p[0],p[2]),f.add(p[1],p[3]))&sign_bit;
  V4 r,sq;
  for(u32 k=0;k<4;++k){r[k]=f.add(f.mul(f.sub(q2[k]^sign,q1[k]),w),q1[k]);sq[k]=f.mul(r[k],r[k]);}
  const u32 k=rsqrtss(f.add(f.add(sq[0],sq[2]),f.add(sq[1],sq[3])),f.mxcsr());
  f.w128(out,{f.mul(r[0],k),f.mul(r[1],k),f.mul(r[2],k),f.mul(r[3],k)});
}
// Kind 1 (284140's loops, partial weight): the output moves toward the
// decoded track by the weight (+20): rotations by nlerp (output first),
// translation and scale lanes by (decoded - out) * weight + out.
void layer_partial(Frame& f,u32 pass,u32 out) {
  const u32 weight=state(f,0x20u);
  if(pass==0u){nlerp(f,out,out,codec_temp,weight);return;}
  const u32 first=pass==1u?0x10u:0x1cu,lanes=pass==1u?3u:1u;
  for(u32 k=0;k<lanes;++k) {
    const u32 o=out+first+4u*k;
    f.w32(o,f.add(f.mul(f.sub(f.r32(codec_temp+first+4u*k),f.r32(o)),weight),f.r32(o)));
  }
}
void reset_tracks(Frame& f){set_state(f,0x64u,0);set_state(f,0x68u,0);set_state(f,0x6cu,0);}
// 2847D0 (masked) / 284D30: kind 0, mode 0. Tracks decode straight into
// the orientations; the output pointer advances per node.
// Kind 4 (293290/2928E0 mode 0, 2947B0/293CB0 mode 1, 295DA0/295280 mode
// 2) is the same loop over the event orientations 283E10 published.
void apply_event(Frame& f,u32 pass,u32 node,u32 out,bool decoded_first);
void frames_direct(Frame& f,bool masked,bool events=false) {
  const u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    set_state(f,0x70u,state(f,0x4cu));
    flag_pass(f,state(f,0x40u+4u*pass),count,[&]{set_state(f,0x70u,state(f,0x70u)+0x100u);},[&](u32 n,bool bit){
      if(bit) {
        if(!masked || node_bit(f,state(f,0x2cu),n)) {
          decode_track(f,pass);
          if(events)apply_event(f,pass,n,state(f,0x70u),masked);
        }
        count_track(f,pass);
      }
      set_state(f,0x70u,state(f,0x70u)+0x20u);
    });
  }
}
// 285A70 (mode 1), 2868A0/2861A0 (mode 2, `root_only`, masked): kind 0
// through the inherited entry's remap. Mode 2 decodes translation and
// scale for node 0 only.
void frames_remapped(Frame& f,bool masked,bool root_only,bool events=false) {
  u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    if(pass==2u)set_state(f,0x70u,state(f,0x4cu));
    flag_pass(f,state(f,0x40u+4u*pass),count,[]{},[&](u32 n,bool bit){
      if(!bit)return;
      u32 out=0,index=0;
      if(remapped(f,n,masked,out,&index)) {
        set_state(f,0x70u,out);
        decode_track(f,pass);
        if(events)apply_event(f,pass,index,state(f,0x70u),false);
      }
      count_track(f,pass);
    });
    if(pass==0u && root_only && s32(count)>1)count=1;
  }
}
// 28ADA0/28B5F0 (kind 2) and 28E730/28F110 (kind 3): mode 0, layered.
// 287130/287A20 (kind 1, `partial`): the same loop with layer_partial.
void frames_layered(Frame& f,bool masked,bool blend,bool partial=false) {
  const u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    u32 out=state(f,0x4cu);
    set_state(f,0x70u,codec_temp);
    flag_pass(f,state(f,0x40u+4u*pass),count,[&]{out+=0x100u;},[&](u32 n,bool bit){
      if(bit) {
        if(!masked || node_bit(f,state(f,0x2cu),n)) {
          decode_track(f,pass);
          if(partial)layer_partial(f,pass,out);
          else layer(f,pass,out,masked,blend);
        }
        count_track(f,pass);
      }
      out+=0x20u;
    });
  }
}
// 290710 (mode 1), 291E30/2912A0 (mode 2): kind 3 through the remap; and
// 28C8F0/28BF00 (kind 2, mode 1, `blend` false), whose translation and
// scale layering takes output-first operands masked or not. Node 0's
// decoded z translation is scaled by the entry's +18 first.
// Kind 1 (`partial`): 2883C0/288E70 (mode 1), 289920/28A3F0 (mode 2).
void frames_layered_remapped(Frame& f,bool masked,bool root_only,bool blend,bool partial=false) {
  u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    set_state(f,0x70u,codec_temp);
    flag_pass(f,state(f,0x40u+4u*pass),count,[]{},[&](u32 n,bool bit){
      if(!bit)return;
      u32 out=0;
      if(remapped(f,n,masked,out)) {
        decode_track(f,pass);
        if(pass==1u && !n)f.w32(codec_temp+0x18u,f.mul(f.r32(state(f,0x28u)+0x18u),f.r32(codec_temp+0x18u)));
        if(partial)layer_partial(f,pass,out);
        else layer(f,pass,out,false,blend);
      }
      count_track(f,pass);
    });
    if(pass==0u && root_only && s32(count)>1)count=1;
  }
}
// RSQRTSS under the live MXCSR: the host instruction's approximation.
u32 rsqrtss(u32 value,u32 mxcsr) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::sse_control(mxcsr);u32 saved{};float result=0.f;const float x=f32(value);
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\trsqrtss %3, %0\n\tldmxcsr %1":"+x"(result),"+m"(saved):"m"(mode),"x"(x));
  return bits(result);
#else
  (void)value;(void)mxcsr;refuse("28B080","RSQRTSS on a non-x86 host");
#endif
}
// The second-frame orientation 0x510EF0 blended into the decoded one
// (0x510F10) by +60. Rotations: the second is sign-aligned to the first
// (dot product lanes (0+2)+(1+3)), interpolated, then renormalized with
// RSQRTSS; translation and scale interpolate.
constexpr u32 codec_second=0x510ef0u;
// Kind 1's loops (`in_order`) store the translation lanes x, y, z; kinds
// 2 and 3 store y, x, z.
void blend_second(Frame& f,u32 pass,bool in_order=false) {
  if(pass==0u){nlerp(f,codec_temp,codec_temp,codec_second,state(f,0x60u));return;}
  if(pass==1u && in_order) {
    const u32 w=state(f,0x60u);
    for(u32 k=0;k<3;++k) {
      const u32 at=codec_temp+0x10u+4u*k,x1=f.r32(at);
      f.w32(at,f.add(f.mul(f.sub(f.r32(codec_second+0x10u+4u*k),x1),w),x1));
    }
    return;
  }
  if(pass==1u) {
    const u32 x1=f.r32(codec_temp+0x10u),w=state(f,0x60u),x2=f.r32(codec_second+0x10u),y2=f.r32(codec_second+0x14u);
    const u32 x=f.add(f.mul(f.sub(x2,x1),w),x1);
    const u32 y1=f.r32(codec_temp+0x14u);
    const u32 y=f.add(f.mul(f.sub(y2,y1),w),y1);
    const u32 z1=f.r32(codec_temp+0x18u);
    f.w32(codec_temp+0x14u,y);
    const u32 z2=f.r32(codec_second+0x18u);
    f.w32(codec_temp+0x10u,x);
    f.w32(codec_temp+0x18u,f.add(f.mul(f.sub(z2,z1),w),z1));
    return;
  }
  const u32 x1=f.r32(codec_temp+0x1cu),x2=f.r32(codec_second+0x1cu);
  f.w32(codec_temp+0x1cu,f.add(f.mul(f.sub(x2,x1),state(f,0x60u)),x1));
}
// 28B080 (kind 2) / 28EAF0 (kind 3): mode 0, unmasked, second frame. Each
// flagged node decodes at the frame (into 0x510F10) and at the second frame
// (+54/+58/+5C into 0x510EF0), blends the two and layers the result like
// 28B5F0/28E730, then restores the frame fields and the output.
// 287480 (kind 1, `partial`): layer_partial, the translation blended in
// lane order, and next frame before frame restored in every pass.
void frames_layered_second(Frame& f,bool blend,bool partial=false) {
  const u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    u32 out=state(f,0x4cu);
    set_state(f,0x70u,codec_temp);
    flag_pass(f,state(f,0x40u+4u*pass),count,[&]{out+=0x100u;},[&](u32,bool bit){
      if(bit) {
        decode_track(f,pass);
        const u32 fraction=state(f,0x1cu),frame=state(f,0x14u),next=state(f,0x18u);
        set_state(f,0x14u,state(f,0x54u));set_state(f,0x18u,state(f,0x58u));set_state(f,0x1cu,state(f,0x5cu));
        set_state(f,0x70u,codec_second);
        decode_track(f,pass);
        const u32 counter=0x64u+4u*pass;
        u32 track=pass?state(f,counter):0u;
        blend_second(f,pass,partial);
        if(partial)layer_partial(f,pass,out);
        else layer(f,pass,out,false,blend);
        if(!pass)track=state(f,counter);
        if(!pass || partial){set_state(f,0x18u,next);set_state(f,0x14u,frame);}
        else{set_state(f,0x14u,frame);set_state(f,0x18u,next);}
        set_state(f,0x1cu,fraction);
        set_state(f,0x70u,codec_temp);
        set_state(f,counter,track+1u);
      }
      out+=0x20u;
    });
  }
}
// 28A7D0 (kind 2) / 28E0B0 (kind 3): mode 0, masked, second frame. A
// flagged node inside the node mask (+2C) decodes at the frame and the
// second frame, blends and layers them (kind 2 with decoded-first
// operands) and restores frame, next frame, fraction and output in that
// order; every flagged node then counts its track.
// 286B30 (kind 1, `partial`): layer_partial and the lane-order translation.
void frames_layered_second_masked(Frame& f,bool blend,bool partial=false) {
  const u32 count=state(f,0x0cu);
  reset_tracks(f);
  for(u32 pass=0;pass<3u;++pass) {
    u32 out=state(f,0x4cu);
    set_state(f,0x70u,codec_temp);
    flag_pass(f,state(f,0x40u+4u*pass),count,[&]{out+=0x100u;},[&](u32 n,bool bit){
      if(bit) {
        if(node_bit(f,state(f,0x2cu),n)) {
          decode_track(f,pass);
          const u32 fraction=state(f,0x1cu),frame=state(f,0x14u),next=state(f,0x18u);
          set_state(f,0x14u,state(f,0x54u));set_state(f,0x18u,state(f,0x58u));set_state(f,0x1cu,state(f,0x5cu));
          set_state(f,0x70u,codec_second);
          decode_track(f,pass);
          blend_second(f,pass,partial);
          if(partial)layer_partial(f,pass,out);
          else layer(f,pass,out,true,blend);
          set_state(f,0x14u,frame);set_state(f,0x18u,next);set_state(f,0x1cu,fraction);
          set_state(f,0x70u,codec_temp);
        }
        count_track(f,pass);
      }
      out+=0x20u;
    });
  }
}
// 283570 with the selectors 2840D0 (kind 0), 2841B0 (kind 2), 284220
// (kind 3), 284300/284390 (mode 2). Of the second-frame loops (+50 flag),
// 28B080/28EAF0 and their masked forms 28A7D0/28E0B0 are native; the others
// are refused by name.
void codec_frames(Frame& f) {
  const u32 kind=state(f,0);
  if(kind>4u)return;
  const u32 mode=state(f,4u);
  if(mode>2u)return;
  const bool masked=state(f,0x2cu)!=0,flag=f.r8(codec_state+0x50u)!=0;
  if(kind==1u) {  // 284140, 284330 (mode 2)
    static constexpr const char* second[3][2]{{nullptr,nullptr},{"2887F0","287D00"},{"289D60","289240"}};
    if(flag && mode!=0u)refuse(second[mode][masked],"animation codec kind 1 second-frame loop");
    if(flag) {
      if(masked)frames_layered_second_masked(f,false,true);
      else frames_layered_second(f,false,true);
    } else if(mode==0u)frames_layered(f,masked,false,true);
    else frames_layered_remapped(f,masked,mode==2u,false,true);
    return;
  }
  if(kind==4u) {  // 284290 (mode 0), 2842C3 (mode 1), 2843C0 (mode 2)
    static constexpr const char* second[3][2]{{"292C90","292280"},{"2940F0","2935C0"},{"2956D0","294B80"}};
    if(flag)refuse(second[mode][masked],"animation codec event second-frame loop");
    if(mode==0u)frames_direct(f,masked,true);
    else frames_remapped(f,masked,mode==2u,true);
    return;
  }
  static constexpr const char* flagged[3][3][2]{
      {{"2849F0","2843F0"},{"285630","284EE0"},{"286460","285CF0"}},
      {{"28B080","28A7D0"},{"28C2C0","28B870"},{"284360","284360"}},
      {{"28EAF0","28E0B0"},{"290030","28F460"},{"291740","290B50"}}};
  static constexpr const char* unported[3][3][2]{
      {{nullptr,nullptr},{nullptr,"285370"},{nullptr,nullptr}},
      {{nullptr,nullptr},{nullptr,nullptr},{"284360","284360"}},
      {{nullptr,nullptr},{nullptr,"28FBA0"},{nullptr,nullptr}}};
  const u32 k=kind==0u?0u:kind-1u;
  if(kind==2u && mode==2u)refuse("284360","animation codec kind 2 mode 2");
  if(flag && kind!=0u && mode==0u){
    if(masked)frames_layered_second_masked(f,kind==3u);
    else frames_layered_second(f,kind==3u);
    return;
  }
  if(flag)refuse(flagged[k][mode][masked],"animation codec second-frame loop");
  if(const char* owner=unported[k][mode][masked])refuse(owner,"animation codec frame loop");
  if(kind==0u) {
    if(mode==0u)frames_direct(f,masked);
    else frames_remapped(f,masked,mode==2u);
  } else if(mode==0u) {
    frames_layered(f,masked,kind==3u);
  } else {
    frames_layered_remapped(f,masked,mode==2u,kind==3u);
  }
}
// CVTTSS2SI: truncation, 0x80000000 for NaN and out of range.
u32 truncated(u32 x) {
  const double d=f32(x);
  if(!(d>-2147483649.0 && d<2147483648.0))return 0x80000000u;
  return u32(std::int32_t(d));
}
u32 clamp_index(u32 x,u32 last){return s32(x)<0?0u:(s32(x)>s32(last)?last:x);}
// [0x510F48]-style clamped store: 0 below, else `last` and then the index
// itself when it does not exceed it.
void store_clamped(Frame& f,u32 offset,u32 index,u32 last) {
  if(s32(index)<0){set_state(f,offset,0);return;}
  set_state(f,offset,last);
  if(s32(index)<=s32(last))set_state(f,offset,index);
}
// 120740 (ECX out, EDX in, distinct): the inverse orientation. Conjugate
// rotation; translation -(R^-1 t) / scale; scale 1 / scale, 10000 for a
// zero scale and 1 kept exactly.
constexpr u32 minus_one_f=0xbf800000u;      // [0x46A0DC] -1.0f
constexpr u32 zero_scale_inverse_f=0x461c4000u; // [0x46A348] 10000.0f
void orientation_inverse(Frame& f,u32 out,u32 in) {
  for(u32 k=0;k<4u;++k)f.w32(out+4u*k,f.r32(in+4u*k));
  u32 x2=f.mul(f.r32(out+0xcu),minus_one_f);
  f.w32(out+0xcu,x2);
  u32 x1=f.mul(f.r32(in+0x10u),f.r32(out));
  u32 x4=f.mul(f.r32(in+0x14u),f.r32(out+4u));
  u32 x6=f.mul(f.r32(in+0x10u),f.r32(out+4u));
  u32 x7=f.mul(f.r32(in+0x14u),f.r32(out+8u));
  x1=f.add(x1,x4);
  x4=f.mul(f.r32(out+8u),f.r32(in+0x18u));
  x1=f.add(x1,x4);
  x4=f.mul(f.r32(out),f.r32(in+0x18u));
  x1=f.mul(x1,two_f);
  u32 x0=f.mul(f.mul(x2,x2),two_f);
  x2=f.mul(x2,two_f);
  u32 x3=f.sub(f.mul(f.r32(out+8u),f.r32(in+0x10u)),x4);
  x4=f.sub(f.mul(f.r32(in+0x14u),f.r32(out)),x6);
  x6=f.sub(f.mul(f.r32(out+4u),f.r32(in+0x18u)),x7);
  x7=f.r32(in+0x10u);
  x6=f.mul(x6,x2);
  x0=f.sub(x0,one);
  x7=f.mul(x7,x0);
  x6=f.add(x6,x7);
  x6=f.add(x6,f.mul(f.r32(out),x1));
  f.w32(out+0x10u,x6);
  x6=f.mul(f.r32(in+0x14u),x0);
  x3=f.mul(x3,x2);
  x6=f.add(x6,f.mul(f.r32(out+4u),x1));
  x6=f.add(x6,x3);
  f.w32(out+0x14u,x6);
  x0=f.mul(x0,f.r32(in+0x18u));
  x3=f.mul(f.r32(out+8u),x1);
  x4=f.mul(x4,x2);
  x3=f.add(f.add(x3,x4),x0);
  f.w32(out+0x18u,x3);
  u32 inverse=one;
  if(f.equal(zero_f,f.r32(in+0x1cu)))inverse=zero_scale_inverse_f;
  else if(!f.equal(one,f.r32(in+0x1cu)))inverse=f.div(one,f.r32(in+0x1cu));
  f.w32(out+0x10u,f.mul(inverse,f.r32(out+0x10u)));
  f.w32(out+0x14u,f.mul(f.r32(out+0x14u),inverse));
  f.w32(out+0x18u,f.mul(f.r32(out+0x18u),inverse));
  f.w32(out+0x10u,f.mul(f.r32(out+0x10u),minus_one_f));
  f.w32(out+0x14u,f.mul(f.r32(out+0x14u),minus_one_f));
  const u32 z=f.mul(f.r32(out+0x18u),minus_one_f);
  f.w32(out+0x1cu,inverse);
  f.w32(out+0x18u,z);
}
// 283E10 (ARGS graph, animation, entry, count, orientations; RET 14) for
// kind 0. Each event row (animation +64 count, +68 rows of 0x1C: +0 node,
// +2 plane flags, +4 packed quaternion, +C translation, +18 scale) whose
// node (through the entry's mask and remap) is below `count` composes the
// node's ancestry (graph +10 rows of 0x20: +4 sibling, +6 child, +8
// parent; 120570 of track deviceanim) from the orientations, inverts it
// (120740), applies the row and publishes the result to each child of the
// node below `count`: its row at 0x50EF10 and its bit in the rotation,
// translation and scale planes (0x56E000/20/40, set or cleared by the row's
// flag bits 0..2). True when a child was published (the loops become kind
// 4).
constexpr u32 event_orientations=0x50ef10u,event_planes=0x56e000u;
bool codec_previous(Frame& f,u32 graph,u32 animation,u32 entry,u32 count,u32 orientations) {
  if(s32(f.r32(animation+0x64u))<=0)return false;
  bool published=false;
  FrameMemory frame(f);
  const NativeObjectFactoryFp fp{f.x87(),f.mxcsr()};
  Locals locals(f);
  const u32 event=locals(0x20u),chain=locals(0x20u),inverse=locals(0x20u),result=locals(0x20u);
  for(u32 n=0,offset=0;;) {
    const u32 row=f.r32(animation+0x68u)+offset;
    u32 node=sx16(u16(f.r16(row)));
    bool admitted=true;
    if(entry) {
      if(!node_bit(f,f.r32(entry+0x14u),node))admitted=false;
      else node=sx16(u16(f.r16(f.r32(entry+0xcu)+node*2u)));
    }
    if(admitted && s32(node)>=0 && s32(node)<s32(count)) {
      const u32 record=f.r32(graph+0x10u)+node*32u;
      const u32 first_parent=sx16(u16(f.r16(record+8u)));
      copy_words(f,chain,orientations+node*32u,8u);
      for(u32 parent=first_parent;parent!=none;parent=sx16(u16(f.r16(f.r32(graph+0x10u)+parent*32u+8u))))
        device_animation::orientation_compose(frame,chain,orientations+parent*32u,chain,fp);
      orientation_inverse(f,inverse,chain);
      f.w128(event,normalized(f,dequantized(f,row+4u)));
      for(u32 k=0;k<3u;++k)f.w32(event+0x10u+4u*k,f.r32(row+0xcu+4u*k));
      f.w32(event+0x1cu,f.r32(row+0x18u));
      device_animation::orientation_compose(frame,result,inverse,event,fp);
      for(u32 child=sx16(u16(f.r16(record+6u)));s32(child)>=0;
          child=sx16(u16(f.r16(f.r32(graph+0x10u)+child*32u+4u)))) {
        if(s32(child)>=s32(count))continue;
        copy_words(f,event_orientations+child*32u,result,8u);
        const u32 bit=1u<<(child&31u),word=u32(s32(child)>>5)*4u;
        for(u32 plane=0;plane<3u;++plane) {
          const u32 at=event_planes+0x20u*plane+word;
          const bool set=(f.r8(row+2u)>>plane)&1u;
          f.w32(at,set?(f.r32(at)|bit):(f.r32(at)&~bit));
        }
        published=true;
      }
    }
    ++n;offset+=0x1cu;
    if(s32(n)>=s32(f.r32(animation+0x64u)))return published;
  }
}
// Kind 4's step after a decoded node whose plane bit is set: rotation event
// * decoded; translation: the event's x, y and z are added in turn into
// the decoded +10 (three stores to +10, the original's); scale event *
// decoded (decoded * event in 2928E0, mode 0 masked).
void apply_event(Frame& f,u32 pass,u32 node,u32 out,bool decoded_first) {
  if(!node_bit(f,event_planes+0x20u*pass,node))return;
  const u32 event=event_orientations+node*32u;
  if(pass==0u){quaternion_compose(f,out,event,out);return;}
  if(pass==1u) {
    u32 x=f.add(f.r32(event+0x10u),f.r32(out+0x10u));
    f.w32(out+0x10u,x);
    x=f.add(f.r32(event+0x14u),x);
    f.w32(out+0x10u,x);
    x=f.add(f.r32(event+0x18u),x);
    f.w32(out+0x10u,x);
    return;
  }
  f.w32(out+0x1cu,decoded_first?f.mul(f.r32(out+0x1cu),f.r32(event+0x1cu)):f.mul(f.r32(event+0x1cu),f.r32(out+0x1cu)));
}
// The per-format decoder table at 0x48C33C (40-byte formats).
void select_decoders(Frame& f,u32 data,u32 forced,bool translation_first) {
  const u32 table=0x48c33cu+f.r8(data)*40u+forced*12u;
  const u32 rotation=f.r32(table),translation=f.r32(table+4u),scale=f.r32(table+8u);
  if(translation_first){set_state(f,0x38u,translation);set_state(f,0x34u,rotation);set_state(f,0x3cu,scale);}
  else{set_state(f,0x34u,rotation);set_state(f,0x3cu,scale);set_state(f,0x38u,translation);}
}
// 2835B0. `block` is 1E5DF0's record: +0 data, +4 header, +8 node count.
void codec_setup(Frame& f,u32 block,u32 frame,u32 weight,u32 graph,u32 animation,u32 count,u32 entry,u32 mask,
    u32 orientations,u32 force,u32 flag,u32 second,u32 second_weight) {
  if(!f.greater(weight,epsilon_f))return;
  u32 index=truncated(frame);
  const u32 last=sx16(std::uint16_t(f.r16(animation+0x14u)))-1u;
  for(u32 at=0x56e000u;at<0x56e058u;at+=4u)f.w32(at,0);
  set_state(f,0x0cu,count);set_state(f,0x10u,count);
  f.w32(0x56e058u,0);f.w32(0x56e05cu,0);
  index=clamp_index(index,last);
  set_state(f,0x28u,entry);set_state(f,0x2cu,mask);
  set_state(f,0x34u,0);set_state(f,0x38u,0);set_state(f,0x3cu,0);
  set_state(f,0x20u,one);
  set_state(f,0x14u,index);set_state(f,0x18u,index);set_state(f,0x1cu,zero_f);
  set_state(f,0x24u,animation);set_state(f,0x30u,0);
  set_state(f,0x40u,0);set_state(f,0x44u,0);set_state(f,0x48u,0);
  set_state(f,0x4cu,orientations);
  f.w8(codec_state+0x50u,flag);
  if(flag&0xffu) {
    const u32 raw=truncated(second);
    const u32 second_index=clamp_index(raw,last);
    set_state(f,0x54u,second_index);
    store_clamped(f,0x58u,raw+1u,last);
    const u32 second_fraction=f.sub(second,f.from_int(second_index));
    set_state(f,0x5cu,second_fraction);
    const u32 current=state(f,0x14u);
    store_clamped(f,0x18u,current+1u,last);
    animation=state(f,0x24u);
    set_state(f,0x1cu,f.sub(frame,f.from_int(current)));
    set_state(f,0x60u,second_weight);
  } else if(force&0xffu) {
    // Forced interpolation keeps `force` (the +12 decoder set) unless the
    // frame is the last or the fraction snaps to a key.
    bool interpolating=false;
    if(index!=last) {
      const u32 next=index+1u;
      store_clamped(f,0x18u,next,last);
      const u32 fraction=f.sub(frame,f.from_int(index));
      set_state(f,0x1cu,fraction);
      if(f.greater(epsilon_f,fraction))set_state(f,0x1cu,zero_f);
      else if(f.greater(fraction,blend_limit_f)){set_state(f,0x14u,next);set_state(f,0x1cu,zero_f);}
      else interpolating=true;
    }
    if(!interpolating)force=0;
  }
  const u32 entry_now=state(f,0x28u);
  set_state(f,4u,0);
  if(entry_now) {
    set_state(f,4u,(f.r8(entry+0x1cu)&1u)?2u:1u);
    set_state(f,0x0cu,f.r8(animation+0x13u));
  } else {
    const u32 nodes=f.r8(animation+0x13u),total=state(f,0x10u);
    set_state(f,0x0cu,nodes);
    if(s32(total)<=s32(nodes))set_state(f,0x0cu,total);
  }
  const bool full=f.greater(weight,blend_limit_f);
  set_state(f,0,0);
  if(f.r8(animation+0x10u)==1u){set_state(f,0,full?2u:3u);set_state(f,0x20u,weight);}
  else if(!full){set_state(f,0,1u);set_state(f,0x20u,weight);}
  const auto streams=[&](u32 base){
    const u32 bytes=((f.r8(block+8u)+0x1fu)>>3)&~3u;
    set_state(f,0x40u,base);set_state(f,0x44u,base+bytes);set_state(f,0x48u,base+bytes*2u);
  };
  if(!f.r8(animation+0x10u) && f.r16(f.r32(block+4u)+6u)) {
    const u32 data=f.r32(block);
    set_state(f,0x30u,data);
    select_decoders(f,data,0,true);
    const u32 header=f.r32(block+4u);
    streams(sx16(std::uint16_t(f.r16(header+6u)))+f.r32(header+0xcu)+f.r32(block));
    f.w8(codec_state+8u,0);
    codec_frames(f);
  }
  if(!f.r32(f.r32(block+4u)+0xcu))return;
  if(!state(f,0) && codec_previous(f,graph,animation,entry,count,orientations))set_state(f,0,4u);
  const u32 header=f.r32(block+4u);
  const u32 data=sx16(std::uint16_t(f.r16(header+6u)))+f.r32(block);
  set_state(f,0x30u,data);
  select_decoders(f,data,(force&0xffu)!=0,false);
  streams(sx16(std::uint16_t(f.r16(header+6u)))+f.r32(header+0xcu)+sx8(std::uint8_t(f.r8(header)))+f.r32(block));
  f.w8(codec_state+8u,force);
  codec_frames(f);
}

// ---- Animation resources (1E5DF0 -> 1E5A70 -> 1262F0) ----
constexpr u32 resource_requests=0x4f041cu,resource_entries=0x4f0418u,resource_base=0x4f0420u,
    resource_cache=0x4f0424u;
// 126190: a pending request datum (+8 row) for an uncached row, else NONE.
u32 pending_request(Frame& f,u32 row) {
  if(f.r32(row+0xcu)!=none)return none;
  const u32 requests=f.r32(resource_requests);
  for(u32 n=0;s32(n)>=0 && s32(n)<s32(f.r32(requests+0x38u));++n) {
    if(!node_bit(f,f.r32(requests+0x48u),n))continue;
    const u32 datum=f.r32(requests+0x24u)*n+f.r32(requests+0x44u);
    if(f.r32(datum+8u)==row)return sx16(std::uint16_t(f.r16(datum)))<<16|n;
  }
  return none;
}
// 1262B0: a cached entry finishes its load (+4 loaded, +8 request).
void resource_touch(Frame& f,u32 entry) {
  if(f.r32(entry+8u)==none)return;
  if(!f.r8(entry+4u))refuse("1236A0","animation resource load wait");
  f.w32(entry+8u,none);
}
// ---- Animation resource cache miss (125FB0, 13FE50, 16E770, 1260C0) ----
thread_local const AnimationResourceReader* bound_reader=nullptr;
// 1263C0 (busy, cache +24) and 126400 (delete, cache +20): the page
// allocator's callbacks for an animation cache page `datum` (ESP +4).
void cache_callback(M& m,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsContactRuntimeAbi& a) {
  const u32 sp=a.gp[4],datum=r32(m,sp+4u);
  const u32 entries=r32(m,resource_entries);
  const u32 entry=(datum&0xffffu)*16u+r32(m,entries+0x44u);
  if(call.target==0x1263c0u) {
    const bool busy=r32(m,entry+8u)!=none && !r8(m,entry+4u);
    a.gp[0]=(a.gp[0]&0xffffff00u)|(busy?1u:0u);
  } else if(call.target==0x126400u) {
    if(r32(m,entry+8u)!=none) {
      if(!r8(m,entry+4u))refuse("1236A0","animation resource load wait (cache eviction)");
      w32(m,entry+8u,none);
    }
    w32(m,r32(m,entry+0xcu)+0xcu,none);
    native_tu15_data_array_delete(m,r32(m,resource_entries),datum);
  } else refuse(hex(call.target).c_str(),"animation cache page callback");
  a.gp[4]=sp+8u;  // RET 4
}
// 13FE50 (cache, bytes, priority) through the native page allocator over a
// private stack in the Frame; returns the page datum or NONE.
u32 cache_pages(Frame& f,u32 cache,u32 bytes,u32 priority) {
  Locals locals(f);
  const u32 stack=locals(0x1400u),top=(stack+0x13e0u)&~15u;
  FrameMemory fm(f);
  for(const auto [at,value]:std::array<std::array<u32,2>,4>{{{top,0x126035u},{top+4u,cache},{top+8u,bytes},{top+12u,priority}}})
    f.w32(at,value);
  SourcePhysicsContactRuntimeAbi abi{};
  abi.gp={0,0,0,priority,top,0,0,0};
  abi.eflags=0x202u;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{f.x87(),f.mxcsr()};
  SourcePhysicsContactRuntimeServices services;
  services.required_owner=[](SourcePhysicsContactMemory& m,const SourcePhysicsContactRuntimeCall& call,
      SourcePhysicsContactRuntimeAbi& state,const SourcePhysicsContactRecordObserver&){cache_callback(m,call,state);};
  native_tu15_page_allocate_abi(fm,abi,services,{});
  if(abi.gp[4]!=top+16u)refuse("13FE50","an unbalanced native page allocation");
  return abi.gp[0];
}
// 125FB0 (row, retry): a cache block for the row's +4 bytes; a failed
// allocation retries three times without the busy callback, each after
// aging the cache (+38) and resetting its eight +44 budgets.
u32 cache_allocate(Frame& f,u32 row) {
  u32 handle=cache_pages(f,f.r32(resource_cache),f.r32(row+4u),0);
  if(handle!=none)return handle;
  f.w32(f.r32(resource_cache)+0x20u,0x126400u);f.w32(f.r32(resource_cache)+0x24u,0);
  for(u32 attempt=3;s32(attempt)>0;--attempt) {
    const u32 cache=f.r32(resource_cache);
    const u32 age=f.r32(cache+0x38u);
    if(age==0x7fffffffu)refuse("13FD10","animation cache age rollover");
    f.w32(cache+0x38u,age+1u);
    for(u32 k=0;k<8;++k)f.w32(cache+0x44u+4u*k,0x7fffffffu);
    handle=cache_pages(f,cache,f.r32(row+4u),0);
    if(handle!=none)break;
  }
  const u32 cache=f.r32(resource_cache);
  f.w32(cache+0x20u,0x126400u);f.w32(cache+0x24u,0x1263c0u);
  return handle;
}
// 16E770 (EAX handle, EDX array): claim the record at the handle's index
// (NONE when out of range or in use): bitmap bit, count, high-water mark,
// the 16E860 clear (with the array's next salt, then advanced), then the
// handle's own salt.
u32 datum_new_at(Frame& f,u32 array,u32 handle) {
  const u32 index=handle&0xffffu;
  if(s32(index)>=s32(f.r32(array+0x20u)))return none;
  const u32 record=f.r32(array+0x24u)*index+f.r32(array+0x44u);
  if(f.r16(record))return none;
  const u32 bits=f.r32(array+0x48u)+(index>>5)*4u;
  f.w32(bits,f.r32(bits)|(1u<<(index&31u)));
  f.w32(array+0x3cu,f.r32(array+0x3cu)+1u);
  if(s32(index)>=s32(f.r32(array+0x38u)))f.w32(array+0x38u,index+1u);
  zero_fill(f.world(),record,f.r32(array+0x24u));
  f.w16(record,f.r16(array+0x40u));
  f.w16(array+0x40u,f.r16(array+0x40u)+1u);
  if(f.r16(array+0x40u)==0xffffu)f.w16(array+0x40u,0x8000u);
  f.w16(record,handle>>16);
  return (sx16(std::uint16_t(handle>>16))<<16)|index;
}
// 1260C0 (EAX entry; name, mode): read the row's resource into its block
// (destination [0x4F0420] + (page start << shift), +4 bytes rounded to 512)
// through the bound reader (1A3B70 at 0x126139), store the job; mode 0
// waits (1262B0); then the page's +14 age (cache +38 minus the mode's age).
void resource_read(Frame& f,u32 entry,u32 mode) {
  const u32 cache=f.r32(resource_cache);
  const u32 row=f.r32(entry+0xcu);
  f.w8(entry+4u,0);
  const u32 page=f.r32(f.r32(cache+0x64u)+0x44u)+(f.r32(row+0xcu)&0xffffu)*24u;
  const u32 destination=f.r32(resource_base)+(f.r32(page+8u)<<(f.r32(cache+0x34u)&31u));
  const u32 bytes=f.r32(row+4u);
  const u32 rounded=bytes&511u?(bytes|511u)+1u:bytes;
  f.w8(entry+4u,0);
  const u32 priority=f.r32(0x474d70u+mode*12u);
  if(!bound_reader || !*bound_reader)refuse("1A3B70","animation resource read without a bound reader");
  const u32 job=(*bound_reader)(f.world(),AnimationResourceRead{row,destination,rounded,f.r32(row+8u),priority,entry+4u});
  f.w32(entry+8u,job);
  if(!mode && f.r32(entry+8u)!=none) {
    if(!f.r8(entry+4u))refuse("1236A0","animation resource load wait");
    f.w32(entry+8u,none);
  }
  const u32 now=f.r32(resource_cache);
  const u32 age=f.r32(now+0x38u)-f.r32(0x474d74u+mode*12u);
  const u32 handle=f.r32(f.r32(entry+0xcu)+0xcu);
  f.w32(f.r32(f.r32(now+0x64u)+0x44u)+(handle&0xffffu)*24u+0x14u,age);
}
// 1262F0 (EAX resource row; `name` the animation's +0): the cached block's
// address, stamped with the cache age. A miss allocates the block (125FB0),
// claims its entry (16E770) and reads the resource (1260C0).
u32 resource_load(Frame& f,u32 row,u32 name) {
  (void)name;
  if(pending_request(f,row)!=none)refuse("16E8A0","animation resource request release");
  const u32 handle=f.r32(row+0xcu);
  if(handle==none) {
    const u32 block=cache_allocate(f,row);
    if(block==none)refuse("125FB0","a failed animation resource cache allocation");
    const u32 entries=f.r32(resource_entries);
    const u32 datum=datum_new_at(f,entries,block);
    if(datum==none)refuse("16E770","an occupied animation cache entry");
    const u32 entry=(datum&0xffffu)*16u+f.r32(entries+0x44u);
    f.w32(entry+8u,none);f.w8(entry+4u,0);f.w32(entry+0xcu,row);
    f.w32(row+0xcu,block);
    resource_read(f,entry,0);
  } else resource_touch(f,(handle&0xffffu)*16u+f.r32(f.r32(resource_entries)+0x44u));
  const u32 cache=f.r32(resource_cache);
  f.w32(f.r32(f.r32(cache+0x64u)+0x44u)+(f.r32(row+0xcu)&0xffffu)*24u+0x14u,f.r32(cache+0x38u));
  const u32 block=f.r32(f.r32(f.r32(cache+0x64u)+0x44u)+(f.r32(row+0xcu)&0xffffu)*24u+8u);
  return (block<<(f.r32(cache+0x34u)&31u))+f.r32(resource_base);
}
// 1E5A70 (ECX graph): the animation's codec data.
u32 resource_data(Frame& f,u32 graph,u32 id) {
  u32 g=graph;
  if(std::uint16_t(id)!=0xffffu) {
    g=0;
    const u32 index=sx16(std::uint16_t(id));
    if(s32(index)<s32(f.r32(graph+0x4cu))) {
      const u32 tag=f.r32(f.r32(graph+0x50u)+(index<<5)+4u);
      if(tag!=none)g=tag_data(f.world(),tag);
    }
  }
  const u32 high=u32(s32(id)>>16);
  const u32 animation=high==none?0:f.r32(g+0x30u)+high*0x6cu;
  if(!(f.r8(animation+0x16u)&0x10u))return f.r32(animation+0x38u);
  return resource_load(f,f.r32(g+0xb0u)+f.r32(animation+0x24u)*20u,f.r32(animation))+f.r32(animation+0x28u);
}
// 1E5DF0 (EDI record): +0 data, +4 header (animation +3C), +8 node count,
// +9 animation +11, +A frame count.
void resource_block(Frame& f,u32 block,u32 graph,u32 id) {
  if(std::uint16_t(id>>16)==0xffffu)return;
  const u32 animation=animation_lookup(f.world(),graph,id);
  const u32 data=resource_data(f,graph,id);
  const u32 nodes=f.r8(animation+0x13u),frames=f.r16(animation+0x14u),extra=f.r8(animation+0x11u);
  f.w32(block+4u,animation+0x3cu);
  f.w8(block+8u,nodes);
  f.w32(block,data);
  f.w8(block+9u,extra);
  f.w16(block+0xau,frames);
}
// 283A90: decode the animation into the orientations.
void animation_codec(Frame& f,u32 graph,u32 id,u32 count,u32 frame,u32 weight,u32 entry,u32 mask,
    u32 orientations,bool force) {
  const u32 animation=animation_lookup(f.world(),graph,id);
  Locals locals(f);
  const u32 block=locals(12);
  f.w32(block,0);f.w32(block+4u,0);f.w8(block+8u,0);f.w8(block+9u,0);
  resource_block(f,block,graph,id);
  codec_setup(f,block,frame,weight,graph,animation,count,entry,mask,orientations,force,0,0,0);
}
// 283D70: decode `id` at `fraction` of its length (frames - 1 + 1e-4,
// clamped) with `weight`, forced interpolation.
void fraction_animation(Frame& f,u32 graph,u32 id,u32 count,u32 fraction,u32 weight,u32 entry,u32 mask,
    u32 orientations) {
  const u32 animation=animation_lookup(f.world(),graph,id);
  Locals locals(f);
  const u32 block=locals(12);
  f.w32(block,0);f.w32(block+4u,0);f.w8(block+8u,0);f.w8(block+9u,0);
  resource_block(f,block,graph,id);
  if(!animation)refuse("283D70","fraction sample of a channel without an animation (the original faults)");
  const u32 last=f.add(f.from_int(sx16(std::uint16_t(f.r16(animation+0x14u)))-1u),epsilon_f);
  u32 frame=f.mul(last,fraction);
  if(f.greater(zero_f,frame))frame=zero_f;
  else if(f.greater(frame,last))frame=last;
  codec_setup(f,block,frame,weight,graph,animation,count,entry,mask,orientations,1,0,0,0);
}
// 283B50's pitch rows under the live x87 control word: FILD down; FADD the
// clamped pitch; FST (the pitch frame) and FSTP QWORD; CRT floor 32B199 and
// ceil 32B0C6 (FRNDINT under control words 0x173F / 0x1B3F, restored before
// the FSTP DWORD). A NaN or infinite argument takes the CRT error path, and
// an inexact FRNDINT with the precision exception unmasked its exception
// path: both are refused.
struct PitchRows {u32 frame,floor,ceil;};
double x87_round(double value,std::uint16_t rounding,std::uint16_t control,const char* owner) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  if((std::bit_cast<std::uint64_t>(value)>>52&0x7ffu)==0x7ffu)refuse(owner,"CRT rounding of a NaN or infinity");
  std::uint16_t saved{};double result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfldl %3\n\tfrndint\n\tfstpl %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(rounding),"m"(value):"st");
  if(!(control&0x20u) && result!=value)refuse(owner,"CRT rounding with the precision exception unmasked");
  return result;
#else
  (void)value;(void)rounding;(void)control;refuse(owner,"x87 rounding on a non-x86 host");
#endif
}
PitchRows x87_pitch_rows(u32 down,u32 pitch,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};
  const std::int32_t rows=s32(down);float frame{};double wide{};
  asm volatile("fnstcw %2\n\tfldcw %3\n\tfildl %4\n\tfadds %5\n\tfsts %0\n\tfstpl %1\n\tfldcw %2"
      :"=m"(frame),"=m"(wide),"+m"(saved):"m"(mode),"m"(rows),"m"(pitch):"st");
  const auto narrow=[&](double value){
    float out{};
    asm volatile("fnstcw %1\n\tfldcw %2\n\tfldl %3\n\tfstps %0\n\tfldcw %1":"=m"(out),"+m"(saved):"m"(mode),"m"(value):"st");
    return bits(out);
  };
  const u32 floor=narrow(x87_round(wide,0x173fu,mode,"32B199"));
  const u32 ceil=narrow(x87_round(double(frame),0x1b3fu,mode,"32B0C6"));
  return {bits(frame),floor,ceil};
#else
  (void)down;(void)pitch;(void)control;refuse("283B50","x87 rounding on a non-x86 host");
#endif
}
// 283B50 (EAX aim record: +0/+4 yaw scales below/above zero, +8/+A left and
// right columns, +C/+10 pitch scales, +14/+16 down and up rows): the aim
// animation's frame at (yaw, pitch). The yaw picks the column; the pitch
// rows below and above interpolate through 2835B0's second frame unless
// the pitch frame is within 1e-4 of one.
void aim_animation(Frame& f,u32 aim,u32 graph,u32 id,u32 count,u32 yaw,u32 pitch,u32 weight,u32 entry,u32 mask,
    u32 orientations) {
  const u32 animation=animation_lookup(f.world(),graph,id);
  Locals locals(f);
  const u32 block=locals(12);
  f.w32(block,0);f.w32(block+4u,0);f.w8(block+8u,0);f.w8(block+9u,0);
  resource_block(f,block,graph,id);
  const u32 right=sx16(u16(f.r16(aim+0xau))),left=sx16(u16(f.r16(aim+8u)));
  const u32 up=sx16(u16(f.r16(aim+0x16u)));
  const u32 columns=left+right+1u,down=sx16(u16(f.r16(aim+0x14u)));
  const auto axis=[&](u32 value,u32 scales,u32 low,u32 high) {
    const u32 scale=f.greater(zero_f,value)?f.r32(aim+scales):f.r32(aim+scales+4u);
    u32 x=f.greater(epsilon_f,scale)?zero_f:f.div(value,scale);
    const u32 lowest=f.from_int(0u-low);
    if(f.greater(lowest,x))return lowest;
    const u32 highest=f.from_int(high);
    return f.greater(x,highest)?highest:x;
  };
  const u32 column=f.add(f.from_int(left),axis(yaw,0u,left,right));
  const auto rows=x87_pitch_rows(down,axis(pitch,0xcu,down,up),f.x87());
  u32 below=zero_f;
  if(!(0.0f>f32(rows.floor))) {
    below=rows.floor;
    const u32 top=f.from_int(down+up);
    if(f.greater(below,top))below=top;
  }
  u32 above=rows.ceil;
  if(f.greater(zero_f,above))above=zero_f;
  else {
    const u32 top=f.from_int(down+up);
    if(f.greater(above,top))above=top;
  }
  const u32 width=f.from_int(columns);
  const u32 first=f.add(f.mul(width,below),column),second=f.add(f.mul(width,above),column);
  const u32 fraction=f.sub(rows.frame,below);
  if(f.greater(epsilon_f,fraction)) {
    codec_setup(f,block,first,weight,graph,animation,count,entry,mask,orientations,1,0,0,0);
  } else if(f.greater(epsilon_f,f.sub(above,rows.frame))) {
    codec_setup(f,block,second,weight,graph,animation,count,entry,mask,orientations,1,0,0,0);
  } else {
    codec_setup(f,block,first,weight,graph,animation,count,entry,mask,orientations,1,1,second,fraction);
  }
}

// ---- BFB90 animation sampling ----
// x87 steps under the live control word: one asm block per step, the 80-bit
// reciprocal root carried exactly between them (FSTP/FLD TBYTE).
struct Extended {std::array<std::uint8_t,10> bytes{};};
Extended x87_reciprocal_root(float value,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};Extended out;const float unit=1.0f;
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfsqrt\n\tfdivrs %4\n\tfstpt %0\n\tfldcw %1"
      :"=m"(out.bytes),"+m"(saved):"m"(mode),"m"(value),"m"(unit):"st");
  return out;
#else
  (void)value;(void)control;refuse("1E1720","x87 normalization on a non-x86 host");
#endif
}
float x87_scaled(const Extended& factor,float value,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfldt %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(factor.bytes),"m"(value):"st");
  return result;
#else
  (void)factor;(void)value;(void)control;refuse("1E1720","x87 normalization on a non-x86 host");
#endif
}
u32 clamp_unit(Frame& f,u32 x) {
  if(f.greater(zero_f,x))return zero_f;
  if(f.greater(x,one))return one;
  return x;
}
// 110FC0: 3x^2 - 2x^3.
u32 smoothstep(Frame& f,u32 x) {
  const u32 square=f.mul(x,x);
  return clamp_unit(f,f.sub(f.mul(square,three_f),f.mul(f.mul(square,x),two_f)));
}
// 1E1920: 6x^5 - 15x^4 + 10x^3.
u32 smootherstep(Frame& f,u32 x) {
  const u32 cube=f.mul(f.mul(x,x),x),fourth=f.mul(cube,x);
  return clamp_unit(f,f.add(f.sub(f.mul(f.mul(fourth,x),six_f),f.mul(fourth,fifteen_f)),f.mul(cube,ten_f)));
}
// 1E1620: the interpolation state's eased fraction (+0 frame, +1 length,
// +2 curve).
u32 interpolation_fraction(Frame& f,u32 state) {
  const u32 length=f.r8(state+1u);
  if(!length)return zero_f;
  u32 span=f.from_int(sx8(std::uint8_t(length)));
  if(!f.greater(span,one))span=one;
  const u32 curve=sx8(std::uint8_t(f.r8(state+2u)))-1u;
  u32 x=f.div(f.from_int(sx8(std::uint8_t(f.r8(state)))),span);
  switch(curve) {
  case 0:x=smoothstep(f,x);break;
  case 1:x=smootherstep(f,x);break;
  case 2:{const u32 d=f.sub(x,one);x=f.sub(one,f.mul(d,d));break;}
  case 3:{const u32 d=f.sub(one,x);x=f.sub(one,f.mul(f.mul(d,d),d));break;}
  default:break;
  }
  return clamp_unit(f,x);
}
// 1E1720: blend the current orientations toward the previous ones.
void crossfade(Frame& f,u32 state,u32 current,u32 count,u32 mask,u32 previous) {
  if(!f.r8(state+1u))return;
  const u32 t=interpolation_fraction(f,state);
  if(s32(count)<=0)return;
  for(u32 n=0,c=current,p=previous;;c+=0x20u,p+=0x20u) {
    const u32 index=sx16(u16(n));
    if(!mask || (f.r32(mask+u32(s32(index)>>5)*4u)>>(index&31u))&1u) {
      const u32 dot=f.add(f.add(f.add(f.mul(f.r32(p),f.r32(c)),f.mul(f.r32(c+4u),f.r32(p+4u))),
          f.mul(f.r32(c+8u),f.r32(p+8u))),f.mul(f.r32(p+0xcu),f.r32(c+0xcu)));
      const u32 keep=f.sub(one,t);
      const u32 toward=f.greater(zero_f,dot)?f.sub(zero_f,t):t;
      f.w32(c,f.add(f.mul(f.r32(p),keep),f.mul(f.r32(c),toward)));
      f.w32(c+4u,f.add(f.mul(f.r32(c+4u),toward),f.mul(keep,f.r32(p+4u))));
      const u32 c2=f.mul(f.r32(c+8u),toward),c3=f.mul(toward,f.r32(c+0xcu));
      const u32 q1=f.r32(c+4u);
      const u32 q2=f.add(c2,f.mul(keep,f.r32(p+8u)));
      f.w32(c+8u,q2);
      const u32 q3=f.add(f.mul(f.r32(p+0xcu),keep),c3);
      const u32 q0=f.r32(c);
      const u32 length=f.add(f.add(f.add(f.mul(q0,q0),f.mul(q1,q1)),f.mul(q2,q2)),f.mul(q3,q3));
      f.w32(c+0xcu,q3);
      if(f.greater(length,zero_f)) {
        const auto k=x87_reciprocal_root(f32(length),f.x87());
        for(u32 lane=0;lane<4;++lane)f.w32(c+4u*lane,bits(x87_scaled(k,f32(f.r32(c+4u*lane)),f.x87())));
      } else {
        f.w32(c,zero_f);f.w32(c+4u,zero_f);f.w32(c+8u,zero_f);f.w32(c+0xcu,one);
      }
      for(u32 at=0x10u;at<0x20u;at+=4u)
        f.w32(c+at,f.add(f.mul(f.sub(f.r32(c+at),f.r32(p+at)),t),f.r32(p+at)));
    }
    ++n;
    if(s32(sx16(u16(n)))>=s32(count))break;
  }
}
// BF9A0: jmad function overlays (graph +44/+48); none on campaign maps.
void overlays(Frame& f,u32 object,u32 manager,u32 mask,u32 count,u32 orientations);
void sample_channel(Frame& f,u32 channel,u32 weight,u32 mask,u32 count,u32 orientations);
void vehicle_animations(Frame& f,u32 object,u32 count,u32 orientations,u32 mask);
void unit_orientation_hook(Frame& f,u32 object,u32 mask,u32 count,u32 orientations);
// 10C0B0: a device with a render model and an animation manager samples
// its position (+18C) and overlay (+1AC) channels at full weight.
void device_orientation_hook(Frame& f,u32 object,u32 mask,u32 count,u32 orientations) {
  const u32 tags=f.r32(tag_instances);
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+(object&0xffffu)*12u+8u);
  const u32 model=f.r32(f.r32((f.r32(body)&0xffffu)*16u+tags+8u)+0x38u);
  if(model==none)return;
  const u32 render=f.r32(f.r32((model&0xffffu)*16u+tags+8u)+4u);
  if(render==none)return;
  (void)f.r32((render&0xffffu)*16u+tags+8u);
  if(f.r16(body+0x12au)==0xffffu)return;
  for(const u32 channel:{body+0x18cu,body+0x1acu})
    if(f.r32(channel)!=none && f.r16(channel+6u)!=0xffffu)sample_channel(f,channel,one,mask,count,orientations);
}
// 10E100: each type chain level's orientation hook (+6C) on (mask, count,
// orientations).
void type_orientation_hooks(Frame& f,u32 object,u32 mask,u32 count,u32 orientations) {
  const u32 body=object_body(f.world(),object);
  const u32 definition=f.r32(type_table+sx8(std::uint8_t(f.r8(body+0xaau)))*4u);
  for(u32 level=0;f.r32(definition+0x84u+level*4u);level=sx16(std::uint16_t(level+1u))) {
    const u32 hook=f.r32(f.r32(definition+0x84u+level*4u)+0x6cu);
    if(!hook)continue;
    if(hook==0x114750u)unit_orientation_hook(f,object,mask,count,orientations);
    else if(hook==0xf34b0u)vehicle_animations(f,object,count,orientations,mask);  // F34B0
    else if(hook==0x10c0b0u)device_orientation_hook(f,object,mask,count,orientations);
    else refuse(hex(hook).c_str(),"type orientation hook");
  }
}
// 1E32A0: the inherited graph's codec entry for `id`, or 0.
u32 codec_entry(const M& m,u32 graph,u32 id) {
  if(u16(id>>16)==0xffffu || std::int16_t(id)<0)return 0;
  const u32 index=sx16(u16(id));
  if(s32(index)>=s32(r32(m,graph+0x4cu)))return 0;
  animation_request(m,graph,id);
  const u32 entry=r32(m,graph+0x50u)+index*32u;
  const u32 tag=r32(m,entry+4u);
  if(tag==none)return 0;
  if(s32(r32(m,entry+0x10u))>0 && s32(r32(m,entry+8u))>0 && r32(m,entry+0x14u) && r32(m,entry+0xcu))return entry;
  (void)animation_lookup(m,graph,id);
  return 0;
}
// The channel's node mask (controls +12 bit 11: graph +5C, bit 10: +7C)
// combined with `mask` (283A50 and its inline forms): a distinct pair
// takes the 284090 intersection (into 0x56E060).
u32 channel_mask(Frame& f,u32 channel,u32 controls,u32 mask) {
  u32 source=0;
  if(controls&0x800u)source=tag_data(f.world(),f.r32(channel))+0x5cu;
  else if(controls&0x400u)source=tag_data(f.world(),f.r32(channel))+0x7cu;
  if(source && mask && source!=mask) {
    // REP MOVSD, then 284090 (EAX 0x56E060, ECX mask) in place: words 0..5
    // by AND to memory; word 7 is read before word 6 is stored, the mask's
    // word 7 after it.
    constexpr u32 d=0x56e060u;
    copy_words(f,d,source,8);
    for(u32 k=0;k<6u;++k){const u32 s=f.r32(mask+4u*k);f.w32(d+4u*k,f.r32(d+4u*k)&s);}
    const u32 w6=f.r32(d+0x18u)&f.r32(mask+0x18u),w7=f.r32(d+0x1cu);
    f.w32(d+0x18u,w6);
    f.w32(d+0x1cu,w7&f.r32(mask+0x1cu));
    return d;
  }
  return source?source:mask;
}
// The channel's inherited codec entry (1E32A0) when both handle words are
// set, else 0.
u32 channel_entry(Frame& f,u32 channel) {
  if(f.r16(channel+6u)==0xffffu || f.r16(channel+4u)==0xffffu)return 0;
  return codec_entry(f.world(),tag_data(f.world(),f.r32(channel)),f.r32(channel+4u));
}
// 1CF210: sample one animation channel (+0 graph, +4 id, +10 resource
// gate, +12 controls, +18 frame, +1C codec state) into the orientations.
void sample_channel(Frame& f,u32 channel,u32 weight,u32 mask,u32 count,u32 orientations) {
  const M& m=f.world();
  u32 animation=0;
  if(f.r16(channel+6u)!=0xffffu)animation=animation_lookup(m,tag_data(m,f.r32(channel)),f.r32(channel+4u));
  if(f.r8(channel+0x10u) && f.r8(animation+0x10u)) {
    animation_request(m,tag_data(m,f.r32(channel)),f.r32(channel+4u));
    return;
  }
  const u32 controls=f.r16(channel+0x12u);
  bool force=(controls>>6)&1u;
  if(controls&1u) {
    const u32 frame=f.r32(channel+0x18u);
    if(!f.equal(frame,zero_f) && (!f.equal(frame,one) || f.r16(f.r32(0x51df54u)+2u)!=0x1eu))force=true;
  }
  const u32 combined=channel_mask(f,channel,controls,mask);
  animation_codec(f,tag_data(m,f.r32(channel)),f.r32(channel+4u),count,f.r32(channel+0x1cu),weight,
      channel_entry(f,channel),combined,orientations,force);
}
// 1E31E0: the aim record (graph +28 rows of 0x1C, from +4) of the handle's
// animation (+12 index, 0xFF none) in the graph or its child, or 0.
u32 aim_record(const M& m,u32 graph,u32 id) {
  const u16 high=u16(id>>16),low=u16(id);
  if(high==0xffffu)return 0;
  animation_request(m,graph,id);
  u32 g=graph;
  if(low!=0xffffu) {
    g=0;
    if(s32(sx16(low))<s32(r32(m,graph+0x4cu))) {
      const u32 tag=r32(m,r32(m,graph+0x50u)+sx16(low)*32u+4u);
      if(tag!=none)g=tag_data(m,tag);
    }
  }
  const std::uint8_t index=r8(m,r32(m,g+0x30u)+sx16(high)*0x6cu+0x12u);
  if(index==0xffu)return 0;
  return r32(m,g+0x28u)+sx8(index)*0x1cu+4u;
}
// 1CF3B0 (ESI channel): the channel's aim animation (1E31E0) at (yaw,
// pitch) with `weight` (283B50).
void sample_aim_channel(Frame& f,u32 channel,u32 yaw,u32 pitch,u32 weight,u32 mask,u32 count,u32 orientations) {
  const M& m=f.world();
  if(f.r16(channel+6u)==0xffffu)return;
  const u32 aim=aim_record(m,tag_data(m,f.r32(channel)),f.r32(channel+4u));
  if(!aim)return;
  const u32 combined=channel_mask(f,channel,f.r16(channel+0x12u),mask);
  const u32 entry=channel_entry(f,channel);
  aim_animation(f,aim,tag_data(m,f.r32(channel)),f.r32(channel+4u),count,yaw,pitch,weight,entry,combined,orientations);
}
// 1CF4B0 (EBX channel, EDX mask): the channel's animation at `fraction` of
// its length with `weight` (283D70).
void sample_fraction_channel(Frame& f,u32 channel,u32 mask,u32 fraction,u32 weight,u32 count,u32 orientations) {
  const M& m=f.world();
  const u32 combined=channel_mask(f,channel,f.r16(channel+0x12u),mask);
  const u32 entry=channel_entry(f,channel);
  fraction_animation(f,tag_data(m,f.r32(channel)),f.r32(channel+4u),count,fraction,weight,entry,combined,
      orientations);
}
// F2C40 (F2CC0's channel callback, F34B0's {count, orientations, mask}): a
// channel animation with an aim record (+12) samples it at (`value`,
// `pitch`) (1CF3B0), any other at the fraction `value` of its length
// (1CF4B0).
void vehicle_channel(Frame& f,u32 channel,u32 value,u32 pitch,u32 weight,u32 count,u32 orientations,u32 mask) {
  const M& m=f.world();
  u32 animation=0;
  if(f.r16(channel+6u)!=0xffffu)animation=animation_lookup(m,tag_data(m,f.r32(channel)),f.r32(channel+4u));
  if(f.r8(animation+0x12u)!=0xffu)sample_aim_channel(f,channel,value,pitch,weight,mask,count,orientations);
  else sample_fraction_channel(f,channel,mask,value,weight,count,orientations);
}

// ---- Vehicle and unit orientation hooks (F34B0 -> F2CC0, 114750) ----
constexpr u32 half_f=0x3f000000u;          // [0x46A0CC] 0.5f
constexpr u32 byte_scale_f=0x3b808081u;    // [0x46A0C4] 1/255
weapon_animation::Fp script_fp(const Frame& f){return {f.x87(),f.mxcsr()};}
// 1CE5D0 (ECX record): channel defaults: graph and key NONE, handle words
// 0xFFFF, bytes +C/+D 0xFF, word +E 0xFFFF, flags 0, rate 1.0, frame 0.
void channel_clear(Frame& f,u32 r) {
  f.w16(r+4u,0xffffu);f.w16(r+6u,0xffffu);f.w32(r,none);f.w16(r+4u,0xffffu);f.w16(r+6u,0xffffu);
  f.w32(r+0x1cu,zero_f);f.w8(r+0x10u,0);f.w8(r+0x11u,0);f.w16(r+0x12u,0);f.w32(r+0x18u,one);f.w16(r+0x14u,0);
  f.w16(r+0x16u,0);f.w32(r+8u,none);f.w16(r+0xeu,0xffffu);f.w8(r+0xcu,0xffu);f.w8(r+0xdu,0xffu);
}
// 1CE720 (track weapon) on a channel record of the Frame.
bool channel_start(Frame& f,u32 record,u32 graph,u32 flags,u32 id) {
  FrameMemory memory(f);
  return weapon_animation::channel_start(memory,record,graph,flags,id,none,0xffu,0xffu,0xffu);
}
// 1E5160 (EAX iterator, EDX graph): the graph's next +40 row (0x28 bytes,
// count +3C) into the iterator (+2A index); a set animation (+24) is
// requested (1E5C80).
bool graph_rows_next(Frame& f,u32 it,u32 graph) {
  const u16 next=u16(f.r16(it+0x2au)+1u);
  const u32 index=sx16(next);
  if(s32(index)>=s32(f.r32(graph+0x3cu)))return false;
  f.w16(it+0x2au,next);
  const u32 row=f.r32(graph+0x40u)+index*0x28u;
  f.w32(it+0x24u,f.r32(row+4u));
  f.w32(it+0x10u,f.r32(row+0x14u));f.w32(it+0xcu,f.r32(row+0x10u));f.w32(it,f.r32(row+8u));
  f.w32(it+8u,f.r32(row+0xcu));f.w32(it+0x20u,index);f.w32(it+4u,f.r32(row+0x18u));f.w32(it+0x1cu,f.r32(row+0x24u));
  f.w32(it+0x18u,f.r32(row+0x20u));f.w32(it+0x14u,f.r32(row+0x1cu));
  if(f.r16(it+0x26u)!=0xffffu)animation_request(f.world(),graph,f.r32(it+0x24u));
  return true;
}
// The vehicle ramps: x / scale (the forward scale for x > 0, else the
// reverse one) mapped to [0, 1] as x * 0.5 + 0.5; 0.5 without a positive
// scale.
u32 vehicle_ramp(Frame& f,u32 x,u32 forward,u32 reverse) {
  if(f.greater(x,zero_f)) {
    if(!f.greater(forward,zero_f))return half_f;
    x=f.div(x,forward);
  } else {
    if(!f.greater(reverse,zero_f))return half_f;
    x=f.div(x,reverse);
  }
  x=f.add(f.mul(x,half_f),half_f);
  if(f.greater(zero_f,x))return zero_f;
  return f.greater(x,one)?one:x;
}
// F2CC0 with F2C40 (F34B0): the vehicle's animation overlays. The overlay
// handles (+404..+418 from 1D3C80, +41C..+424 from 1D4450) are cached per
// graph (+400). Each set one starts on a private channel (1CE720) and is
// sampled through F2C40: +404 at the steering value +35C, +408 at the
// lateral velocity (BC200) over the definition's +1F4, +40C/+410 at the
// ramps of the speed +354 and the forward velocity, +414/+418 at +360/+364
// over the definition's +20C; then the graph's +40 rows at their body +398
// byte / 255 (0xFF: 1.0), and the three aim overlays at the clamped
// +288..+290 * 0.5 + 0.5.
void vehicle_animations(Frame& f,u32 object,u32 count,u32 orientations,u32 mask) {
  M& m=f.world();
  const u32 row=(object&0xffffu)*12u;
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+row+8u);
  const u32 tags=f.r32(tag_instances);
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  const u32 hlmt=f.r32((f.r32(definition+0x38u)&0xffffu)*16u+tags+8u);
  const u32 render=f.r32((f.r32(hlmt+4u)&0xffffu)*16u+tags+8u);
  const auto linear=native_tu15_object_velocities(m,object).linear;  // BC200
  const u32 vx=bits(linear.x),vy=bits(linear.y),vz=bits(linear.z);
  if(!render)return;
  if(f.r16(f.r32(f.r32(f.r32(object_pool)+0x44u)+row+8u)+0x12au)==0xffffu)return;
  const u32 manager=sx16(u16(f.r16(body+0x12au)))+body;
  Locals locals(f);
  const u32 channel=locals(0x20),rows_channel=locals(0x20),it=locals(0x2c);
  channel_clear(f,channel);
  if(f.r32(manager+0x68u)!=f.r32(body+0x400u) && f.r32(manager+0x74u)!=none && f.r32(manager+0x78u)!=none) {
    f.w32(body+0x400u,f.r32(manager+0x68u));
    static constexpr std::array<std::array<u32,2>,6> overlays{{{0x404u,0x8000078u},{0x408u,0x4000079u},
        {0x40cu,0x800007au},{0x410u,0x800007bu},{0x414u,0xc00007du},{0x418u,0x110005c9u}}};
    // 1D3C80 with both keys 0x7000101 (the manager's +74/+78) and 1D4450:
    // track units' manager_overlay_trial / manager_aim_handle (the trial
    // channel record is private there, as on the original's stack).
    for(const auto [at,name]:overlays)
      f.w32(body+at,manager_overlay_trial(m,manager,0x7000101u,0x7000101u,name,f.fp()));
    for(u32 k=0;k<3;++k)f.w32(body+0x41cu+4u*k,manager_aim_handle(m,manager,k));
  }
  const auto sample=[&](u32 record,u32 value){vehicle_channel(f,record,value,zero_f,one,count,orientations,mask);};
  const auto started=[&](u32 record,u32 handle_at) {
    return f.r16(handle_at+2u)!=0xffffu && f.r32(manager+0x68u)!=none &&
        channel_start(f,record,f.r32(manager+0x68u),0,f.r32(handle_at));
  };
  if(started(channel,body+0x404u))sample(channel,f.r32(body+0x35cu));
  if(started(channel,body+0x408u)) {
    const u32 lx=f.sub(f.mul(f.r32(body+0x80u),f.r32(body+0x78u)),f.mul(f.r32(body+0x84u),f.r32(body+0x74u)));
    const u32 ly=f.sub(f.mul(f.r32(body+0x70u),f.r32(body+0x84u)),f.mul(f.r32(body+0x7cu),f.r32(body+0x78u)));
    const u32 lz=f.sub(f.mul(f.r32(body+0x7cu),f.r32(body+0x74u)),f.mul(f.r32(body+0x70u),f.r32(body+0x80u)));
    const u32 lateral=f.add(f.add(f.mul(vx,lx),f.mul(ly,vy)),f.mul(vz,lz));
    sample(channel,f.mul(f.add(f.div(lateral,f.r32(definition+0x1f4u)),one),half_f));
  }
  if(started(channel,body+0x40cu))
    sample(channel,vehicle_ramp(f,f.r32(body+0x354u),f.r32(definition+0x1f4u),f.r32(definition+0x1f8u)));
  if(started(channel,body+0x410u)) {
    const u32 speed=f.add(f.add(f.mul(f.r32(body+0x78u),vz),f.mul(f.r32(body+0x74u),vy)),f.mul(vx,f.r32(body+0x70u)));
    sample(channel,vehicle_ramp(f,speed,f.r32(definition+0x1f4u),f.r32(definition+0x1f8u)));
  }
  for(const u32 at:{0x414u,0x418u}) {
    if(!started(channel,body+at))continue;
    const u32 scale=f.r32(definition+0x20cu);
    sample(channel,f.greater(scale,zero_f)?f.div(f.r32(body+at-0xb4u),scale):zero_f);
  }
  channel_clear(f,rows_channel);
  f.w32(it,none);f.w32(it+8u,zero_f);f.w32(it+0xcu,zero_f);f.w32(it+0x10u,zero_f);f.w32(it+0x20u,none);
  for(u32 at=0x24u;at<0x2cu;at+=2u)f.w16(it+at,0xffffu);
  while(graph_rows_next(f,it,tag_data(m,f.r32(manager+0x68u)))) {
    if(f.r32(manager+0x68u)==none ||
        !channel_start(f,rows_channel,f.r32(manager+0x68u),0,f.r32(it+0x24u)))continue;
    const u32 level=f.r8(f.r32(it+0x20u)+body+0x398u);
    sample(rows_channel,level==0xffu?one:f.mul(f.from_int(level),byte_scale_f));
  }
  for(u32 k=0;k<3;++k) {
    const u32 at=body+0x41cu+4u*k;
    if(f.r16(at+2u)==0xffffu)continue;
    u32 value=f.add(f.mul(f.r32(body+0x288u+4u*k),half_f),half_f);
    if(f.greater(zero_f,value))value=zero_f;
    else if(f.greater(value,one))value=one;
    if(f.r32(manager+0x68u)!=none && channel_start(f,channel,f.r32(manager+0x68u),0,f.r32(at)))sample(channel,value);
  }
}

constexpr u32 fade_fast_f=0x3e88b439u;     // [0x450170] 0.267 s
constexpr u32 fade_slow_f=0x3e08b439u;     // [0x46A1E0] 0.1335 s
// 1D3740 (EAX manager, EBX flags 0): 1D36C0 (track weapon) starts `record`
// with `handle` (key NONE, bytes 0xFF); false without a graph.
bool manager_start(Frame& f,u32 manager,u32 record,u32 handle,u32 flags=0) {
  if(f.r32(manager+0x68u)==none)return false;
  FrameMemory memory(f);
  return weapon_animation::manager_start_channel(memory,manager,flags,record,handle,none,0xffu,0xffu,0xffu,script_fp(f));
}
// ---- BF9A0 function overlays ----
// BFB1A..BFB49: FILD (tick + object), +2^32 when negative, FMUL the game
// time's +4, FMUL [0x46A17C] (1/30); the extended value 37DC0A receives.
long double x87_overlay_time(std::int32_t ticks,u32 rate_bits,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};long double result{};
  const float rate=f32(rate_bits),scale=f32(0x3d088889u),wrap=f32(0x4f800000u);
  if(ticks<0)
    asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfadds %6\n\tfmuls %4\n\tfmuls %5\n\tfstpt %0\n\tfldcw %1"
        :"=m"(result),"+m"(saved):"m"(mode),"m"(ticks),"m"(rate),"m"(scale),"m"(wrap):"st");
  else
    asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfmuls %5\n\tfstpt %0\n\tfldcw %1"
        :"=m"(result),"+m"(saved):"m"(mode),"m"(ticks),"m"(rate),"m"(scale):"st");
  return result;
#else
  (void)ticks;(void)rate_bits;(void)control;refuse("BF9A0","x87 overlay time on a non-x86 host");
#endif
}
// 37DC0A (CRT fmod, FILD frame count divisor): a normal dividend runs
// 37DC14 (FPREM until C2 clears, exact), a zero or denormal one 331F68
// (FLDZ); other operand classes reach the CRT error methods (3334F0).
// BFB53: FSTP dword of the remainder under the caller's rounding.
u32 x87_overlay_frame(long double dividend,std::int32_t frames,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  if(!frames)refuse("3334F0","CRT fmod by a zero frame count (37DC0A error method)");
  long double remainder{};
  switch(std::fpclassify(dividend)) {
  case FP_NORMAL:
    asm volatile("fildl %2\n\tfldt %1\n\t"
        "1:\n\tfprem\n\tfnstsw %%ax\n\ttestw $0x400, %%ax\n\tjnz 1b\n\tfstp %%st(1)\n\tfstpt %0"
        :"=m"(remainder):"m"(dividend),"m"(frames):"ax","cc","st","st(1)");
    break;
  case FP_ZERO:case FP_SUBNORMAL:remainder=0.0L;break;
  default:refuse("3334F0","CRT fmod of an infinite or NaN overlay time (37DC0A error method)");
  }
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float result{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfldt %3\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(remainder):"st");
  return bits(result);
#else
  (void)dividend;(void)frames;(void)control;refuse("BF9A0","x87 overlay frame on a non-x86 host");
#endif
}
// BF9A0 (object, mode, manager, mask, count, orientations): each function
// overlay of the manager's graph (+44 rows of 0x14 at +48: +4 animation, +A
// mode, +C function) requests its animation (1E5C80); with a function and a
// graph, a private channel starts on it (1D36C0, flags 0x7F) and the
// object's function value (BCB70) sets the channel's frame (1CE810): mode 0
// scales the animation's last frame (+14 - 1) and samples at full weight,
// mode 1 takes (tick + object) game time modulo the frame count (37DC0A)
// and samples at the value (1CF210). Other modes only start the channel.
void overlays(Frame& f,u32 object,u32 manager,u32 mask,u32 count,u32 orientations) {
  Locals locals(f);
  const u32 record=locals(0x20);
  channel_clear(f,record);
  FrameMemory fm(f);
  for(std::uint16_t index=0xffffu;;) {
    const u32 graph=tag_data(f.world(),f.r32(manager+0x68u));
    index=std::uint16_t(index+1u);
    const u32 n=sx16(index);
    if(s32(n)>=s32(f.r32(graph+0x44u)))return;
    const u32 row=f.r32(graph+0x48u)+n*0x14u;
    const u32 id=f.r32(row+4u),function=f.r32(row+0xcu);
    const u32 mode=f.r16(row+0xau);
    if(std::uint16_t(id>>16)!=0xffffu)animation_request(f.world(),graph,id);
    if(!function || f.r32(manager+0x68u)==none)continue;
    if(!manager_start(f,manager,record,id,0x7fu))continue;
    u32 animation=0;
    if(f.r16(record+6u)!=0xffffu)animation=animation_lookup(f.world(),tag_data(f.world(),f.r32(record)),f.r32(record+4u));
    OriginalOutput value{};
    const auto active=object_function_value(fm,object,function,value,f.x87(),f.mxcsr());
    if(auto& log=node_rebuild_debug_log();log.enabled) {
      char text[160];
      std::snprintf(text,sizeof text,"BF9A0 object %08x type %d tag %08x function %08x mode %u active %u value %08x frames %d\n",
          object,int(std::int8_t(f.r8(object_body(f.world(),object)+0xaau))),f.r32(object_body(f.world(),object)),
          function,mode,unsigned(active),value.value,animation?int(std::int16_t(f.r16(animation+0x14u))):-1);
      log.text+=text;
    }
    if(mode!=0u && mode!=1u)continue;
    if(!animation)refuse("BF9A0","an overlay channel without an animation (the original faults)");
    const std::int32_t frames=std::int32_t(sx16(std::uint16_t(f.r16(animation+0x14u))));
    u32 weight=one;
    if(!mode) {
      weapon_animation::channel_set_frame(fm,record,f.mul(f.from_int(u32(frames-1)),value.value),script_fp(f));
    } else {
      const u32 time=f.r32(0x51df54u);
      const auto elapsed=x87_overlay_time(std::int32_t(f.r32(time+8u)+object),f.r32(time+4u),f.x87());
      weapon_animation::channel_set_frame(fm,record,x87_overlay_frame(elapsed,frames,f.x87()),script_fp(f));
      weight=value.value;
    }
    sample_channel(f,record,weight,mask,count,orientations);
  }
}
// BBFF0 (EAX object, ESI forward, EDI up): the object's forward (+70) and
// up (+7C), through its parent node matrix (+18 node of the parent's +116
// block) when attached.
void object_world_axes(Frame& f,u32 object,u32 forward,u32 up) {
  const u32 rows=f.r32(f.r32(object_pool)+0x44u);
  const u32 body=f.r32(rows+(object&0xffffu)*12u+8u);
  const u32 parent=f.r32(body+0x14u);
  if(parent==none) {
    if(forward)copy_words(f,forward,body+0x70u,3);
    if(up)copy_words(f,up,body+0x7cu,3);
    return;
  }
  const u32 parent_body=f.r32(rows+(parent&0xffffu)*12u+8u);
  const u32 node=sx16(u16(f.r16(parent_body+0x116u)))+sx8(std::uint8_t(f.r8(body+0x18u)))*0x34u+parent_body;
  for(const auto [out,in]:{std::array<u32,2>{forward,body+0x70u},std::array<u32,2>{up,body+0x7cu}}) {
    if(!out)continue;
    const u32 y=f.r32(in+4u),z=f.r32(in+8u),x=f.r32(in);
    for(u32 k=0;k<3;++k)
      f.w32(out+4u*k,f.add(f.add(f.mul(f.r32(node+0x1cu+4u*k),z),f.mul(f.r32(node+0x10u+4u*k),y)),
          f.mul(f.r32(node+4u+4u*k),x)));
  }
}
// x87 under the live control word. 114750's aim angles: yaw = FPATAN(left,
// forward), pitch = FPATAN(up, FSQRT(forward^2 + left^2)), each FSTP to a
// float.
std::array<u32,2> x87_aim_angles(u32 forward,u32 left,u32 up,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float yaw{},pitch{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[left]\n\tflds %[forward]\n\tfpatan\n\tfstps %[yaw]\n\t"
      "flds %[up]\n\tflds %[forward]\n\tfmuls %[forward]\n\tflds %[left]\n\tfmuls %[left]\n\tfaddp %%st,%%st(1)\n\t"
      "fsqrt\n\tfxch %%st(1)\n\tfxch %%st(1)\n\tfpatan\n\tfstps %[pitch]\n\tfldcw %[saved]"
      :[yaw]"=m"(yaw),[pitch]"=m"(pitch),[saved]"+m"(saved)
      :[mode]"m"(mode),[left]"m"(left),[forward]"m"(forward),[up]"m"(up):"st","st(1)","st(2)");
  return {bits(yaw),bits(pitch)};
#else
  (void)forward;(void)left;(void)up;(void)control;refuse("114750","x87 angles on a non-x86 host");
#endif
}
// 1208F0 (ECX vector, EAX out): yaw FPATAN(y, x) and pitch FPATAN(z,
// FSQRT(y*y + x*x)), the original's register sequence.
std::array<u32,2> x87_vector_angles(u32 x,u32 y,u32 z,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float yaw{},pitch{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[y]\n\tflds %[x]\n\tfpatan\n\tfstps %[yaw]\n\t"
      "flds %[y]\n\tflds %[x]\n\tflds %[z]\n\tfld %%st(2)\n\tfmulp %%st,%%st(3)\n\tfld %%st(1)\n\tfmul %%st(2),%%st\n\t"
      "faddp %%st,%%st(3)\n\tfxch %%st(2)\n\tfsqrt\n\tfxch %%st(2)\n\tfxch %%st(1)\n\tfxch %%st(2)\n\tfpatan\n\t"
      "fstp %%st(1)\n\tfstps %[pitch]\n\tfldcw %[saved]"
      :[yaw]"=m"(yaw),[pitch]"=m"(pitch),[saved]"+m"(saved)
      :[mode]"m"(mode),[x]"m"(x),[y]"m"(y),[z]"m"(z):"st","st(1)","st(2)","st(3)");
  return {bits(yaw),bits(pitch)};
#else
  (void)x;(void)y;(void)z;(void)control;refuse("1208F0","x87 angles on a non-x86 host");
#endif
}
// FLD; FSUB 0.5; FABS; FLD 1e-4; FCOMIP; JA: 1e-4 > |x - 0.5| under the
// live control word.
bool x87_near_half(u32 x,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};std::uint8_t above{};
  const u32 center=half_f,limit=epsilon_f;
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\tflds %[x]\n\tfsubs %[center]\n\tfabs\n\tflds %[limit]\n\t"
      "fcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[above]\n\tfldcw %[saved]"
      :[above]"=qm"(above),[saved]"+m"(saved):[mode]"m"(mode),[x]"m"(x),[center]"m"(center),[limit]"m"(limit)
      :"st","st(1)","cc");
  return above!=0;
#else
  (void)x;(void)control;refuse("114750","x87 compare on a non-x86 host");
#endif
}
// FLD; FABS; FLD 1e-4; FCOMIP: 1e-4 > |x| (exact, unordered false).
bool tiny(u32 x){return f32(epsilon_f)>std::fabs(f32(x));}
// CB4A0 (ECX unit): the seat record (+1FC index, 0xB0 rows at the parent
// unit's definition +1CC), or 0.
u32 unit_seat(Frame& f,u32 unit) {
  const u32 rows=f.r32(f.r32(object_pool)+0x44u);
  const u32 body=f.r32(rows+(unit&0xffffu)*12u+8u);
  const u32 parent=f.r32(body+0x14u);
  if(parent==none)return 0;
  const u16 seat=u16(f.r16(body+0x1fcu));
  if(seat==0xffffu)return 0;
  const u32 parent_body=f.r32(rows+(parent&0xffffu)*12u+8u);
  if(!((1u<<(f.r8(parent_body+0xaau)&31u))&3u))return 0;
  return sx16(seat)*0xb0u+f.r32(tag_data(f.world(),f.r32(parent_body))+0x1ccu);
}
// 111F70 (EAX unit): the aim block's +DC channel is set and running (+EE
// bit 0, no +ED bits 0/3) and the aim byte's bit 7 is set.
bool aim_channel_live(Frame& f,u32 unit) {
  const u32 body=object_body(f.world(),unit);
  const u32 aim=sx16(u16(f.r16(body+0x33eu)))+body;
  if(f.r32(aim+0xdcu)==none || f.r16(aim+0xe2u)==0xffffu || !(f.r8(aim+0xeeu)&1u) || (f.r8(aim+0xedu)&9u))return false;
  return (f.r8(aim)>>7)&1u;
}
// 111500 (ECX manager): playback settled: +6C bit 0; else with channel 2
// (+40) set, unless it runs (+52 bit 0 without +51 bits 0/3); else channel
// 0's stopped state (E8D50, track weapon), false when it is unset.
bool manager_settled(Frame& f,u32 manager) {
  if(f.r8(manager+0x6cu)&1u)return true;
  if(f.r32(manager+0x40u)!=none && f.r16(manager+0x46u)!=0xffffu)
    return !(f.r8(manager+0x52u)&1u) || (f.r8(manager+0x51u)&9u);
  if(f.r32(manager)==none || f.r16(manager+6u)==0xffffu)return false;
  return weapon_animation::channel_stopped(f.world(),manager);
}
// 114750: the unit's aim block (+33E) overlays and aiming. Set overlay
// channels (+9C, +BC, +DC at the seat's weight) and the +104/+108 handles
// sample at full weight; the aim byte loses bits 5 and 6. Unless the
// definition (+BC bits 10/11) disables it: the moving overlay (+208
// weight, 1E3F30 0x400004C), the three aim overlays (+118..+120 at the
// clamped +288..+290) and the aiming blend: the angles from the unit's
// axes (BBFF0) to the aim target (+168) pick the aim records (1E31E0) of
// the +FC/+100 handles, sampled (1CF3B0) by the +90/+88 fades; the look
// target (+18C) through +114 by the +8C fade; then the +10C/+110 weapon
// handles. The +90/+8C fades restart (1E14F0) at 0.267 s when the
// animation allows aiming/looking (113C70), else 0.1335 s.
void unit_orientation_hook(Frame& f,u32 object,u32 mask,u32 count,u32 orientations) {
  M& m=f.world();
  const u32 row=(object&0xffffu)*12u;
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+row+8u);
  const u32 tags=f.r32(tag_instances);
  const u32 manager=sx16(u16(f.r16(body+0x12au)))+body;
  const u32 aim=sx16(u16(f.r16(body+0x33eu)))+body;
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  const u32 graph=f.r32((f.r32(manager+0x68u)&0xffffu)*16u+tags+8u);
  (void)f.r32((f.r32(f.r32((f.r32(definition+0x38u)&0xffffu)*16u+tags+8u)+4u)&0xffffu)*16u+tags+8u);
  Locals locals(f);
  const u32 record=locals(0x20),first=locals(0x20),second=locals(0x20),forward=locals(12),up=locals(12);
  channel_clear(f,record);
  const auto sample=[&](u32 channel,u32 weight){sample_channel(f,channel,weight,mask,count,orientations);};
  const auto aim_sample=[&](u32 channel,u32 yaw,u32 pitch,u32 weight) {
    sample_aim_channel(f,channel,yaw,pitch,weight,mask,count,orientations);
  };
  for(const u32 at:{0x9cu,0xbcu})if(f.r32(aim+at)!=none && f.r16(aim+at+6u)!=0xffffu)sample(aim+at,one);
  if(f.r32(aim+0xdcu)!=none && f.r16(aim+0xe2u)!=0xffffu) {
    const u32 seat=unit_seat(f,object);
    u32 weight=one;
    if(aim_channel_live(f,object) && seat && f.greater(f.r32(seat+0x1cu),zero_f))weight=f.r32(seat+0x1cu);
    sample(aim+0xdcu,weight);
  }
  for(const auto [at,flags]:{std::array<u32,2>{0x104u,0x400u},std::array<u32,2>{0x108u,0x800u}}) {
    if(f.r16(aim+at+2u)==0xffffu || f.r32(manager+0x68u)==none)continue;
    if(channel_start(f,record,f.r32(manager+0x68u),flags,f.r32(aim+at)))sample(record,one);
  }
  f.w8(aim,f.r8(aim)&0x9fu);
  // The definition's +BC bits 11 (overlays) and 10 (aiming), a missing
  // graph and state 0xE0000C2 skip to the weapon handles (0x11533E).
  [&] {
    if((f.r32(definition+0xbcu)>>11)&1u || f.r32(manager+0x68u)==none)return;
    if(f.greater(f.r32(body+0x208u),zero_f)) {
      const u32 current=object_body(m,object);
      if(f.r16(sx16(u16(f.r16(current+0x33eu)))+current+0x36u)==0xffffu) {
        weapon_animation::FrameWord handle;  // 1D4400
        device_animation::overlay_lookup(m,tag_data(m,f.r32(manager+0x68u)),0x400004cu,f.r32(manager+0x70u),
            f.r32(manager+0x74u),f.r32(manager+0x78u),nullptr,nullptr,nullptr,handle);
        if(u16(handle()>>16)!=0xffffu && manager_start(f,manager,record,handle()))sample(record,f.r32(body+0x208u));
      }
    }
    if(f.r32(manager+0x7cu)==0xe0000c2u)return;
    if((f.r8(aim)>>1)&1u) {
      for(u32 k=0;k<3;++k) {
        const u32 handle=f.r32(aim+0x118u+4u*k);
        if(u16(handle>>16)==0xffffu)continue;
        u32 x=f.add(f.mul(f.r32(body+0x288u+4u*k),half_f),half_f);
        if(f.greater(zero_f,x))x=zero_f;
        else if(f.greater(x,one))x=one;
        if(x87_near_half(x,f.x87()))continue;
        if(manager_start(f,manager,record,handle))sample_fraction_channel(f,record,mask,x,one,count,orientations);
      }
    }
    if((f.r32(definition+0xbcu)>>10)&1u)return;
    const u32 defaults=f.r32(0x474d24u);
    u32 yaw=f.r32(defaults),pitch=f.r32(defaults+4u);
    object_world_axes(f,object,forward,up);
    const u32 fx=f.r32(forward),fy=f.r32(forward+4u),fz=f.r32(forward+8u);
    const u32 ux=f.r32(up),uy=f.r32(up+4u),uz=f.r32(up+8u);
    const u32 aiming=f.r32(aim+0xfcu),looking=f.r32(aim+0x100u);
    const u32 lx=f.sub(f.mul(fz,uy),f.mul(uz,fy)),ly=f.sub(f.mul(uz,fx),f.mul(fz,ux)),lz=f.sub(f.mul(fy,ux),f.mul(uy,fx));
    const auto dots=[&](u32 at)->std::array<u32,3> {
      const u32 tz=f.r32(at+8u),ty=f.r32(at+4u),tx=f.r32(at);
      return {f.add(f.add(f.mul(fz,tz),f.mul(fy,ty)),f.mul(tx,fx)),f.add(f.add(f.mul(lz,tz),f.mul(ly,ty)),f.mul(tx,lx)),
              f.add(f.add(f.mul(uz,tz),f.mul(uy,ty)),f.mul(tx,ux))};
    };
    const auto ranges=[&](u32 at,u32 rec) {
      f.w32(at,f.sub(zero_f,f.mul(f.from_int(sx16(u16(f.r16(rec+8u)))),f.r32(rec))));
      f.w32(at+4u,f.mul(f.from_int(sx16(u16(f.r16(rec+0xau)))),f.r32(rec+4u)));
      f.w32(at+8u,f.sub(zero_f,f.mul(f.from_int(sx16(u16(f.r16(rec+0x14u)))),f.r32(rec+0xcu))));
      f.w32(at+0xcu,f.mul(f.from_int(sx16(u16(f.r16(rec+0x16u)))),f.r32(rec+0x10u)));
    };
    std::uint8_t allowed=0;
    u32 seconds=fade_slow_f;
    if(u16(aiming>>16)!=0xffffu || u16(looking>>16)!=0xffffu) {
      const u32 aim_record_1=u16(aiming>>16)!=0xffffu?aim_record(m,graph,aiming):0u;
      const u32 aim_record_2=u16(looking>>16)!=0xffffu?aim_record(m,graph,looking):0u;
      if(aim_record_1 || aim_record_2) {
        const auto [d_forward,d_left,d_up]=dots(body+0x168u);
        f.w8(aim,f.r8(aim)|0x20u);
        const auto angles=x87_aim_angles(d_forward,d_left,d_up,f.x87());
        yaw=angles[0];pitch=angles[1];
        ranges(aim+4u,aim_record_1?aim_record_1:aim_record_2);
        allowed=unit_animation_allows(m,object,4);
        const u32 state=f.r32(manager+0x7cu);
        if((state==0x700005cu || state==0x700005du) && !manager_settled(f,manager))allowed=0;
        const bool centered=tiny(pitch) && tiny(yaw);
        if(f.r8(aim+0x91u)) {
          const u32 w1=interpolation_fraction(f,aim+0x90u),w2=interpolation_fraction(f,aim+0x88u);
          channel_clear(f,first);channel_clear(f,second);
          manager_start(f,manager,first,aiming);
          manager_start(f,manager,second,looking);
          if(f.greater(w1,epsilon_f)) {
            if(!aim_record_1) {
              if(aim_record_2)aim_sample(second,yaw,pitch,w1);
            } else if(!aim_record_2) {
              if(f.greater(blend_limit_f,w1) || !centered)aim_sample(first,yaw,pitch,w1);
            } else {
              u32 a=f.mul(f.sub(one,w2),w1),b=f.mul(w2,w1);
              if(f.greater(epsilon_f,a)) {
                aim_sample(second,yaw,pitch,one);
              } else {
                bool sample_first;
                if(f.greater(epsilon_f,b)){a=one;b=zero_f;sample_first=!centered;}
                else if(!f.greater(a,epsilon_f))sample_first=false;
                else sample_first=f.greater(blend_limit_f,a) || !centered;
                if(sample_first)aim_sample(first,yaw,pitch,a);
                if(f.greater(b,epsilon_f))aim_sample(second,yaw,pitch,b);
              }
            }
          }
        }
        seconds=allowed?fade_fast_f:fade_slow_f;
      }
    }
    animation_fade_start(m,aim+0x90u,allowed,seconds,f.fp());  // 1E14F0 (track units)
    allowed=0;
    seconds=fade_slow_f;
    if(!(f.r8(body+0x212u)==0xffu && f.r32(body+0x13cu)==none)) {
      const u32 target=f.r32(aim+0x114u);
      if(u16(target>>16)!=0xffffu) {
        manager_start(f,manager,record,target);
        const u32 rec=f.r32(record)!=none && f.r16(record+6u)!=0xffffu?aim_record(m,graph,target):0u;
        if(rec) {
          const auto [d_forward,d_left,d_up]=dots(body+0x18cu);
          const auto angles=x87_vector_angles(d_forward,d_left,d_up,f.x87());
          f.w8(aim,f.r8(aim)|0x40u);
          const u32 dy=f.sub(angles[0],yaw),dp=f.sub(angles[1],pitch);
          ranges(aim+0x14u,rec);
          allowed=unit_animation_allows(m,object,5);
          if(f.r8(aim+0x8du)) {
            const u32 w=interpolation_fraction(f,aim+0x8cu);
            if(f.greater(w,epsilon_f) && (f.greater(blend_limit_f,w) || !tiny(dy) || !tiny(dp)))
              aim_sample(record,dy,dp,w);
          }
          seconds=allowed?fade_fast_f:fade_slow_f;
        }
      }
    }
    animation_fade_start(m,aim+0x8cu,allowed,seconds,f.fp());  // 1E14F0
  }();
  const u32 current=object_body(m,object);
  if(f.r8(current+0x212u)!=0xffu && f.r8(current+0x213u)!=0xffu) {
    for(const auto [at,flags]:{std::array<u32,2>{0x10cu,0x400u},std::array<u32,2>{0x110u,0x800u}}) {
      if(f.r16(aim+at+2u)==0xffffu || f.r32(manager+0x68u)==none)continue;
      if(channel_start(f,record,f.r32(manager+0x68u),flags,f.r32(aim+at)))sample(record,one);
    }
    return;
  }
  if(f.r16(aim+0x10eu)==0xffffu)return;
  u32 flags=0;
  if(f.r8(aim+0x81u) && f.greater(epsilon_f,interpolation_fraction(f,aim+0x80u)))flags=0x400u;
  if(f.r32(manager+0x68u)==none)return;
  if(manager_start(f,manager,record,f.r32(aim+0x10cu),flags))sample(record,one);
}
// 1D61B0: the second channel's blend weight from manager +80.
u32 blend_fraction(Frame& f,u32 manager) {
  u32 x=f.add(f.r32(manager+0x80u),epsilon_f);
  if(f.greater(zero_f,x))x=zero_f;
  else if(f.greater(x,blend_limit_f))x=blend_limit_f;
  return clamp_unit(f,f.mul(x,blend_scale_f));
}
// 1D5840: channel 2 (+40), the channel 1 (+20) blend, else channel 0.
void sample_channels(Frame& f,u32 manager,u32 weight,u32 mask,u32 orientations,u32 count) {
  if(f.r8(0x47c1b5u) && f.r32(manager+0x40u)!=none && f.r16(manager+0x46u)!=0xffffu) {
    sample_channel(f,manager+0x40u,weight,mask,count,orientations);
    if(s32(count)<=0 || (mask && !(f.r8(mask)&1u)))return;
    refuse("1D63C0","animation channel post-process");
  }
  if(f.r8(0x47c1b4u) && f.r32(manager+0x20u)!=none) {
    if(f.r16(manager+0x26u)!=0xffffu && f.at_least(f.r32(manager+0x80u),blend_limit_f)) {
      sample_channel(f,manager+0x20u,weight,mask,count,orientations);
      return;
    }
    if(f.r32(manager+0x20u)!=none && f.r16(manager+0x26u)!=0xffffu && !f.at_least(epsilon_f,f.r32(manager+0x80u))
        && !f.at_least(f.r32(manager+0x80u),blend_limit_f)) {
      const u32 blend=blend_fraction(f,manager);
      sample_channel(f,manager,weight,mask,count,orientations);
      sample_channel(f,manager+0x20u,f.mul(blend,weight),mask,count,orientations);
      return;
    }
  }
  sample_channel(f,manager,weight,mask,count,orientations);
}
// BFB90.
void sample_animation(Frame& f,u32 object,u32 mode,u32 manager,u32 orientations,u32 count,u32 mask) {
  (void)mode;
  u32 nodes=count;
  if(mask) {
    const u32 last=node_mask_last(f.world(),mask,count);
    if(s32(last)>=0 && s32(last+1u)<s32(nodes))nodes=last+1u;
  }
  if(!nodes)return;
  if(manager_valid(f.world(),manager)) {
    (void)animation_lookup(f.world(),tag_data(f.world(),f.r32(manager)),f.r32(manager+4u));
    sample_channels(f,manager,one,mask,orientations,nodes);
  }
  overlays(f,object,manager,mask,nodes,orientations);
  type_orientation_hooks(f,object,mask,nodes,orientations);
  if(f.r8(manager+0x65u) && !(f.r8(manager+0x67u)&2u)) {
    const u32 body=object_body(f.world(),object);
    crossfade(f,manager+0x64u,orientations,sx16(u16(nodes)),mask,sx16(std::uint16_t(f.r16(body+0x10eu)))+body);
  }
}

// ---- Havok-driven nodes (1DBBD0, 1DBFD0, 1D8B90) and crate bounds (1DC7A0) ----
constexpr u32 component_pool=0x52be5cu;   // Havok component data array (rows of 0xA0 at +44)
constexpr u32 havok_world=0x52be48u;
constexpr u32 sphere_pad_f=0x3a83126fu;   // [0x46A180] 0.001f
constexpr u32 broadphase_aabb=0x2eb0c0u;  // hk3AxisSweep (vtable 0x41C6C0) +20 getAabb

// The six literal hkMotion::getType owners (motion vtable +18).
u32 motion_type(Frame& f,u32 motion,const char* owner) {
  const u32 method=f.r32(f.r32(motion)+0x18u);
  switch(method) {
  case 0x31d520u:return 2u;
  case 0x31e2f0u:return 3u;
  case 0x2e8cf0u:return 4u;
  case 0x31dd70u:return 5u;
  case 0x31edf0u:return 6u;
  case 0x31ede0u:return 7u;
  default:refuse(owner,"motion type method "+hex(method));
  }
}
// MOVAPS load: a misaligned operand faults in the original.
V4 load_aligned(Frame& f,u32 at,const char* owner) {
  if(at&15u)refuse(owner,"a misaligned MOVAPS operand (the original faults)");
  return load128(f,at);
}
// 2E4650 -> 2E44D0: a * b over hkTransform rows (rotation columns, then the
// translation, which also adds a's): ((a0*x + a1*y) + a2*z) per row.
std::array<V4,4> transform_product(Frame& f,const std::array<V4,4>& a,u32 b) {
  std::array<V4,4> out;
  for(u32 row=0;row<4;++row) {
    const V4 x=load_aligned(f,b+16u*row,"2E44D0");
    const V4 z=mulps(a[2],broadcast(x[2]),f.mxcsr());
    const V4 y=mulps(a[1],broadcast(x[1]),f.mxcsr());
    out[row]=addps(addps(mulps(a[0],broadcast(x[0]),f.mxcsr()),y,f.mxcsr()),z,f.mxcsr());
  }
  out[3]=addps(out[3],a[3],f.mxcsr());
  return out;
}
// 1D8B90 (ECX component, EAX row, ESI out): the row's motion transform
// (rigid body +3C motion +80), times the component's +30 transform under +4
// bit 6, as a unit-scale node matrix (thirteen scalar stores).
void motion_node_matrix(Frame& f,u32 component,u32 row,u32 out) {
  const u32 rigid_body=f.r32(f.r32(component+0x70u)+row*0x60u+0x40u);
  const u32 transform=f.r32(rigid_body+0x3cu)+0x80u;
  std::array<V4,4> t;
  for(u32 k=0;k<4;++k)t[k]=load_aligned(f,transform+16u*k,"1D8B90");
  if((f.r32(component+4u)>>6)&1u)t=transform_product(f,t,component+0x30u);
  f.w32(out,one);
  for(u32 k=0;k<4;++k)for(u32 lane=0;lane<3;++lane)f.w32(out+4u+12u*k+4u*lane,t[k][lane]);
}
// The dword of `node` in a mask of `words` initialized dwords (BF0C0's mask
// is stack the original zeroes only that far; the delivery mask has 8).
u32 mask_word(u32 mask,u32 node,u32 words,const char* owner) {
  const std::int32_t word=s32(node)>>5;
  if(word<0 || u32(word)>=words)refuse(owner,"a physics node beyond its zeroed node mask");
  return mask+u32(word)*4u;
}
bool mask_test(Frame& f,u32 mask,u32 node,u32 words,const char* owner) {
  return (f.r32(mask_word(mask,node,words,owner))>>(node&31u))&1u;
}
void mask_set(Frame& f,u32 mask,u32 node,u32 words,const char* owner) {
  const u32 at=mask_word(mask,node,words,owner);
  f.w32(at,f.r32(at)|(1u<<(node&31u)));
}
struct PhysicsModel {u32 body,physics,render;};
// The component's object body, its model's physics model (hlmt +24) and
// render model (hlmt +4).
PhysicsModel physics_model(Frame& f,u32 component) {
  const u32 object=f.r32(component+8u);
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+(object&0xffffu)*12u+8u);
  const u32 tags=f.r32(tag_instances);
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  const u32 hlmt=f.r32((f.r32(definition+0x38u)&0xffffu)*16u+tags+8u);
  return {body,f.r32((f.r32(hlmt+0x24u)&0xffffu)*16u+tags+8u),f.r32((f.r32(hlmt+4u)&0xffffu)*16u+tags+8u)};
}
// 1DBFD0 (ECX component; node, mask, delivered, orientations): below
// `node` (physics model +CC rows of 12: +6 parent, +8 sibling, +A child), a
// physics-delivered node (`mask`) publishes its local orientation (144090,
// 145560, 144E60); a node only `delivered` reaches takes the render model's
// default local orientation (+4C rows of 0x60) under its parent's matrix.
void physics_node_children(Frame& f,u32 component,u32 node,u32 mask,u32 words,u32 delivered,u32 orientations,
    unsigned depth) {
  if(depth>256u)refuse("1DBFD0","a physics node tree deeper than 256 levels");
  const PhysicsModel model=physics_model(f,component);
  const u32 entry=f.r32(model.physics+0xccu)+node*12u;
  const u32 parent=f.r16(entry+6u);
  FrameMemory fm(f);
  if(parent!=0xffffu) {
    const u32 bit=1u<<(node&31u);
    if(f.r32(mask_word(mask,node,words,"1DBFD0"))&bit) {
      if(orientations) {
        Locals locals(f);
        const u32 inverse=locals(0x34),local=locals(0x34);
        const u32 nodes=sx16(std::uint16_t(f.r16(model.body+0x116u)))+model.body;
        device_animation::matrix_inverse(fm,inverse,nodes+sx16(std::uint16_t(parent))*0x34u,script_fp(f));
        product(f,inverse,nodes+sx16(std::uint16_t(node))*0x34u,local);
        device_animation::matrix_orientation(fm,orientations+node*32u,local,script_fp(f));
      }
    } else if(f.r32(mask_word(delivered,node,8u,"1DBFD0"))&bit) {
      const u32 record=f.r32(model.render+0x4cu)+node*0x60u;
      const u32 nodes=sx16(std::uint16_t(f.r16(model.body+0x116u)))+model.body;
      const u32 matrix=nodes+sx16(std::uint16_t(node))*0x34u;
      quaternion_rotation(fm,record+0x18u,matrix+4u,f.mxcsr());
      f.w32(matrix+0x28u,0);f.w32(matrix+0x2cu,0);f.w32(matrix+0x30u,0);
      f.w32(matrix,one);
      f.w32(matrix+0x28u,f.r32(record+0xcu));
      f.w32(matrix+0x2cu,f.r32(record+0x10u));
      f.w32(matrix+0x30u,f.r32(record+0x14u));
      product(f,nodes+sx16(std::uint16_t(parent))*0x34u,matrix,matrix);
      mask_set(f,mask,node,words,"1DBFD0");
      if(orientations) {
        const u32 to=orientations+node*32u;
        for(u32 k=0;k<4;++k)f.w32(to+4u*k,f.r32(record+0x18u+4u*k));
        for(u32 k=0;k<3;++k)f.w32(to+0x10u+4u*k,f.r32(record+0xcu+4u*k));
        f.w32(to+0x1cu,one);
      }
    }
  }
  for(u32 child=sx16(std::uint16_t(f.r16(entry+0xau)));child!=none;) {
    const u32 sibling=f.r32(model.physics+0xccu)+child*12u+8u;
    physics_node_children(f,component,child,mask,words,delivered,orientations,depth+1u);
    child=sx16(std::uint16_t(f.r16(sibling)));
  }
}
// 1DBBD0 (component, mask, height): every row's (+70 rows of 0x60: +40
// rigid body, +48 physics-model record indices, +4C count) non-fixed
// records (physics model +3C rows of 0x90, +0 node) whose node is not yet
// in `mask` and not below the row's first record's node: the node matrix
// from the motion (1D8B90), raised by `height`; then the masks and 1DBFD0.
void physics_node_delivery(Frame& f,u32 component,u32 mask,u32 words,u32 height) {
  if((f.r32(component+4u)>>9)&1u)return;
  if(s32(f.r32(component+0x74u))<=0)return;
  const PhysicsModel model=physics_model(f,component);
  Locals locals(f);
  const u32 delivered=locals(0x20);
  for(u32 k=0;k<8;++k)f.w32(delivered+4u*k,0);
  const u32 orientations=f.r16(model.body+0x112u)!=0xffffu?sx16(std::uint16_t(f.r16(model.body+0x112u)))+model.body:0u;
  bool any=false;
  for(u32 r=0;s32(r)<s32(f.r32(component+0x74u));++r) {
    const u32 offset=r*0x60u;
    const u32 root=f.r32(model.physics+0x3cu)+sx8(f.r8(f.r32(f.r32(component+0x70u)+offset+0x48u)))*0x90u;
    for(u32 k=0;s32(k)<s32(f.r32(f.r32(component+0x70u)+offset+0x4cu));++k) {
      const u32 row=f.r32(component+0x70u)+offset;
      const u32 record=f.r32(model.physics+0x3cu)+sx8(f.r8(f.r32(row+0x48u)+k))*0x90u;
      if(f.r16(record)==0xffffu)continue;
      if(motion_type(f,f.r32(f.r32(row+0x40u)+0x3cu),"1DBBD0")==6u)continue;
      const u32 node=sx16(std::uint16_t(f.r16(record)));
      if(mask_test(f,mask,node,words,"1DBBD0"))continue;
      if(node!=none) {
        const u32 root_node=sx16(std::uint16_t(f.r16(root)));
        bool below=false;
        for(u32 at=node,steps=0;;) {
          if(at==root_node){below=f.r16(record)!=std::uint16_t(root_node);break;}
          at=sx16(std::uint16_t(f.r16(f.r32(model.physics+0xccu)+at*12u+6u)));
          if(at==none)break;
          if(++steps>0x10000u)refuse("1DBBD0","a cyclic physics node parent chain (the original never returns)");
        }
        if(below)continue;
      }
      u32 parent=sx16(std::uint16_t(f.r16(f.r32(model.physics+0xccu)+node*12u+6u)));
      const u32 nodes=sx16(std::uint16_t(f.r16(model.body+0x116u)))+model.body;
      const u32 matrix=nodes+node*0x34u;
      motion_node_matrix(f,component,r,matrix);
      f.w32(matrix+0x30u,f.add(f.r32(matrix+0x30u),height));
      if(f.r16(record)!=f.r16(root)) {
        const u32 vtable=f.r32(f.r32(f.r32(f.r32(component+0x70u)+offset+0x40u)+0xcu));
        refuse("1DBBD0","child shape transform method "+hex(f.r32(vtable+0x34u)));
      }
      mask_set(f,mask,sx16(std::uint16_t(f.r16(record))),words,"1DBBD0");
      any=true;
      mask_set(f,delivered,sx16(std::uint16_t(f.r16(record))),8u,"1DBBD0");
      for(u32 steps=0;parent!=none;) {
        mask_set(f,delivered,parent,8u,"1DBBD0");
        parent=sx16(std::uint16_t(f.r16(f.r32(model.physics+0xccu)+parent*12u+6u)));
        if(++steps>0x10000u)refuse("1DBBD0","a cyclic physics node parent chain (the original never returns)");
      }
    }
  }
  if(any && s32(f.r32(model.render+0x48u))>0)
    physics_node_children(f,component,0,mask,words,delivered,orientations,0);
}

// 2EB0C0 -> 2EA050: a broadphase node's AABB endpoints (min x +8, y +0,
// z +2; max x +A, y +4, z +6 into the +4C/+58/+64 axes) dequantized in
// x87 under the live control word, exactly as the original: the x and y
// reciprocals stay on the register stack, z's is stored as a float first.
struct Dequantized {std::array<u32,3> lo,hi;};
Dequantized x87_dequantize(const std::array<std::int32_t,6>& e,const std::array<u32,3>& scale,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};
  const float unit=1.0f,sx=f32(scale[0]),sy=f32(scale[1]),sz=f32(scale[2]);
  float x0{},y0{},z0{},x1{},y1{},z1{},iz{};
  asm volatile(
      "fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[unit]\n\tfdivs %[sx]\n\t"
      "fildl %[xa]\n\tfmul %%st(1), %%st\n\tfstps %[x0]\n\t"
      "flds %[unit]\n\tfdivs %[sy]\n\t"
      "fildl %[ya]\n\tfmul %%st(1), %%st\n\tfstps %[y0]\n\t"
      "flds %[unit]\n\tfdivs %[sz]\n\tfstps %[iz]\n\t"
      "fildl %[za]\n\tfmuls %[iz]\n\tfstps %[z0]\n\t"
      "fildl %[xb]\n\tfmul %%st(2), %%st\n\tfstps %[x1]\n\t"
      "fildl %[yb]\n\tfmul %%st(1), %%st\n\tfstps %[y1]\n\t"
      "fstp %%st(0)\n\tfstp %%st(0)\n\t"
      "fildl %[zb]\n\tfmuls %[iz]\n\tfstps %[z1]\n\t"
      "fldcw %[saved]"
      :[saved]"+m"(saved),[x0]"=m"(x0),[y0]"=m"(y0),[z0]"=m"(z0),[x1]"=m"(x1),[y1]"=m"(y1),[z1]"=m"(z1),[iz]"+m"(iz)
      :[mode]"m"(mode),[unit]"m"(unit),[sx]"m"(sx),[sy]"m"(sy),[sz]"m"(sz),[xa]"m"(e[0]),[ya]"m"(e[1]),[za]"m"(e[2]),
       [xb]"m"(e[3]),[yb]"m"(e[4]),[zb]"m"(e[5])
      :"st","st(1)","st(2)");
  return {{bits(x0),bits(y0),bits(z0)},{bits(x1),bits(y1),bits(z1)}};
#else
  (void)e;(void)scale;(void)control;refuse("2EA050","x87 dequantization on a non-x86 host");
#endif
}
// 1DC7A0 (component, center, radius): a crate's sphere from the union of
// its rows' broadphase AABBs, each padded by its records' largest +80
// radius unless that is exactly 0.0: center 0.5 * (min + max), radius
// sqrt of the squared half extents + 0.001. False (no stores) without
// rows, component +4 bits 5 and 11, or with definition +1C bit 1.
bool crate_sphere(Frame& f,u32 component,u32 center,u32 radius) {
  if(s32(f.r32(component+0x74u))<=0)return false;
  const u32 flags=f.r32(component+4u);
  if(!((flags>>5)&1u) || !((flags>>11)&1u))return false;
  const u32 object=f.r32(component+8u);
  const u32 tags=f.r32(tag_instances);
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+(object&0xffffu)*12u+8u);
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  if(f.r8(definition+0x1cu)&2u)return false;
  const u32 hlmt=f.r32((f.r32(definition+0x38u)&0xffffu)*16u+tags+8u);
  const u32 physics=f.r32((f.r32(hlmt+0x24u)&0xffffu)*16u+tags+8u);
  V4 lo{},hi{};
  for(u32 r=0;s32(r)<s32(f.r32(component+0x74u));++r) {
    const u32 row=f.r32(component+0x70u)+r*0x60u;
    const u32 collidable=f.r32(row+0x40u)+0xcu;
    u32 largest=zero_f;
    for(u32 k=0;s32(k)<s32(f.r32(row+0x4cu));++k) {
      const u32 value=f.r32(f.r32(physics+0x3cu)+sx8(f.r8(f.r32(row+0x48u)+k))*0x90u+0x80u);
      if(!f.greater(largest,value))largest=value;
    }
    const u32 broadphase=f.r32(f.r32(havok_world)+0xc4u);
    const u32 method=f.r32(f.r32(broadphase)+0x20u);
    if(method!=broadphase_aabb)refuse("1DC7A0","broadphase AABB method "+hex(method));
    if(!collidable)refuse("2EB0C0","a null broadphase handle (the original faults)");
    const u32 node=(f.r32(collidable+0x10u)<<4)+f.r32(broadphase+0x40u);
    const u32 ax=f.r32(broadphase+0x4cu),ay=f.r32(broadphase+0x58u),az=f.r32(broadphase+0x64u);
    const auto end=[&](u32 axis,u32 at){return std::int32_t(f.r16(axis+u32(f.r16(node+at))*4u));};
    const std::array<std::int32_t,6> e{end(ax,8u),end(ay,0u),end(az,2u),end(ax,0xau),end(ay,4u),end(az,6u)};
    const V4 offset=load_aligned(f,broadphase+0x10u,"2EA050");
    const auto q=x87_dequantize(e,{f.r32(broadphase+0x30u),f.r32(broadphase+0x34u),f.r32(broadphase+0x38u)},f.x87());
    V4 low=subps({q.lo[0],q.lo[1],q.lo[2],0u},offset,f.mxcsr());
    V4 high=subps({q.hi[0],q.hi[1],q.hi[2],0u},offset,f.mxcsr());
    if(!f.equal(largest,zero_f))
      for(u32 k=0;k<3;++k){low[k]=f.sub(low[k],largest);high[k]=f.add(high[k],largest);}
    if(!r){lo=low;hi=high;}
    else{lo=minps(lo,low,f.mxcsr());hi=maxps(hi,high,f.mxcsr());}
  }
  const V4 sum=addps(lo,hi,f.mxcsr());
  const V4 extent=subps(hi,lo,f.mxcsr());
  V4 squares=mulps(broadcast(half_f),extent,f.mxcsr());
  squares=mulps(squares,squares,f.mxcsr());
  const V4 middle=mulps(broadcast(half_f),sum,f.mxcsr());
  for(u32 k=0;k<3;++k)f.w32(center+4u*k,middle[k]);
  const u32 total=f.add(squares[2],f.add(squares[1],squares[0]));
  f.w32(radius,f.add(sqrtss(total,f.mxcsr()),sphere_pad_f));
  return true;
}

// BFF20.
bool update_bounds(Frame& f,u32 object) {
  M& m=f.world();
  const u32 body=object_body(m,object);
  const u32 definition=tag_data(m,f.r32(body));
  const bool connected=(f.r32(body+4u)>>8)&1u;
  const u32 physics=f.r32(body+0xb4u);
  bool physical=false;
  if(physics!=none && ((1u<<(f.r8(body+0xaau)&31u))&0x800u) && ((f.r8(body+0xc0u)>>6)&1u))
    physical=crate_sphere(f,f.r32(f.r32(component_pool)+0x44u)+(physics&0xffffu)*0xa0u,body+0x30u,body+0x3cu);
  if(!physical) {
    const u32 nodes=sx16(std::uint16_t(f.r16(body+0x116u)))+body;
    u32 ox=f.r32(definition+8u),oy=f.r32(definition+0xcu),oz=f.r32(definition+0x10u);
    if(!f.equal(f.r32(nodes),one)){ox=f.mul(f.r32(nodes),ox);oy=f.mul(f.r32(nodes),oy);oz=f.mul(f.r32(nodes),oz);}
    for(u32 k=0;k<3;++k) {
      const u32 v=f.add(f.add(f.add(f.mul(f.r32(nodes+0x1cu+4u*k),oz),f.mul(f.r32(nodes+0x10u+4u*k),oy)),
          f.mul(f.r32(nodes+4u+4u*k),ox)),f.r32(nodes+0x28u+4u*k));
      f.w32(body+0x30u+4u*k,v);
    }
    f.w32(body+0x3cu,f.mul(f.r32(body+0xa0u),f.r32(definition+4u)));
  }
  if(f.greater(f.r32(definition+0x20u),zero_f)) {
    for(u32 k=0;k<3;++k)f.w32(body+0x50u+4u*k,f.add(f.r32(body+0x30u+4u*k),f.r32(definition+0x24u+4u*k)));
    f.w32(body+0x5cu,f.mul(f.r32(body+0xa0u),f.r32(definition+0x20u)));
  } else {
    f.w32(body+0x50u,f.r32(body+0x30u));
    const u32 y=f.r32(body+0x34u),z=f.r32(body+0x38u);
    f.w32(body+0x54u,y);f.w32(body+0x58u,z);
    f.w32(body+0x5cu,f.r32(body+0x3cu));
  }
  const u32 sphere=(f.r16(definition+2u)>>12)&1u?body+0x50u:body+0x30u;
  const u32 cx=f.r32(sphere),cy=f.r32(sphere+4u),cz=f.r32(sphere+8u),radius=f.r32(sphere+0xcu);
  if(connected) {
    const u32 dz=f.sub(cz,f.r32(body+0x48u)),dx=f.sub(cx,f.r32(body+0x40u)),dy=f.sub(cy,f.r32(body+0x44u));
    const u32 distance=f.add(f.add(f.mul(dz,dz),f.mul(dx,dx)),f.mul(dy,dy));
    if(!f.greater(distance,sphere_moved_f) && f.equal(f.r32(body+0x4cu),radius))return false;
    object_disconnect_from_map(m,object,f.fp());  // BA7E0 (track placement)
  }
  f.w32(body+0x40u,cx);f.w32(body+0x44u,cy);f.w32(body+0x48u,cz);f.w32(body+0x4cu,radius);
  if(connected)object_reconnect_to_map(m,object,0,f.fp());  // BA630 (track placement)
  return true;
}

// BF0C0.
void update_node_matrices(Frame& f,u32 object) {
  M& m=f.world();
  const u32 tags=f.r32(tag_instances);
  const u32 row=(object&0xffffu)*12u;
  const u32 body=f.r32(f.r32(f.r32(object_pool)+0x44u)+row+8u);
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  const u32 count=u32((std::uint64_t(0x4ec4ec4fu)*sx16(std::uint16_t(f.r16(body+0x114u))))>>32)>>4;
  const u32 nodes=sx16(std::uint16_t(f.r16(body+0x116u)))+body;
  f.w32(body+4u,f.r32(body+4u)&0xbfffffffu);
  bool own_orientations=true;
  const bool biped=((1u<<(f.r8(definition)&31u))&1u)!=0;
  if(biped && f.r8(object_body(m,object)+0x34bu)==2u)return;
  // L_70: the physics node mask, (count+31)>>5 dwords zeroed.
  const u32 mask_words=u32(s32(count+0x1fu)>>5);
  if(mask_words>8u)refuse("BF0C0","node mask beyond 256 nodes");
  Locals frame_locals(f);
  const u32 mask=frame_locals(0x20);  // the original's stack mask: only mask_words dwords are zeroed
  for(u32 k=0;k<mask_words;++k)f.w32(mask+4u*k,0);
  const u32 physics=f.r32(body+0xb4u);
  if(physics!=none && ((f.r8(body+0xc0u)>>6)&1u)) {
    const u32 component=f.r32(f.r32(component_pool)+0x44u)+(physics&0xffffu)*0xa0u;
    physics_node_delivery(f,component,mask,mask_words,biped?physics_height(f,object):zero_f);  // 1DBBD0
  }
  const u32 current=object_body(m,object);
  u32 orientations=0;
  if(f.r16(current+0x112u)!=0xffffu)orientations=sx16(std::uint16_t(f.r16(body+0x112u)))+body;
  else {orientations=orientation_scratch;own_orientations=false;}
  const u32 model_tag=f.r32(definition+0x38u);
  const u32 hlmt=model_tag==none?0:f.r32((model_tag&0xffffu)*16u+tags+8u);
  const u32 render_tag=model_tag==none?none:f.r32(hlmt+4u);
  if(render_tag==none) {
    // BF5C3: node 0 from the body placement and the parent node.
    const u32 parent=f.r32(body+0x14u);
    u32 parent_matrix=0;bool parent_mirrored=false;
    if(parent!=none) {
      const u32 parent_body=f.r32(f.r32(f.r32(object_pool)+0x44u)+(parent&0xffffu)*12u+8u);
      parent_matrix=sx16(std::uint16_t(f.r16(parent_body+0x116u)))+sx8(std::uint8_t(f.r8(body+0x18u)))*0x34u+parent_body;
      parent_mirrored=(f.r32(parent_body+4u)>>10)&1u;
    }
    root_matrix(f,body+0x7cu,body+0x70u,parent_mirrored,body+0x64u,f.r32(body+0xa0u),
        (f.r32(body+4u)>>10)&0xffffff01u,parent_matrix,nodes);
    return;
  }
  const u32 mode=f.r32((render_tag&0xffffu)*16u+tags+8u);
  const u32 parent=f.r32(body+0x14u);
  u32 parent_matrix=0;bool parent_mirrored=false,world_relative=false;
  if(parent!=none) {
    parent_matrix=object_node_matrix(m,parent,sx8(std::uint8_t(f.r8(body+0x18u))));
    parent_mirrored=(f.r32(object_body(m,parent)+4u)>>10)&1u;
  }
  if(!((f.r32(current+4u)>>29)&1u) && !(biped && f.r8(current+0x34bu))) {
    default_orientations(f,mode,orientations);
    if(object_has_animation(m,object) && !object_animation_frozen(m,object)) {
      const u32 manager=sx16(std::uint16_t(f.r16(body+0x12au)))+body;
      if(f.r32(manager+0x68u)!=none) {
        if(!(f.r8(render_only_flag)&1u))f.w32(render_only_flag,f.r32(render_only_flag)|1u);
        u32 animation_mask=0;
        if(own_orientations && !node_mask_empty(m,hlmt+0xa4u)) {
          copy_words(f,render_only_mask,hlmt+0xa4u,8);
          node_mask_invert(m,render_only_mask);
          node_mask_limit(m,render_only_mask,count);
          animation_mask=render_only_mask;
        }
        sample_animation(f,object,mode,manager,orientations,count,animation_mask);
        flags_assign_bit(m,body+4u,30,animation_mask!=0);
        if(f.r32(manager+0x68u)!=none && manager_valid(m,manager))
          world_relative=(f.r8(manager_animation(m,manager)+0x16u)>>1)&1u;
      }
    }
  }
  const auto mask_bit=[&](u32 node)->bool {
    const u32 word=u32(s32(node)>>5);
    if(word>=mask_words)refuse("BF0C0","model node beyond the zeroed node mask");
    return (f.r32(mask+word*4u)>>(node&31u))&1u;
  };
  if(!f.r32(mode+0x48u))copy_words(f,orientations,f.r32(identity_orientation),8);
  if(!mask_bit(0)) {
    if(world_relative) {
      orientation(f,nodes,orientations);
    } else {
      Locals locals(f);
      const u32 up=locals(12),position=locals(12),forward=locals(12),root=locals(0x34),local=locals(0x34);
      for(u32 k=0;k<3;++k){f.w32(position+4u*k,f.r32(body+0x64u+4u*k));}
      for(u32 k=0;k<3;++k){f.w32(forward+4u*k,f.r32(body+0x70u+4u*k));f.w32(up+4u*k,f.r32(body+0x7cu+4u*k));}
      if(f.r32(body+0xd4u)!=none && (f.r8(body+0xd8u)&1u))refuse("B3AE0","network position prediction");
      root_matrix(f,up,forward,parent_mirrored,position,f.r32(body+0xa0u),(f.r32(body+4u)>>10)&0xffffff01u,
          parent_matrix,root);
      orientation(f,local,orientations);
      product(f,root,local,nodes);
    }
    type_node_hooks(f,object,nodes);
  }
  if(s32(f.r32(mode+0x48u))<=0)return;
  for(u32 node=1;s32(node)<s32(f.r32(mode+0x48u));++node) {
    const u32 record=f.r32(mode+0x4cu)+node*0x60u;
    if(mask_bit(node))continue;
    const u32 matrix=nodes+node*0x34u;
    orientation(f,matrix,orientations+node*0x20u);
    product(f,sx16(std::uint16_t(f.r16(record+4u)))*0x34u+nodes,matrix,matrix);
  }
}

void rebuild(Frame& f,u32 object) {
  M& m=f.world();
  const u32 body=object_body(m,object);
  update_node_matrices(f,object);
  update_bounds(f,object);
  for(u32 child=f.r32(body+0x10u);child!=none;) {
    const u32 child_body=object_body(m,child);
    if(!((1u<<(f.r8(child_body+0xaau)&31u))&0x80u))rebuild(f,child);
    child=f.r32(child_body+0xcu);
  }
}

// ---- BF640 deferred node matrices (render presentation) ----
constexpr u32 deferred_matrices=0x4e57e8u;  // BF640's 255-matrix scratch (before 0x4E8BC0)
constexpr u32 third_f=0x3eaaaaabu,quarter_f=0x3e800000u;  // [0x46A174] [0x46A130]
constexpr u32 seventeen_f=0x41880000u;                    // 110F55's immediate 17.0f
constexpr u32 ik_yaw_f=0x3f060a92u,ik_pitch_f=0x3e860a92u;  // [0x46A1E4] pi/6, [0x46A1EC] pi/12
constexpr u32 overlay_byte_scale=0x5564dcu,overlay_signed_scale=0x5564d8u;  // .bss scales (startup)
constexpr u32 hand_ik_enabled=0x474c04u,player_cameras_live=0x474c24u;
constexpr u32 player_pool=0x4f54f0u,player_cameras=0x4f655cu;
constexpr u32 hand_ik_flags=0x56e8e4u,hand_ik_yaw=0x56e8e0u,hand_ik_pitch=0x56e8dcu;
u32 unit_aim_block(Frame& f,u32 object) {
  const u32 body=object_body(f.world(),object);
  return sx16(u16(f.r16(body+0x33eu)))+body;
}
// B8780 (ECX record): the channel's animation (1E3150), 0 without one.
u32 record_animation(Frame& f,u32 record) {
  if(f.r16(record+6u)==0xffffu)return 0;
  return animation_lookup(f.world(),tag_data(f.world(),f.r32(record)),f.r32(record+4u));
}
// 110240 (EAX unit): the aim block's +36 handle is set and either its +40
// blend is at least 1e-4 or its +2 state is 1..6.
bool unit_aim_overlay_live(Frame& f,u32 object) {
  const u32 aim=unit_aim_block(f,object);
  const bool blended=f.at_least(f.r32(aim+0x40u),epsilon_f);
  const u32 state=f.r8(aim+2u);
  const bool stepping=state!=0u && state<7u;
  return f.r16(aim+0x36u)!=0xffffu && (blended || stepping);
}
// 1114B0 (EAX unit): the +5C overlay handle is set and one of the 13
// frame weights +6E..+7A is nonzero.
bool unit_frame_overlay_live(Frame& f,u32 object) {
  const u32 aim=unit_aim_block(f,object);
  if(u16(f.r32(aim+0x5cu)>>16)==0xffffu)return false;
  for(u32 n=0;n<13u;++n)if(f.r8(aim+0x6eu+n))return true;
  return false;
}
// 2130C0 (block, mask): OR the decoded animation's node flag streams into
// the byte mask (header animation +3C: +0 static bytes, +1 animated bytes,
// +6 offset, +C data): each mask byte takes the rotation, translation and
// scale streams' bytes in turn (three byte stores), first for the animated
// block, then (with static bytes and a nonzero +6) the static one.
void animation_node_flags(Frame& f,u32 block,u32 mask) {
  const auto merge=[&](u32 base,std::int32_t total) {
    const std::int32_t n=total/3;
    if(n>32)refuse("2130C0","node flag streams beyond the caller's 256-node mask (the original overwrites its frame)");
    for(std::int32_t k=0;k<n;++k) {
      const u32 at=mask+u32(k);
      u32 value=f.r8(at)|f.r8(base+u32(k));
      f.w8(at,value);
      value|=f.r8(base+u32(n+k));
      f.w8(at,value);
      value|=f.r8(base+u32(2*n+k));
      f.w8(at,value);
    }
  };
  u32 header=f.r32(block+4u);
  if(f.r8(header+1u))
    merge(sx16(u16(f.r16(header+6u)))+f.r32(header+0xcu)+sx8(std::uint8_t(f.r8(header)))+f.r32(block),
        std::int32_t(sx8(std::uint8_t(f.r8(header+1u)))));
  header=f.r32(block+4u);
  if(f.r8(header) && f.r16(header+6u))
    merge(sx16(u16(f.r16(header+6u)))+f.r32(header+0xcu)+f.r32(block),std::int32_t(sx8(std::uint8_t(f.r8(header)))));
}
// The private channel 1112E0/110C10 build inline (1CE5D0's values).
void overlay_record(Frame& f,u32 r) {
  f.w32(r+0x1cu,zero_f);f.w32(r,none);f.w16(r+4u,0xffffu);f.w16(r+6u,0xffffu);f.w8(r+0x10u,0);f.w8(r+0x11u,0);
  f.w16(r+0x12u,0);f.w32(r+0x18u,one);f.w16(r+0x14u,0);f.w16(r+0x16u,0);f.w32(r+8u,none);f.w16(r+0xeu,0xffffu);
  f.w8(r+0xcu,0xffu);f.w8(r+0xdu,0xffu);
}
// The sampled channel's resource record (1E5DF0, cleared first) and its
// node flags into `flags` (2130C0).
void overlay_flags(Frame& f,u32 record,u32 flags) {
  Locals locals(f);
  const u32 block=locals(12);
  f.w32(block,0);f.w32(block+4u,0);f.w8(block+8u,0);f.w8(block+9u,0);
  resource_block(f,block,tag_data(f.world(),f.r32(record)),f.r32(record+4u));
  animation_node_flags(f,block,flags);
}
void set_record_frame(Frame& f,u32 record,u32 frame) {
  FrameMemory memory(f);
  weapon_animation::channel_set_frame(memory,record,frame,script_fp(f));
}
// FILD the byte weight; FMUL [0x5564DC]; FSTP dword under the live control
// word (1112E0).
u32 x87_byte_weight(u32 value,u32 scale,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float result{};
  const std::int32_t integer=std::int32_t(value);const float factor=f32(scale);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(result),"+m"(saved):"m"(mode),"m"(integer),"m"(factor):"st");
  return bits(result);
#else
  (void)value;(void)scale;(void)control;refuse("1112E0","x87 overlay weight on a non-x86 host");
#endif
}
// 1112E0 (EAX unit; manager, render, sample mask, count, orientations,
// flags): the +5C frame overlay on a private channel (1CE720, start flags
// 0x40); each of the 13 frames 2n + [aim +40] inside the animation with a
// nonzero +6E byte samples there (1CE810, 1CF210) at byte * [0x5564DC];
// a sampled overlay ORs its node flags into `flags`.
void unit_frame_overlays(Frame& f,u32 object,u32 manager,u32 sample_mask,u32 count,u32 orientations,u32 flags) {
  const u32 aim=unit_aim_block(f,object);
  const u32 handle=f.r32(aim+0x5cu);
  if(u16(handle>>16)==0xffffu)return;
  Locals locals(f);
  const u32 record=locals(0x20);
  overlay_record(f,record);
  const u32 graph=f.r32(manager+0x68u);
  if(graph==none || !channel_start(f,record,graph,0x40u,handle))return;
  u32 animation=0;
  if(f.r16(record+6u)!=0xffffu)animation=animation_lookup(f.world(),tag_data(f.world(),f.r32(record)),f.r32(record+4u));
  bool sampled=false;
  for(u32 n=0;n<13u;++n) {
    const u32 frame=f.add(f.mul(f.from_int(n),two_f),f.r32(aim+0x40u));
    if(!f.at_least(frame,zero_f))continue;
    if(!animation)refuse("1112E0","frame overlay without an animation (the original faults)");
    if(!f.greater(f.from_int(sx16(u16(f.r16(animation+0x14u)))),frame))continue;
    const u32 level=f.r8(aim+0x6eu+n);
    if(!level)continue;
    const u32 weight=x87_byte_weight(level,f.r32(overlay_byte_scale),f.x87());
    set_record_frame(f,record,frame);
    sample_channel(f,record,weight,sample_mask,count,orientations);
    sampled=true;
  }
  if(sampled)overlay_flags(f,record,flags);
}
// FLD; FABS; FLD 1e-4; FCOMIP; JA: 1e-4 > |x| (exact; unordered false).
bool x87_below_epsilon(u32 x) {
  if((x&0x7fffffffu)>0x7f800000u)return false;
  return (x&0x7fffffffu)<epsilon_f;
}
// 110C10 (EAX unit; manager, render, sample mask, count, orientations,
// flags): the aim block's overlays. With its +40 blend above 1e-4: +30 at
// frame 0 and that weight, then +34's 12 rows (frame +44 byte, weight +50
// byte * [0x5564D8] * blend, skipped below 1e-4 or past the animation);
// with a zero blend and the +2 state 1..6, +34 through the manager
// (1D3740) at frame 17 and state / 3 (rising) or (7 - state) / 4 (falling)
// when the animation has more than 17 frames; any other blend only
// requests +30/+34 (1E5C80). A sampled overlay ORs its node flags.
void unit_aim_overlays(Frame& f,u32 object,u32 manager,u32 sample_mask,u32 count,u32 orientations,u32 flags) {
  M& m=f.world();
  const u32 aim=unit_aim_block(f,object);
  Locals locals(f);
  const u32 record=locals(0x20);
  if(f.greater(f.r32(aim+0x40u),epsilon_f)) {
    overlay_record(f,record);
    const u32 first=f.r32(aim+0x30u);
    if(u16(first>>16)!=0xffffu && f.r32(manager+0x68u)!=none &&
        channel_start(f,record,f.r32(manager+0x68u),0,first)) {
      set_record_frame(f,record,zero_f);
      sample_channel(f,record,f.r32(aim+0x40u),sample_mask,count,orientations);
      overlay_flags(f,record,flags);
    }
    const u32 second=f.r32(aim+0x34u);
    if(u16(second>>16)==0xffffu || f.r32(manager+0x68u)==none ||
        !channel_start(f,record,f.r32(manager+0x68u),0,second))return;
    const u32 animation=record_animation(f,record);
    if(!animation)refuse("110C10","aim overlay without an animation (the original faults)");
    bool sampled=false;
    for(u32 n=0;n<12u;++n) {
      const u32 level=sx8(std::uint8_t(f.r8(aim+0x50u+n)));
      const u32 frame=sx8(std::uint8_t(f.r8(aim+0x44u+n)));
      const u32 weight=f.mul(f.mul(f.from_int(level),f.r32(overlay_signed_scale)),f.r32(aim+0x40u));
      if(s32(frame)>=s32(sx16(u16(f.r16(animation+0x14u)))))continue;
      if(x87_below_epsilon(weight))continue;
      set_record_frame(f,record,f.from_int(frame));
      sample_channel(f,record,weight,sample_mask,count,orientations);
      sampled=true;
    }
    if(sampled)overlay_flags(f,record,flags);
    return;
  }
  if(!f.equal(f.r32(aim+0x40u),zero_f)) {
    if(f.r16(aim+0x32u)!=0xffffu && f.r32(manager+0x68u)!=none)
      animation_request(m,tag_data(m,f.r32(manager+0x68u)),f.r32(aim+0x30u));
    if(f.r16(aim+0x36u)!=0xffffu && f.r32(manager+0x68u)!=none)
      animation_request(m,tag_data(m,f.r32(manager+0x68u)),f.r32(aim+0x34u));
    return;
  }
  if(f.r16(aim+0x36u)==0xffffu)return;
  const u32 state=f.r8(aim+2u);
  if(!state || state>=7u)return;
  channel_clear(f,record);
  if(!manager_start(f,manager,record,f.r32(aim+0x34u)))return;
  const u32 animation=record_animation(f,record);
  if(!animation)refuse("110C10","stepped overlay without an animation (the original faults)");
  if(s32(sx16(u16(f.r16(animation+0x14u))))<=0x11)return;
  const u32 phase=f.r8(aim+2u);
  const u32 weight=phase<3u?f.mul(f.from_int(phase),third_f):f.mul(f.from_int(7u-phase),quarter_f);
  set_record_frame(f,record,seventeen_f);
  sample_channel(f,record,weight,sample_mask,count,orientations);
  overlay_flags(f,record,flags);  // 1CF630: 1E5DF0 of the channel's graph and handle
}
// 115940 (EAX unit): hand IK applies: no +BC bit 11 overlays flag, an
// animated manager, [0x474C04], a first-person or camera-held unit (+148
// bit 15, +137 bit 0) and a hand node (+1F6/+1F7).
bool unit_hand_ik_live(Frame& f,u32 object) {
  const u32 body=object_body(f.world(),object);
  const u32 manager=sx16(u16(f.r16(body+0x12au)))+body;
  if((f.r32(tag_data(f.world(),f.r32(body))+0xbcu)>>11)&1u)return false;
  if(f.r32(manager+0x68u)==none || f.r32(manager)==none || f.r16(manager+6u)==0xffffu)return false;
  if(!f.r8(hand_ik_enabled))return false;
  if(!(f.r8(body+0x149u)&0x80u) && !(f.r8(body+0x137u)&1u))return false;
  return f.r8(body+0x1f6u)!=0xffu || f.r8(body+0x1f7u)!=0xffu;
}
// 1447E0 (EAX out; a, b, c): rotation matrix of the angles (FCOS/FSIN of
// each under the live control word, FSTP dword); out +4 = cos a * cos b
// and out +10 = -(sin a * cos b) in x87, the other terms in SSE.
void euler_matrix(Frame& f,u32 out,u32 a,u32 b,u32 c) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto mode=sse::x87_control(f.x87());std::uint16_t saved{};
  float cc{},sc{},cb{},sb{},ca{},sa{},cacb{},sacb{};
  asm volatile("fnstcw %[saved]\n\tfldcw %[mode]\n\t"
      "flds %[c]\n\tfcos\n\tfstps %[cc]\n\tflds %[c]\n\tfsin\n\tfstps %[sc]\n\t"
      "flds %[b]\n\tfcos\n\tfstps %[cb]\n\tflds %[b]\n\tfsin\n\tfstps %[sb]\n\t"
      "flds %[a]\n\tfcos\n\tfstps %[ca]\n\tflds %[a]\n\tfsin\n\tfstps %[sa]\n\t"
      "flds %[ca]\n\tfmuls %[cb]\n\tfstps %[cacb]\n\tflds %[sa]\n\tfmuls %[cb]\n\tfchs\n\tfstps %[sacb]\n\t"
      "fldcw %[saved]"
      :[cc]"=m"(cc),[sc]"=m"(sc),[cb]"=m"(cb),[sb]"=m"(sb),[ca]"=m"(ca),[sa]"=m"(sa),[cacb]"=m"(cacb),
       [sacb]"=m"(sacb),[saved]"+m"(saved)
      :[mode]"m"(mode),[a]"m"(a),[b]"m"(b),[c]"m"(c):"st");
  const u32 CC=bits(cc),SC=bits(sc),CB=bits(cb),SB=bits(sb),CA=bits(ca),SA=bits(sa);
  const u32 sbsc=f.mul(SB,SC),sbcc=f.mul(SB,CC);
  const u32 x7=f.mul(sbsc,CA);
  f.w32(out,one);
  f.w32(out+8u,f.sub(f.mul(SA,CC),x7));
  f.w32(out+4u,bits(cacb));
  f.w32(out+0xcu,f.add(f.mul(sbcc,CA),f.mul(SA,SC)));
  f.w32(out+0x10u,bits(sacb));
  const u32 x2=f.sub(f.mul(CA,SC),f.mul(sbcc,SA));
  f.w32(out+0x14u,f.add(f.mul(sbsc,SA),f.mul(CA,CC)));
  f.w32(out+0x1cu,f.sub(zero_f,SB));
  f.w32(out+0x18u,x2);
  f.w32(out+0x20u,f.sub(zero_f,f.mul(CB,SC)));
  f.w32(out+0x24u,f.mul(CB,CC));
  f.w32(out+0x28u,zero_f);f.w32(out+0x2cu,zero_f);f.w32(out+0x30u,zero_f);
#else
  (void)f;(void)out;(void)a;(void)b;(void)c;refuse("1447E0","x87 FSIN/FCOS on a non-x86 host");
#endif
}
// 115500 (ECX marker matrix, EBX target, EDI node matrix): turn the node
// toward the target: the target in the basis of the node position and
// the marker's forward/up (144BF0, track deviceanim), unscaled, unit
// (RSQRTSS), its yaw and pitch (x87) clamped to +-[0x56E8E0] (pi/6) and
// +-[0x56E8DC] (pi/12), lazily published from .rdata; node = basis *
// rotation (1447E0, 145560), keeping the node's scale.
void hand_ik(Frame& f,u32 marker,u32 target,u32 node) {
  u32 flags=f.r32(hand_ik_flags);
  u32 yaw_limit,pitch_limit;
  if(!(flags&1u)){flags|=1u;f.w32(hand_ik_flags,flags);yaw_limit=ik_yaw_f;f.w32(hand_ik_yaw,yaw_limit);}
  else yaw_limit=f.r32(hand_ik_yaw);
  if(!(flags&2u)){flags|=2u;f.w32(hand_ik_flags,flags);pitch_limit=ik_pitch_f;f.w32(hand_ik_pitch,pitch_limit);}
  else pitch_limit=f.r32(hand_ik_pitch);
  const u32 scale=f.r32(node);
  Locals locals(f);
  const u32 basis=locals(0x34),rotation=locals(0x34);
  {
    FrameMemory memory(f);
    device_animation::basis_matrix(memory,basis,node+0x28u,marker+4u,marker+0x1cu,script_fp(f));
  }
  u32 x=zero_f,y=zero_f,z=zero_f;
  const u32 b0=f.r32(basis);
  if(!f.equal(b0,zero_f)) {
    u32 d0=f.sub(f.r32(target),f.r32(basis+0x28u));
    u32 d1=f.sub(f.r32(target+4u),f.r32(basis+0x2cu));
    u32 d2=f.sub(f.r32(target+8u),f.r32(basis+0x30u));
    if(!f.equal(b0,one)) {
      const u32 s=f.div(one,b0);
      d0=f.mul(s,d0);d1=f.mul(s,d1);d2=f.mul(s,d2);
    }
    const auto at=[&](u32 k){return f.r32(basis+k);};
    x=f.add(f.add(f.mul(at(0xcu),d2),f.mul(at(8u),d1)),f.mul(at(4u),d0));
    y=f.add(f.add(f.mul(at(0x18u),d2),f.mul(at(0x14u),d1)),f.mul(at(0x10u),d0));
    z=f.add(f.add(f.mul(at(0x24u),d2),f.mul(at(0x20u),d1)),f.mul(at(0x1cu),d0));
    pitch_limit=f.r32(hand_ik_pitch);
  }
  const u32 n2=f.add(f.add(f.mul(z,z),f.mul(y,y)),f.mul(x,x));
  if(!f.equal(n2,zero_f)) {
    const u32 r=rsqrtss(n2,f.mxcsr());
    x=f.mul(r,x);y=f.mul(y,r);z=f.mul(z,r);
  }
  const auto [yaw_angle,pitch_angle]=x87_aim_angles(x,y,z,f.x87());
  u32 yaw=yaw_angle;
  const u32 low_yaw=f.sub(zero_f,yaw_limit);
  if(f.greater(low_yaw,yaw))yaw=low_yaw;
  else if(f.greater(yaw,yaw_limit))yaw=yaw_limit;
  u32 pitch=pitch_angle;
  const u32 low_pitch=f.sub(zero_f,pitch_limit);
  if(f.greater(low_pitch,pitch))pitch=low_pitch;
  else if(f.greater(pitch,pitch_limit))pitch=pitch_limit;
  euler_matrix(f,rotation,yaw,pitch,zero_f);
  product(f,basis,rotation,node);
  f.w32(node,scale);
}
// 1159D0 (EAX unit; mask, count, matrices): with hand IK live, the target
// is the unit's +1A4 (first person, +148 bit 15) or the first local
// player's camera (+137 bit 0: 150900, 172200); with the 0x4000095 marker
// (BAD60, one entry) each hand node (+1F6, +1F7) below `count` and in
// `mask` (C1E90) turns toward it (115500 on the marker's world matrix).
void unit_hand_ik(Frame& f,u32 object,u32 mask,u32 count,u32 matrices) {
  if(!unit_hand_ik_live(f,object))return;
  const u32 body=object_body(f.world(),object);
  u32 target=0;
  if(f.r8(body+0x149u)&0x80u)target=body+0x1a4u;
  else {
    if(!(f.r8(body+0x137u)&1u))return;
    const u32 players=f.r32(player_pool);
    u32 player=none;
    for(u32 n=0;n<4u;++n)if(f.r32(players+0xcu+4u*n)!=none){player=n;break;}
    if(player==none)return;
    if(f.r16(player_cameras_live)==0xffffu)target=0;
    else target=player_cameras+player*0x358u;
  }
  if(!target)return;
  Locals locals(f);
  const u32 marker=locals(0x70);
  std::uint32_t found=0;
  {
    FrameMemory memory(f);
    found=object_marker_matrices(memory,object,0x4000095u,1,false,marker,f.mxcsr());
  }
  if(std::int16_t(found)<=0)return;
  for(const u32 at:{0x1f6u,0x1f7u}) {
    const u32 hand=f.r8(body+at);
    if(hand&0x80u)continue;
    if(s32(hand)>=s32(count))continue;
    if(mask && !node_bit(f,mask,hand))continue;
    hand_ik(f,marker+0x38u,target,matrices+hand*0x34u);
  }
}
// BF640 (object, selected sections, matrices): the render presentation's
// node matrices. Unless the object has no orientations (+112), is frozen
// (+B3) or has no model: a deferred (+4 bit 30), unparented-animation
// (+4 bit 29 clear) object whose selected sections touch the model's +C4
// mask resamples its animation (BFB90 with the model's +A4 mask, clearing
// bit 30); a unit with live aim (110240) or frame (1114B0) overlays copies
// its orientations to 0x4E8BC0 and matrices to 0x4E57E8 and layers them
// (1112E0, 110C10) under a private copy of the +A4 mask; then the masked
// child nodes rebuild (144CF0, 145560) and a unit's hands take IK
// (1159D0). Returns the matrices used (`matrices` or 0x4E57E8).
u32 deferred_node_matrices(Frame& f,u32 object,u32 selected,u32 matrices) {
  M& m=f.world();
  const u32 row=f.r32(f.r32(object_pool)+0x44u)+(object&0xffffu)*12u;
  if(f.r16(f.r32(row+8u)+0x112u)==0xffffu)return matrices;
  if(object!=none && f.r8(f.r32(row+8u)+0xb3u))return matrices;
  const u32 tags=f.r32(tag_instances);
  const u32 body=f.r32(row+8u);
  const u32 definition=f.r32((f.r32(body)&0xffffu)*16u+tags+8u);
  const u32 model_tag=f.r32(definition+0x38u);
  if(model_tag==none)return matrices;
  const u32 manager=sx16(u16(f.r16(body+0x12au)))+body;
  const u32 blocks=sx16(u16(f.r16(body+0x110u)));
  const u32 model=f.r32((model_tag&0xffffu)*16u+tags+8u);
  const u32 render=f.r32((f.r32(model+4u)&0xffffu)*16u+tags+8u);
  const u32 orientations=sx16(u16(f.r16(body+0x112u)))+body;
  if(s32(blocks)<0)refuse("BF640","negative orientation block size (the original's node loops run off the image)");
  const u32 count=blocks>>5;
  if(count>256u)refuse("BF640","orientation block beyond the 256-node mask");
  Locals locals(f);
  const u32 mask=locals(0x20);
  copy_words(f,mask,model+0xa4u,8);
  u32 result=matrices,source=0,sampled=0,ik_matrices=matrices;
  if((f.r32(body+4u)>>30)&1u) {
    const u32 current=f.r32(row+8u);
    const bool biped=((1u<<(f.r8(definition)&31u))&1u)!=0;
    if(!((f.r32(current+4u)>>29)&1u) && !(biped && f.r8(current+0x34bu))) {
      bool touched=false;
      const u32 sections=f.r32(render+0x1cu);
      for(u32 n=0;s32(n)<s32(sections) && !touched;++n) {
        const u32 section=f.r8(selected+n);
        if(section!=0xffu && node_bit(f,model+0xc4u,section))touched=true;
      }
      if(touched) {
        f.w32(body+4u,f.r32(body+4u)&0xbfffffffu);
        sample_animation(f,object,render,manager,orientations,count,model+0xa4u);
        source=orientations;sampled=matrices;ik_matrices=matrices;
      }
    }
  }
  const bool unit=((1u<<(f.r8(row+3u)&31u))&3u)!=0;
  if(unit && (unit_aim_overlay_live(f,object) || unit_frame_overlay_live(f,object))) {
    copy_words(f,orientation_scratch,orientations,count*8u);
    copy_words(f,deferred_matrices,matrices,count*13u);
    unit_frame_overlays(f,object,manager,0,count,orientation_scratch,mask);
    unit_aim_overlays(f,object,manager,0,count,orientation_scratch,mask);
    sampled=deferred_matrices;ik_matrices=deferred_matrices;source=orientation_scratch;
  } else if(!sampled || !source) {
    if(ik_matrices && unit)unit_hand_ik(f,object,model+0xa4u,count,ik_matrices);
    return result;
  }
  for(u32 n=1;s32(n)<s32(count);++n) {
    if(!node_bit(f,mask,n))continue;
    const u32 out=sampled+n*0x34u;
    orientation(f,out,source+n*0x20u);
    product(f,sx16(u16(f.r16(f.r32(render+0x4cu)+n*0x60u+4u)))*0x34u+sampled,out,out);
  }
  result=sampled;
  if(ik_matrices && unit)unit_hand_ik(f,object,model+0xa4u,count,ik_matrices);
  return result;
}

// ---- 16A4DD: the first-person animation sampler (169C46's update) ----
// The view and slot layout is in the header. 16A4DD's stack locals (the
// 1CE5D0 channel record, 1441C0's root matrix, the 1E5470 queue) live in
// the Frame; every other access is the world's.
constexpr u32 fp_views=0x4f6498u,fp_banks=0x4f6494u,character_table=0x4ecc1cu;
constexpr u32 fp_light_seconds_f=0x3e4ccccdu;  // [0x46A16C] 0.2f
constexpr u32 fp_fade_seconds_f=0x3ecccccdu;   // [0x46A240] 0.4f
constexpr u32 fp_state_seconds_f=0x3e88b439u;  // [0x450170] 0.267f
constexpr u32 fp_quick_seconds_f=0x3e08b439u;  // [0x46A1E0] 0.1335f
constexpr u32 four_f=0x40800000u,five_f=0x40a00000u,seven_f=0x40e00000u,eight_f=0x41000000u;  // [0x46A120] [0x46A100] [0x46A2B8] [0x46A128]
thread_local const FirstPersonResiduals* bound_residuals=nullptr;
void residual_effects(u32 user,u32 object,bool attach) {
  if(!bound_residuals || !bound_residuals->effects)refuse("17A570","first-person weapon effect attachment");
  bound_residuals->effects(user,object,attach);
}
void residual_predict(u32 block) {
  if(!bound_residuals || !bound_residuals->predict)refuse("171440","first-person weapon resource prediction");
  bound_residuals->predict(block);
}
void residual_sound_stop(u32 sound) {
  if(!bound_residuals || !bound_residuals->sound_stop)refuse("128E70","first-person sound stop");
  bound_residuals->sound_stop(sound);
}
// FLD dword then FSTP dword: exact, except that the load quiets an sNaN.
u32 x87_reload(u32 x){return (x&0x7f800000u)==0x7f800000u && (x&0x3fffffu) && !(x&0x400000u)?x|0x400000u:x;}
u32 fp_view(Frame& f,u32 user){return f.r32(fp_views)+user*0x20ccu;}
u32 fp_slot(Frame& f,u32 user,u32 slot){return slot*0x1010u+fp_view(f,user)+0xcu;}
u32 fp_bank(Frame& f,u32 user,u32 slot){return ((slot+user*2u)<<12)+f.r32(fp_banks);}
u32 fp_body(Frame& f,u32 object){return f.r32(f.r32(f.r32(object_pool)+0x44u)+(object&0xffffu)*12u+8u);}
u32 fp_tag(Frame& f,u32 tag){return f.r32((tag&0xffffu)*16u+f.r32(tag_instances)+8u);}
// 11E060 (ESI weapon definition, EDX variant): the first-person model row
// (+2A8 count, +2AC rows of 0x10): variants 1 and 3 take the second, any
// other the first; a missing row falls back to the first, 0 without rows.
u32 fp_model_entry(Frame& f,u32 definition,u32 variant) {
  const u32 wanted=variant<=3u?(variant&1u):0u;
  const u32 count=f.r32(definition+0x2a8u);
  if(s32(wanted)<s32(count))return f.r32(definition+0x2acu)+wanted*16u;
  if(s32(count)>0)return f.r32(definition+0x2acu);
  return 0;
}
// 169432 (EAX user): 0x400054B for a unit holding two weapons (CFB40: body
// +212 and +213 both set), else 0x30000D9.
u32 fp_base_key(Frame& f,u32 user) {
  const u32 unit=f.r32(fp_view(f,user)+4u);
  if(unit==none)return 0x30000d9u;
  const u32 body=fp_body(f,unit);
  return f.r8(body+0x212u)!=0xffu && f.r8(body+0x213u)!=0xffu?0x400054bu:0x30000d9u;
}
// 16B1BC (EAX user, ECX slot; visible): slot flag bit 0.
void fp_slot_visible(Frame& f,u32 user,u32 slot,bool visible) {
  const u32 s=fp_slot(f,user,slot);
  const u32 flags=f.r32(s);
  if((flags&1u)!=u32(visible))f.w32(s,visible?flags|1u:flags&~1u);
}
// 17A570's scan of the effect datums ([0x4F720C]: +24 size, +38 count, +44
// data, +48 bitmap; 0x190-byte datums by the handle's index). It stores
// nothing unless an effect follows `object`: attaching, one with +48 ==
// object and +2 bit 7 (it then stores +58 and runs 17B1F0); detaching, one
// with +58 == user and +48 == object (it then deletes the flagged events,
// 16E8A0, and stores +58). Returns whether such an effect exists.
bool fp_effects_pending(Frame& f,u32 user,u32 object,bool attach) {
  const u32 array=f.r32(0x4f720cu);
  for(u32 n=0;s32(n)<s32(f.r32(array+0x38u));++n) {
    if(!((f.r32(f.r32(array+0x48u)+(n>>5)*4u)>>(n&31u))&1u))continue;
    const u32 handle=sx16(u16(f.r16(f.r32(array+0x24u)*n+f.r32(array+0x44u))))<<16|n;
    if(handle==none)return false;
    const u32 datum=(handle&0xffffu)*0x190u+f.r32(array+0x44u);
    if(attach) {
      if(f.r32(datum+0x48u)==object && (f.r8(datum+2u)&0x80u))return true;
    } else if(f.r32(datum+0x58u)==user && f.r32(datum+0x48u)==object) {
      return true;
    }
  }
  return false;
}
// 16B1F5 (EAX slot, EDI weapon; user): a changed weapon detaches the old
// one's effects (17A570 attach 0), stores +4 and attaches the new one's.
// 17A570 runs (a residual owner) only when an effect follows the weapon.
void fp_slot_weapon(Frame& f,u32 user,u32 slot,u32 weapon) {
  const u32 s=fp_slot(f,user,slot);
  const u32 old=f.r32(s+4u);
  if(weapon==old)return;
  if(old!=none && fp_effects_pending(f,user,old,false))residual_effects(user,old,false);
  f.w32(s+4u,weapon);
  if(weapon!=none && fp_effects_pending(f,user,weapon,true))residual_effects(user,weapon,true);
}
// 16B2B9 (EAX user, ECX slot): the weapon's resource prediction (171440 on
// definition +2B8), then +F0 = 30.
void fp_slot_predict(Frame& f,u32 user,u32 slot) {
  const u32 s=fp_slot(f,user,slot);
  const u32 weapon=f.r32(s+4u);
  // 171440 with an empty block (+0 count) stores nothing.
  if(weapon!=none) {
    const u32 block=fp_tag(f,f.r32(fp_body(f,weapon)))+0x2b8u;
    if(s32(f.r32(block))>0)residual_predict(block);
  }
  f.w16(s+0xf0u,0x1eu);
}
// 169B81 (EAX user, ECX slot): clears the manager's +64 fade bytes +1, +0, +3.
void fp_fade_clear(Frame& f,u32 user,u32 slot) {
  const u32 fade=fp_slot(f,user,slot)+0x6cu;
  f.w8(fade+1u,0);f.w8(fade,0);f.w8(fade+3u,0);
}
// 169B0A (EAX user, ECX slot; seconds): for a posed view (+0 bit 1) the
// orientations become the previous ones (+800) and the manager's fade
// restarts (1E14F0, BL 1) unless it has longer to run (1E16E0).
void fp_fade_previous(Frame& f,u32 user,u32 slot,u32 seconds) {
  const u32 view=fp_view(f,user);
  const u32 s=slot*0x1010u+view+0xcu;
  if(!(f.r8(view)&2u))return;
  const u32 bytes=f.r32(s+0x300u)<<5;
  const u32 bank=fp_bank(f,user,slot);
  if(bytes>0x100000u)refuse("169B0A","an orientation copy beyond the first-person banks");
  copy_words(f,bank+0x800u,bank,bytes>>2);
  const u32 remaining=animation_fade_remaining(f.world(),s+0x6cu,f.mxcsr());
  if(f.at_least(seconds,remaining))animation_fade_start(f.world(),s+0x6cu,1,x87_reload(seconds),f.fp());
}
// 1CE6D0 (EAX record): channel defaults (1CE5D0's values, its own order).
void channel_reset(Frame& f,u32 r) {
  f.w32(r,none);f.w16(r+4u,0xffffu);f.w16(r+6u,0xffffu);f.w32(r+0x1cu,zero_f);f.w8(r+0x10u,0);f.w8(r+0x11u,0);
  f.w16(r+0x12u,0);f.w32(r+0x18u,one);f.w16(r+0x14u,0);f.w16(r+0x16u,0);f.w32(r+8u,none);f.w8(r+0xcu,0xffu);
  f.w8(r+0xdu,0xffu);f.w16(r+0xeu,0xffffu);
}
// 1D4090 (EAX manager, ESI name): the graph node named `name`, else NONE.
u32 graph_node_named(Frame& f,u32 manager,u32 name) {
  const u32 graph=fp_tag(f,f.r32(manager+0x68u));
  const u32 count=f.r32(graph+0xcu);
  if(s32(count)<=0)return none;
  u32 row=f.r32(graph+0x10u);
  for(u32 n=0;s32(n)<s32(count);++n,row+=0x20u)if(f.r32(row)==name)return n;
  return none;
}
// 1E5740 (ECX graph, ESI flags): the first graph node whose +A byte has
// every bit of `flags`, else NONE.
u32 graph_node_flagged(Frame& f,u32 graph,u32 flags) {
  const u32 count=f.r32(graph+0xcu);
  if(s32(count)<=0)return none;
  u32 at=f.r32(graph+0x10u)+0xau;
  for(u32 n=0;s32(n)<s32(count);++n,at+=0x20u)if((f.r8(at)&flags)==flags)return n;
  return none;
}
// 1D4060 (EAX manager; model tag, count, map) -> 1E5770: each model node
// (+48 count, +4C rows of 0x60, +0 name) maps to the manager graph's node of
// that name (+C count, +10 rows of 0x20), NONE when absent; true when every
// node maps. The model's count is stored first and re-read each pass.
bool model_node_map(Frame& f,u32 manager,u32 model_tag,u32 count_out,u32 map) {
  const u32 graph=fp_tag(f,f.r32(manager+0x68u));
  const u32 model=fp_tag(f,model_tag);
  f.w32(count_out,f.r32(model+0x48u));
  bool all=true;
  for(u32 n=0;s32(n)<s32(f.r32(model+0x48u));++n) {
    const u32 nodes=f.r32(graph+0xcu);
    u32 found=none;
    if(s32(nodes)>0) {
      const u32 name=f.r32(f.r32(model+0x4cu)+n*0x60u);
      u32 row=f.r32(graph+0x10u);
      for(u32 k=0;s32(k)<s32(nodes);++k,row+=0x20u)if(name==f.r32(row)){found=k;break;}
    }
    f.w32(map+n*4u,found);
    if(found==none)all=false;
  }
  return all;
}
// 1E5470 / 1E5540: the node matrices of the graph's node tree in
// breadth-first order (the original's 255-entry stack queue).
void node_hierarchy(Frame& f,u32 graph,u32 out,u32 orientations,u32 root,u32 mirrored,u32 source) {
  if(s32(f.r32(graph+0xcu))<=0)return;
  std::array<u32,255> queue{};
  u32 length=1,next=0;
  Locals locals(f);
  const u32 local=locals(0x34);
  const auto push=[&](u32 node) {
    if(length>=queue.size())refuse("1E5470","a node tree beyond the original's 255-entry queue");
    queue[length++]=node;
  };
  do {
    const u32 node=queue[next++];
    const u32 row=f.r32(graph+0x10u)+node*0x20u;
    u32 parent=node?sx16(u16(f.r16(row+8u)))*0x34u+out:root;
    orientation(f,local,node*0x20u+orientations);
    if(mirrored!=none && sx16(u16(mirrored))==node) {
      // The Y-mirrored local matrix: scale 1, forward and up with y
      // negated, left = forward x up, position y negated; under the
      // mirrored node's source.
      const auto at=[&](u32 k){return f.r32(local+4u*k);};
      const u32 fx=at(1),fy=at(2),fz=at(3),ux=at(7),uy=at(8),uz=at(9),px=at(10),py=at(11),pz=at(12);
      const u32 neg_py=f.sub(zero_f,py),neg_fy=f.sub(zero_f,fy),neg_uy=f.sub(zero_f,uy);
      const u32 lx=f.sub(f.mul(fz,neg_uy),f.mul(uz,neg_fy));
      const u32 ly=f.sub(f.mul(fx,uz),f.mul(fz,ux));
      const u32 lz=f.sub(f.mul(ux,neg_fy),f.mul(fx,neg_uy));
      const std::array<u32,13> mirror{one,fx,neg_fy,fz,lx,ly,lz,ux,neg_uy,uz,px,neg_py,pz};
      for(u32 k=0;k<13u;++k)f.w32(local+4u*k,mirror[k]);
      parent=sx16(u16(source))*0x34u+out;
    }
    product(f,parent,local,node*0x34u+out);
    const u16 sibling=u16(f.r16(row+4u));
    if(sibling!=0xffffu)push(sx16(sibling));
    const u16 child=u16(f.r16(row+6u));
    if(child!=0xffffu)push(sx16(child));
  } while(next!=length);
}
// 10B0F0 (EAX weapon): shared ammunition: game mode 2 ([[0x4F3218]+8]), an
// owned item (+12C bit 0, +154 unit) of a player (+13C) whose game-engine
// gate (15C7F0) admits it.
bool weapon_shared_ammunition(Frame& f,u32 weapon) {
  if(f.r32(f.r32(0x4f3218u)+8u)!=2u)return false;
  const u32 body=fp_body(f,weapon);
  if(!(f.r8(body+0x12cu)&1u))return false;
  const u32 owner=f.r32(body+0x154u);
  if(owner==none)return false;
  const u32 player=f.r32(fp_body(f,owner)+0x13cu);
  if(player==none)return false;
  // 15C7F0 (stack player): the engine's variant table ([0x56DFA0] by
  // [[0x4F63B8]+C14]) is set, [[0x4F3218]+180] is 7, the player's bit is in
  // the word at [[0x51DF9C]], and [[0x4F3218]+22C] bit 5.
  if(!f.r32(f.r32(f.r32(0x4f63b8u)+0xc14u)*4u+0x56dfa0u))return false;
  const u32 globals=f.r32(0x4f3218u);
  if(f.r32(globals+0x180u)!=7u)return false;
  if(!(f.r16(f.r32(0x51df9cu))&(1u<<(player&31u))))return false;
  return (f.r8(globals+0x22cu)&0x20u)!=0;
}
// 1059C0 (EAX magazine; weapon, own only): the magazine's rounds (body
// +22A + 16 n, or the definition's +2C4 row +C for shared ammunition) plus
// those of the owner unit's other weapons (+218..+224) of the same tag.
u32 weapon_reserve(Frame& f,u32 magazine,u32 weapon,bool own) {
  const u32 body=fp_body(f,weapon);
  const bool shared=weapon_shared_ammunition(f,weapon);
  const auto rounds=[&](u32 b,u32 tag,u32& total)->bool {
    if(shared) {
      const u32 definition=fp_tag(f,tag);
      if(s32(f.r32(definition+0x2c0u))<=s32(magazine))return false;
      total=sx16(u16(f.r16(f.r32(definition+0x2c4u)+magazine*0x5cu+0xcu)));
      return true;
    }
    total=sx16(u16(f.r16(magazine*16u+b+0x22au)));
    return true;
  };
  u32 total=0;
  rounds(body,f.r32(body),total);
  const u32 owner=f.r32(body+0x154u);
  if(owner==none || own)return total;
  const u32 unit=fp_body(f,owner);
  for(u32 k=0;k<4u;++k) {
    const u32 other=f.r32(unit+0x218u+4u*k);
    if(other==none || other==weapon)continue;
    const u32 tag=f.r32(fp_body(f,other));
    if(tag!=f.r32(body))continue;
    u32 more=0;
    if(rounds(fp_body(f,other),tag,more))total+=more;
  }
  return total;
}
// 105C10 (EAX magazine, EDX weapon; own only): 1059C0 plus the magazine's
// loaded rounds (+22C + 16 n).
u32 weapon_rounds(Frame& f,u32 magazine,u32 weapon,bool own) {
  const u32 loaded=magazine*16u+fp_body(f,weapon)+0x224u;
  const u32 reserve=weapon_reserve(f,magazine,weapon,own);
  return reserve+sx16(u16(f.r16(loaded+8u)));
}
void first_person_rebuild(Frame& f,u32 user,u32 slot);
// 169863 (EAX key; user, slot, stop sound).
void first_person_state(Frame& f,u32 key,u32 user,u32 slot,bool stop_sound) {
  M& m=f.world();
  const u32 view=fp_view(f,user);
  const u32 s=slot*0x1010u+view+0xcu;
  if(f.r32(view+4u)==none)return;
  const u32 weapon=f.r32(s+4u);
  if(weapon==none)return;
  const u32 current=f.r32(s+0x84u);
  const u32 definition=fp_tag(f,f.r32(fp_body(f,weapon)));
  const u32 base=fp_base_key(f,user);
  // The current state excludes some keys: a repeated ready (0x5000024) or
  // 0x1000006E, a repeated 0x6000006 of a +12C bit 17 weapon, and
  // 0x8000063/0x9000062 outside 0x400000C/0x600005F.
  if(key==0x5000024u) {
    if(current==key)return;
  } else if(key==0x6000006u) {
    if(current==key && ((f.r32(definition+0x12cu)>>17)&1u))return;
  } else if(key==0x8000063u || key==0x9000062u) {
    if(current!=0x400000cu && current!=0x600005fu)return;
  } else if(key==0x1000006eu) {
    if(current==key)return;
  } else if(key==none) {
    return;
  }
  const std::int32_t k=s32(key);
  u32 seconds=fp_state_seconds_f;
  const auto quick=[&]{seconds=fp_quick_seconds_f;};
  const auto instant=[&]{seconds=zero_f;};
  if(k>0xb0005b1) {
    if(key==0xc000064u || key==0xc000073u || key==0xc0005aeu)quick();
    else if(k<=0xe000606){}
    else if(k<=0xe00060a)quick();
    else if(key==0x1000006eu)instant();
  } else if(k==0xb0005b1) {
    instant();
  } else if(k<=0x900006b) {
    if(k>=0x900006a)instant();
    else if(key==0x500000au)quick();
    else if(key==0x5000024u)instant();
    else if(k>0x6000005 && k<=0x6000007)instant();
  } else if(key==0xa000066u) {
    instant();
  } else if(key==0xb000065u) {
    quick();
  }
  bool clear=false;
  if(k>0xc000075)clear=key==0xc000077u || (k>0xe000606 && k<=0xe00060a) || key==0x1000006eu;
  else clear=k==0xc000075 || key==0x500000au || key==0x5000024u || key==0xc000073u;
  if(clear)fp_fade_clear(f,user,slot);
  if(f.greater(seconds,zero_f))fp_fade_previous(f,user,slot,x87_reload(seconds));
  const u32 start=base==0x400054bu?(slot?0x203fu:0x103fu):0x3fu;
  // On MCC caches this call searches the state key and the dual class by
  // their cache names (weapon_animation::QueryNamespace).
  if(!weapon_animation::manager_set_state(m,s+8u,key,0x7000101u,base,0x7000001u,0x82u,start,script_fp(f),
      weapon_animation::QueryOrigin::first_person_state))return;
  if(!stop_sound)return;
  const u32 sound=f.r32(s+0x1008u);
  if(sound==none)return;
  // The +100C word, sign-extended, can never equal 0xB00006D.
  residual_sound_stop(sound);
  f.w32(s+0x1008u,none);
  f.w16(s+0x100cu,0xffffu);
}
// 16AD45 (user, slot).
void first_person_rebuild(Frame& f,u32 user,u32 slot) {
  M& m=f.world();
  const u32 characters=f.r32(character_table);
  const u32 view=fp_view(f,user);
  const u32 s=slot*0x1010u+view+0xcu;
  const bool visible=(f.r8(s)&1u)!=0;
  if(visible)fp_slot_visible(f,user,slot,false);
  fp_slot_weapon(f,user,slot,none);
  f.w32(view,f.r32(view)&~2u);
  // The hand matrix: unit scale and axes, zero position.
  const std::array<u32,13> identity{one,one,zero_f,zero_f,zero_f,one,zero_f,zero_f,zero_f,one,zero_f,zero_f,zero_f};
  for(u32 k=0;k<13u;++k)f.w32(view+0x2098u+4u*k,identity[k]);
  f.w32(view,f.r32(view)&~4u);
  f.w32(s+0x304u,0);
  f.w32(s,f.r32(s)&~6u);
  const auto tail=[&]{fp_slot_predict(f,user,slot);};
  const u32 unit=f.r32(view+4u);
  if(unit==none)return tail();
  const u32 variant=f.r32(view+8u);
  if(s32(variant)<0 || s32(variant)>=s32(f.r32(characters+0x138u)))return tail();
  const u32 character=variant*0xbcu+f.r32(characters+0x13cu);
  const u32 index=sx8(std::uint8_t(f.r8(fp_body(f,unit)+slot+0x212u)));
  // CE230 (EAX unit, CX index): the unit's weapon (+218 rows), NONE for -1.
  const u32 weapon=u16(index)==0xffffu?none:f.r32(sx16(u16(index))*4u+fp_body(f,unit)+0x218u);
  if(weapon==none)return tail();
  const u32 definition=fp_tag(f,f.r32(fp_body(f,weapon)));
  const u32 entry=fp_model_entry(f,definition,variant);
  if(!entry || f.r32(entry+4u)==none || f.r32(entry+0xcu)==none)return tail();
  const u32 fade=view+0x202cu;
  f.w8(fade+1u,0);f.w8(fade,0);f.w8(fade+3u,0);
  const u32 manager=s+8u;
  animation_manager_setup(m,manager,f.r32(entry+0xcu),none,false,f.x87(),f.mxcsr());
  if(!slot)f.w32(view+0x2094u,graph_node_named(f,manager,0xe0000e0u));
  const u32 body_model=f.r32(character+4u);
  if(body_model!=none && model_node_map(f,manager,body_model,s+0xf8u,s+0x1fcu)) {
    f.w32(s,f.r32(s)|4u);
    if(slot) {
      const u32 graph=fp_tag(f,f.r32(manager+0x68u));
      for(u32 k=0;s32(k)<s32(f.r32(s+0xf8u));++k) {
        const u32 at=s+0x1fcu+4u*k;
        const u32 node=f.r32(at);
        if(node!=none && !(f.r8(node*32u+f.r32(graph+0x10u)+0xau)&0x20u))f.w32(at,none);
      }
    }
  }
  if(model_node_map(f,manager,f.r32(entry+4u),s+0xf4u,s+0xfcu)) {
    f.w32(s,f.r32(s)|2u);
    f.w16(s+0x2fcu,0xffffu);
    f.w16(s+0x2feu,0xffffu);
    if(slot && f.r32(s+0xf4u)) {
      const u32 graph=fp_tag(f,f.r32(manager+0x68u));
      const u32 node=f.r32(s+0xfcu);
      if(node!=none) {
        const u16 parent=u16(f.r16(node*32u+f.r32(graph+0x10u)+8u));
        if(parent!=0xffffu) {
          const u32 flags=(f.r8(sx16(parent)*32u+f.r32(graph+0x10u)+0xau)&8u)?0x10u:8u;
          const u32 source=graph_node_flagged(f,graph,flags);
          f.w16(s+0x2feu,source);
          if(u16(source)!=0xffffu)f.w16(s+0x2fcu,node);
        }
      }
    }
  }
  const u32 flags=f.r32(s);
  if((flags&2u) && (flags&4u)) {
    const u32 base=fp_base_key(f,user);
    fp_slot_weapon(f,user,slot,weapon);
    f.w16(s+0xecu,0);
    f.w32(s+0xe4u,zero_f);
    f.w32(s+0xe8u,zero_f);
    first_person_state(f,0x400000cu,user,slot,true);
    f.w32(s+0x1008u,none);
    f.w16(s+0x100cu,0xffffu);
    channel_reset(f,s+0xb8u);
    channel_reset(f,s+0x98u);
    for(const u32 at:{0xdcu,0xdeu,0xe0u,0xe2u,0xd8u,0xdau})f.w16(s+at,0xffffu);
    {
      // The overlay channels: on MCC caches these four lookups search the
      // dual class by its cache name (weapon_animation::QueryNamespace).
      const weapon_animation::QueryScope names(weapon_animation::QueryOrigin::first_person_channel,none);
      manager_start_named_overlay(m,manager,base,0x7000101u,0x12000068u,s+0xb8u,0x803fu,f.fp());
      f.w32(s+0xdcu,manager_overlay_trial(m,manager,base,0x7000101u,0x8000061u,f.fp()));
      f.w32(s+0xe0u,manager_overlay_trial(m,manager,base,0x7000101u,0xa000069u,f.fp()));
      f.w32(s+0xd8u,manager_overlay_trial(m,manager,base,0x7000101u,0xe0000deu,f.fp()));
    }
    if(visible)fp_slot_visible(f,user,slot,true);
  }
  tail();
}
// 16A4DD (user, slot).
void first_person_animation(Frame& f,u32 user,u32 slot) {
  M& m=f.world();
  const u32 view=fp_view(f,user);
  const u32 s=slot*0x1010u+view+0xcu;
  const u32 weapon=f.r32(s+4u);
  if(weapon==none)return;
  const u32 variant=f.r32(view+8u);
  const u32 body=fp_body(f,weapon);
  const u32 definition=fp_tag(f,f.r32(body));
  const u32 entry=fp_model_entry(f,definition,variant);
  if(!entry)refuse("16A4DD","a weapon without first-person model rows (the original faults)");
  const u32 fp_model=fp_tag(f,f.r32(entry+4u));
  const u32 body_model=fp_tag(f,f.r32(variant*0xbcu+f.r32(f.r32(character_table)+0x13cu)+4u));
  const u32 manager=s+8u;
  Locals locals(f);
  const u32 record=locals(0x20);
  channel_clear(f,record);  // 1CE5D0
  if(f.r32(s+0xf4u)!=f.r32(fp_model+0x48u) || f.r32(s+0xf8u)!=f.r32(body_model+0x48u))
    first_person_rebuild(f,user,slot);
  // 1D3FE0: the manager graph's node count (+C, signed word).
  const u32 count=sx16(u16(f.r16(fp_tag(f,f.r32(manager+0x68u))+0xcu)));
  const u32 bank=fp_bank(f,user,slot);
  f.w32(s+0x300u,count);
  if(s32(count)>0)
    for(u32 n=0,at=bank;s32(n)<s32(f.r32(s+0x300u));++n,at+=0x20u)copy_words(f,at,f.r32(identity_orientation),8);
  // The models' default orientations through the node maps.
  const auto publish=[&](u32 model,u32 map) {
    for(u32 counter=0;s32(sx16(u16(counter)))<s32(f.r32(model+0x48u));++counter) {
      const u32 n=sx16(u16(counter));
      const u16 index=u16(f.r16(map+n*4u));
      const u32 row=n*0x60u+f.r32(model+0x4cu);
      if(index==0xffffu)continue;
      const u32 to=sx16(index)*0x20u+bank;
      copy_words(f,to,row+0x18u,4);
      copy_words(f,to+0x10u,row+0xcu,3);
      f.w32(to+0x1cu,one);
    }
  };
  publish(fp_model,s+0xfcu);
  publish(body_model,s+0x1fcu);
  if(manager_valid(m,manager)) {  // B87B0
    sample_channels(f,manager,one,0,bank,f.r32(s+0x300u));                    // 1D5840
    overlays(f,weapon,manager,0,f.r32(s+0x300u),bank);                        // BF9A0
    const u32 animation=record_animation(f,manager);                         // B8780
    const bool light=animation && (f.r8(animation+0x18u)&0x10u);
    animation_fade_start(m,view+0x202cu,light?0:1,light?fp_light_seconds_f:fp_fade_seconds_f,f.fp());  // 1E14F0
    bool aimed=false;
    u32 weight=zero_f;
    if(f.r8(view+0x202du) && (weight=interpolation_fraction(f,view+0x202cu),f.greater(weight,epsilon_f))) {
      if(weapon_animation::channel_valid(m,s+0x98u))
        sample_channel(f,s+0x98u,x87_reload(weight),0,f.r32(s+0x300u),bank);
      if(weapon_animation::channel_valid(m,s+0xb8u))
        sample_channel(f,s+0xb8u,f.mul(f.add(f.r32(body+0x188u),half_f),weight),0,f.r32(s+0x300u),bank);
      if(f.r16(s+0xdau)!=0xffffu && manager_start(f,manager,record,f.r32(s+0xd8u))) {
        sample_aim_channel(f,record,x87_reload(f.r32(view+0x2040u)),x87_reload(f.r32(view+0x2054u)),x87_reload(weight),0,
            f.r32(s+0x300u),bank);  // 1CF3B0
        aimed=true;
      }
      if(f.r16(s+0xdeu)!=0xffffu && manager_start(f,manager,record,f.r32(s+0xdcu))) {
        const u32 movement=record_animation(f,record);
        if(!movement)refuse("16A4DD","a movement overlay without an animation (the original faults)");
        if(s32(sx16(u16(f.r16(movement+0x14u))))<9) {
          f.w16(s+0xdcu,0xffffu);
          f.w16(s+0xdeu,0xffffu);
        } else {
          // Each axis samples the movement overlay at its positive or
          // negative frame with |value| * weight.
          const auto axis=[&](u32 at,u32 positive,u32 negative) {
            if(f.greater(f.r32(at),zero_f)) {
              set_record_frame(f,record,positive);
              sample_channel(f,record,f.mul(f.r32(at),weight),0,f.r32(s+0x300u),bank);
            } else if(f.greater(zero_f,f.r32(at))) {
              set_record_frame(f,record,negative);
              sample_channel(f,record,f.sub(zero_f,f.mul(f.r32(at),weight)),0,f.r32(s+0x300u),bank);
            }
          };
          axis(view+0x2030u,zero_f,one);
          axis(view+0x2034u,three_f,two_f);
          if(!aimed) {
            axis(view+0x2040u,four_f,five_f);
            axis(view+0x2044u,seven_f,six_f);
          }
          if(f.greater(f.r32(s+0xe4u),zero_f)) {
            set_record_frame(f,record,eight_f);
            sample_channel(f,record,f.mul(f.r32(s+0xe4u),weight),0,f.r32(s+0x300u),bank);
          }
        }
      }
    }
    // The ammunition overlay (+E0): a magazine weapon (+292 == 2) in its
    // reload states eases the displayed rounds (+22C) toward the
    // remaining ones over the reload's last frames; any other weapon shows
    // its +22C rounds clamped to the animation.
    if(f.r16(s+0xe2u)!=0xffffu && manager_start(f,manager,record,f.r32(s+0xe0u))) {
      const u32 ammunition=record_animation(f,record);
      u32 frame;
      const u32 state=f.r32(manager+0x7cu);
      if(f.r16(definition+0x292u)==2u && (state==0xc000064u || state==0xb000065u)) {
        u32 shown=f.r16(body+0x22cu);
        const u16 span=u16(f.r16(body+0x228u)-f.r16(body+0x226u));
        if(s32(sx16(span))>=0x2c) {
          u32 scale=f.mul(f.from_int(sx16(span)-0x2cu),fp_light_seconds_f);
          if(f.greater(scale,one))scale=one;
          const u32 magazines=f.r32(definition+0x2c4u);
          u32 rounds=weapon_rounds(f,0,weapon,false);
          const u32 capacity=sx16(u16(f.r16(magazines+0xau)));
          if(s32(sx16(u16(rounds)))>s32(sx16(u16(capacity))))rounds=capacity;
          const u32 delta=sx16(u16(rounds))-sx16(u16(shown));
          shown+=truncated(f.mul(f.from_int(delta),scale));
        }
        frame=sx16(u16(shown));
      } else {
        const u16 shown=u16(f.r16(body+0x22cu));
        if(s32(sx16(shown))<0)frame=0;
        else {
          if(!ammunition)refuse("16A4DD","an ammunition overlay without an animation (the original faults)");
          frame=sx16(u16(f.r16(ammunition+0x14u)))-1u;
          if(s32(sx16(shown))<=s32(frame))frame=sx16(shown);
        }
      }
      set_record_frame(f,record,f.from_int(frame));
      sample_channel(f,record,one,0,f.r32(s+0x300u),bank);
    }
    if((f.r8(view)&2u) && f.r8(manager+0x65u))  // 1D4560
      crossfade(f,manager+0x64u,bank,sx16(u16(f.r16(s+0x300u))),0,bank+0x800u);
  }
  f.w32(view,f.r32(view)|2u);
  // 1441C0: the root matrix: unit scale and axes at the definition's +308.
  const u32 root=locals(0x34);
  const std::array<u32,10> axes{one,one,zero_f,zero_f,zero_f,one,zero_f,zero_f,zero_f,one};
  for(u32 k=0;k<10u;++k)f.w32(root+4u*k,axes[k]);
  copy_words(f,root+0x28u,definition+0x308u,3);
  const u16 mirrored=u16(f.r16(s+0x2fcu));
  f.w32(s+0x304u,f.r32(s+0x300u));
  const u32 graph=fp_tag(f,f.r32(manager+0x68u));
  if(mirrored!=0xffffu && u16(f.r16(s+0x2feu))!=0xffffu)
    node_hierarchy(f,graph,s+0x308u,bank,root,mirrored,f.r16(s+0x2feu));  // 1D4030 -> 1E5540
  else
    node_hierarchy(f,graph,s+0x308u,bank,root,none,0);                     // 1D4000 -> 1E5470
  if(slot)return;
  const u32 hand=f.r32(view+0x2094u);
  if(hand==none){f.w32(view,f.r32(view)&~4u);return;}
  copy_words(f,view+0x2098u,hand*0x34u+s+0x308u,13);
  f.w32(view+0x20c0u,f.sub(f.r32(view+0x20c0u),f.r32(definition+0x308u)));
  f.w32(view+0x20c4u,f.sub(f.r32(view+0x20c4u),f.r32(definition+0x30cu)));
  const u32 z=f.sub(f.r32(view+0x20c8u),f.r32(definition+0x310u));
  f.w32(view,f.r32(view)|4u);
  f.w32(view+0x20c8u,z);
}
// ---- 169C46: the first-person update ----
// 168D55 runs 169C46 once per local user and tick. It moves the user's
// first-person view: two pairs of springs sway the weapon with the unit's
// aim velocity and the look input, then each weapon slot advances its
// animation manager (reporting animation events), drives its motion and
// overlay channels, plays the idle fidget, samples the pose (16A4DD) and
// counts down to its next resource prediction.
//
// The layouts below name the fields this system reads and writes. Every
// access still goes through the Frame, in the original's order, so the
// world sees the same stores (address, width, value, sequence).

// TU1.5 .rdata constants.
constexpr u32 fp_aim_rate_f=0x42900000u;       // [0x46AAF0] 72.0: aim spring acceleration
constexpr u32 fp_look_rate_f=0x41d80000u;      // [0x46AAEC] 27.0: look spring acceleration
constexpr u32 fp_minus_one_f=0xbf800000u;      // [0x46A0DC] -1.0
constexpr u32 fp_moving_f=0x3dcccccdu;         // [0x46A178] 0.1: the moving speed
constexpr u32 fp_frame_rate_f=0x41f00000u;     // [0x46A20C] 30.0 frames per second
constexpr u32 fp_frame_seconds_f=0x3d088889u;  // [0x46A17C] 1/30
constexpr u32 fp_settle_rate_f=0x41100000u;    // [0x44A498] 9.0: settle spring acceleration
constexpr u32 fp_random_unit_f=0x37800080u;    // [0x46A160] 1/65535
constexpr u32 fp_blend_low_f=0x3ee66666u;      // [0x46A330] 0.45
constexpr u32 fp_blend_high_f=0x3f0ccccdu;     // [0x46A234] 0.55
constexpr u32 fp_flying_ticks_f=0x3e3851ecu;   // [0x46A458] 0.18 seconds
constexpr u32 fp_event_key=0x600008au;         // the key of frame and effect event records
constexpr u32 fp_idle_state=0x400000cu,fp_fidget_state=0x600005fu,fp_overlay_state=0xb000067u;
constexpr u32 fp_airborne_overlay=0x60000d8u,fp_moving_overlay=0x6000060u,fp_overlay_name=0x12000068u;

// [0x51DF54] game time: +2 ticks per second (word), +4 seconds per tick.
struct GameTime {
  Frame& f;
  u32 at()const{return f.r32(0x51df54u);}
  u32 ticks_per_second()const{return sx16(u16(f.r16(at()+2u)));}
  u32 tick_seconds()const{return f.r32(at()+4u);}
};

// [0x4F9B54] input rows (0x94 bytes per user).
struct UserInput {
  Frame& f;
  u32 user;
  u32 row()const{return f.r32(0x4f9b54u)+user*0x94u;}
  u32 look(u32 axis)const{return f.r32(row()+0x24u+4u*axis);}  // yaw, pitch
  u32 look_address()const{return row()+0x24u;}
  u32 action()const{return f.r16(row()+0x42u);}               // 18A830: NONE without an action
  u32 magnitude(u32 n)const{return f.r32(row()+0x74u+4u*n);}  // 18A210 takes the larger
};

// An animation channel record (0x20 bytes).
struct AnimationChannel {
  Frame& f;
  u32 at;
  u32 graph_tag()const{return f.r32(at);}
  // The animation handle: high word the animation, low word the child
  // graph (0xFFFF: the channel's own graph).
  u32 handle()const{return f.r32(at+4u);}
  void set_handle(u32 v){f.w32(at+4u,v);}
  u16 child()const{return u16(f.r16(at+4u));}
  u16 animation()const{return u16(f.r16(at+6u));}
  u32 key()const{return f.r32(at+8u);}
  u32 delay()const{return f.r8(at+0x10u);}
  void set_delay(u32 v){f.w8(at+0x10u,v);}
  // +11 state: bit 0 holding, bit 2 wrapped this tick, bit 3 at its end.
  u32 state()const{return f.r8(at+0x11u);}
  void set_state(u32 v){f.w8(at+0x11u,v);}
  // +12 controls: bit 0 playing, 1 loops, 3 unit rate for a variant, 5 a
  // random variant at loop frame 0, 7/8/9 no frame/sound/effect events,
  // 12/13 admit the gated events.
  u32 controls()const{return f.r16(at+0x12u);}
  void set_controls(u32 v){f.w16(at+0x12u,v);}
  u32 events()const{return f.r16(at+0x14u);}  // frame event bits passed this tick
  void set_events(u32 v){f.w16(at+0x14u,v);}
  void clear_events(){f.w16(at+0x14u,0);f.w16(at+0x16u,0);}
  u32 rate()const{return f.r32(at+0x18u);}
  void set_rate(u32 v){f.w32(at+0x18u,v);}
  u32 frame()const{return f.r32(at+0x1cu);}
  bool set()const{return graph_tag()!=none && animation()!=0xffffu;}
};

// An animation manager (0x84 bytes): channels 0 (primary), 1 (blend) and
// 2 (transition), two fades, the graph, the mode keys and the state key.
struct AnimationManager {
  Frame& f;
  u32 at;
  AnimationChannel channel(u32 n)const{return {f,at+0x20u*n};}
  u32 fade(u32 n)const{return at+0x60u+4u*n;}  // +60 and +64 fade records
  u32 graph_tag()const{return f.r32(at+0x68u);}
  bool has_graph()const{return graph_tag()!=none;}
  // +6C flags: bit 0 held (channels only check), bit 5 the blend channel samples.
  u32 flags()const{return f.r8(at+0x6cu);}
  void set_flags(u32 v){f.w8(at+0x6cu,v);}
  bool held()const{return flags()&1u;}
  u32 mode_key(u32 n)const{return f.r32(at+0x70u+4u*n);}  // +70, +74, +78
  u32 state()const{return f.r32(at+0x7cu);}
  u32 blend()const{return f.r32(at+0x80u);}
};

// A graph animation row (0x6C bytes).
struct AnimationDefinition {
  Frame& f;
  u32 at;
  u32 frame_count()const{return sx16(u16(f.r16(at+0x14u)));}
  u32 flags()const{return f.r8(at+0x18u);}  // bit 1: no interpolation into it
  u32 loop_frame()const{return f.r16(at+0x2eu);}
  // Event tables: frame events (+4C/+50, 4-byte rows: bit, frame), sound
  // events (+54/+58, 8-byte rows: reference, frame, key) and effect events
  // (+5C/+60, 4-byte rows: reference, frame).
  u32 event_count(u32 table)const{return f.r32(at+0x4cu+8u*table);}
  u32 event_row(u32 table,u32 n)const{return f.r32(at+0x50u+8u*table)+n*(table==1u?8u:4u);}
};

// The first-person view of a local user ([0x4F6498] + user * 0x20CC).
struct FirstPersonView {
  Frame& f;
  u32 at;
  // +0 flags: bit 0 updated, bit 1 posed, bit 2 hand matrix set.
  u32 flags()const{return f.r32(at);}
  void set_flags(u32 v){f.w32(at,v);}
  u32 unit()const{return f.r32(at+4u);}
  u32 slot(u32 n)const{return at+0xcu+0x1010u*n;}
  u32 fade()const{return at+0x202cu;}
  // The springs (value, rate): aim (+2030/+2038), look (+2040/+2048), per
  // axis; the previous look angles (+2050) and their change (+2058).
  u32 aim_value(u32 axis)const{return at+0x2030u+4u*axis;}
  u32 aim_rate(u32 axis)const{return at+0x2038u+4u*axis;}
  u32 look_value(u32 axis)const{return at+0x2040u+4u*axis;}
  u32 look_rate(u32 axis)const{return at+0x2048u+4u*axis;}
  u32 previous_look(u32 axis)const{return at+0x2050u+4u*axis;}
  u32 look_change(u32 axis)const{return at+0x2058u+4u*axis;}
};

// A first-person weapon slot (0x1010 bytes in the view).
struct FirstPersonSlot {
  Frame& f;
  u32 at;
  u32 flags()const{return f.r32(at);}  // bit 0 visible
  bool visible()const{return f.r8(at)&1u;}
  u32 weapon()const{return f.r32(at+4u);}
  AnimationManager manager()const{return {f,at+8u};}
  AnimationChannel motion()const{return {f,at+0x98u};}   // walk/airborne overlay
  AnimationChannel overlay()const{return {f,at+0xb8u};}  // the 0x12000068 overlay
  u32 settle_value()const{return at+0xe4u;}
  u32 settle_rate()const{return at+0xe8u;}
  u32 idle_ticks()const{return f.r16(at+0xecu);}
  void set_idle_ticks(u32 v){f.w16(at+0xecu,v);}
  u32 idle_wait()const{return f.r16(at+0xeeu);}
  void set_idle_wait(u32 v){f.w16(at+0xeeu,v);}
  u32 prediction_countdown()const{return f.r16(at+0xf0u);}
  void set_prediction_countdown(u32 v){f.w16(at+0xf0u,v);}
  u32 state_countdown()const{return f.r16(at+0xf2u);}
  void set_state_countdown(u32 v){f.w16(at+0xf2u,v);}
  u32 node_count()const{return f.r32(at+0x300u);}
  void set_event_sound(u32 v){f.w32(at+0x1008u,v);}
  void set_event_state(u32 v){f.w16(at+0x100cu,v);}
};

// The unit's body fields this system reads.
struct FirstPersonUnit {
  Frame& f;
  u32 body;
  u32 aim_velocity(u32 n)const{return f.r32(body+0x1b0u+4u*n);}
  u32 aim_velocity_address()const{return body+0x1b0u;}
  bool seated()const{return f.r8(body+0xaau);}
  u32 physics_flags()const{return f.r8(body+0x348u);}  // bit 0 flying, bit 4 airborne
  bool flight_blocked()const{return f.r8(body+0xc1u)&1u;}
  u32 flight_ticks()const{return sx8(std::uint8_t(f.r8(body+0x399u)));}
  u32 flight_mode()const{return f.r8(body+0x3dcu);}
  u32 parent()const{return f.r32(body+0x14u);}
};
#if HALO2_NATIVE_SCRIPT_X86_FP
// 121890's 12193C..121951: FLD v0, FMUL dt, FSUBR delta, FABS, FLD limit,
// FCOMIP, JB: the limit is below the remaining distance (or unordered).
bool x87_spring_far(u32 v0,u32 dt,u32 delta,u32 limit,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};unsigned char below{};
  const float a=f32(v0),b=f32(dt),c=f32(delta),d=f32(limit);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfmuls %4\n\tfsubrs %5\n\tfabs\n\tflds %6\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tsetb %0\n\tfldcw %1"
      :"=q"(below),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c),"m"(d):"cc","st","st(1)");
  return below;
}
// 121981..1219A5: FLD delta, FABS, FMUL accel, FADD st0,st0 against the
// squared limit (FCOMIP + JB): below (or unordered) FSQRT, FSTP dword.
bool x87_spring_speed(u32 delta,u32 accel,u32 limit2,u32& speed,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};unsigned char below{};float out{};
  const float a=f32(delta),b=f32(accel),c=f32(limit2);
  asm volatile("fnstcw %2\n\tfldcw %3\n\tflds %4\n\tfabs\n\tfmuls %5\n\tfadd %%st(0), %%st\n\tflds %6\n\t"
      "fld %%st(1)\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tsetb %0\n\tfsqrt\n\tfstps %1\n\tfldcw %2"
      :"=q"(below),"=m"(out),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c):"cc","st","st(1)","st(2)");
  speed=bits(out);
  return below;
}
// 1219BD..1219DE: FLD speed, FSUB v0, FDIV dt, FST dword (the rate), FABS,
// FLD accel, FXCH, FCOMIP + JBE: the extended |rate| is above accel.
bool x87_spring_rate(u32 speed,u32 v0,u32 dt,u32 accel,u32& rate,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};unsigned char above{};float out{};
  const float a=f32(speed),b=f32(v0),c=f32(dt),d=f32(accel);
  asm volatile("fnstcw %2\n\tfldcw %3\n\tflds %4\n\tfsubs %5\n\tfdivs %6\n\tfsts %1\n\tfabs\n\tflds %7\n\t"
      "fxch %%st(1)\n\tfcomip %%st(1), %%st\n\tfstp %%st(0)\n\tseta %0\n\tfldcw %2"
      :"=q"(above),"=m"(out),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c),"m"(d):"cc","st","st(1)");
  rate=bits(out);
  return above;
}
// 24570 (EAX vector) then FSTP dword: the x87 length's register sequence.
u32 x87_vector_length(u32 x,u32 y,u32 z,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float out{};
  const float a=f32(x),b=f32(y),c=f32(z);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %5\n\tflds %4\n\tflds %3\n\t"
      ".byte 0xd9,0xc0,0xd8,0xc9,0xd9,0xc2,0xd8,0xcb,0xde,0xc1,0xd9,0xc3,0xd8,0xcc,0xde,0xc1,0xd9,0xfa,"
      "0xdd,0xdb,0xdd,0xd8,0xdd,0xd8\n\tfstps %0\n\tfldcw %1"
      :"=m"(out),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c):"st","st(1)","st(2)","st(3)");
  return bits(out);
}
// 16A224..16A253: FLD rate, FADD 1.0, FMUL dt, FADD st0,st0, FLD frame,
// FMUL [0x46A17C], FADDP, FSTP dword.
u32 x87_overlay_seconds(u32 rate,u32 dt,u32 frame,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float out{};
  const float a=f32(rate),b=f32(one),c=f32(dt),d=f32(frame),e=f32(fp_frame_seconds_f);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfadds %4\n\tfmuls %5\n\tfadd %%st(0), %%st\n\tflds %6\n\t"
      "fmuls %7\n\tfaddp\n\tfstps %0\n\tfldcw %1"
      :"=m"(out),"+m"(saved):"m"(mode),"m"(a),"m"(b),"m"(c),"m"(d),"m"(e):"st","st(1)");
  return bits(out);
}
// 37DC0A (CRT fmod; ST1 dividend, ST0 divisor, both FLD dword): 331ECB's
// FXAM classes select 37DC14 (FPREM until C2 clears, exact) for two normal
// operands and 331F68 (FLDZ) for a zero or denormal dividend over a normal
// divisor; every other class pair reaches the CRT error methods (3334F0).
// Then FMUL [0x46A20C] and FSTP dword under the caller's control word.
u32 x87_fmod_frame(u32 dividend,u32 divisor,std::uint16_t control) {
  const float a=f32(dividend),b=f32(divisor);
  if(std::fpclassify(b)!=FP_NORMAL)refuse("3334F0","CRT fmod by a zero, denormal or non-finite divisor (37DC0A error method)");
  long double remainder{};
  switch(std::fpclassify(a)) {
  case FP_NORMAL: {
    const long double x=a,y=b;
    asm volatile("fldt %2\n\tfldt %1\n\t"
        "1:\n\tfprem\n\tfnstsw %%ax\n\ttestw $0x400, %%ax\n\tjnz 1b\n\tfstp %%st(1)\n\tfstpt %0"
        :"=m"(remainder):"m"(x),"m"(y):"ax","cc","st","st(1)");
    break;
  }
  case FP_ZERO:case FP_SUBNORMAL:remainder=0.0L;break;
  default:refuse("3334F0","CRT fmod of an infinite or NaN dividend (37DC0A error method)");
  }
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float out{};const float scale=f32(fp_frame_rate_f);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfldt %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(out),"+m"(saved):"m"(mode),"m"(remainder),"m"(scale):"st");
  return bits(out);
}
// 259B0 (ECX seed; low, high) then FSTP dword: FILD (seed >> 16), FMUL
// [0x46A160], FLD high, FSUB low, FMULP, FADD low.
u32 x87_random_range(u32 draw,u32 low,u32 high,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};float out{};
  const std::int32_t r=s32(draw);const float a=f32(fp_random_unit_f),b=f32(high),c=f32(low);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tflds %5\n\tfsubs %6\n\tfmulp\n\tfadds %6\n\t"
      "fstps %0\n\tfldcw %1"
      :"=m"(out),"+m"(saved):"m"(mode),"m"(r),"m"(a),"m"(b),"m"(c):"st","st(1)");
  return bits(out);
}
// 16A415..16A435: FILD draw, FMUL [0x46A160], FLD chance, FXCH, FCOMIP +
// JB: the draw is below the chance (or unordered).
bool x87_random_below(u32 draw,u32 chance,std::uint16_t control) {
  const auto mode=sse::x87_control(control);std::uint16_t saved{};unsigned char below{};
  const std::int32_t r=s32(draw);const float a=f32(fp_random_unit_f),b=f32(chance);
  asm volatile("fnstcw %1\n\tfldcw %2\n\tfildl %3\n\tfmuls %4\n\tflds %5\n\tfxch %%st(1)\n\t"
      "fcomip %%st(1), %%st\n\tfstp %%st(0)\n\tsetb %0\n\tfldcw %1"
      :"=q"(below),"+m"(saved):"m"(mode),"m"(r),"m"(a),"m"(b):"cc","st","st(1)");
  return below;
}
#else
[[noreturn]] void fp_update_x87(){refuse("169C46","x87 first-person update on a non-x86 host");}
bool x87_spring_far(u32,u32,u32,u32,std::uint16_t){fp_update_x87();}
bool x87_spring_speed(u32,u32,u32,u32&,std::uint16_t){fp_update_x87();}
bool x87_spring_rate(u32,u32,u32,u32,u32&,std::uint16_t){fp_update_x87();}
u32 x87_vector_length(u32,u32,u32,std::uint16_t){fp_update_x87();}
u32 x87_overlay_seconds(u32,u32,u32,std::uint16_t){fp_update_x87();}
u32 x87_fmod_frame(u32,u32,std::uint16_t){fp_update_x87();}
u32 x87_random_range(u32,u32,u32,std::uint16_t){fp_update_x87();}
bool x87_random_below(u32,u32,std::uint16_t){fp_update_x87();}
#endif
// 121890, the critically damped spring: moves `value` toward `target`
// under a rate limit and an acceleration. When one step's travel at most
// reaches the target it arrives (rate 0, value the clamped target);
// otherwise the rate eases toward the speed that stops at the target
// (sqrt(2 a d), at most the limit) and the value integrates with it. The
// distance test, the square root and the rate's rounding are x87; the rest
// is SSE (see the x87 helpers above). 169C46 never wraps (DL 0 at all
// five calls); the wrapped form is unproven and refuses.
bool fp_spring(Frame& f,u32 rate_at,u32 value_at,bool wrap,u32 limit,u32 target,u32 dt,u32 accel,u32 low,u32 high) {
  if(wrap)refuse("121890","a wrapped spring (DL set), unreached from 169C46");
  const u32 rate=f.r32(rate_at),value=f.r32(value_at);
  const u32 limit_squared=f.mul(limit,limit);
  const u32 distance=f.sub(target,value);
  const u32 accelerating=f.mul(f.mul(dt,dt),accel);
  const u32 coasting=f.mul(dt,limit);
  const u32 reach=f.greater(accelerating,coasting)?coasting:accelerating;
  if(!x87_spring_far(rate,dt,distance,reach,f.x87())) {
    u32 arrived=target;
    if(f.greater(low,target))arrived=low;
    else if(f.greater(target,high))arrived=high;
    f.w32(rate_at,zero_f);
    f.w32(value_at,arrived);
    return true;
  }
  u32 speed=limit;
  if(u32 root=0;x87_spring_speed(distance,accel,limit_squared,root,f.x87()))speed=root;
  if(f.greater(zero_f,distance))speed=f.sub(zero_f,speed);
  u32 change=0;
  if(x87_spring_rate(speed,rate,dt,accel,change,f.x87())) {
    // Beyond the acceleration: its sign times the acceleration.
    const std::int32_t sign=f.equal(change,zero_f)?0:f.greater(zero_f,change)?-1:1;
    change=f.mul(f.from_int(u32(sign)),accel);
  }
  const u32 step=f.mul(change,dt);
  const u32 next_rate=f.add(step,rate);
  u32 next=f.add(f.mul(step,half_f),next_rate);
  next=f.add(f.mul(next,dt),value);
  if(f.greater(low,next))next=low;
  else if(f.greater(next,high))next=high;
  f.w32(rate_at,next_rate);
  f.w32(value_at,next);
  return false;
}
// C1F90: an angle change wrapped into (-pi, pi] by the runtime's half
// turn ([0x5564C8]) and turn ([0x5564C4]).
u32 fp_angle_change(Frame& f,u32 angle,u32 previous) {
  u32 change=f.sub(angle,previous);
  const u32 half=f.r32(0x5564c8u),turn=f.r32(0x5564c4u);
  if(f.at_least(change,half))change=f.sub(change,turn);
  if(f.at_least(f.sub(zero_f,half),change))change=f.add(turn,change);
  return change;
}
// 158290: the user's camera director ([0x4F5514] rows of 0x140): 0 first
// person (mode 245400 at a zero blend), 1, 2 or 3 otherwise.
u32 fp_director_kind(Frame& f,u32 user) {
  const u32 director=0x4f5514u+user*0x140u;
  const u32 mode=f.r32(director+0xcu);
  if(mode==0x245400u)return f.equal(f.r32(director+4u),zero_f)?0u:3u;
  if(mode==0x245ea0u)return 1;
  if(mode==0x16f6a0u || (mode==0x246380u && f.r8(0x52d6a0u)))return 2;
  return 3;
}
// 1E15D0: one tick of a fade record (+0 position, +1 length, +3 flags: bit
// 0 rising, bit 1 done). Returns whether the fade was running.
bool fp_fade_tick(Frame& f,u32 fade) {
  const u32 length=f.r8(fade+1u);
  if(!length)return false;
  const u32 flags=f.r8(fade+3u);
  if(flags&2u)return false;
  const auto signed_byte=[](u32 b){return s32(sx8(std::uint8_t(b)));};
  if(flags&1u) {
    const u32 position=(f.r8(fade)+1u)&0xffu;
    f.w8(fade,position);
    if(signed_byte(position)<signed_byte(length))return true;
    f.w8(fade,length);
    f.w8(fade+3u,flags|2u);
    return true;
  }
  const u32 position=(f.r8(fade)-1u)&0xffu;
  f.w8(fade,position);
  if(signed_byte(position)>0)return true;
  f.w8(fade,0);
  f.w8(fade+3u,flags|2u);
  return true;
}
// 1CEE60: the graph that holds a channel's animation: its own graph, or the
// child graph (+4C count, +50 rows of 0x20, +4 tag) the handle names; 0
// when the child is out of range or unset.
u32 fp_channel_graph(const AnimationChannel& channel) {
  Frame& f=channel.f;
  const u32 graph=fp_tag(f,channel.graph_tag());
  const u16 child=channel.child();
  if(child==0xffffu)return graph;
  if(s32(sx16(child))>=s32(f.r32(graph+0x4cu)))return 0;
  const u32 tag=f.r32(f.r32(graph+0x50u)+sx16(child)*0x20u+4u);
  return tag==none?0:fp_tag(f,tag);
}
// The callback 1D39C0 hands to the channels it advances. 169C46 passes
// 169C21/169C33, which report slot 0/1's weapon sound events (169BA2);
// 1D3860's single-channel tick passes none.
struct EventCallback {
  bool first_person=false;
  u32 user=0,slot=0;
};
// The event record 1CF700 builds for the callback (0x14 bytes on its
// stack: +0 type, +2 frame, +4 reference, +8 index, +C flags, +10 key).
struct AnimationEvent {
  u32 type=0,frame_word=0,reference=none,index=none,flags=0,key=fp_event_key;
  u32 frame=0;  // the frame as a float, the callback's second argument
};
void fp_report_event(Frame& f,const EventCallback& callback,const AnimationEvent& event);
// 1CF700: the animation's events in [min(from, to), max(from, to)).
// Frame events set their bit in the returned mask; with a callback every
// frame event (type 0), sound event (type 1, through the graph's +18
// references) and effect event (type 2, +20 references) in range is
// reported. A reference row's +8 flags gate it on the channel controls
// (bit 1: +12 bit 13, bit 2: +12 bit 12) and on the direction (bit 4:
// forward only, bit 5: backward only).
u32 fp_animation_events(Frame& f,const EventCallback* callback,u32 graph,const AnimationDefinition& animation,
    u32 controls,u32 from,u32 to) {
  u32 mask=0;
  const u32 low=f.greater(from,to)?to:from;
  const u32 high=f.greater(from,to)?from:to;
  const auto in_range=[&](u32 frame){return f.at_least(frame,low) && f.greater(high,frame);};
  const auto admitted=[&](u32 gate) {
    if((gate&2u) && !(controls&0x2000u))return false;
    if((gate&4u) && !(controls&0x1000u))return false;
    bool forward_ok=true;
    if((gate&0x10u) && f.greater(from,to))forward_ok=false;
    if((gate&0x20u) && f.greater(to,from))return false;
    return forward_ok;
  };
  if(!(controls&0x80u))
    for(u32 n=0;s32(n)<s32(animation.event_count(0));++n) {
      const u32 row=animation.event_row(0,n);
      const u32 word=f.r16(row+2u);
      const u32 frame=f.from_int(sx16(u16(word)));
      if(!in_range(frame))continue;
      const u32 bit=sx16(u16(f.r16(row)));
      mask|=1u<<(bit&31u);
      if(callback)fp_report_event(f,*callback,{.type=0,.frame_word=word,.index=bit,.frame=frame});
    }
  if(!(controls&0x100u))
    for(u32 n=0;s32(n)<s32(animation.event_count(1));++n) {
      const u32 row=animation.event_row(1,n);
      const u32 word=f.r16(row+2u);
      const u32 frame=f.from_int(sx16(u16(word)));
      if(!in_range(frame))continue;
      const u16 index=u16(f.r16(row));
      if(index==0xffffu)continue;
      const u32 reference=f.r32(graph+0x18u)+sx16(index)*12u;
      if(!reference)continue;
      const u32 sound=f.r32(reference+4u);
      if(sound==none || !callback)continue;
      const u32 gate=f.r16(reference+8u);
      if(!admitted(gate))continue;
      fp_report_event(f,*callback,{.type=1,.frame_word=word,.reference=sound,.flags=gate,.key=f.r32(row+4u),.frame=frame});
    }
  if(!(controls&0x200u))
    for(u32 n=0;s32(n)<s32(animation.event_count(2));++n) {
      const u32 row=animation.event_row(2,n);
      const u32 word=f.r16(row+2u);
      const u32 frame=f.from_int(sx16(u16(word)));
      if(!in_range(frame))continue;
      const u16 index=u16(f.r16(row));
      if(index==0xffffu)continue;
      const u32 reference=f.r32(graph+0x20u)+sx16(index)*12u;
      if(!reference)continue;
      const u32 effect=f.r32(reference+4u);
      if(effect==none)continue;
      const u32 gate=f.r16(reference+8u);
      if(!admitted(gate) || !callback)continue;
      fp_report_event(f,*callback,{.type=2,.frame_word=word,.reference=effect,.flags=gate,.frame=frame});
    }
  return mask;
}
// 1CE950: moves a channel to `frame` (not below 0), reporting the events it
// passes. At its end a looping channel wraps to its loop frame (a random
// variant first, 1D3660, for a +12 bit 5 channel at loop frame 0) and
// reports the wrapped events; any other channel stops on its last frame. A
// callback that changes the channel's animation ends the move. The frame
// is published through 1CE810.
void fp_channel_advance(AnimationChannel channel,u32 frame,const AnimationManager& manager,const EventCallback* callback) {
  Frame& f=channel.f;
  M& m=f.world();
  u32 handle=channel.handle();
  const u32 graph=fp_channel_graph(channel);
  u32 row=0;
  if(channel.animation()!=0xffffu)row=weapon_animation::graph_animation(m,fp_tag(f,channel.graph_tag()),handle);
  const u32 previous=channel.frame();
  channel.clear_events();
  if(!row)refuse("1CE950","a channel without an animation (the original faults)");
  const auto animation=[&]{return AnimationDefinition{f,row};};  // the row changes with a variant
  const u32 last=f.sub(f.from_int(animation().frame_count()),epsilon_f);
  u32 next=f.greater(zero_f,frame)?zero_f:frame;
  channel.set_state(channel.state()&0xf1u);
  const auto unchanged=[&]{return u16(handle)==channel.child() && u16(handle>>16)==channel.animation();};
  u32 events=fp_animation_events(f,callback,graph,animation(),channel.controls(),previous,next);
  if(!unchanged())return;
  channel.set_events(channel.events()|events);
  if(f.at_least(next,last)) {
    events=fp_animation_events(f,callback,graph,animation(),channel.controls(),previous,0x7f7fffffu);  // FLT_MAX
    if(!unchanged())return;
    channel.set_events(channel.events()|events);
    const u32 controls=channel.controls();
    if(!(controls&2u)) {
      channel.set_state(channel.state()|8u);
      next=f.sub(f.from_int(animation().frame_count()),epsilon_f);
    } else {
      if((controls&0x20u) && !animation().loop_frame()) {
        weapon_animation::FrameWord variant;
        weapon_animation::manager_variant(m,manager.at,variant,channel.handle(),script_fp(f));
        handle=variant();
        channel.set_handle(handle);
        if(channel.controls()&8u)channel.set_rate(one);  // re-read after 1D3660, as the original
        row=weapon_animation::channel_animation(m,channel.at);
      }
      const u32 loop=f.from_int(sx16(u16(animation().loop_frame())));
      next=f.add(f.sub(next,last),loop);
      events=fp_animation_events(f,callback,graph,animation(),channel.controls(),loop,next);
      if(!unchanged())return;
      channel.set_events(channel.events()|events);
      channel.set_state(channel.state()|4u);
    }
  }
  if(!unchanged())return;
  weapon_animation::channel_set_frame(m,channel.at,next,script_fp(f));
}
// 1CEB70: one tick of a channel: clears its event words; a delayed channel
// counts its delay down; a playing one (controls bit 0, not holding and
// not at its end) advances by seconds * rate * 30 frames.
void fp_channel_tick(AnimationChannel channel,const AnimationManager& manager,const EventCallback* callback) {
  Frame& f=channel.f;
  const u32 delay=channel.delay();
  channel.clear_events();
  if(delay){channel.set_delay(delay-1u);return;}
  if(!(channel.controls()&1u) || (channel.state()&9u))return;
  const u32 frames=f.mul(f.mul(GameTime{f}.tick_seconds(),channel.rate()),fp_frame_rate_f);
  fp_channel_advance(channel,f.add(frames,channel.frame()),manager,callback);
}
// 1D3930: which of the primary and blend channels samples the +12 bits
// 7..9 (and manager +6C bit 5): with [0x47C1B4] and a set blend channel,
// a blend above 0.55 gives them to the blend channel; at or below 0.45 (or
// without a blend) the primary channel keeps them; in between nothing
// changes.
void fp_manager_blend_flags(AnimationManager& manager) {
  Frame& f=manager.f;
  AnimationChannel primary=manager.channel(0),blend=manager.channel(1);
  u32 primary_controls=primary.controls(),blend_controls=blend.controls();
  const bool blending=f.r8(0x47c1b4u) && blend.set() && !f.greater(fp_blend_low_f,manager.blend());
  if(blending) {
    if(!f.greater(manager.blend(),fp_blend_high_f))return;
    manager.set_flags(manager.flags()|0x20u);
    primary_controls|=0x380u;blend_controls&=~0x380u;
  } else {
    manager.set_flags(manager.flags()&0xdfu);
    primary_controls&=~0x380u;blend_controls|=0x380u;
  }
  if(primary.set())primary.set_controls(primary_controls);
  if(blend.set())blend.set_controls(blend_controls);
}
// 1D39C0: one tick of an animation manager. Its set channels advance while
// it has a graph (a held manager only checks). A transition channel that
// finished (+11 bits 1/3) hands over: the nodes interpolate over 0.267 s
// (1D38A0) unless its animation forbids it (+18 bit 1), the channel resets
// and the primary channel stops holding. Then the blend flags (1D3930);
// with [0x47C1B4] the blend channel follows the primary's fraction.
// Returns false when a set channel met a manager without a graph.
bool fp_manager_advance(AnimationManager manager,const EventCallback& callback,u32 node_count,u32 destination,u32 source) {
  Frame& f=manager.f;
  M& m=f.world();
  bool result=true;
  for(u32 n=0;n<3u && result;++n) {
    AnimationChannel channel=manager.channel(n);
    if(!channel.set())continue;
    if(!manager.has_graph()){result=false;continue;}
    if(!manager.held())fp_channel_tick(channel,manager,&callback);
  }
  AnimationChannel transition=manager.channel(2);
  if(transition.set() && (transition.state()&0xau)) {
    const u32 row=weapon_animation::graph_animation(m,fp_tag(f,transition.graph_tag()),transition.handle());
    if(!row)refuse("1D39C0","a transition channel without an animation (the original faults)");
    if(!(AnimationDefinition{f,row}.flags()&2u))
      animation_interpolation_start(m,node_count,manager.at,fp_state_seconds_f,destination,source,f.fp());
    // 1CE5D0's defaults, in 1D39C0's own store order.
    const u32 t=transition.at;
    f.w32(t,none);f.w16(t+4u,0xffffu);f.w16(t+6u,0xffffu);f.w32(t+0x1cu,zero_f);
    f.w8(t+0x10u,0);f.w8(t+0x11u,0);f.w16(t+0x12u,0);f.w32(t+0x18u,one);f.w16(t+0x14u,0);
    f.w16(t+0x16u,0);f.w32(t+8u,none);f.w8(t+0xcu,0xffu);f.w8(t+0xdu,0xffu);f.w16(t+0xeu,0xffffu);
    AnimationChannel primary=manager.channel(0);
    if(primary.set() && (primary.controls()&1u))primary.set_state(primary.state()&0xfeu);
  }
  fp_manager_blend_flags(manager);
  AnimationChannel blend=manager.channel(1);
  if(f.r8(0x47c1b4u) && blend.set()) {
    AnimationChannel primary=manager.channel(0);
    if(primary.animation()==0xffffu)refuse("1D39C0","a blend without a primary animation (the original faults)");
    const u32 primary_row=weapon_animation::graph_animation(m,fp_tag(f,primary.graph_tag()),primary.handle());
    if(!primary_row)refuse("1D39C0","a primary channel without an animation (the original faults)");
    u32 fraction=zero_f;
    if(const u32 frames=AnimationDefinition{f,primary_row}.frame_count();s32(sx16(u16(frames)))>0) {
      fraction=f.div(primary.frame(),f.from_int(frames));
      if(f.greater(zero_f,fraction))fraction=zero_f;
      else if(f.greater(fraction,one))fraction=one;
    }
    const u32 blend_row=weapon_animation::graph_animation(m,fp_tag(f,blend.graph_tag()),blend.handle());
    if(!blend_row)refuse("1D39C0","a blend channel without an animation (the original faults)");
    fp_channel_advance(blend,f.mul(f.from_int(AnimationDefinition{f,blend_row}.frame_count()),fraction),manager,&callback);
  }
  return result;
}
// 1D3860: one channel's tick, without events, on a manager with a graph.
bool fp_manager_channel_tick(AnimationManager manager,AnimationChannel channel) {
  if(!manager.has_graph() || !channel.set())return false;
  if(!manager.held())fp_channel_tick(channel,manager,nullptr);
  return true;
}
// The 18BE50 residual. Unbound, the refusal names the original's EBX
// (sound) and ESI (slot) at the 169C02 call, which the oracle compares.
u32 residual_sound_start(u32 sound,u32 object,u32 position,u32 direction,u32 slot) {
  if(!bound_residuals || !bound_residuals->sound_start) {
    char text[64];
    std::snprintf(text,sizeof text,"(EBX %08X, ESI %08X)",sound,slot);
    refuse("18BE50",std::string("first-person weapon sound event ")+text);
  }
  return bound_residuals->sound_start(sound,object,position,direction);
}
// 169BA2 through 169C21/169C33: a sound event of the first-person weapon's
// animation plays the sound on the weapon (18BE50 with [0x474D08] and
// [0x474CE8]), unless the user's camera is not first person and the
// reference is first-person only (+8 bit 3). The slot keeps the sound
// and its state key's low word.
void fp_report_event(Frame& f,const EventCallback& callback,const AnimationEvent& event) {
  if(!callback.first_person)refuse("1CF700","an animation event callback other than 169C21/169C33");
  const u32 view=fp_view(f,callback.user);
  if(!view || event.type!=1u || event.reference==none)return;
  if(fp_director_kind(f,callback.user) && (event.flags&8u))return;
  FirstPersonSlot slot{f,FirstPersonView{f,view}.slot(callback.slot)};
  slot.set_event_sound(residual_sound_start(event.reference,slot.weapon(),f.r32(0x474d08u),f.r32(0x474ce8u),slot.at));
  slot.set_event_state(f.r16(slot.manager().at+0x7cu));
}
// 16961F: the slot's state animation ended. Most states return to idle;
// some continue to a follow-up; 0x8000025 holds the manager instead. The
// next state is entered without the sound stop (169863).
void fp_state_ended(Frame& f,u32 user,u32 index) {
  FirstPersonView view{f,fp_view(f,user)};
  FirstPersonSlot slot{f,view.slot(index)};
  AnimationManager manager=slot.manager();
  const u32 definition=fp_tag(f,f.r32(fp_body(f,slot.weapon())));
  u32 next=none;
  switch(const u32 state=manager.state()) {
  case fp_idle_state:case 0x500000au:case 0x5000024u:case fp_fidget_state:case 0x8000063u:case 0x8000071u:
  case 0x9000062u:case 0x900006au:case 0x900006bu:case 0xb000065u:case 0xb0005b1u:case 0xb0005b2u:case 0xc000064u:
  case 0xc000077u:case 0xd000021u:case 0xe000607u:case 0xe000608u:case 0xe000609u:case 0xe00060au:
  case 0x11000074u:case 0x11000076u:case 0x140005b3u:
    next=fp_idle_state;break;
  case 0x6000006u:case 0x6000007u:
    // A weapon with +12C bit 17 stays while the state countdown runs; a
    // +292 kind-3 weapon continues 0x6000007 to 0xA000066.
    if(!((f.r32(definition+0x12cu)>>17)&1u) || s32(sx16(u16(slot.state_countdown())))<=0)next=fp_idle_state;
    if(f.r16(definition+0x292u)==3u && state==0x6000007u)next=0xa000066u;
    break;
  case 0x8000025u:manager.set_flags(manager.flags()|1u);return;
  case 0xa000066u:
    if(unit_actions::weapon_slot_flagged(f.world(),view.unit(),index))return;  // F0E00
    next=0x8000071u;break;
  case 0xa0005bbu:next=0xb0005b2u;break;
  case 0xb00006du:case 0x1000006cu:case 0x1000006eu:next=0xa000066u;break;
  case 0xc0005aeu:case 0x140005b0u:case 0x150005afu:next=0xb0005b1u;break;
  case 0xc000073u:next=0x11000074u;break;
  case 0xc000075u:next=0x11000076u;break;
  case 0x130005bcu:next=0x140005b3u;break;
  default:break;
  }
  if(next!=none)first_person_state(f,next,user,index,false);
}
// D28F0 -> E65A0: the unit flies: not seated, flight not blocked, flying
// (+348 bit 0) for at least 0.18 s of ticks (FISTP), in flight mode 1 or 3
// and without a parent.
bool fp_unit_flying(const FirstPersonUnit& unit) {
  Frame& f=unit.f;
  if(unit.seated() || unit.flight_blocked() || !(unit.physics_flags()&1u))return false;
  const u32 ticks=sse::fistp32(f32(f.mul(f.from_int(GameTime{f}.ticks_per_second()),fp_flying_ticks_f)),f.x87());
  if(s32(unit.flight_ticks())<s32(ticks))return false;
  if(unit.flight_mode()!=1u && unit.flight_mode()!=3u)return false;
  return unit.parent()==none;
}
// The weapon sway springs (four 121890 calls): the aim springs follow the
// unit's aim velocity (accel 72, limit 15, or the definition's nonzero
// +2E0/+2E4), the look springs the last look change per second clamped to
// [-1, 1], pitch negated (accel 27, limit 6, or +2E8/+2EC). Arguments
// pass through FLD/FSTP (x87_reload).
void fp_sway(Frame& f,FirstPersonView& view,const FirstPersonUnit& unit) {
  u32 aim_accel=fp_aim_rate_f,aim_limit=fifteen_f,look_accel=fp_look_rate_f,look_limit=six_f;
  if(view.slot(0) && f.r32(view.slot(0)+4u)!=none) {
    const u32 definition=fp_tag(f,f.r32(fp_body(f,f.r32(view.slot(0)+4u))));
    const auto tuned=[&](u32 at,u32& out){const u32 v=f.r32(definition+at);if(!f.equal(v,zero_f))out=v;};
    tuned(0x2e0u,aim_accel);tuned(0x2e4u,aim_limit);tuned(0x2e8u,look_accel);tuned(0x2ecu,look_limit);
  }
  GameTime time{f};
  const u32 low=x87_reload(fp_minus_one_f);
  for(u32 axis=0;axis<2u;++axis)
    fp_spring(f,view.aim_rate(axis),view.aim_value(axis),false,aim_limit,x87_reload(unit.aim_velocity(axis)),
        x87_reload(time.tick_seconds()),x87_reload(aim_accel),low,one);
  const auto clamp=[&](u32 x){return f.greater(fp_minus_one_f,x)?fp_minus_one_f:f.greater(x,one)?one:x;};
  const u32 yaw=clamp(f.div(f.r32(view.look_change(0)),time.tick_seconds()));
  const u32 pitch=clamp(f.mul(f.div(fp_minus_one_f,time.tick_seconds()),f.r32(view.look_change(1))));
  const std::array<u32,2> look{yaw,pitch};
  for(u32 axis=0;axis<2u;++axis)
    fp_spring(f,view.look_rate(axis),view.look_value(axis),false,look_limit,x87_reload(look[axis]),
        x87_reload(time.tick_seconds()),x87_reload(look_accel),low,one);
}
// The slot's motion channel (+98): moving above 0.1 (the aim velocity's x87
// length, 24570) plays 0x6000060 at a rate of the speed clamped to [0.5,
// 1]; airborne (CAEF0, with 1D4510's 0x60000D8 overlay in the mode) plays
// 0x60000D8 at 1.0; a flying unit (D28F0) neither. A channel playing the
// other overlay, or any while standing, resets (standing idle also fades
// the pose, 169B0A); a moving or airborne slot without one starts it
// (1D37D0) with the quick fade.
void fp_motion(Frame& f,u32 user,u32 index,FirstPersonSlot& slot,const FirstPersonUnit& unit,u32 base_key) {
  M& m=f.world();
  FirstPersonView view{f,fp_view(f,user)};
  AnimationManager manager=slot.manager();
  const u32 speed=x87_vector_length(unit.aim_velocity(0),unit.aim_velocity(1),unit.aim_velocity(2),f.x87());
  bool moving=false,airborne=false;
  if(f32(speed)>f32(fp_moving_f)) {  // FLD, FCOMIP: no denormal flush
    moving=true;
    const FirstPersonUnit holder{f,fp_body(f,view.unit())};
    airborne=!holder.seated() && ((holder.physics_flags()>>4)&1u);
  }
  if(fp_unit_flying(FirstPersonUnit{f,fp_body(f,view.unit())})){moving=false;airborne=false;}
  if(airborne) {
    weapon_animation::FrameWord handle;
    device_animation::overlay_lookup(m,fp_tag(f,manager.graph_tag()),fp_airborne_overlay,manager.mode_key(0),
        manager.mode_key(1),manager.mode_key(2),nullptr,nullptr,nullptr,handle);
    if(u16(handle()>>16)==0xffffu)airborne=false;
  }
  AnimationChannel motion=slot.motion();
  if(weapon_animation::channel_valid(m,motion.at)) {
    if(moving && !airborne)motion.set_rate(f.greater(half_f,speed)?half_f:f.greater(speed,one)?one:speed);
    else if(airborne)motion.set_rate(one);
    fp_manager_channel_tick(manager,motion);
    bool restart=true;
    if(moving || airborne) {
      const bool playing_airborne=motion.key()==fp_airborne_overlay;
      restart=airborne?!playing_airborne:playing_airborne;
    }
    if(restart) {
      channel_reset(f,motion.at);
      if(!moving && !airborne && manager.state()==fp_idle_state)
        fp_fade_previous(f,user,index,x87_reload(fp_state_seconds_f));
    }
  }
  if((moving || airborne) && !weapon_animation::channel_valid(m,motion.at)) {
    manager_start_named_overlay(m,manager.at,base_key,0x7000101u,airborne?fp_airborne_overlay:fp_moving_overlay,motion.at,
        airborne?0x3fu:0x803fu,f.fp());
    fp_fade_previous(f,user,index,x87_reload(fp_quick_seconds_f));
  }
}
// The slot's overlay channel (+B8): in state 0xB000067 the 0x12000068
// overlay restarts and runs at (weapon +188 + 1) * dt * 2 + frame / 30
// seconds modulo its length (37DC0A), times 30 frames; in any other state
// it resets.
void fp_overlay(Frame& f,FirstPersonSlot& slot,u32 weapon_body,u32 base_key) {
  M& m=f.world();
  AnimationManager manager=slot.manager();
  AnimationChannel overlay=slot.overlay();
  if(!weapon_animation::channel_valid(m,overlay.at))return;
  if(manager.state()!=fp_overlay_state){channel_reset(f,overlay.at);return;}
  manager_start_named_overlay(m,manager.at,base_key,0x7000101u,fp_overlay_name,overlay.at,0x803fu,f.fp());
  const u32 seconds=x87_overlay_seconds(f.r32(weapon_body+0x188u),GameTime{f}.tick_seconds(),overlay.frame(),f.x87());
  const u32 length=device_animation::channel_seconds(m,overlay.at,script_fp(f));
  weapon_animation::channel_set_frame(m,overlay.at,x87_fmod_frame(seconds,length,f.x87()),script_fp(f));
}
// The idle fidget: with the springs, inputs and settle value at rest in
// the idle state, the slot counts idle ticks against a random wait
// (259B0 over [[0x4ECC1C]+134]+94..+98 seconds, 1494F0 to ticks); past it
// a seed draw at or above +9C starts the fidget 0x600005F. Any motion
// clears the count and ends a running fidget.
void fp_fidget(Frame& f,u32 user,u32 index,FirstPersonSlot& slot) {
  FirstPersonView view{f,fp_view(f,user)};
  UserInput input{f,user};
  const u32 magnitude=f.greater(input.magnitude(0),input.magnitude(1))?input.magnitude(0):input.magnitude(1);
  const bool still=f.equal(magnitude,zero_f) && u16(input.action())==0xffffu && f.equal(f.r32(slot.settle_value()),zero_f) &&
      f.equal(f.r32(view.aim_value(0)),zero_f) && f.equal(f.r32(view.aim_value(1)),zero_f) &&
      f.equal(f.r32(view.look_value(0)),zero_f) && f.equal(f.r32(view.look_value(1)),zero_f);
  const u32 state=slot.manager().state();
  if(!still) {
    slot.set_idle_ticks(0);
    if(state==fp_fidget_state)first_person_state(f,fp_idle_state,user,index,true);
    return;
  }
  if(state!=fp_idle_state){slot.set_idle_ticks(0);return;}
  const u32 globals=f.r32(f.r32(character_table)+0x134u);
  const u32 random=f.r32(0x4f3cd8u);  // +4 the seed
  const auto draw=[&]{const u32 seed=f.r32(random+4u)*0x19660du+0x3c6ef35fu;f.w32(random+4u,seed);return seed>>16;};
  if(!slot.idle_wait()) {
    const u32 wait=x87_random_range(draw(),x87_reload(f.r32(globals+0x94u)),x87_reload(f.r32(globals+0x98u)),f.x87());
    slot.set_idle_wait(sse::fistp32(f32(f.mul(f.from_int(GameTime{f}.ticks_per_second()),wait)),f.x87()));
  }
  const u32 ticks=(slot.idle_ticks()+1u)&0xffffu;
  slot.set_idle_ticks(ticks);
  if(s32(sx16(u16(ticks)))<=s32(sx16(u16(slot.idle_wait()))))return;
  slot.set_idle_wait(0);
  if(!x87_random_below(draw(),f.r32(globals+0x9cu),f.x87()))first_person_state(f,fp_fidget_state,user,index,true);
}
// 169C46 (user).
void first_person_update(Frame& f,u32 user) {
  M& m=f.world();
  FirstPersonView view{f,fp_view(f,user)};
  if(view.unit()==none)return;
  const FirstPersonUnit unit{f,fp_body(f,view.unit())};
  const u32 base_key=fp_base_key(f,user);
  fp_sway(f,view,unit);
  // The look input's angle changes since the last update (C1F90), or the
  // zero vector ([0x474D24]) on the first; then the new angles.
  UserInput input{f,user};
  const u32 flags=view.flags();
  if(flags&1u) {
    for(u32 axis=0;axis<2u;++axis)
      f.w32(view.look_change(axis),fp_angle_change(f,input.look(axis),x87_reload(f.r32(view.previous_look(axis)))));
  } else {
    const u32 zero=f.r32(0x474d24u);
    for(u32 axis=0;axis<2u;++axis)f.w32(view.look_change(axis),f.r32(zero+4u*axis));
  }
  for(u32 axis=0;axis<2u;++axis)f.w32(view.previous_look(axis),input.look(axis));
  view.set_flags(flags|1u);
  for(u32 index=0;index<2u;++index) {
    FirstPersonSlot slot{f,view.slot(index)};
    if(const u32 countdown=slot.state_countdown();s32(sx16(u16(countdown)))>0)slot.set_state_countdown(countdown-1u);
    // 16B1BC: visible in a first-person camera with a valid manager (B87B0).
    bool visible=false;
    if(view.unit()!=none && slot.weapon()!=none && !fp_director_kind(f,user))visible=manager_valid(m,slot.manager().at);
    fp_slot_visible(f,user,index,visible);
    if(const u32 weapon=slot.weapon();weapon!=none) {
      const u32 bank=fp_bank(f,user,index);
      const EventCallback events{true,user,index};
      // The state animation's end (primary channel +11 bits 1/3).
      if(fp_manager_advance(slot.manager(),events,slot.node_count(),bank+0x800u,bank) && (f.r8(slot.at+0x19u)&0xau))
        fp_state_ended(f,user,index);
      fp_motion(f,user,index,slot,unit,base_key);
      fp_overlay(f,slot,fp_body(f,weapon),base_key);
      // The settle spring (accel 9, limit 6) toward 0; at 1 its rate stops.
      fp_spring(f,slot.settle_rate(),slot.settle_value(),false,six_f,zero_f,x87_reload(GameTime{f}.tick_seconds()),
          fp_settle_rate_f,zero_f,one);
      if(f.equal(f.r32(slot.settle_value()),one))f.w32(slot.settle_rate(),zero_f);
      // 1D5790: the manager's fades (+64, cleared when its +67 bit 1 is set, then +60).
      AnimationManager manager=slot.manager();
      fp_fade_tick(f,manager.fade(1));
      if(f.r8(manager.fade(1)+3u)&2u){f.w8(manager.fade(1)+1u,0);f.w8(manager.fade(1),0);f.w8(manager.fade(1)+3u,0);}
      fp_fade_tick(f,manager.fade(0));
      fp_fidget(f,user,index,slot);
      if(slot.visible()) {  // 16A479 -> 16A4DD
        if(bound_residuals && bound_residuals->sampler_entry)bound_residuals->sampler_entry(user,index);
        first_person_animation(f,user,index);
      }
    }
    // The resource prediction countdown (16B2B9 unless [0x51DF50]+5 loads).
    const u32 ticks=(slot.prediction_countdown()-1u)&0xffffu;
    slot.set_prediction_countdown(ticks);
    if(s32(sx16(u16(ticks)))<=0) {
      const u32 loading=f.r32(0x51df50u);
      if(!loading || !f.r8(loading+5u))fp_slot_predict(f,user,index);
    }
  }
  // Without a visible slot the view is no longer updated; its fade ticks.
  if(!(f.r8(view.slot(0))&1u) && !(f.r8(view.slot(1))&1u))view.set_flags(view.flags()&~1u);
  if(f.r8(view.fade()+1u))fp_fade_tick(f,view.fade());
}
}

NodeRebuildDebugLog& node_rebuild_debug_log(){thread_local NodeRebuildDebugLog log;return log;}
bool node_mask_empty(const M& m,std::uint32_t mask) {
  bool empty=true;
  for(u32 n=8;n-->0;)empty=!r32(m,mask+4u*n) && empty;
  return empty;
}
void node_mask_invert(M& m,std::uint32_t mask) {
  for(u32 n=0;n<7;++n)w32(m,mask+4u*n,~r32(m,mask+4u*n));
  const u32 last=~r32(m,mask+0x1cu);
  w32(m,mask+0x1cu,last);
  w32(m,mask+0x1cu,last&0xffu);
}
void node_mask_limit(M& m,std::uint32_t mask,std::uint32_t count) {
  const u32 last=u32(s32(count+0x1fu)>>5)-1u,words=last+1u;
  if(s32(words)<8)for(u32 n=0,k=8u-words;n<k;++n)w32(m,mask+(words+n)*4u,0);
  const u32 tail=count&0x1fu;
  const u32 keep=tail?0xffffffffu>>(32u-tail):0xffffffffu;
  w32(m,mask+last*4u,r32(m,mask+last*4u)&keep);
}
std::uint32_t node_mask_last(const M& m,std::uint32_t mask,std::uint32_t count) {
  const u32 words=u32(s32(count+0x1fu)>>5),tail=count&0x1fu;
  const u32 keep=tail?0xffffffffu>>(32u-tail):0xffffffffu;
  const auto highest=[](u32 value)->u32{return value?u32(31-std::countl_zero(value)):none;};
  u32 found=highest(r32(m,mask+words*4u-4u)&keep);
  if(s32(found)>=0)found=words*32u+found-32u;
  if(found!=none)return found;
  for(u32 word=words-2u,base=word*32u;s32(word)>=0;--word,base-=32u) {
    found=highest(r32(m,mask+word*4u));
    if(s32(found)>=0)found+=base;
    if(found!=none)break;
  }
  return found;
}
void flags_assign_bit(M& m,std::uint32_t flags,std::uint32_t bit,bool set) {
  const u32 value=1u<<(bit&31u);
  if(set)w32(m,flags,r32(m,flags)|value);
  else w32(m,flags,r32(m,flags)&~value);
}
bool object_animation_frozen(const M& m,std::uint32_t object) {
  return object!=none && r8(m,object_body(m,object)+0xb3u)!=0;
}
bool animation_manager_valid(const M& m,std::uint32_t manager){return manager_valid(m,manager);}
std::uint32_t animation_definition(const M& m,std::uint32_t graph,std::uint32_t id){return animation_lookup(m,graph,id);}
std::uint32_t animation_manager_definition(const M& m,std::uint32_t manager){return manager_animation(m,manager);}
std::uint32_t biped_physics_height(const M& m,std::uint32_t object,std::uint32_t mxcsr) {
  Frame f(const_cast<M&>(m),mxcsr);
  return physics_height(f,object);
}
void model_default_orientations(M& m,std::uint32_t model,std::uint32_t orientations) {
  Frame f(m,0x1f80u);
  default_orientations(f,model,orientations);
}
void orientation_matrix(M& m,std::uint32_t out,std::uint32_t q,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  orientation(f,out,q);
}
std::uint32_t matrix_product(M& m,std::uint32_t a,std::uint32_t b,std::uint32_t out,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  return product(f,a,b,out);
}
void object_root_matrix(M& m,std::uint32_t up,std::uint32_t forward,bool parent_mirrored,std::uint32_t position,
    std::uint32_t scale_bits,std::uint32_t mirror,std::uint32_t parent,std::uint32_t out,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  root_matrix(f,up,forward,parent_mirrored,position,scale_bits,mirror,parent,out);
}
void object_type_node_hooks(M& m,std::uint32_t object,std::uint32_t nodes,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  type_node_hooks(f,object,nodes);
}
void unit_node_matrix_hook(M& m,std::uint32_t object,std::uint32_t nodes,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  unit_hook(f,object,nodes);
}
void scenery_node_matrix_hook(M& m,std::uint32_t object,std::uint32_t nodes,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  scenery_hook(f,object,nodes);
}
void device_orientation_hook(M& m,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  device_orientation_hook(f,object,mask,count,orientations);
}
void vehicle_orientation_hook(M& m,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  vehicle_animations(f,object,count,orientations,mask);
}
void unit_orientation_hook(M& m,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  unit_orientation_hook(f,object,mask,count,orientations);
}
void animation_aim_channel(M& m,std::uint32_t channel,std::uint32_t yaw,std::uint32_t pitch,std::uint32_t weight,
    std::uint32_t mask,std::uint32_t count,std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  sample_aim_channel(f,channel,yaw,pitch,weight,mask,count,orientations);
}
void animation_fraction_channel(M& m,std::uint32_t channel,std::uint32_t mask,std::uint32_t fraction,
    std::uint32_t weight,std::uint32_t count,std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  sample_fraction_channel(f,channel,mask,fraction,weight,count,orientations);
}
AnimationResourceReaderScope::AnimationResourceReaderScope(const AnimationResourceReader& reader):previous_(bound_reader) {
  bound_reader=&reader;
}
AnimationResourceReaderScope::~AnimationResourceReaderScope(){bound_reader=previous_;}
void object_physics_nodes(M& m,std::uint32_t component,std::uint32_t mask,std::uint32_t height,NodeRebuildFp& fp) {
  Frame f(m,fp);
  physics_node_delivery(f,component,mask,8u,height);
}
bool crate_physics_sphere(M& m,std::uint32_t component,std::uint32_t center,std::uint32_t radius,NodeRebuildFp& fp) {
  Frame f(m,fp);
  return crate_sphere(f,component,center,radius);
}
bool object_update_bounds(M& m,std::uint32_t object,NodeRebuildFp& fp) {
  Frame f(m,fp);
  return update_bounds(f,object);
}
void object_update_node_matrices(M& m,std::uint32_t object,NodeRebuildFp& fp) {
  Frame f(m,fp);
  update_node_matrices(f,object);
}
void animation_sample_channel(M& m,std::uint32_t channel,std::uint32_t weight,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  sample_channel(f,channel,weight,mask,count,orientations);
}
void animation_sample_channels(M& m,std::uint32_t manager,std::uint32_t weight,std::uint32_t mask,std::uint32_t orientations,
    std::uint32_t count,NodeRebuildFp& fp) {
  Frame f(m,fp);
  sample_channels(f,manager,weight,mask,orientations,count);
}
void object_function_overlays(M& m,std::uint32_t object,std::uint32_t manager,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp& fp) {
  Frame f(m,fp);
  overlays(f,object,manager,mask,count,orientations);
}
std::uint32_t object_deferred_node_matrices(M& m,std::uint32_t object,std::uint32_t selected,std::uint32_t matrices,
    NodeRebuildFp& fp) {
  Frame f(m,fp);
  return deferred_node_matrices(f,object,selected,matrices);
}
FirstPersonResidualScope::FirstPersonResidualScope(const FirstPersonResiduals& residuals):previous_(bound_residuals) {
  bound_residuals=&residuals;
}
FirstPersonResidualScope::~FirstPersonResidualScope(){bound_residuals=previous_;}
void first_person_animation(M& m,std::uint32_t user,std::uint32_t slot,NodeRebuildFp& fp) {
  Frame f(m,fp);
  first_person_animation(f,user,slot);
}
void first_person_rebuild(M& m,std::uint32_t user,std::uint32_t slot,NodeRebuildFp& fp) {
  Frame f(m,fp);
  first_person_rebuild(f,user,slot);
}
void first_person_state(M& m,std::uint32_t key,std::uint32_t user,std::uint32_t slot,bool stop_sound,NodeRebuildFp& fp) {
  Frame f(m,fp);
  first_person_state(f,key,user,slot,stop_sound);
}
void first_person_update(M& m,std::uint32_t user,NodeRebuildFp& fp) {
  Frame f(m,fp);
  first_person_update(f,user);
}
void animation_node_hierarchy(M& m,std::uint32_t graph,std::uint32_t out,std::uint32_t orientations,std::uint32_t root,
    std::uint32_t mirrored,std::uint32_t source,NodeRebuildFp& fp) {
  Frame f(m,fp);
  node_hierarchy(f,graph,out,orientations,root,mirrored,source);
}
void unit_frame_overlays(M& m,std::uint32_t object,std::uint32_t manager,std::uint32_t sample_mask,std::uint32_t count,
    std::uint32_t orientations,std::uint32_t flags,NodeRebuildFp& fp) {
  Frame f(m,fp);
  unit_frame_overlays(f,object,manager,sample_mask,count,orientations,flags);
}
void unit_aim_overlays(M& m,std::uint32_t object,std::uint32_t manager,std::uint32_t sample_mask,std::uint32_t count,
    std::uint32_t orientations,std::uint32_t flags,NodeRebuildFp& fp) {
  Frame f(m,fp);
  unit_aim_overlays(f,object,manager,sample_mask,count,orientations,flags);
}
void animation_node_flags(M& m,std::uint32_t block,std::uint32_t mask) {
  Frame f(m,0x1f80u);
  animation_node_flags(f,block,mask);
}
void unit_hand_ik(M& m,std::uint32_t object,std::uint32_t mask,std::uint32_t count,std::uint32_t matrices,
    NodeRebuildFp& fp) {
  Frame f(m,fp);
  unit_hand_ik(f,object,mask,count,matrices);
}
void hand_ik(M& m,std::uint32_t marker,std::uint32_t target,std::uint32_t node,NodeRebuildFp& fp) {
  Frame f(m,fp);
  hand_ik(f,marker,target,node);
}
void euler_matrix(M& m,std::uint32_t out,std::uint32_t a,std::uint32_t b,std::uint32_t c,NodeRebuildFp& fp) {
  Frame f(m,fp);
  euler_matrix(f,out,a,b,c);
}
void object_rebuild_nodes(M& m,std::uint32_t object,NodeRebuildFp& fp) {
  Frame f(m,fp);
  rebuild(f,object);
}
std::uint32_t animation_interpolation_fraction(const M& m,std::uint32_t state,std::uint32_t mxcsr) {
  Frame f(const_cast<M&>(m),mxcsr);
  return interpolation_fraction(f,state);
}
void animation_crossfade(M& m,std::uint32_t state,std::uint32_t current,std::uint32_t count,std::uint32_t mask,
    std::uint32_t previous,NodeRebuildFp& fp) {
  Frame f(m,fp);
  crossfade(f,state,current,count,mask,previous);
}
std::uint32_t animation_blend_fraction(const M& m,std::uint32_t manager,std::uint32_t mxcsr) {
  Frame f(const_cast<M&>(m),mxcsr);
  return blend_fraction(f,manager);
}
std::uint32_t animation_codec_entry(const M& m,std::uint32_t graph,std::uint32_t id){return codec_entry(m,graph,id);}
void object_sample_animation(M& m,std::uint32_t object,std::uint32_t mode,std::uint32_t manager,
    std::uint32_t orientations,std::uint32_t count,std::uint32_t mask,NodeRebuildFp& fp) {
  Frame f(m,fp);
  sample_animation(f,object,mode,manager,orientations,count,mask);
}
void animation_codec_track(M& m,std::uint32_t decoder,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  codec_track(f,decoder);
}
void animation_codec_frames(M& m,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  codec_frames(f);
}
void animation_codec_setup(M& m,const AnimationCodecSetup& s,std::uint32_t mxcsr) {
  Frame f(m,mxcsr);
  codec_setup(f,s.block,s.frame,s.weight,s.graph,s.animation,s.count,s.entry,s.mask,s.orientations,s.force,s.flag,
      s.second_frame,s.second_weight);
}
std::uint32_t animation_resource_load(M& m,std::uint32_t row,std::uint32_t name) {
  Frame f(m,0x1f80u);
  return resource_load(f,row,name);
}
std::uint32_t animation_resource_data(M& m,std::uint32_t graph,std::uint32_t id) {
  Frame f(m,0x1f80u);
  return resource_data(f,graph,id);
}
void animation_decode(M& m,std::uint32_t graph,std::uint32_t id,std::uint32_t count,std::uint32_t frame,
    std::uint32_t weight,std::uint32_t entry,std::uint32_t mask,std::uint32_t orientations,bool force,NodeRebuildFp& fp) {
  Frame f(m,fp);
  animation_codec(f,graph,id,count,frame,weight,entry,mask,orientations,force);
}
}
