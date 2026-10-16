// Exercise the actual private packet codec without adding a public gameplay API.
#include "../src/multiplayer_network.cpp"
#include <iostream>
#include <limits>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply original maps");
  const std::filesystem::path maps=argv[1];Cache cache(maps/"lockout.map");
  const auto content=cache.load_content(0,true);auto rules=slayer_rules({"Slayer",0,{}});
  rules.weapon=cache.slayer_starting_weapon(5);
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("HUD A");match.join("HUD B");
  const std::array<unsigned,2> locals{0,1};
  unsigned cases{};
  for(unsigned flags=0;flags<32;++flags)for(unsigned variant=0;variant<4;++variant) {
    auto state=match.state();
    for(unsigned id=0;id<2;++id) {
      auto &p=state.players[id];p.source_hud_target_flags=id?31u-flags:flags;
      p.source_hud_autoaimed=id?bool(variant&1u):!bool(variant&1u);
      p.weapon_trigger={0x42,7,5,128,bool(variant&1u),{}};
      if(variant&2u)p.weapon_trigger.target_position=std::array{10.25f,-20.5f,30.75f};
      p.secondary_weapon.datum=0x12340010u+id;p.secondary_weapon.definition=p.weapon;
      p.secondary_weapon.trigger={0x22,3,6,64,!bool(variant&1u),{}};
      p.secondary_weapon.magazine_mode=std::int16_t(variant);
      if(!(variant&2u))p.secondary_weapon.trigger.target_position=std::array{-10.5f,20.25f,-30.75f};
      auto &vehicle=p.source_vehicle_hud;
      vehicle.ownership.seated=bool(variant);
      vehicle.ownership.handheld_visible=!(variant&1u);
      if(variant)vehicle.ownership.parent_hud=0xa3210900u+id;
      if(variant) {
        p.source_parent=0x54320030u+id;p.source_seat=1;
        p.source_vehicle_camera=NativeDeathCameraView{{1,2,3},{1,0,0},{0,0,1},1.2f,
            (id?-1.f:1.f)*.125f*float(variant+1)};
      }
      if(variant&2u) {
        vehicle.ownership.vehicle_weapon=0x43210020u+id;vehicle.definition=p.weapon;
        auto &w=vehicle.weapon;w.datum=vehicle.ownership.vehicle_weapon;w.present=1;
        w.loaded=-3;w.reserve=321;w.capacity=12;w.maximum_reserve=1000;
        w.heat=.75f;w.age=.125f;w.charge=.25f;w.lock_progress=.5f;
        w.target_position={10.5f,-20.25f,30.75f};
        w.overheated=bool(flags&1u);w.magazine_working=bool(flags&2u);w.input_held=bool(flags&4u);
        w.charging=bool(flags&8u);w.target=bool(flags&16u);w.locking=variant==2;w.locked=variant==3;
      }
    }
    Packet bytes;encode_state(bytes,state,locals);Reader reader{bytes};
    const auto decoded=decode_state(reader,locals);reader.end();
    for(unsigned id=0;id<2;++id)if(decoded.players[id].source_hud_target_flags!=state.players[id].source_hud_target_flags ||
        decoded.players[id].source_hud_autoaimed!=state.players[id].source_hud_autoaimed ||
        decoded.players[id].weapon_trigger!=state.players[id].weapon_trigger ||
        decoded.players[id].secondary_weapon.trigger!=state.players[id].secondary_weapon.trigger ||
        decoded.players[id].secondary_weapon.magazine_mode!=state.players[id].secondary_weapon.magazine_mode ||
        decoded.players[id].source_vehicle_hud.ownership.seated!=state.players[id].source_vehicle_hud.ownership.seated ||
        decoded.players[id].source_vehicle_hud.ownership.handheld_visible!=state.players[id].source_vehicle_hud.ownership.handheld_visible ||
        decoded.players[id].source_vehicle_hud.ownership.parent_hud!=state.players[id].source_vehicle_hud.ownership.parent_hud ||
        decoded.players[id].source_vehicle_hud.definition!=state.players[id].source_vehicle_hud.definition ||
        decoded.players[id].source_vehicle_camera.has_value()!=state.players[id].source_vehicle_camera.has_value() ||
        (state.players[id].source_vehicle_camera && decoded.players[id].source_vehicle_camera->aim_displacement!=
            state.players[id].source_vehicle_camera->aim_displacement) ||
        std::memcmp(&decoded.players[id].source_vehicle_hud.weapon,&state.players[id].source_vehicle_hud.weapon,sizeof(SourceWeaponHudState)))
      throw FormatError("Native snapshot lost independent per-hand HUD targeting");
    ++cases;
  }
  {
    auto state=match.state();auto &p=state.players[0];p.source_parent=0x54320030u;p.source_seat=1;
    p.source_vehicle_camera=NativeDeathCameraView{{1,2,3},{1,0,0},{0,0,1},1.2f,
        std::numeric_limits<float>::quiet_NaN()};
    bool rejected=false;try{Packet bytes;encode_state(bytes,state,locals);}catch(const FormatError &){rejected=true;}
    if(!rejected)throw FormatError("Native camera packet accepted a non-finite HUD aim displacement");
  }
  for(const auto flags:{4u,0x80u}) {
    Packet bytes;VehicleWeaponHudStatus status;
    if(flags==4u) {encode_vehicle_hud(bytes,status);bytes[0]=std::byte(flags);}
    else {
      status.ownership={true,false,0x12340000};status.definition=0x10000;
      status.weapon.datum=status.ownership.vehicle_weapon;status.weapon.present=1;
      encode_vehicle_hud(bytes,status);bytes.back()=std::byte(flags);
    }
    bool rejected=false;try {Reader reader{bytes};(void)decode_vehicle_hud(reader);}catch(const FormatError &){rejected=true;}
    if(!rejected)throw FormatError("Native packet accepted invalid vehicle HUD flags");
  }
  std::cout<<cases<<" actual packet-codec round trips preserved independent hand/vehicle HUD state and camera aim; malformed flags/non-finite aim rejected\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
