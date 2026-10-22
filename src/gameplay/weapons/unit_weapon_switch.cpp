#include "halo2/unit_weapon_switch.hpp"
#include <initializer_list>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
void require(bool available, const char *message) {
  if (!available) throw std::runtime_error(message);
}
std::uint32_t read(std::span<const std::byte> bytes, std::size_t at, unsigned count = 4) {
  require(at + count <= bytes.size(), "Truncated source weapon-switch input");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(std::to_integer<unsigned>(bytes[at+i])) << (i*8);
  return result;
}
std::span<std::byte> body(const UnitWeaponSwitchServices &s, std::uint32_t unit) {
  require(bool(s.unit_body), "Source switch unit owner is unavailable");
  auto b = s.unit_body(unit);
  require(b.size() >= 0x348, "Truncated source switch unit storage");
  return b;
}
std::int32_t selector(std::span<const std::byte> b, std::size_t at) {
  const auto byte = read(b,at,1);
  const auto result = byte < 128 ? std::int32_t(byte) : std::int32_t(byte)-256;
  require(result >= -1 && result < 4, "Unsupported source switch slot storage");
  return result;
}
std::uint32_t selected(std::span<const std::byte> b, std::size_t at) {
  const auto slot = selector(b,at);
  return slot == -1 ? none : read(b,0x218+std::size_t(slot)*4);
}
std::uint32_t leaf(const UnitWeaponSwitchServices &s, std::uint32_t entry,
                   std::initializer_list<std::uint32_t> args) {
  require(bool(s.invoke), "Source switch owning leaf is unavailable");
  require(args.size() <= 6, "Source switch leaf packet extent");
  WeaponSwitchLeafCall call{entry,{},unsigned(args.size())};
  unsigned i=0; for (auto x:args) call.arguments[i++]=x;
  return s.invoke(call);
}
bool truth(std::uint32_t value) { return (value&0xffu) != 0; }
void store(std::span<std::byte> b, std::size_t at, std::uint32_t value, unsigned bytes,
           std::uint32_t instruction, std::uint32_t datum, WeaponSwitchBuffer kind,
           const WeaponSwitchStoreObserver &observe) {
  const auto before=read(b,at,bytes);
  for (unsigned i=0;i<bytes;++i) b[at+i]=std::byte((value>>(i*8))&0xff);
  if (observe) observe({instruction,datum,kind,at,bytes,before,read(b,at,bytes)});
}
unsigned hand(bool primary) { return primary ? 0u : 1u; }
bool primary_type(std::uint32_t type) {
  require(type==8 || type==18,"Unsupported source weapon-switch action type");
  return type==8;
}
std::uint32_t put_away_action(bool primary) { return primary ? 0x08000025u : 0x0d000539u; }
void equip(std::uint32_t unit, bool primary, bool silent,
           const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  equip_requested_source_weapon(unit,primary,0x07000101u,silent,silent,s,o);
}
void clear_and_equip(std::uint32_t unit, bool primary, const UnitWeaponSwitchServices &s,
                     const WeaponSwitchStoreObserver &o) {
  const auto item=selected(body(s,unit),0x212+hand(primary));
  if (item!=none) {
    // Original query's return is ignored; preserve its owning call/order.
    leaf(s,0x115800,{unit,put_away_action(primary)});
    if (truth(leaf(s,0x1042e0,{item,1}))) {
      leaf(s,0xc9a40,{unit,0});
      leaf(s,0xefe30,{unit,primary?18u:19u});
      leaf(s,0x115650,{unit,primary?0u:3u,put_away_action(primary),0x3e88b439,0});
    }
    clear_current_source_weapon(unit,primary,s,o);
  }
  equip(unit,primary,false,s,o);
}
}

bool put_away_source_weapon(std::uint32_t unit, bool primary, bool immediate,
    bool silent, const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  const auto item=selected(body(s,unit),0x212+hand(primary));
  if (item==none) return true;
  const auto action=put_away_action(primary);
  if (truth(leaf(s,0x115800,{unit,action})) && !immediate) return false;
  if (silent && immediate) { clear_current_source_weapon(unit,primary,s,o); return true; }
  if (truth(leaf(s,0x1042e0,{item,unsigned(immediate)}))) {
    leaf(s,0xc9a40,{unit,0});
    if (!silent) leaf(s,0xefe30,{unit,primary?18u:19u});
    if (!truth(leaf(s,0x115650,{unit,primary?0u:3u,action,0x3e88b439,0}))) {
      clear_current_source_weapon(unit,primary,s,o); return true;
    }
    if (!immediate) return false;
    clear_current_source_weapon(unit,primary,s,o); return true;
  }
  clear_current_source_weapon(unit,primary,s,o); return true;
}

bool begin_source_weapon_switch(std::uint32_t unit, std::span<const std::byte> request,
    const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  require(request.size()==32,"Source switch request must contain32bytes");
  const bool primary=primary_type(read(request,0));
  const bool immediate=read(request,4,1)!=0, silent=read(request,5,1)!=0;
  auto b=body(s,unit); const unsigned h=hand(primary);
  if (!immediate) {
    if (read(b,0x216+h,1)==read(b,0x212+h,1)) return false;
    if (truth(leaf(s,0x112270,{unit}))) return false;
    if (read(body(s,unit),0x134)&(1u<<25)) return false;
    if (truth(leaf(s,0xe8610,{unit,22}))) return false;
  }
  b=body(s,unit);
  if (selector(b,0x216+h)!=-1) {
    if (selected(b,0x216+h)==none) return false;
    if (!truth(leaf(s,0xceaf0,{unit,none}))) return false;
  }
  for (auto type:{0u,10u,27u,22u}) leaf(s,0xe7ff0,{unit,type});
  if (put_away_source_weapon(unit,primary,immediate,silent,s,o)) {
    equip(unit,primary,silent,s,o); return true;
  }
  require(bool(s.action_component),"Source switch action component is unavailable");
  auto control=s.action_component(unit);
  require(control.size()>=0x44,"Truncated source switch action component");
  const unsigned type=primary?8u:18u; const auto at=4+(type>>5)*4;
  store(control,at,read(control,at)|(1u<<(type&31)),4,0xea160,unit,
        WeaponSwitchBuffer::ActionComponent,o);
  return true;
}

