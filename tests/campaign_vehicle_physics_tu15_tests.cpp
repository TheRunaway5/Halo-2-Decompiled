#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_object_location.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <cstdlib>
#include <set>
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/math.hpp"
#include "halo2/original_data.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cmath>
using namespace halo2;using Json=nlohmann::json;
int main(int argc,char **argv){Json report{{"reference","English TU1.5 only"},{"passed",false},{"full_physics_parity",false}};std::filesystem::path output;
 try{
  if(argc!=4)throw FormatError("Supply original XBE, campaign map and diagnostic output");output=argv[3];std::filesystem::create_directories(output/"source-storage");
  report["stage"]="load actual campaign content";
  const auto original=OriginalData::load_xbe(argv[1]);if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Campaign physics requires TU1.5");
  Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);const auto bsp=std::getenv("HALO2_TU15_CAMPAIGN_BSP")?std::stoul(std::getenv("HALO2_TU15_CAMPAIGN_BSP")):0u;const auto content=cache.load_content(bsp,false);report["loaded_bsp"]=bsp;
  std::vector<const ScenarioObject*> candidates;
  for(const auto &p:content.objects)if(p.type==1 && p.placement>=0 && !p.model_attachment) {
   const auto tag=cache.load_source_tag_metadata(p.definition);const Bytes data(tag.bytes_at(tag.source_address(),tag.size()));
   if(data.u16(0x1f0u)==3)candidates.push_back(&p);
  }
  if(candidates.empty())throw FormatError("Selected campaign BSP has no authored class3 vehicle placement");
  const auto selected=std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")?std::stol(std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")):candidates.front()->placement;
  const auto at=std::find_if(candidates.begin(),candidates.end(),[&](const auto*p){return p->placement==selected;});
  if(at==candidates.end())throw FormatError("Requested original class3 placement unavailable");
  const auto &placement=**at;report["placement"]={{"index",placement.placement},{"definition",placement.definition},{"flags",placement.flags},{"origin_bsp",placement.origin_bsp}};
  if(std::getenv("HALO2_TU15_CAMPAIGN_SPATIAL_SURVEY")) {
    report["spatial_survey"]=Json::array();
    for(unsigned b=0;b<cache.bsps().size();++b) {
      const auto geometry=cache.load_bsp_content(b);const CollisionWorld terrain(geometry.collision);
      Json hits=Json::array();
      for(unsigned n=0;n<32;++n) {
        const auto angle=float(n)*6.28318530718f/32.f;
        const auto candidate=placement.position+Vec3{std::cos(angle),std::sin(angle),0}*(std::abs(placement.scale)*placement.bounding_radius+2*content.player.height);
        const auto floor=terrain.raycast(candidate+Vec3{0,0,32},{0,0,-1},256);
        if(floor)hits.push_back({{"angle",n},{"distance",floor->distance},{"position",{floor->position.x,floor->position.y,floor->position.z}},{"normal",{floor->normal.x,floor->normal.y,floor->normal.z}}});
      }
      report["spatial_survey"].push_back({{"bsp",b},{"name",cache.bsps()[b].name},{"triangles",geometry.collision.size()},{"floor_hits",hits}});
    }
    report["passed"]=true;report["boundary"]="Read-only cached BSP survey around the original placement; no physics construction or motion proof.";
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<'\n';return 0;
  }
  report["stage"]="construct source world";MultiplayerObjectRuntime objects(argv[1],cache,bsp,output/"source-storage");auto &world=objects.tu15_world();
  report["stage"]="original map initialization";world.initialize_map();
  report["stage"]="original campaign option/clock resource configuration";world.configure_campaign_session(1,1,30,0x42815u);
  const auto word=[&](auto p){return Bytes(world.memory().view(p,4)).u32(0);};const auto game=word(0x4f3218u);
  if(word(game+8u)!=1 || Bytes(world.memory().view(game+0x132u,2)).u16(0)!=1)throw FormatError("Campaign source option binding differs");
  report["stage"]="admit two original campaign players and controllers";
  if(cache.spawns().size()<2 && !std::getenv("HALO2_TU15_CAMPAIGN_OBSERVE_VEHICLE"))
    throw FormatError("Campaign controller probe needs two authored starts or declared cached-BSP observer starts");
  std::array<Spawn,2> starts{};
  if(cache.spawns().size()>=2)starts={cache.spawns()[0],cache.spawns()[1]};
  if(std::getenv("HALO2_TU15_CAMPAIGN_OBSERVE_VEHICLE")) {
    const CollisionWorld terrain(content.collision);
    const auto radius=std::abs(placement.scale)*placement.bounding_radius;const auto atmosphere=cache.load_bsp_atmosphere(bsp);
    for(unsigned slot=0;slot<2;++slot) {
      bool found=false;
      for(unsigned n=slot;n<32;n+=2) {
        const auto angle=float(n)*6.28318530718f/32.f;
        auto candidate=placement.position+Vec3{std::cos(angle),std::sin(angle),0}*(radius+2*content.player.height);
        auto origin=candidate+Vec3{0,0,2*radius+2*content.player.height};float reach=4*radius+8*content.player.height;
        for(unsigned layer=0;layer<16 && reach>0;++layer) {
          const auto floor=terrain.raycast(origin,{0,0,-1},reach);if(!floor)break;
          const auto foot=floor->position+Vec3{0,0,content.player.radius};
          const auto location=source_object_point_location(atmosphere,std::int16_t(bsp),foot+Vec3{0,0,.5f*content.player.height});
          if(floor->normal.z>=content.player.maximum_slope_cosine && location.cluster>=0 &&
              !terrain.overlaps(foot,content.player.height,content.player.radius)) {
            starts[slot].position=foot;
            starts[slot].facing=std::atan2(placement.position.y-foot.y,placement.position.x-foot.x);
            found=true;break;
          }
          reach-=floor->distance+content.player.radius*.1f;
          origin=floor->position-Vec3{0,0,content.player.radius*.1f};
        }
        if(found)break;
      }
      if(!found)throw FormatError("Authored vehicle has no clear nearby cached BSP observer start");
    }
  }
  report["declared_initial_starts"]=Json::array();for(const auto&s:starts)report["declared_initial_starts"].push_back({{"position",{s.position.x,s.position.y,s.position.z}},{"facing",s.facing}});
  DatumPool players(world.memory(),world.player_pool());
  std::vector<std::pair<std::uint32_t,std::uint32_t>> participants;
  for(unsigned slot=0;slot<2;++slot) {
    const auto player=players.allocate_at(slot);if(player==null_datum)throw FormatError("Campaign player pool exhausted");
    auto configuration=source_default_player_configuration();constexpr std::uint32_t profile=0x6005b000u;
    world.memory().write(profile,std::as_bytes(std::span(configuration)));auto input=world.prepare_source_call();input.gp[2]=profile;
    world.run_actual(0x14e7b0u,input);world.memory().read(profile,std::as_writable_bytes(std::span(configuration)));
    objects.construct_player(player,slot,configuration);const auto &spawn=starts[slot];
    const auto unit=objects.create_biped(content.player.datum,spawn.position,direction(spawn.facing),player);
    if(unit==null_datum)throw FormatError("Campaign player biped birth returned NONE");
    objects.bind_player(player,unit);objects.initialize_player_equipment(player,unit);objects.set_local_player(player,slot);
    participants.emplace_back(player,unit);
  }
  report["players"]=participants;
  test::LivePhysicsEntryCaptureTu15 captured(output/"original-world-capture"),birth(output/"original-birth-capture"),class3(output/"original-class3-capture"),objects_frame(output/"original-object-frame-capture"),post_frame(output/"original-post-frame-capture"),unit_delivery(output/"original-unit-input-capture"),vehicle_update(output/"original-vehicle-update-capture"),vehicle_controller(output/"original-vehicle-controller-capture"),controller_admission(output/"original-controller-admission-capture");
  unsigned current_tick=0;const bool unit_input=std::getenv("HALO2_TU15_CAMPAIGN_UNIT_INPUT")!=nullptr;
  const bool neutral_unit_input=std::getenv("HALO2_TU15_CAMPAIGN_UNIT_INPUT_NEUTRAL")!=nullptr;
  std::uint32_t object=null_datum;report["unit_control_admission"]=Json::array();
  SourcePhysicsEntryObserverBinding observation(world.memory(),[&](bool entering,std::uint32_t entry,const auto&a){
    if(object!=null_datum && current_tick==30 && (entry==0x14fec0u || entry==0xb8f40u || entry==0xb9090u || entry==0xb9180u || entry==0xf2340u || entry==0xf48c0u || entry==0xc92c0u)) {
      const auto body=world.source_object_body(object);Vec3 throttle,requested;
      world.memory().read(body+0x1b0u,std::as_writable_bytes(std::span(&throttle,1)));
      world.memory().read(body+0x354u,std::as_writable_bytes(std::span(&requested,1)));
      report["unit_control_admission"].push_back({{"entry",entry},{"entering",entering},{"unit_flags",word(body+0x14cu)},{"controller_flags",word(body+0x134u)},{"action_flags",word(body+0x148u)},{"vehicle_flags",word(body+0x348u)},{"throttle",{throttle.x,throttle.y,throttle.z}},{"requested",{requested.x,requested.y,requested.z}}});
    }
    if(entry==0xce440u)controller_admission.observe(entering,a,world.memory(),entry);
    else if(entry==0xf2340u && (!unit_input || current_tick>=35))vehicle_update.observe(entering,a,world.memory(),entry);
    else if(entry==0xf48c0u && (!unit_input || current_tick>=35))vehicle_controller.observe(entering,a,world.memory(),entry);
    else if(entry==0xb8f40u && (!unit_input || current_tick>=35))objects_frame.observe(entering,a,world.memory(),entry);
    else if(entry==0x1cc8d0u && (!unit_input || current_tick>=35))captured.observe(entering,a,world.memory(),entry);
    else if(entry==0xb9180u && (!unit_input || current_tick>=35))post_frame.observe(entering,a,world.memory(),entry);
    else if(entry==0xc92c0u && current_tick>=30)unit_delivery.observe(entering,a,world.memory(),entry);
    else if(entry==0xc1120u)birth.observe(entering,a,world.memory(),entry);
    else if(entry==0xf0e70u)class3.observe(entering,a,world.memory(),entry);
  });
  report["stage"]="authored runtime class3 birth";object=world.create_scenario_object(placement,SourceScenarioBirth::runtime);
  if(object==null_datum)throw FormatError("Original runtime class3 birth returned NONE");report["object"]=object;
  if(std::getenv("HALO2_TU15_CAMPAIGN_NATIVE_UNIT_CONTROLLER"))objects.set_unit_controller_active(object,true);
  report["steps"]=Json::array();auto &steps=report["steps"];report["stage"]="complete original object/world frames";
  unsigned controller_stores=0;std::set<std::uint32_t> controller_pcs;
  world.observe_physics_stores([&](auto pc,const auto&){if(pc>=0xf48c0u && pc<0xf5570u){++controller_stores;controller_pcs.insert(pc);}});
  struct StopStoreObservation {PhysicsLiveWorldTu15 &world;~StopStoreObservation(){world.observe_physics_stores({});}} stop{world};
  float maximum_vehicle_distance=0;
  for(unsigned n=0;n<(unit_input?90u:10u);++n){
   current_tick=n;report["active_tick"]=n;
   if(unit_input) {
    constexpr std::uint32_t packet=0x6005b100u;
    auto input=world.prepare_source_call();input.gp[2]=packet;world.run_actual(0xc93d0u,input);
    const Vec3 throttle=n<30 || neutral_unit_input?Vec3{}:Vec3{1,0,.5f};
    world.memory().write(packet+0x14u,std::as_bytes(std::span(&throttle,1)));
    const auto facing=objects.object_transform(object).basis[0];
    for(auto offset:{0x28u,0x34u,0x40u})world.memory().write(packet+offset,std::as_bytes(std::span(&facing,1)));
    input=world.prepare_source_call();input.gp[0]=object;input.gp[1]=packet;world.run_actual(0xc92c0u,input);
   }
   for(unsigned slot=0;slot<participants.size();++slot) {
    std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
    const auto yaw=starts[slot].facing,forward=!unit_input && n>=5?1.f:0.f;
    std::memcpy(command.data()+4,&yaw,4);std::memcpy(command.data()+0xc,&forward,4);
    objects.deliver_command(participants[slot].first,participants[slot].second,command);
   }
   objects.step_objects();const auto body=world.source_object_body(object);if(!body)throw FormatError("Actual class3 generation retired during the bounded frame probe");
   Vec3 p,v;world.memory().read(body+0x64u,std::as_writable_bytes(std::span(&p,1)));world.memory().read(body+0x88u,std::as_writable_bytes(std::span(&v,1)));
   if(!std::isfinite(p.x+p.y+p.z+v.x+v.y+v.z))throw FormatError("Class3 source body published nonfinite motion");
   maximum_vehicle_distance=std::max(maximum_vehicle_distance,length(p-placement.position));
   report["maximum_vehicle_distance"]=maximum_vehicle_distance;report["class3_controller_stores"]=controller_stores;report["class3_controller_store_pcs"]=controller_pcs;
   Json controllers=Json::array();for(const auto &[player,unit]:participants) {
    const auto motion=objects.biped_motion(unit);controllers.push_back({{"player",player},{"unit",unit},{"position",{motion.position.x,motion.position.y,motion.position.z}},{"velocity",{motion.velocity.x,motion.velocity.y,motion.velocity.z}}});
   }
   steps.push_back({{"step",n},{"position",{p.x,p.y,p.z}},{"velocity",{v.x,v.y,v.z}},{"physics_component",word(body+0xb4u)},{"controllers",controllers}});
  }
  report["declared_unit_input"]=unit_input;report["neutral_unit_input"]=neutral_unit_input;
  if(std::getenv("HALO2_TU15_CAMPAIGN_REQUIRE_CONTROLLED_MOTION") && (!controller_stores || maximum_vehicle_distance<=content.player.radius))
    throw FormatError("Actual class3 controller did not produce nonvacuous solved vehicle motion");
  report["passed"]=true;report["stage"]="bounded source world frames completed";
  report["boundary"]="Actual authored campaign placement, original kind1 options, two admitted source players with declared authored or cached-BSP initial observer starts and neutral/forward player command input, optional declared NPC C93D0/C92C0 input, and full retained source object/world frames. No supplied motion/contact/solver result. This is a physics integration probe, not completed campaign AI/script/controller integration or visible multiplayer proof.";
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(!output.empty())std::ofstream(output/"report.json")<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
