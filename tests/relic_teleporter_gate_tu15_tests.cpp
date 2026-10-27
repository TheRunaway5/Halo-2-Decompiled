#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_teleporter.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
nlohmann::json vector(Vec3 v){return {v.x,v.y,v.z};}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-relic-teleporter-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create owned Relic test directory");path=name;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=2)return 2;
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Cache cache(argv[1]);check(cache.is_mcc() && cache.name()=="dune","Relic acceptance requires prepared MCC dune.map");
  const auto content=cache.load_content(0,true);
  SourceTeleporterBank pairs;source_teleporter_initialize(pairs,content.netgame_flags);
  check(pairs.count==2,"Relic must have two authored teleporter directions");
  std::size_t placement=content.objects.size();
  for(std::size_t i=0;i<content.devices.size();++i)if(content.devices[i] && content.devices[i]->control) {
    check(placement==content.objects.size(),"Relic must have one authored control");placement=i;
  }
  check(placement<content.objects.size(),"Relic switch definition missing");
  const auto &definition=*content.devices[placement];const auto &object=content.objects[placement];
  const auto basis=authored_basis(object.rotation);
  SourceDeviceWorldOwner initial;initial.root.position=object.position;initial.root.scale=object.scale;
  initial.root.basis={basis.forward,basis.left,basis.up};
  initial.motion.position=definition.placement.initial_position;initial.motion.power=definition.placement.initial_power;
  const auto nodes=source_control_world_nodes(definition,initial);
  check(!nodes.empty(),"Relic switch has no actual targeting hierarchy");
  const auto button=source_shadow_sphere(definition.control_bounds,nodes[0],initial.root.scale).center;
  check(!cache.spawns().empty(),"Relic has no ordinary spawn input definition");
  auto start=cache.spawns().front();start.position=button+nodes[0].basis[0]*.6f;
  const CollisionWorld floor_world(content.collision);
  const auto floor=floor_world.raycast(start.position+Vec3{0,0,.35f},{0,0,-1},3);
  check(floor && floor->normal.z>.9f,"No actual floor at the declared Relic switch approach");
  start.position.z=floor->position.z+.01f;start.facing=std::atan2(button.y-start.position.y,button.x-start.position.x);
  Files files;MultiplayerObjectRuntime world(cache,0,files.path);
  unsigned blocked_stores{};
  world.observe_physics_stores([&](auto pc,const auto &store) {
    if(pc==0x1a67fcu && store.written.size()==1 && (std::to_integer<unsigned>(store.written[0])&4u))
      ++blocked_stores;
  });
  const UiVariant variant{"Slayer",0,{{89,5}}};auto rules=slayer_rules(variant);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  world.configure_session(source_game_variant(variant),1,30,rules.random_seed);
  MultiplayerMatch match(content,0,std::span(&start,1),rules,{}, {},&world);
  match.configure_weapon_pickups();
  const auto player=match.join("Relic ordinary switch approach");
  const auto first_person=cache.load_first_person_binding(rules.weapon,2);
  match.configure_replicated_first_person(first_person);
  const std::array locals{player};match.configure_local_views(locals,first_person);
  std::uint32_t control=null_datum;
  for(const auto &[handle,owner]:match.simulation().devices().owners())if(owner.placement==placement)control=handle;
  check(control!=null_datum,"Relic source control generation missing");
  const auto group=definition.placement.position_group;
  check(group>=0 && match.simulation().devices().groups()[unsigned(group)].value==0,
      "Relic switch did not begin in its authored closed shared group");
  std::array<Controls,multiplayer_capacity> controls{};
  auto &input=controls[player];input.yaw=start.facing;
  const auto aim=[&](Vec3 target) {
    const auto &unit=match.unit(player);
    const auto delta=target-ordinary_unit_eye(unit.body.position,unit.definition.camera_height,
        unit.definition.crouch_camera_height,unit.crouch);
    input.yaw+=std::remainder(std::atan2(delta.y,delta.x)-unit.yaw,6.28318530718f);
    input.pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-unit.pitch;
  };
  bool prompt{};unsigned changed{},partial{};
  for(unsigned tick=0;tick<180 && !changed;++tick) {
    aim(button);const auto target=match.action_prompt(player).target;
    prompt|=target.action==3 && target.object==control;
    input.pickup=true;input.pickup_milliseconds=std::uint16_t((tick+1)*1000/30);
    match.step(controls);
    if(match.simulation().devices().groups()[unsigned(group)].value==1)changed=match.state().tick;
  }
  std::cout<<nlohmann::json{{"declared_birth",vector(start.position)},{"switch_center",vector(button)},
      {"control",control},{"prompt_seen",prompt},{"changed_tick",changed},
      {"actual_position",vector(match.unit(player).body.position)}}.dump()<<'\n';
  check(prompt && changed,"Ordinary held use did not activate the actual Relic switch");
  input.pickup=false;input.pickup_milliseconds=0;
  bool opened{};
  for(unsigned tick=0;tick<240 && !opened;++tick) {
    match.step(controls);opened=true;
    for(const auto &[handle,owner]:match.simulation().devices().owners()) {
      const auto &d=*content.devices[owner.placement];
      if(!d.machine || d.placement.position_group!=group)continue;
      partial+=owner.motion.position>0 && owner.motion.position<1;
      opened&=owner.motion.position==1;
    }
  }
  check(opened && partial,"Relic shared gate/shrouds did not advance through original device motion");
  // Navigate from the switch to its nearby entrance using ordinary input.
  // On the outside, walk clear and return to exercise the reverse direction.
  unsigned nearby=0;float best=1e30f;
  for(unsigned i=0;i<unsigned(pairs.count);++i) {
    const auto p=content.netgame_flags[unsigned(pairs.pairs[i].entrance)].position;
    const auto d=p-start.position;const auto squared=dot(d,d);
    if(squared<best){best=squared;nearby=i;}
  }
  const auto walk=[&](Vec3 target,unsigned serial,unsigned limit) {
    input.forward=1;
    for(unsigned tick=0;tick<limit && match.player(player).source_teleport_serial<serial;++tick) {
      const auto &u=match.unit(player);const auto delta=target-u.body.position;
      input.yaw+=std::remainder(std::atan2(delta.y,delta.x)-u.yaw,6.28318530718f);
      input.pitch-=u.pitch;match.step(controls);
    }
    input.forward=0;
  };
  walk(content.netgame_flags[unsigned(pairs.pairs[nearby].entrance)].position,1,240);
  std::cout<<nlohmann::json{{"nearby_pair",nearby},{"first_serial",match.player(player).source_teleport_serial},
      {"first_position",vector(match.unit(player).body.position)}}.dump()<<'\n';
  check(match.player(player).source_teleport_serial==1,"Opened Relic shroud still blocked the normal entrance approach");
  const auto &outside=content.netgame_flags[unsigned(pairs.pairs[1-nearby].entrance)];
  const auto away=outside.position-direction(outside.facing)*1.25f;
  input.forward=1;
  for(unsigned tick=0;tick<90;++tick) {
    const auto &u=match.unit(player);const auto delta=away-u.body.position;
    if(delta.x*delta.x+delta.y*delta.y<.04f)break;
    input.yaw+=std::remainder(std::atan2(delta.y,delta.x)-u.yaw,6.28318530718f);
    match.step(controls);
  }
  input.forward=0;
  const auto delta=match.unit(player).body.position-outside.position;
  check(dot(delta,delta)>.25f,"Reverse Relic route did not leave the original entrance radius");
  walk(outside.position,2,180);
  std::cout<<nlohmann::json{{"reverse_serial",match.player(player).source_teleport_serial},
      {"final_position",vector(match.unit(player).body.position)},{"partial_device_ticks",partial}}.dump()<<'\n';
  check(match.player(player).source_teleport_serial==2,"Opened reverse Relic teleporter did not admit ordinary walking");
  walk(content.netgame_flags[unsigned(pairs.pairs[nearby].entrance)].position,3,180);
  check(match.player(player).source_teleport_serial==3,"Relic local player could not return to block the outside exit");
  LanHost host(match,0,0x1569u);LanClient client(host.join_address(),0x1569u,1);
  std::array<Controls,1> remote{};
  for(unsigned tick=0;tick<20 && !client.ready();++tick) {
    client.send(remote);host.receive(controls);host.broadcast();client.receive();
  }
  check(client.ready() && client.error().empty(),"Relic actual UDP client did not join");
  const auto remote_player=client.slots().front();remote[0].yaw=client.state().players[remote_player].yaw;
  const auto network_step=[&](bool broadcast) {
    client.send(remote);host.receive(controls);match.step(controls);
    if(broadcast){host.broadcast();client.receive();check(client.error().empty(),"Relic actual UDP snapshot failed");}
  };
  const auto destination=content.netgame_flags[unsigned(pairs.pairs[nearby].entrance)].position;
  const auto initial_blocked=blocked_stores;
  for(unsigned tick=0;tick<180 && blocked_stores==initial_blocked;++tick) {
    const auto &state=client.state().players[remote_player];const auto delta=destination-state.position;
    remote[0].yaw+=std::remainder(std::atan2(delta.y,delta.x)-state.yaw,6.28318530718f);
    remote[0].forward=1;network_step(true);
  }
  remote[0].forward=0;
  check(blocked_stores>initial_blocked && match.player(remote_player).source_teleport_serial==0,
      "Opened Relic exit did not block the approaching remote participant through original contact queries");
  for(unsigned tick=0;tick<6;++tick)network_step(true);
  check(match.player(remote_player).source_teleport_serial==0,"Occupied Relic exit admitted the waiting remote player");
  // Move the actual outside blocker away, retaining the remote player's
  // ordinary waiting body. Omit snapshots until the completed traversal.
  const auto &blocker=match.unit(player);
  const auto away_delta=away-blocker.body.position;
  input.yaw+=std::remainder(std::atan2(away_delta.y,away_delta.x)-blocker.yaw,6.28318530718f);
  input.forward=1;
  for(unsigned tick=0;tick<90 && !match.player(remote_player).source_teleport_serial;++tick)network_step(false);
  input.forward=0;host.broadcast();client.receive();
  std::cout<<nlohmann::json{{"blocked_source_stores",blocked_stores-initial_blocked},
      {"remote_serial",match.player(remote_player).source_teleport_serial},
      {"remote_position",vector(match.unit(remote_player).body.position)}}.dump()<<'\n';
  check(match.player(remote_player).source_teleport_serial==1 && client.error().empty() &&
      client.state().players[remote_player].source_teleport_serial==1 &&
      client.state().players[remote_player].source_teleport_sound==match.player(remote_player).source_teleport_sound,
      "Relic remote traversal did not survive actual blocker movement and omitted UDP snapshots");
  std::cout<<"PASS original Relic held switch, shared gate/shroud motion, both walking directions, occupied exit and UDP retry\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
