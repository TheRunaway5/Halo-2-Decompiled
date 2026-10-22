#include "halo2/unit_grenade.hpp"
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
std::uint8_t byte(const SourceGrenadeAction &s,std::size_t o) {return std::to_integer<std::uint8_t>(s[o]);}
std::int8_t signed_byte(const SourceGrenadeAction &s,std::size_t o) {return std::bit_cast<std::int8_t>(byte(s,o));}
template<class T> T load(const std::byte *p) {T value;std::memcpy(&value,p,sizeof value);return value;}
template<class T> void store(std::byte *p,T value) {std::memcpy(p,&value,sizeof value);}
GrenadeCallbackStep complete(bool value) {return {GrenadeCallback::complete,value,0};}
float sub(float a,float b) {volatile float value=a-b;return value;}
float mul(float a,float b) {volatile float value=a*b;return value;}
float divide(float a,float b) {volatile float value=a/b;return value;}
}
void initialize_source_grenade_action(SourceGrenadeAction &s) {
  s.fill(std::byte{});
  for(const auto o:{0x10,0x18,0x2c})store(s.data()+o,0xffffffffu);
  store(s.data()+0x14,std::uint16_t{0xffff});
}
void source_grenade_begin_channel_prefix(SourceGrenadeAction &s,std::uint8_t mode,std::uint8_t ticks) {
  s[0xc]=std::byte{1};s[0xe]=std::byte{};s[0xf]=std::byte{};
  s[0xd]=std::byte{mode};s[0xf]=std::byte{ticks};
}
GrenadeCallbackStep source_grenade_update(SourceGrenadeAction &s) {
  s[0xe]=std::byte{std::uint8_t(byte(s,0xe)+1)};
  switch(signed_byte(s,0xc)) {
    case 1:if(signed_byte(s,0xe)>=2)return {GrenadeCallback::prepare_projectile,std::nullopt,0};break;
    case 2:if(signed_byte(s,0xe)>=signed_byte(s,0xf))return {GrenadeCallback::release_projectile,std::nullopt,0};break;
    case 3:return {GrenadeCallback::animation_query,std::nullopt,0x0d000021};
    default:break;
  }
  return complete(true);
}
GrenadeCallbackStep source_grenade_animation_reply(SourceGrenadeAction &s,std::uint32_t id,std::optional<bool> active) {
  if(!active || (id!=0x0d000021 && id!=0x1000006c))return {GrenadeCallback::unavailable,std::nullopt,id};
  if(*active)return complete(true);
  if(id==0x0d000021)return {GrenadeCallback::animation_query,std::nullopt,0x1000006c};
  s[0xc]=std::byte{};return complete(false);
}
bool source_grenade_release_empty(SourceGrenadeAction &s) {
  if(load<std::uint32_t>(s.data()+0x10)!=0xffffffffu)return false;
  s[0xc]=std::byte{3};return true;
}
GrenadePreparationResult source_grenade_prepare_prefix(SourceGrenadeAction &s,const GrenadePreparation &c) {
  bool debited=false;
  if(c.current==-1) {s[0xc]=std::byte{3};return {GrenadePreparationStop::complete,false,0,0};}
  if(c.current<0 || std::size_t(c.current)>=c.counts.size())return {GrenadePreparationStop::unavailable,false,0,0};
  if(!c.definition_present)return {GrenadePreparationStop::unavailable,false,0,0};
  if(c.unit_object_12c==0xffffffffu && byte(s,0xd)==0) {
    auto &count=c.counts[std::size_t(c.current)];
    if(std::bit_cast<std::int8_t>(count)<=0) {s[0xc]=std::byte{3};return {GrenadePreparationStop::complete,false,0,0};}
    --count;debited=true;
  }
  if(!*c.definition_present) {s[0xc]=std::byte{3};return {GrenadePreparationStop::complete,debited,debited?0x400000u:0u,0};}
  return {GrenadePreparationStop::marker_request,debited,debited?0x400000u:0u,0x09000100};
}
void patch_source_grenade_creation_request(std::span<std::byte,0xc4> request,
    const std::array<float,3> &aim,const std::array<float,3> &point,const GrenadeDirectionConstants &c) {
  store(request.data()+0x18,load<std::uint32_t>(request.data()+0x18)|4u);
  std::memcpy(request.data()+0x28,aim.data(),12);
  std::array<float,3> up;
  const auto x=std::abs(aim[0]),y=std::abs(aim[1]),z=std::abs(aim[2]);
  if(x<=y && x<=z)up={0.f,aim[2],sub(0.f,aim[1])};
  else if(y<=z)up={sub(0.f,aim[2]),0.f,aim[0]};
  else up={aim[1],sub(0.f,aim[0]),0.f};
  const double xx=double(up[0])*double(up[0]);
  const double yy=double(up[1])*double(up[1]);
  const double zz=double(up[2])*double(up[2]);
  const double xy=xx+yy;
  const double length=std::sqrt(xy+zz);
  if(!(double(c.normalization_epsilon)>std::abs(length))) {
    const float reciprocal=divide(c.one,static_cast<float>(length));
    for(auto &v:up)v=mul(v,reciprocal);
  }
  std::memcpy(request.data()+0x34,up.data(),12);
  std::memcpy(request.data()+0x1c,point.data(),12);
}
} // namespace halo2
