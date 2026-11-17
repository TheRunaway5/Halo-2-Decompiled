// Track "havok": the factory's Havok owners bound to the existing src/physics
// natives (see docs/native_object_factory_tracks/havok.md). Every service
// below is a native owner or a named refusal; nothing executes original code.
#include "halo2/native_object_factory_havok_tu15.hpp"
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_component_reset.hpp"
#include "halo2/native_tu15_component_state.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include "halo2/native_tu15_pair_consumer.hpp"
#include "halo2/native_tu15_phantom_body_velocity.hpp"
#include "halo2/native_tu15_phantom_overlap_add.hpp"
#include "halo2/native_tu15_phantom_pair_delete.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include "halo2/native_tu15_simple_phantom_overlap_remove.hpp"
#include "halo2/native_tu15_stack_arena.hpp"
#include "halo2/native_tu15_body_references.hpp"
#include "halo2/native_tu15_centered_shape_bounds.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include "halo2/native_tu15_dirty_island_objects.hpp"
#include "halo2/native_tu15_gameplay_policy_destruction.hpp"
#include "halo2/native_tu15_island_construction.hpp"
#include "halo2/native_tu15_island_lifetime.hpp"
#include "halo2/native_tu15_pair_creation_queue_binding.hpp"
#include "halo2/native_tu15_pair_handlers.hpp"
#include "halo2/native_tu15_shape_bounds.hpp"
#include "halo2/native_tu15_sphere_shape_bounds.hpp"
#include "native_object_factory_tu15_internal.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <xmmintrin.h>

namespace halo2::native_object_factory_tu15 {
namespace {
using detail::M;
using detail::r16;
using detail::r32;
using detail::r8;
using detail::refuse;
using detail::s32;
using detail::w16;
using detail::w32;
namespace sse=native_campaign_script_fp;
constexpr std::uint32_t havok_allocator_global=0x48c938u;  // allocator object, vtable 0x45F4CC
constexpr std::uint32_t array_allocate_method=0x149da0u,array_release_method=0x149e00u;
constexpr std::uint32_t pool_allocate_method=0x233250u,pool_release_method=0x233420u;
constexpr std::uint32_t arena_allocate_method=0x2e10a0u,arena_release_method=0x2e1110u;  // allocator +24/+28
constexpr std::uint32_t default_listener=0x2e4d60u;          // RET 4
constexpr std::uint32_t havok_world_global=0x52be48u;
constexpr std::uint32_t broadphase_add_method=0x2ec4b0u,broadphase_remove_method=0x2ec9b0u;
constexpr std::uint32_t aabb_phantom_aabb=0x315170u,aabb_phantom_destructor=0x315620u;
constexpr std::uint32_t aabb_phantom_add_overlap=0x315300u,aabb_phantom_remove_overlap=0x315100u;
constexpr std::uint32_t group_filter=0x2e9a00u,phantom_pair_create=0x31ffc0u,phantom_pair_delete=0x320040u;

std::string hex(std::uint32_t value) {
  char text[16];std::snprintf(text,sizeof text,"%X",value);return text;
}

// World memory as the Havok natives need it. Counter updates (DEC/INC
// [mem]) go through a checked read-modify-write: the world's own when it
// has one, else one read then one store, as the original's RMW. Addresses
// in [window, 2^32) are the owners' private stack locals (the original
// keeps them on its stack, outside the world): never forwarded. The top 32
// bytes are 1D8470's two 31C450 results; below them, owners push frames.
class HavokMemory final:public SourcePhysicsContactMemory,public NativeTu15RetainedCounterStorage {
  // A private frame presented at the original's stack address (identity).
  struct Alias {std::uint32_t identity,size,at;};
  M& base_;std::vector<std::byte> frame_;std::uint32_t top_=havok_locals;std::vector<Alias> aliases_;
  static bool local(std::uint32_t address,std::size_t size){
    return address>=window && std::uint64_t(address)+size<=0x100000000ull;}
  std::uint32_t redirect(std::uint32_t address,std::size_t size)const{
    for(auto alias=aliases_.rbegin();alias!=aliases_.rend();++alias)
      if(address>=alias->identity && std::uint64_t(address)+size<=std::uint64_t(alias->identity)+alias->size)
        return alias->at+(address-alias->identity);
    return address;
  }
public:
  static constexpr std::uint32_t window=0xffffe000u,havok_locals=0xffffffe0u;
  explicit HavokMemory(M& base):base_(base),frame_(0x2000u){}
  std::uint32_t top()const{return top_;}
  // A zeroed, 16-byte aligned private frame of `bytes`.
  std::uint32_t push(std::uint32_t bytes){
    const auto at=(top_-bytes)&~15u;
    if(at<window+0x40u || at>top_)throw std::runtime_error("Native object factory: havok private frames exhausted");
    std::fill_n(frame_.begin()+(at-window),top_-at,std::byte{});top_=at;return at;
  }
  void pop(std::uint32_t mark){top_=mark;}
  // Present private bytes at `identity` (an original stack address whose
  // value the original publishes); accesses there never reach the world.
  void alias(std::uint32_t identity,std::uint32_t size,std::uint32_t at){aliases_.push_back({identity,size,at});}
  void unalias(){aliases_.pop_back();}
  void read(std::uint32_t address,std::span<std::byte> output)const override{
    address=redirect(address,output.size());
    if(local(address,output.size())){std::copy_n(frame_.begin()+(address-window),output.size(),output.begin());return;}
    base_.read(address,output);}
  void write(std::uint32_t address,std::span<const std::byte> input)override{
    address=redirect(address,input.size());
    if(local(address,input.size())){std::copy(input.begin(),input.end(),frame_.begin()+(address-window));return;}
    base_.write(address,input);}
  NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t address,unsigned width,std::uint32_t adjustment)override{
    if(auto* counters=dynamic_cast<NativeTu15RetainedCounterStorage*>(&base_))
      return counters->adjust_counter(address,width,adjustment);
    std::array<std::byte,4> before{};
    base_.read(address,std::span(before).first(width));
    const auto update=native_tu15_adjust_counter_value(before,width,adjustment);
    base_.write(address,std::span<const std::byte>(update.written).first(width));
    return update;
  }
};
// An owner's private stack frame, released on exit. With an identity (the
// original's stack address of a local whose address the original stores in
// the world), `at` is that identity and the bytes stay private.
struct Frame {
  HavokMemory& m;std::uint32_t mark,at;bool aliased=false;
  Frame(HavokMemory& memory,std::uint32_t bytes):m(memory),mark(memory.top()),at(memory.push(bytes)){}
  Frame(HavokMemory& memory,std::uint32_t bytes,std::uint32_t identity):Frame(memory,bytes){
    if(identity){m.alias(identity,bytes,at);at=identity;aliased=true;}
  }
  ~Frame(){if(aliased)m.unalias();m.pop(mark);}
  Frame(const Frame&)=delete;Frame& operator=(const Frame&)=delete;
};
// The original ESP `n` bytes below an owner's entry ESP; 0 (no stack model,
// production) stays 0 and private frames keep their own addresses.
std::uint32_t below(std::uint32_t sp,std::uint32_t n){return sp?sp-n:0u;}
// AND ESP,-16 after PUSH EBP.
std::uint32_t aligned_frame(std::uint32_t sp){return sp?(sp-4u)&~15u:0u;}
// Nested owners share one adapter, so private frames never shadow each other.
template<class F> decltype(auto) with_havok(M& m,F&& f) {
  if(auto* h=dynamic_cast<HavokMemory*>(&m))return f(*h);
  HavokMemory h(m);return f(h);
}

// The heap (pool) +10/+14 owners the 149DA0/149E00 routes call.
NativeTu15ArrayAllocatorServices pool_services(M& m) {
  NativeTu15ArrayAllocatorServices heap;
  heap.allocate_owner=[&m](const NativeTu15ArrayAllocationRequest& r,HavokFp&) {
    if(r.owner!=pool_allocate_method)refuse("149DA0","heap allocation method "+hex(r.owner));
    NativeTu15PoolMemory pool(m,r.receiver);
    return native_tu15_pool_allocate(pool,r.bytes);
  };
  heap.release_owner=[&m](const NativeTu15ArrayReleaseRequest& r,HavokFp&) {
    if(r.owner!=pool_release_method)refuse("149E00","heap release method "+hex(r.owner));
    if(std::bit_cast<std::int32_t>(r.bytes)>0x2000)refuse("233420","a pool release above 0x2000 bytes (import free)");
    NativeTu15PoolMemory pool(m,r.receiver);
    native_tu15_pool_release(pool,r.data,r.bytes);
  };
  heap.default_heap=[](HavokFp&)->std::uint32_t{refuse("149930","the lazy default Havok heap");};
  return heap;
}
}

NativeTu15ArrayGrowthServices havok_allocator(M& m) {
  NativeTu15ArrayGrowthServices s;
  s.allocate=[&m](const NativeTu15ArrayAllocationRequest& r,HavokFp& fp) {
    if(r.owner!=array_allocate_method)refuse("[0x48C938]+10","allocator method "+hex(r.owner));
    NativeTu15ArrayAllocatorMemory allocator(m);
    return native_tu15_route_array_allocation(allocator,r.bytes,r.memory_class,fp,pool_services(m));
  };
  s.release=[&m](const NativeTu15ArrayReleaseRequest& r,HavokFp& fp) {
    if(r.owner!=array_release_method)refuse("[0x48C938]+14","allocator method "+hex(r.owner));
    NativeTu15ArrayAllocatorMemory allocator(m);
    native_tu15_route_array_release(allocator,r.data,r.bytes,r.memory_class,fp,pool_services(m));
  };
  return s;
}

std::uint32_t havok_array_allocate(M& m,std::uint32_t bytes,std::uint32_t memory_class,HavokFp& fp) {
  const auto receiver=r32(m,havok_allocator_global);
  return havok_allocator(m).allocate({receiver,r32(m,r32(m,receiver)+0x10u),bytes,memory_class},fp);
}

void havok_array_release(M& m,std::uint32_t data,std::uint32_t bytes,std::uint32_t memory_class,HavokFp& fp) {
  const auto receiver=r32(m,havok_allocator_global);
  havok_allocator(m).release({receiver,r32(m,r32(m,receiver)+0x14u),data,bytes,memory_class},fp);
}

void havok_grow_array(M& m,std::uint32_t header,std::uint32_t stride,HavokFp& fp) {
  NativeTu15ArrayGrowthMemory array(m,{header});
  native_tu15_grow_array(array,stride,fp,havok_allocator(m));
}

namespace {
NativeTu15IslandActivationServices activation_services(M& m) {
  NativeTu15IslandActivationServices s;
  s.grow_array=[&m](NativeTu15PhysicsArrayHeader header,std::uint32_t stride,HavokFp& fp) {
    havok_grow_array(m,header.identity,stride,fp);
  };
  // 322660 handles the default 2E4D60 listener itself; any other is refused.
  s.island_activated=[](NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner owner,NativeTu15PhysicsIsland,HavokFp&) {
    refuse("322660","world island-activation listener "+hex(owner.identity));
  };
  s.body_activated=[](NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner owner,NativeTu15PhysicsBody,HavokFp&) {
    refuse("322660","entity activation listener "+hex(owner.identity));
  };
  s.destroy_body=[](NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner owner,std::uint32_t,HavokFp&) {
    refuse("3165D0","body destructor "+hex(owner.identity));
  };
  return s;
}
}

void havok_activate_body(M& m,std::uint32_t body,HavokFp& fp) {
  with_havok(m,[&](HavokMemory& memory){
    native_tu15_activate_physics_body(memory,{body},fp,activation_services(memory));});
}

void havok_component_reset(M& m,std::uint32_t component,HavokFp& fp) {
  with_havok(m,[&](HavokMemory& memory){
    NativeTu15ComponentResetServices s;
    s.allocation=havok_allocator(memory);
    // 1D8470 runs the 2E4D60 default destructor itself; any other is refused.
    s.destroy_body=[](NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner owner,std::uint32_t,HavokFp&,unsigned) {
      refuse("1D8470","body destructor "+hex(owner.identity));
    };
    // 31C450's two removed-value results are 1D8470's stack locals.
    native_tu15_reset_physics_component(memory,component,{HavokMemory::havok_locals,HavokMemory::havok_locals+4u},fp,0,s);
  });
}

