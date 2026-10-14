#include "halo2/multiplayer_world.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/native_player_profile.hpp"
#include <bit>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <set>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc<5 || argc>7)throw FormatError("Supply MAP SHARED TU15 OUTPUT_DIRECTORY [human|elite] [STARTING_WEAPON]");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  if(argc>=6 && std::string_view(argv[5])!="human" && std::string_view(argv[5])!="elite")
    throw FormatError("Invalid player species");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  options.starting_weapon=argc==7?std::stoul(argv[6]):5;options.session_random_seed=0x12345678;options.defer_initial_player_spawn=true;
  const bool elite=argc>=6 && std::string_view(argv[5])=="elite";
  std::array<std::uint8_t,9> appearance{};appearance[4]=elite?1:0;
  options.selected_profiles={native_selected_player_configuration("Source animation",appearance,-1)};
  const unsigned representation=elite?3:2;
  if(multiplayer_representation(options.selected_profiles[0])!=representation)
    throw FormatError("Native profile did not select the requested source representation");
  Cache placement_cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto content=placement_cache.load_content(0,true);CollisionWorld geometry(content.collision);
  bool placed=false;Spawn spawn{};
  for(const auto &candidate:placement_cache.spawns()) {
    for(unsigned direction=0;direction<4 && !placed;++direction) {
      const float heading=float(direction)*1.5707963267948966f;const Vec3 d{std::cos(heading),std::sin(heading),0};
      const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
      if(geometry.raycast(eye,d,3.f) || geometry.raycast(eye,d*-1.f,3.f))continue;
      bool floor=true;
      for(float distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
        const auto hit=geometry.raycast(candidate.position+d*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
        floor&=hit && hit->normal.z>.9f;
      }
      if(floor){spawn=candidate;spawn.facing=heading;placed=true;}
    }
    if(placed)break;
  }
  if(!placed)throw FormatError("No clear authored locomotion corridor/floor");
  options.native_spawn_inputs={spawn};
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
  options.pre_player_update_owner=[&](MultiplayerMatch &match) {
    for(const auto handle:players){const auto *p=match.player_from_datum(handle);
      if(p && !(p->source_flags&1u))match.apply_source_player_activation(handle,p->source_flags,p->source_flags|1u);}
  };
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  if(!world.source_objects || !world.source_objects->owns_physics())throw FormatError("Actual source physics owner missing");
  std::array<Controls,multiplayer_capacity> controls{};float yaw=spawn.facing;
  std::optional<RenderModel> model;std::vector<unsigned> legs,heads;
  std::array<std::set<std::uint64_t>,7> leg_poses,head_poses;
  float maximum_crouch{},maximum_speed{};unsigned airborne{},landed{};bool was_airborne{};
  const auto hash=[&](std::span<const Transform> nodes,std::span<const unsigned> selected) {
    std::uint64_t h=14695981039346656037ull;const auto root=inverse_transform(nodes.front());
    for(const auto index:selected){const auto t=compose_transform(root,nodes[index]);
      for(const auto v:{t.basis[0],t.basis[1],t.basis[2],t.position})for(float f:{v.x,v.y,v.z}) {
        if(!std::isfinite(f))throw FormatError("Actual source locomotion contains a nonfinite node");
        h=(h^std::bit_cast<std::uint32_t>(f))*1099511628211ull;
      }}return h;
  };
  for(unsigned tick=0;tick<180;++tick) {
    unsigned stage=tick<20?0:tick<40?1:tick<60?2:tick<80?3:tick<100?4:tick<120?5:6;
    auto &input=controls[0];input.yaw=yaw+(tick>=80?1.5f:0.f);
    input.pitch=stage==4?.4f:stage==5?-.4f:0.f;
    input.crouch=stage==1 || stage==2;input.forward=stage==2?.5f:stage==3?-.5f:0.f;
    input.jump=tick==120;match.step(controls);
    const auto &unit=match.unit(0);if(tick==0)yaw=unit.yaw;
    if(!model) {
      const auto definition=world.source_objects->object_definition(unit.source_datum);
      if(definition!=content.player_representations[representation].biped.datum)
        throw FormatError("Actual source biped differs from the selected player species");
      if(unit.weapon.definition!=world.cache->slayer_starting_weapon(options.starting_weapon))
        throw FormatError("Actual held source weapon differs from the requested starting weapon");
      model=world.cache->load_model(definition,false);
      std::cout<<"Source representation="<<representation<<" biped="<<std::hex<<definition<<std::dec
        <<" nodes="<<model->bones.size()<<" weapon="<<std::hex<<unit.weapon.definition<<std::dec<<'\n';
      for(unsigned n=0;n<model->bones.size();++n) {
        const auto name=world.cache->string_id(model->bones[n].name);
        if(name.ends_with("thigh") || name.ends_with("calf") || name.ends_with("foot"))legs.push_back(n);
        if(name.ends_with("head") || name.ends_with("spine"))heads.push_back(n);
      }
      if(legs.size()<6 || heads.empty())throw FormatError("Actual source biped leg/head mapping missing");
    }
    const auto nodes=world.source_objects->world_nodes(unit.source_datum);
    if(nodes.size()!=model->bones.size())throw FormatError("Actual source WORLD node count differs from rendered biped");
    const auto motion=world.source_objects->biped_motion(unit.source_datum);
    const auto published=match.state();
    for(const auto owner:{unit.source_datum,unit.weapon.source_datum}) {
      const auto actual=world.source_objects->world_nodes(owner);
      const auto pose=std::find_if(published.source_body_poses.begin(),published.source_body_poses.end(),
        [&](const auto &p){return p.object==owner;});
      if(pose==published.source_body_poses.end() || pose->nodes.size()!=actual.size() ||
         !std::equal(actual.begin(),actual.end(),pose->nodes.begin(),[](const auto &a,const auto &b){
           return a.scale==b.scale && std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis))==0 &&
             std::memcmp(&a.position,&b.position,sizeof(a.position))==0;}))
        throw FormatError("Published body/held-weapon WORLD nodes differ from the actual source owner");
    }
    const auto source_body=world.source_objects->source_object_body(unit.source_datum);
    const bool supported=(Bytes(world.source_objects->memory().view(source_body+0x348u,4)).u32(0)&1u)==0;
    if(unit.body.grounded!=supported || match.state().players[0].native_grounded!=supported)
      throw FormatError("Actual source support was not published to native animation consumers");
    if(tick%20==19 || tick==120)std::cout<<"Source motion tick="<<tick<<" position="<<motion.position.x<<','<<motion.position.y<<','<<motion.position.z
      <<" velocity="<<motion.velocity.x<<','<<motion.velocity.y<<','<<motion.velocity.z<<" physical="<<unsigned(motion.physical_state)
      <<" grounded="<<unit.body.grounded<<" source_supported="<<supported<<'\n';
    maximum_crouch=std::max(maximum_crouch,motion.crouch);
    maximum_speed=std::max(maximum_speed,length(motion.velocity));
    if(tick>=120) {
      if(!unit.body.grounded){++airborne;was_airborne=true;}
      else if(was_airborne)++landed;
    }
    if(tick%20>=5){leg_poses[stage].insert(hash(nodes,legs));head_poses[stage].insert(hash(nodes,heads));}
  }
  for(unsigned stage=0;stage<7;++stage)std::cout<<"Source pose stage="<<stage<<" legs="<<leg_poses[stage].size()<<" head="<<head_poses[stage].size()<<'\n';
  std::cout<<"Source crouch="<<maximum_crouch<<" speed="<<maximum_speed<<" airborne="<<airborne<<" landed="<<landed<<'\n';
  if(maximum_crouch<.5f || maximum_speed<.1f || leg_poses[2].size()<3 || leg_poses[3].size()<3)
    throw FormatError("Live source crouch/walk did not animate its real leg nodes");
  if(leg_poses[0]==leg_poses[1])throw FormatError("Live source crouch retained its standing pose");
  if(head_poses[4]==head_poses[5])throw FormatError("Live source pitch alternatives retained the same head pose");
  if(!airborne || !landed)throw FormatError("Live source jump/landing was not reached");
  std::cout<<"PASS actual source body WALK/CROUCH/AIM/JUMP/LANDING inputs and changing WORLD poses\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
