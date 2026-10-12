#include "halo2/physics_body_slots.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;
using Observer=SourcePhysicsContactRecordObserver;
std::uint32_t read(const Memory&m,std::uint32_t p,unsigned n=4) {
  std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
  return v;
}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
void write(Memory&m,const Observer&o,std::uint32_t site,std::uint32_t p,
           unsigned n,std::uint64_t v) {
  SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(n);s.written.resize(n);
  m.read(p,s.before);for(unsigned i=0;i<n;++i)s.written[i]=std::byte((v>>(i*8))&255u);
  m.write(p,s.written);if(o)o(s);
}
[[noreturn]]void unavailable(const char*s){throw std::runtime_error(s);}
// Original318900 moves following pairs in order. Its out value is a discarded
// source stack local in these owners; the actual property bank stores remain.
void remove_property(Memory&m,std::uint32_t p,std::uint32_t key,const Observer&o) {
  auto n=std::uint32_t{0};
  for(;signed_word(n)<signed_word(read(m,p+0x34));++n)
    if(read(m,read(m,p+0x30)+n*8u)==key)break;
  if(signed_word(n)>=signed_word(read(m,p+0x34)))return;
  write(m,o,0x318953,p+0x34,4,read(m,p+0x34)-1u);
  for(;signed_word(n)<signed_word(read(m,p+0x34));++n) {
    const auto dst=read(m,p+0x30)+n*8u;
    write(m,o,0x31896a,dst,4,read(m,dst+8));
    write(m,o,0x31896f,dst+4,4,read(m,dst+12));
  }
}
void replace_property(Memory&m,std::uint32_t body,std::uint32_t key,
                      std::uint32_t value,const Observer&o) {
  const auto bank=read(m,body+0x30),count=read(m,body+0x34);
  for(std::uint32_t n=0;signed_word(n)<signed_word(count);++n)
    if(read(m,bank+n*8u)==key){remove_property(m,body,key,o);break;}
  source_add_physics_entity_property_1_1(m,body,key,value,o);
}
std::uint32_t motion_kind(const Memory&m,std::uint32_t body) {
  const auto motion=read(m,body+0x3c),target=read(m,read(m,motion)+0x18);
  // Complete original constant-return virtuals; do not read an invented mode
  // field or let a provider claim a successful kind for an unknown vtable.
  if(target==0x31b250)return 7;
  if(target==0x9d900)return 4;
  if(target==0x9b560)return 5;
  if(target==0x31b260)return 6;
  unavailable("Required original virtual motion-kind owner unavailable");
}
}
std::uint32_t source_construct_physics_body_slot_1_1(Memory&m,
 const SourcePhysicsBodySlotConstruction&in,const Observer&o) {
  const auto p=in.slot;
  write(m,o,0x1dc0cc,p+0x40,4,in.body);
  write(m,o,0x1dc0d4,p+0x44,1,0);write(m,o,0x1dc0d8,p+0x45,1,0);
  write(m,o,0x1dc0dc,p+0x4c,4,0);write(m,o,0x1dc0e3,p+0x50,4,0x80000004);
  write(m,o,0x1dc0f1,p+0x48,4,p+0x54);
  auto v=read(m,0x46fc44);
  for(const auto [site,offset,source]:std::array<std::array<std::uint32_t,3>,6>{{
     {0x1dc0fb,0xc,0},{0x1dc100,0x10,4},{0x1dc109,0x14,8},
     {0x1dc111,0x18,0},{0x1dc116,0x1c,4},{0x1dc122,0x20,8}}})
    write(m,o,site,p+offset,4,read(m,v+source));
  v=read(m,0x46fc28);
  for(const auto [site,offset]:std::array<std::array<std::uint32_t,2>,3>{{
      {0x1dc129,0},{0x1dc12e,4},{0x1dc139,8}}})
    write(m,o,site,p+offset,4,read(m,v+offset));
  write(m,o,0x1dc13c,p+0x30,8,0);write(m,o,0x1dc13c,p+0x38,8,0);
  replace_property(m,in.body,0x2003,0,o);
  const auto cap=read(m,p+0x50)&0x7fffffffu;
  if(signed_word(cap)<signed_word(in.count)) {
    const auto twice=cap*2u;
    source_grow_physics_body_bank_1_1(m,p+0x48,
      signed_word(in.count)>=signed_word(twice)?in.count:twice,1,o);
  }
  write(m,o,0x1dc16a,p+0x4c,4,in.count);
  write(m,o,0x1dc17a,p+0x45,1,
    (in.flag_word&255u)?read(m,p+0x45,1)|1u:read(m,p+0x45,1)&0xfeu);
  for(std::uint32_t n=0;signed_word(n)<signed_word(in.count);++n)
    write(m,o,0x1dc18b,read(m,p+0x48)+n,1,read(m,in.body_indices+n,1));
  return p;
}
void source_refresh_physics_body_slot_1_1(Memory&m,std::uint32_t component,
 std::uint32_t index,const Observer&o) {
  const auto slot=read(m,component+0x70)+index*0x60u;
  if(read(m,component+4)&0x40u)
    unavailable("Required original1D42D0 component-position owner unavailable");
  const auto body=read(m,slot+0x40),motion=read(m,body+0x3c);
  std::array<std::uint32_t,9> words{};
  for(unsigned n=0;n<3;++n)words[n]=read(m,motion+0xb0+n*4u);
  const auto inactive=read(m,body+0x40,1)!=0;
  for(unsigned n=0;n<3;++n) {
    words[n+3]=read(m,(inactive?read(m,0x46fc44):motion+0x40)+n*4u);
    words[n+6]=read(m,(inactive?read(m,0x46fc44):motion+0x50)+n*4u);
  }
  constexpr std::array sites{0x1d2a5bu,0x1d2a61u,0x1d2a68u,
    0x1d2a6eu,0x1d2a74u,0x1d2a7eu,0x1d2a82u,0x1d2a85u,0x1d2a89u};
  for(unsigned n=0;n<9;++n)write(m,o,sites[n],slot+n*4u,4,words[n]);
}
std::uint32_t source_commit_physics_body_slot_1_1(Memory&m,
 const SourcePhysicsBodySlotCommit&in,const Observer&o) {
  const auto p=in.component,old_count=read(m,p+0x74);
  const auto object=read(m,read(m,read(m,0x4e78d0)+0x44)+(read(m,p+8)&0xffffu)*12u+8);
  if(old_count==(read(m,p+0x78)&0x7fffffffu))
    source_double_physics_body_bank_1_1(m,p+0x70,0x60,o);
  const auto slot=read(m,p+0x70)+read(m,p+0x74)*0x60u;
  write(m,o,0x1d3acc,p+0x74,4,read(m,p+0x74)+1u);
  // OriginalJZ retains1D3AC7 ADD flags across LEA/MOV: it tests the slot.
  if(slot!=0)
    source_construct_physics_body_slot_1_1(m,
      {slot,in.body,in.body_indices,in.count,in.flag_word},o);
  source_refresh_physics_body_slot_1_1(m,p,old_count,o);
  replace_property(m,in.body,0x2001,read(m,object+0xb4),o);
  replace_property(m,in.body,0x2002,old_count,o);
  auto kind=motion_kind(m,in.body);
  if(kind!=7) {
    kind=motion_kind(m,in.body);
    if(kind!=6)write(m,o,0x1d3b24,p+4,4,read(m,p+4)|0x8000u);
  }
  return kind;
}
} // namespace halo2