void havok_component_activate(M& m,std::uint32_t component,HavokFp& fp) {
  if(!((r32(m,component+4u)>>5)&1u))return;
  for(std::uint32_t n=0,offset=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(r32(m,component+0x74u));
      ++n,offset+=0x60u)
    havok_activate_body(m,r32(m,r32(m,component+0x70u)+offset+0x40u),fp);
}

namespace {
// The body's motion (+3C) vtable +18 kind getter: the six literal owners.
std::uint32_t motion_kind(const M& m,std::uint32_t body,const char* owner) {
  const auto motion=r32(m,body+0x3cu);
  const auto method=r32(m,r32(m,motion)+0x18u);
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
}

void havok_object_component_velocities(M& base,std::uint32_t object,std::uint32_t linear,
    std::uint32_t angular,bool force,HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    const auto datum=r32(m,detail::object_body(m,object)+0xb4u);
    if(datum==detail::none)return;
    const auto component=r32(m,r32(m,0x52be5cu)+0x44u)+(datum&0xffffu)*0xa0u;
    const auto body=[&](std::uint32_t slot){
      return r32(m,r32(m,component+0x70u)+slot*0x60u+0x40u);
    };
    // 1D9EC0: without force, delivery requires at least one active island.
    // An absent island is inactive (315C90), and the count is signed/live.
    if(!force) {
      bool active=false;
      for(std::uint32_t k=0;s32(k)<s32(r32(m,component+0x74u));++k) {
        const auto island=r32(m,body(k)+0x44u);
        if(island && r8(m,island+0x29u)){active=true;break;}
      }
      if(!active)return;
    }
    const auto motion_type=[&](std::uint32_t rigid_body){return motion_kind(m,rigid_body,"1CD6A0");};
    const auto visit=[&](std::uint32_t slot){
      const auto rigid_body=body(slot);
      if(r8(m,rigid_body+0x40u) || motion_type(rigid_body)==6u)return;
      const auto activation=activation_services(m);
      if(linear)native_tu15_set_phantom_body_velocity(m,component,slot,NativeTu15PhantomVelocityKind::linear,
          [&](unsigned lane){return r32(m,linear+lane*4u);},fp,activation);
      if(angular)native_tu15_set_phantom_body_velocity(m,component,slot,NativeTu15PhantomVelocityKind::angular,
          [&](unsigned lane){return r32(m,angular+lane*4u);},fp,activation);
    };
    const auto header=detail::object_row(m,object);
    if((1u<<(r8(m,header+3u)&31u))&2u) {
      const auto slot=std::bit_cast<std::int8_t>(r8(m,component+0x18u));
      if(slot>=0 && slot<s32(r32(m,component+0x74u)))visit(std::uint32_t(slot));
    } else {
      for(std::uint32_t slot=0;s32(slot)<s32(r32(m,component+0x74u));++slot)visit(slot);
    }
    if(force && (r32(m,component+4u)&0x20u))havok_component_activate(m,component,fp);
  });
}

namespace {
// One 16-byte MOVAPS store.
void w128(M& m,std::uint32_t at,const std::array<std::byte,16>& value){m.write(at,value);}
std::array<std::byte,16> r128(const M& m,std::uint32_t at){std::array<std::byte,16> value{};m.read(at,value);return value;}
// One 8-byte MOVQ load/store.
std::uint64_t r64(const M& m,std::uint32_t at){return r32(m,at)|std::uint64_t(r32(m,at+4u))<<32;}
void w64(M& m,std::uint32_t at,std::uint64_t value){
  std::array<std::byte,8> bytes{};for(unsigned k=0;k<8;++k)bytes[k]=std::byte(value>>(8u*k));m.write(at,bytes);}
bool less(std::uint32_t a,std::uint32_t b){return s32(a)<s32(b);}
}

std::uint32_t havok_aabb_phantom_info(M& m,std::uint32_t info) {
  if((info+0x20u)&15u)refuse("31C3B0","a misaligned AABB (MOVAPS fault)");
  // 31FCA0 (through 31FCD0)
  detail::w32(m,info+0xcu,0);detail::w32(m,info+0x10u,0);detail::w32(m,info+0x14u,0x80000000u);
  detail::w32(m,info,0);detail::w32(m,info+4u,0);detail::w8(m,info+8u,1);
  w128(m,info+0x20u,{});w128(m,info+0x30u,{});
  return info;
}

std::uint32_t havok_aabb_phantom_construct(M& m,std::uint32_t phantom,std::uint32_t info) {
  if(((info+0x20u)|(phantom+0x60u))&15u)refuse("3154E0","a misaligned AABB (MOVAPS fault)");
  using detail::w32;
  detail::w16(m,phantom+6u,1);
  for(const auto offset:{0x8u,0xcu,0x14u,0x18u})w32(m,phantom+offset,0);
  w32(m,phantom+0x10u,detail::none);
  w32(m,phantom+0x1cu,0);w32(m,phantom+0x20u,0);w32(m,phantom+0x28u,0);
  w32(m,phantom+0x24u,2);
  w32(m,phantom+0x2cu,0);w32(m,phantom+0x30u,0);w32(m,phantom+0x34u,0);w32(m,phantom+0x38u,0x80000000u);
  w32(m,phantom+0x3cu,0);w32(m,phantom+0x40u,0);w32(m,phantom+0x44u,0x80000000u);
  w32(m,phantom+0x48u,0);w32(m,phantom+0x4cu,0);w32(m,phantom+0x50u,0x80000000u);
  w32(m,phantom+0x2cu,phantom);
  w32(m,phantom,0x41d87cu);
  w32(m,phantom+0x80u,0);w32(m,phantom+0x84u,0);w32(m,phantom+0x88u,0x80000000u);
  w128(m,phantom+0x60u,r128(m,info+0x20u));
  w128(m,phantom+0x70u,r128(m,info+0x30u));
  w32(m,phantom+0x28u,r32(m,info));
  return phantom;
}

