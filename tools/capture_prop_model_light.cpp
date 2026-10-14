#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/scenery.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <bit>
using namespace halo2;
using Json=nlohmann::json;
namespace {void check(bool ok,const char* why){if(!ok)throw FormatError(why);}}
int main(int argc,char**argv)try {
  check(argc>=8 && argc<=10,"Supply MAP SHARED_MAPS TU15_XBE WIDTH HEIGHT VIEWS FRESH_OUTPUT [PROP_TAG_NAME [PINNED_SHADER_DIRECTORY]]");
  const auto original=OriginalData::load_xbe(argv[3]);
  check(original->image==OriginalData::Image::TitleUpdate15,"Prop light capture requires OG Xbox TU1.5");
  const Cache cache(argv[1],argv[2],original);const auto content=cache.load_content(0,true);
  const std::string name=argc>=9?argv[8]:"scenarios\\objects\\multi\\lockout\\power_module\\power_module";
  const ScenarioObject* prop=nullptr;
  for(const auto& object:content.objects)if(object.type==11 && scenery_automatic_creation_allowed(object,2)) {
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && tag->name==name){prop=&object;break;}
  }
  check(prop,"Requested authored automatic BLOC placement is absent");
  const auto placement=std::size_t(prop-content.objects.data());
  const auto out=std::filesystem::absolute(argv[7]);check(!std::filesystem::exists(out),"Prop capture output already exists");
  std::filesystem::create_directories(out);
  const CollisionWorld terrain(content.collision);const auto basis=authored_basis(prop->rotation);
  const auto target=prop->position+Vec3{0,0,.6f};Vec3 position{};bool admitted=false;
  for(float distance:{4.f,6.f,2.f,1.f}) {
    for(float side:{2.f,0.f,-2.f}) {
      auto candidate=prop->position+basis.forward*distance+basis.left*side;
      const auto floor=terrain.raycast(candidate+Vec3{0,0,1},{0,0,-1},16);if(!floor)continue;
      candidate.z=floor->position.z+content.player.radius;
      const auto eye=candidate+Vec3{0,0,content.player.camera_height},delta=target-eye;
      if(terrain.raycast(eye,normalized(delta),std::max(0.f,length(delta)-.05f)))continue;
      position=candidate;admitted=true;break;
    }
    if(admitted)break;
  }
  check(admitted,"No declared prop camera has an original BSP floor and clear sightline");
  const auto eye=position+Vec3{0,0,content.player.camera_height},look=normalized(target-eye);
  const auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
  const bool grenade=std::getenv("HALO2_CAPTURE_MODEL_GRENADE")!=nullptr;
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=std::stoul(argv[4]);options.height=std::stoul(argv[5]);options.local_players=std::stoul(argv[6]);
  if(argc==10)options.capture_shader_directory=std::filesystem::absolute(argv[9]);
  options.source_object_world=true;options.source_object_savegame_directory=out/"savegame";
  options.hidden=true;options.validation=true;options.mute=true;options.starting_weapon=5;
  options.capture_tick=grenade?200:8;options.reference_tick_cadence=true;options.reference_render_fraction=0;
  options.reference_auxiliary_seed=0x12345678u;
  Spawn spawn;spawn.position=position;spawn.facing=yaw;spawn.bsp=0;options.native_spawn_inputs={spawn};
  options.input_script=out/"inputs.txt";options.screenshot=out/"prop.ppm";
  {std::ofstream f(options.input_script);f.precision(9);
    for(unsigned tick=0;tick<options.capture_tick;++tick)for(unsigned view=0;view<options.local_players;++view)
      f<<tick<<' '<<view<<" 0 0 "<<(grenade && tick==20 && view==0?1:0)<<' '<<yaw<<' '<<pitch<<'\n';}
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  Json report={{"authority","OG Xbox TU1.5"},{"boundary","Native live BLOC base/light handoff. Declared camera/spawn and actual grenade input; no substituted light or original-frame parity claim."},
    {"name",name},{"definition",prop->definition},{"placement",placement},{"draws",Json::array()}};
  report["pinned_shader_directory"]=options.capture_shader_directory.string();
  std::set<std::uint32_t> owners;std::uint32_t source_tick{},light_tick{};unsigned light_packets{};
  Json current_inputs=Json::array();std::map<std::uint64_t,NativeGrenadeDetonation> events;
  std::vector<unsigned> drawn(options.local_players),lit(options.local_players),visible(options.local_players),
      grenade_packets(options.local_players);
  std::vector<std::vector<Vertex>> geometry(options.local_players);
  const auto tag_name=[&](std::uint32_t id){const auto tag=cache.query_tag({.handle=TagHandle{id}});return tag?tag->name:std::string{};};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state) {
    source_tick=state.tick;current_inputs=Json::array();
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
      {"unoccluded_triangle_centers",visible.at(view)}};
  };
  const auto result=run_multiplayer(options);report["runtime_exit"]=result;report["source_owners"]=owners;
  report["draw_counts"]=drawn;report["lit_draws"]=lit;report["model_light_tick"]=light_tick;report["model_light_packets"]=light_packets;
  report["grenade_model_packets_by_view"]=grenade_packets;
  std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  check(result==0 && !owners.empty() && std::filesystem::exists(options.screenshot),"Live prop capture did not complete");
  check(!grenade || (light_tick && light_packets),"Actual grenade produced no source light packets on the selected BLOC owner");
  for(unsigned view=0;view<options.local_players;++view)check(drawn[view] && lit[view] && visible[view],"A local view lacks visible source prop base geometry and valid lighting");
  if(grenade)for(const auto count:grenade_packets)check(count,"A local view lacks model packets from the actual selected grenade effect");
  std::cout<<"PASS visible live source BLOC base/light handoff; matched original pixels remain separate\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
