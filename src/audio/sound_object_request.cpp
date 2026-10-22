#include "halo2/sound_object_request.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <unordered_set>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
template<class F> const F &required(const F &f,const char *why) {
  if(!f)throw FormatError(why);
  return f;
}
std::uint32_t word(std::span<const std::byte> data,std::size_t offset,unsigned bytes=4) {
  if(offset>data.size() || bytes>data.size()-offset)
    throw FormatError("Original sound object packet is truncated");
  std::uint32_t v=0;for(unsigned i=0;i<bytes;++i)v|=std::to_integer<std::uint32_t>(data[offset+i])<<(8*i);
  return v;
}
std::span<const std::byte> read(const SoundObjectRequestMemory &m,std::uint32_t p,std::size_t n) {
  const auto data=required(m.read,"Original sound object memory owner unavailable")(p,n);
  if(data.size()<n)throw FormatError("Original sound object memory owner returned truncated storage");
  return data.first(n);
}
std::uint32_t u(const SoundObjectRequestMemory &m,std::uint32_t p,unsigned n=4) {
  return word(read(m,p,n),0,n);
}
std::int32_t i32(std::uint32_t v) {return std::bit_cast<std::int32_t>(v);}
std::int16_t i16(std::uint32_t v) {return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t header(const SoundObjectRequestMemory &m,std::uint32_t d) {
  return u(m,m.object_pool+0x44)+(d&0xffffu)*12u;
}
std::uint32_t body(const SoundObjectRequestMemory &m,std::uint32_t d) {
  return u(m,header(m,d)+8);
}
std::uint32_t root(const SoundObjectRequestMemory &m,std::uint32_t d) {
  std::uint32_t last=none;std::unordered_set<std::uint32_t> visited;
  while(d!=none) {
    if(!visited.insert(d).second)throw FormatError("Cyclic original sound object parent chain");
    last=d;d=u(m,body(m,d)+0x14);
  }
  return body(m,last); // NONE is not a synthesized root-location result.
}
std::uint32_t valid_body(const SoundObjectRequestMemory &m,std::uint32_t d) {
  if(d==none || std::int32_t(d&0xffffu)>=i32(u(m,m.object_pool+0x38)))return 0;
  const auto h=u(m,m.object_pool+0x44)+u(m,m.object_pool+0x24)*(d&0xffffu);
  const auto salt=i16(u(m,h,2));
  if(!salt || std::int32_t(salt)!=(i32(d)>>16))return 0;
  // BBCB0's mask isFFFFFFFF at this caller, so every actual header type is
  // admitted. It still returns the literal header body pointer, including0.
  return u(m,h+8);
}
bool rejected(const SoundObjectRequestMemory &m,std::uint32_t d) {
  std::unordered_set<std::uint32_t> visited;
  for(;;) {
    if(!visited.insert(d).second)throw FormatError("Cyclic original rejected sound parent chain");
    const auto h=header(m,d),b=u(m,h+8);
    if((u(m,h+2,1)&0x10) || (u(m,b+4,1)&1))return true;
    d=u(m,b+0x14);if(d==none)return false;
  }
}
template<std::size_t N> std::array<std::byte,N> packet(const SoundObjectRequestMemory &m,std::uint32_t p) {
  std::array<std::byte,N> out;const auto data=read(m,p,N);
  std::copy(data.begin(),data.end(),out.begin());return out;
}
float f(std::span<const std::byte> a,std::size_t p) {return std::bit_cast<float>(word(a,p));}
float mul(float a,float b) {volatile float q=a*b;return q;}
float add(float a,float b) {volatile float q=a+b;return q;}
float sub(float a,float b) {volatile float q=a-b;return q;}
// FISTP of an already spilled binary32. Integer logic implements the actual
// source RC bits independently of the host's ambient floating-point mode.
std::uint32_t fistp(float value,std::uint16_t control) {
  const auto bits=std::bit_cast<std::uint32_t>(value),magnitude=bits&0x7fffffffu;
  const bool negative=bits>>31;
  if(magnitude>=0x7f800000u)return 0x80000000u;
  if(!magnitude)return 0;
  const int exponent=(magnitude>>23)?int((magnitude>>23)&255)-127:-126;
  const std::uint64_t significand=(magnitude&0x7fffffu)|((magnitude>>23)?0x800000u:0u);
  std::uint64_t integer=0,remainder=0,half=0;
  if(exponent>=31) {
    if(negative && magnitude==0x4f000000u)return 0x80000000u;
    return 0x80000000u;
  }
  if(exponent>=23)integer=significand<<(exponent-23);
  else {
    const unsigned shift=unsigned(23-exponent);
    if(shift<64) {integer=significand>>shift;remainder=significand&((std::uint64_t(1)<<shift)-1);half=std::uint64_t(1)<<(shift-1);}
    else remainder=significand;
  }
  const unsigned mode=(control>>10)&3;
  bool increment=false;
  if(mode==0)increment=half && (remainder>half || (remainder==half && (integer&1)));
  else if(mode==1)increment=negative && remainder;
  else if(mode==2)increment=!negative && remainder;
  if(increment)++integer;
  if(integer>(negative?0x80000000ull:0x7fffffffull))return 0x80000000u;
  const auto out=std::uint32_t(integer);return negative?0u-out:out;
}
void store(std::span<std::byte,48> location,std::size_t offset,std::uint32_t value,unsigned size,
           std::uint32_t instruction,const SoundObjectRequestObserver &observe) {
  const auto before=word(location,offset,size);
  for(unsigned i=0;i<size;++i)location[offset+i]=std::byte(value>>(8*i));
  if(observe)observe({instruction,offset,size,before,value});
}
struct Velocity {
  std::array<std::byte,12> data;
  std::array<std::uint32_t,3> instructions;
};
Velocity velocity(std::uint32_t d,const SoundObjectRequestMemory &m,
                  const SoundObjectRequestConstants &c) {
  const auto b=root(m,d),physical=u(m,b+0xb4);
  if(physical!=none) {
    const auto records=u(m,m.physics_pool+0x44),record=records+(physical&0xffffu)*0xa0u;
    const auto selected=std::bit_cast<std::int8_t>(std::uint8_t(u(m,record+0x18,1)));
    const auto count=i32(u(m,record+0x74));
    if(selected>=0 && selected<count) {
      if(u(m,b+0xc1,1)&1)return {c.zero_vector,{0xbb15d,0xbb162,0xbb168}};
      const auto motion=u(m,u(m,record+0x70)+std::uint32_t(selected)*0x60u+0x40);
      if(u(m,motion+0x40,1))return {c.zero_vector,{0x1d43fb,0x1d4400,0x1d4406}};
      return {packet<12>(m,u(m,motion+0x3c)+0x40),{0x1d43e6,0x1d43e8,0x1d43ed}};
    }
  }
  return {packet<12>(m,b+0x88),{0xbb1b6,0xbb1bb,0xbb1c1}};
}
bool location_activity(std::uint32_t d,const SoundObjectRequestMemory &m) {
  if(d==none)return false;
  const auto b=body(m,d),a=b+std::uint32_t(std::int32_t(i16(u(m,b+0x12a,2))));
  std::uint32_t selected=none;
  if(u(m,a+0x68)!=none && u(m,a)!=none && i16(u(m,a+6,2))!=-1)selected=u(m,a+0x7c);
  return selected==0x0e0000c2u;
}
void type7_location(std::uint32_t b,std::array<std::byte,8> &location,
                    const SoundObjectRequestServices &s) {
  const auto &m=s.memory;const auto object_slot=i16(u(m,b+0x1da,2));
  if(object_slot==-1)return;
  bool any=false;
  for(unsigned v=0;v<4;++v)if(required(s.associations.local_player,
      "Original PVS sound local-player owner unavailable")(v)!=none){any=true;break;}
  if(!any)return;
  //126AB0 repeats the source first-valid-user scan before resolving the bank.
  std::int32_t selected=-1;
  for(unsigned v=0;v<4;++v)if(s.associations.local_player(v)!=none){selected=std::int32_t(v);break;}
  const auto listener=m.sound_system+0x88+std::uint32_t(selected)*0x48u;
  if(!listener || !u(m,listener+6,1))return;
  const auto cluster=i16(u(m,listener+4,2));if(cluster==-1)return;
  const auto bank=m.render_bsp;
  if(i32(u(m,bank+0x224))<=0)return;
  const auto pvs=u(m,bank+0x228);
  const auto count=i32(u(m,pvs+0x2c));
  std::int32_t slot=-1;
  for(std::int32_t n=0;n<count;++n)if(std::int32_t(std::bit_cast<std::int8_t>(
      std::uint8_t(u(m,u(m,pvs+0x30)+std::uint32_t(n),1))))==object_slot){slot=n;break;}
  if(slot==-1)return;
  //24BF30 repeats+224, then tests the ordered two bit banks; bank0 wins.
  if(i32(u(m,bank+0x224))<=0)return;
  const auto words=i32(u(m,bank+0x9c)+0x1f)>>5;
  const auto bit=1u<<(std::uint32_t(std::int32_t(cluster))&31);
  const auto bits=u(m,u(m,bank+0x228)+0x10);
  std::int32_t choice=-1;
  for(std::int32_t n=0;n<2;++n) {
    const auto index=std::uint32_t(words)*(std::uint32_t(slot)*2u+std::uint32_t(n))+
                     std::uint32_t(std::int32_t(cluster)>>5);
    if(u(m,bits+index*4)&bit){choice=n;break;}
  }
  if(choice==-1)return;
  const auto mapping=u(m,u(m,bank+0x220)+0xc);
  const auto mapped=i16(u(m,mapping+std::uint32_t(std::uint16_t(object_slot))*2,2));
  const auto address=u(m,bank+0x60)+(std::uint32_t(choice)+std::uint32_t(std::int32_t(mapped))*0x12u)*2u;
  const auto result=u(m,address,2);location[4]=std::byte(result);location[5]=std::byte(result>>8);
}
}