namespace {
// ---- The Havok stack arena ([0x48C938] +8 cursor, +C remaining, +14
// boundary) and its temporary arrays {data,count,capacity|flag,block,reserve}.
std::uint32_t arena_allocate(M& m,std::uint32_t bytes,HavokFp& fp) {
  const auto arena=r32(m,havok_allocator_global),remaining=r32(m,arena+0xcu);
  if(less(remaining,bytes)) {
    const auto owner=r32(m,r32(m,arena)+0x24u);
    if(owner!=arena_allocate_method)refuse("[0x48C938]+24","stack arena allocation method "+hex(owner));
    return native_tu15_allocate_stack_arena(m,arena,bytes,fp,havok_allocator(m));
  }
  const auto cursor=r32(m,arena+8u);
  w32(m,arena+8u,cursor+bytes);w32(m,arena+0xcu,remaining-bytes);
  return cursor;
}
void arena_release(M& m,std::uint32_t block,std::uint32_t bytes,HavokFp& fp) {
  const auto arena=r32(m,havok_allocator_global);
  if(bytes) {
    const auto boundary=r32(m,arena+0x14u);
    if(block==boundary || !boundary) {
      const auto owner=r32(m,r32(m,arena)+0x28u);
      if(owner!=arena_release_method)refuse("[0x48C938]+28","stack arena release method "+hex(owner));
      native_tu15_release_stack_arena(m,arena,block,bytes,fp,havok_allocator(m));
      return;
    }
  }
  const auto cursor=r32(m,arena+8u),remaining=r32(m,arena+0xcu);
  w32(m,arena+8u,cursor-bytes);w32(m,arena+0xcu,remaining+bytes);
}
// 3177E0 (stride 8) / 2EBA20 (stride 4).
void arena_array_construct(M& m,std::uint32_t header,std::uint32_t count,std::uint32_t stride,HavokFp& fp) {
  w32(m,header,0);w32(m,header+4u,0);w32(m,header+8u,0x80000000u);
  const auto data=arena_allocate(m,(count*stride+15u)&~15u,fp);
  w32(m,header,data);w32(m,header+0xcu,data);w32(m,header+8u,count|0x80000000u);w32(m,header+0x10u,count);
}
// 317870 (stride 8) / 2EB850 (stride 4).
void arena_array_destroy(M& m,std::uint32_t header,std::uint32_t stride,HavokFp& fp) {
  const auto count=r32(m,header+0x10u),block=r32(m,header+0xcu);
  arena_release(m,block,(count*stride+15u)&~15u,fp);
  const auto capacity=r32(m,header+8u);
  if(s32(capacity)>=0)havok_array_release(m,r32(m,header),(capacity&0x7fffffffu)*stride,0x12u,fp);
}

// ---- hk3AxisSweep (broadphase vtable 0x41C6C0) ----
// Node (16 bytes): +0 y min, +2 z min, +4 y max, +6 z max, +8 x min, +A x max
// (endpoint positions), +C handle (or marker cell offset | 1). Axis arrays
// at +4C/+58/+64 hold {value word, node word} endpoints between sentinels.

// AABB quantization: ((v + offset) * scale) clamped to [0, 65532] + 65536.
std::array<std::uint32_t,4> quantize(const M& m,std::uint32_t vector,std::uint32_t offset,std::uint32_t scale,
    std::uint32_t mxcsr) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const auto load=[&](std::uint32_t at){const auto b=r128(m,at);__m128 v;std::memcpy(&v,b.data(),16);return v;};
  __m128 value=load(vector);const __m128 add=load(offset),multiply=load(scale);
  const __m128 upper=load(0x41c6a0u),lower=load(0x41c690u),bias=load(0x41c680u);
  const auto mode=sse::sse_control(mxcsr);std::uint32_t saved{};
  asm volatile("stmxcsr %1\n\tldmxcsr %2\n\taddps %3, %0\n\tmulps %4, %0\n\tminps %5, %0\n\t"
      "maxps %6, %0\n\taddps %7, %0\n\tldmxcsr %1"
      :"+x"(value),"+m"(saved):"m"(mode),"x"(add),"x"(multiply),"x"(upper),"x"(lower),"x"(bias));
  std::array<std::uint32_t,4> out{};std::memcpy(out.data(),&value,16);return out;
#else
  (void)m;(void)vector;(void)offset;(void)scale;(void)mxcsr;
  refuse("2EC4B0","packed SSE quantization on this host");
#endif
}
void toggle(M& m,std::uint32_t bits,std::uint32_t node) {
  const auto at=bits+std::uint32_t(s32(node)>>5)*4u;w32(m,at,r32(m,at)^(1u<<(node&31u)));
}
void clear(M& m,std::uint32_t bits,std::uint32_t node) {
  const auto at=bits+std::uint32_t(s32(node)>>5)*4u;w32(m,at,r32(m,at)&~(1u<<(node&31u)));
}
std::uint64_t lanes(std::uint64_t value,const std::array<std::uint16_t,4>& delta,bool subtract) {
  std::uint64_t out=0;
  for(unsigned k=0;k<4;++k) {
    const auto word=std::uint16_t(value>>(16u*k));
    out|=std::uint64_t(std::uint16_t(subtract?word-delta[k]:word+delta[k]))<<(16u*k);
  }
  return out;
}
// PCMPGTW against two thresholds, PADDW: 0, -1 or -2 per lane.
std::array<std::uint16_t,4> above(std::uint64_t value,const std::array<std::uint16_t,4>& first,
    const std::array<std::uint16_t,4>& second) {
  std::array<std::uint16_t,4> out{};
  for(unsigned k=0;k<4;++k) {
    const auto word=std::int16_t(std::uint16_t(value>>(16u*k)));
    out[k]=std::uint16_t((word>std::int16_t(first[k])?0xffffu:0u)+(word>std::int16_t(second[k])?0xffffu:0u));
  }
  return out;
}
// 2EAFB0: insert a min/max endpoint pair (thiscall axis; RET 18).
void axis_insert(M& m,std::uint32_t axis,std::uint32_t index,std::uint32_t minimum,std::uint32_t maximum,
    std::uint32_t min_out,std::uint32_t max_out,HavokFp& fp) {
  const auto count=r32(m,axis+4u)+2u;const auto capacity=r32(m,axis+8u)&0x7fffffffu;
  if(less(capacity,count)) {
    auto grown=capacity*2u;if(!less(count,grown))grown=count;
    native_tu15_reserve_array(m,{axis},grown,4u,fp,havok_allocator(m));
  }
  auto p=r32(m,axis)+count*4u-0xcu;
  w32(m,axis+4u,count);
  const auto hi=std::uint16_t(maximum),lo=std::uint16_t(minimum),id=std::uint16_t(index);
  while(hi<=r16(m,p)){w32(m,p+8u,r32(m,p));p-=4u;}
  w16(m,p+8u,hi);w16(m,p+0xau,id);
  w16(m,max_out,std::uint16_t((s32(p-r32(m,axis))>>2)+2));
  while(lo<r16(m,p)){w32(m,p+4u,r32(m,p));p-=4u;}
  w16(m,p+6u,id);w16(m,p+4u,lo);
  w16(m,min_out,std::uint16_t((s32(p-r32(m,axis))>>2)+1));
}
// 2E9FD0: remove the endpoints at `minimum` and `maximum` (REP MOVSD twice).
void axis_remove(M& m,std::uint32_t axis,std::uint32_t minimum,std::uint32_t maximum) {
  const auto data=r32(m,axis);auto at=data+minimum*4u;const auto stop=data+maximum*4u-4u;
  if(at<stop) {
    const auto n=((stop-at-1u)>>2)+1u;
    for(std::uint32_t k=0;k<n;++k)w32(m,at+k*4u,r32(m,at+k*4u+4u));
    at+=n*4u;
  }
  const auto count=r32(m,axis+4u)-2u;const auto end=r32(m,axis)+count*4u;
  w32(m,axis+4u,count);
  if(at<end) {
    const auto n=((end-at-1u)>>2)+1u;
    for(std::uint32_t k=0;k<n;++k)w32(m,at+k*4u,r32(m,at+k*4u+8u));
  }
}
// 2E9CF0 (inserted: thresholds min-1/max-2, PSUBW) and 2E9DF0 (removed:
// thresholds min/max, PADDW): MMX repair of the nodes' endpoint positions.
// The x lanes of the high quadword are masked to the first two words; the
// handle word pair is rewritten unchanged.
void repair_positions(M& m,std::uint32_t bank,std::uint32_t count,std::uint32_t node,bool inserted) {
  const std::uint16_t low=inserted?1u:0u,high=inserted?2u:0u;
  const auto x0=std::uint16_t(r16(m,node+8u)-low),y0=std::uint16_t(r16(m,node)-low),z0=std::uint16_t(r16(m,node+2u)-low);
  const auto x1=std::uint16_t(r16(m,node+0xau)-high),y1=std::uint16_t(r16(m,node+4u)-high),z1=std::uint16_t(r16(m,node+6u)-high);
  const std::array<std::uint16_t,4> x_min{x0,x0,0xffffu,0xffffu},x_max{x1,x1,0xffffu,0xffffu};
  const std::array<std::uint16_t,4> min{y0,z0,y0,z0},max{y1,z1,y1,z1};
  const auto end=bank+(count<<4);
  for(auto p=bank;p<end;p+=16u) {
    auto x_delta=above(r64(m,p+8u),x_max,x_min);x_delta[2]=x_delta[3]=0;
    w64(m,p,lanes(r64(m,p),above(r64(m,p),min,max),inserted));
    w64(m,p+8u,lanes(r64(m,p+8u),x_delta,inserted));
  }
}
// The sweep's overlap bitset: {data,count,capacity|flag,block,reserve}, one
// bit per node, zeroed in 16-byte MOVAPS blocks.
void bitset_zero(M& m,std::uint32_t bits,std::uint32_t nodes,const char* owner) {
  const auto blocks=s32(nodes)>>7;
  if(blocks<0)return;
  if(bits&15u)refuse(owner,"a misaligned overlap bitset (MOVAPS fault)");
  for(std::int32_t k=0;k<=blocks;++k)w128(m,bits+std::uint32_t(k)*16u,{});
}
// The marker (cell) seed shared by add and remove: a node whose x minimum
// value falls in marker cell `bucket` starts from that marker's overlap list
// instead of the axis start. Returns the scan start.
std::uint32_t marker_seed(M& m,std::uint32_t broadphase,std::uint32_t bits,std::uint32_t bucket,std::uint32_t scan) {
  if(!(s32(bucket)>0))return scan;
  const auto cell=r32(m,broadphase+0x78u)+bucket*16u-16u;
  toggle(m,bits,r16(m,cell));
  auto list=r32(m,cell+4u);const auto count=r32(m,cell+8u);
  if(s32(count-1u)>=0)for(auto left=count;left;--left){toggle(m,bits,r16(m,list));list+=2u;}
  const auto marker=r32(m,broadphase+0x40u)+std::uint32_t(r16(m,cell))*16u;
  auto table=r32(m,broadphase+0x4cu);
  const auto end=table+std::uint32_t(r16(m,marker+0xau))*4u;
  for(auto p=table+std::uint32_t(r16(m,marker+8u))*4u+4u;p<end;p+=4u)
    if(!(r8(m,p)&1u))clear(m,bits,r16(m,p+2u));
  table=r32(m,broadphase+0x4cu);
  return table+std::uint32_t(r16(m,marker+8u))*4u+4u;
}
// The x sweep up to `x_min` (every endpoint toggles its node), then the
// minimum endpoints inside (x_min, x_max): the bitset holds the x overlaps.
std::uint32_t x_sweep(M& m,std::uint32_t broadphase,std::uint32_t bits,std::uint32_t table,std::uint32_t scan,
    std::uint32_t x_min,std::uint32_t x_max) {
  for(const auto edge=table+x_min*4u;scan<edge;scan+=4u)toggle(m,bits,r16(m,scan+2u));
  const auto edge=r32(m,broadphase+0x4cu)+x_max*4u;
  for(scan+=4u;scan<edge;scan+=4u)if(!(r8(m,scan)&1u))toggle(m,bits,r16(m,scan+2u));
  return scan;
}
void append_pair(M& m,std::uint32_t pairs,std::uint32_t handle,std::uint32_t code,HavokFp& fp) {
  if(r32(m,pairs+4u)==(r32(m,pairs+8u)&0x7fffffffu))havok_grow_array(m,pairs,8u,fp);
  const auto count=r32(m,pairs+4u),data=r32(m,pairs);
  w32(m,data+count*8u,handle);w32(m,data+count*8u+4u,code);
  w32(m,pairs+4u,r32(m,pairs+4u)+1u);
}
// Every x-overlapping node whose y/z endpoint positions overlap `node`
// (packed-word borrow test). `found(other, code)` handles each.
template<class F> void overlaps(M& m,std::uint32_t broadphase,std::uint32_t bits,std::uint32_t node,F&& found) {
  auto base=r32(m,broadphase+0x40u);
  const auto end=bits+std::uint32_t(s32(r32(m,broadphase+0x44u))>>5)*4u+4u;
  for(auto word=bits;word<end;word+=4u,base+=0x200u) {
    auto set=r32(m,word);auto other=base;
    while(set) {
      if(!(set&0xffu)){other+=0x80u;set>>=8;continue;}
      if(set&1u) {
        const auto node_low=r32(m,node),other_low=r32(m,other);
        const auto node_high=r32(m,node+4u),other_high=r32(m,other+4u);
        if(!(((other_high-node_low)|(node_high-other_low))&0x80008000u))found(other,r32(m,other+0xcu));
      }
      other+=0x10u;set>>=1;
    }
  }
}

// ---- Island pair collector (vtable 0x41D9A0): 3181E0/318020's local
// handler for entity pairs (type 1,1) while a fixed body is added or
// removed. {+0 vtable, +4 collidable, +8 island data, +C count, +10
// capacity|flag, +14 four inline islands}.
constexpr std::uint32_t collector_vtable=0x41d9a0u,collector_create=0x317b70u,collector_delete=0x317c40u;
constexpr std::uint32_t island_pair_create=0x320ef0u,island_pair_delete=0x320260u;
void collector_construct(M& m,std::uint32_t c,std::uint32_t collidable) {
  w32(m,c,collector_vtable);w32(m,c+8u,c+0x14u);w32(m,c+0xcu,0);w32(m,c+0x10u,0x80000004u);w32(m,c+4u,collidable);
}
// 317D20.
void collector_destroy(M& m,std::uint32_t c,HavokFp& fp) {
  if(const auto capacity=r32(m,c+0x10u);s32(capacity)>=0)
    havok_array_release(m,r32(m,c+8u),(capacity&0x7fffffffu)*4u,0x12u,fp);
  w32(m,c,0x41d92cu);
}
// The pair's other body (collidable +20 owner), from the collector's own handle.
std::uint32_t collector_other_body(const M& m,std::uint32_t c,std::uint32_t pair) {
  const auto collidable=r32(m,c+4u);const auto handle=collidable?collidable+0x10u:0u;
  auto other=r32(m,pair);if(other==handle)other=r32(m,pair+4u);
  return r32(m,(other?other-0x10u:0u)+0x20u);
}
// Append {pair[0], pair[1]} to an island pair list (stride 8).
void append_pair_words(M& m,std::uint32_t list,std::uint32_t pair,HavokFp& fp) {
  if(r32(m,list+4u)==(r32(m,list+8u)&0x7fffffffu))havok_grow_array(m,list,8u,fp);
  const auto count=r32(m,list+4u);const auto first=r32(m,pair);const auto data=r32(m,list);
  w32(m,data+count*8u,first);w32(m,data+count*8u+4u,r32(m,pair+4u));
  w32(m,list+4u,r32(m,list+4u)+1u);
}
// The island joins the collector's list unless it is already there.
void collector_note(M& m,std::uint32_t c,std::uint32_t island,HavokFp& fp) {
  const auto list=c+8u;const auto count=r32(m,list+0x4u);
  if(s32(count)>0) {
    auto p=r32(m,list);
    for(std::uint32_t k=0;s32(k)<s32(count);++k,p+=4u)if(r32(m,p)==island)return;
  }
  if(r32(m,list+4u)==(r32(m,list+8u)&0x7fffffffu))havok_grow_array(m,list,4u,fp);
  w32(m,r32(m,list)+r32(m,list+4u)*4u,island);
  w32(m,list+4u,r32(m,list+4u)+1u);
}
// 317B70 (create: island +80) / 317C40 (delete: 317CD0, island +8C).
void collector_pair(M& m,std::uint32_t c,std::uint32_t pair,bool created,HavokFp& fp) {
  const auto body=collector_other_body(m,c,pair);
  if(r8(m,body+0x40u))return;
  const auto island=r32(m,body+0x44u);
  append_pair_words(m,island+(created?0x80u:0x8cu),pair,fp);
  collector_note(m,c,island,fp);
}

