// Diagnostic only. Build in an owned source snapshot through run_tests.py.
// Synthetic projectile inputs below test definition admission, never births.
#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_frustum_camera.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool ok,const char *why){if(!ok)throw FormatError(why);}
Json projectile_banks(const std::filesystem::path &map) {
  Cache cache(map,map.parent_path());
  require(cache.is_mcc(),"Prepared MCC content required");
  std::vector<std::uint32_t> definitions;
  for(const auto &tag:cache.tags())if(tag.group=="proj")definitions.push_back(tag.datum);
  const auto content=cache.load_bsp_content(0);CollisionWorld collision(content.collision);Mesh mesh;
  NativeWeaponProjectileScene scene(cache,0,mesh,collision,definitions,2048);
  const auto point=cache.spawns().front().position+Vec3{0,0,2};
  SourceObjectViewLod view;view.input=[](auto,auto,auto,auto){return SourceObjectLodInput{0,0x80000u,0};};
  SourceFrustumProjectionInput projection_input;projection_input.projection_angle=.8655884265899658f;
  projection_input.outer=projection_input.safe={0,0,480,640};
  const auto projection=source_frustum_projection(projection_input);
  const auto camera=source_view_projection({point-Vec3{3,0,0},{1,0,0},{0,0,1},
      .015625f,1024.f,{},projection.projection_program});
  Json rows=Json::array();unsigned frame=0;
  for(const auto &tag:cache.tags())if(tag.group=="proj") {
    Json row{{"definition",tag.datum},{"name",tag.name},{"declared_presentation_input",true}};
    try {
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
      row["model"]=root.u32(0x38);row["attachments"]=Json::array();
      const Bytes projectile(metadata.bytes_at(metadata.source_address(),metadata.size()));
      row["arming_seconds"]=projectile.f32(0xcc);
      row["timer_seconds"]={projectile.f32(0xd4),projectile.f32(0xd8)};
      const auto descriptor=read_tag_block_descriptor(root,0x94);
      if(descriptor.count) {
        const auto extent=descriptor.extent(0x18,4095);
        const auto owner=cache.query_tag({.contains=TagAddressRange{descriptor.address,extent},.require_address=true});
        require(bool(owner),"Attachment metadata has no owner");(void)cache.tag_payload(owner->handle());
        const Bytes bytes(cache.read_tag_bytes(descriptor.address,extent));
        for(unsigned i=0;i<unsigned(descriptor.count);++i) {
          const auto datum=bytes.u32(i*0x18+4);
          const auto attached=cache.query_tag({.handle=TagHandle{datum}});
          row["attachments"].push_back({{"group",bytes.u32(i*0x18)},
              {"name",attached?attached->name:"NONE"},{"datum",datum},
              {"marker",bytes.u32(i*0x18+8)},{"color",bytes.u16(i*0x18+0xc)},
              {"primary_function",bytes.u32(i*0x18+0x10)},
              {"secondary_function",bytes.u32(i*0x18+0x14)}});
        }
      }
      const std::array<NativeWeaponProjectile,1> live{{
          {0x12340001u,tag.datum,point,{10,0,0},{1,0,0},{0,0,1},1}}};
      scene.synchronize(live);
      std::size_t vertices=0,light_commands=0;
      for(const auto &draw:scene.draw_view(frame++,view))vertices+=draw.vertices.size();
      for(const auto &draw:scene.light_volume_view(0,{1,0,0},camera,{0,0,640,480},1024.f))
        light_commands+=draw.volume && !draw.volume->volumes.empty();
      row["model_vertices"]=vertices;row["light_volume_commands"]=light_commands;
    }catch(const std::exception &e){row["error"]=e.what();}
    rows.push_back(std::move(row));scene.synchronize({});
  }
  return {{"scope","definition admission only; no source births, GPU or original pixels"},{"rows",rows}};
}
Json zoom_draws(const std::filesystem::path &map,std::string_view name,unsigned presses,bool metadata_only=false) {
  Cache cache(map,map.parent_path());require(cache.is_mcc(),"Prepared MCC content required");
  const auto content=cache.load_content(0,true);
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();
  options.local_players=1;options.source_object_world=true;options.session_random_seed=0x105240u;
  if(const auto *players=std::getenv("HALO2_AUDIT_LOCAL_PLAYERS"))options.local_players=unsigned(std::stoul(players));
  bool selected=false;std::uint32_t selected_definition=null_datum;const auto choices=cache.multiplayer_weapon_choices();
  for(unsigned option=3;option<=20;++option) {
    std::uint32_t seed{};const auto id=source_starting_weapon(null_datum,option,null_datum,choices,seed);
    if(id!=null_datum && content.weapons.at(id).name.ends_with("\\"+std::string(name))) {
      options.starting_weapon=option;selected_definition=id;selected=true;break;
    }
  }
  require(selected,"Requested authored starting weapon absent");
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
  const auto player=world.local_slots.at(0);
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
  require(world.match.state().players[player].weapon==selected_definition,
      "Actual source starting equipment differs from the requested weapon");
  Json grenade_fields=Json::array();
  const auto &memory=world.source_objects->memory();
  const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
  for(const auto &tag:cache.tags())if(tag.group=="proj" &&
      (tag.name.ends_with("frag_grenade\\frag_grenade") || tag.name.ends_with("plasma_grenade\\plasma_grenade"))) {
    const auto metadata=word(word(0x4f0414u)+(tag.datum&65535u)*16u+8u);
    const auto real=[&](unsigned offset){return std::bit_cast<float>(word(metadata+offset));};
    grenade_fields.push_back({{"name",tag.name},{"source_metadata_address",metadata},
        {"arming_seconds",real(0xcc)},{"timer_seconds",{real(0xd4),real(0xd8)}}});
  }
  if(metadata_only)return {{"scope","Actual source-world metadata after creation and60 ordinary simulation ticks; no GPU or detonation measurement"},
      {"actual_source_weapon_definition",selected_definition},{"actual_source_world_grenade_metadata",grenade_fields}};
  options.desktop=std::make_shared<DesktopSession>();
  auto *window=options.desktop->window(640,480,false);SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
  // Declared PC presentation inputs, not a captured Xbox startup history.
  options.desktop->auxiliary_random()=0x2468ace0u;
  options.desktop->display_settings.field_of_view=78;
  options.desktop->keyboard_settings=PcKeyboardSettings{};
  options.frames=36;options.width=640;options.height=480;options.mute=true;
  if(const auto *capture=std::getenv("HALO2_COMBAT_AUDIT_CAPTURE"))options.screenshot=capture;
  options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
  Json input_samples=Json::array();
  options.input_observer=[&,window](unsigned step,std::span<const Controls> input,const auto &state) {
    input_samples.push_back({{"step",step},{"source_tick",state.tick},{"delivered_zoom",input.front().zoom},
        {"source_zoom",state.players[player].source_zoom.current}});
    if(step==0) {
      SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=SDL_GetWindowID(window);
      focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;require(SDL_PushEvent(&focus)==1,"Cannot queue owned window focus");
    }
    for(unsigned press=0;press<presses;++press)if(step==2+press*8 || step==3+press*8) {
      SDL_Event event{};event.type=step==2+press*8?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      event.button.button=SDL_BUTTON_RIGHT;event.button.windowID=SDL_GetWindowID(window);
      require(SDL_PushEvent(&event)==1,"Cannot queue actual zoom input");
    }
  };
  Json samples=Json::array(),views=Json::array();unsigned frames=0,scoped_draws=0,unscoped_draws=0;
  options.animation_observer=[&](unsigned,unsigned id,const AnimationRenderObservation &draw) {
    if(id!=player || !draw.first_person)return;
    const auto state=world.match.state();const auto selector=state.players[player].source_zoom.current;
    if(selector>=0 && !draw.vertices.empty())++scoped_draws;
    if(selector<0 && !draw.vertices.empty())++unscoped_draws;
    samples.push_back({{"tick",draw.tick},{"zoom",selector},{"vertices",draw.vertices.size()}});
  };
  options.view_observer=[&](unsigned view,const Camera &camera,float fov,std::int16_t selector) {
    if(!view)views.push_back({{"tick",world.match.state().tick},{"zoom",selector},
        {"source_horizontal_fov",fov},{"render_projection_degrees",camera.field_of_view}});
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame){frames+=frame.submitted;};
  const auto status=run_multiplayer(options);
  return {{"scope","actual SDL zoom input, source unit state and native first-person draw submission"},
      {"weapon",name},{"presses",presses},{"local_players",options.local_players},
      {"actual_source_weapon_definition",selected_definition},
      {"declared_cosmetic_seed",0x2468ace0u},{"declared_PC_FOV",78},{"runtime_status",status},{"frames",frames},
      {"scoped_first_person_draws",scoped_draws},{"unscoped_first_person_draws",unscoped_draws},
      {"final_zoom",world.match.state().players[player].source_zoom.current},{"samples",samples},{"views",views},
      {"input_samples",input_samples},
      {"actual_source_world_grenade_metadata",grenade_fields},
      {"scoped_mesh_submission_detected",scoped_draws>0},
      {"visual_symptom_verified",false}};
}
}
int main(int argc,char **argv)try {
  require(argc>=4,"Supply mode MCC_MAP REPORT [WEAPON PRESSES]");SDL_SetMainReady();
  const bool metadata=std::string_view(argv[1])=="metadata";
  require(std::string_view(argv[1])=="projectiles" || metadata || argc==6,"Zoom mode requires WEAPON PRESSES");
  const auto result=std::string_view(argv[1])=="projectiles"?projectile_banks(argv[2]):
      zoom_draws(argv[2],metadata?"sniper_rifle":argv[4],metadata?0u:unsigned(std::stoul(argv[5])),metadata);
  std::ofstream(argv[3])<<result.dump(2)<<'\n';std::cout<<result.dump()<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 2;}
