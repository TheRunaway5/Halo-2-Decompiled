#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_game_variant.hpp"
#include <iostream>
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <cstring>
using namespace halo2;
int main(int argc,char**argv)try {
 if(argc!=5)throw FormatError("Supply CAMPAIGN_MAP MAPS XBE OUTPUT");
 std::cout<<std::unitbuf;const auto original=OriginalData::load_xbe(argv[3]);Cache cache(argv[1],argv[2],original);
 const std::string vehicle_name=std::getenv("HALO2_CAMPAIGN_CAMERA_DEFINITION")?
   std::getenv("HALO2_CAMPAIGN_CAMERA_DEFINITION"):"objects\\vehicles\\ghost\\ghost";
 const unsigned seat_index=std::getenv("HALO2_CAMPAIGN_CAMERA_SEAT")?std::stoul(std::getenv("HALO2_CAMPAIGN_CAMERA_SEAT")):0;
 const bool declared_seat_api=std::getenv("HALO2_CAMERA_DECLARED_SEAT_API");
 const bool multiplayer=cache.load_source_geometry_scenario_type()==1;
 auto content=cache.load_content(0,multiplayer);std::filesystem::create_directories(argv[4]);
 const auto placement=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p){
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.datum==p.definition;});
  return p.type==1 && p.placement>=0 && !p.model_attachment && tag!=cache.tags().end() && tag->name==vehicle_name;
 });
 const auto vehicle_tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.group=="vehi" && t.name==vehicle_name;});
 if(vehicle_tag==cache.tags().end())throw FormatError("Missing requested original vehicle definition");
 const auto vehicle_definition=vehicle_tag->datum;
 MultiplayerObjectRuntime objects(argv[3],cache,0,std::filesystem::path(argv[4])/"source-storage");auto &world=objects.tu15_world();
 if(multiplayer)objects.configure_session(source_game_variant_default(0),1,30,0x1234);
 else {world.initialize_map();world.configure_campaign_session(1,1,30,0x1234);}
 std::uint32_t vehicle=null_datum;
 if(placement!=content.objects.end())vehicle=world.create_scenario_object(*placement,SourceScenarioBirth::runtime);
 else {
   if(cache.spawns().empty())throw FormatError("Campaign has no authored player start for the declared constructor input");
   CollisionWorld terrain(content.collision);const auto start=cache.spawns().front();
   const auto floor=terrain.raycast(start.position+Vec3{0,0,32},{0,0,-1},256);
   if(!floor)throw FormatError("Authored campaign start has no BSP floor for constructor input");
   constexpr std::uint32_t request=0x60010000u;
   auto input=world.prepare_source_call(std::array{vehicle_definition,null_datum,0u});input.gp[6]=request;
   world.run_actual(0xb9960u,input);
   const Vec3 position=floor->position,forward{1,0,0};
   world.memory().write(request+0x1c,std::as_bytes(std::span(&position,1)));
   world.memory().write(request+0x28,std::as_bytes(std::span(&forward,1)));
   vehicle=world.call_actual(0xb9b70u,std::array{request});
   // Metadata input only; object birth remains the original generic API.
   ScenarioObject metadata;metadata.type=1;metadata.definition=vehicle_definition;
   content.objects.push_back(metadata);content.unit_interactions=load_native_unit_interaction_assets(cache,content);
 }
 if(vehicle==null_datum)throw FormatError("Original campaign Ghost birth rejected");
 const auto &definition=content.unit_interactions->units.at(vehicle_definition);const auto &seat=definition.seats.at(seat_index);
 const auto marker=definition.markers.find(seat.entry_marker);
 Spawn spawn;
 if(declared_seat_api) {
   if(cache.spawns().empty())throw FormatError("Declared seat API input needs an authored player start");
   spawn=cache.spawns().front();
 } else {
   if(marker==definition.markers.end())throw FormatError("Authored campaign seat has no ordinary entry marker");
   const auto markers=pose_model_markers(marker->second,objects.world_nodes(vehicle),definition.permutations,{},1);
   if(markers.empty())throw FormatError("Campaign vehicle has no live entry marker");
   spawn.position=markers.front().world.position+markers.front().world.basis[0]*(seat.radius*.5f);
   CollisionWorld terrain(content.collision);const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
   if(!floor)throw FormatError("Campaign vehicle has no nearby cached BSP floor");spawn.position.z=floor->position.z+content.player.radius;
   spawn.facing=std::atan2(markers.front().world.position.y-spawn.position.y,markers.front().world.position.x-spawn.position.x);
 }
 DatumPool players(world.memory(),world.player_pool());const auto player=players.allocate_at(0);
 auto configuration=source_default_player_configuration();constexpr std::uint32_t profile=0x6005b000;
 world.memory().write(profile,std::as_bytes(std::span(configuration)));auto a=world.prepare_source_call();a.gp[2]=profile;world.run_actual(0x14e7b0,a);
 world.memory().read(profile,std::as_writable_bytes(std::span(configuration)));objects.construct_player(player,0,configuration);
 const auto unit=objects.create_biped(content.player.datum,spawn.position,direction(spawn.facing),player);
 objects.bind_player(player,unit);objects.initialize_player_equipment(player,unit);objects.set_local_player(player,0);
 if(std::getenv("HALO2_CAMERA_PLAYER_PROHIBITED")) {
   if(!(seat.flags&0x10000u))throw FormatError("Expected an authored player-prohibited seat");
   test::LivePhysicsEntryCaptureTu15 capture(std::filesystem::path(argv[4])/"captures/campaign/admission");
   SourcePhysicsEntryObserverBinding observer(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
     if(entry==0xcb7a0u)capture.observe(entering,abi,objects.memory(),entry);
   });
   if(world.call_actual(0xcb7a0u,std::array{unit,vehicle,seat_index,0u,0u})&255u)
     throw FormatError("Original admission accepted a player-prohibited seat");
   std::cout<<"PASS original player-prohibited seat admission: "<<vehicle_name<<" seat="<<seat_index<<'\n';return 0;
 }
 NativeVehicleCamera camera(objects,*original);bool entered=false;unsigned camera_frames=0;
 test::LivePhysicsEntryCaptureTu15 dialogue(std::filesystem::path(argv[4])/"captures/campaign/dialogue");
 test::LivePhysicsEntryCaptureTu15 director(std::filesystem::path(argv[4])/"captures/campaign/director");
 test::LivePhysicsEntryCaptureTu15 seat_entry(std::filesystem::path(argv[4])/"captures/campaign/seat-entry");
 SourcePhysicsEntryObserverBinding observation(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
   if(entry==0x217470u)dialogue.observe(entering,abi,objects.memory(),entry);
   if(entry==0xec780u)seat_entry.observe(entering,abi,objects.memory(),entry);
   if(entry==0x157fe0u && entered)director.observe(entering,abi,objects.memory(),entry);
 });
 if(declared_seat_api) {
   std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);objects.prepare_player_look(player,unit,command);
   // Declared component input to originalEC780, not ordinary admission proof:
   // skip entry-animation selection, action kind1F, no output, force attach.
   // OriginalBAF10 andEC995 still own every parent/seat write.
   auto entry=world.prepare_source_call(std::array{vehicle,seat_index,1u,0x1fu,0u,1u});entry.gp[0]=unit;
   world.run_actual(0xec780u,entry);
   const auto motion=objects.biped_motion(unit);entered=motion.parent==vehicle;
   if(!entered)throw FormatError("Declared original seat API rejected its component input");
 }
 for(unsigned tick=0;tick<180;++tick) {
  std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
  const auto put=[&](unsigned p,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)command[p+i]=std::byte(v>>(8*i));};
  put(4,std::bit_cast<std::uint32_t>(spawn.facing));
  if(!entered){put(0x1c,source_pickup_buttons(true,900,false,0,source_pickup_hold_threshold(content.pickup_hold_seconds)),2);put(0x28,5);put(0x2a,seat_index,2);put(0x2c,vehicle);}
  if(declared_seat_api) {
    const auto angle=float(tick%9)*.75f;
    put(4,std::bit_cast<std::uint32_t>(angle));put(8,std::bit_cast<std::uint32_t>(angle-3.f));
  }
  objects.prepare_player_look(player,unit,command);
  if(!declared_seat_api){objects.populate_weapon_targeting(player,unit,command);objects.deliver_command(player,unit,command);objects.step_objects();}
  const auto motion=objects.biped_motion(unit);entered|=motion.parent==vehicle;
  const auto view=camera.update_player(0,player,Bytes(std::as_bytes(std::as_const(players).lookup(player))).u16(2),1.f/30.f);
  if(view && motion.parent==vehicle) {
    if(view->camera_mode!=1)throw FormatError("Original campaign seat did not select third person");
    if(!std::isfinite(view->horizontal_fov) || view->horizontal_fov<=0)throw FormatError("Campaign source camera lost its lens");
    ++camera_frames;
  }
 }
 if(!entered || !camera_frames)throw FormatError("Campaign Ghost did not admit its original seated camera");
 std::cout<<"PASS "<<(declared_seat_api?"declared original seat API camera component":"actual campaign options/profile/vehicle/player/command frames")<<": "<<vehicle_name<<" seat="<<seat_index<<" camera frames="<<camera_frames<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