// ---- Pair dispatch (37CBF0 normalize, 37C960 merge) through the world's
// typed dispatcher (+138) and collision filter (+D0 +8).
NativeTu15PairConsumerServices pair_services(HavokMemory& m);
// 320EF0/320260's island owners: activation (316D40), deferral (320DA0);
// contact manager/agent creation and destruction are refused by name.
NativeTu15PairHandlerServices island_handler_services(HavokMemory& m) {
  NativeTu15PairHandlerServices s;
  s.activate_island=[&m](NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,HavokFp& fp) {
    native_tu15_activate_physics_island(m,world,island,fp,activation_services(m));
  };
  s.defer_pair=[&m](NativeTu15PhysicsIsland island,std::uint32_t pair,HavokFp& fp) {
    native_tu15_defer_bound_collision_pair(m,island.identity,pair,fp,havok_allocator(m));
  };
  s.merge_bodies=[](NativeTu15PhysicsWorld,NativeTu15PhysicsBody,NativeTu15PhysicsBody,HavokFp&) {
    refuse("320EF0","the body-island merge 317200 and contact agent creation");
  };
  s.agents.create_manager=[](std::uint32_t,NativeTu15ContactVirtualOwner owner,NativeTu15ContactPayload,
      NativeTu15ContactPayload,NativeTu15ContactContext,HavokFp&)->NativeTu15ActivationContactManager {
    refuse("320EF0","contact manager factory "+hex(owner.identity));
  };
  s.destroy_agent=[](NativeTu15ContactAgent,NativeTu15ContactVirtualOwner owner,HavokFp&) {
    refuse("320260","contact agent destruction "+hex(owner.identity));
  };
  s.agents.release_manager=[](NativeTu15ActivationContactManager,NativeTu15ContactVirtualOwner owner,HavokFp&) {
    refuse("320260","contact manager release "+hex(owner.identity));
  };
  return s;
}
NativeTu15PairConsumerServices pair_services(HavokMemory& m) {
  NativeTu15PairConsumerServices s;
  s.filter_pair=[&m](std::uint32_t filter,NativeTu15PhysicsVirtualOwner owner,std::uint32_t decision,
      std::uint32_t first,std::uint32_t second,HavokFp&) {
    if(owner.identity!=group_filter)refuse("37C960","collision filter "+hex(owner.identity));
    return native_tu15_activation_group_filter(m,filter,decision,{first},{second});
  };
  s.create_pair=[&m](std::uint32_t handler,NativeTu15PhysicsVirtualOwner owner,std::uint32_t pair,HavokFp& fp) {
    if(owner.identity==collector_create){collector_pair(m,handler,pair,true,fp);return;}
    if(owner.identity==island_pair_create) {
      Frame formal(m,0x10u);w32(m,formal.at,pair);
      native_tu15_create_pair(m,handler,pair,{formal.at},fp,island_handler_services(m));
      return;
    }
    if(owner.identity!=phantom_pair_create)refuse("37C960","pair creation handler "+hex(owner.identity));
    NativeTu15PhantomPairCreateServices phantom;
    phantom.add_overlap=[&m](std::uint32_t receiver,NativeTu15ContactVirtualOwner child,NativeTu15ContactPayload other,
        std::uint32_t flag_word,HavokFp& state) {
      if(child.identity!=aabb_phantom_add_overlap)refuse("31FFC0","phantom overlap addition "+hex(child.identity));
      Frame event(m,0x10u);
      NativeTu15PhantomOverlapAddServices overlap;
      overlap.allocation=havok_allocator(m);
      overlap.added=[](std::uint32_t,NativeTu15ContactVirtualOwner target,NativeTu15PhantomOverlapAddedEvent,HavokFp&) {
        if(target.identity!=default_listener)refuse("314F80","phantom overlap-added listener "+hex(target.identity));
      };
      native_tu15_add_simple_phantom_overlap(m,receiver,other,flag_word,{event.at},state,overlap);
    };
    native_tu15_create_phantom_pair(m,pair,fp,phantom);
  };
  s.delete_pair=[&m](std::uint32_t handler,NativeTu15PhysicsVirtualOwner owner,std::uint32_t pair,HavokFp& fp) {
    if(owner.identity==collector_delete){collector_pair(m,handler,pair,false,fp);return;}
    if(owner.identity==island_pair_delete) {
      Frame formal(m,0x10u);w32(m,formal.at,pair);
      native_tu15_delete_pair(m,handler,pair,NativeTu15PairDeletionScratch{formal.at},fp,island_handler_services(m));
      return;
    }
    if(owner.identity!=phantom_pair_delete)refuse("37C960","pair deletion handler "+hex(owner.identity));
    NativeTu15PhantomPairDeleteServices phantom;
    phantom.remove_overlap=[&m](std::uint32_t receiver,NativeTu15ContactVirtualOwner child,NativeTu15ContactPayload other,
        HavokFp& state) {
      if(child.identity==default_listener){native_tu15_ignore_removed_phantom_overlap();return;}
      if(child.identity!=aabb_phantom_remove_overlap)refuse("320040","phantom overlap removal "+hex(child.identity));
      Frame event(m,0x10u);
      NativeTu15PhantomOverlapRemoveServices overlap;
      overlap.removed=[](std::uint32_t,NativeTu15ContactVirtualOwner target,NativeTu15PhantomOverlapEvent,HavokFp&) {
        if(target.identity!=default_listener)refuse("314FE0","phantom overlap-removed listener "+hex(target.identity));
      };
      native_tu15_remove_simple_phantom_overlap(m,receiver,other,{event.at},state,overlap);
    };
    native_tu15_delete_phantom_pair(m,pair,fp,phantom);
  };
  // 321510's temporary records (arena +24/+28) and its owned record array
  // (allocator +10/+14).
  s.arena.allocate_block=[&m](NativeTu15BroadPhaseAllocator a,NativeTu15PhysicsVirtualOwner owner,std::uint32_t bytes,
      HavokFp& fp) {
    if(owner.identity!=arena_allocate_method)refuse("[0x48C938]+24","stack arena allocation method "+hex(owner.identity));
    return native_tu15_allocate_stack_arena(m,a.identity,bytes,fp,havok_allocator(m));
  };
  s.arena.release_block=[&m](NativeTu15BroadPhaseAllocator a,NativeTu15PhysicsVirtualOwner owner,std::uint32_t block,
      std::uint32_t bytes,HavokFp& fp) {
    if(owner.identity!=arena_release_method)refuse("[0x48C938]+28","stack arena release method "+hex(owner.identity));
    native_tu15_release_stack_arena(m,a.identity,block,bytes,fp,havok_allocator(m));
  };
  s.arena.release_owned_pairs=[&m](NativeTu15BroadPhaseAllocator a,NativeTu15PhysicsVirtualOwner owner,std::uint32_t data,
      std::uint32_t bytes,std::uint32_t memory_class,HavokFp& fp) {
    havok_allocator(m).release({a.identity,owner.identity,data,bytes,memory_class},fp);
  };
  s.allocate_owned_records=[&m](NativeTu15BroadPhaseAllocator a,NativeTu15PhysicsVirtualOwner owner,std::uint32_t bytes,
      std::uint32_t memory_class,HavokFp& fp) {
    return havok_allocator(m).allocate({a.identity,owner.identity,bytes,memory_class},fp);
  };
  return s;
}
// 37C960 with its formal argument slots as private locals; the interior
// decision slot starts as first.data, the tail one as the filter.
void merge_pairs(HavokMemory& m,std::uint32_t dispatcher,NativeTu15BroadPhasePairs first,NativeTu15BroadPhasePairs second,
    std::uint32_t filter,HavokFp& fp) {
  Frame frame(m,0x10u);
  const NativeTu15PairMergeScratch scratch{frame.at+4u,frame.at+8u,frame.at,frame.at+0xcu};
  w32(m,scratch.interior_decision,first.data);w32(m,scratch.tail_decision,filter);
  native_tu15_merge_pairs(m,dispatcher,first,second,filter,scratch,fp,pair_services(m));
}
// 321510 (thiscall island; first data/count, second data/count).
void consume_pairs(HavokMemory& m,std::uint32_t island,NativeTu15BroadPhasePairs first,NativeTu15BroadPhasePairs second,
    HavokFp& fp) {
  Frame frame(m,0x40u);
  const NativeTu15PairConsumerScratch scratch{{frame.at},{frame.at+0x20u,frame.at+0x24u,frame.at+0x28u,frame.at+0x2cu}};
  native_tu15_consume_pairs(m,{island},first,second,scratch,fp,pair_services(m));
}
// 318550/318680's pair tail: 37CBF0 then 37C960 with the new pairs as the
// added or the removed range.
void dispatch_pairs(HavokMemory& m,std::uint32_t world,std::uint32_t pairs,bool added,HavokFp& fp) {
  if(!r32(m,pairs+4u))return;
  native_tu15_normalize_pairs(m,{r32(m,pairs),r32(m,pairs+4u)});
  const auto parent=r32(m,world+0xd0u);const auto filter=parent?parent+8u:0u;
  const NativeTu15BroadPhasePairs found{r32(m,pairs),r32(m,pairs+4u)},empty{};
  merge_pairs(m,r32(m,world+0x138u),added?found:empty,added?empty:found,filter,fp);
}
// 322280 (slot +8, added) / 3222F0 (slot +4, removed): the world's phantom
// listeners (+54/+58); 3221A0 (+C) / 322210 (+8): its entity listeners
// (+48/+4C). Newest first, then null entries compacted.
void world_listeners(M& m,std::uint32_t world,std::uint32_t array,std::uint32_t slot,const char* owner) {
  for(auto k=r32(m,world+array+4u)-1u;s32(k)>=0;--k) {
    const auto listener=r32(m,r32(m,world+array)+k*4u);
    if(!listener)continue;
    const auto method=r32(m,r32(m,listener)+slot);
    if(method!=default_listener)refuse(owner,"world listener "+hex(method));
  }
  for(auto k=r32(m,world+array+4u)-1u;s32(k)>=0;--k) {
    if(r32(m,r32(m,world+array)+k*4u))continue;
    const auto count=r32(m,world+array+4u)-1u;
    w32(m,world+array+4u,count);
    for(auto j=k;less(j,count);) {
      const auto data=r32(m,world+array);w32(m,data+j*4u,r32(m,data+j*4u+4u));
      if(!less(++j,r32(m,world+array+4u)))break;
    }
  }
}
void phantom_listeners(M& m,std::uint32_t world,std::uint32_t slot,const char* owner) {
  world_listeners(m,world,0x54u,slot,owner);
}
// 315620(1): the hkAabbPhantom destructor chain 315570, 31FAA0, 315400,
// B4150, then the release of the phantom itself.
void destroy_aabb_phantom(M& m,std::uint32_t phantom,HavokFp& fp) {
  const auto method=r32(m,r32(m,phantom));
  if(method!=aabb_phantom_destructor)refuse("1CE340","phantom destructor "+hex(method));
  w32(m,phantom,0x41d87cu);                                   // 315570
  if(const auto capacity=r32(m,phantom+0x88u);s32(capacity)>=0)
    havok_array_release(m,r32(m,phantom+0x80u),(capacity&0x7fffffffu)*4u,0x12u,fp);
  w32(m,phantom,0x41d854u);                                   // 31FAA0
  for(auto k=r32(m,phantom+0x4cu)-1u;s32(k)>=0;--k) {
    const auto listener=r32(m,r32(m,phantom+0x48u)+k*4u);
    if(!listener)continue;
    const auto removed=r32(m,r32(m,listener));
    if(removed!=default_listener)refuse("31FAA0","phantom deletion listener "+hex(removed));
  }
  if(const auto capacity=r32(m,phantom+0x50u);s32(capacity)>=0)
    havok_array_release(m,r32(m,phantom+0x48u),(capacity&0x7fffffffu)*4u,0x12u,fp);
  if(const auto capacity=r32(m,phantom+0x44u);s32(capacity)>=0)
    havok_array_release(m,r32(m,phantom+0x3cu),(capacity&0x7fffffffu)*4u,0x12u,fp);
  w32(m,phantom,0x41d840u);                                   // 315400
  if(const auto shape=r32(m,phantom+0xcu)) {
    w16(m,shape+6u,std::uint16_t(r16(m,shape+6u)-1u));
    if(!r16(m,shape+6u)) {
      if(!native_tu15_reserved_body_reference(m,{shape}))refuse("315400","phantom shape destructor");
      w16(m,shape+6u,1);
    }
  }
  if(const auto capacity=r32(m,phantom+0x38u);s32(capacity)>=0)
    havok_array_release(m,r32(m,phantom+0x30u),(capacity&0x7fffffffu)*8u,0x12u,fp);
  const auto size=r16(m,phantom+4u);                          // B4150
  w32(m,phantom,0x45eddcu);
  if(size&0x8000u)refuse("B4150","2E12A0 for a flagged phantom allocation");
  w32(m,phantom,0x45edf0u);
  havok_array_release(m,phantom,r16(m,phantom+4u),0x2cu,fp);  // 315620 flag 1
}
// A phantom reference that reached zero: 283450 keeps a reserved one alive
// (count 1), anything else runs the destructor virtual.
void release_phantom(M& m,std::uint32_t phantom,HavokFp& fp) {
  if(native_tu15_reserved_body_reference(m,{phantom}))w16(m,phantom+6u,1);
  else destroy_aabb_phantom(m,phantom,fp);
}
}

