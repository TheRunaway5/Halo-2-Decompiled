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
#include "halo2/native_unit_interaction.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/particle_draw.hpp"
#include "../tests/helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool ok,const char *why){if(!ok)throw FormatError(why);}
Json banshee_setup(const std::filesystem::path &map) {
  Cache cache(map,map.parent_path());const auto content=cache.load_content(0,true);
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,placement]:assets.netgame_placements) {
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name.find("\\banshee\\")==std::string::npos)continue;
    const auto basis=authored_basis(object.rotation);
    const auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    const auto &definition=assets.units.at(object.definition);const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];if(!(seat.flags&4u))continue;
      const auto marker=definition.markers.find(seat.entry_marker);if(marker==definition.markers.end())continue;
      const auto entrances=pose_model_markers(marker->second,nodes,definition.permutations,{},1);if(entrances.empty())continue;
      auto point=entrances.front().world.position+entrances.front().world.basis[0]*(seat.radius*.5f);
      CollisionWorld terrain(content.collision);const auto floor=terrain.raycast(point+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;point.z=floor->position.z+content.player.radius;
      const auto look=entrances.front().world.position-(point+Vec3{0,0,content.player.camera_height});
      const auto airborne=cache.spawns().front().position+Vec3{0,0,12};
      return {{"declared_setup_only",true},{"position",{point.x,point.y,point.z}},
        {"yaw",std::atan2(look.y,look.x)},{"airborne",{airborne.x,airborne.y,airborne.z}},
        {"seat",index},{"definition",object.definition},{"unit_source_datum_offset",offsetof(Unit,source_datum)},
        {"unit_parent_offset",offsetof(Unit,source_parent)},{"unit_yaw_offset",offsetof(Unit,yaw)},
        {"controls_fire_offset",offsetof(Controls,fire)},{"controls_size",sizeof(Controls)}};
    }
  }
  throw FormatError("No actual authored Banshee entrance");
}
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
      if(root.u32(0x38)!=null_datum && tag.name.ends_with("needler_shard\\needler_shard")) {
        row["model_shader_externs"]=Json::array();
        for(const auto &material:cache.load_model(tag.datum).mesh.materials) {
          std::set<unsigned> pixel,vertex,state;
          for(const auto &pass:material.passes) {
            for(const auto &r:pass.pixel_externs)pixel.insert(r.source);
            for(const auto &r:pass.vertex_externs)vertex.insert(r.source);
            for(const auto &r:pass.state_externs)state.insert(r.source);
          }
          row["model_shader_externs"].push_back({{"name",material.name},{"template",material.template_name},
              {"pixel",pixel},{"vertex",vertex},{"state",state}});
        }
      }
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
Json zoom_draws(const std::filesystem::path &map,std::string_view name,unsigned presses,bool metadata_only=false,bool firing=false,bool throwing=false) {
  Cache cache(map,map.parent_path());require(cache.is_mcc(),"Prepared MCC content required");
  const auto content=cache.load_content(0,true);
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();
  options.local_players=1;options.source_object_world=true;options.session_random_seed=0x105240u;
  if(firing || throwing)options.local_players=2;
  const bool camouflaged=std::getenv("HALO2_COMBAT_AUDIT_CAMOUFLAGE")!=nullptr;
  if(throwing)options.variant.values.set(83,name=="plasma"?15:16);
  if(const auto *players=std::getenv("HALO2_AUDIT_LOCAL_PLAYERS"))options.local_players=unsigned(std::stoul(players));
  require(!firing || options.local_players==2,"Firing fixture requires its actual target player");
  bool selected=false;std::uint32_t selected_definition=null_datum;const auto choices=cache.multiplayer_weapon_choices();
  for(unsigned option=3;option<=20;++option) {
    std::uint32_t seed{};const auto id=source_starting_weapon(null_datum,option,null_datum,choices,seed);
    if(id!=null_datum && content.weapons.at(id).name.ends_with("\\"+std::string(throwing?"smg":name))) {
      options.starting_weapon=option;selected_definition=id;selected=true;break;
    }
  }
  require(selected,"Requested authored starting weapon absent");
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
  const auto player=world.local_slots.at(0);
  std::array<Controls,multiplayer_capacity> controls{};
  for(const auto slot:world.local_slots) {
    controls[slot].yaw=world.match.state().players[slot].yaw;
    controls[slot].pitch=world.match.state().players[slot].pitch;
  }
  for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
  if(camouflaged) {
    require(firing,"Camouflage attachment fixture requires its actual target");
    auto &source=world.source_objects->tu15_world();auto abi=source.prepare_source_call({});
    abi.gp[0]=world.match.player(world.local_slots.at(1)).source_datum;
    abi.gp[2]=0;abi.gp[7]=300;
    source.run_actual(0x154bd0u,abi); // Complete TU1.5 powerup grant; EAX/EDX/EDI inputs.
  }
  Json firing_setup=Json::object();
  if(firing || throwing) {
    const auto state=world.match.state();const auto target=world.local_slots.at(1);
    const auto &biped=content.player_representations.at(2).biped;
    const auto eye=state.players[player].position+Vec3{0,0,biped.camera_height};
    const auto &collision=world.match.simulation().collision();bool placed=false;
    for(const Vec3 axis:std::array<Vec3,4>{{{1,0,0},{0,1,0},{-1,0,0},{0,-1,0}}}) {
      const auto point=state.players[player].position+axis*1.5f;
      const auto ground=collision.raycast(point+Vec3{0,0,2},{0,0,-1},8);
      if(!ground || ground->normal.z<.65f)continue;
      const auto foot=ground->position+Vec3{0,0,.05f};
      const auto aim=foot+Vec3{0,0,biped.height*.5f};const auto delta=aim-eye;const auto distance=length(delta);
      if(collision.raycast(eye,delta*(1.f/distance),distance-.01f))continue;
      Spawn spawn;spawn.position=foot;spawn.facing=std::atan2(0.f-axis.y,0.f-axis.x);
      test::place_player(world.match,target,spawn);
      world.source_objects->tu15_world().set_object_position(state.players[target].source_unit,foot);
      placed=true;break;
    }
    require(placed,"Needler fixture has no clear nearby actual BSP floor");
    for(unsigned tick=0;tick<60;++tick) {
      const auto state=world.match.state();
      constexpr std::uint32_t source_eye=0x6005b800u,source_velocity=source_eye+0x20u;
      auto &source=world.source_objects->tu15_world();
      auto abi=source.prepare_source_call(std::array{source_eye,source_velocity});
      abi.gp[0]=state.players[player].source_unit;source.run_actual(0x1582f0u,abi);
      const auto &bank=world.source_objects->memory();
      const auto real=[&](unsigned at){return Bytes(bank.view(at,4)).f32(0);};
      const Vec3 eye{real(source_eye),real(source_eye+4),real(source_eye+8)};
      const auto delta=world.source_objects->object_sphere(state.players[target].source_unit).center-eye;
      controls[player].yaw=std::atan2(delta.y,delta.x);
      controls[player].pitch=std::atan2(delta.z,std::sqrt(delta.x*delta.x+delta.y*delta.y));
      world.match.step(controls);
    }
    const auto settled=world.match.state();
    firing_setup={{"declared_source_B95D0_target_placement",true},{"target_slot",target},
        {"target_unit",settled.players[target].source_unit},
        {"shooter_yaw",settled.players[player].yaw},{"input_yaw",controls[player].yaw},
        {"shooter_pitch",settled.players[player].pitch},{"input_pitch",controls[player].pitch},
        {"source_look_input_ticks",60},{"supplied_hit_or_attachment_results",false}};
  }
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
  options.frames=throwing?150:firing?30:36;options.width=640;options.height=480;options.mute=true;
  if(const auto *capture=std::getenv("HALO2_COMBAT_AUDIT_CAPTURE"))options.screenshot=capture;
  options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
  Json input_samples=Json::array();
  Json grenade_samples=Json::array(),detonation_samples=Json::array();
  std::set<std::uint32_t> explosion_effects;unsigned grenade_pulses{},grenade_draws{},explosion_draws{};
  bool grenade_prepared{},grenade_released{};std::uint64_t detonation_frontier{};
  options.input_observer=[&,window](unsigned step,std::span<const Controls> input,const auto &state) {
    input_samples.push_back({{"step",step},{"source_tick",state.tick},{"delivered_zoom",input.front().zoom},
        {"source_zoom",state.players[player].source_zoom.current},{"fire",input.front().fire}});
    if(throwing) {
      grenade_pulses+=input[player].grenade;
      for(const auto &g:state.grenade_projectiles)if(g.unit==state.players[player].source_unit) {
        grenade_prepared|=g.parent!=null_datum;grenade_released|=g.parent==null_datum;
        grenade_samples.push_back({{"tick",state.tick},{"object",g.datum},{"parent",g.parent},
          {"type",g.type},{"timer",g.timer},{"arming",g.arming},{"age",g.age},
          {"position",{g.position.x,g.position.y,g.position.z}}});
      }
      for(const auto &d:state.grenade_detonations)if(d.serial>detonation_frontier) {
        detonation_frontier=d.serial;explosion_effects.insert(d.effect);
        detonation_samples.push_back({{"tick",d.tick},{"projectile",d.projectile},
          {"definition",d.definition},{"effect",d.effect},{"source_instance",d.source_effect_instance},
          {"position",{d.position.x,d.position.y,d.position.z}}});
      }
      if(step==2) {
        SDL_Event event{};event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(window);
        event.key.keysym.scancode=SDL_SCANCODE_Q;event.key.keysym.sym=SDLK_q;
        require(SDL_PushEvent(&event)==1,"Cannot queue actual grenade key down");
        event.type=SDL_KEYUP;require(SDL_PushEvent(&event)==1,"Cannot queue actual grenade key up");
      }
    }
    if(step==0) {
      SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=SDL_GetWindowID(window);
      focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;require(SDL_PushEvent(&focus)==1,"Cannot queue owned window focus");
    }
    if(firing && (step==2 || step==12)) {
      SDL_Event event{};event.type=step==2?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      event.button.button=SDL_BUTTON_LEFT;event.button.windowID=SDL_GetWindowID(window);
      require(SDL_PushEvent(&event)==1,"Cannot queue actual Needler fire input");
    }
    for(unsigned press=0;!firing && !throwing && press<presses;++press)if(step==2+press*8 || step==3+press*8) {
      SDL_Event event{};event.type=step==2+press*8?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      event.button.button=SDL_BUTTON_RIGHT;event.button.windowID=SDL_GetWindowID(window);
      require(SDL_PushEvent(&event)==1,"Cannot queue actual zoom input");
    }
  };
  if(throwing) {
    options.grenade_draw_observer=[&](unsigned view,std::uint32_t,std::uint32_t,std::uint32_t,std::span<const Vertex> vertices) {
      if(!view && !vertices.empty())++grenade_draws;
    };
    options.effect_observer=[&](unsigned view,std::uint32_t,std::span<const ParticleDraw> draws) {
      if(!view)for(const auto &draw:draws)if(explosion_effects.contains(draw.effect_definition))++explosion_draws;
    };
  }
  Json samples=Json::array(),views=Json::array();unsigned frames=0,scoped_draws=0,unscoped_draws=0;
  std::set<std::uint32_t> live_shots,attached_shots,glow_shots,model_shots,raw_attached_shots;
  Json camouflage_samples=Json::array();
  unsigned glow_draws{},glow_vertices{};
  const auto projectile=content.weapons.at(selected_definition).projectile;
  if(firing) {
    options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &) {
      if(view)return;
      for(const auto &p:state.weapon_projectiles)if(p.definition==projectile)live_shots.insert(p.datum);
      for(const auto &p:state.source_dynamic_objects)if(p.type==5 && p.definition==projectile) {
        attached_shots.insert(p.object);
        const auto body=world.source_objects->object_body(p.parent);
        const auto primary=Bytes(world.source_objects->memory().view(body+0x2b0u,4)).f32(0);
        require(p.camouflage==std::clamp(primary,0.f,1.f),"Attached projectile lost its actual first-biped camouflage");
        camouflage_samples.push_back({{"tick",state.tick},{"object",p.object},{"parent",p.parent},
            {"source_primary",primary},{"published_scalar37",p.camouflage}});
      }
      for(const auto &p:world.source_objects->live_objects())
        if(p.type==5 && p.definition==projectile && p.parent!=null_datum)raw_attached_shots.insert(p.object);
    };
    options.weapon_light_volume_observer=[&](unsigned view,std::uint32_t,const SourceLightVolumeDraw &draw) {
      if(view)return;const auto state=world.match.state();
      const bool free=std::any_of(state.weapon_projectiles.begin(),state.weapon_projectiles.end(),
          [&](const auto &p){return p.datum==draw.object && p.definition==projectile;});
      const bool attached=std::any_of(state.source_dynamic_objects.begin(),state.source_dynamic_objects.end(),
          [&](const auto &p){return p.object==draw.object && p.definition==projectile;});
      if(free || attached){glow_shots.insert(draw.object);++glow_draws;glow_vertices+=draw.vertices.size();}
    };
    options.dynamic_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,std::span<const Vertex> vertices) {
      if(!view && definition==projectile && !vertices.empty())model_shots.insert(owner);
    };
  }
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
  Json wire=Json::object();
  if(firing && name=="needler") {
    const auto before=world.match.state();
    unsigned actual{};for(const auto &o:before.source_dynamic_objects)actual+=o.type==5;
    if(actual) {
      LanHost host(world.match,0,0x434f4d424154ull);
      LanClient client("127.0.0.1:"+std::to_string(host.port()),0x434f4d424154ull,1);
      std::array<Controls,multiplayer_capacity> delivered{};std::array<Controls,1> local{};
      for(unsigned i=0;i<30 && !client.ready();++i) {
        client.send(local);host.receive(delivered);host.broadcast();client.receive();
      }
      require(client.ready() && client.error().empty(),"Actual attached-projectile UDP observer did not join");
      host.broadcast();client.receive();unsigned received{};
      for(const auto &o:client.state().source_dynamic_objects)if(o.type==5) {
        ++received;const auto found=std::find_if(before.source_dynamic_objects.begin(),before.source_dynamic_objects.end(),
            [&](const auto &source){return source.object==o.object;});
        require(found!=before.source_dynamic_objects.end() && found->parent==o.parent &&
            found->definition==o.definition && found->has_world_nodes==o.has_world_nodes && found->camouflage==o.camouflage,
            "Attached projectile UDP changed its full generation, definition, parent or WORLD ownership");
      }
      require(received==actual,"Actual UDP snapshot omitted attached projectiles");
      wire={{"passed",true},{"actual_parented_projectiles",actual},{"received",received},
          {"scope","Actual localhost UDP snapshot after the rendered encounter; joined gameplay pixels remain separate"}};
    }
  }
  if(camouflaged && firing)require(std::any_of(camouflage_samples.begin(),camouflage_samples.end(),
      [](const auto &row){return row.at("published_scalar37").template get<float>()>0;}),
      "Actual camouflage variant did not reach an attached needle");
  return {{"scope","actual SDL zoom input, source unit state and native first-person draw submission"},
      {"weapon",name},{"presses",presses},{"local_players",options.local_players},
      {"actual_source_weapon_definition",selected_definition},
      {"declared_cosmetic_seed",0x2468ace0u},{"declared_PC_FOV",78},{"runtime_status",status},{"frames",frames},
      {"scoped_first_person_draws",scoped_draws},{"unscoped_first_person_draws",unscoped_draws},
      {"final_zoom",world.match.state().players[player].source_zoom.current},{"samples",samples},{"views",views},
      {"input_samples",input_samples},
      {"firing",firing},{"firing_setup",firing_setup},{"live_projectiles",live_shots},
      {"attached_projectile_udp",wire},
      {"camouflage_samples",camouflage_samples},
      {"throwing",throwing},{"grenade_pulses",grenade_pulses},{"grenade_prepared",grenade_prepared},
      {"grenade_released",grenade_released},{"grenade_model_draws",grenade_draws},
      {"explosion_particle_draws",explosion_draws},{"grenade_samples",grenade_samples},{"detonations",detonation_samples},
      {"attached_projectiles",attached_shots},{"glow_projectiles",glow_shots},{"attached_model_projectiles",model_shots},
      {"raw_source_attached_projectiles",raw_attached_shots},
      {"glow_draws",glow_draws},{"glow_vertices",glow_vertices},
      {"actual_source_world_grenade_metadata",grenade_fields},
      {"scoped_mesh_submission_detected",scoped_draws>0},
      {"visual_symptom_verified",false}};
}
}
int main(int argc,char **argv)try {
  require(argc>=4,"Supply mode MCC_MAP REPORT [WEAPON PRESSES]");SDL_SetMainReady();
  const bool metadata=std::string_view(argv[1])=="metadata";
  const bool setup=std::string_view(argv[1])=="banshee-setup";
  require(std::string_view(argv[1])=="projectiles" || setup || metadata || argc==6,"Zoom mode requires WEAPON PRESSES");
  const auto result=setup?banshee_setup(argv[2]):std::string_view(argv[1])=="projectiles"?projectile_banks(argv[2]):
      zoom_draws(argv[2],metadata?"sniper_rifle":argv[4],metadata?0u:unsigned(std::stoul(argv[5])),metadata,
          std::string_view(argv[1])=="fire",std::string_view(argv[1])=="grenade");
  std::ofstream(argv[3])<<result.dump(2)<<'\n';std::cout<<result.dump()<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 2;}
