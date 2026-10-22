#include "helpers/gameplay_fixture.hpp"
#include "halo2/prop_sandbox.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/particle_material.hpp"
#include <iostream>
#include <set>
#include <fstream>
#include <cstring>
using namespace halo2;
namespace {
void check(bool b,const char* m){if(!b)throw FormatError(m);}
}
int main(int argc,char**argv){try{
  if(argc==5 && std::string(argv[4])=="--all-maps"){
    const auto original=OriginalData::load_xbe(argv[3]);
    const std::filesystem::path disc=argv[2],dlc=disc.parent_path().parent_path()/"map-pack/maps";
    unsigned maps=0,objects=0,faces=0,impact_owners=0;
    const auto verify=[&](const std::filesystem::path& path){
      try{
      Cache c(path,disc,original);auto content=c.load_content(0,true);
      auto rules=slayer_rules({"Slayer",0,{}});rules.time_ticks=0;
      for(const auto& [id,w]:content.weapons)if(w.smg)rules.weapon=id;
      MultiplayerMatch m(content,0,c.spawns(),rules);m.join("Prop import");
      std::array<Controls,16> neutral{};neutral[0].yaw=m.unit(0).yaw;m.step(neutral);
      const auto states=m.simulation().props().states();unsigned admitted=0;
      for(const auto& state:states){const auto& d=*content.prop_sandbox.at(state.placement);
        admitted+=d.uniform_impact_material && d.uniform_impact_region_absorption;}
      ++maps;objects+=states.size();faces+=m.simulation().props().triangles().size();impact_owners+=admitted;
      std::cout<<path.stem().string()<<" props="<<states.size()<<" admitted impact material owners="<<admitted<<'\n';
      }catch(const std::exception& e){throw FormatError(path.string()+": "+e.what());}
    };
    for(const auto name:{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
        "foundation","headlong","lockout","midship","waterworks","zanzibar"})verify(disc/(std::string(name)+".map"));
    for(const auto name:{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"})
      verify(dlc/(std::string(name)+".map"));
    check(maps==21,"All-map prop importer coverage changed");
    std::cout<<maps<<" real maps imported/constructed/stepped; "<<objects<<" retained props; "<<faces
        <<" posed prop triangles; "<<impact_owners<<" admitted impact material owners\n";return 0;
  }
  if(argc!=4 && !(argc==6 && std::string(argv[4])=="--death-fixture"))return 77;
  std::vector<std::byte> death_fixture;
  if(argc==6){
    std::ifstream file(argv[5],std::ios::binary);std::vector<char> raw((std::istreambuf_iterator<char>(file)),{});
    for(const auto c:raw)death_fixture.push_back(std::byte(static_cast<unsigned char>(c)));
    const Bytes bytes(death_fixture);
    check(death_fixture.size()==144 && std::memcmp(death_fixture.data(),"PDTH1101",8)==0 && bytes.u32(8)==2 && bytes.u32(12)==8,
        "Original death fixture schema changed");
  }
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  auto content=cache.load_content(0,true);
  auto rules=slayer_rules({"Slayer",0,{}});rules.time_ticks=0;
  for(const auto& [id,w]:content.weapons)if(w.smg)rules.weapon=id;
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Collision probe");
  const auto& sim=match.simulation();
  auto states=sim.props().states();std::vector<std::uint32_t> modules;
  for(const auto& s:states)if(s.definition==0xe6f80583u)modules.push_back(s.handle);
  check(modules.size()==14,"Live Lockout did not retain all fourteen power modules");
  const auto triangles=sim.props().triangles();
  check(sim.collision().size()==content.collision.size()+triangles.size(),"Live mover omitted prop collision triangles");
  SceneObjects scene(content,0,SourceSceneGameContext{2,2});PropSandbox props(content);props.synchronize(scene);
  const auto state=props.state(modules.front());const auto& d=*content.prop_sandbox.at(state.placement);
  check(d.vitality.health==30 && d.vitality.shield==0 && d.vitality.default_section==1,
      "Authored power-module vitality changed");
  check(d.sections.at(1).flags==10 && d.sections[1].responses.at(0).flags==0x100080u &&
      d.sections[1].responses[0].damage==0xe72b05b6u,"Authored destruction response changed");
  for(const auto datum:{0xe71c05a7u,0xe71f05aau,0xe72005abu,0xe72205adu,0xe72405afu,0xe72505b0u,0xe72905b4u}){
    const auto material=cache.load_particle_material(datum,15,0,0);
    check(material.vertex.classification==(datum==0xe72405afu?8u:7u),
        "Authored canister particle classification changed");
    unsigned textures=0;
    for(const auto& texture:material.textures)if(texture){
      ++textures;check(texture->format==11 && texture->layers==1 && texture->depth==0 && !texture->mips.empty(),
          "Actual canister BC texture did not use the existing literal decoder");
    }
    check(textures>0,"Actual canister material lost its texture binding");
  }
  unsigned casts=0,blocked=0;
  CollisionWorld terrain(content.collision);
  for(const auto& t:triangles){
    const auto center=(t.a+t.b+t.c)/3.f;
    const auto n=normalized(t.source_normal);if(std::abs(n.z)>.25f)continue;
    const auto hit=props.raycast(center+n, n*-2);
    if(!hit)continue;++casts;
    // Explicit native test placement/velocity. No original input provenance
    // or solver parity is claimed for this host backend exercise.
    Body body{center+n*.5f-Vec3{0,0,content.player.height*.5f},n*-2,
        content.player.height,content.player.radius};
    auto without=body;terrain.move(without,.4f);sim.collision().move(body,.4f);
    if(dot(body.position-without.position,n)>.1f)++blocked;
  }
  check(casts>0 && blocked>0,"Actual prop faces did not block the live collision backend");
  // Public Match controls -> source SMG dispatch -> actual projectile trace
  // -> retained prop health -> pending response -> blast -> Scene removal.
  // Only the test shooter's location is supplied by the fixture.
  Vec3 firing_origin{},firing_target{};bool vantage=false;
  for(const auto& t:triangles){
    if(t.source.instance!=int(state.placement))continue;
    const auto n=normalized(t.source_normal);if(std::abs(n.z)>.25f)continue;
    const auto center=(t.a+t.b+t.c)/3.f;
    for(const float distance:{.8f,1.2f,1.6f,2.f,2.4f}) {
      auto feet=center+n*distance-Vec3{0,0,content.player.camera_height};
      if(const auto floor=terrain.raycast(feet+Vec3{0,0,1.5f},{0,0,-1},4.f))
        feet.z=std::max(feet.z,floor->position.z+.02f);
      const auto& body=match.unit(0).body;
      if(sim.collision().overlaps(feet,body.height,body.radius))continue;
      const auto origin=feet+Vec3{0,0,content.player.camera_height};
      const auto delta=center-origin;
      const auto hit=props.raycast(origin,delta);
      if(!hit || hit->handle!=modules.front() || terrain.raycast(origin,normalized(delta),length(delta)-.01f))continue;
      firing_origin=origin;firing_target=center;vantage=true;break;
    }
    if(vantage)break;
  }
  check(vantage,"No actual source-authored module face available for Match firing test");
  const auto aim=normalized(firing_target-firing_origin);
  Spawn firing;firing.bsp=0;firing.position=firing_origin-Vec3{0,0,content.player.camera_height};
  firing.facing=std::atan2(aim.y,aim.x);

  test::place_player(match,0,firing);
  std::array<Controls,multiplayer_capacity> controls{};
  controls[0].yaw=firing.facing;controls[0].pitch=std::atan2(aim.z,std::hypot(aim.x,aim.y));
  for(unsigned i=0;i<30;++i){test::place_player(match,0,firing);match.step(controls);}
  controls[0].fire=true;
  bool live_damage=false,live_removed=false,live_blast=false;unsigned fired_ticks=0;
  PropSandboxEvent live_event;std::uint64_t live_event_tick{};
  for(;fired_ticks<240 && !live_removed;++fired_ticks){
    test::place_player(match,0,firing);
    match.step(controls);
    const auto current=match.simulation().props().states();
    for(const auto& s:current)if(s.handle==modules.front() && s.vitality.body<1)live_damage=true;
    for(const auto& e:match.take_prop_events())if(e.handle==modules.front()){
      live_removed=e.remove;live_blast=e.damage==0xe72b05b6u;
      live_event=e;live_event_tick=match.simulation().ticks();
      check(e.owner.player==match.player(0).source_datum,"Actual shot owner was lost before delayed blast");
    }
  }
  unsigned prop_contacts=0;bool retained_vitality_scale=false;
  for(const auto &hit:match.state().bullet_impacts)if(!hit.terrain){
    ++prop_contacts;
    check(d.uniform_impact_material && d.uniform_impact_region_absorption &&
        hit.material==d.vitality.body_material,
        "Prop impact lost D7B80's authored damage material override");
    check(hit.presentation && hit.presentation->scale_b>=0 && hit.presentation->scale_b<=1,
        "Prop impact lost its retained response and vitality scaleB");
    retained_vitality_scale|=hit.presentation->scale_b<1;
    check(hit.projectile!=null_datum && hit.weapon==rules.weapon,
        "Prop impact lost its projectile/weapon owner");
    check(length(hit.normal)>.99f && length(hit.normal)<1.01f,
        "Prop impact lost its actual posed plane normal");
  }
  check(prop_contacts>0,"Actual nearest prop contacts did not reach presentation");
  check(retained_vitality_scale,"Prop contacts retained a fixed scaleB instead of actual vitality");
  check(live_damage && live_removed && live_blast,"Normal Match SMG fire did not damage/explode/remove the power module");
  check(match.unit(0).vitality.shield<1 || match.unit(0).vitality.body<1,
      "Actual close-range module blast did not damage the native shooter");
  check(std::none_of(match.simulation().scene().objects().begin(),match.simulation().scene().objects().end(),
      [&](const auto& o){return o.handle==modules.front();}),"Live explosion retained its Scene object");
  const auto removals=match.state().prop_removals;
  check(!removals.empty() && removals[0].handle==modules.front() && removals[0].placement==state.placement &&
      removals[0].definition==0xe6f80583u,"Authoritative prop lifetime lost its authored identity");
  const auto& removal=removals[0];
  check(removal.effect==live_event.effect && removal.effect==0xe70a0595u && removal.event_tick==live_event_tick &&
      removal.event_tick<=match.state().tick &&
      removal.effect_position.x==live_event.effect_position.x && removal.effect_position.y==live_event.effect_position.y &&
      removal.effect_position.z==live_event.effect_position.z && removal.effect_forward.x==live_event.effect_forward.x &&
      removal.effect_forward.y==live_event.effect_forward.y && removal.effect_forward.z==live_event.effect_forward.z,
      "Prop tombstone reconstructed or lost the actual emitted visual request");
  LanHost host(match,0,0x50524f50);LanClient client("127.0.0.1:"+std::to_string(host.port()),0x50524f50,1);
  std::array<Controls,1> remote{};
  for(unsigned i=0;i<20 && !client.ready();++i){client.send(remote);host.receive(controls);host.broadcast();client.receive();}
  check(client.ready() && client.state().prop_removals==removals,"Late LAN join omitted authoritative prop removal bank");
  MultiplayerMatch replica(content,0,cache.spawns(),rules);
  replica.replicate_prop_removals(client.state().prop_removals);
  check(replica.simulation().props().triangles().size()==match.simulation().props().triangles().size(),
      "Client prop removal retained a ghost collider");
  replica.replicate_prop_removals(client.state().prop_removals);
  check(replica.state().prop_removals==removals,"Repeated prop removal was not idempotent");
  auto invalid=removals;invalid[0].definition^=1;bool rejected=false;
  try{replica.replicate_prop_removals(invalid);}catch(const FormatError&){rejected=true;}
  check(rejected && replica.state().prop_removals==removals,"Forged prop definition mutated client lifetime bank");
  auto weapon=content.weapons.at(rules.weapon);
  unsigned measured_row=0;
  const auto compare_original=[&]{
    if(death_fixture.empty())return;
    std::array<std::byte,16> actual{};
    const auto& source=props.state(modules.front()).sections;
    for(unsigned s=0;s<2;++s){
      const auto store=[&](unsigned off,std::uint16_t value){actual[s*8+off]=std::byte(value);actual[s*8+off+1]=std::byte(value>>8);};
      store(0,source[s].destroyed);actual[s*8+2]=std::byte(source[s].damage);store(4,source[s].pending);
    }
    check(measured_row<8 && std::equal(actual.begin(),actual.end(),death_fixture.begin()+16+16*measured_row),
        "Native death/countdown bytes differ from independently executed original");
    ++measured_row;
  };
  compare_original();
  const auto direct_amount=weapon.damage_amount?weapon.damage_amount->upper_minimum:weapon.damage_min;
  for(unsigned n=0;n<100 && !props.state(modules.front()).vitality.dead;++n)
    props.damage(modules.front(),weapon,direct_amount);
  check(props.state(modules.front()).vitality.dead,"Direct authored damage did not deplete prop health");
  check(props.state(modules.front()).sections[1].pending==48,"Original .2-second response did not retain six ticks");
  compare_original();
  for(unsigned n=0;n<5;++n){check(props.tick().empty(),"Power module dispatched before source countdown expired");compare_original();}
  const auto events=props.tick();check(events.size()==1 && events[0].remove &&
      events[0].damage==0xe72b05b6u && events[0].effect==0xe70a0595u,"Source response effect/damage/removal order missing");
  check(props.state(modules.front()).sections[1].destroyed==1,"Original response destroyed bit missing");
  compare_original();
  check(props.tick().empty(),"Destroyed response emitted twice");
  check(props.triangles().size()<triangles.size(),"Destroyed prop retained its collider");
  scene.destroy(modules.front());props.synchronize(scene);
  check(props.states().size()+1==states.size(),"Destroyed scene generation retained its state");
  const auto& area=*content.prop_area_damage.at(0xe72b05b6u);
  check(prop_area_fraction(area,.4f)==1 && prop_area_fraction(area,2.f)==0 &&
      prop_area_fraction(area,3.f)==0,"Source radius attenuation boundary changed");
  for(const auto datum:{0xe7ae0629u,0xe7c3063eu}){
    const auto grenade=load_prop_area_damage_definition(cache,datum);
    const bool frag=datum==0xe7ae0629u;
    check(grenade->source_58==(frag?4.5f:3.5f) && grenade->source_64==(frag?10.f:8.f) &&
        grenade->source_68==0 && prop_area_candidate_radius(*grenade)==grenade->source_58 &&
        prop_area_fraction(*grenade,grenade->radius_maximum)==0,
        "Authored grenade acceleration inputs suppressed or enlarged the health damage leaf");
  }
  unsigned queries=0;Vec3 corrected{.1f,.2f,.3f};
  check(!prop_area_occluded({0,0,0},{1,2,3},true,area.core_radius,
      [&](Vec3 start,Vec3 delta,std::uint32_t mask){
        ++queries;
        if(queries==1){check(mask==0x04a08c2du,"Core clip mask changed");return PropAreaCastResult{true,corrected};}
        check(queries==2 && mask==0x14a08c2du && start.x==corrected.x && start.y==corrected.y && start.z==corrected.z,
            "Source clip endpoint was not consumed by visibility cast");
        check(delta.x==1-corrected.x && delta.y==2-corrected.y && delta.z==3-corrected.z,"Core endpoint delta changed");
        return PropAreaCastResult{false,start+delta};
      }) && queries==2,"First visible core ray did not admit source area damage");
  queries=0;
  check(prop_area_occluded({0,0,0},{1,2,3},true,area.core_radius,
      [&](Vec3 start,Vec3 delta,std::uint32_t){++queries;return PropAreaCastResult{true,start+delta};}) && queries==8,
      "Fully occluded biped did not issue all four original core ray pairs");
  std::cout<<"14 actual modules; "<<triangles.size()<<" authored prop triangles; "<<casts
      <<" posed casts; "<<blocked<<" blocked native moves; Match fire/blast/removal in "<<fired_ticks
      <<" ticks; health/pending/marker/area visibility passed\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