void havok_broadphase_add_object(M& base,std::uint32_t broadphase,std::uint32_t handle,std::uint32_t aabb,
    std::uint32_t pairs,HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    if((aabb|broadphase)&15u)refuse("2EC4B0","a misaligned AABB or broadphase (MOVAPS fault)");
    const auto low=quantize(m,aabb,broadphase+0x10u,broadphase+0x30u,fp.mxcsr);
    const auto high=quantize(m,aabb+0x10u,broadphase+0x20u,broadphase+0x30u,fp.mxcsr);
    std::array<std::uint32_t,3> minimum{},maximum{};
    for(unsigned k=0;k<3;++k){minimum[k]=(low[k]>>7)&0xfffeu;maximum[k]=((high[k]>>7)&0xffffu)|1u;}
    const auto index=r32(m,broadphase+0x44u);
    if(index==(r32(m,broadphase+0x48u)&0x7fffffffu))havok_grow_array(m,broadphase+0x40u,16u,fp);
    const auto count=r32(m,broadphase+0x44u),bank=r32(m,broadphase+0x40u),fresh=bank+count*16u;
    w32(m,broadphase+0x44u,count+1u);
    axis_insert(m,broadphase+0x4cu,index,minimum[0],maximum[0],fresh+8u,fresh+0xau,fp);
    axis_insert(m,broadphase+0x58u,index,minimum[1],maximum[1],fresh,fresh+4u,fp);
    axis_insert(m,broadphase+0x64u,index,minimum[2],maximum[2],fresh+2u,fresh+6u,fp);
    repair_positions(m,bank,index,fresh,true);
    w32(m,fresh+0xcu,handle);w32(m,handle,index);
    Frame frame(m,0x20u);const auto bitset=frame.at;
    const auto nodes=r32(m,broadphase+0x44u);
    arena_array_construct(m,bitset,std::uint32_t(s32(nodes)>>5)+4u,4u,fp);
    const auto x_max=r16(m,fresh+0xau),x_min=r16(m,fresh+8u);const auto bits=r32(m,bitset);
    bitset_zero(m,bits,nodes,"2EC4B0");
    auto table=r32(m,broadphase+0x4cu);auto scan=table+4u;
    if(r32(m,broadphase+0x70u)) {
      const auto bucket=std::uint32_t(s32(minimum[0])>>((16u-r32(m,broadphase+0x74u))&31u));
      if(s32(bucket)>0){scan=marker_seed(m,broadphase,bits,bucket,scan);table=r32(m,broadphase+0x4cu);}
    }
    x_sweep(m,broadphase,bits,table,scan,x_min,x_max);
    overlaps(m,broadphase,bits,fresh,[&](std::uint32_t,std::uint32_t code) {
      if(!(code&1u)){append_pair(m,pairs,handle,code,fp);return;}
      const auto list=r32(m,broadphase+0x78u)+(code&~1u)+4u;
      if(r32(m,list+4u)==(r32(m,list+8u)&0x7fffffffu))havok_grow_array(m,list,2u,fp);
      const auto n=r32(m,list+4u),data=r32(m,list);
      w16(m,data+n*2u,std::uint16_t(index));
      w32(m,list+4u,r32(m,list+4u)+1u);
    });
    arena_array_destroy(m,bitset,4u,fp);
  });
}

void havok_broadphase_remove_object(M& base,std::uint32_t broadphase,std::uint32_t handle,std::uint32_t pairs,
    HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    const auto count=r32(m,broadphase+0x44u),bank=r32(m,broadphase+0x40u),index=r32(m,handle);
    const auto node=index*16u+bank;
    {
      Frame frame(m,0x20u);const auto bitset=frame.at;
      arena_array_construct(m,bitset,std::uint32_t(s32(count)>>5)+4u,4u,fp);
      const auto x_max=r16(m,node+0xau),x_min=r16(m,node+8u);
      const auto value=r16(m,r32(m,broadphase+0x4cu)+std::uint32_t(x_min)*4u);
      const auto bits=r32(m,bitset);
      bitset_zero(m,bits,count,"2EC9B0");
      auto scan=r32(m,broadphase+0x4cu)+4u;
      if(r32(m,broadphase+0x70u)) {
        const auto bucket=std::uint32_t(s32(value)>>((16u-r32(m,broadphase+0x74u))&31u));
        scan=marker_seed(m,broadphase,bits,bucket,scan);
      }
      x_sweep(m,broadphase,bits,r32(m,broadphase+0x4cu),scan,x_min,x_max);
      overlaps(m,broadphase,bits,node,[&](std::uint32_t,std::uint32_t code) {
        if(!(code&1u)){append_pair(m,pairs,handle,code,fp);return;}
        const auto cell=r32(m,broadphase+0x78u)+(code&~1u);
        const auto n=r32(m,cell+8u);std::uint32_t at=detail::none;
        if(s32(n)>0) {
          auto list=r32(m,cell+4u);
          for(std::uint32_t k=0;less(k,n);++k,list+=2u)if(r16(m,list)==std::uint16_t(index)){at=k;break;}
        }
        const auto last=r32(m,cell+8u)-1u;w32(m,cell+8u,last);
        const auto list=r32(m,cell+4u);w16(m,list+at*2u,r16(m,list+last*2u));
      });
      arena_array_destroy(m,bitset,4u,fp);
    }
    const auto nodes=r32(m,broadphase+0x40u);
    axis_remove(m,broadphase+0x4cu,r16(m,node+8u),r16(m,node+0xau));
    axis_remove(m,broadphase+0x58u,r16(m,node),r16(m,node+4u));
    axis_remove(m,broadphase+0x64u,r16(m,node+2u),r16(m,node+6u));
    repair_positions(m,nodes,count,node,false);
    if(index<count-1u) {
      const auto last=r32(m,broadphase+0x40u)+count*16u-16u;
      for(std::uint32_t k=0;k<16u;k+=4u)w32(m,node+k,r32(m,last+k));
      const auto id=std::uint16_t(index);
      w16(m,r32(m,broadphase+0x4cu)+std::uint32_t(r16(m,node+8u))*4u+2u,id);
      w16(m,r32(m,broadphase+0x4cu)+std::uint32_t(r16(m,node+0xau))*4u+2u,id);
      if(const auto code=r32(m,node+0xcu);!(code&1u)) {
        w16(m,r32(m,broadphase+0x58u)+std::uint32_t(r16(m,node))*4u+2u,id);
        w16(m,r32(m,broadphase+0x58u)+std::uint32_t(r16(m,node+4u))*4u+2u,id);
        w16(m,r32(m,broadphase+0x64u)+std::uint32_t(r16(m,node+2u))*4u+2u,id);
        w16(m,r32(m,broadphase+0x64u)+std::uint32_t(r16(m,node+6u))*4u+2u,id);
        w32(m,r32(m,node+0xcu),index);
      }
      else w16(m,r32(m,broadphase+0x78u)+(code&~1u),id);
      if(r32(m,broadphase+0x70u) && !(r8(m,node+0xcu)&1u)) {
        // The moved node's marker cells name it by its old (last) index.
        const auto x0=r16(m,node+8u);const auto table=r32(m,broadphase+0x4cu);
        const auto shift=std::uint32_t(std::uint16_t(16u-r16(m,broadphase+0x74u)))&31u;
        auto first=std::uint32_t(r16(m,table+std::uint32_t(x0)*4u))>>shift;
        if(s32(first)>0) {
          const auto marker=r16(m,r32(m,broadphase+0x78u)+first*16u-16u);
          if(r16(m,r32(m,broadphase+0x40u)+std::uint32_t(marker)*16u+0xau)>x0)--first;
        }
        const auto final=(std::uint32_t(r16(m,table+std::uint32_t(r16(m,node+0xau))*4u))>>shift)-1u;
        if(!(s32(first)>s32(final))) {
          const auto old=std::uint16_t(count-1u);
          auto offset=first*16u;
          for(auto n=final-first+1u;n;--n,offset+=16u) {
            const auto cell=r32(m,broadphase+0x78u)+offset;const auto size=r32(m,cell+8u);
            std::uint32_t at=detail::none;
            if(s32(size)>0) {
              auto list=r32(m,cell+4u);
              for(std::uint32_t k=0;less(k,size);++k,list+=2u)if(r16(m,list)==old){at=k;break;}
            }
            w16(m,r32(m,cell+4u)+at*2u,id);
          }
        }
      }
    }
    const auto remaining=count-1u;const auto capacity=r32(m,broadphase+0x48u)&0x7fffffffu;
    if(less(capacity,remaining)) {
      auto grown=capacity*2u;if(!less(remaining,grown))grown=remaining;
      native_tu15_reserve_array(m,{broadphase+0x40u},grown,16u,fp,havok_allocator(m));
    }
    w32(m,broadphase+0x44u,remaining);
  });
}

std::uint32_t havok_world_add_phantom(M& base,std::uint32_t world,std::uint32_t phantom,HavokFp& fp) {
  return with_havok(base,[&](HavokMemory& m)->std::uint32_t{
    // The calcAabb (+18) and broadphase addObject (+4) owners are named
    // before any store.
    if(const auto method=r32(m,r32(m,phantom)+0x18u);method!=aabb_phantom_aabb)
      refuse("318550","phantom AABB method "+hex(method));
    if((phantom+0x60u)&15u)refuse("315170","a misaligned phantom AABB (MOVAPS fault)");
    if(const auto method=r32(m,r32(m,r32(m,world+0xc4u))+4u);method!=broadphase_add_method)
      refuse("318550","broadphase addObject "+hex(method));
    w32(m,phantom+8u,world);
    Frame frame(m,0x40u);const auto pairs=frame.at,aabb=frame.at+0x20u;
    arena_array_construct(m,pairs,r32(m,world+0x234u),8u,fp);
    w128(m,aabb,r128(m,phantom+0x60u));w128(m,aabb+0x10u,r128(m,phantom+0x70u));  // 315170
    const auto collidable=phantom+0xcu;
    havok_broadphase_add_object(m,r32(m,world+0xc4u),collidable?collidable+0x10u:0u,aabb,pairs,fp);
    const auto id=r32(m,world+0x130u);
    w32(m,world+0x130u,id+1u);w32(m,phantom+0x20u,id);
    dispatch_pairs(m,world,pairs,true,fp);
    w16(m,phantom+6u,std::uint16_t(r16(m,phantom+6u)+1u));
    if(r32(m,world+0x124u)==(r32(m,world+0x128u)&0x7fffffffu))havok_grow_array(m,world+0x120u,4u,fp);
    w32(m,r32(m,world+0x120u)+r32(m,world+0x124u)*4u,phantom);
    w32(m,world+0x124u,r32(m,world+0x124u)+1u);
    phantom_listeners(m,world,8u,"322280");
    arena_array_destroy(m,pairs,8u,fp);
    return phantom;
  });
}

