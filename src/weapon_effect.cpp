#include "halo2/weapon_effect.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
void require(bool value,const char *why) {
  if(!value)throw std::runtime_error(why);
}
std::uint32_t read(std::span<const std::byte> b,std::size_t p,unsigned n=4) {
  require(p<=b.size() && n<=b.size()-p,"Truncated source weapon-effect storage");
  std::uint32_t v=0;
  for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[p+i]))<<(8*i);
  return v;
}
void write(std::span<std::byte> b,std::size_t p,std::uint32_t v,unsigned n=4) {
  require(p<=b.size() && n<=b.size()-p,"Truncated source effect output");
  for(unsigned i=0;i<n;++i)b[p+i]=std::byte((v>>(8*i))&0xffu);
}
WeaponEffectObject object(const WeaponEffectServices &s,std::uint32_t d) {
  require(bool(s.object),"Source weapon-effect object resolver unavailable");
  auto v=s.object(d);
  require(v.body.size()>=0x158,"Truncated source weapon-effect object");
  return v;
}
bool cluster_visible(std::span<const std::byte> mask,std::uint16_t cluster) {
  require(cluster<512,"Source effect cluster exceeds512-bit visibility owner");
  return (read(mask,std::size_t(cluster>>5)*4)&(1u<<(cluster&31)))!=0;
}
}
std::uint32_t source_weapon_effect_object(std::uint32_t weapon,
                                          const WeaponEffectServices &s) {
  const auto v=object(s,weapon);
  if ((v.header_flags&0x10) || (read(v.body,4,1)&1) || v.definition_model==none) {
    const auto parent=read(v.body,0x14);
    if(parent!=none)return parent;
  }
  return weapon;
}
AttachedEffectRequest source_attached_effect_request(std::uint32_t effect,
    std::uint32_t selected,std::uint32_t a,std::uint32_t b,const WeaponEffectServices &s) {
  // 1781F0 clears24 words, then writes its defaults before BACC0/177DA0.
  AttachedEffectRequest r;
  write(r.packet,0,5);write(r.packet,4,effect);write(r.packet,8,selected);
  write(r.packet,0xc,none);write(r.packet,0x10,none);
  write(r.packet,0x14,0xffff,2);write(r.packet,0x18,0xffff,2);
  write(r.packet,0x1c,s.directions_address);write(r.packet,0x20,2);
  write(r.packet,0x40,a);write(r.packet,0x44,b);
  write(r.packet,0x54,0xff808080);write(r.packet,0x58,0xff808080);
  require(bool(s.center),"Source attached-effect BACC0/current-matrix owner unavailable");
  const auto center=s.center(selected);
  for(unsigned i=0;i<12;++i) {
    r.directions[i]=r.directions[0x1c+i]=center[i];
    r.directions[0xc+i]=s.gravity[i];r.directions[0x28+i]=s.up[i];
  }
  write(r.directions,0x18,0x070000c0);write(r.directions,0x34,0x020000ca);
  return r;
}
std::uint32_t dispatch_source_weapon_effect(const WeaponEffectInput &i,
                                            const WeaponEffectServices &s) {
  if(i.effect==none)return none;
  const auto selected=source_weapon_effect_object(i.weapon,s);
  const auto v=object(s,i.weapon); // original107B50 resolves its original item again.
  const auto owner=(read(v.body,0x12c,1)&1)?read(v.body,0x154):none;
  require(bool(s.tag_group),"Source weapon-effect tag group unavailable");
  const auto group=s.tag_group(i.effect);
  if(group==0x65666665) {
    if(i.previous_handle!=none) {
      require(bool(s.refresh),"Source retained effect-pool refresh owner unavailable");
      if(s.refresh(i.previous_handle,i.effect))return i.previous_handle;
    }
    auto request=source_attached_effect_request(i.effect,selected,i.a_bits,i.b_bits,s);
    require(bool(s.create),"Source effect allocation/attachment/event owner unavailable");
    return s.create(request);
  }
  if(group==0x736e6421) {
    require(bool(s.sound),"Source weapon effect sound owner unavailable");
    s.sound(i.effect,owner,i.a_bits);
  }
  return none;
}
bool update_source_effect_visibility(std::span<std::byte> e,std::uint32_t flags,
                                    const EffectVisibilityServices &s) {
  require(e.size()==0x190,"Source effect must retain400literalbytes");
  const auto cluster=std::uint16_t(read(e,0x20,2));
  bool suppressed=true;
  if(cluster!=0xffff) {
    if ((s.special_view && read(e,0x48)!=none) || s.camera_override) suppressed=false;
    else suppressed=!cluster_visible((flags&4)?s.all_player_pvs:s.local_pvs,cluster);
  }
  const auto byte=read(e,2,1);
  if(suppressed==bool(byte&8))return false;
  write(e,2,suppressed?(byte|8):(byte&0xf7),1);
  if(!suppressed)return false;
  if(!(read(e,2,2)&0x202)) {
    require(bool(s.retire),"Source invisible effect retirement owner unavailable");
    return s.retire();
  }
  return false;
}
} // namespace halo2
