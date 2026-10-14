// Exercise the actual private packet codec without adding a public gameplay API.
#include "../src/multiplayer_network.cpp"
#include <iostream>
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
      if(!(variant&2u))p.secondary_weapon.trigger.target_position=std::array{-10.5f,20.25f,-30.75f};
    }
    Packet bytes;encode_state(bytes,state,locals);Reader reader{bytes};
    const auto decoded=decode_state(reader,locals);reader.end();
    for(unsigned id=0;id<2;++id)if(decoded.players[id].source_hud_target_flags!=state.players[id].source_hud_target_flags ||
        decoded.players[id].source_hud_autoaimed!=state.players[id].source_hud_autoaimed ||
        decoded.players[id].weapon_trigger!=state.players[id].weapon_trigger ||
        decoded.players[id].secondary_weapon.trigger!=state.players[id].secondary_weapon.trigger)
      throw FormatError("Native snapshot lost independent per-hand HUD targeting");
    ++cases;
  }
  std::cout<<cases<<" actual packet-codec round trips preserved both players' aim flags, lock state and per-hand marker positions\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