void havok_world_remove_phantom(M& base,std::uint32_t world,std::uint32_t phantom,HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    if(const auto method=r32(m,r32(m,r32(m,world+0xc4u))+0xcu);method!=broadphase_remove_method)
      refuse("318680","broadphase removeObject "+hex(method));
    Frame frame(m,0x20u);const auto pairs=frame.at;
    arena_array_construct(m,pairs,r32(m,world+0x234u),8u,fp);
    const auto collidable=phantom+0xcu;
    havok_broadphase_remove_object(m,r32(m,world+0xc4u),collidable?collidable+0x10u:0u,pairs,fp);
    dispatch_pairs(m,world,pairs,false,fp);
    phantom_listeners(m,world,4u,"3222F0");
    w16(m,phantom+6u,std::uint16_t(r16(m,phantom+6u)-1u));
    const bool released=!r16(m,phantom+6u);
    w32(m,phantom+8u,0);
    if(released)release_phantom(m,phantom,fp);
    const auto count=r32(m,world+0x124u);std::uint32_t at=detail::none;
    if(s32(count)>0) {
      auto p=r32(m,world+0x120u);
      for(std::uint32_t k=0;less(k,count);++k,p+=4u)if(r32(m,p)==phantom){at=k;break;}
    }
    const auto last=r32(m,world+0x124u)-1u;w32(m,world+0x124u,last);
    const auto data=r32(m,world+0x120u);w32(m,data+at*4u,r32(m,data+last*4u));
    arena_array_destroy(m,pairs,8u,fp);
  });
}

void havok_phantom_wake(M& base,std::uint32_t object,HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    Frame frame(m,0x40u);const auto info=frame.at;
    havok_aabb_phantom_info(m,info);
    const auto body=detail::object_body(m,object);
    const auto mx=fp.mxcsr;
    const auto radius=sse::addss(sse::mulss(detail::f32(r32(m,0x48b874u)),detail::f32(r32(m,0x46a11cu)),mx),
        detail::f32(r32(m,body+0x3cu)),mx);
    for(unsigned k=0;k<3;++k) {
      const auto center=detail::f32(r32(m,body+0x30u+4u*k));
      w32(m,info+0x20u+4u*k,detail::bits(sse::subss(center,radius,mx)));
      w32(m,info+0x30u+4u*k,detail::bits(sse::addss(center,radius,mx)));
    }
    w32(m,info+0x2cu,0);w32(m,info+0x3cu,0);
    const auto phantom=havok_array_allocate(m,0x90u,0x2cu,fp);
    w16(m,phantom+4u,0x90u);
    havok_aabb_phantom_construct(m,phantom,info);
    havok_world_add_phantom(m,r32(m,havok_world_global),phantom,fp);
    for(std::uint32_t k=0;less(k,r32(m,phantom+0x84u));++k) {
      const auto collidable=r32(m,r32(m,phantom+0x80u)+k*4u);
      if(r32(m,collidable+0x18u)!=1u)continue;
      const auto owner=r32(m,collidable+0x20u);
      if(!owner || r8(m,owner+0x40u))continue;
      havok_activate_body(m,owner,fp);
    }
    havok_world_remove_phantom(m,r32(m,havok_world_global),phantom,fp);
    w16(m,phantom+6u,std::uint16_t(r16(m,phantom+6u)-1u));
    if(!r16(m,phantom+6u))release_phantom(m,phantom,fp);
    if(const auto capacity=r32(m,info+0x14u);s32(capacity)>=0)
      havok_array_release(m,r32(m,info+0xcu),(capacity&0x7fffffffu)*8u,0x12u,fp);
  });
}

