#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/vehicle_shading_reference.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/world_effects.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <bit>
#include <chrono>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
namespace {void check(bool v,const char* why){if(!v)throw FormatError(why);}}
namespace {std::vector<VehicleShadingReference> reference_vehicles;bool reference_image_storage{};}
namespace halo2 {
void vehicle_shading_reference_material_storage(Material& material){
  // Diagnostic only: isolate the pinned emulator texture decoder from ordinary
  // Vulkan compressed sampling. This is absent from the shipping runtime.
  if(reference_image_storage)prepare_frontend_material_storage(material);
}
const VehicleShadingReference* vehicle_shading_reference(std::uint32_t definition,int row){
  const auto found=std::find_if(reference_vehicles.begin(),reference_vehicles.end(),[&](const auto& r){return r.definition==definition && r.netgame_row==row;});
  return found==reference_vehicles.end()?nullptr:&*found;
}
}
int main(int argc,char**argv)try{
  check(argc>=8 && argc<=11,"Supply MAP SHARED_MAPS TU15_XBE_OR_native-mcc WIDTH HEIGHT VIEWS FRESH_OUTPUT [VEHICLE_TAG_NAME [PRIMARY_HEAVY_CHOICE [REFERENCE_INPUTS_JSON]]]");
#ifdef HALO2_LIVE_VEHICLE_MODEL_CAPTURE
  check(argc!=11,"The live model collector diagnostic does not accept substituted reference owners");
#endif
  const bool mcc=std::string_view(argv[3])=="native-mcc";
  const auto original=mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[3]);
  if(original)check(original->image==OriginalData::Image::TitleUpdate15,"Vehicle captures require OG Xbox TU1.5");
  check(!mcc || argc!=11,"MCC live captures do not accept substituted original reference owners");
  MultiplayerOptions options;
  if(argc>=10)options.variant.values.set(78,std::stoi(argv[9]));
  const auto *parameter=std::getenv("HALO2_CAPTURE_VEHICLE_VARIANT_PARAMETER");
  const auto *choice=std::getenv("HALO2_CAPTURE_VEHICLE_VARIANT_CHOICE");
  check(bool(parameter)==bool(choice),"Declare both vehicle variant parameter and choice");
  if(parameter) {
    const std::string p=parameter,c=choice;std::size_t parsed_p{},parsed_c{};
    const auto index=std::stoi(p,&parsed_p),value=std::stoi(c,&parsed_c);
    check(parsed_p==p.size() && parsed_c==c.size() && (index==76 || index==78 || index==81) &&
        value>=0 && value<=15,"Unsupported explicit live vehicle variant input");
    options.variant.values.set(index,value);
  }
  Json reference;
  if(argc==11){
    std::ifstream file(argv[10]);check(bool(file),"Missing original vehicle frame inputs");file>>reference;
    check(reference.at("reference")=="OG Xbox TU1.5" && reference.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original vehicle frame authority");
    reference_image_storage=reference.value("reference_texture_storage",std::string{})=="xemu-0.8.136-expanded-bc";
    const auto scalar=[](const Json& word){const auto value=std::bit_cast<float>(word.get<std::uint32_t>());check(std::isfinite(value),"Nonfinite original vehicle frame input");return value;};
    const auto vector=[&](const Json& words,unsigned offset){return Vec3{scalar(words.at(offset)),scalar(words.at(offset+1)),scalar(words.at(offset+2))};};
    const auto& c=reference.at("camera");check(c.size()==12,"Incomplete original view camera");Camera camera;
    camera.position=vector(c,0);camera.forward=vector(c,3);camera.up=vector(c,6);camera.oriented=true;
    options.capture_camera=camera;options.capture_projection=Vec2{scalar(c.at(10)),scalar(c.at(11))};
    for(const auto& input:reference.at("vehicles")){
      VehicleShadingReference frame;frame.definition=input.at("definition");frame.netgame_row=input.at("netgame_row");
      for(const auto& node:input.at("nodes")){check(node.size()==13,"Incomplete original vehicle WORLD node");frame.nodes.push_back({scalar(node.at(0)),{vector(node,1),vector(node,4),vector(node,7)},vector(node,10)});}
      frame.sphere={vector(input.at("sphere"),0),scalar(input.at("sphere").at(3))};frame.origin=vector(input.at("origin"),0);frame.velocity=vector(input.at("velocity"),0);
      frame.geometry_flags=input.at("geometry_flags");frame.geometry_control_flags=input.at("geometry_control_flags");
      frame.expected_geometry_lods=input.at("geometry_lods").get<decltype(frame.expected_geometry_lods)>();
      frame.flags=input.at("flags");frame.frozen=input.at("frozen");frame.expected_lighting=input.at("expected_lighting").get<decltype(frame.expected_lighting)>();reference_vehicles.push_back(std::move(frame));
    }
  }
  const Cache cache(argv[1],argv[2],original);
  check(cache.is_mcc()==mcc,"Vehicle capture received the wrong cache platform");
  const auto content=cache.load_content(0,true,source_game_variant(options.variant));
  const std::string vehicle_name=argc>=9?argv[8]:"objects\\vehicles\\warthog\\warthog";
  const ScenarioObject* vehicle=nullptr;
  for(const auto& o:content.objects)if(o.type==1 && o.netgame_row>=0 && !o.model_attachment){
    if(source_netgame_equipment_selector(content.netgame_equipment.at(unsigned(o.netgame_row)),true,2)<0)continue;
    const auto tag=cache.query_tag({.handle=TagHandle{o.definition}});
    if(tag && tag->name==vehicle_name && (argc!=11 || vehicle_shading_reference(o.definition,o.netgame_row))){vehicle=&o;break;}
  }
  check(vehicle,"Vehicle capture requires the selected authored vehicle netgame placement");
  if(argc==11)check(vehicle_shading_reference(vehicle->definition,vehicle->netgame_row),"Original frame omitted selected vehicle/row");
  const auto out=std::filesystem::absolute(argv[7]);check(!std::filesystem::exists(out),"Vehicle capture output already exists");std::filesystem::create_directories(out);
  const auto basis=authored_basis(vehicle->rotation);const auto target=vehicle->position+Vec3{0,0,.6f};
  Vec3 position{};bool camera_admitted=false;const CollisionWorld terrain(content.collision);
  auto camera_distances=mcc?std::vector<float>{8.f,12.f,6.f,4.f,2.f,1.f}:
      std::vector<float>{4.f,6.f,2.f,1.f};
  float camera_azimuth{};auto camera_forward=basis.forward,camera_left=basis.left;
  const auto camera_scalar=[&](const char* key)->std::optional<float> {
    const auto *raw=std::getenv(key);if(!raw)return {};
    check(mcc,"Declared camera survey inputs require native MCC captures");
    const std::string value=raw;std::size_t consumed{};const auto result=std::stof(value,&consumed);
    check(consumed==value.size() && std::isfinite(result),"Invalid declared camera survey input");return result;
  };
  if(const auto degrees=camera_scalar("HALO2_CAPTURE_VEHICLE_CAMERA_AZIMUTH")) {
    check(*degrees>=-360 && *degrees<=360,"Camera azimuth must be within360 degrees");
    camera_azimuth=*degrees;const auto angle=*degrees*(3.14159265358979323846f/180.f);
    camera_forward=basis.forward*std::cos(angle)+basis.left*std::sin(angle);
    camera_left=basis.left*std::cos(angle)-basis.forward*std::sin(angle);
  }
  if(const auto distance=camera_scalar("HALO2_CAPTURE_VEHICLE_CAMERA_DISTANCE")) {
    check(*distance>=.5f && *distance<=24,"Declared camera distance must be from0.5 to24");
    camera_distances={*distance};
  }
  for(float distance:camera_distances){
    for(float side:{2.f,0.f,-2.f}){
      auto candidate=vehicle->position+camera_forward*distance+camera_left*side;
      const auto floor=terrain.raycast(candidate+Vec3{0,0,1},{0,0,-1},16);
      if(!floor)continue;candidate.z=floor->position.z+content.player.radius;
      const auto eye=candidate+Vec3{0,0,content.player.camera_height};const auto delta=target-eye;
      if(terrain.raycast(eye,normalized(delta),std::max(0.f,length(delta)-.05f)))continue;
      position=candidate;camera_admitted=true;break;
    }
    if(camera_admitted)break;
  }
  check(camera_admitted,"No declared camera has an original BSP floor and clear vehicle sightline");
  const auto eye=position+Vec3{0,0,content.player.camera_height};const auto look=normalized(target-eye);
  const auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
  Spawn spawn;spawn.position=position;spawn.facing=yaw;spawn.bsp=0;
  options.map=argv[1];options.shared_maps=argv[2];if(!mcc)options.gameplay_xbe=argv[3];
  options.width=std::stoul(argv[4]);options.height=std::stoul(argv[5]);options.local_players=std::stoul(argv[6]);
  options.source_object_world=true;options.source_object_savegame_directory=out/"source-savegame";
  options.hidden=true;options.validation=!mcc;options.mute=true;options.starting_weapon=5;
  options.capture_tick=8;options.reference_tick_cadence=true;options.reference_render_fraction=0;
  bool grenade_capture=false,grenade_detonated=false;unsigned grenade_light_tick{},grenade_light_packets{};
  bool fixed_grenade_replay_end=false;
#ifdef HALO2_LIVE_VEHICLE_MODEL_CAPTURE
  grenade_capture=std::getenv("HALO2_CAPTURE_VEHICLE_GRENADE")!=nullptr;
  if(grenade_capture){
    options.capture_tick=200;options.reference_auxiliary_seed=0x12345678u;
    if(const auto* end=std::getenv("HALO2_CAPTURE_VEHICLE_GRENADE_END_TICK")) {
      const std::string value=end;std::size_t parsed{};const auto tick=std::stoul(value,&parsed);
      check(parsed==value.size() && tick>=200 && tick<=1000000,"Cleanup replay requires a finite tick from200 to1000000");
      options.capture_tick=unsigned(tick);fixed_grenade_replay_end=true;
    }
  }
#endif
  options.native_spawn_inputs={spawn};options.input_script=out/"inputs.txt";options.screenshot=out/"vehicle.ppm";
  {std::ofstream input(options.input_script);input.precision(9);
    for(unsigned t=0;t<options.capture_tick;++t)for(unsigned p=0;p<options.local_players;++p)
      input<<t<<' '<<p<<" 0 0 "<<(grenade_capture && t==20 && p==0?1:0)<<' '<<yaw<<' '<<pitch<<'\n';}
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  // Explicit pre-tick scene creation through the real source netgame/vehicle
  // constructor. Automatic netgame-row scheduling is a separate owner.
  const auto placement=std::size_t(vehicle-content.objects.data());
  const auto root=options.prepared_world->match.create_world_vehicle(placement);
  check(root!=null_datum,"Original vehicle constructor did not admit its authored netgame row");
  for(unsigned i=0;i<content.objects.size();++i)if(content.objects[i].model_attachment && content.objects[i].model_attachment->parent==placement)
    options.prepared_world->match.create_world_vehicle(i);
  Json report={{"reference","OG Xbox TU1.5"},{"boundary","Native live vehicle shading with original assets and physics inputs. Declared camera/spawn; not a matched original framebuffer."},
    {"vehicle",vehicle_name},{"primary_heavy_choice",source_game_variant_get(source_game_variant(options.variant),78)},
    {"netgame_row",vehicle->netgame_row},{"definition",vehicle->definition},{"eye",{eye.x,eye.y,eye.z}},{"target",{target.x,target.y,target.z}},{"views",options.local_players},{"draws",Json::array()},{"source_inputs",Json::array()}};
  report["runtime_cache_source"]=mcc?"MCC":"offline Xbox reference";
  report["Vulkan_validation_layer_requested"]=options.validation;
  report["native_gameplay_xbe_argument_supplied"]=!mcc;
  report["declared_camera_survey"]={{"azimuth_degrees",camera_azimuth},{"distances",camera_distances},
      {"vehicle_moved",false},{"BSP_floor_and_center_sightline_required",true}};
  if(parameter)report["declared_vehicle_variant"]={{"parameter",std::stoi(parameter)},{"choice",std::stoi(choice)}};
  if(argc==11){report["original_frame_inputs"]=std::filesystem::absolute(argv[10]).string();report["boundary"]="Offline shader comparison with captured original camera, WORLD nodes, sphere, velocity and flags; native lighting is independently computed. Other world objects, clocks, first person and full simulation are not restored.";}
  report["reference_texture_storage"]=reference_image_storage?"xemu-0.8.136-expanded-bc":"ordinary-native-compressed";
  const auto *forced_fog=std::getenv("HALO2_REFERENCE_FORCE_ATMOSPHERIC_FOG");
  const bool fog_override=forced_fog && *forced_fog;
  report["forced_atmospheric_fog_control"]=fog_override;
  report["eligible_for_original_pixel_parity"]=!fog_override;
  if(fog_override)report["boundary"]=report["boundary"].get<std::string>()+
      " Counterfactual presentation-only atmospheric fog override; excluded from original pixel acceptance.";

  std::vector<unsigned> lit(options.local_players),drawn(options.local_players),visible(options.local_players);
  std::vector<std::vector<Vertex>> geometry(options.local_players);
  std::set<std::uint32_t> source_owners;
  std::uint32_t source_input_tick{};Json current_source_inputs=Json::array();
  std::map<std::uint64_t,NativeGrenadeDetonation> actual_detonations;
  const auto tag_name=[&](std::uint32_t id){const auto tag=cache.query_tag({.handle=TagHandle{id}});return tag?tag->name:std::string{};};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state){
    if(grenade_capture && !grenade_detonated && !state.grenade_detonations.empty())
      std::cout<<"Actual scripted grenade effect delivery: tick="<<state.tick
          <<" events="<<state.grenade_detonations.size()<<'\n'<<std::flush;
    grenade_detonated|=!state.grenade_detonations.empty();
    for(const auto& d:state.grenade_detonations)actual_detonations.emplace(d.serial,d);
    source_input_tick=state.tick;current_source_inputs=Json::array();
    for(const auto& v:state.world_vehicles){check(v.lighting_owner!=null_datum && v.lighting_definition!=null_datum,"Vehicle lost its actual source lighting owner");source_owners.insert(v.lighting_owner);
      current_source_inputs.push_back({{"object",v.handle},{"root",v.lighting_owner},{"definition",v.lighting_definition},
        {"origin",{v.lighting_origin.x,v.lighting_origin.y,v.lighting_origin.z}},{"velocity",{v.lighting_velocity.x,v.lighting_velocity.y,v.lighting_velocity.z}},
        {"flags",v.lighting_flags},{"frozen",v.lighting_frozen}});
    }
    if(state.tick==options.capture_tick)report["source_inputs"]=current_source_inputs;
  };
  options.effect_light_capture_request=[&](unsigned view,std::uint32_t tick,
      std::span<const EffectLightRawDraw> packets,std::span<const WorldEffectLight> lights) {
    if(!grenade_capture || view || !grenade_detonated || grenade_light_tick)return false;
    std::map<std::uint32_t,unsigned> model_packets;
    for(const auto& packet:packets)if(packet.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object) {
      ++model_packets[packet.light_identity];++grenade_light_packets;
    }
    if(!grenade_light_packets)return false;
    // Capture the first completed post-detonation model-light frame. Its
    // original effect identity determines the label; timing alone does not.
    check(source_input_tick==tick,"Vehicle light capture lost its completed simulation input frame");
    grenade_light_tick=tick;
    report["first_model_light_source_inputs"]=current_source_inputs;
    if(!fixed_grenade_replay_end){options.capture_tick=tick;report["source_inputs"]=current_source_inputs;}
    report["model_light_sources"]=Json::array();
    for(const auto& [identity,count]:model_packets) {
      const auto light=std::find_if(lights.begin(),lights.end(),[&](const auto& l){return l.identity==identity;});
      check(light!=lights.end(),"Submitted model light lost its actual occupied source identity");
      Json matches=Json::array();
      for(const auto& [serial,d]:actual_detonations)if(d.projectile==light->effect_owner)
        matches.push_back({{"serial",serial},{"tick",d.tick},{"projectile",d.projectile},{"effect",d.effect},
          {"effect_tag",tag_name(d.effect)},{"direct_effect_match",d.effect==light->effect_definition}});
      report["model_light_sources"].push_back({{"identity",identity},{"definition",light->definition},
        {"light_tag",tag_name(light->definition)},{"effect_owner",light->effect_owner},
        {"effect_definition",light->effect_definition},{"effect_tag",tag_name(light->effect_definition)},
        {"delivery_tick",light->tick},{"model_packets",count},{"grenade_owner_matches",matches},
        {"position",{light->position.x,light->position.y,light->position.z}}});
    }
    report["capture_reason"]="First actual model-light packets after scripted grenade detonation; consult delivery provenance for effect attribution.";
    return !fixed_grenade_replay_end;
  };
  options.vehicle_draw_observer=[&](unsigned view,std::uint32_t object,std::uint32_t definition,const ObjectLightingState& light,bool valid,unsigned lod,std::span<const Vertex> vertices){
    if(!vertices.empty()){++drawn.at(view);if(valid)++lit.at(view);}
    std::map<std::uint32_t,unsigned> materials;for(const auto& vertex:vertices)++materials[vertex.material];
    Json selected=Json::array();for(const auto& [slot,count]:materials)selected.push_back({{"resident_material",slot},{"vertices",count}});
    report["draws"].push_back({{"view",view},{"object",object},{"definition",definition},{"lighting_valid",valid},{"geometry_lod",lod},{"lighting",light.source_values()},{"vertices",vertices.size()},{"materials",selected}});
    if(const auto* frame=vehicle_shading_reference(definition,vehicle->netgame_row)){
      report["draws"].back()["original_geometry_lod"]=frame->expected_geometry_lods.at(view);
      report["draws"].back()["original_geometry_lod_equal"]=int(lod)==frame->expected_geometry_lods.at(view);
      std::array<std::uint32_t,21> actual{};const auto values=light.source_values();for(unsigned i=0;i<actual.size();++i)actual[i]=std::bit_cast<std::uint32_t>(values[i]);
      report["draws"].back()["original_lighting_words"]=frame->expected_lighting;report["draws"].back()["native_lighting_words"]=actual;
      // The record's final dword is packed metadata; source_values serializes
      // a dummy zero there. Dynamic lighting externs consume the first20 floats.
      report["draws"].back()["original_lighting_metadata"]=frame->expected_lighting.back();
      report["draws"].back()["original_lighting_shader_floats_equal"]=std::equal(actual.begin(),actual.begin()+20,frame->expected_lighting.begin());
    }
    geometry.at(view).insert(geometry.at(view).end(),vertices.begin(),vertices.end());
  };
  options.view_observer=[&](unsigned view,const Camera& camera,float horizontal_fov,std::int16_t){
    visible.at(view)=0;const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
    const auto right=normalized(cross(forward,camera.oriented?camera.up:Vec3{0,0,1}));const auto up=cross(right,forward);
    const float horizontal=std::tan(horizontal_fov*.5f),aspect=float(options.width)/float(options.height)*(options.local_players==2?2.f:1.f);
    const auto& vertices=geometry.at(view);
    for(std::size_t i=0;i+2<vertices.size();i+=3){
      const auto center=(vertices[i].position+vertices[i+1].position+vertices[i+2].position)/3.f;
      const auto delta=center-camera.position;const auto ahead=dot(delta,forward);
      if(ahead<=0 || std::abs(dot(delta,right))>ahead*horizontal || std::abs(dot(delta,up))>ahead*horizontal/aspect)continue;
      const auto distance=length(delta);
      if(distance>.01f && !terrain.raycast(camera.position,delta/distance,distance-.01f))++visible.at(view);
    }
    geometry.at(view).clear();
    report["final_view_geometry"][std::to_string(view)]={{"eye",{camera.position.x,camera.position.y,camera.position.z}},{"unoccluded_triangle_centers",visible.at(view)}};
  };
  const auto result=run_multiplayer(options);report["runtime_exit"]=result;report["source_owners"]=source_owners.size();report["lit_draws"]=lit;report["draw_counts"]=drawn;
  bool reference_verified=true;unsigned reference_draws{};
  if(argc==11) {
    for(const auto& draw:report["draws"])if(draw.contains("original_lighting_shader_floats_equal")) {
      ++reference_draws;reference_verified=reference_verified && draw["original_lighting_shader_floats_equal"].get<bool>() && draw["original_geometry_lod_equal"].get<bool>();
    }
    reference_verified=reference_verified && reference_draws>0 && options.local_players==1;
    report["original_lighting_and_geometry_verified"]=reference_verified;
  }
  std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  if(grenade_capture) {
    report["grenade_detonated"]=grenade_detonated;report["model_light_tick"]=grenade_light_tick;
    report["model_light_packets"]=grenade_light_packets;
    report["fixed_cleanup_replay"]=fixed_grenade_replay_end;report["capture_tick"]=options.capture_tick;
    std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
    check(grenade_detonated && grenade_light_tick && grenade_light_packets,"No actual model-light packets were completed after the scripted grenade");
  }
  check(reference_verified,"Captured original vehicle lighting/geometry differs or the fixture does not represent this view layout");
  check(result==0 && !source_owners.empty() && std::filesystem::exists(options.screenshot),"Vehicle capture did not complete");
  for(unsigned v=0;v<options.local_players;++v){check(drawn[v] && lit[v],"A local view lacks a live vehicle lighting draw");
    if(!fixed_grenade_replay_end)check(visible[v]>0,"Vehicle geometry is outside the actual view or occluded by the map; draw admission is not appearance proof");}
  // Exercise the real transport after capturing, so the joining remote unit
  // cannot change the scene whose pixels were saved above.
  {
    auto& match=options.prepared_world->match;
    constexpr std::uint32_t key=0x76656869u;
    LanHost host(match,0,key);LanClient client("127.0.0.1:"+std::to_string(host.port()),key,1);
    std::array<Controls,1> input{};std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned attempt=0;attempt<40 && !client.ready();++attempt){
      client.send(input);host.receive(controls);host.broadcast();client.receive();
      if(!client.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    check(client.ready(),"Vehicle lighting UDP snapshot did not arrive");
    const auto expected=match.state();const auto& actual=client.state();
    check(!expected.world_vehicles.empty() && actual.world_vehicles.size()==expected.world_vehicles.size(),"UDP vehicle owner bank differs");
    const auto same=[](float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);};
    for(unsigned i=0;i<expected.world_vehicles.size();++i){
      const auto& a=actual.world_vehicles[i];const auto& e=expected.world_vehicles[i];
      check(a.handle==e.handle && a.placement==e.placement && a.lighting_owner==e.lighting_owner &&
          a.lighting_definition==e.lighting_definition && a.lighting_flags==e.lighting_flags && a.lighting_frozen==e.lighting_frozen &&
          a.geometry_flags==e.geometry_flags && a.geometry_control_flags==e.geometry_control_flags &&
          same(a.lighting_origin.x,e.lighting_origin.x) && same(a.lighting_origin.y,e.lighting_origin.y) && same(a.lighting_origin.z,e.lighting_origin.z) &&
          same(a.lighting_velocity.x,e.lighting_velocity.x) && same(a.lighting_velocity.y,e.lighting_velocity.y) && same(a.lighting_velocity.z,e.lighting_velocity.z),
          "Vehicle lighting inputs changed in actual UDP transport");
    }
    report["udp_vehicle_owners"]=actual.world_vehicles.size();report["udp_lighting_inputs_bit_exact"]=true;
    std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  }
  std::cout<<"PASS live source vehicle lighting in "<<options.local_players<<" views; original framebuffer comparison remains required\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