std::uint32_t source_sound_compressed_forward(Vec3 v,const SoundObjectRequestConstants &c,
                                             std::uint16_t fpcw) {
  const float lo=std::bit_cast<float>(c.minus_one_bits),hi=std::bit_cast<float>(c.one_bits);
  const auto clamp=[&](float a){return lo>a?lo:(a>hi?hi:a);};
  const auto z=fistp(mul(sub(clamp(v.z),lo),std::bit_cast<float>(c.z_scale_bits)),fpcw);
  const auto y=fistp(mul(sub(clamp(v.y),lo),std::bit_cast<float>(c.xy_scale_bits)),fpcw);
  const auto x=fistp(mul(sub(clamp(v.x),lo),std::bit_cast<float>(c.xy_scale_bits)),fpcw);
  return (((z<<11)|y)<<11)|x;
}
Vec3 source_sound_marker_position(std::span<const std::byte,52> matrix,Vec3 p,
                                  const SoundObjectRequestConstants &c) {
  const auto scale=f(matrix,0);
  if(scale!=std::bit_cast<float>(c.one_bits)) {p.x=mul(scale,p.x);p.y=mul(scale,p.y);p.z=mul(scale,p.z);}
  return {
    add(add(add(mul(f(matrix,0x1c),p.z),mul(f(matrix,0x10),p.y)),mul(f(matrix,4),p.x)),f(matrix,0x28)),
    add(add(add(mul(f(matrix,0x20),p.z),mul(f(matrix,0x14),p.y)),mul(f(matrix,8),p.x)),f(matrix,0x2c)),
    add(add(add(mul(f(matrix,0x24),p.z),mul(f(matrix,0x18),p.y)),mul(f(matrix,0xc),p.x)),f(matrix,0x30))};
}
std::array<std::byte,8> source_sound_object_location(std::uint32_t d,const SoundObjectRequestMemory &m) {
  return packet<8>(m,root(m,d)+0x28);
}
std::array<std::byte,52> source_sound_object_node(std::uint32_t d,std::int16_t n,
                                               const SoundObjectRequestMemory &m) {
  const auto b=body(m,d);
  return packet<52>(m,b+std::uint32_t(std::int32_t(i16(u(m,b+0x116,2)))+std::int32_t(n)*0x34));
}
std::array<std::byte,12> source_sound_object_velocity(std::uint32_t d,
    const SoundObjectRequestMemory &m,const SoundObjectRequestConstants &c) {return velocity(d,m,c).data;}