namespace {
constexpr std::uint32_t gameplay_listener_global=0x52d880u,entity_count_global=0x48b868u;
constexpr std::uint32_t island_destructor=0x322100u,island_memory_method=0x320130u,motion_storage_method=0x323420u;
constexpr std::uint32_t broadphase_entity_add=0x2ec4b0u,broadphase_entity_remove=0x2ec9b0u;
constexpr std::uint32_t body_record_destructor=0x1d9c80u;

// ---- Body listener arrays: 315D60 (+70) / 315DA0 (+60) clear the entry
// (or the word before the array when absent); 315E50 / 315E90 append.
void remove_listener(M& m,std::uint32_t body,std::uint32_t array,std::uint32_t listener) {
  const auto count=r32(m,body+array+4u);
  std::uint32_t at=detail::none;
  if(s32(count)>0) {
    auto p=r32(m,body+array);
    for(std::uint32_t k=0;s32(k)<s32(count);++k,p+=4u)if(r32(m,p)==listener){at=k;break;}
  }
  w32(m,r32(m,body+array)+at*4u,0);
}
void add_listener(M& m,std::uint32_t body,std::uint32_t array,std::uint32_t listener,HavokFp& fp) {
  if(r32(m,body+array+4u)==(r32(m,body+array+8u)&0x7fffffffu))havok_grow_array(m,body+array,4u,fp);
  w32(m,r32(m,body+array)+r32(m,body+array+4u)*4u,listener);
  w32(m,body+array+4u,r32(m,body+array+4u)+1u);
}
// 31FDE0: the entity's removal (+7C), contact (+88, through +8) and
// third (+94, through +8) listeners. Only the default 2E4D60 is native.
void entity_removed_listeners(const M& m,std::uint32_t body) {
  for(auto k=r32(m,body+0x80u)-1u;s32(k)>=0;--k) {
    const auto listener=r32(m,r32(m,body+0x7cu)+k*4u);
    if(listener && r32(m,r32(m,listener)+8u)!=default_listener)
      refuse("31FDE0","entity removal listener "+hex(r32(m,r32(m,listener)+8u)));
  }
  for(const auto array:{0x88u,0x94u})
    for(std::uint32_t k=0;s32(k)<s32(r32(m,body+array+4u));++k) {
      const auto entry=r32(m,r32(m,body+array)+k*4u);
      if(entry && r32(m,r32(m,entry+8u)+8u)!=default_listener)
        refuse("31FDE0","entity contact listener "+hex(r32(m,r32(m,entry+8u)+8u)));
    }
}
// The motion's +14 storage owner (323420: 0x80) and the island's +10
// memory owner (320130: +14 accumulates).
std::uint32_t motion_storage(const M& m,std::uint32_t body,const char* owner) {
  const auto method=r32(m,r32(m,r32(m,body+0x3cu))+0x14u);
  if(method!=motion_storage_method)refuse(owner,"motion storage method "+hex(method));
  return 0x80u;
}
void island_memory(M& m,std::uint32_t island,std::uint32_t delta,const char* owner) {
  const auto method=r32(m,r32(m,island)+0x10u);
  if(method!=island_memory_method)refuse(owner,"island memory method "+hex(method));
  w32(m,island+0x14u,r32(m,island+0x14u)+delta);
}
// 320DE0: body joins the island (+3C list, body +44/+58).
void island_add_body(M& m,std::uint32_t island,std::uint32_t body,HavokFp& fp) {
  w32(m,body+0x44u,island);
  const auto list=island+0x3cu;
  if(r32(m,list+4u)==(r32(m,list+8u)&0x7fffffffu))havok_grow_array(m,list,4u,fp);
  w32(m,r32(m,list)+r32(m,list+4u)*4u,body);
  w32(m,list+4u,r32(m,list+4u)+1u);
  w32(m,body+0x58u,r32(m,island+0x40u)-1u);
  const auto size=motion_storage(m,body,"320DE0");
  island_memory(m,island,size,"320DE0");
}
// 320150: the swap removal from the island, body +44/+58 cleared.
void island_remove_body(M& m,std::uint32_t island,std::uint32_t body) {
  const auto data=r32(m,island+0x3cu),last=r32(m,data+r32(m,island+0x40u)*4u-4u);
  w32(m,data+r32(m,body+0x58u)*4u,last);
  const auto index=r32(m,body+0x58u);
  w32(m,r32(m,r32(m,island+0x3cu)+index*4u)+0x58u,index);
  w32(m,island+0x40u,r32(m,island+0x40u)-1u);
  w32(m,body+0x44u,0);w32(m,body+0x58u,detail::none);
  const auto size=motion_storage(m,body,"320150");
  island_memory(m,island,0u-size,"320150");
  detail::w8(m,island+0x28u,1);
}
// A world island list ({data,count}) swap removal with the moved island's +24.
void world_island_remove(M& m,std::uint32_t list,std::uint32_t island) {
  const auto data=r32(m,list),last=r32(m,data+r32(m,list+4u)*4u-4u);
  w32(m,data+r32(m,island+0x24u)*4u,last);
  const auto index=r32(m,island+0x24u);
  w32(m,r32(m,r32(m,list)+index*4u)+0x24u,index);
  w32(m,list+4u,r32(m,list+4u)-1u);
}
// The island's deleting destructor (vtable +0, 322100).
void destroy_island(HavokMemory& m,std::uint32_t island,HavokFp& fp) {
  const auto method=r32(m,r32(m,island));
  if(method!=island_destructor)refuse("318360","island destructor "+hex(method));
  NativeTu15IslandLifetimeServices s;
  s.allocation=havok_allocator(m);
  s.allocate_arena=[&m](std::uint32_t arena,NativeTu15PhysicsVirtualOwner owner,std::uint32_t bytes,HavokFp& state) {
    if(owner.identity!=arena_allocate_method)refuse("[0x48C938]+24","stack arena allocation method "+hex(owner.identity));
    return native_tu15_allocate_stack_arena(m,arena,bytes,state,havok_allocator(m));
  };
  s.release_arena=[&m](std::uint32_t arena,NativeTu15PhysicsVirtualOwner owner,std::uint32_t block,std::uint32_t bytes,
      HavokFp& state) {
    if(owner.identity!=arena_release_method)refuse("[0x48C938]+28","stack arena release method "+hex(owner.identity));
    native_tu15_release_stack_arena(m,arena,block,bytes,state,havok_allocator(m));
  };
  native_tu15_destroy_physics_island(m,{island},1u,fp,s);
}
// A reference that reached zero: 283450 keeps a reserved one (count 1);
// the destructor virtual of an entity is refused.
void release_entity_reference(M& m,std::uint32_t body,const char* owner) {
  if(native_tu15_reserved_body_reference(m,{body})){w16(m,body+6u,1);return;}
  refuse(owner,"entity destructor "+hex(r32(m,r32(m,body))));
}
// 318360: detach the entity from its island (an emptied world island is
// unlisted and destroyed), then drop the world's deferred references
// (+114/+118: 3174A0 compaction, reference release).
void detach_entity(HavokMemory& m,std::uint32_t world,std::uint32_t body,HavokFp& fp) {
  const auto island=r32(m,body+0x44u);
  w32(m,body+8u,0);
  island_remove_body(m,island,body);
  if(r32(m,island+0x24u)!=detail::none && !r32(m,island+0x40u)) {
    world_island_remove(m,world+(r8(m,island+0x29u)?8u:0x14u),island);
    destroy_island(m,island,fp);
  }
  NativeTu15BodyReferenceServices references;
  references.reserve_array=[&m](NativeTu15PhysicsArrayHeader header,std::uint32_t capacity,std::uint32_t stride,HavokFp& state) {
    native_tu15_reserve_array(m,{header.identity},capacity,stride,state,havok_allocator(m));
  };
  for(std::uint32_t k=0;s32(k)<s32(r32(m,world+0x118u));++k) {
    native_tu15_compact_body_references(m,{r32(m,r32(m,world+0x114u)+k*4u)},fp,references);
    const auto released=r32(m,r32(m,world+0x114u)+k*4u);
    w16(m,released+6u,std::uint16_t(r16(m,released+6u)-1u));
    if(!r16(m,released+6u))release_entity_reference(m,released,"318360");
  }
  w32(m,world+0x118u,0);
}
// The (1,1) entity pair collector: 3181E0/318020's fixed-body pair route.
// Its address (an original stack local, `identity`) is published at the
// dispatcher's +24 slot for the merge, then +100 is restored.
void collect_entity_pairs(HavokMemory& m,std::uint32_t world,std::uint32_t collidable,NativeTu15BroadPhasePairs pairs,
    bool added,std::uint32_t identity,HavokFp& fp) {
  Frame frame(m,0x24u,identity);const auto c=frame.at;
  collector_construct(m,c,collidable);
  w32(m,r32(m,world+0x138u)+0x24u,c);
  const auto parent=r32(m,world+0xd0u);const auto filter=parent?parent+8u:0u;
  merge_pairs(m,r32(m,world+0x138u),added?pairs:NativeTu15BroadPhasePairs{},added?NativeTu15BroadPhasePairs{}:pairs,
      filter,fp);
  const auto dispatcher=r32(m,world+0x138u);w32(m,dispatcher+0x24u,dispatcher+0x100u);
  const auto list=added?0x80u:0x8cu;
  for(std::uint32_t k=0;s32(k)<s32(r32(m,c+0xcu));++k) {
    const auto island=r32(m,r32(m,c+8u)+k*4u);
    const NativeTu15BroadPhasePairs pending{r32(m,island+list),r32(m,island+list+4u)};
    consume_pairs(m,island,added?pending:NativeTu15BroadPhasePairs{},added?NativeTu15BroadPhasePairs{}:pending,fp);
    // The masked capacity is never negative: no 2E31E0 reserve.
    w32(m,island+list+4u,0);
  }
  collector_destroy(m,c,fp);
}
// 3181E0 (thiscall world; entity): broadphase removal (only 2EC9B0) and
// the removed pairs: the fixed body's collector, else its island's 321510.
void remove_entity_broadphase(HavokMemory& m,std::uint32_t world,std::uint32_t body,std::uint32_t sp,HavokFp& fp) {
  const auto collidable=body+0xcu;
  if(r32(m,collidable)) {
    if(const auto method=r32(m,r32(m,r32(m,world+0xc4u))+0xcu);method!=broadphase_entity_remove)
      refuse("3181E0","broadphase removeObject "+hex(method));
  }
  Frame frame(m,0x20u);const auto pairs=frame.at;
  arena_array_construct(m,pairs,r32(m,world+0x234u),8u,fp);
  if(r32(m,collidable)) {
    havok_broadphase_remove_object(m,r32(m,world+0xc4u),collidable+0x10u,pairs,fp);
    const NativeTu15BroadPhasePairs removed{r32(m,pairs),r32(m,pairs+4u)};
    if(s32(removed.count)>0) {
      if(r8(m,body+0x40u))collect_entity_pairs(m,world,collidable,removed,false,below(sp,0x30u),fp);
      else consume_pairs(m,r32(m,body+0x44u),{},removed,fp);
    }
  }
  arena_array_destroy(m,pairs,8u,fp);
}

// ---- Shape bounds (the shape vtable +18 owners that are native).
bool native_shape_bounds(std::uint32_t owner) {
  return owner==0x306760u||owner==0x307e30u||owner==0x2e8d90u||owner==0x2e8120u;
}
void shape_bounds(HavokMemory& m,std::uint32_t shape,std::uint32_t transform,std::uint32_t margin,std::uint32_t output,
    HavokFp& fp,const char* caller) {
  const auto owner=r32(m,r32(m,shape)+0x18u);
  if(owner==0x306760u)native_tu15_convex_box_bounds(m,{shape},{transform},margin,{output},fp,0);
  else if(owner==0x307e30u)native_tu15_centered_shape_bounds(m,{shape},{transform},margin,{output},fp,0);
  else if(owner==0x2e8d90u)native_tu15_sphere_shape_bounds(m,{shape},{transform},margin,{output},fp,0);
  else if(owner==0x2e8120u)native_tu15_endpoint_shape_bounds(m,{shape},{transform},margin,{output},fp,0);
  else refuse(caller,"shape bounds owner "+hex(owner));
}
// FLD m32; FMUL m32; FSTP m32 under the live x87 control word.
std::uint32_t x87_multiply(std::uint32_t a,std::uint32_t b,std::uint16_t control) {
#if HALO2_NATIVE_SCRIPT_X86_FP
  const std::uint16_t mode=sse::x87_control(control);std::uint16_t saved{};std::uint32_t out{};
  asm volatile("fnstcw %1\n\tfldcw %2\n\tflds %3\n\tfmuls %4\n\tfstps %0\n\tfldcw %1"
      :"=m"(out),"+m"(saved):"m"(mode),"m"(a),"m"(b):"st");
  return out;
#else
  (void)a;(void)b;(void)control;refuse("318020","x87 margin on this host");
#endif
}
// 318020 (thiscall world; entity): the AABB (shape +18 under the motion's
// transform, margin world +CC +8 * [0x46A0CC]), broadphase addition (only
// 2EC4B0) and the new pairs: the fixed body's collector, else 321510.
void add_entity_broadphase(HavokMemory& m,std::uint32_t world,std::uint32_t body,std::uint32_t sp,HavokFp& fp) {
  const auto collidable=body+0xcu;
  if(!r32(m,collidable))return;
  if(const auto owner=r32(m,r32(m,r32(m,collidable))+0x18u);!native_shape_bounds(owner))
    refuse("318020","shape bounds owner "+hex(owner));
  if(const auto method=r32(m,r32(m,r32(m,world+0xc4u))+4u);method!=broadphase_entity_add)
    refuse("318020","broadphase addObject "+hex(method));
  Frame frame(m,0x60u);const auto pairs=frame.at,aabb=frame.at+0x20u;
  arena_array_construct(m,pairs,r32(m,world+0x234u),8u,fp);
  const auto margin=x87_multiply(r32(m,r32(m,world+0xccu)+8u),r32(m,0x46a0ccu),fp.control_word);
  shape_bounds(m,r32(m,collidable),r32(m,collidable+8u)+0x20u,margin,aabb,fp,"318020");
  havok_broadphase_add_object(m,r32(m,world+0xc4u),collidable+0x10u,aabb,pairs,fp);
  const NativeTu15BroadPhasePairs added{r32(m,pairs),r32(m,pairs+4u)};
  if(s32(added.count)>0) {
    if(r8(m,body+0x40u))collect_entity_pairs(m,world,collidable,added,true,below(aligned_frame(sp),0x34u),fp);
    else consume_pairs(m,r32(m,body+0x44u),added,{},fp);
  }
  arena_array_destroy(m,pairs,8u,fp);
}
// 316950: a fixed entity joins the world's fixed island (+2C); another one
// a new island (0xB0 bytes, class 0x2D, 3212C0) appended to the active
// list, or with +23C clear the first active island (created when none).
void attach_entity_island(HavokMemory& m,std::uint32_t world,std::uint32_t body,HavokFp& fp) {
  w32(m,body+8u,world);
  if(r8(m,body+0x40u)){island_add_body(m,r32(m,world+0x2cu),body,fp);return;}
  const auto create=[&](bool join) {
    const auto island=havok_array_allocate(m,0xb0u,0x2du,fp);
    w16(m,island+4u,0xb0u);
    native_tu15_construct_physics_island(m,{island},{world});
    w32(m,island+0x24u,r32(m,world+0xcu));
    if(join)island_add_body(m,island,body,fp);
    if(r32(m,world+0xcu)==(r32(m,world+0x10u)&0x7fffffffu))havok_grow_array(m,world+8u,4u,fp);
    w32(m,r32(m,world+8u)+r32(m,world+0xcu)*4u,island);
    w32(m,world+0xcu,r32(m,world+0xcu)+1u);
  };
  if(r8(m,world+0x23cu)){create(true);return;}
  if(!r32(m,world+0xcu))create(false);
  island_add_body(m,r32(m,r32(m,world+8u)),body,fp);
}
// 282C10 (dirty islands): native, with 1CC310/1D9510 bound to this track
// and its other owners refused by name.
void process_dirty_islands(HavokMemory& m,HavokFp& fp) {
  NativeTu15DirtyIslandObjectServices s;
  s.set_component_flags=[&m](std::uint32_t datum,std::uint32_t flags,HavokFp& state,unsigned) {
    havok_component_state_flags(m,datum,flags,state);
  };
  s.remove_component=[&m](std::uint32_t component,HavokFp& state,unsigned){havok_component_remove(m,component,state);};
  s.collect_memory=[](HavokFp&,unsigned){refuse("282C10","memory collection 1496F0");};
  s.destroy_component=[](std::uint32_t,HavokFp&,unsigned){refuse("282C10","component destruction 1D7CA0");};
  s.free_component=[](std::uint32_t,std::uint32_t,HavokFp&,unsigned){refuse("282C10","component release 16E8A0");};
  s.delete_object=[](std::uint32_t,HavokFp&,unsigned){refuse("282C10","object deletion BA570");};
  s.defer_object=[](std::uint32_t,HavokFp&,unsigned){refuse("282C10","object deferral BA8C0");};
  native_tu15_process_dirty_island_objects(m,fp,0,s);
}
// 1D9AD0 (EAX body, ESI record, stack object handle): the gameplay body
// record: 31E820 (31E760's half-extent length under the identity
// transform 0x46E400), +60 the object, vtable 0x461970, +64 the body's
// 0x2002 property (0 when absent).
std::uint32_t build_body_record(HavokMemory& m,std::uint32_t body,std::uint32_t record,std::uint32_t object,HavokFp& fp) {
  if((record+0x10u)&15u)refuse("31E820","a misaligned body record (MOVAPS fault)");
  w16(m,record+6u,1);w32(m,record,0x41eb6cu);
  w32(m,record+0x50u,0x3c23d70au);w32(m,record+0x54u,0x3ba3d70au);
  w32(m,record+0x58u,0x3dcccccdu);w32(m,record+0x5cu,0x3e4ccccdu);
  std::array<std::byte,16> largest{};
  for(unsigned k=0;k<16;k+=4){largest[k]=std::byte(0xff);largest[k+1]=std::byte(0xff);largest[k+2]=std::byte(0x7f);largest[k+3]=std::byte(0x7f);}
  w128(m,record+0x10u,largest);w128(m,record+0x30u,largest);
  w128(m,record+0x20u,{});w32(m,record+0x2cu,0x3f800000u);
  w128(m,record+0x40u,{});w32(m,record+0x4cu,0x3f800000u);
  if(const auto shape=r32(m,body+0xcu)) {
    Frame bounds(m,0x20u);
    shape_bounds(m,shape,0x46e400u,0,bounds.at,fp,"31E760");
    const auto mx=fp.mxcsr;const auto half=detail::f32(0x3f000000u);
    std::array<float,3> square{};
    for(unsigned k=0;k<3;++k) {
      const auto extent=sse::mulss(half,sse::subss(detail::f32(r32(m,bounds.at+16u+4u*k)),detail::f32(r32(m,bounds.at+4u*k)),mx),mx);
      square[k]=sse::mulss(extent,extent,mx);
    }
    w32(m,record+8u,detail::bits(sse::addss(square[2],sse::addss(square[1],square[0],mx),mx)));
  }
  else w32(m,record+8u,0xbf800000u);
  w32(m,record+0x60u,object);
  w32(m,record,0x461970u);
  std::uint32_t value=0;
  for(std::uint32_t k=0;s32(k)<s32(r32(m,body+0x34u));++k)
    if(r32(m,r32(m,body+0x30u)+k*8u)==0x2002u){value=r32(m,r32(m,body+0x30u)+k*8u+4u);break;}
  w32(m,record+0x64u,value);
  return record;
}
// A body record reference that reached zero: 283450 keeps a reserved one,
// else its destructor (only 1D9C80, the gameplay record) runs.
void release_body_record(HavokMemory& m,std::uint32_t record,HavokFp& fp) {
  if(native_tu15_reserved_body_reference(m,{record})){w16(m,record+6u,1);return;}
  const auto method=r32(m,r32(m,record));
  if(method!=body_record_destructor)refuse("1D97F0","body record destructor "+hex(method));
  NativeTu15GameplayPolicyDestructionServices s{havok_allocator(m),
      [](std::uint32_t,std::uint32_t,HavokFp&){refuse("1D9C80","a flagged record allocation (2E12A0)");}};
  native_tu15_destroy_gameplay_body_policy(m,record,1u,fp,s);
}
// 1CC560 (stack body, flag): the entity limit (1CDD60 refused), the count,
// 319370, the optional 315CE0 activation request, 282C10.
void world_add_body(HavokMemory& m,std::uint32_t body,bool flag,std::uint32_t sp,HavokFp& fp) {
  if(s32(r32(m,entity_count_global))>=0x352)refuse("1CC560","the entity limit handler 1CDD60");
  w32(m,entity_count_global,r32(m,entity_count_global)+1u);
  havok_world_add_entity(m,r32(m,havok_world_global),body,fp,below(sp,0xcu));
  if(flag && r32(m,r32(m,body+0x44u)+0x40u)==1u) {
    // 315CE0 -> 316C90: an active, listed island queues the body (+108).
    if(const auto world=r32(m,body+8u)) {
      const auto island=r32(m,body+0x44u);
      if(r8(m,island+0x29u) && r32(m,island+0x24u)!=detail::none) {
        if(r32(m,world+0x10cu)==(r32(m,world+0x110u)&0x7fffffffu))havok_grow_array(m,world+0x108u,4u,fp);
        w32(m,r32(m,world+0x108u)+r32(m,world+0x10cu)*4u,body);
        w32(m,world+0x10cu,r32(m,world+0x10cu)+1u);
        w16(m,body+6u,std::uint16_t(r16(m,body+6u)+1u));
      }
    }
  }
  process_dirty_islands(m,fp);
}
}

