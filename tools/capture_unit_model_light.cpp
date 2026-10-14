#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/scenery.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {void check(bool ok,const char* why){if(!ok)throw FormatError(why);}}
int main(int argc,char** argv)try {
  check(argc==8,"Supply MAP SHARED_MAPS TU15_XBE WIDTH HEIGHT FRESH_OUTPUT PINNED_SHADER_DIRECTORY");
  const auto original=OriginalData::load_xbe(argv[3]);
  check(original->image==OriginalData::Image::TitleUpdate15,"Unit light capture requires OG Xbox TU1.5");
  const Cache cache(argv[1],argv[2],original);const auto content=cache.load_content(0,true);
  const auto out=std::filesystem::absolute(argv[6]);
  check(!std::filesystem::exists(out),"Unit capture output already exists");
  std::filesystem::create_directories(out);
  const CollisionWorld terrain(content.collision);
  // The authored Lockout module anchors a declared diagnostic spawn search.
  // Floors and clear sightlines come from the original BSP. The native spawn
  // selector chooses which participant receives each entry in this pool.
  const ScenarioObject* anchor=nullptr;
  for(const auto& object:content.objects)if(object.type==11 && scenery_automatic_creation_allowed(object,2)) {
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && tag->name=="scenarios\\objects\\multi\\lockout\\power_module\\power_module") {anchor=&object;break;}
  }
  check(anchor,"Unit capture needs the authored Lockout module placement");
  const auto basis=authored_basis(anchor->rotation);
  const auto floor_position=[&](Vec3 p)->std::optional<Vec3> {
    const auto floor=terrain.raycast(p+Vec3{0,0,1},{0,0,-1},16);
    if(!floor)return {};
    p.z=floor->position.z+content.player.radius;return p;
  };
  const auto a=floor_position(anchor->position+basis.forward*4.f+basis.left*2.f);
  const auto b=floor_position(anchor->position+basis.forward*1.f+basis.left*2.f);
  check(a && b,"Declared unit capture spawn pool has no original BSP floor");
  check(length(*a-*b)>2.f,"Declared unit spawn entries overlap");
  const auto eye_a=*a+Vec3{0,0,content.player.camera_height};
  const auto eye_b=*b+Vec3{0,0,content.player.camera_height};
  const auto delta=eye_b-eye_a;
  check(!terrain.raycast(eye_a,normalized(delta),length(delta)-.05f),"Declared unit spawn pool lacks a clear BSP sightline");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=std::stoul(argv[4]);options.height=std::stoul(argv[5]);options.local_players=1;
  options.capture_shader_directory=std::filesystem::absolute(argv[7]);
  options.source_object_world=true;options.source_object_savegame_directory=out/"savegame";
  options.hidden=true;options.validation=true;options.mute=true;options.starting_weapon=5;
  options.capture_tick=200;options.reference_tick_cadence=true;options.reference_render_fraction=0;
  options.reference_auxiliary_seed=0x12345678u;
  Spawn spawn_a,spawn_b;spawn_a.position=*a;spawn_b.position=*b;spawn_a.bsp=spawn_b.bsp=0;
  spawn_a.facing=std::atan2(delta.y,delta.x);spawn_b.facing=spawn_a.facing+3.14159265358979323846f;
  options.native_spawn_inputs={spawn_a,spawn_b};
  options.input_script=out/"inputs.txt";options.screenshot=out/"unit.ppm";
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto target_slot=options.prepared_world->match.join("Source light receiver");
  const auto initial=options.prepared_world->match.state();
  const auto camera_slot=options.prepared_world->local_slots.at(0);
  const auto& camera_player=initial.players.at(camera_slot);
  const auto& target_player=initial.players.at(target_slot);
  check(camera_player.source_unit!=null_datum && target_player.source_unit!=null_datum,
      "Native participant construction did not publish both real biped owners");
  check(length(camera_player.position-target_player.position)>2.f,"Native selector placed both participants together");
  const auto aim=normalized(target_player.position+Vec3{0,0,content.player.camera_height*.55f}-
      (camera_player.position+Vec3{0,0,content.player.camera_height}));
  const auto yaw=std::atan2(aim.y,aim.x),pitch=std::atan2(aim.z,std::hypot(aim.x,aim.y));
  {std::ofstream f(options.input_script);f.precision(9);
    for(unsigned tick=0;tick<options.capture_tick;++tick)
      f<<tick<<" 0 0 0 "<<(tick==20?1:0)<<' '<<yaw<<' '<<pitch<<'\n';}
  const std::set<std::uint32_t> owners{target_player.source_unit,target_player.source_weapon};
  Json report={{"authority","OG Xbox TU1.5"},
    {"boundary","Actual native biped/held-weapon base/light join and grenade delivery. Declared offline spawn pool; no substituted poses, lights or original-frame parity claim."},
    {"source_owners",owners},{"camera_slot",camera_slot},{"target_slot",target_slot},
    {"pinned_shader_directory",options.capture_shader_directory.string()},
    {"initial_players",Json::array()},{"draws",Json::array()}};
  for(const auto slot:{camera_slot,target_slot}) {const auto& p=initial.players[slot];
    report["initial_players"].push_back({{"slot",slot},{"unit",p.source_unit},{"weapon",p.source_weapon},
      {"position",{p.position.x,p.position.y,p.position.z}}});}
  report["spawn_pool"]=Json::array();for(const auto& p:options.native_spawn_inputs)
    report["spawn_pool"].push_back({{"position",{p.position.x,p.position.y,p.position.z}},{"facing",p.facing},{"bsp",p.bsp}});
  std::map<std::uint64_t,NativeGrenadeDetonation> events;
  std::uint32_t source_tick{},light_tick{};unsigned light_packets{},draws{},lit{},visible{};
  Json current_inputs=Json::array();std::vector<Vertex> geometry;
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state) {
    source_tick=state.tick;current_inputs=Json::array();
    for(const auto& event:state.grenade_detonations)events.emplace(event.serial,event);
    auto& source=*options.prepared_world->source_objects;
    for(const auto owner:owners) {
      const auto nodes=source.world_nodes(owner);const auto sphere=source.object_sphere(owner);
      Json bank=Json::array();static_assert(sizeof(Transform)==52);
      for(const auto& node:nodes)bank.push_back(std::bit_cast<std::array<std::uint32_t,13>>(node));
      current_inputs.push_back({{"object",owner},{"nodes",bank},
        {"sphere",{sphere.center.x,sphere.center.y,sphere.center.z,sphere.radius}},
        {"velocity",std::bit_cast<std::array<std::uint32_t,3>>(source.object_linear_velocity(owner))}});
    }
  };
  options.effect_light_capture_request=[&](unsigned view,std::uint32_t tick,
      std::span<const EffectLightRawDraw> packets,std::span<const WorldEffectLight> lights) {
    if(view || light_tick || events.empty())return false;
    std::map<std::uint32_t,std::set<std::uint32_t>> selected;
    for(const auto& packet:packets) {const auto& owner=packet.source.owner();
      if(owner.kind==MenuWorldSourceOwnerKind::dynamic_object && owner.generation && owners.contains(*owner.generation))
        selected[packet.light_identity].insert(*owner.generation);}
    if(selected.empty())return false;
    check(source_tick==tick,"Unit light capture lost its completed source frame");
    report["model_light_sources"]=Json::array();
    for(const auto& [identity,receivers]:selected) {
      const auto at=std::find_if(lights.begin(),lights.end(),[&](const auto& light){return light.identity==identity;});
      check(at!=lights.end(),"Unit light packet lost its original occupied light identity");
      Json matches=Json::array();for(const auto& [serial,event]:events)
        if(event.projectile==at->effect_owner && event.effect==at->effect_definition)
          matches.push_back({{"serial",serial},{"tick",event.tick},{"projectile",event.projectile},{"effect",event.effect}});
      check(!matches.empty(),"Unit illumination does not belong to actual grenade effect delivery");
      unsigned count{};for(const auto& packet:packets)if(packet.light_identity==identity &&
          packet.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object &&
          packet.source.owner().generation && owners.contains(*packet.source.owner().generation))++count;
      light_packets+=count;
      report["model_light_sources"].push_back({{"identity",identity},{"receivers",receivers},
        {"definition",at->definition},{"effect_owner",at->effect_owner},{"effect_definition",at->effect_definition},
        {"delivery_tick",at->tick},{"model_packets",count},{"original_effect_matches",matches}});
    }
    light_tick=tick;options.capture_tick=tick;report["source_inputs"]=current_inputs;return true;
  };
  options.unit_source_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,
      const ObjectLightingState& light,bool valid,unsigned lod,std::span<const Vertex> vertices) {
    if(!owners.contains(owner))return;check(view==0,"Single-view unit capture received another viewport");
    ++draws;lit+=valid;geometry.insert(geometry.end(),vertices.begin(),vertices.end());
    report["draws"].push_back({{"tick",source_tick},{"object",owner},{"definition",definition},
      {"geometry_lod",lod},{"lighting_valid",valid},{"lighting",light.source_values()},{"vertices",vertices.size()}});
    if(light_tick && source_tick==light_tick) {
      std::ofstream file(out/("base-"+std::to_string(owner)+".bin"),std::ios::binary);
      file.write(reinterpret_cast<const char*>(vertices.data()),vertices.size_bytes());
      check(bool(file),"Could not retain the actual captured unit base vertices");
    }
  };
  options.view_observer=[&](unsigned view,const Camera& camera,float horizontal_fov,std::int16_t) {
    check(!view,"Single-view unit capture received another camera");visible=0;
    const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
    const auto right=normalized(cross(forward,camera.oriented?camera.up:Vec3{0,0,1})),up=cross(right,forward);
    const float horizontal=std::tan(horizontal_fov*.5f),aspect=float(options.width)/options.height;
    for(std::size_t n=0;n+2<geometry.size();n+=3) {
      const auto center=(geometry[n].position+geometry[n+1].position+geometry[n+2].position)/3.f;
      const auto delta=center-camera.position;const auto ahead=dot(delta,forward);
      if(ahead<=0 || std::abs(dot(delta,right))>ahead*horizontal || std::abs(dot(delta,up))>ahead*horizontal/aspect)continue;
      const auto distance=length(delta);if(distance>.01f && !terrain.raycast(camera.position,delta/distance,distance-.01f))++visible;
    }
    geometry.clear();report["final_view_geometry"]={{"eye",{camera.position.x,camera.position.y,camera.position.z}},
      {"unoccluded_triangle_centers",visible}};
  };
  const auto result=run_multiplayer(options);
  report["runtime_exit"]=result;report["model_light_tick"]=light_tick;report["model_light_packets"]=light_packets;
  report["draw_counts"]=draws;report["lit_draws"]=lit;
  std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  check(result==0 && std::filesystem::exists(options.screenshot),"Live unit capture did not complete");
  check(light_tick && light_packets,"Actual grenade produced no joined unit light packets");
  check(draws && lit && visible,"Capture lacks visible joined unit geometry and valid lighting");
  std::cout<<"PASS actual native biped/held-weapon base and grenade model light; original-frame parity remains separate\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
