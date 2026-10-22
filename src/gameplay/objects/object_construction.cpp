#include "halo2/object_construction.hpp"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace halo2 { namespace {
constexpr std::uint32_t none=0xffffffffu;
using R=ObjectConstructionRegister;
std::uint32_t plus(std::uint32_t a,std::uint32_t b) {
  if(b>std::numeric_limits<std::uint32_t>::max()-a) throw std::out_of_range("object construction address overflow");
  return a+b;
}
struct Owner {
 const SourceObjectConstructionState &s;const SourceGenericObjectConstructionServices &v;
 ObjectConstructionFloatingPointContext &fp;
 const ObjectConstructionObserver &observer;
 std::uint32_t get(std::uint32_t a,unsigned n=4)const {
  auto b=v.read(a,n);if(b.size()!=n)throw std::out_of_range("object construction read extent");
  std::uint32_t x=0;for(unsigned i=0;i<n;++i)x|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return x;
 }
 void put(std::uint32_t pc,std::uint32_t a,std::uint32_t x,unsigned n=4)const {
  auto b=v.memory(a,n);if(b.size()!=n)throw std::out_of_range("object construction write extent");
  std::uint32_t before=0;for(unsigned i=0;i<n;++i)before|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  for(unsigned i=0;i<n;++i)b[i]=std::byte((x>>(8*i))&255);
  if(observer)observer({pc,a,n,before,x&(n==4?none:((1u<<(n*8))-1))});
 }
 std::uint32_t local(unsigned o)const{return plus(s.private_frame,o);}
 std::uint32_t header(std::uint32_t datum)const {
  auto p=get(s.object_pool_slot);auto capacity=get(plus(p,0x20));
  auto index=datum&65535;if(index>=capacity)throw std::out_of_range("object construction header index");
  return plus(get(plus(p,0x44)),index*12);
 }
 std::uint32_t body(std::uint32_t datum)const{return get(plus(header(datum),8));}
 std::uint32_t tag(std::uint32_t datum)const {
  return get(plus(get(s.tag_table_slot),(datum&65535)*16+8));
 }
 std::uint32_t call(std::uint32_t pc,std::uint32_t entry,
     std::initializer_list<std::pair<R,std::uint32_t>> regs={},
     std::initializer_list<std::uint32_t> args={},bool xmm0=false)const {
  ObjectConstructionCall c{};c.instruction=pc;c.entry=entry;c.xmm0_zero=xmm0;
  for(auto [r,x]:regs){auto i=unsigned(r);c.register_mask|=1u<<i;c.registers[i]=x;}
  for(auto x:args){if(c.argument_count==c.arguments.size())throw std::logic_error("object construction argument extent");c.arguments[c.argument_count++]=x;}
  return v.invoke(c,fp);
 }
};
}
bool source_object_construction_positive_scalar(std::uint32_t bits,
    ObjectConstructionFloatingPointContext &fp) {
  auto magnitude=bits&0x7fffffffu;
  if(magnitude>0x7f800000u) {
   fp.mxcsr|=1;
   if(!(fp.mxcsr&0x80))throw std::runtime_error("source COMISS unmasked invalid exception");
   return false;
  }
  if(magnitude&&magnitude<0x800000u) {
   if(fp.mxcsr&0x40)return false; // actual DAZ control precedes denormal use
   fp.mxcsr|=2;
   if(!(fp.mxcsr&0x100))throw std::runtime_error("source COMISS unmasked denormal exception");
  }
  return magnitude&&!(bits&0x80000000u);
}
std::uint32_t construct_source_object(const SourceObjectConstructionState &s,
    std::uint32_t q,const ObjectConstructionOperands &operands,
    ObjectConstructionFloatingPointContext &fp,
    const SourceGenericObjectConstructionServices &v,const ObjectConstructionObserver &observer) {
 if(!v.read||!v.memory||!v.base_extent||!v.invoke)throw std::invalid_argument("object construction requires owning providers");
 Owner o{s,v,fp,observer};
 auto read=[&](std::uint32_t a,unsigned n=4){return o.get(a,n);};
 auto write=[&](std::uint32_t pc,std::uint32_t a,std::uint32_t x,unsigned n=4){o.put(pc,a,x,n);};
 auto at=[&](unsigned n){return plus(q,n);};
 auto l=[&](unsigned n){return o.local(n);};
 write(0xb8a43,l(0x14),none);write(0xb8a4b,l(0x1b),0,1);
 if(!(read(at(0x18),1)&0x10)) {
  if(read(q)==none)return none;
  o.call(0xb8a5e,0x10cbd0,{{R::Ebx,q}});
 }
 auto definition=read(q);if(definition==none)return none;
 auto def=o.tag(definition);auto type=std::bit_cast<std::int16_t>(std::uint16_t(read(def,2)));
 write(0xb8a84,l(0x20),def);
 auto model=read(plus(def,0x38));auto hlmt=model==none?0:o.tag(model);
 write(0xb8a96,l(0x1c),0);if(hlmt)write(0xb8aac,l(0x1c),hlmt);
 auto bit=1u<<(std::uint16_t(type)&31);
 if(bit&0x1883)o.call(0xb8abf,0x148360);
 auto unit=o.call(0xb8acb,0xbd170,{}, {v.base_extent(type)});
 write(0xb8ad3,l(0x14),unit);if(unit==none)return none;
 auto h=o.header(unit), b=read(plus(h,8));
 write(0xb8afe,plus(h,2),read(plus(h,2),1)|8,1);
 write(0xb8b01,l(0x24),(unit&65535)*12);
 write(0xb8b0b,plus(h,3),read(def,1),1);
 write(0xb8b11,b,read(q));write(0xb8b17,l(0x34),h);write(0xb8b1b,l(0x13),0,1);
 if(read(at(0xb),1)==255) {
  auto world=read(s.world_slot);write(0xb8b2a,plus(world,0x18),read(plus(world,0x18))+1);
  write(0xb8b2d,plus(b,0xaa),read(def,2),1);
  write(0xb8b3c,plus(b,0xab),2,1);write(0xb8b43,plus(b,0xa8),65535,2);
  write(0xb8b4d,plus(b,0xa4),read(plus(world,0x18)));write(0xb8b53,plus(b,0x1a),65535,2);
  write(0xb8b57,plus(b,0xae),operands.sentinel_word,1);
 } else {
  write(0xb8b62,plus(b,0xa4),read(at(4)));write(0xb8b6b,plus(b,0xa8),read(at(8)));
  write(0xb8b77,plus(b,0x1a),read(at(0x10),2),2);write(0xb8b7b,plus(b,0xae),read(at(8)),1);
 }
 constexpr std::array<unsigned,5> from{0x1c,0x28,0x34,0x40,0x4c},to{0x64,0x70,0x7c,0x88,0x94};
 constexpr std::array<std::array<std::uint32_t,3>,5> pcs{{{0xb8b8c,0xb8b91,0xb8b97},{0xb8ba2,0xb8ba7,0xb8bad},{0xb8bb8,0xb8bbd,0xb8bc3},{0xb8bd1,0xb8bd6,0xb8bdc},{0xb8bea,0xb8bef,0xb8bf5}}};
 for(unsigned i=0;i<5;++i)for(unsigned j=0;j<3;++j)write(pcs[i][j],plus(b,to[i]+j*4),read(at(from[i]+j*4)));
 write(0xb8bfb,plus(b,0xa0),read(at(0x58)));
 write(0xb8c16,plus(b,4),(read(plus(b,4))&~0x400u)|((read(at(0x18),1)&1)?0x400:0));
 write(0xb8c3f,plus(b,4),(read(plus(b,4))&~0x200u)|((hlmt&&read(plus(hlmt,0xc))!=none)?0x200:0));
 auto result=o.call(0xb8c50,0xc0890,{{R::Ecx,unit}},{l(0x2c),l(0x28)});
 auto flags=(read(plus(b,4))&0x7fffffffu)|((result&255)?0x80000000u:0);
 write(0xb8c73,plus(b,4),flags);write(0xb8c76,plus(h,4),65535,2);
 write(0xb8c7a,plus(b,0x28),none);write(0xb8c7d,plus(b,0x2c),65535,2);write(0xb8c81,plus(b,0x2e),operands.sentinel_word,2);
 write(0xb8c85,plus(b,0x60),none);write(0xb8c8b,plus(b,0xb0),read(at(0x14),1),1);
 write(0xb8c91,plus(b,0x14),none);write(0xb8c94,plus(b,0xc),none);write(0xb8c97,plus(b,0x10),none);
 write(0xb8c9a,plus(b,0xac),65535,2);write(0xb8ca1,plus(b,0xaf),255,1);write(0xb8ca8,plus(b,0x108),255,1);write(0xb8caf,plus(b,0x109),255,1);
 if(read(plus(def,2),1)&1)write(0xb8cbc,plus(b,4),read(plus(b,4))|0x10000);
 auto now=o.body(unit);
 if(read(plus(now,4),1)&1) {
  write(0xb8ce4,plus(now,4),read(plus(now,4))&~1u);
  if(o.call(0xb8ce9,0xbac10,{{R::Eax,unit}})&255)o.call(0xb8cfb,0xbfe20,{}, {unit,0,1,0,0});
  o.call(0xb8d04,0xb9a60,{{R::Esi,unit}});
 }
 write(0xb8d11,plus(b,0xc2),read(at(0x70),2),2);write(0xb8d1b,plus(b,0xc4),read(at(0x68)));write(0xb8d24,plus(b,0xc8),read(at(0x6c)));
 write(0xb8d2a,plus(b,0xb1),255,1);write(0xb8d31,plus(b,0xcc),none);write(0xb8d37,plus(b,0xd0),65535,2);write(0xb8d3e,plus(b,0xc0),0,2);
 auto f=(read(at(0x18))&8)!=0;write(f?0xb8d52:0xb8d5b,plus(b,0xc1),(read(plus(b,0xc1),1)&254)|(f?1:0),1);
 write(0xb8d62,plus(b,0xb4),none);write(0xb8d68,plus(b,0xd4),none);write(0xb8d6e,plus(b,0xd8),0,1);
 write(0xb8d7c,plus(b,0xe0),read(at(0xb0),2),2);write(0xb8d8a,plus(b,0xe2),read(at(0xb2),2),2);
 auto header_bit=1u<<(read(plus(h,3),1)&31);bool six=false;
 if(!(header_bit&0x80))six=std::bit_cast<std::int32_t>(read(plus(read(s.clock_slot),8)))>=6;
 write(six?0xb8db1:0xb8db9,plus(b,0xb3),six?6:0,1);
 std::int32_t nodes=1,other=1;std::uint32_t extra=0;bool matrices=false,manager=false;
 write(0xb8dcd,l(0x2c),1);write(0xb8dd1,l(0x28),1);write(0xb8dd5,l(0x30),0);
 if(hlmt) {
  auto n=std::bit_cast<std::int32_t>(read(plus(hlmt,0x78)));if(n>=1){nodes=n;write(0xb8dea,l(0x2c),std::uint32_t(n));}
  n=std::bit_cast<std::int32_t>(read(plus(hlmt,0x70)));if(n>=1){other=n;write(0xb8df5,l(0x28),std::uint32_t(n));}
  if(std::bit_cast<std::int32_t>(read(plus(hlmt,0x60)))>0){extra=read(plus(read(plus(hlmt,0x64)),0xbc));write(0xb8e09,l(0x30),extra);}
  if(read(plus(hlmt,0x14))!=none) {
   o.call(0xb8e17,0x1ce8c0,{{R::Eax,l(0x38)}});
   auto graph=read(plus(read(l(0x1c)),0x14));auto actual_model=read(plus(read(l(0x20)),0x38));
   if(o.call(0xb8e30,0x1ceac0,{{R::Esi,l(0x38)},{R::Edi,graph},{R::Edx,actual_model}}, {1})&255) {
    manager=true;write(0xb8e46,l(0x13),1,1);
    auto type_bit=1u<<(read(read(l(0x20)),1)&31);matrices=(type_bit&0x7e0)==0;
    if(type_bit&0x380) {auto d=o.tag(read(q));if(read(plus(d,0xbc),1)&4)matrices=true;}
   }
   o.call(0xb8e84,0x1cea90,{{R::Eax,l(0x38)}});
  }
 }
 auto append=[&](std::uint32_t pc,std::uint32_t descriptor,std::uint32_t bytes,std::uint32_t alignment) {
  return (o.call(pc,0xbd270,{{R::Eax,unit}}, {descriptor,bytes,alignment})&255)!=0;
 };
 bool admitted=append(0xb8ea8,0x11c,std::uint16_t(read(plus(def,0x94),2)<<3),0);
 if(admitted)admitted=append(0xb8eca,0x120,extra*8,0);
 if(admitted)admitted=append(0xb8eee,0x124,std::uint16_t(read(plus(def,0xac),2)*24),0);
 if(admitted)admitted=append(0xb8f0e,0x114,std::uint32_t(nodes)*0x34,0);
 if(admitted)admitted=append(0xb8f2e,0x118,std::uint32_t(other)*10,0);
 if(admitted)admitted=append(0xb8f4e,0x110,matrices?std::uint32_t(nodes)*32:0,4);
 if(admitted)admitted=append(0xb8f6e,0x10c,matrices?std::uint32_t(nodes)*32:0,4);
 if(admitted)admitted=append(0xb8f8f,0x128,manager?0x90:0,0);
 if(admitted)admitted=(o.call(0xb8f9b,0x1c7290,{{R::Ecx,read(q)}})&255)!=0;
 if(!admitted)write(0xb8faa,l(0x1b),1,1);
 b=o.body(unit);
 auto release=[&](){o.call(0xb9278,0xbd1f0,{}, {unit});write(0xb927d,l(0x14),none);return none;};
 if(!admitted)return release();
 if(read(l(0x13),1)) {
  auto offset=std::bit_cast<std::int16_t>(std::uint16_t(read(plus(b,0x12a),2)));
  // Signed component offsets are actual retained fields; reject host wrap.
  auto manager_at=std::int64_t(b)+offset;if(manager_at<0||manager_at>none)throw std::out_of_range("object construction manager offset");
  auto m=o.call(0xb8fd9,0x1ce9b0,{{R::Eax,std::uint32_t(manager_at)}});
  auto graph=read(plus(read(l(0x1c)),0x14)),actual_model=read(plus(read(l(0x20)),0x38));
  auto ok=o.call(0xb8ff0,0x1ceac0,{{R::Esi,m},{R::Edi,graph},{R::Edx,actual_model}}, {1})&255;
  write(ok?0xb9001:0xb900b,plus(b,4),(read(plus(b,4))&~0x800u)|(ok?0x800:0));
 } else write(0xb9010,plus(b,0xb3),0,1);
 def=read(l(0x20));
 if(std::bit_cast<std::int32_t>(read(plus(def,0x94)))>0) {
  auto signed_size=std::bit_cast<std::int16_t>(std::uint16_t(read(plus(b,0x11c),2)));
  auto bytes=(std::uint32_t(std::int32_t(signed_size))>>3)<<3;
  auto offset=std::bit_cast<std::int16_t>(std::uint16_t(read(plus(b,0x11e),2)));
  auto begin=std::int64_t(b)+offset;if(begin<0||begin>none)throw std::out_of_range("object construction clear offset");
  auto p=std::uint32_t(begin);for(std::uint32_t i=0;i<bytes/4;++i){write(0xb9043,p,none);p=plus(p,4);}
  for(unsigned i=0;i<(bytes&3);++i){write(0xb904a,p,255,1);p=plus(p,1);}
 }
 result=o.call(0xb9056,0x10cc20,{{R::Ebx,unit}}, {q,l(0x1b)});b=o.body(unit);write(0xb9073,l(0x1c),b);
 if(!(result&255)){o.call(0xb926e,0x10cd80,{{R::Ebx,unit}});return release();}
 bool saved=(read(plus(b,4))&0x20000)!=0;write(0xb9085,l(0x13),saved?1:0,1);
 if(read(at(0x18))&6)write(0xb9090,plus(b,4),read(plus(b,4))&~0x20000u);
 o.call(0xb909c,0xbb430,{{R::Eax,unit},{R::Ecx,read(at(0xc))}});
 o.call(0xb90a9,0xbb480,{}, {unit,read(at(0xac))});
 o.call(0xb90b7,0xbf130,{}, {unit,read(at(0x74)),at(0x78)});
 auto current=o.body(unit);auto p=o.call(0xb90ca,0xd7160,{{R::Ecx,unit}});
 std::uint32_t x=0,y=0;if(p){x=read(plus(p,0x28));y=read(plus(p,0x8c));}
 write(0xb90f1,plus(current,0xe4),x);auto positive_x=source_object_construction_positive_scalar(x,fp);
 write(0xb90fc,plus(current,0xe8),y);auto positive_y=source_object_construction_positive_scalar(y,fp);
 write(0xb910f,plus(current,0xec),positive_x?operands.one_bits:0);
 write(0xb911d,plus(current,0xf0),positive_y?operands.one_bits:0);
 o.call(0xb9125,0xbf7d0,{}, {unit});b=read(l(0x1c));write(0xb9139,plus(b,0x24),read(at(0xa8)));
 if(read(plus(o.body(unit),0x12a),2)!=65535)o.call(0xb914f,0xbb2c0,{{R::Eax,unit}});
 o.call(0xb9155,0xbdf10,{}, {unit});
 auto world=read(s.world_slot);
 if(world&&read(world,1)) {
  if(!read(at(0xb8),1)) {
   auto ok=o.call(0xb9179,0xc0400,{{R::Edi,at(0xbc)}}, {unit})&255;
   b=read(l(0x1c));write(0xb9184,at(0xb8),ok,1);
  }
  auto location=read(at(0xb8),1)?at(0xbc):0;o.call(0xb9198,0xb94f0,{}, {unit,location});
 }
 o.call(0xb919f,0xbf0c0,{{R::Eax,unit}});o.call(0xb91a6,0xb8250,{{R::Eax,unit}});
 f=(read(at(0x18))&0x20)!=0;write(f?0xb91b5:0xb91be,plus(b,0xc0),(read(plus(b,0xc0),1)&253)|(f?2:0),1);
 o.call(0xb91ca,0xb9780,{}, {unit});o.call(0xb91cf,0xbf580,{{R::Esi,unit}});o.call(0xb91d6,0x10cd10,{{R::Ebx,unit}});
 auto effect=read(plus(read(l(0x20)),0x50));
 if(effect!=none)o.call(0xb91f6,0x1781f0,{{R::Edx,unit},{R::Ebx,at(0x68)}},{effect,0,0,0},true);
 o.call(0xb91fd,0x2e1c0,{{R::Eax,unit}});b=read(l(0x1c));
 saved=read(l(0x13),1)!=0;write(0xb921f,plus(b,4),(read(plus(b,4))&~0x20000u)|(saved?0x20000:0));
 o.call(0xb9222,0x10d520,{{R::Edx,unit}});
 if((read(plus(b,4))&0x20000)&&!(read(plus(read(l(0x34)),2),1)&1)) {
  if((o.call(0xb923c,0xb9710)&255)&&!(read(at(0x18))&2)) {
   if(!(read(at(0x18))&4)||read(plus(b,0x2c),2)!=65535)o.call(0xb9258,0xb9430,{}, {unit});
  }
 }
 return unit;
}
} // namespace halo2