void havok_world_remove_entity(M& base,std::uint32_t world,std::uint32_t result,std::uint32_t body,HavokFp& fp,
    std::uint32_t sp) {
  with_havok(base,[&](HavokMemory& m){
    if(r8(m,world+0x12cu)==1u) {
      if(r32(m,world+0xf4u)==(r32(m,world+0xf8u)&0x7fffffffu))havok_grow_array(m,world+0xf0u,4u,fp);
      w32(m,r32(m,world+0xf0u)+r32(m,world+0xf4u)*4u,body);
      w32(m,world+0xf4u,r32(m,world+0xf4u)+1u);
      w16(m,body+6u,std::uint16_t(r16(m,body+6u)+1u));
      detail::w8(m,result,0);
      return;
    }
    if(s32(r32(m,world+0x24u))>0)refuse("3198A0","pending world operations 319750 (island merges)");
    remove_entity_broadphase(m,world,body,below(sp,0x10u),fp);
    world_listeners(m,world,0x48u,8u,"322210");
    entity_removed_listeners(m,body);
    detach_entity(m,world,body,fp);
    w16(m,body+6u,std::uint16_t(r16(m,body+6u)-1u));
    if(!r16(m,body+6u)) {
      if(native_tu15_reserved_body_reference(m,{body})){w16(m,body+6u,1);detail::w8(m,result,1);return;}
      refuse("3198A0","entity destructor "+hex(r32(m,r32(m,body))));
    }
    detail::w8(m,result,1);
  });
}

std::uint32_t havok_world_add_entity(M& base,std::uint32_t world,std::uint32_t body,HavokFp& fp,std::uint32_t sp) {
  return with_havok(base,[&](HavokMemory& m)->std::uint32_t{
    const auto saved=r8(m,world+0x12cu);
    detail::w8(m,world+0x12cu,1);
    std::uint32_t id;
    if(r8(m,body+0x40u)){id=r32(m,world+0x134u);w32(m,world+0x134u,id-1u);}
    else{id=r32(m,world+0x130u);w32(m,world+0x130u,id+1u);}
    w32(m,body+0x20u,id);
    attach_entity_island(m,world,body,fp);
    add_entity_broadphase(m,world,body,below(sp,0x14u),fp);
    detail::w8(m,world+0x12cu,saved);
    w16(m,body+6u,std::uint16_t(r16(m,body+6u)+1u));
    world_listeners(m,world,0x48u,0xcu,"3221A0");
    return body;
  });
}

void havok_component_remove(M& base,std::uint32_t component,HavokFp& fp,std::uint32_t sp) {
  with_havok(base,[&](HavokMemory& m){
    if(s32(r32(m,component+0x80u))>0)refuse("1D9510","constraint removal 316AA0");
    for(std::uint32_t k=0,offset=0;s32(k)<s32(r32(m,component+0x74u));++k,offset+=0x60u) {
      const auto body=r32(m,r32(m,component+0x70u)+offset+0x40u);
      remove_listener(m,body,0x70u,r32(m,gameplay_listener_global));
      if((r8(m,component+4u)&1u) && !r8(m,body+0x40u) && motion_kind(m,body,"1D9510")!=6u) {
        const auto listener=r32(m,gameplay_listener_global);
        remove_listener(m,body,0x60u,listener?listener+4u:0u);
      }
      w32(m,entity_count_global,r32(m,entity_count_global)-1u);
      Frame result(m,0x10u);
      havok_world_remove_entity(m,r32(m,havok_world_global),result.at,body,fp,below(sp,0x28u));
      process_dirty_islands(m,fp);
    }
    if(r32(m,component+0x94u))refuse("1D9510","the attached object wake (+94, BBBC0)");
    if(r8(m,component+4u)&1u) {
      if(const auto datum=r32(m,component+0x20u);datum!=detail::none &&
          r16(m,r32(m,r32(m,0x52d690u)+0x44u)+(datum&0xffffu)*0x40u+2u))
        refuse("1D9510","model constraint removal 22D0A0");
      if(const auto capacity=r32(m,component+0x90u);s32(capacity)>=0)
        havok_array_release(m,r32(m,component+0x88u),(capacity&0x7fffffffu)*0x48u,0x12u,fp);
      w32(m,component+0x88u,0);w32(m,component+0x8cu,0);w32(m,component+0x90u,0x80000000u);
    }
    if(const auto phantom=r32(m,component+0x98u))havok_world_remove_phantom(m,r32(m,havok_world_global),phantom,fp);
    if(const auto phantom=r32(m,component+0x9cu))havok_world_remove_phantom(m,r32(m,havok_world_global),phantom,fp);
    w32(m,component+4u,r32(m,component+4u)&~0x20u);
  });
}

void havok_component_add(M& base,std::uint32_t component,HavokFp& fp,std::uint32_t sp) {
  with_havok(base,[&](HavokMemory& m){
    if(s32(r32(m,component+0x80u))>0)refuse("1D97F0","constraint addition 317350");
    if(const auto phantom=r32(m,component+0x9cu))havok_world_add_phantom(m,r32(m,havok_world_global),phantom,fp);
    if(const auto phantom=r32(m,component+0x98u))havok_world_add_phantom(m,r32(m,havok_world_global),phantom,fp);
    const auto object=detail::object_body(m,r32(m,component+8u));
    const auto frame=aligned_frame(sp);
    const auto activation=activation_services(m);
    for(std::uint32_t k=0,offset=0;s32(k)<s32(r32(m,component+0x74u));++k,offset+=0x60u) {
      const auto body=r32(m,r32(m,component+0x70u)+offset+0x40u);
      add_listener(m,body,0x70u,r32(m,gameplay_listener_global),fp);
      if((r8(m,component+4u)&1u) && !r8(m,body+0x40u) && motion_kind(m,body,"1D97F0")!=6u) {
        const auto listener=r32(m,gameplay_listener_global);
        add_listener(m,body,0x60u,listener?listener+4u:0u,fp);
        w16(m,body+0x5cu,0);
      }
      world_add_body(m,body,(r8(m,object+0xc1u)&1u)!=0,below(frame,0x3cu),fp);
      native_tu15_set_phantom_body_velocity(m,component,k,NativeTu15PhantomVelocityKind::linear,
          [&](unsigned lane){return r32(m,object+0x88u+lane*4u);},fp,activation);
      native_tu15_set_phantom_body_velocity(m,component,k,NativeTu15PhantomVelocityKind::angular,
          [&](unsigned lane){return r32(m,object+0x94u+lane*4u);},fp,activation);
      auto record=havok_array_allocate(m,0x70u,0x26u,fp);
      w16(m,record+4u,0x70u);
      record=build_body_record(m,body,record,r32(m,component+8u),fp);
      if((r32(m,component+4u)>>11)&1u) {
        const auto chain=detail::tag_data(m,r32(m,detail::tag_data(m,r32(m,detail::tag_data(m,
            r32(m,detail::object_body(m,r32(m,component+8u))))+0x38u))+0x24u));
        const auto one=detail::f32(r32(m,0x46a0d0u));
        if(!(detail::f32(r32(m,chain+8u))==one) || !(detail::f32(r32(m,chain+0xcu))==one))
          refuse("1D97F0","the scaled body record (31E430/31E560/31E590)");
      }
      if(record)w16(m,record+6u,std::uint16_t(r16(m,record+6u)+1u));
      if(const auto old=r32(m,body+0x54u)) {
        w16(m,old+6u,std::uint16_t(r16(m,old+6u)-1u));
        if(!r16(m,old+6u))release_body_record(m,old,fp);
      }
      w32(m,body+0x54u,record);
      w16(m,record+6u,std::uint16_t(r16(m,record+6u)-1u));
      if(!r16(m,record+6u))release_body_record(m,record,fp);
    }
    if(s32(r32(m,component+0x80u))>0)refuse("1D97F0","constraint addition 317350");
    w32(m,component+4u,(r32(m,component+4u)&~8u)|0x20u);
  });
}

void havok_component_refresh(M& base,std::uint32_t datum,HavokFp& fp) {
  with_havok(base,[&](HavokMemory& m){
    const auto component=r32(m,r32(m,0x52be5cu)+0x44u)+(datum&0xffffu)*0xa0u;
    const auto header=detail::object_row(m,r32(m,component+8u));
    const auto body=r32(m,header+8u);
    const bool active=(r8(m,header+2u)&1u)!=0;
    const auto type=std::int8_t(r8(m,body+0xaau));
    // The byte map at 0x1D74BC (types 0..12) selects the 0x1D74A4 entry;
    // its word 5 (types 2-6 and 8-10) is not an owner.
    static constexpr std::array<std::uint8_t,13> map{0,1,5,5,5,5,5,2,5,5,5,3,4};
    if(type<0 || type>12)refuse("1D73D0","object type "+hex(std::uint8_t(type))+" outside the jump-table map");
    switch(map[std::size_t(type)]) {
    case 0:refuse("1DEE30","unit (biped) shape construction");
    case 1:refuse("1DD9A0","vehicle shape construction");
    case 2:
      w32(m,component+4u,r32(m,component+4u)|0x80u);
      refuse("1DDBF0","machine shape construction");  // its AL becomes +1C
    case 3:refuse("1DD950","crate shape construction");
    case 4:refuse("1DEF50","creature shape construction");
    default:refuse("1D73D0","object type "+hex(std::uint8_t(type))+" without a shape builder");
    }
    // Once a builder is native: 1CD6A0 (retained +88/+94, no force), then
    // +4 |= 1 and bit 17 from the header's +2 bit 0 sampled above.
    havok_object_component_velocities(m,r32(m,component+8u),body+0x88u,body+0x94u,false,fp);
    const auto flags=r32(m,component+4u)|1u;
    w32(m,component+4u,flags);
    w32(m,component+4u,active?flags|0x20000u:flags&~0x20000u);
  });
}

void havok_component_state_flags(M& m,std::uint32_t object,std::uint32_t flags,HavokFp& fp,std::uint32_t sp) {
  NativeTu15ComponentStateServices s;
  // 1CC310 calls 1D9510/1D73D0/1D97F0 16 bytes below its entry ESP. 1D9510
  // (native: havok_component_remove) stays a refusal here while the units
  // track's 1CC310 cases stop at its entry (their fixture has no entity
  // world); bind it as `havok_component_remove(m,row,state,below(sp,0x10u))`
  // together with that stop list's update.
  s.remove=[](std::uint32_t,HavokFp&,unsigned){refuse("1D9510","Havok component removal (bound by units' stops)");};
  s.reset=[&m](std::uint32_t row,HavokFp& state,unsigned){havok_component_reset(m,row,state);};
  // 1D73D0 is refused at its entry until one of its shape builders is
  // native: the composed tracks (units, modelstate) stop their 1CC310 cases
  // there. Then bind havok_component_refresh here and move their stops.
  s.refresh=[](std::uint32_t,HavokFp&,unsigned){refuse("1D73D0","Havok component refresh (shape builders)");};
  s.activate=[&m,sp](std::uint32_t row,HavokFp& state,unsigned){havok_component_add(m,row,state,below(sp,0x10u));};
  native_tu15_set_component_state_flags(m,object,flags,fp,0,s);
}
}