bool source_sound_object_request(std::uint32_t owner,std::uint32_t sound,
    std::span<const std::byte,40> marker,std::span<std::byte,48> location,
    const SoundObjectRequestConstants &c,std::uint16_t fpcw,
    const SoundObjectRequestServices &s,const SoundObjectRequestObserver &observe) {
  const auto &m=s.memory;const auto b=valid_body(m,owner);
  if(!b || rejected(m,owner))return false;
  if(u(m,b+0x10a,1)&4) {
    const auto definition=required(s.associations.sound_definition,"Original sound owner definition unavailable")(sound);
    const auto cls=required(s.associations.sound_class,"Original sound owner class unavailable")(
        std::bit_cast<std::int8_t>(definition[2]));
    if(word(cls,0xa,2)&0x800)return false;
  }
  auto packed=source_sound_object_location(owner,m);
  if(i16(word(packed,4,2))==-1)return false;
  const auto type=u(m,header(m,owner)+3,1);
  const auto marker_node=word(marker,0,1);
  const auto matrix=source_sound_object_node(owner,std::int16_t(marker_node<255?marker_node:0),m);
  const auto position=source_sound_marker_position(matrix,{f(marker,4),f(marker,8),f(marker,0xc)},c);
  store(location,0xc,std::bit_cast<std::uint32_t>(position.x),4,0x143d48,observe);
  store(location,0x10,std::bit_cast<std::uint32_t>(position.y),4,0x143d74,observe);
  store(location,0x14,std::bit_cast<std::uint32_t>(position.z),4,0x143da1,observe);
  const Vec3 forward={f(marker,0x10),f(marker,0x14),f(marker,0x18)};
  const Vec3 transformed={
    add(add(mul(f(matrix,0x1c),forward.z),mul(f(matrix,0x10),forward.y)),mul(f(matrix,4),forward.x)),
    add(add(mul(f(matrix,0x20),forward.z),mul(f(matrix,0x14),forward.y)),mul(f(matrix,8),forward.x)),
    add(add(mul(f(matrix,0x24),forward.z),mul(f(matrix,0x18),forward.y)),mul(f(matrix,0xc),forward.x))};
  store(location,0x18,source_sound_compressed_forward(transformed,c,fpcw),4,0x18df47,observe);
  const auto v=velocity(owner,m,c);
  for(unsigned i=0;i<3;++i)store(location,0x1c+i*4,word(v.data,i*4),4,v.instructions[i],observe);
  if((1u<<(type&31))&3) {
    if(location_activity(owner,m))packed=required(s.point_location,
        "Original sound active-unit point/PVS location owner unavailable")(position);
  } else if(type==7)type7_location(b,packed,s);
  store(location,0x2c,word(packed,4),4,0x18e01d,observe);
  store(location,0x28,word(packed,0),4,0x18e02d,observe);
  const auto definition=required(s.associations.sound_definition,"Original sound owner audibility definition unavailable")(sound);
  auto audible=std::uint8_t(std::bit_cast<std::int8_t>(std::uint8_t(word(location,3,1)))>>4);
  if(!(std::to_integer<unsigned>(definition[0])&4)) {
    auto associations=s.associations;
    associations.object_parent=[&](auto d){return u(m,body(m,d)+0x14);};
    audible=source_sound_audibility(owner,audible,associations);
  }
  const auto original=word(location,3,1);
  store(location,3,original^((original^audible)&15),1,0x18e044,observe);
  return true;
}
} // namespace halo2
