#include "halo2/world_effects.hpp"
#include "halo2/material_effects.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace halo2;
namespace {void check(bool value,const char *why){if(!value)throw FormatError(why);}}
int main(int argc,char **argv)try{
  check(argc==4 || argc==5,"Supply MAP SHARED XBE [ORIGINAL_ROUTING]");
  struct Routing {std::uint32_t foot=0xffffffffu;std::vector<std::uint32_t> sounds;};
  std::map<std::pair<std::uint32_t,unsigned>,Routing> routing;
  if(argc==5){
    std::ifstream file(argv[4]);if(!file)return 77;std::string line;
    while(std::getline(file,line)){
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);std::uint32_t weapon;unsigned material,count;
      check(bool(row>>std::hex>>weapon>>std::dec>>material>>count),"Invalid original melee routing row");
      auto &r=routing[{weapon,material}];
      for(unsigned n=0;n<count;++n){char kind;std::uint32_t datum;check(bool(row>>kind>>std::hex>>datum),"Truncated original melee routing");if(kind=='F')r.foot=datum;else{check(kind=='S',"Unknown original melee request");r.sounds.push_back(datum);}}
    }
  }
  auto cache=std::make_shared<Cache>(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  auto content=cache->load_content(0,true);
  const auto sword=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return bool(p.second.inventory_flags&(1u<<24));});
  const auto smg=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return bool(p.second.smg);});
  check(sword!=content.weapons.end() && smg!=content.weapons.end(),"Actual sword/SMG absent");
  // Lockout's original sword action15 has twelve sound rows and no visual
  // rows. A missing particle draw is therefore expected for this contact
  // path; blade/character effects have different lifecycle owners.
  const auto sword_tag=cache->load_source_tag_metadata(sword->first);
  const Bytes sword_root(sword_tag.bytes_at(sword_tag.source_address(),0x5c));
  const auto foot_tag=cache->load_source_tag_metadata(sword_root.u32(0x58));
  const Bytes foot_root(foot_tag.bytes_at(foot_tag.source_address(),8));
  check(foot_root.u32(0)>15,"Original sword action15 missing");
  const Bytes sword_contact(cache->read_tag_bytes(foot_root.u32(4)+15*0x18,0x18));
  check(sword_contact.u32(8)==12 && sword_contact.u32(16)==0,"Original sword contact bank changed");
  const auto globals=cache->query_tag({.group=tag_groups::globals});
  const auto gt=cache->load_source_tag_metadata(globals->datum);const Bytes gr(gt.bytes_at(gt.source_address(),0x158));
  const auto materials=cache->read_tag_bytes(gr.u32(0x154),gr.u32(0x150)*0xb4);
  if(argc==5)check(routing.size()==2*gr.u32(0x150),"Incomplete original melee routing fixture");
  unsigned draws{},sound_requests{},cases{};
  for(const auto weapon:{sword->first,smg->first})for(unsigned material=0;material<gr.u32(0x150);++material){
    const auto tag=cache->load_source_tag_metadata(weapon);const Bytes w(tag.bytes_at(tag.source_address(),0x1fc));
    const MeleeImpact hit{1,1,0x12340000,weapon,w.u32(0x1c0),cache->spawns().front().position,{-1,0,0},{1,0,0},{0,0,0},std::int16_t(material)};
    Mesh am,bm;CollisionWorld collision(content.collision);
    NativeWorldEffects actual(*cache,am,collision,0x12345678),expected(*cache,bm,collision,0x12345678);
    actual.melee_impact(hit);std::vector<std::uint32_t> wanted;
    const auto original=routing.empty()?Routing{}:routing.at({weapon,material});
    if(!routing.empty())check(bool(w.u32(0x12c)&(1u<<24))==(original.foot!=null_datum),"Original melee-only admission differs");
    if(w.u32(0x12c)&(1u<<24)){
      const auto foot=routing.empty()?w.u32(0x58):original.foot;const auto position=hit.position+hit.normal*.01f;
      const WorldEffectFrame frame{position,hit.normal,hit.forward,hit.forward,{0,0,1},false,1,1};
      for(auto e:source_material_visual_effects(*cache,foot,-1,hit.material,15,0))if(e!=null_datum)expected.spawn(e,hit.attacker,hit.tick,frame);
      for(auto s:source_material_sound_effects(*cache,foot,-1,hit.material,15,0))if(s!=null_datum)wanted.push_back(s);
    }else{
      if(!routing.empty())wanted=original.sounds;
      else{
        const auto sound=Bytes(materials).u32(material*0xb4+0x64);if(sound!=null_datum)wanted.push_back(sound);
        if(hit.damage!=null_datum){const auto dt=cache->load_source_tag_metadata(hit.damage);const Bytes d(dt.bytes_at(dt.source_address(),0xb0));if(d.u32(0xac)!=null_datum)wanted.push_back(d.u32(0xac));}
      }
    }
    actual.advance(8,tick_seconds);expected.advance(8,tick_seconds);
    const auto a=actual.diagnostics(),b=expected.diagnostics();
    check(a.systems==b.systems && a.particles==b.particles,"Melee/sword selected the wrong authored systems");
    MuzzleEffectView view{};view.forward={1,0,0};view.up={0,0,1};view.position=hit.position-Vec3{3,0,0};view.width=640;view.height=480;view.orientation={1,0,0,0,1,0,0,0,1};
    const auto av=actual.draw(view,.5f,tick_seconds),bv=expected.draw(view,.5f,tick_seconds);
    check(av.size()==bv.size(),"Sword contact lost authored particle draws");
    for(unsigned n=0;n<av.size();++n)check(av[n].vertex.attributes==bv[n].vertex.attributes && av[n].vertex.constants==bv[n].vertex.constants,"Sword contact offset/scales/direction differ");
    std::vector<std::uint32_t> got;
    for(const auto &s:actual.take_sounds())if(s.kind==WorldEffectSoundKind::Material){check(s.owner==hit.attacker && s.tick==hit.tick,"Melee sound owner/tick lost");got.push_back(s.sound);}
    check(got==wanted,"Melee material/damage sound selection differs");
    draws+=av.size();sound_requests+=got.size();++cases;
  }
  // Native diagnostic geometry, not original collision/mover proof. It
  // supplies an actual nearby wall so the genuine sword action hits terrain.
  content.collision={{{-100,-100,10},{100,-100,10},{100,100,10},0,0},{{-100,-100,10},{100,100,10},{-100,100,10},0,0},
      {{.7f,-100,0},{.7f,100,0},{.7f,100,100},0,0},{{.7f,-100,0},{.7f,100,100},{.7f,-100,100},0,0}};
  unsigned native_contacts{};
  for(unsigned representation:{2u,3u}){
    SlayerRules rules;rules.weapon=smg->first;rules.score_limit=0;rules.time_ticks=0;
    Spawn spawn;spawn.player_type=0;spawn.position={0,0,10};std::array<Spawn,1> spawns{spawn};
    MultiplayerMatch match(content,0,spawns,rules);match.configure_weapon_pickups();match.configure_melee_animation(cache,*cache->original_data()->melee_constants);
    const std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(representation-2),0,0,0,0};
    const auto attacker=match.join("Sword effects",-1,native_selected_player_configuration("Sword effects",appearance,-1));
    std::array<Controls,16> input{};for(unsigned t=0;t<90;++t)match.step(input);
    const auto pickup=std::find_if(match.weapon_pickups().begin(),match.weapon_pickups().end(),[&](const auto &p){return p.definition==sword->first;});
    check(pickup!=match.weapon_pickups().end(),"Actual sword pickup unavailable");
    const_cast<Unit&>(match.unit(attacker)).body.position=pickup->position-Vec3{.2f,0,0};input[attacker].pickup=true;input[attacker].pickup_milliseconds=1000;match.step(input);input[attacker].pickup=false;
    check(match.unit(attacker).weapon.definition==sword->first,"Actual sword pickup failed");
    const_cast<Unit&>(match.unit(attacker)).body.position={0,0,10};for(unsigned t=0;t<30;++t)match.step(input);
    std::uint64_t previous{};
    for(bool primary:{false,true}){
      input[attacker].fire=primary;input[attacker].melee=!primary;match.step(input);input[attacker].fire=input[attacker].melee=false;
      check(match.unit(attacker).melee_action.active(),"Sword regular/special animation not admitted");
      const auto state=match.unit(attacker).melee_action;check(state.impact>=0,"Sword strike lost authored impact frame");
      // EB700 compares the retained progress before increment. A special
      // windup admits its strike at progress0 after that tick's update;
      // ordinary admission has already advanced to1 on the request tick.
      const auto impact_tick=match.state().tick+unsigned(int(state.impact)-int(state.progress)+1);
      bool delivered{};
      for(unsigned t=0;t<100;++t){
        for(const auto &hit:match.state().melee_impacts)if(hit.serial>previous){
          check(hit.tick==impact_tick,"Sword contact not delivered on authored impact counter");
          check(hit.weapon==sword->first && hit.attacker==match.unit(attacker).source_datum && hit.material==0,"Sword contact lost weapon/owner/material");
          previous=hit.serial;delivered=true;++native_contacts;
        }
        if(delivered && !match.unit(attacker).melee_action.active())break;
        match.step(input);
      }
      check(delivered,"Sword terrain strike applied no contact presentation");
    }
    LanHost host(match,0,0x4d454c45454658ull);LanClient peer(host.join_address(),0x4d454c45454658ull,1);
    std::array<Controls,16> received{};std::array<Controls,1> outgoing{};
    for(unsigned t=0;t<40 && !peer.ready();++t){peer.send(outgoing);host.receive(received);host.broadcast();peer.receive();}
    check(peer.ready(),"Melee LAN handshake failed");
    const auto &remote=peer.state().melee_impacts;const auto local=match.state().melee_impacts;
    check(remote.size()==local.size() && !local.empty(),"LAN lost melee contacts");
    const auto equal=[](Vec3 a,Vec3 b){return a.x==b.x && a.y==b.y && a.z==b.z;};
    for(unsigned n=0;n<local.size();++n)check(remote[n].serial==local[n].serial && remote[n].tick==local[n].tick && remote[n].attacker==local[n].attacker && remote[n].weapon==local[n].weapon && remote[n].damage==local[n].damage && remote[n].material==local[n].material && equal(remote[n].position,local[n].position) && equal(remote[n].normal,local[n].normal) && equal(remote[n].forward,local[n].forward) && equal(remote[n].attacker_position,local[n].attacker_position),"LAN changed melee contact presentation");
  }
  check(draws==0 && sound_requests && native_contacts==4,"Melee/sword coverage incomplete");
  std::cout<<"PASS melee/sword material cases="<<cases<<" particle draws="<<draws<<" sounds="<<sound_requests<<" native regular/special contacts="<<native_contacts<<"; both representations and LAN\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