bool update_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  const bool primary=primary_type(type);const auto h=hand(primary);auto b=body(s,unit);
  if (read(b,0x216+h,1)==read(b,0x212+h,1)) {
    if (truth(leaf(s,0x115870,{unit,put_away_action(primary)}))) {
      const auto item=selected(body(s,unit),0x212+h);
      if (item!=none) { leaf(s,0x10a140,{item,0}); leaf(s,0x104500,{item}); }
    }
    return false;
  }
  if (truth(leaf(s,0x112270,{unit}))) return true;
  if (!put_away_source_weapon(unit,primary,false,false,s,o)) return true;
  equip(unit,primary,false,s,o); return false;
}
void finish_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  clear_and_equip(unit,primary_type(type),s,o);
}
void interrupt_source_weapon_switch(std::uint32_t unit, std::uint32_t type,
    const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  const bool primary=primary_type(type);
  require(bool(s.action_component),"Source switch action component is unavailable");
  const auto control=s.action_component(unit);
  if (read(control,4+(type>>5)*4)&(1u<<(type&31))) clear_and_equip(unit,primary,s,o);
}
void clear_current_source_weapon(std::uint32_t unit, bool primary,
    const UnitWeaponSwitchServices &s, const WeaponSwitchStoreObserver &o) {
  auto b=body(s,unit);const unsigned h=hand(primary);const auto item=selected(b,0x212+h);
  if (item==none) return;
  leaf(s,0x105750,{item});leaf(s,0x1042e0,{item,1});
  require(bool(s.weapon_body),"Source switch weapon storage is unavailable");
  auto w=s.weapon_body(item);
  store(w,0x16e,0,2,0xe9b06,item,WeaponSwitchBuffer::Weapon,o);
  leaf(s,0x108230,{item});leaf(s,0xfb3f0,{item});
  store(b,0x212+h,0xff,1,0xe9b21,unit,WeaponSwitchBuffer::Unit,o);
  leaf(s,0xefe30,{unit,primary?24u:25u});
}
void equip_requested_source_weapon(std::uint32_t unit, bool primary, std::uint32_t mode,
    bool immediate, bool silent, const UnitWeaponSwitchServices &s,
    const WeaponSwitchStoreObserver &o) {
  auto b=body(s,unit);const unsigned h=hand(primary);
  const bool other_item=selected(b,0x216+unsigned(primary))!=none;
  const bool special=((1u<<(read(b,0xaa,1)&31))>>1)&1;
  const auto slot=selector(b,0x216+h);
  auto fallback=[&] { leaf(s,0xe9c50,{unit,unsigned(primary)}); };
  if (slot==-1 || selector(b,0x212)==slot || selector(b,0x213)==slot) { fallback(); return; }
  const auto item=read(b,0x218+std::size_t(slot)*4);
  if (item==none) { fallback(); return; }
  require(bool(s.weapon_definition),"Source switch weapon definition is unavailable");
  const auto definition=s.weapon_definition(item);
  const auto weapon_class=other_item?0x0400054bu:read(definition,0x288);
  const auto weapon_name=read(definition,0x28c);
  const bool capability=truth(leaf(s,0x111490,{unit,mode,weapon_class,weapon_name}));
  if (capability || special) {
    leaf(s,0x111500,{unit,unsigned(primary),weapon_class,weapon_name});
  } else if (read(b,0x1fc,2)==0xffff) { fallback(); return; }
  leaf(s,0xd1d20,{unit,h});
  store(b,0x212+h,std::uint32_t(slot),1,0xe9ec1,unit,WeaponSwitchBuffer::Unit,o);
  require(bool(s.source_tick),"Source switch game tick producer is unavailable");
  store(b,0x228+std::size_t(slot)*4,s.source_tick(),4,0xe9ecb,unit,WeaponSwitchBuffer::Unit,o);
  const bool handled=truth(leaf(s,0xefee0,{unit,unsigned(primary)}));
  leaf(s,0x1040f0,{item,unsigned(silent),unsigned(immediate)});
  if (!silent) leaf(s,0xefe30,{unit,handled?22u:(primary?20u:21u)});
  leaf(s,0xc9a40,{unit,0});
  for (auto at:{0x1c8u,0x1ccu,0x1d0u})
    store(b,at,none,4,at==0x1c8?0xe9f2a:at==0x1cc?0xe9f30:0xe9f36,unit,WeaponSwitchBuffer::Unit,o);
  store(b,0x1e4,0,4,0xe9f42,unit,WeaponSwitchBuffer::Unit,o);
  store(b,0x1e8,0,4,0xe9f4a,unit,WeaponSwitchBuffer::Unit,o);
  store(b,0x1e0,0,2,0xe9f52,unit,WeaponSwitchBuffer::Unit,o);
  if (immediate || (other_item && primary)) return;
  const auto channel=primary?0u:3u;
  if (handled && truth(leaf(s,0x115650,{unit,channel,primary?0x1000006eu:0x15000538u,0x3e88b439,0}))) return;
  leaf(s,0x115650,{unit,channel,primary?0x05000024u:0x0a000537u,0x3e88b439,0});
}
} // namespace halo2
