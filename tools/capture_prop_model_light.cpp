#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/scenery.hpp"
#include "halo2/math.hpp"
#include "halo2/source_native_light_snapshot_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <bit>
#include <algorithm>
#include <limits>
using namespace halo2;
using Json=nlohmann::json;
namespace {void check(bool ok,const char* why){if(!ok)throw FormatError(why);}}
int main(int argc,char**argv)try {
  check(argc>=8 && argc<=10,"Supply MAP SHARED_MAPS TU15_XBE WIDTH HEIGHT VIEWS FRESH_OUTPUT [PROP_TAG_NAME [PINNED_SHADER_DIRECTORY]]");
  const auto original=OriginalData::load_xbe(argv[3]);
  check(original->image==OriginalData::Image::TitleUpdate15,"Prop light capture requires OG Xbox TU1.5");
  const Cache cache(argv[1],argv[2],original);const auto content=cache.load_content(0,true);
  const bool rocket=std::getenv("HALO2_CAPTURE_MODEL_ROCKET")!=nullptr;
  if(rocket)check(cache.is_mcc(),"Large-sphere runtime capture requires a prepared MCC cache");
  const std::string name=argc>=9?argv[8]:"scenarios\\objects\\multi\\lockout\\power_module\\power_module";
  const ScenarioObject* prop=nullptr;
  for(const auto& object:content.objects)if(object.type==11 && scenery_automatic_creation_allowed(object,2)) {
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && tag->name==name){prop=&object;break;}
  }
  check(prop,"Requested authored automatic BLOC placement is absent");
  if(rocket) {
    const auto object=cache.load_source_tag_metadata(prop->definition);const Bytes obje(object.bytes_at(object.source_address(),0x3c));
    const auto hlmt=cache.load_source_tag_metadata(obje.u32(0x38));const Bytes model(hlmt.bytes_at(hlmt.source_address(),0x10));
    check(model.u32(4)!=0xffffffffu&&model.u32(12)!=0xffffffffu,"Rocket target lacks its original render/collision models");
    const auto mode=cache.load_source_tag_metadata(model.u32(4));const Bytes root(mode.bytes_at(mode.source_address(),0x80));
    check(std::bit_cast<std::int32_t>(root.u32(0x74))>0&&std::bit_cast<std::int32_t>(root.u32(0x7c))>0,
        "Rocket target is not a C19D0 PRT/render-leaf object");
  }
  const auto placement=std::size_t(prop-content.objects.data());
  const auto out=std::filesystem::absolute(argv[7]);check(!std::filesystem::exists(out),"Prop capture output already exists");
  std::filesystem::create_directories(out);
  const CollisionWorld terrain(content.collision);const auto basis=authored_basis(prop->rotation);
  const auto choose_position=[&](Vec3 origin,Vec3 target) {
    for(float distance:{4.f,6.f,2.f,1.f})for(float side:{2.f,0.f,-2.f}) {
      auto candidate=origin+basis.forward*distance+basis.left*side;
      const auto floor=terrain.raycast(candidate+Vec3{0,0,1},{0,0,-1},16);if(!floor)continue;
      candidate.z=floor->position.z+content.player.radius;
      const auto eye=candidate+Vec3{0,0,content.player.camera_height},delta=target-eye;
      if(terrain.raycast(eye,normalized(delta),std::max(0.f,length(delta)-.05f)))continue;
      return candidate;
    }
    throw FormatError("No declared prop camera has an original BSP floor and clear sightline");
  };
  auto target=prop->position+Vec3{0,0,.6f};auto position=choose_position(prop->position,target);
  auto eye=position+Vec3{0,0,content.player.camera_height},look=normalized(target-eye);
  auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
  const bool grenade=std::getenv("HALO2_CAPTURE_MODEL_GRENADE")!=nullptr;
  check(!rocket||!grenade,"Select one actual firing or grenade workload");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=std::stoul(argv[4]);options.height=std::stoul(argv[5]);options.local_players=std::stoul(argv[6]);
  if(argc==10)options.capture_shader_directory=std::filesystem::absolute(argv[9]);
  options.source_object_world=true;options.source_object_savegame_directory=out/"savegame";
  options.hidden=true;options.validation=true;options.mute=true;options.starting_weapon=rocket?8:5;
  options.capture_tick=grenade||rocket?200:8;options.reference_tick_cadence=true;options.reference_render_fraction=0;
  if(rocket)if(const auto* limit=std::getenv("HALO2_CAPTURE_MODEL_LIMIT")) {
    options.capture_tick=std::stoul(limit);check(options.capture_tick>=40&&options.capture_tick<=200,"Rocket capture limit requires40..200 ticks");
  }
  if(rocket)check(options.local_players==1,"Large-sphere prop capture currently requires one actual view");
  options.reference_auxiliary_seed=0x12345678u;
  Spawn spawn;spawn.position=position;spawn.facing=yaw;spawn.bsp=0;
  if(!rocket)options.native_spawn_inputs={spawn};
  options.input_script=out/"inputs.txt";options.screenshot=out/"prop.ppm";
  // Declared-spawn acceptance uses the existing offline activation policy.
  // Keep physical views unbound until the real player update owns birth and
  // completes its first-person temporal phase; eager birth has no packet to
  // reinstall when the runtime binds its presentation views.
  options.defer_initial_player_spawn=!rocket;
  std::vector<std::uint32_t> players;
  if(!rocket)options.player_birth_owner=[&](MultiplayerPlayer& player,unsigned) {
    players.push_back(player.source_datum);player.source_flags=8;
  };
  if(!rocket)options.pre_player_update_owner=[&](MultiplayerMatch& match) {
    for(const auto datum:players) {
      const auto* player=match.player_from_datum(datum);check(player,"Prop capture lost its real player owner");
      if(!(player->source_flags&1u))match.apply_source_player_activation(datum,player->source_flags,player->source_flags|1u);
    }
  };
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  Json report={{"authority","OG Xbox TU1.5"},{"boundary","Native live BLOC base/light handoff. Declared camera/spawn and actual grenade input; no substituted light or original-frame parity claim."},
    {"name",name},{"definition",prop->definition},{"placement",placement},{"draws",Json::array()}};
  report["pinned_shader_directory"]=options.capture_shader_directory.string();
  report["deferred_native_birth"]=options.defer_initial_player_spawn;
  report["original_engine_rules"]=options.prepared_world->rules.requires_source_engine;
  float raw_yaw=yaw,raw_pitch=pitch;
  if(rocket) {
    check(options.prepared_world->rules.requires_source_engine,"Rocket capture requires the original multiplayer engine");
    auto& world=*options.prepared_world;auto& source=*world.source_objects;
    std::optional<std::uint32_t> selected;
    for(const auto& object:world.match.simulation().scene().objects())if(object.placement==placement) {
      check(!selected,"Rocket target has multiple source placement owners");selected=object.handle;
    }
    check(selected.has_value(),"Rocket target lacks its actual source placement owner");
    check(source.object_definition(*selected)==prop->definition,"Rocket placement identity names a different native definition");
    const auto sphere=source.object_sphere(*selected);
    const auto surface=terrain.raycast(sphere.center+Vec3{0,0,1},{0,0,-1},32);
    check(surface.has_value(),"Rocket target has no actual BSP surface below its source sphere");
    target=surface->position;
    // Use authored player positions so the collision controller does not
    // have to depenetrate a terrain-only candidate inside the moving prop.
    //12 units is a declared test standoff, not a recovered gameplay constant.
    std::optional<std::size_t> chosen;float distance=std::numeric_limits<float>::infinity();
    const auto authored_spawns=cache.spawns();
    for(std::size_t i=0;i<authored_spawns.size();++i) {
      const auto& candidate=authored_spawns[i];if(candidate.bsp!=0)continue;
      const auto candidate_eye=candidate.position+Vec3{0,0,content.player.camera_height};
      const auto delta=target-candidate_eye;const auto length_to_target=length(delta);
      if(length_to_target<12.f||length_to_target>=distance)continue;
      if(terrain.raycast(candidate_eye,delta/length_to_target,length_to_target-.05f))continue;
      chosen=i;distance=length_to_target;
    }
    check(chosen.has_value(),"No authored player position has the declared standoff and clear target sightline");
    position=authored_spawns[*chosen].position;
    const auto state=world.match.state();const auto& player=state.players.at(world.local_slots.at(0));
    check(player.source_unit!=null_datum,"Original engine has not completed the player's real birth");
    const auto before=source.biped_motion(player.source_unit);
    source.tu15_world().set_object_position(player.source_unit,position); // Whole original B95D0 input owner.
    const auto after=source.biped_motion(player.source_unit);position=after.position;
    eye=position+Vec3{0,0,content.player.camera_height};look=normalized(target-eye);
    yaw=std::atan2(look.y,look.x);pitch=std::atan2(look.z,std::hypot(look.x,look.y));
    //18A0C0 owns the raw sample baseline. Start with zero samples, then
    // supply one real PC look delta after birth rather than rebasing it to
    // the desired processed angle on every frame.
    raw_yaw=std::remainder(yaw-player.yaw,6.283185307f);raw_pitch=pitch-player.pitch;
    report["declared_position_input"]={{"owner",0xb95d0u},{"unit",player.source_unit},
      {"authored_spawn",*chosen},{"declared_minimum_standoff",12.f},
      {"before",{before.position.x,before.position.y,before.position.z}},
      {"after",{after.position.x,after.position.y,after.position.z}},
      {"target",{target.x,target.y,target.z}},{"desired_look",{yaw,pitch}},
      {"source_sphere",{sphere.center.x,sphere.center.y,sphere.center.z,sphere.radius}},
      {"initial_look",{player.yaw,player.pitch}},{"raw_look_delta",{raw_yaw,raw_pitch}}};
    report["boundary"]="Original multiplayer engine, actual player birth and whole B95D0 position input near a real PRT owner. Real PC look delta and rocket firing; capture only at nonzero diffuse model packets from a source light with radius>=3. No light, visibility, pose or cull outcome is supplied.";
  }
  {std::ofstream f(options.input_script);f.precision(9);
    for(unsigned tick=0;tick<options.capture_tick;++tick)for(unsigned view=0;view<options.local_players;++view)
      f<<tick<<' '<<view<<' '<<(rocket&&tick>=20&&tick<100?1:0)<<" 0 "<<(grenade && tick==20 && view==0?1:0)<<' '
       <<(rocket&&tick<10?0.f:raw_yaw)<<' '<<(rocket&&tick<10?0.f:raw_pitch)<<'\n';}
  std::set<std::uint32_t> owners;std::uint32_t source_tick{},light_tick{};unsigned light_packets{};
  unsigned firing_frames{};
  const auto light_clusters=rocket?cache.load_object_light_spatial_bsp(0).clusters.size():0u;
  Json current_inputs=Json::array();std::map<std::uint64_t,NativeGrenadeDetonation> events;
  std::vector<unsigned> drawn(options.local_players),lit(options.local_players),visible(options.local_players),
      grenade_packets(options.local_players),prt_draws(options.local_players);
  std::vector<std::vector<Vertex>> geometry(options.local_players);
  const auto tag_name=[&](std::uint32_t id){const auto tag=cache.query_tag({.handle=TagHandle{id}});return tag?tag->name:std::string{};};
  options.input_observer=[&](unsigned,std::span<const Controls> controls,const MultiplayerState& state) {
    source_tick=state.tick;current_inputs=Json::array();
    if(rocket&&(state.tick<=2||state.tick==10||state.tick==20||state.tick==30||state.tick==40||state.tick==60)) {
      const auto& player=state.players.at(options.prepared_world->local_slots.at(0));
      report["player_trajectory"].push_back({{"tick",state.tick},{"position",{player.position.x,player.position.y,player.position.z}},
        {"look",{player.yaw,player.pitch}},{"dead",player.dead}});
    }
    firing_frames+=std::any_of(controls.begin(),controls.end(),[](const auto& input){return input.fire;});
    for(const auto& event:state.grenade_detonations)events.emplace(event.serial,event);
    for(const auto& object:options.prepared_world->match.simulation().scene().objects())if(object.placement==placement) {
      owners.insert(object.handle);auto& source=*options.prepared_world->source_objects;
      const auto nodes=source.world_nodes(object.handle);const auto sphere=source.object_sphere(object.handle);
      Json bank=Json::array();static_assert(sizeof(Transform)==52);
      for(const auto& node:nodes)bank.push_back(std::bit_cast<std::array<std::uint32_t,13>>(node));
      current_inputs.push_back({{"object",object.handle},{"nodes",bank},{"sphere",{sphere.center.x,sphere.center.y,sphere.center.z,sphere.radius}},
        {"velocity",std::bit_cast<std::array<std::uint32_t,3>>(source.object_linear_velocity(object.handle))}});
    }
    if(state.tick==options.capture_tick)report["source_inputs"]=current_inputs;
  };
  options.effect_light_capture_request=[&](unsigned view,std::uint32_t tick,std::span<const EffectLightRawDraw> packets,std::span<const WorldEffectLight> lights) {
    if(rocket) {
      if(view||light_tick||source_tick<=20||!firing_frames)return false;
      check(source_tick==tick,"Large-sphere capture lost its completed input frame");
      const auto snapshot=source_native_light_snapshot_tu15(options.prepared_world->source_objects->memory(),light_clusters);
      std::map<std::uint32_t,unsigned> selected;
      std::map<std::uint32_t,ShaderVector> diffuse_constants;
      for(const auto& packet:packets) {
        const auto& owner=packet.source.owner();
        if(owner.kind==MenuWorldSourceOwnerKind::dynamic_object&&owner.generation&&owners.contains(*owner.generation)) {
          const auto& color=packet.source.constants().at(37); // Completed sphere diffuse c37, including selected fade.
          if(std::any_of(color.begin(),color.begin()+3,[](float v){return std::isfinite(v)&&v>0.f;})) {
            ++selected[packet.light_identity];diffuse_constants.try_emplace(packet.light_identity,color);
          }
        }
      }
      Json accepted=Json::array();unsigned count{};
      for(const auto& [identity,n]:selected) {
        if(!snapshot.lights.contains(identity))continue;
        const auto& light=snapshot.lights.record(identity);
        const auto name=tag_name(light[1]);const auto radius=std::bit_cast<float>(light[0x24/4]);
        if(!(radius>=3.f)||name.find("rocket")==std::string::npos)continue;
        accepted.push_back({{"identity",identity},{"definition",light[1]},{"light_tag",name},
          {"spatial_radius",radius},{"model_packets",n},{"diffuse_vertex_c37",diffuse_constants.at(identity)}});count+=n;
      }
      if(!count)return false;
      light_tick=tick;light_packets=count;grenade_packets.at(view)=count;options.capture_tick=tick;
      report["model_light_sources"]=std::move(accepted);report["source_inputs"]=current_inputs;return true;
    }
    if(grenade && light_tick && tick==light_tick) {
      check(source_tick==tick,"Local-view prop light capture lost its completed input frame");
      for(const auto& packet:packets) {
        const auto& owner=packet.source.owner();
        if(owner.kind!=MenuWorldSourceOwnerKind::dynamic_object || !owner.generation ||
           !owners.contains(*owner.generation))continue;
        for(const auto& selected:report.at("model_light_sources"))
          if(selected.at("identity").get<std::uint32_t>()==packet.light_identity)++grenade_packets.at(view);
      }
      return false;
    }
    if(!grenade || view || light_tick || events.empty())return false;
    std::map<std::uint32_t,unsigned> selected;
    for(const auto& packet:packets) {
      const auto& owner=packet.source.owner();
      if(owner.kind==MenuWorldSourceOwnerKind::dynamic_object && owner.generation && owners.contains(*owner.generation))++selected[packet.light_identity];
    }
    if(selected.empty())return false;
    check(source_tick==tick,"Prop light capture lost its actual completed input frame");
    report["model_light_sources"]=Json::array();
    for(const auto& [id,count]:selected) {
      const auto light=std::find_if(lights.begin(),lights.end(),[&](const auto& l){return l.identity==id;});
      check(light!=lights.end(),"Prop light packet lost its occupied source light identity");
      Json matches=Json::array();for(const auto& [serial,event]:events)
        if(event.projectile==light->effect_owner && event.effect==light->effect_definition)
          matches.push_back({{"serial",serial},{"tick",event.tick},{"projectile",event.projectile},{"effect",event.effect}});
      check(!matches.empty(),"Selected prop illumination is not the actual grenade effect delivery");
      report["model_light_sources"].push_back({{"identity",id},{"definition",light->definition},{"light_tag",tag_name(light->definition)},
        {"effect_owner",light->effect_owner},{"effect_definition",light->effect_definition},{"effect_tag",tag_name(light->effect_definition)},
        {"delivery_tick",light->tick},{"model_packets",count},{"original_effect_matches",matches}});light_packets+=count;
    }
    grenade_packets.at(view)=light_packets;
    light_tick=tick;options.capture_tick=tick;report["source_inputs"]=current_inputs;return true;
  };
  options.prop_source_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,const ObjectLightingState& light,bool valid,unsigned lod,std::span<const Vertex> vertices) {
    if(!owners.contains(owner))return;check(definition==prop->definition,"Prop source base changed its authored definition");
    ++drawn.at(view);lit.at(view)+=valid;geometry.at(view).insert(geometry.at(view).end(),vertices.begin(),vertices.end());
    report["draws"].push_back({{"tick",source_tick},{"view",view},{"object",owner},{"geometry_lod",lod},{"lighting_valid",valid},
      {"lighting",light.source_values()},{"vertices",vertices.size()}});
  };
  if(rocket)options.dynamic_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,std::span<const Vertex> vertices) {
    if(!owners.contains(owner))return;check(definition==prop->definition,"PRT base submission changed its actual definition");
    ++drawn.at(view);++prt_draws.at(view);geometry.at(view).insert(geometry.at(view).end(),vertices.begin(),vertices.end());
    report["draws"].push_back({{"tick",source_tick},{"view",view},{"object",owner},
      {"route","actual dynamic-object submission; PRT target verified by metadata"},{"vertices",vertices.size()}});
  };
  options.view_observer=[&](unsigned view,const Camera& camera,float horizontal_fov,std::int16_t) {
    visible.at(view)=0;const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
    const auto right=normalized(cross(forward,camera.oriented?camera.up:Vec3{0,0,1})),up=cross(right,forward);
    const float horizontal=std::tan(horizontal_fov*.5f),aspect=float(options.width)/options.height*(options.local_players==2?2.f:1.f);
    const auto& vertices=geometry.at(view);
    for(std::size_t n=0;n+2<vertices.size();n+=3) {
      const auto center=(vertices[n].position+vertices[n+1].position+vertices[n+2].position)/3.f;
      const auto delta=center-camera.position;const auto ahead=dot(delta,forward);
      if(ahead<=0 || std::abs(dot(delta,right))>ahead*horizontal || std::abs(dot(delta,up))>ahead*horizontal/aspect)continue;
      const auto distance=length(delta);if(distance>.01f && !terrain.raycast(camera.position,delta/distance,distance-.01f))++visible.at(view);
    }
    geometry.at(view).clear();report["final_view_geometry"][std::to_string(view)]={{"eye",{camera.position.x,camera.position.y,camera.position.z}},
      {"forward",{forward.x,forward.y,forward.z}},
      {"unoccluded_triangle_centers",visible.at(view)}};
  };
  const auto result=run_multiplayer(options);report["runtime_exit"]=result;report["source_owners"]=owners;
  report["draw_counts"]=drawn;report["lit_draws"]=lit;report["model_light_tick"]=light_tick;report["model_light_packets"]=light_packets;
  report["prt_base_submissions"]=prt_draws;
  report["grenade_model_packets_by_view"]=grenade_packets;
  report["rocket_workload"]=rocket;report["firing_input_frames"]=firing_frames;
  report["input_script"]=options.input_script.string();report["actual_capture_tick"]=options.capture_tick;
  std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  check(result==0 && !owners.empty() && std::filesystem::exists(options.screenshot),"Live prop capture did not complete");
  check(!grenade || (light_tick && light_packets),"Actual grenade produced no source light packets on the selected BLOC owner");
  check(!rocket || (light_tick&&light_packets&&firing_frames),"Actual rocket produced no large-sphere packets on the PRT owner");
  for(unsigned view=0;view<options.local_players;++view)check(drawn[view] && (rocket?prt_draws[view]:lit[view]) && visible[view],
      "A local view lacks visible base geometry from its actual ordinary/PRT submission owner");
  if(grenade)for(const auto count:grenade_packets)check(count,"A local view lacks model packets from the actual selected grenade effect");
  std::cout<<"PASS visible live source BLOC base/light handoff; matched original pixels remain separate\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
