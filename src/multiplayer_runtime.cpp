#include "material_frame_selection.hpp"
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
#include "halo2/vehicle_shading_reference.hpp"
#endif
#include "model_world_pose_cache.hpp"
#ifdef HALO2_PROFILE_MATERIAL_FRAME_ORACLE
#include "material_frame_oracle.hpp"
#endif
#include "halo2/audio.hpp"
#include "halo2/rumble.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_map_effects.hpp"
#include "halo2/unit_damage_audio.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/source_device_event.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_locomotion.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/action_prompt_bindings.hpp"
#include "halo2/appearance.hpp"
#include "halo2/player_armor_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/texture.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_weather_world.hpp"
#include "halo2/source_weather_rsqrt.hpp"
#include "halo2/source_weather_execution.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_character_shaders.hpp"
#include "halo2/source_shield_model_shaders.hpp"
#include "halo2/shield_overlay.hpp"
#include "halo2/source_camouflage_context.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_netgame_weapon_scene.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/scenery.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_grenade_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_scenery_initial_state.hpp"
#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/hud.hpp"
#include "halo2/math.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/multiplayer_presentation.hpp"
#include "halo2/model.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/frontend_model_ik.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/effect_sound.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/sky.hpp"
#include "halo2/transform.hpp"
#include "halo2/water.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <thread>
#include <ctime>
namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
struct Platform {
  std::shared_ptr<DesktopSession> desktop;
  SDL_Window *window{};
  std::vector<SDL_GameController *> pads;
  Platform(std::shared_ptr<DesktopSession> session, unsigned width,
           unsigned height, bool hidden)
      : desktop(session ? std::move(session) : std::make_shared<DesktopSession>()) {
    window = desktop->window(width, height, hidden);
    refresh();
  }
  void refresh() {
    std::erase_if(pads, [](auto *pad) {
      if (SDL_GameControllerGetAttached(pad))
        return false;
      SDL_GameControllerClose(pad);
      return true;
    });
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
      if (SDL_IsGameController(i)) {
        const auto id = SDL_JoystickGetDeviceInstanceID(i);
        if (std::none_of(pads.begin(), pads.end(), [&](auto *p) {
              return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p)) ==
                     id;
            }))
          if (auto *p = SDL_GameControllerOpen(i))
            pads.push_back(p);
      }
  }
  ~Platform() {
    for (auto *pad : pads)
      SDL_GameControllerClose(pad);
  }
};
struct Visual {
  RenderModel model;
  unsigned material{};
  std::optional<AnimationClip> idle;
  std::optional<SourceDynamicModelShaders> shaders;
};
void place_vertices(std::span<Vertex> vertices,unsigned material,const Transform &transform) {
  auto rotate = [&](Vec3 v) {
    return transform.basis[0] * v.x + transform.basis[1] * v.y +
           transform.basis[2] * v.z;
  };
  for (auto &v : vertices) {
    v.position = transform.position + rotate(v.position) * transform.scale;
    v.normal = rotate(v.normal);
    v.tangent = rotate(v.tangent);
    v.binormal = rotate(v.binormal);
    v.material += material;
  }
}
void append(std::vector<Vertex> &out, std::vector<Vertex> vertices,
            unsigned material, const Transform &transform) {
  place_vertices(vertices,material,transform);
  out.insert(out.end(), vertices.begin(), vertices.end());
}
void append_world(std::vector<Vertex> &out,std::vector<Vertex> vertices,unsigned material) {
  for(auto &vertex:vertices)vertex.material+=material;
  out.insert(out.end(),vertices.begin(),vertices.end());
}
Camera player_camera(const MultiplayerState::Player &p,
                     const BipedDefinition &d,float horizontal_fov,Vec2 screen_offset={}) {
  Camera c;
  c.position = ordinary_unit_eye(p.position, d.camera_height,
                                 d.crouch_camera_height, p.crouch);
  c.yaw = p.yaw;
  c.pitch = p.pitch;
  c.field_of_view = source_camera_angles(horizontal_fov).projection_angle * 57.295779513f;
  const auto f=direction(c.yaw,c.pitch);
  const auto aim=source_observer_aim(f,cross(f,{-std::sin(c.yaw),std::cos(c.yaw),0}),horizontal_fov,screen_offset);
  c.oriented=true;c.forward=aim.forward;c.up=aim.up;
  return c;
}
} // namespace
static int run_multiplayer_impl(const MultiplayerOptions &options,
                               MultiplayerRuntimeOutcome *outcome) {
  if((options.session_world_update || options.session_world_loaded || options.session_view_black ||
      options.session_audio_update) &&
      (!options.join.empty() || options.online_match))
    throw FormatError("Local session world owner cannot control a remote or ONLINE match");
  if ((!options.input_script.empty() || options.capture_tick) &&
      ((options.input_script.empty() && !options.input_observer) || !options.capture_tick || options.frames ||
       options.capture_tick > 1000000 || options.screenshot.empty() ||
       options.host_port || !options.join.empty() || options.dedicated || options.replay))
    throw FormatError("Input-script capture requires a local finite tick capture");
  const bool finite_observer=options.input_observer && (options.frames || options.capture_tick) && !options.ticks &&
      !options.host_port && options.join.empty() && !options.dedicated && !options.replay && !options.online_match;
  if((options.reference_auxiliary_seed || options.reference_tick_cadence) &&
      !(options.reference_tick_cadence && !options.reference_auxiliary_seed && finite_observer) &&
      (options.input_script.empty() || !options.capture_tick || options.frames ||
       options.screenshot.empty() || options.host_port || !options.join.empty() ||
       options.dedicated || options.replay || options.online_match))
    throw FormatError("Explicit capture RNG/cadence requires an isolated finite local input-script capture");
  if((options.capture_camera || options.capture_root) && ((!options.frames && !options.capture_tick) || options.screenshot.empty() || options.local_players!=1))
    throw FormatError("Source view replay requires a finite one-player screenshot capture");
  if(options.capture_projection && (!options.capture_camera || options.capture_projection->x<=0 ||
      options.capture_projection->x>=3.14159265358979323846f || options.capture_projection->y<=0 ||
      !std::isfinite(options.capture_projection->x) || !std::isfinite(options.capture_projection->y)))
    throw FormatError("Reference projection requires a captured camera, finite vertical angle and positive lighting scale");
  std::optional<ReferencePlayerState> reference_player;
  if(options.reference_render_fraction && ((!finite_observer && (!options.capture_tick || (options.reference_player.empty() && options.input_script.empty()))) ||
      !std::isfinite(*options.reference_render_fraction) || *options.reference_render_fraction<0 || *options.reference_render_fraction>1))
    throw FormatError("Explicit interpolation requires a finite reference-player or input-script tick capture");
  if(!options.reference_player.empty()) {
    if((!options.frames && !options.capture_tick) || options.screenshot.empty() || options.local_players!=1 ||
       options.capture_camera || options.capture_root || options.host_port || !options.join.empty() ||
       options.dedicated || options.ticks || options.replay)
      throw FormatError("Original player replay requires an isolated finite one-player screenshot capture");
    reference_player=load_reference_player_state(options.reference_player);
    if(reference_player->map!=options.map.stem().string())
      throw FormatError("Original player fixture belongs to a different map");
  }
  if (options.local_players > 4 ||
      (!options.dedicated && !options.local_players))
    throw FormatError("Multiplayer needs 1..4 local players");
  if (options.dedicated && (!options.host_port || !options.join.empty()))
    throw FormatError("Dedicated mode requires --host PORT");
  if (options.host_port > 65535 || (options.host_port && !options.join.empty()))
    throw FormatError("Choose --host PORT or --join ADDRESS:PORT");
  if (options.capture_players &&
      (!options.frames || options.join.empty() ||
       options.capture_players > multiplayer_capacity ||
       options.capture_players < options.local_players))
    throw FormatError("Participant-gated capture requires a finite LAN client run");
  const auto world=options.prepared_world?options.prepared_world:
      std::make_shared<MultiplayerPreparedWorld>(options);
  if(!world->matches(options))throw FormatError("Prepared native world belongs to a different request");
  const auto cache_owner=world->cache;
  const auto &cache=*cache_owner;
  const auto& tags=world->tags;
  auto& content=world->content;
  auto& rules=world->rules;
  auto& match=world->match;
  auto variant=options.variant;
  if(options.starting_weapon)variant.values.set(0x59,int(options.starting_weapon));
  if(!cache.original_data() || !cache.original_data()->grenade_constants)
    std::cerr<<"Grenades unavailable: selected executable has no verified projectile operands\n";
  const auto fingerprint = options.host_port || !options.join.empty() || options.online_match
      ? custom_slayer_fingerprint(multiplayer_fingerprint(options.map, cache.shared_directory(),
                                std::uint32_t(options.bsp), rules.weapon, options.gameplay_xbe),variant)
      : 0;
  std::unique_ptr<LanHost> host;
  std::unique_ptr<LanClient> client;
  std::vector<unsigned> ids;
  if(options.online_match) {
    if(options.host_port || !options.join.empty())throw FormatError("ONLINE requires its matched transport");
    const auto &online=*options.online_match;
    const auto local_slots=online.local_slots();
    if(local_slots.size()!=options.local_players ||
       (!options.selected_profiles.empty() && options.selected_profiles.size()!=local_slots.size()) ||
       (!options.selected_teams.empty() && options.selected_teams.size()!=local_slots.size()))
      throw FormatError("ONLINE local selections differ from the authenticated couch roster");
    const auto phase=online.phase();
    if(phase!=OnlineMatchSession::Phase::Loading && phase!=OnlineMatchSession::Phase::Failed)
      throw FormatError("ONLINE match is no longer loading");
    if(online.host()==online.self()) {
      for(unsigned i=0;i<local_slots.size();++i) {
        const auto &self=online.roster().at(local_slots[i]);
        ids.push_back(match.join(self.name,rules.team_mode ? self.team : -1,
            options.selected_profiles.empty()?std::nullopt:std::optional(options.selected_profiles[i]),self.slot));
      }
      host=std::make_unique<LanHost>(match,options.online_match,fingerprint,online.roster());
    } else client=std::make_unique<LanClient>(options.online_match,online.host(),fingerprint,options.selected_teams,options.selected_profiles);
  } else if (!options.join.empty())
    client = std::make_unique<LanClient>(options.join, fingerprint,
                                         options.local_players,options.selected_teams,options.selected_profiles);
  else {
    ids=world->local_slots;
    if (options.host_port)
      host = std::make_unique<LanHost>(match, options.host_port, fingerprint);
  }
  std::cout << "Native Slayer: " << options.map.filename().string()
            << " local_players=" << options.local_players
            << " primary=" << content.weapons.at(rules.weapon).name << '\n';
  if (!options.gameplay_xbe.empty())
    std::cout << "Original multiplayer balance: " << cache.original_data()->source.string()
              << " stores=" << cache.multiplayer_balance_writes()
              << " netgame_stores=" << cache.multiplayer_netgame_writes() << '\n';
  if (host)
    std::cout << "LAN host UDP port=" << host->port()
              << " capacity=" << rules.maximum_players << std::endl;
  if (client)
    std::cout << "Connecting to LAN host " << options.join << std::endl;
  std::array<Controls, multiplayer_capacity> controls{};
  for (auto id : ids)
    controls[id].yaw = match.unit(id).yaw;
  if(options.session_world_loaded)options.session_world_loaded(match,ids);
  const auto retain_outcome = [&](MultiplayerExitReason reason,
                                  const MultiplayerState &state,
                                  bool client_snapshot_received = true) {
    if (!outcome) return;
    if (client) {
      *outcome = retain_multiplayer_outcome(reason,
          MultiplayerOutcomeAuthority::client_snapshot,
          client_snapshot_received ? &state : nullptr);
      return;
    }
    const auto recording = match.result_recording();
    *outcome = retain_multiplayer_outcome(reason,
        host ? MultiplayerOutcomeAuthority::host : MultiplayerOutcomeAuthority::local,
        &state, MultiplayerOutcomeBanksView{
            &match.source_match_statistics(), match.recorded_statistics(),
            &recording,match.recorded_events()});
  };
  // The peer can leave between the frontend handoff and this world owner.
  // Retain only the actual initialized host state, or no client snapshot.
  if(options.online_match && options.online_match->phase()==OnlineMatchSession::Phase::Failed) {
    retain_outcome(MultiplayerExitReason::online_disconnected,match.state(),false);return 0;
  }
  if (options.dedicated || options.ticks) {
    if (client)
      throw FormatError("Headless client mode is unavailable");
    const auto limit = options.ticks ? options.ticks : options.frames;
    auto next = Clock::now();
    bool hosted_session_started=!ids.empty();
    auto exit_reason=MultiplayerExitReason::headless_limit;
    for (unsigned t = 0; !limit || t < limit; ++t) {
      if (host)
        host->receive(controls);
      // A native dedicated host has no active session before its first
      // admitted participant. Calling the original active-engine end query
      // on that empty roster would finish it before the first UDP hello.
      // Once started, preserve genuine empty-roster/departure ending rules.
      hosted_session_started=hosted_session_started || (host && host->peers());
      if(!host || hosted_session_started)match.step(controls);
      if(options.session_world_update && options.session_world_update(match)) {
        exit_reason=MultiplayerExitReason::session_world_close;
        break;
      }
      if (host)
        host->broadcast();
      if (host) {
        next += std::chrono::microseconds(33333);
        std::this_thread::sleep_until(next);
      }
    }
    const auto s = match.state();
    std::cout << "Multiplayer ticks=" << s.tick << " players="
              << std::count_if(s.players.begin(), s.players.end(),
                               [](const auto &p) { return p.connected; })
              << " finished=" << s.finished << '\n';
    retain_outcome(exit_reason, s);
    return 0;
  }
  Platform platform(options.desktop, options.width, options.height, options.hidden);
  std::shared_ptr<GameVoice> game_voice;
  if(options.online_match) {
    game_voice=options.online_match->retained_voice();
    if(!game_voice){game_voice=std::make_shared<GameVoice>();game_voice->enable(options.match_voice);options.online_match->retain_voice(game_voice);}
  }
  auto mesh = cache.load_bsp(options.bsp);
  std::optional<SourceBspShaderScene> bsp_copy_scene;
  std::optional<Mesh> bsp_reference_mesh;
  std::optional<SourceBspShaderScene> bsp_reference_scene;
  std::optional<SourceBspResidentPlan> bsp_shaders;
  if(!options.bsp_reference_screenshot.empty()) {
    if(options.source_bsp_copy_reference || options.frames!=1 || options.screenshot.empty())
      throw FormatError("Paired BSP verification requires a finite resident initial-frame capture");
    bsp_reference_mesh=mesh;
    bsp_reference_scene.emplace(cache,*bsp_reference_mesh,options.bsp);
  }
  if(options.source_bsp_copy_reference)bsp_copy_scene.emplace(cache,mesh,options.bsp);
  else bsp_shaders.emplace(cache,mesh,options.bsp);
  const auto atmosphere = cache.load_bsp_atmosphere(options.bsp);
  const auto source_portals=cache.load_source_portal_bsp(options.bsp);
  SourceWeatherWorld weather_world(cache,options.bsp);
  auto &auxiliary_random=platform.desktop->auxiliary_random();
  if(options.reference_auxiliary_seed)auxiliary_random=*options.reference_auxiliary_seed;
  const auto capture_auxiliary_start=auxiliary_random;
  {
    SourceWeatherExecution arithmetic;
    weather_world.initialize(auxiliary_random);
  }
  std::map<std::uint32_t,Texture> weather_sprites;
  for(const auto &[datum,owner]:weather_world.definitions())
    for(const auto &system:owner.definition.particles)if(system.type!=2 &&
        !weather_sprites.contains(system.sprites)) {
      auto sprite=cache.load_bitmap(system.sprites,0);
      if(sprite.format==16 && sprite.width==32 && sprite.height==32 && sprite.mips.size()==4)
        weather_sprites.emplace(system.sprites,std::move(sprite));
      else std::cerr<<"Weather sprite graphics pending for authored bitmap "
          <<std::hex<<system.sprites<<std::dec<<" ("<<sprite.width<<'x'<<sprite.height
          <<", format="<<sprite.format<<", mips="<<sprite.mips.size()<<")\n";
    }
  std::map<std::uint32_t,Texture> weather_layers;
  for(const auto &[datum,owner]:weather_world.definitions())
    for(const auto &plate:owner.definition.plates)for(auto bitmap:plate.textures)
      if(!weather_layers.contains(bitmap)) {
        const auto texture=cache.load_bitmap(bitmap,0);
        if(texture.format!=16 || texture.layers!=1 || texture.depth)
          throw FormatError("weather plate needs the proved authored 2D DXT45 texture profile");
        weather_layers.emplace(bitmap,prepare_xemu_bc3_texture(texture));
      }
  if(!weather_world.definitions().empty()) {
    unsigned resident=0;
    for(const auto &[datum,definition]:weather_world.definitions())
      for(const auto &resource:definition.particles)resident+=resource.published();
    std::cout<<"Source weather resources: "<<resident<<" published particle systems, "
        <<weather_world.bsp().palette.size()<<" palette records\n";
  }
  const auto source_bsp_geometry=cache.load_source_bsp_admission(options.bsp);
  const auto source_portal_definition=cache.load_source_portal_mask_definition(options.bsp);
  const auto lighting = cache.load_bsp_lighting(options.bsp);
  const auto scenery_probes=cache.load_scenery_lightprobes(options.bsp);
  const auto object_lighting_definition=cache.load_object_lighting_definition();
  const auto surface_lighting=cache.load_bsp_surface_lighting(options.bsp);
  const CollisionWorld object_light_collision(content.collision);
  prepare_bsp_planar_fog(cache,mesh,atmosphere);
  const auto water=cache.load_bsp_water(options.bsp);
  const auto copy_world_vertices=options.source_bsp_copy_reference?mesh.vertices:std::vector<Vertex>{};
  std::vector<unsigned> water_material_bases;
  for(unsigned d=0;d<water.definitions.size();++d) {
    const auto &definition=water.definitions[d];
    const auto tag=cache.query_tag({.handle=TagHandle{definition.shader}});
    if(tag)for(auto &material:mesh.materials)
      if(material.water_pass && material.name==tag->name)material.water_definition=int(d);
    if(!water.definitions.empty())for(auto &material:mesh.materials)
      if(material.water_pass && material.forward_vertex==7)material.water_definition=0;
    water_material_bases.push_back(unsigned(mesh.materials.size()));
    for(auto material:definition.geometry.materials) {
      material.water_definition=int(d);mesh.materials.push_back(std::move(material));
    }
  }
  auto visual = [&](std::uint32_t datum, const ObjectColors *colors=nullptr, const ScenarioObject *object=nullptr,
                    bool dynamic_lighting=false, std::optional<float> camouflage=std::nullopt) {
    Visual v;
    v.model = cache.load_model(datum,true,object?object->variant:0);
    if(object) {
      const auto definition=cache.load_scenery_definition(datum);
      if(!definition)throw FormatError("Original scenery definition missing: "+v.model.name);
      const auto implementation=scenery_lighting_implementation(lighting,v.model,*object,*definition);
      if(implementation==SceneryLightingImplementation::unsupported)
        throw FormatError("Original scenery lighting implementation3 remains unresolved: "+v.model.name);
      dynamic_lighting=implementation==SceneryLightingImplementation::dynamic;
      if(!dynamic_lighting && !cache.apply_scenery_lighting(v.model,lighting,*object))
        throw FormatError("Original scenery vertex-light bucket missing: "+v.model.name);
    }
    if(dynamic_lighting)for(auto &material:v.model.mesh.materials) {
      if(material.lods.empty())continue;
      if(!prepare_dynamic_material(cache,material,{},true,camouflage))
        throw FormatError("Multiplayer dynamic shader requires unresolved inputs: "+material.name);
    }
    if(object) {
      const auto color_definition=cache.load_source_object_color_definition(datum);
      if(!color_definition.channels.empty()) {
        for(const auto&channel:color_definition.channels)if(channel.function_count)
          throw FormatError("Scenery current color functions require source inputs");
        const auto selected=cache.load_initial_model_selection(datum,object->variant);
        if(!selected.resolved())throw FormatError("Scenery color variant requires source initial selection");
        const auto definition=cache.load_initial_model_definition(datum);
        const auto actual_variant=selected.variant_index<0?0u:
            definition.variants.at(unsigned(selected.variant_index)).name;
        const auto authored_colors=source_object_initial_colors(color_definition,object->position,actual_variant);
        for(auto &material:v.model.mesh.materials)apply_object_colors(material,authored_colors);
      }
    }
    if(colors)for(auto &material:v.model.mesh.materials)apply_object_colors(material,*colors);
    if(dynamic_lighting && !object) {
      const auto source=cache.query_tag({.handle=TagHandle{datum}});
      if(!source)throw FormatError("dynamic model source tag missing");
      const auto mode=source->group=="mode"?datum:cache.load_initial_model_definition(datum).render_model;
      v.shaders.emplace(cache,v.model,mode,2,camouflage);
    }
    v.material = unsigned(mesh.materials.size());
    mesh.materials.insert(mesh.materials.end(), v.model.mesh.materials.begin(),
                          v.model.mesh.materials.end());
    return v;
  };
  struct VehicleVisual {
    std::array<RenderModel,6> lods;
    unsigned material{};
    std::unique_ptr<SourceCharacterShaders> shaders;
    SourceGeometryLodDefinition geometry;
    SourceObjectColorDefinition colors;
    std::uint32_t variant_name{};
  };
  std::map<std::pair<std::uint32_t,std::uint32_t>,VehicleVisual> vehicle_visuals;
  std::map<std::uint32_t,ObjectLightQueryContext> vehicle_light_definitions;
  for(const auto &o:content.objects)if(o.type==1 && o.definition!=null_datum) {
    const auto key=std::pair{o.definition,o.variant};if(vehicle_visuals.contains(key))continue;
    VehicleVisual vehicle;
    const auto initial=cache.load_initial_model_definition(o.definition);
    const auto selected=cache.load_initial_model_selection(o.definition,o.variant);
    if(!selected.resolved())throw FormatError("Vehicle initial variant requires its original selection owner");
    vehicle.variant_name=selected.variant_index<0?0u:initial.variants.at(unsigned(selected.variant_index)).name;
    vehicle.colors=cache.load_source_object_color_definition(o.definition);
    vehicle.geometry=cache.load_source_geometry_lod_definition(o.definition);
    // Prepare every literal source geometry bank. Resident materials are shared;
    // the union of section origins binds compiled inputs for every bank once.
    RenderModel bindings;
    for(unsigned lod=0;lod<vehicle.lods.size();++lod) {
      auto& model=vehicle.lods[lod];
      model=cache.load_model(o.definition,lod==5,o.variant,selected,lod);
      if(lod==5)bindings.mesh.materials=std::move(model.mesh.materials);
      else model.mesh.materials.clear();
      bindings.mesh.vertices.insert(bindings.mesh.vertices.end(),model.mesh.vertices.begin(),model.mesh.vertices.end());
      bindings.vertex_origins.insert(bindings.vertex_origins.end(),model.vertex_origins.begin(),model.vertex_origins.end());
    }
    vehicle.shaders=std::make_unique<SourceCharacterShaders>(cache,bindings,o.definition);
    vehicle_light_definitions.emplace(o.definition,vehicle.shaders->light_query_context());
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
    for(auto& material:bindings.mesh.materials)vehicle_shading_reference_material_storage(material);
#endif
    vehicle.material=unsigned(mesh.materials.size());
    mesh.materials.insert(mesh.materials.end(),std::make_move_iterator(bindings.mesh.materials.begin()),std::make_move_iterator(bindings.mesh.materials.end()));
    vehicle_visuals.emplace(key,std::move(vehicle));
  }
  const auto &weapon = content.weapons.at(rules.weapon);
  if(reference_player && (reference_player->unit_definition!=content.player.datum ||
                         reference_player->weapon_definition!=weapon.datum))
    throw FormatError("Original player fixture has different unit or weapon definitions");
  struct WeaponWorldVisual {
    Visual gun;
    std::array<unsigned,multiplayer_capacity*2> materials;
  };
  std::map<std::uint32_t,WeaponWorldVisual> body_guns;
  std::map<std::uint32_t,SourceWorldWeaponDefinition> weapon_shader_definitions;
  for(const auto &[datum,definition]:content.weapons)
    weapon_shader_definitions.emplace(datum,cache.load_source_world_weapon_definition(datum));
  const unsigned object_slots=host || client ? multiplayer_capacity:
      (ids.empty()?0u:*std::max_element(ids.begin(),ids.end())+1u);
  const auto player_emblem_palette=cache.load_full_scoreboard_assets().palette;
  const auto player_emblem_scenario=cache.load_source_geometry_scenario_type();
  for(const auto &[datum,definition]:content.weapons)if(!definition.first_person_models.empty() && definition.first_person_models.front().model!=null_datum && definition.first_person_models.front().animation!=null_datum) {
    auto &world=body_guns[datum];world.gun=visual(datum,nullptr,nullptr,true,0.f);
    world.gun.idle=cache.load_initialized_object_idle(datum);
    for(unsigned i=0;i<object_slots*2;++i) {
      world.materials[i]=unsigned(mesh.materials.size());
      mesh.materials.insert(mesh.materials.end(),world.gun.model.mesh.materials.begin(),world.gun.model.mesh.materials.end());
    }
  }
  struct SpeciesVisual {
    struct ShieldVisual {
      std::optional<SourceShieldModelShaders> shaders;
      std::array<unsigned,multiplayer_capacity> materials{};
      unsigned count{};
      std::uint32_t render_model{};
    };
    Visual player,hands;
    std::map<std::uint32_t,ShieldVisual> body_shields,hand_shields;
    std::vector<Visual> guns;
    ShadowCasterDefinition player_bounds;
    WeaponAttachmentMarkers body_attachment;
    std::optional<WeaponAttachmentMarkers> secondary_attachment;
    std::shared_ptr<NativePlayerAnimation> body_animation;
    std::array<unsigned,multiplayer_capacity> player_materials;
    AnimationClip fp_idle;
    FirstPersonIkDefinition fp_ik;
    std::array<FrontendModelWeaponIk,2> body_ik;
    std::optional<FirstPersonBindingDefinition> fp_definition;
    std::optional<AnimationClip> fp_fire,fp_reload,fp_reload_empty,fp_ready,fp_throw;
    std::map<std::uint32_t,AnimationClip> fp_clips;
    std::vector<ObjectAttachmentMarker> muzzle_markers;
    std::map<std::uint32_t,std::vector<ObjectAttachmentMarker>> effect_markers;
    std::map<std::string,std::vector<ObjectAttachmentMarker>> shield_markers;
    std::uint32_t foot_effects{};
    std::int8_t foot_variant{};
    std::vector<std::vector<ObjectAttachmentMarker>> foot_markers;
  };
  std::map<std::pair<unsigned,std::uint32_t>,SpeciesVisual> species;
  std::vector<FirstPersonBindingDefinition> local_definitions;
  for(const auto &[weapon_datum,weapon]:content.weapons) {
  if(weapon.first_person_models.empty() || weapon.first_person_models.front().model==null_datum || weapon.first_person_models.front().animation==null_datum)continue;
  const auto weapon_animation=cache.load_weapon_animation_identifiers(weapon.datum);
  for(unsigned representation:{2u,3u}) {
    if(representation>=content.player_representations.size() ||
        content.player_representations[representation].biped.datum==null_datum)continue;
    const auto &authored=content.player_representations[representation];
    auto &v=species[{representation,weapon_datum}];
    const auto colors=reference_player?reference_player->colors:cache.load_object_colors(authored.biped.datum);
    const auto common=std::find_if(species.begin(),species.end(),[&](const auto &entry) {
      return entry.first.first==representation && entry.first.second!=weapon_datum;
    });
    if(common!=species.end()) {
      v.player=common->second.player;v.player_materials=common->second.player_materials;
      v.hands=common->second.hands;
    } else {
      v.player=visual(authored.biped.datum,&colors,nullptr,true);
      v.hands=visual(authored.hands,&colors,nullptr,true);
      v.player_materials[0]=v.player.material;
      for(unsigned i=1;i<object_slots;++i) {
        v.player_materials[i]=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),v.player.model.mesh.materials.begin(),v.player.model.mesh.materials.end());
      }
    }
    v.player_bounds=cache.load_object_shadow_definition(authored.biped.datum);
    const auto parent_selection=cache.load_initial_model_selection(authored.biped.datum,0);
    const auto child_selection=cache.load_initial_model_selection(weapon.datum,0);
    if(!parent_selection.resolved() || !child_selection.resolved())
      throw FormatError("Held weapon requires its actual initial region selection: "+weapon.name);
    v.body_attachment=select_weapon_attachment(cache.load_weapon_attachment(authored.biped.datum,weapon.datum,0),
        parent_selection.permutations,child_selection.permutations);
    if(weapon.dual_wieldable) {
      v.secondary_attachment=select_weapon_attachment(cache.load_weapon_attachment(authored.biped.datum,weapon.datum,1),parent_selection.permutations,child_selection.permutations);
    }
    v.player.idle=cache.load_unit_weapon_action(v.player.model.animation,0x06000086u,
        weapon_animation[0],weapon_animation[1],0x0400000cu);
    v.body_animation=std::make_shared<NativePlayerAnimation>(cache,v.player.model.animation,weapon.datum);
    for(unsigned crouch=0;crouch<2;++crouch)
      v.body_ik[crouch]=cache.load_unit_weapon_ik(authored.biped.datum,weapon.datum,
          crouch?0x06000087u:0x06000086u);
    // Original169D8A selects weapon record1 for Elite, falling back to0
    // only when the weapon has no second authored record.
    const auto record=representation==3 && weapon.first_person_models.size()>1 ? 1u:0u;
    const auto &fp=weapon.first_person_models.at(record);
    for(unsigned i=0;i<options.local_players*2;++i) {
      if(i==0)v.guns.push_back(visual(fp.model,nullptr,nullptr,true,0.f));
      else {
        auto copy=v.guns.front();copy.material=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),copy.model.mesh.materials.begin(),copy.model.mesh.materials.end());
        v.guns.push_back(std::move(copy));
      }
    }
    v.fp_idle=cache.load_action(fp.animation,"any","any","any","idle");
    v.fp_ik=cache.load_first_person_ik(fp.animation,fp.model,authored.hands);
    v.fp_definition=cache.load_first_person_binding(weapon.datum,representation);
    const auto &definition=*v.fp_definition;
    if(definition.arms_model!=authored.hands || definition.weapon_model!=fp.model || definition.graph!=fp.animation)
      throw FormatError("First-person rendering disagrees with authored binding definitions");
    local_definitions.push_back(definition);
    auto clocks=definition.clock_clips;
    clocks.insert(clocks.end(),definition.dual_clock_clips.begin(),definition.dual_clock_clips.end());
    for(const auto &clock:clocks) {
      auto clip=cache.load_animation(definition.graph,clock.index);
      v.fp_clips.emplace(clock.index,clip);
      switch(clock.activity) {
      case 0x05000024:v.fp_ready=clip;break;
      case 0x06000006:v.fp_fire=clip;break;
      case 0x0b000065:v.fp_reload=clip;break;
      case 0x0c000064:v.fp_reload_empty=clip;break;
      case 0x0d000021:v.fp_throw=clip;break;
      }
    }
    const auto biped=tags.get({authored.biped.datum},tag_groups::biped);
    v.foot_effects=biped.u32(0x58);
    v.foot_variant=std::int8_t(std::to_integer<unsigned>(biped.read(0x1a,1)[0]));
    const auto model=tags.get({biped.u32(0x38)},tag_groups::model);
    const auto render=tags.get({model.u32(4)},tag_groups::render_model);
    const auto& info=render.info();
    const auto render_model=cache.load_source_tag_metadata(info.datum);
    const auto names=biped.block(0x2f8,4);
    for(std::size_t n=0;n<names.size();++n)
      v.foot_markers.push_back(source_model_markers(render_model,names.element(n).u32(0)));
  }
  }
  const auto make_shields=[&](const RenderModel &model,std::uint32_t mode,
      const VitalityDefinition &vitality,bool first_person) {
    std::map<std::uint32_t,SpeciesVisual::ShieldVisual> result;
    const std::array shaders{first_person?vitality.shield_damaged_first_person_shader:vitality.shield_damaged_shader};
    for(const auto shader:shaders)if(shader!=null_datum && !result.contains(shader)) {
      auto &layer=result[shader];layer.render_model=mode;std::vector<Material> materials;
      layer.shaders.emplace(cache,model,mode,shader,materials);layer.count=unsigned(materials.size());
      for(unsigned player=0;player<object_slots;++player) {
        layer.materials[player]=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),materials.begin(),materials.end());
      }
    }
    return result;
  };
  for(auto &[key,body]:species) {
    const auto &representation=content.player_representations.at(key.first);
    const auto common=std::find_if(species.begin(),species.end(),[&](const auto &entry) {
      return entry.first.first==key.first && !entry.second.body_shields.empty();
    });
    if(common!=species.end()) {
      body.body_shields=common->second.body_shields;body.hand_shields=common->second.hand_shields;
    } else {
      const auto mode=cache.load_initial_model_definition(representation.biped.datum).render_model;
      body.body_shields=make_shields(body.player.model,mode,representation.biped.vitality,false);
      body.hand_shields=make_shields(body.hands.model,representation.hands,representation.biped.vitality,true);
    }
  }
  const auto species_for=[&](const MultiplayerState::Player &p)->SpeciesVisual& {
    return species.at({multiplayer_representation(p.source_configuration),p.weapon==null_datum?rules.weapon:p.weapon});
  };
  const auto biped_for=[&](const MultiplayerState::Player &p)->const BipedDefinition& {
    return multiplayer_biped(content,multiplayer_representation(p.source_configuration));
  };
  if(!client)match.configure_local_views(ids,local_definitions);
  std::array<FirstPersonViewBinding,4> client_fp_views;
  std::array<FirstPersonViewBinding,4> client_fp_origins;
  std::array<bool,4> client_fp_clock_available{};
  std::array<bool,4> client_secondary_clock_available{};
  std::array<std::uint32_t,4> client_fp_reported_units;
  client_fp_reported_units.fill(null_datum);
  // Retain the existing native visual-event primary channel until the
  // original game-tick dispatch owner is connected. Borrowing gameplay's
  // deterministic word from this render-batched owner would make gameplay
  // RNG depend on frame batching. All proven auxiliary consumers share the
  // actual application word; this legacy primary channel is not Xbox history.
  std::uint32_t visual_event_primary=0x12345678u;
  const SourceEffectRandom effect_random{visual_event_primary,auxiliary_random};
  NativeMuzzleEffects muzzle_effects(cache,effect_random);
  muzzle_effects.collision_query([&](auto flags,Vec3 origin,Vec3 delta)->std::optional<ParticleCollisionHit> {
    if(world->source_objects && world->source_objects->owns_physics())
      return world->source_objects->particle_collision(flags,origin,delta);
    const auto distance=length(delta);if(!(distance>0))return {};
    const auto hit=match.simulation().collision().raycast(origin,delta,distance);if(!hit)return {};
    return ParticleCollisionHit{hit->distance/distance,hit->position,hit->normal,true,std::int16_t(hit->material)};
  });
  NativeWorldEffects world_effects(cache,mesh,match.simulation().collision(),effect_random);
  if(world->source_objects && world->source_objects->owns_physics())
    world_effects.collision_query([&](auto flags,Vec3 origin,Vec3 delta) {
      return world->source_objects->particle_collision(flags,origin,delta);
    });
  for(auto &[key,v]:species) {
    const auto &vitality=content.player_representations.at(key.first).biped.vitality;
    for(const auto effect:{vitality.shield_damaged_effect,vitality.shield_depleted_effect,vitality.shield_recharging_effect})
      world_effects.preload_attached(effect);
    const auto mode=cache.load_initial_model_definition(content.player_representations.at(key.first).biped.datum).render_model;
    const auto metadata=cache.load_source_tag_metadata(mode);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x60));
    const Bytes groups(metadata.bytes_at(root.u32(0x5c),std::size_t(root.u32(0x58))*12));
    for(unsigned n=0;n<root.u32(0x58);++n) {
      const auto name=groups.u32(n*12);
      v.shield_markers.emplace(cache.string_id(name),source_model_markers(metadata,name));
    }
  }
  std::uint64_t world_impact_serial{},world_melee_serial{},world_detonation_serial{},world_unit_effect_serial{},world_voice_serial{};
  bool world_effect_baseline=!client;
  std::set<std::string> reported_world_effect_gaps;
  NativeEffectSounds effect_sounds(cache);
  std::vector<std::pair<std::uint32_t,std::uint32_t>> weapon_effect_sounds;
  std::uint32_t client_effect_serial{};
  bool client_effect_baseline{};
  // Native effect slots are bounded; retain the full salt alongside the
  // source instance so retired/reused slots cannot inherit an old refresh.
  struct FiringOwner {std::uint32_t weapon=null_datum,presentation=null_datum,source=null_datum;};
  std::map<std::uint16_t,FiringOwner> source_firing_instances;
  std::set<std::uint32_t> world_firing_effects;
  std::map<std::uint32_t,std::map<std::string,std::vector<ObjectAttachmentMarker>>> firing_world_markers;
  std::uint32_t world_weapon_effect_serial{};
  bool client_prop_effect_baseline{};
  NativeDeviceEventCursor device_event_cursor(!client);
  std::set<std::uint32_t> heard_prop_effects;
  std::uint32_t muzzle_effect=null_datum;
  const auto stock_smg=std::find_if(content.weapons.begin(),content.weapons.end(),
      [](const auto &entry){return entry.second.smg.has_value();});
  if(stock_smg!=content.weapons.end() && !stock_smg->second.firing_effects.empty()) {
    muzzle_effect=stock_smg->second.firing_effects[0].effects[0];
    for(auto &[key,v]:species)if(content.weapons.at(key.second).smg) {
      v.muzzle_markers=source_model_markers(cache.load_source_tag_metadata(v.fp_definition->weapon_model),
          muzzle_effects.marker_name(muzzle_effect));
      if(v.muzzle_markers.empty())throw FormatError("Authored first-person muzzle marker is missing");
    }
  }
  const auto preload_weapon_world_effect=[&](std::uint32_t datum,std::uint32_t effect) {
      world_effects.preload_attached(effect);world_firing_effects.insert(effect);
      if(!firing_world_markers.contains(datum)) {
        const auto model=cache.load_initial_model_definition(datum).render_model;
        if(model!=null_datum) {
          const auto metadata=cache.load_source_tag_metadata(model);
          const Bytes root(metadata.bytes_at(metadata.source_address(),0x60));
          const Bytes groups(metadata.bytes_at(root.u32(0x5c),std::size_t(root.u32(0x58))*12));
          auto &bank=firing_world_markers[datum];
          for(unsigned n=0;n<root.u32(0x58);++n) {
            const auto name=groups.u32(n*12);bank.emplace(cache.string_id(name),source_model_markers(metadata,name));
          }
        }
      }
  };
  std::set<std::uint32_t> firing_effects;
  std::set<std::uint32_t> firing_sound_effects;
  for(const auto &[datum,d]:content.weapons)if(d.firing_presentation)
  for(const auto &entry:d.firing_effects)for(const auto effect:entry.effects) {
    if(effect==null_datum || !cache.query_tag({.handle=TagHandle{effect},.group=tag_groups::effect,.require_address=true}))continue;
    firing_sound_effects.insert(effect);
    try {
      preload_weapon_world_effect(datum,effect);
    }catch(const FormatError &e){std::cerr<<"World firing effect "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
    try {
    if(cache.load_muzzle_effect(effect).light_parts && !world_firing_effects.contains(effect))
      throw FormatError("Firing light parts have no completed live world owner");
    const auto names=muzzle_effects.marker_names(effect);
    for(auto &[key,v]:species)if(key.second==datum)for(auto name:names) {
      auto markers=source_model_markers(cache.load_source_tag_metadata(v.fp_definition->weapon_model),name);
      if(markers.empty())throw FormatError("Authored ordinary firing effect marker is missing");
      v.effect_markers.emplace(name,std::move(markers));
    }
    firing_effects.insert(effect);
    }catch(const FormatError &e){std::cerr<<"Firing effect "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
  }
  std::set<std::uint32_t> ready_effects;
  for(const auto &[datum,d]:content.weapons)if(d.ready_effect!=null_datum)try {
    if(cache.load_muzzle_effect(d.ready_effect).light_parts)
      preload_weapon_world_effect(datum,d.ready_effect);
    const auto names=muzzle_effects.marker_names(d.ready_effect);
    for(auto &[key,v]:species)if(key.second==datum)for(auto name:names) {
      auto markers=source_model_markers(cache.load_source_tag_metadata(v.fp_definition->weapon_model),name);
      if(markers.empty())throw FormatError("Authored ready effect marker is missing");
      v.effect_markers.emplace(name,std::move(markers));
    }
    ready_effects.insert(d.ready_effect);
  }catch(const FormatError &e){std::cerr<<"Ready effect "<<std::hex<<d.ready_effect<<std::dec<<": "<<e.what()<<'\n';}
  for(auto &[key,v]:species)if(!v.muzzle_markers.empty())
    v.effect_markers.emplace(muzzle_effects.marker_name(muzzle_effect),v.muzzle_markers);
  struct PresentationOwnerLifetime {
    MultiplayerMatch *match{};
    ~PresentationOwnerLifetime(){if(match)match->release_presentation_owners();}
  } presentation_owner_lifetime{client?nullptr:&match};
  if(!client)match.configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &input) {
      if(input.effect==null_datum)return null_datum;
      const auto tag=cache.query_tag({.handle=TagHandle{input.effect}});
      if(tag && tag->group=="snd!") {
        weapon_effect_sounds.emplace_back(input.effect,input.weapon);return null_datum;
      }
      std::optional<SourceWeaponEffectPresentation> source;
      Vec3 velocity=unit.body.velocity;
      auto &binding=source_firing_instances[std::uint16_t(input.weapon)];
      const auto source_previous=binding.weapon==input.weapon && binding.presentation==input.previous_handle
          ?binding.source:null_datum;
      if(tag && tag->group=="effe" && world->source_objects && world->source_objects->owns_physics()) {
        auto original=input;original.previous_handle=source_previous;
        source=world->source_objects->request_weapon_effect(original);
        velocity=world->source_objects->object_linear_velocity(input.weapon);
        if(!source->colors_observed)throw FormatError("Prepared shot has no completed original effect-color owner");
        match.weapon_effect_presentation(unit.source_datum,input.weapon,*source,velocity);
      }
      const bool local=std::any_of(ids.begin(),ids.end(),[&](unsigned id){return match.player(id).source_datum==unit.source_player;});
      const bool fp=input.effect==muzzle_effect || firing_effects.contains(input.effect) || ready_effects.contains(input.effect);
      if(local && fp) {
        auto presentation=input;
        if(source && (source->instance==null_datum || source->instance!=source_previous))presentation.previous_handle=null_datum;
        const auto handle=source?muzzle_effects.request(presentation,std::uint32_t(match.simulation().ticks()+1),
            source->colors,velocity,world_firing_effects.contains(input.effect)):
            muzzle_effects.request(presentation,std::uint32_t(match.simulation().ticks()+1));
        binding={input.weapon,handle,source?source->instance:null_datum};return handle;
      }
      if(!world_firing_effects.contains(input.effect) && firing_sound_effects.contains(input.effect))
        effect_sounds.spawn(input.effect,input.weapon,unit.body.position,std::bit_cast<float>(input.a_bits),std::bit_cast<float>(input.b_bits));
      const auto handle=source?source->instance:null_datum;binding={input.weapon,handle,handle};return handle;
    });
  struct FpWeaponAttachment {std::uint32_t effect{},scale_a{},scale_b{};};
  std::map<std::uint32_t,std::vector<FpWeaponAttachment>> fp_weapon_attachments;
  for(const auto &[datum,weapon]:content.weapons)if(weapon.inventory_flags&(1u<<24)) {
    const auto metadata=cache.load_source_tag_metadata(datum);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
    const auto count=root.u32(0x94);
    const Bytes attachments(count?metadata.bytes_at(root.u32(0x98),std::size_t(count)*0x18):std::span<const std::byte>{});
    for(unsigned n=0;n<count;++n) {
      const auto at=n*0x18;
      if(attachments.u32(at)!=0x65666665u || attachments.u32(at+4)==null_datum)continue;
      const auto effect=attachments.u32(at+4),a=attachments.u32(at+0x10),b=attachments.u32(at+0x14);
      try {
        if(attachments.u32(at+0xc) || (a && !source_weapon_shader_input(a,{})) ||
            (b && !source_weapon_shader_input(b,{})))
          throw FormatError("Blade attachment requires its actual object-function/color owner");
        const auto markers=muzzle_effects.marker_names(effect);
        for(auto &[key,visual]:species)if(key.second==datum)
          for(const auto name:markers)
            visual.effect_markers.try_emplace(name,source_model_markers(
                cache.load_source_tag_metadata(visual.fp_definition->weapon_model),name));
        fp_weapon_attachments[datum].push_back({effect,a,b});
      }catch(const FormatError &error) {std::cerr<<"Authored blade attachment: "<<error.what()<<'\n';}
    }
  }
  SourceSceneryInitialState scenery_initial_state;
  struct SceneryQueryLifetime {
    std::size_t placement{};
    std::uint64_t created_tick{};
    bool native_animated_bounds_admitted{};
    std::uint64_t advanced_tick{};
    std::optional<SourceSceneryIdlePhaseDefinition> idle_definition;
    SourceSceneryIdlePhaseState idle_state,previous_idle_state;
    bool idle_paired{};
  };
  std::map<std::uint32_t,SceneryQueryLifetime> scenery_query_lifetimes;
  SourceSceneryScene source_scenery(cache,content.objects,options.bsp,mesh,object_light_collision,{},
      [&](std::size_t,const ScenarioObject &object) {
        // Only the four already identified Zanzibar RNG-dependent pillars
        // retain their old preview. An additional unresolved owner is an error.
        const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
        return tag &&
            tag->name=="scenarios\\objects\\multi\\zanzibar\\pillar\\pillar" &&
            object.placement>=19 && object.placement<=22;
      });
  source_scenery.birth_query_capture([&](std::size_t source_object,std::uint32_t handle,
      std::uint64_t created_tick,std::uint32_t definition,const Transform &node0,float scale) {
    const auto &live=match.simulation().scene().get(handle);
    const auto &authored=content.objects.at(source_object);
    if(live.placement!=source_object || live.created_tick!=created_tick || authored.definition!=definition)
      throw FormatError("Scenery constructor bounds require their actual full-handle lifetime");
    scenery_initial_state.capture(handle,definition,
        cache.load_object_shadow_definition(definition),node0,scale);
    SceneryQueryLifetime lifetime;
    lifetime.placement=source_object;lifetime.created_tick=created_tick;lifetime.advanced_tick=created_tick;
    if(!scenery_query_lifetimes.emplace(handle,std::move(lifetime)).second)
      throw FormatError("Scenery constructor bounds repeated a live full handle");
  });
  float scenery_render_fraction=1;
  source_scenery.animation_frames([&](std::size_t source_object,std::uint32_t handle,
      std::uint64_t created_tick,std::uint64_t simulation_tick,const AnimationClip &clip) -> AnimationFrameWindow {
    const auto &live=match.simulation().scene().get(handle);
    auto &lifetime=scenery_query_lifetimes.at(handle);
    if(live.placement!=source_object || live.created_tick!=created_tick ||
        lifetime.placement!=source_object || lifetime.created_tick!=created_tick || simulation_tick<created_tick)
      throw FormatError("Scenery animation requires its actual native constructor clock");
    if(!lifetime.idle_definition) {
      lifetime.idle_definition=cache.load_source_scenery_idle_phase_definition(clip);
      lifetime.idle_state=source_scenery_idle_phase_initial(*lifetime.idle_definition,tick_seconds);
    }
    const auto &definition=*lifetime.idle_definition;
    if(definition.graph!=clip.source_graph || definition.animation!=clip.source_index ||
        simulation_tick<lifetime.advanced_tick)
      throw FormatError("Scenery idle controller changed its retained binding or clock");
    // Catch up the existing native simulation clock one admitted tick at a
    // time, using original controller arithmetic and the retained loop carry.
    // B8600/BC470 activation and the original clock producer remain separate;
    // native tick history is never presented as captured source scheduling.
    while(lifetime.advanced_tick<simulation_tick) {
      lifetime.previous_idle_state=lifetime.idle_state;lifetime.idle_paired=true;
      source_scenery_idle_phase_step(lifetime.idle_state,definition,tick_seconds);
      ++lifetime.advanced_tick;
    }
    lifetime.native_animated_bounds_admitted=true;
    if(!lifetime.idle_paired)return {lifetime.idle_state.frame,lifetime.idle_state.frame,1};
    return {lifetime.previous_idle_state.frame,lifetime.idle_state.frame,scenery_render_fraction};
  });
  source_scenery.lighting_query_state([&](std::size_t source_object,std::uint32_t handle,
      const Transform &node0,float scale) {
    const auto &live=match.simulation().scene().get(handle);
    const auto &authored=content.objects.at(source_object);
    const auto &lifetime=scenery_query_lifetimes.at(handle);
    if(live.placement!=source_object || lifetime.placement!=source_object ||
        live.created_tick!=lifetime.created_tick)
      throw FormatError("Scenery lighting requires its actual cached-bounds lifetime");
    // Frozen lighting still consumes bounds refreshed by an admitted animated
    // pose update. This ports BC820/BDEF0 for this native update; it does not
    // establish the original activation scheduler's equality to native ticks.
    const auto &state=lifetime.native_animated_bounds_admitted?
        scenery_initial_state.refresh_cached_bounds(handle,authored.definition,
            cache.load_object_shadow_definition(authored.definition),node0,scale,true):
        scenery_initial_state.retained(handle,authored.definition,node0,scale,false);
    return SourceSceneryQueryState{state.sphere.center,state.sphere.radius,state.velocity,
        state.query_runtime_flags,state.frozen};
  });
  source_scenery.synchronize(match.simulation().scene());
  const auto &scenery_counts=source_scenery.stats();
  std::cout<<"Source scenery: "<<scenery_counts.render_models<<" resolved models, "
      <<scenery_counts.baked_models<<" baked and "<<scenery_counts.dynamic_models
      <<" dynamic, six resident geometry levels and source idle phase; native update scheduler\n";
  if(scenery_counts.unresolved_selections)
    std::cout<<"Scenery preview boundary: "<<scenery_counts.unresolved_selections
        <<" Zanzibar pillars retain legacy geometry until source initialization RNG is supplied\n";
  struct Scenery {
    Visual visual;
    Transform transform;
    SceneryDefinition definition;
    bool dynamic_lighting{},reported{};
    const SceneryLightprobe *probe{};
    ObjectLightingCache light;
    Vec3 query_origin;
  };
  std::vector<Scenery> scenery;
  for (const auto source_object : source_scenery.unresolved_selection_owners()) {
    const auto &object=content.objects.at(source_object);
      try {
        auto v = visual(object.definition,nullptr,&object);
        v.idle=cache.load_initialized_object_idle(object.definition);
        const auto b = authored_basis(object.rotation);
        const auto definition=*cache.load_scenery_definition(object.definition);
        const bool dynamic=scenery_lighting_implementation(lighting,v.model,object,definition)==SceneryLightingImplementation::dynamic;
        const auto probe=dynamic?scenery_lightprobe(scenery_probes,object,definition):nullptr;
        const Transform placement{object.scale,{b.forward,b.left,b.up},object.position};
        Vec3 query_origin;
        if(dynamic && !probe) {
          const auto bounds=cache.load_object_shadow_definition(object.definition);
          const auto root=source_object_transform(object.position,b.forward,b.up,object.scale,false);
          const auto node0=compose_transform(root,pose_model_node(v.model,0,v.idle?&*v.idle:nullptr));
          query_origin=source_shadow_sphere(bounds,node0,object.scale).center;
        }
        scenery.push_back(
            {std::move(v),placement,definition,dynamic,false,probe,{},query_origin});
      } catch (const FormatError &e) {
        std::cerr << "Scenery: " << e.what() << '\n';
      }
  }
  SourceMachineScene machines(cache,content.objects,options.bsp,mesh,object_light_collision);
  machines.synchronize(match.simulation().scene(),&match.simulation().devices());
  if(reference_player && !reference_player->devices.empty())
    machines.set_reference_devices(reference_player->devices,mesh);
  SourcePortalMaskState source_portal_mask;
  std::map<std::uint32_t,SourcePortalOwnerState> source_portal_owners;
  {
    std::vector<SourcePortalOwnerState> owners;
    for(const auto &packet:machines.portal_owners())owners.push_back(packet.owner);
    source_portal_mask.reset_acoustic();
    source_portal_mask.initialize(source_portal_definition,owners);
    for(const auto &owner:owners)source_portal_owners.emplace(owner.handle,owner);
  }
  std::uint64_t source_portal_tick=match.state().tick;
  const auto &machine_counts=machines.stats();
  std::cout<<"Source machines: "<<machine_counts.placements<<" placements, "
           <<machine_counts.render_models<<" render models, "<<machine_counts.prt_materials
           <<" PRT material uses, "<<machine_counts.initial_state_only
           <<" door/platform placements; authoritative simulation/replica device state\n";
  SourcePropInitialState prop_initial_state;
  const auto geometry_scenario_type=cache.load_source_geometry_scenario_type();
  std::function<std::optional<SourcePropQueryState>(std::uint32_t)> live_prop_query;
  std::set<std::uint32_t> prop_query_handles;
  SourcePropScene props(cache,content.objects,options.bsp,mesh,object_light_collision);
  props.region_state([&](std::uint32_t handle){return match.simulation().props().region_selection(handle);});
  SourceMapEffects map_effects(cache,content.objects,world_effects);
  {
    const auto state=map_effects.stats();
    std::cout<<"Map effect attachments: "<<state.prepared<<'/'<<state.attachments<<" prepared\n";
    for(const auto &gap:state.unavailable)std::cerr<<"Map effect attachment boundary: "<<gap<<'\n';
  }
  std::optional<SourceGrenadeScene> grenade_scene;
  std::vector<std::uint32_t> projectile_definitions;
  for(const auto &[datum,weapon]:content.weapons)
    if(weapon.projectile!=null_datum)projectile_definitions.push_back(weapon.projectile);
  NativeWeaponProjectileScene weapon_projectile_scene(cache,options.bsp,mesh,
      object_light_collision,projectile_definitions,match.native_object_capacity());
  if(cache.original_data() && cache.original_data()->grenade_constants &&
     cache.original_data()->object_construction_operands)
    grenade_scene.emplace(cache,options.bsp,mesh,object_light_collision,match.native_object_capacity(),
        cache.original_data()->object_construction_operands->one_bits);
  // SceneObjects currently has no type11 physics advance. Retain the original
  // first BDEF0 output, proven before B8890 registers physics; later transform
  // changes require the actual subsequent physics/cache producer.
  props.synchronize(match.simulation().scene());
  props.lighting_query_state([&](std::size_t source_object,std::uint32_t handle,
                                 const Transform& node0,float scale) {
    if(live_prop_query)if(const auto live=live_prop_query(handle))return *live;
    const auto &authored=content.objects.at(source_object);
    if(!prop_initial_state.contains(handle)) {
      const auto &live=match.simulation().scene().get(handle);
      const auto same=[](float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);};
      const auto same_vector=[&](Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);};
      if(live.placement!=source_object || !same_vector(live.position,authored.position) ||
          !same_vector(live.rotation,authored.rotation) || !same(live.scale,authored.scale))
        throw FormatError("Prop lighting requires its actual constructor/cache history");
      prop_initial_state.capture(handle,authored.definition,
          cache.load_object_shadow_definition(authored.definition),node0,scale);
      prop_query_handles.insert(handle);
    }
    const auto &state=prop_initial_state.retained(handle,authored.definition,node0,scale,false);
    return SourcePropQueryState{{state.sphere.center,state.sphere.radius},state.velocity,state.query_runtime_flags,state.frozen};
  });
  const auto &prop_counts=props.stats();
  std::cout<<"Source props: "<<prop_counts.placements<<" placements, "
           <<prop_counts.render_models<<" render models, "<<prop_counts.union_dynamic_materials
           <<" dynamic and "<<prop_counts.union_prt_materials<<" PRT resident material slots across six geometry levels; initial cached physics state\n";
  std::cout<<"Native map pickups and retained weapon inventory enabled\n";
  std::vector<std::optional<SkyDefinition>> skies;
  std::vector<std::optional<ObjectSkyLightingDefinition>> sky_lighting;
  std::vector<std::optional<Visual>> sky_models;
  for(unsigned i=0;i<atmosphere.skies.size();++i) {
    skies.push_back(cache.load_sky(i));sky_models.emplace_back();
    sky_lighting.push_back(cache.object_sky_lighting_available(i)?
        cache.load_object_sky_lighting(i):std::optional<ObjectSkyLightingDefinition>{});
    if(skies.back() && skies.back()->model!=null_datum)sky_models.back()=visual(skies.back()->model);
  }
  std::map<std::pair<unsigned,std::uint32_t>,std::unique_ptr<MultiplayerHud>> huds,offhand_huds;
  std::map<unsigned,std::unique_ptr<MultiplayerHud>> unit_huds;
  const auto voice_range=cache.load_hud(multiplayer_biped(content,2).datum,rules.weapon).sensor.range;
  for(const auto &[key,v]:species) {
    auto authored_hud=cache.load_hud(multiplayer_biped(content,key.first).datum,key.second);
    if(!unit_huds.contains(key.first)) {
      auto unit_hud=authored_hud;
      std::erase_if(unit_hud.widgets,[](const auto &widget){return widget.weapon_type==1;});
      unit_huds.emplace(key.first,std::make_unique<MultiplayerHud>(std::move(unit_hud),
          cache.shared_directory()/"fonts",mesh,options.local_players,options.variant.name,
          cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets));
    }
    std::erase_if(authored_hud.widgets,[](const auto &widget){return widget.weapon_type!=1;});
    authored_hud.action_messages.enabled=false;
    huds.emplace(key,std::make_unique<MultiplayerHud>(authored_hud,
        cache.shared_directory()/"fonts",mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets));
    if(content.weapons.at(key.second).dual_wieldable)
      offhand_huds.emplace(key,std::make_unique<MultiplayerHud>(std::move(authored_hud),
          cache.shared_directory()/"fonts",mesh,options.local_players,options.variant.name,
          cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,1));
  }
  MultiplayerScoreboard scoreboard(cache,cache.shared_directory()/"fonts",mesh,variant,options.ui_assets);
  std::unique_ptr<GamePauseMenu> pause_menu;
  if(!options.ui_assets.empty()) {
    std::unique_ptr<char,decltype(&SDL_free)> pref(SDL_GetPrefPath("Halo2Native","Halo2"),SDL_free);
    pause_menu=std::make_unique<GamePauseMenu>(cache.shared_directory(),options.ui_assets,mesh,false,
        pref?std::filesystem::path(pref.get())/"menu.cfg":std::filesystem::path{},options.menu_ui_assets);
  }
  std::optional<unsigned> session_black_material;
  if(options.session_view_black) {
    session_black_material=unsigned(mesh.materials.size());
    mesh.materials.push_back({});
    mesh.materials.back().ui_full_surface=true;
  }
  std::cout<<"Source resident material roots: "<<mesh.materials.size()<<'\n';
  SourceBspEffectLights bsp_effect_lights(cache,mesh,0,world_effects.light_definitions());
  for(const auto& gap:bsp_effect_lights.unavailable())
    if(reported_world_effect_gaps.insert(gap).second)
      std::cerr<<"BSP effect light material boundary: "<<gap<<'\n';
  const auto renderer_owner=platform.desktop->make_renderer(mesh,options.validation,{},bsp_shaders?&*bsp_shaders:nullptr);
  auto& renderer=*renderer_owner;
  if(pause_menu)pause_menu->bind(platform.window,*platform.desktop,renderer);
  renderer.debug_panel().share_display_settings(platform.desktop->display_settings);
  if(options.debug_panel)renderer.debug_panel().set_visible(true);
  renderer.measure_frames(bool(options.frame_observer));
  renderer.scene_bsp_atmosphere(atmosphere);
  std::cout << "Multiplayer GPU: " << renderer.device_name()
            << " split_views=" << options.local_players << std::endl;
  std::unique_ptr<MultiplayerAudio> audio;
  const auto audio_bsp=cache.load_multiplayer_audio_bsp(options.bsp);
  bool ambient_unavailable_reported{};
  if (!options.mute)
    try {
      const auto seed=platform.desktop->fresh_secondary_random();
      audio = std::make_unique<MultiplayerAudio>(cache,seed,
          SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,512,{}},
          options.audio_observer,&auxiliary_random);
      if(auto error=audio->speaker_layout(platform.desktop->display_settings.speakers);!error.empty())std::cerr<<"Audio layout: "<<error<<'\n';
      if(pause_menu)pause_menu->audio_layout([&](unsigned layout){return audio?audio->speaker_layout(layout):std::string{};});
      for(const auto &[representation,v]:species)if(v.fp_definition)for(const auto &clip:v.fp_definition->clock_clips)
        for(const auto &event:clip.events)if(event.type==1)try{audio->prepare(event.tag);}
          catch(const std::exception &e){std::cerr<<"Authored sound "<<std::hex<<event.tag<<std::dec<<": "<<e.what()<<'\n';}
      if(!client)match.configure_first_person_event_owner(
          [&](unsigned view,unsigned hand,const FirstPersonViewBinding &binding,const FirstPersonAnimationEvent &event){
            auto selected=binding;selected.hands[0]=binding.hands[hand];
            try{audio->first_person(view,selected,event);}
            catch(const std::exception &e){std::cerr<<"First-person audio: "<<e.what()<<'\n';}
          });
      std::cout<<"Multiplayer SDL audio: authored first-person events and BSP ambient tracks\n";
    } catch (const std::exception &e) {
      audio.reset();
      std::cerr << "Audio: " << e.what() << '\n';
    }
  const auto device_event_audio=[&](std::span<const SourceDeviceWorldEvent> bank){
    for(const auto &event:device_event_cursor.observe(bank)){
      const auto tag=cache.query_tag({.handle=TagHandle{event.tag}});
      if(!tag)throw FormatError("Device event has no resident tag");
      const auto group=tag->group=="snd!"?0x736e6421u:tag->group=="effe"?0x65666665u:0u;
      switch(source_device_event_kind(event.tag,group)){
      case SourceDeviceEventKind::Sound:
        // Original107980 requests189060 with this device, markerNONE and
        // control1. Native PCM uses the authored bank and actual full owner.
        // Xbox attached spatial admission/occlusion remains a separate owner.
        if(audio)try{audio->impulse(event.tag,1,MultiplayerAudioKind::Impulse,event.handle);}
          catch(const std::exception &error){std::cerr<<"Device event sound: "<<error.what()<<'\n';}
        break;
      case SourceDeviceEventKind::Effect:
        if(reported_world_effect_gaps.insert("Device176780 attachment/location producer unavailable").second)
          std::cerr<<"Device176780 attachment/location producer unavailable\n";
        break;
      case SourceDeviceEventKind::Ignore:break;
      }
    }
  };
  if(options.online_match) {
    options.online_match->loaded(fingerprint,variant);
    const auto deadline=Clock::now()+std::chrono::seconds(185);
    std::cout<<"ONLINE waiting for matched players to load"<<std::endl;
    while(options.online_match->phase()!=OnlineMatchSession::Phase::Running) {
      if(options.online_match->phase()==OnlineMatchSession::Phase::Failed || Clock::now()>deadline) {
        retain_outcome(MultiplayerExitReason::online_disconnected,match.state(),false);return 0;
      }
      SDL_Event event;while(SDL_PollEvent(&event)) {
        if(event.type==SDL_QUIT){retain_outcome(MultiplayerExitReason::window_quit,match.state(),false);return 0;}
        if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){retain_outcome(MultiplayerExitReason::escape,match.state(),false);return 0;}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const auto admission_deadline=Clock::now()+std::chrono::seconds(15);
    std::array<Controls,4> neutral{};
    // No world ticks before every real participant has been admitted.
    for(;;) {
      if(host) { host->receive(controls);host->broadcast(); }
      if(client) { client->send(std::span(neutral).first(options.local_players));client->receive(); }
      const auto admission_state=client?client->state():match.state();
      const bool admitted=(!client || client->ready()) &&
          std::count_if(admission_state.players.begin(),admission_state.players.end(),[](const auto &p){return p.connected;})==int(options.online_match->roster().size());
      if(admitted)break;
      if(options.online_match->phase()==OnlineMatchSession::Phase::Failed || Clock::now()>admission_deadline || (client && !client->error().empty())) {
        retain_outcome(MultiplayerExitReason::online_disconnected,client?client->state():match.state(),client && client->ready());return 0;
      }
      SDL_Event event;while(SDL_PollEvent(&event)) {
        if(event.type==SDL_QUIT){retain_outcome(MultiplayerExitReason::window_quit,match.state(),false);return 0;}
        if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){retain_outcome(MultiplayerExitReason::escape,match.state(),false);return 0;}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout<<"ONLINE match started with "<<options.online_match->roster().size()<<" real players"<<std::endl;
  }
  std::unique_ptr<NativeUnitDamageAudio> damage_audio;
  if(audio)damage_audio=std::make_unique<NativeUnitDamageAudio>(cache,*audio,auxiliary_random);
  MultiplayerState current = client && client->ready() ? client->state() : match.state(), previous = current;
  live_prop_query=[&](std::uint32_t handle)->std::optional<SourcePropQueryState> {
    if(!current.source_scene_roots_authoritative)return {};
    const auto root=std::find_if(current.source_scene_roots.begin(),current.source_scene_roots.end(),
        [&](const auto &root){return root.object==handle;});
    const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
        [&](const auto &pose){return pose.object==handle;});
    if(root==current.source_scene_roots.end() || pose==current.source_body_poses.end())
      throw FormatError("Live prop lighting lost its completed source bounds owner");
    return SourcePropQueryState{{pose->sphere.center,pose->sphere.radius},root->velocity,
        root->runtime_flags,root->frozen};
  };
  const auto update_audio_listeners=[&]{
    if(!audio)return;
    std::vector<Vec3> positions;
    for(const auto id:ids)if(current.players[id].connected){
      const auto &p=current.players[id];const auto &biped=biped_for(p);
      positions.push_back(player_camera(p,biped,biped.camera_field_of_view,content.player_screen_offset).position);
    }
    audio->listeners(positions);
  };
  update_audio_listeners();
  const auto consume_client_weapon_effects=[&]{
    if(!client_effect_baseline) {
      if(!current.weapon_effects.empty())client_effect_serial=current.weapon_effects.back().serial;
      client_effect_baseline=true;return;
    }
    for(const auto &effect:current.weapon_effects)if(effect.serial>client_effect_serial) {
      client_effect_serial=effect.serial;
      const auto local=std::find_if(ids.begin(),ids.end(),[&](auto id) {
        const auto &p=current.players[id];return p.connected && p.source_unit==effect.unit &&
            (p.source_weapon==effect.weapon || p.secondary_weapon.datum==effect.weapon);
      });
      if(local==ids.end())continue;
      const auto &p=current.players[*local];
      if(effect.effect==muzzle_effect || firing_effects.contains(effect.effect) || ready_effects.contains(effect.effect)) {
        auto &binding=source_firing_instances[std::uint16_t(effect.weapon)];
        const auto previous=binding.weapon==effect.weapon && effect.source_instance!=null_datum && binding.source==effect.source_instance
            ?binding.presentation:null_datum;
        const WeaponEffectInput input{effect.effect,effect.weapon,previous,effect.a_bits,effect.b_bits};
        const auto handle=effect.source_presentation?muzzle_effects.request(input,effect.tick,effect.colors,
            effect.velocity,world_firing_effects.contains(effect.effect)):muzzle_effects.request(input,effect.tick);
        binding={effect.weapon,handle,effect.source_instance};
      }else {
        const auto tag=cache.query_tag({.handle=TagHandle{effect.effect}});
        if(tag && tag->group=="snd!")weapon_effect_sounds.emplace_back(effect.effect,effect.weapon);
        else if(!world_firing_effects.contains(effect.effect) && firing_sound_effects.contains(effect.effect))
          effect_sounds.spawn(effect.effect,effect.weapon,p.position,std::bit_cast<float>(effect.a_bits),std::bit_cast<float>(effect.b_bits));
      }
    }
  };
  SceneInterpolation scene_interpolation;
  scene_interpolation.reset(current.tick,match.simulation().scene().objects());
  SourceWorldInterpolation source_body_interpolation;
  const auto capture_source_body_poses=[&](bool reset=false) {
    if(reset)source_body_interpolation.reset(current.tick,current.source_body_poses);
    else source_body_interpolation.capture(current.tick,current.source_body_poses);
  };
  capture_source_body_poses(true);
  if(client && client->ready())ids.assign(client->slots().begin(),client->slots().end());
  auto update_sensors=[&] {
    for(unsigned view=0;view<ids.size();++view) {
      const auto id=ids[view];
      unit_huds.at(multiplayer_representation(current.players[id].source_configuration))->update_motion_sensor(current,id,view);
    }
  };
  update_sensors();
  std::array<std::uint32_t,4> uploaded_sensor_tick{~0u,~0u,~0u,~0u};
  std::array<FirstPersonAimBlend,multiplayer_capacity> aim_blends;
  std::array<FirstPersonAimBlend,multiplayer_capacity> body_aim_blends;
  struct NativeBodyChannels {
    NativeLocomotionFrame primary;
    const AnimationClip *secondary{},*offhand{};
    float secondary_frame{},offhand_frame{};
    std::uint32_t secondary_activity=null_datum,display_activity=null_datum;
    NativePlayerAnimation *left{};
    bool moving{},dual{};
  };
  // Rendering and aim/IK admission consume the same actual selected channels.
  const auto select_native_body=[&](const MultiplayerState::Player &a,NativeLocomotionFrame presentation,double body_tick) {
    const auto &body=species_for(a);
    const AnimationClip *primary=presentation.clip;
    auto primary_activity=presentation.activity;
    const bool body_moving=presentation.activity==0x0a000014u || presentation.activity==0x09000015u ||
        presentation.activity==0x09000016u || presentation.activity==0x0a000017u;
    const AnimationClip *grenade_overlay=nullptr;
    if((a.source_action_word&(1u<<22)) && a.grenade_action.animation_active) {
      const auto *clip=match.grenade_clip(a.grenade_action.graph,a.grenade_action.clip);
      if(clip && clip->animation_type==0){primary=clip;presentation.frame=a.grenade_action.frame;primary_activity=a.grenade_action.activity;}
      else grenade_overlay=clip;
    }
    const bool dual=a.secondary_weapon.datum!=null_datum;
              const AnimationClip *action=grenade_overlay;
              auto action_activity=grenade_overlay?a.grenade_action.activity:null_datum;
              float action_frame=grenade_overlay?a.grenade_action.frame:0;
              const AnimationClip *offhand=nullptr;float offhand_frame=0;
              NativePlayerAnimation *left=nullptr;
              if(dual) {
                auto left_player=a;left_player.weapon=a.secondary_weapon.definition;
                left=species_for(left_player).body_animation.get();
              }
              // Snapshot ticks own the action; rendering samples them without
              // restarting the animation or scaling it to a guessed duration.
              if(a.melee_action.active()) {
                action=body.body_animation->action(a.melee_action.activity,a.native_crouching,dual);
                action_activity=a.melee_action.activity;
                action_frame=float(std::max(0.,body_tick-double(a.melee_action.started)));
              }
              if(!a.dead && !a.melee_action.active() && !(a.source_action_word&(1u<<22))) {
                const auto activity=a.reload_seconds>0?0x08000002u:0x06000006u;
                const auto start=a.reload_seconds>0?a.reload_tick:a.shot_tick;
                if(start && body_tick>=double(start)) {
                  action=body.body_animation->action(activity,a.native_crouching,dual);
                  action_activity=activity;
                  action_frame=float(body_tick-double(start));
                  if(action && action_frame>=float(action->frame_count))action=nullptr;
                }
                // Other overlays (ready, put-away, melee, alternate fire)
                // require an actual published action and an authored binding.
                if(!action && a.first_person_clock.present) {
                  const auto &clock=a.first_person_clock.current;
                  if(clock.activity!=null_datum && clock.activity!=0x0400000cu &&
                      clock.activity!=0x06000006u && clock.activity!=0x0c000064u && clock.activity!=0x0b000065u) {
                    action=body.body_animation->action(clock.activity,a.native_crouching,dual);
                    action_activity=clock.activity;
                    action_frame=clock.frame;
                    if(action && action_frame>=float(action->frame_count))action=nullptr;
                  }
                }
                if(dual) {
                  const auto &weapon=a.secondary_weapon;
                  const auto start=weapon.reload_seconds>0?weapon.reload_tick:weapon.shot_tick;
                  if(start && body_tick>=double(start)) {
                    offhand=left->action(
                        weapon.reload_seconds>0?0x0d00053au:0x0b00053eu,a.native_crouching,true);
                    offhand_frame=float(body_tick-double(start));
                    if(offhand && offhand_frame>=float(offhand->frame_count))offhand=nullptr;
                  }
                }
              }
    if(action && action->animation_type==0){primary=action;presentation.frame=action_frame;primary_activity=action_activity;action=nullptr;}
    const auto composed_activity=action && a.melee_action.active()?a.melee_action.activity:primary_activity;
    return NativeBodyChannels{{primary,presentation.frame,primary_activity},action,offhand,
        action_frame,offhand_frame,action?action_activity:null_datum,composed_activity,left,body_moving,dual};
  };
  const auto compose_native_body=[&](unsigned id,const MultiplayerState::Player &a,NativeLocomotionFrame presentation,double body_tick,float relative_yaw,
      NativeLocomotionFrame *posed=nullptr) {
    const auto &body=species_for(a);
    const auto channels=select_native_body(a,presentation,body_tick);
    if(posed)*posed={channels.primary.clip,channels.primary.frame,channels.display_activity};
    return body.body_animation->pose(body.player.model,*channels.primary.clip,channels.primary.frame,a.pitch,
        a.native_crouching,channels.dual,channels.moving,channels.secondary,channels.secondary_frame,
        channels.offhand,channels.offhand_frame,relative_yaw,0,channels.left,channels.primary.activity,
        source_aim_blend_weight(body_aim_blends[id]),
        !aim_blends[id].count || source_aim_blend_weight(aim_blends[id])>=
            std::bit_cast<float>(cache.original_data()->unit_animation_constants->frame_subtractor_bits));
  };
  std::array<NativePlayerTransition,multiplayer_capacity> body_transitions;
  std::array<std::unique_ptr<NativeLocomotion>,multiplayer_capacity> gait;
  std::array<bool,multiplayer_capacity> gait_dual{};
  std::array<std::uint32_t,multiplayer_capacity> gait_units{},gait_weapons{};
  std::array<bool,multiplayer_capacity> gait_unavailable{};
  const auto update_native_gait=[&](unsigned id) {
    const auto &p=current.players[id];
    if(!p.connected||p.dead){gait[id].reset();return;}
    const auto &v=species_for(p);
    const auto &player=v.player;
    const auto &foot_markers=v.foot_markers;
    const bool dual=p.secondary_weapon.datum!=null_datum;
    if(gait_units[id]!=p.source_unit||gait_weapons[id]!=p.source_weapon||gait_dual[id]!=dual){
      gait[id].reset();body_transitions[id]={};gait_unavailable[id]=false;gait_units[id]=p.source_unit;gait_weapons[id]=p.source_weapon;
      gait_dual[id]=dual;
    }
    if(gait_unavailable[id])return;
    try{
      if(!gait[id]) {
        gait[id]=std::make_unique<NativeLocomotion>(cache,player.model.animation,p.weapon,
            std::array<std::uint32_t,2>{rules.random_seed,p.source_unit},dual);
        gait[id]->bind_facing(biped_for(p).datum,p.yaw);
      }
      // Only a type0 throw owns the base. Masked throws retain the running
      // gait clock and its current foot events beneath the authored overlay.
      if((p.source_action_word&(1u<<22))&&p.grenade_action.animation_active) {
        const auto *clip=match.grenade_clip(p.grenade_action.graph,p.grenade_action.clip);
        if(clip && clip->animation_type==0)return;
      }
      gait[id]->tick(p.native_forward,p.native_right,p.native_grounded,p.native_crouching,p.velocity,
          [&](const NativeFootstepEvent &event){
            if(!audio||p.native_support_material==0xffff)return;
            // The triangle importer already resolves BSP-local material rows
            // to global material indices. Snapshot support excludes appended
            // prop namespaces; applying the BSP mapping again loses footsteps.
            const auto native_frame=gait[id]->frame();const auto basis=authored_basis({gait[id]->facing(),0,0});
            const auto root=source_object_transform(p.position,basis.forward,basis.up,1,false);
            const auto nodes=pose_model_nodes_world_frame(player.model,root,native_frame.clip,event.frame);
            const auto position=authored_footstep_position(foot_markers,nodes,event.marker,p.position);
            const auto listener=player_camera(current.players[ids.front()],biped_for(current.players[ids.front()]),
                biped_for(current.players[ids.front()]).camera_field_of_view,content.player_screen_offset).position;
            // Native audible-distance adaptation of the original10-unit
            // foot-kind cull; Xbox PVS/listener pooling remains separate.
            if(dot(position-listener,position-listener)>100.f)return;
            for(const auto sound:authored_footstep_sounds(cache,v.foot_effects,
                std::bit_cast<std::int16_t>(p.native_support_material),event.kind,v.foot_variant))
              if(sound!=null_datum)audio->impulse_at(sound,position,1,MultiplayerAudioKind::Footstep,p.source_unit);
          },&p.landing,&p.yaw);
      NativeLocomotionFrame composed;
      const auto locals=compose_native_body(id,p,gait[id]->frame(),double(current.tick),std::remainder(p.yaw-gait[id]->facing(),6.283185307179586f),&composed);
      if(cache.original_data()->unit_constructor_tail_constants)
        body_transitions[id].tick(composed.activity,p.native_crouching,locals,*cache.original_data()->unit_constructor_tail_constants);
    }catch(const std::exception&e){gait_unavailable[id]=true;gait[id].reset();std::cerr<<"Native gait/footstep owner: "<<e.what()<<'\n';}
  };
  std::optional<std::uint64_t> world_effect_tick;
  std::set<std::uint32_t> attached_effect_objects;
  const auto update_world_effects=[&](bool accepted_replica=false) {
    if(client && !accepted_replica && !world_effect_baseline)return;
    if(client && accepted_replica) {
      if(current.source_scene_roots_authoritative)
        match.replicate_source_scene(current.source_scene_roots,current.source_body_poses);
      match.replicate_devices(current.devices);
      match.replicate_prop_removals(current.prop_removals);
      match.replicate_prop_regions(current.prop_regions);
    }
    //Publish the actual current scene pose on the simulation tick before
    //emission. The later render interpolation is a separate read-only sample.
    machines.synchronize(match.simulation().scene(),&match.simulation().devices());
    props.synchronize(match.simulation().scene());
    source_scenery.synchronize(match.simulation().scene());
    const auto render_fraction=scenery_render_fraction;scenery_render_fraction=1;
    machines.update_pose(current.devices.tick,std::int32_t(current.tick),mesh);
    props.update_pose(std::int32_t(current.tick),mesh);
    source_scenery.update_pose(current.tick,std::int32_t(current.tick),mesh);
    scenery_render_fraction=render_fraction;
    auto map_owners=source_scenery.effect_owners();
    for(const auto &owner:machines.effect_owners())map_owners.push_back(owner);
    for(const auto &owner:props.effect_owners())map_owners.push_back(owner);
    map_effects.synchronize(map_owners,current.tick);
    std::set<std::uint32_t> present;
    const auto publish_markers=[&](std::uint32_t handle,const auto &visual,std::span<const Transform> nodes) {
      NativeWorldEffects::ObjectMarkers markers;
      const auto append=[&](const std::string &name,const Transform &t) {
        markers[name].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
      };
      for(const auto &[name,authored]:visual.shield_markers)
        for(const auto &marker:pose_model_markers(authored,nodes))append(name,marker.world);
      //178360 grants only literal root a node-zero fallback.
      if(!markers.contains("root"))append("root",nodes.front());
      world_effects.object_markers(handle,std::move(markers));present.insert(handle);
    };
    for(unsigned id=0;id<multiplayer_capacity;++id) {
      const auto &p=current.players[id];
      const auto handle=p.dead?p.source_previous_unit:p.source_unit;
      if(!p.connected || handle==null_datum)continue;
      const auto &v=species_for(p);
      std::vector<Transform> nodes;
      if(current.source_body_poses_authoritative) {
        const auto found=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
            [&](const auto &pose){return pose.object==handle;});
        if(found==current.source_body_poses.end())continue;
        nodes=found->nodes;
      }else if(!p.dead) {
        const float heading=gait[id]?gait[id]->facing():p.yaw;
        const auto b=authored_basis({heading,0,0});
        const auto root=source_object_transform(p.position,b.forward,b.up,1,false);
        if(gait[id] && v.body_animation && !reference_player) {
          auto locals=compose_native_body(id,p,gait[id]->frame(),double(current.tick),
              std::remainder(p.yaw-heading,6.283185307179586f));
          body_transitions[id].apply(locals,1);
          nodes=pose_model_locals_nodes_world(v.player.model,root,locals);
        }else nodes=pose_model_nodes_world(v.player.model,root,v.player.idle?&*v.player.idle:nullptr,
            float(current.tick)*tick_seconds);
      }else if(attached_effect_objects.contains(handle)) {
        // Retain the last completed node bank until the corpse gets its actual
        // source pose or its full handle disappears; do not bind a new unit.
        present.insert(handle);continue;
      }
      if(nodes.empty())continue;
      publish_markers(handle,v,nodes);
    }
    for(const auto &player:current.players)if(player.connected && !player.dead)
      for(const auto &[weapon,definition]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{{
          {player.source_weapon,player.weapon},{player.secondary_weapon.datum,player.secondary_weapon.definition}}}) {
        const auto authored=firing_world_markers.find(definition);
        if(weapon==null_datum || authored==firing_world_markers.end())continue;
        const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
            [&](const auto &p){return p.object==weapon;});
        if(pose==current.source_body_poses.end() || pose->nodes.empty())continue;
        NativeWorldEffects::ObjectMarkers markers;
        for(const auto &[name,bank]:authored->second)for(const auto &marker:pose_model_markers(bank,pose->nodes)) {
          const auto &t=marker.world;markers[name].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
        }
        if(!markers.contains("root")) {
          const auto &t=pose->nodes.front();markers["root"].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
        }
        world_effects.object_markers(weapon,std::move(markers));present.insert(weapon);
      }
    if(current.source_body_poses_authoritative)for(const auto &corpse:current.corpses) {
      if(present.contains(corpse.object))continue;
      const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
          [&](const auto &p){return p.object==corpse.object;});
      if(pose==current.source_body_poses.end())continue;
      const auto &visual=species.at({multiplayer_representation(corpse.configuration),rules.weapon});
      publish_markers(corpse.object,visual,pose->nodes);
    }
    for(const auto handle:attached_effect_objects)if(!present.contains(handle))world_effects.remove_object(handle);
    attached_effect_objects=std::move(present);
    if(!world_effect_baseline){
      if(!current.bullet_impacts.empty())world_impact_serial=current.bullet_impacts.back().serial;
      if(!current.melee_impacts.empty())world_melee_serial=current.melee_impacts.back().serial;
      if(!current.grenade_detonations.empty())world_detonation_serial=current.grenade_detonations.back().serial;
      if(!current.unit_damage_effects.empty())world_unit_effect_serial=current.unit_damage_effects.back().serial;
      if(!current.weapon_effects.empty())world_weapon_effect_serial=current.weapon_effects.back().serial;
      world_effect_baseline=true;
    }
    for(const auto &effect:current.weapon_effects)if(effect.serial>world_weapon_effect_serial) {
      world_weapon_effect_serial=effect.serial;
      if(!world_firing_effects.contains(effect.effect))continue;
      const bool local=std::any_of(ids.begin(),ids.end(),[&](auto id) {
        const auto &player=current.players[id];return player.connected && player.source_unit==effect.unit &&
            (player.source_weapon==effect.weapon || player.secondary_weapon.datum==effect.weapon);
      });
      const bool fp=effect.effect==muzzle_effect || firing_effects.contains(effect.effect) || ready_effects.contains(effect.effect);
      world_effects.request_weapon(effect.effect,effect.weapon,effect.tick,effect.colors,effect.velocity,
          std::bit_cast<float>(effect.a_bits),std::bit_cast<float>(effect.b_bits),effect.source_instance,!local || !fp);
    }
    // Both journals share the collision owner's serial. Preserve contact
    // creation order when a render frame receives several simulation ticks.
    struct Contact {std::uint64_t serial;const BulletImpact *bullet;const MeleeImpact *melee;};
    std::vector<Contact> contacts;
    for(const auto &e:current.bullet_impacts)if(e.serial>world_impact_serial)contacts.push_back({e.serial,&e,nullptr});
    for(const auto &e:current.melee_impacts)if(e.serial>world_melee_serial)contacts.push_back({e.serial,nullptr,&e});
    std::sort(contacts.begin(),contacts.end(),[](const auto &a,const auto &b){return a.serial<b.serial;});
    for(const auto &contact:contacts){
      if(contact.bullet)world_impact_serial=contact.serial;else world_melee_serial=contact.serial;
      try{
        if(contact.bullet)world_effects.impact(*contact.bullet);
        else world_effects.melee_impact(*contact.melee);
      }catch(const FormatError&error){
        if(reported_world_effect_gaps.insert(error.what()).second)std::cerr<<"Authored impact visual: "<<error.what()<<'\n';
      }
    }
    for(const auto&event:current.grenade_detonations)if(event.serial>world_detonation_serial){
      world_detonation_serial=event.serial;
      try{
        if(!event.source_locations.empty())world_effects.spawn_source(event.effect,event.projectile,event.tick,event.source_locations,event.colors,event.controls);
        else world_effects.spawn(event.effect,event.projectile,event.tick,
          {event.position,event.up,event.forward,event.forward,event.up,false});
      }catch(const FormatError&error){
        if(reported_world_effect_gaps.insert(error.what()).second)std::cerr<<"Authored grenade visual: "<<error.what()<<'\n';
      }
    }
    for(const auto &event:current.unit_damage_effects)if(event.serial>world_unit_effect_serial) {
      world_unit_effect_serial=event.serial;
      world_effects.spawn_attached(event.effect.datum,event.target,event.owner.unit,event.tick,event.colors,event.controls);
    }
    if(damage_audio) {
      damage_audio->advance(current.tick);
      if(!world_effect_tick && !current.unit_damage_voices.empty())world_voice_serial=current.unit_damage_voices.back().serial;
      for(const auto &event:current.unit_damage_voices)if(event.serial>world_voice_serial) {
        world_voice_serial=event.serial;
        const auto target=std::find_if(current.players.begin(),current.players.end(),[&](const auto &p) {
          return p.connected && (p.source_unit==event.target || p.source_previous_unit==event.target);
        });
        if(target!=current.players.end())damage_audio->damage(biped_for(*target),event);
      }
      for(unsigned view=0;view<ids.size();++view) {
        const auto &p=current.players[ids[view]];
        damage_audio->hud(view,p.source_unit,p.dead || !p.connected,p.health,p.shield,
            biped_for(p).vitality.shield,p.shield_recharging,multiplayer_representation(p.source_configuration)==3);
      }
    }
    world_effects.advance(current.tick,tick_seconds);
    for(const auto &sound:world_effects.take_sounds())
      if(audio)try{audio->impulse_at(sound.sound,sound.position,sound.gain,
          MultiplayerAudioKind::Impulse,sound.owner);}
      catch(const FormatError &error){
        if(reported_world_effect_gaps.insert(error.what()).second)
          std::cerr<<"Authored world effect audio: "<<error.what()<<'\n';
      }
    world_effect_tick=current.tick;
  };
  std::array<ObjectLightingCache,multiplayer_capacity> object_light_cache;
  std::array<std::uint32_t,multiplayer_capacity> object_light_units;
  object_light_units.fill(null_datum);
  std::map<std::uint32_t,ObjectLightingCache> corpse_light_cache;
  std::map<std::uint32_t,ObjectLightingCache> vehicle_light_cache;
  std::map<std::uint32_t,std::array<std::int8_t,4>> vehicle_previous_lods;
  std::array<ShadowCasterSphere,multiplayer_capacity> object_owner_spheres;
  std::array<bool,multiplayer_capacity> aim_alive{};
  const auto update_aim=[&](const MultiplayerState &state,unsigned elapsed_ticks) {
    for(unsigned id=0;id<multiplayer_capacity;++id) {
      const auto &p=state.players[id];
      if(!p.connected || p.dead){aim_alive[id]=false;continue;}
      const auto &v=species_for(p);
      const auto &player=v.player;
      const auto channels=select_native_body(p,gait[id]?gait[id]->frame():
          NativeLocomotionFrame{player.idle?&*player.idle:nullptr,0,0x0400000cu},double(state.tick));
      if(!aim_alive[id]){aim_blends[id]={};body_aim_blends[id]={};aim_alive[id]=true;}
      auto &blend=aim_blends[id];
      FirstPersonAimEligibility input;
      input.dual_wield=channels.dual;
      input.base_state=channels.primary.activity;
      input.base_animation_flags=channels.primary.clip?channels.primary.clip->authored_flags:0;
      input.secondary_action=channels.secondary_activity;
      input.secondary_animation_flags=channels.secondary?channels.secondary->authored_flags:0;
      const auto image=cache.original_data();
      if(!image || !image->unit_constructor_tail_constants)throw FormatError("Missing original body aim transition operands");
      const auto &constants=*image->unit_constructor_tail_constants;
      const bool body_eligible=!((input.base_animation_flags|input.secondary_animation_flags)&16);
      const auto clock=[] {
        SourceLocalTransitionClock c{{},0x27f,0x1f80};c.record[2]=std::byte(30);
        const auto dt=std::bit_cast<std::uint32_t>(1.f/30.f);
        for(unsigned i=0;i<4;++i)c.record[4+i]=std::byte(dt>>(i*8));
        return c;
      };
      for(unsigned tick=0;tick<elapsed_ticks;++tick) {
        source_aim_blend_advance(blend);
        source_aim_blend_select(blend,source_weapon_aim_eligible(input),30);
        //112D50 bit4 and113830's90 slot have their own transition speed;
        // the support-hand80 slot above uses1112F0's separate action gate.
        auto &body_blend=body_aim_blends[id];source_aim_blend_advance(body_blend);
        start_source_transition_counter(std::as_writable_bytes(std::span(&body_blend,1)),
            body_eligible?constants.normal_blend_bits:constants.special_blend_bits,
            body_eligible,constants.one_bits,clock);
      }
    }
  };
  std::optional<std::uint32_t> muzzle_pose_tick;
  std::vector<std::array<std::uint32_t,3>> muzzle_pose_owners;
  const auto update_muzzle_effects=[&] {
    std::vector<std::array<std::uint32_t,3>> owners;
    for(unsigned view=0;view<ids.size();++view) {
      const auto &binding=client?client_fp_views[view]:match.first_person_view(view);
      owners.push_back({binding.unit,binding.hands[0].weapon,binding.hands[1].weapon});
    }
    if(muzzle_pose_tick==current.tick && owners==muzzle_pose_owners)return;
    muzzle_pose_tick=current.tick;muzzle_pose_owners=std::move(owners);
    std::vector<std::uint32_t> attached_weapons;
    for(unsigned view=0;view<ids.size();++view) {
      const auto &primary=current.players[ids[view]];
      if(!primary.connected || primary.source_unit==null_datum)continue;
      const auto &registered=client?client_fp_views[view]:match.first_person_view(view);
      const auto &biped=biped_for(primary);
      const auto camera=options.capture_camera.value_or(player_camera(primary,biped,biped.camera_field_of_view,content.player_screen_offset));
      const auto transform=camera_transform(camera.position,camera.forward,camera.up);
      MuzzleEffectView effects_view{};
      effects_view.position=camera.position;effects_view.forward=camera.forward;effects_view.up=camera.up;
      Transform primary_adjustment;
      for(unsigned hand_index=0;hand_index<2;++hand_index) {
        const auto &hand=registered.hands[hand_index];
        if(hand.weapon==null_datum)continue;
        auto player=primary;
        if(hand_index) {
          const auto &secondary=primary.secondary_weapon;
          if(secondary.datum!=hand.weapon)throw FormatError("FP emission hand differs from its authoritative weapon");
          player.weapon=secondary.definition;player.source_weapon=secondary.datum;
          player.loaded=secondary.loaded;player.ammunition_available=secondary.ammunition_available;
          player.reload_tick=secondary.reload_tick;
        }
        const auto &visual=species_for(player);const auto &weapon=content.weapons.at(player.weapon);
        if(!primary.dead)if(const auto attached=fp_weapon_attachments.find(player.weapon);attached!=fp_weapon_attachments.end()) {
          const auto &shader=hand_index?primary.secondary_weapon.shader:primary.weapon_shader;
          for(const auto &attachment:attached->second) {
            const float a=attachment.scale_a?*source_weapon_shader_input(attachment.scale_a,shader):0.f;
            const float b=attachment.scale_b?*source_weapon_shader_input(attachment.scale_b,shader):1.f;
            muzzle_effects.attachment(attachment.effect,hand.weapon,current.tick,{a,b},
                attachment.scale_a!=0x09000568u || source_weapon_turned_on(shader).enabled);
          }
          attached_weapons.push_back(hand.weapon);
        }
        if(!visual.fp_definition || visual.effect_markers.empty())continue;
        const auto clip=visual.fp_clips.find(hand.animation_identifier>>16);
        if(clip==visual.fp_clips.end())throw FormatError("FP emission clock has no decoded original clip");
        auto binding=registered;binding.hands[0]=hand;
        auto origin=binding;
        const auto frame=first_person_render_frame(origin,binding,*visual.fp_definition,1,false).frame;
        const AnimationClip *ammunition=nullptr;float ammunition_frame{};
        if(visual.fp_definition->ammunition[hand.dual_wield?1:0]) {
          ammunition=&*visual.fp_definition->ammunition[hand.dual_wield?1:0];
          ammunition_frame=first_person_ammunition_frame(std::int16_t(player.loaded),
              std::bit_cast<std::int16_t>(std::uint16_t(player.ammunition_available)),std::int16_t(weapon.capacity),
              std::bit_cast<std::int16_t>(std::uint16_t(current.tick-player.reload_tick)),weapon.weapon_type,
              hand.activity,ammunition->frame_count);
        }
        const float aiming=primary.secondary_weapon.datum==null_datum?source_aim_blend_weight(aim_blends[ids[view]]):0.f;
        const auto pose=pose_first_person_models_world_frame(visual.guns[view*2+hand_index].model,visual.hands.model,
            clip->second,frame,weapon.first_person_offset,transform,visual.fp_ik,aiming,hand_index!=0,
            hand_index?&primary_adjustment:nullptr,ammunition,ammunition_frame);
        if(!hand_index)primary_adjustment=pose.camera_adjustment;
        muzzle_effects.pose(hand.weapon,[&](std::uint32_t name) {
          const auto authored=visual.effect_markers.find(name);
          if(authored==visual.effect_markers.end())throw FormatError("FP emission has no authored marker");
          const auto markers=pose_model_markers(authored->second,pose.weapon_nodes);
          std::vector<Transform> frames;frames.reserve(markers.size());
          for(const auto &marker:markers)frames.push_back(compose_transform(inverse_transform(transform),marker.world));
          return frames;
        },effects_view,current.tick,tick_seconds);
      }
    }
    muzzle_effects.retire_attachments_except(attached_weapons);
    muzzle_effects.advance(current.tick,tick_seconds);
  };
  std::array<bool,multiplayer_capacity> object_light_alive{},object_light_reported{};
  std::array<float, 4> yaw{}, pitch{};
  std::array<Controls, 4> local{};
  std::array<Controls, 4> pad_controls{},pad_previous{},pad_pending{};
  std::array<bool,4> pad_zoom_held{},pad_zoom_pending{};
  std::array<bool,4> death_cycle_pressed{};
  bool keyboard_jump_held{};
  std::array<std::array<NativeInteractionHold,2>,4> pad_holds{};
  MultiplayerKbm keyboard(SDL_GetWindowID(platform.window), platform.desktop->keyboard_settings);
  std::array<std::int16_t,4> view_zoom{-1,-1,-1,-1};
  std::array<std::uint32_t,4> view_zoom_unit{null_datum,null_datum,null_datum,null_datum};
  std::array<std::uint32_t,4> view_zoom_reset{};
  const auto synchronize_zoom=[&] {
    for(unsigned view=0;view<ids.size();++view) {
      const auto &player=current.players[ids[view]];
      if(view_zoom_unit[view]!=player.source_unit || view_zoom_reset[view]!=player.zoom_reset_serial)
        view_zoom[view]=-1;
      view_zoom_unit[view]=player.source_unit;
      view_zoom_reset[view]=player.zoom_reset_serial;
    }
  };
  synchronize_zoom();
  const auto &scope_constants=content.original_data->unit_scope_camera_constants;
  std::array<UnitScopeCameraProducer,4> scope_producers{};
  std::array<UnitScopeCameraState,4> scope_observers{};
  std::optional<NativeVehicleCamera> vehicle_camera;
  if(world->source_objects && !client && world->source_objects->owns_physics())
    vehicle_camera.emplace(*world->source_objects,*content.original_data);
  std::optional<NativeDeathCamera> death_camera;
  if(world->source_objects)
    death_camera.emplace(*world->source_objects,*content.original_data,auxiliary_random);
  if(scope_constants)
    for(auto &observer:scope_observers)observer=source_scope_camera_initial(*scope_constants);
  bool scope_unavailable_reported{};
  std::array<bool, 4> dead{};
  std::array<std::uint32_t, multiplayer_capacity> heard_shots{};
  for (unsigned i = 0; i < ids.size(); ++i) {
    yaw[i] = current.players[ids[i]].yaw;
    pitch[i] = current.players[ids[i]].pitch;
  }
  bool running = true, joined = !client || client->ready();
  auto exit_reason = MultiplayerExitReason::frame_limit;
  MaterialFrameSelection material_frame(mesh.materials);
  std::vector<int> player_armor_owners(mesh.materials.size(),-1);
  std::vector<int> player_damage_owners(mesh.materials.size(),-1);
  std::map<std::uint32_t,std::vector<ObjectFunction>> player_damage_functions;
  const auto player_damage_function_tables=cache.shader_function_tables();
  for(const auto &[key,body]:species) {
    const auto datum=content.player_representations.at(key.first).biped.datum;
    if(!player_damage_functions.contains(datum))
      player_damage_functions.emplace(datum,cache.load_object_functions(datum));
  }
  for(const auto &[key,body]:species)
    for(unsigned player=0;player<object_slots;++player)
      for(unsigned offset=0;offset<body.player.model.mesh.materials.size();++offset) {
        auto &owner=player_armor_owners.at(body.player_materials[player]+offset);
        if(owner!=-1 && owner!=int(player))throw FormatError("Armor material has two player owners");
        owner=int(player);
        player_damage_owners.at(body.player_materials[player]+offset)=int(player);
      }
  for(const auto &[key,body]:species)
    for(const auto *layers:{&body.body_shields,&body.hand_shields})
      for(const auto &[shader,layer]:*layers)
        for(unsigned player=0;player<object_slots;++player)
          for(unsigned offset=0;offset<layer.count;++offset)
            player_damage_owners.at(layer.materials[player]+offset)=int(player);
  unsigned frames = 0, steps = 0, peak_players = 0;
  struct ScriptedEffectInput {bool fire{},reload{},grenade{};std::optional<Vec2> look;Vec2 move;};
  std::map<unsigned, std::array<std::optional<ScriptedEffectInput>,4>> scripted_inputs;
  if (!options.input_script.empty()) {
    std::ifstream file(options.input_script);
    if (!file) throw FormatError("Could not open multiplayer input script");
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      unsigned tick, player, fire, reload;
      std::string extra;
      if (!(row >> tick >> player >> fire >> reload) ||
          tick >= options.capture_tick || player >= options.local_players ||
          fire > 1 || reload > 1 || scripted_inputs[tick][player])
        throw FormatError("Invalid multiplayer input-script row");
      ScriptedEffectInput input{bool(fire),bool(reload),false,{}, {}};
      if(row>>extra){
        if(extra!="0" && extra!="1")throw FormatError("Invalid scripted grenade pulse");
        input.grenade=extra=="1";
        float yaw,pitch;
        row>>std::ws;
        if(!row.eof()){
          if(!(row>>yaw>>pitch)||!std::isfinite(yaw)||!std::isfinite(pitch)||pitch < -1.5f || pitch>1.5f)
            throw FormatError("Invalid scripted look input");
          input.look=Vec2{yaw,pitch};
        }
        if(row>>extra){
          if(extra!="move" || !(row>>input.move.x>>input.move.y) ||
              !std::isfinite(input.move.x) || !std::isfinite(input.move.y) ||
              std::abs(input.move.x)>1.f || std::abs(input.move.y)>1.f)
            throw FormatError("Invalid scripted movement input");
          if(row>>extra)throw FormatError("Extra multiplayer input-script columns");
        }
      }
      scripted_inputs[tick][player] = input;
    }
  }
  bool capture_ready = !options.capture_players;
  double accumulator = 0;
  // Explicit native elapsed admission: reuse this match loop's existing
  // clamped platform frame duration. The source clock remains absolute-double
  // and view0-owned; this is not an Xbox vblank/governor implementation.
  double weather_realtime=0;
  SoundClassFades peer_sound_classes;
  bool peer_game_over_fade_started=false;
  // Retain staging capacity; rebuild every frame from the current source state.
  std::vector<Vertex> hud_vertices;
  std::vector<std::vector<Vertex>> worlds(options.local_players),
      foregrounds(options.local_players),backgrounds(options.local_players),
      reflection_backgrounds(options.local_players);
  std::vector<std::vector<SourceWeatherPointDraw>> weather_points(options.local_players);
  std::vector<std::vector<SourceWeatherLineDraw>> weather_lines(options.local_players);
  std::vector<std::vector<SourceWeatherPlateDraw>> weather_plates(options.local_players);
  platform.desktop->renderer_ready(renderer_owner);
  auto last = Clock::now(), snapshot_time = last;
  std::array<GameVoicePlayer,16> voice_players{};
  for(auto &p:voice_players)p.disabled=true;
  std::array<unsigned,16> voice_sequences{};
  std::array<double,16> voice_credits{};voice_credits.fill(10);
  auto voice_credit_at=Clock::now(),voice_preferences_at=last;
  bool voice_preferences_sent{};
  GameVoiceSnapshot voice_previous;
  GameVoiceSpeaker voice_speaker;
  std::array<AutoCenterState,4> auto_center;std::array<float,4> prior_input_pitch{};
  unsigned mute_target{};
  std::string voice_notice;
  auto voice_notice_until=last;
  const std::string game_window_title=SDL_GetWindowTitle(platform.window);
  if(!options.input_instances.empty() && options.input_instances.size()!=options.local_players)
    throw FormatError("Local input bindings must match the selected roster");
  const auto selected_pad_for=[&](unsigned view)->SDL_GameController* {
    if(!options.input_instances.empty()) {
      for(auto *pad:platform.pads)
        if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==options.input_instances[view])return pad;
      return nullptr;
    }
    const int index=options.local_players==1?0:int(view)-1;
    return index>=0 && unsigned(index)<platform.pads.size()?platform.pads[index]:nullptr;
  };
  std::array<RumblePlayer,4> rumble;
  std::map<std::uint32_t,std::shared_ptr<const RumbleDefinition>> rumble_definitions;
  const auto rumble_tables=cache.shader_function_tables();
  std::uint64_t rumble_serial=client && !current.rumble_events.empty()?current.rumble_events.back().serial:0;
  const auto rumble_events=[&](const MultiplayerState &state) {
    for(const auto &row:state.rumble_events)if(row.serial>rumble_serial) {
      rumble_serial=row.serial;
      for(unsigned view=0;view<ids.size();++view)if(ids[view]==unsigned(row.event.owner+1)) {
        auto &definition=rumble_definitions[row.event.definition];
        if(!definition)definition=std::make_shared<const RumbleDefinition>(load_rumble_definition(cache,row.event.definition));
        rumble[view].play(definition,row.event.shielded,row.event.scale);
      }
    }
  };
  std::array<bool,4> prompt_gamepad{};
  for(unsigned i=0;i<options.local_players;++i)prompt_gamepad[i]=selected_pad_for(i)!=nullptr;
  bool voice_caption{};
  auto announce_voice=[&](std::string text,Clock::time_point at) {
    voice_notice=std::move(text);voice_notice_until=at+std::chrono::seconds(3);
    SDL_SetWindowTitle(platform.window,(game_window_title+" | "+voice_notice).c_str());voice_caption=true;
    std::cout<<voice_notice<<'\n';
  };
  std::string voice_error;
  bool postgame_published{};
  std::map<std::uint32_t,ModelWorldPoseCache> retained_world_weapon_poses;
  while (running && (!options.frames || frames < options.frames)) {
    if(options.menu_audio_update)options.menu_audio_update();
    if(options.online_match) {
      const auto phase=options.online_match->phase();
      if(phase==OnlineMatchSession::Phase::Failed){exit_reason=MultiplayerExitReason::online_disconnected;break;}
      if(phase==OnlineMatchSession::Phase::Reviewing) {
        if(client){client->receive();current=client->state();}
        try {
          online_postgame_validate(current,options.variant,options.online_match->roster(),options.online_match->postgame());
          options.online_match->confirm_result();
        } catch(const FormatError &error) {
          std::cerr<<error.what()<<'\n';exit_reason=MultiplayerExitReason::online_disconnected;break;
        }
      }
      if(phase==OnlineMatchSession::Phase::Postgame) {
        if(client){client->receive();current=client->state();}
        exit_reason=MultiplayerExitReason::online_postgame;break;
      }
      if(!client && !postgame_published && current.game_over_complete() && current.results_finalized) {
        const auto report=online_postgame_publish(match,options.variant,options.online_match->roster());
        options.online_match->complete(report.source);postgame_published=true;
      }
    }
    for(unsigned view=0;view<options.local_players;++view)if(auto *pad=selected_pad_for(view)) {
      const auto motors=rumble[view].evaluate(*rumble_tables);
      send_controller_rumble(pad,motors,platform.desktop->controller_settings.vibration,pause_menu && pause_menu->visible(),
          SDL_GetWindowFlags(platform.window)&SDL_WINDOW_INPUT_FOCUS);
    }
    std::vector<SoundEvent> events;
    const auto now = Clock::now();
    MultiplayerFrameTiming timing;timing.frame=frames;timing.views=options.local_players;
    auto measured_at=now;
    const auto measure=[&](double &value) {
      if(options.frame_observer) {
        const auto end=Clock::now();value+=std::chrono::duration<double,std::milli>(end-measured_at).count();measured_at=end;
      }
    };
    const float audio_elapsed=options.reference_tick_cadence?tick_seconds:
        std::chrono::duration<float>(now-last).count();
    const float elapsed=std::min(.1f,audio_elapsed);
    last = now;
    if(options.reference_tick_cadence)accumulator=tick_seconds;
    else accumulator += elapsed;
    weather_realtime+=double(elapsed);
    weather_world.begin_frame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      // Keep each viewport's labels on its most recently used input device.
      // Stick drift inside the gameplay dead zone must not steal KBM labels.
      if((e.type==SDL_KEYDOWN && e.key.windowID==SDL_GetWindowID(platform.window)) ||
          (e.type==SDL_MOUSEBUTTONDOWN && e.button.windowID==SDL_GetWindowID(platform.window)) ||
          (e.type==SDL_MOUSEMOTION && e.motion.windowID==SDL_GetWindowID(platform.window) &&
           (e.motion.xrel || e.motion.yrel)))prompt_gamepad[0]=false;
      if(e.type==SDL_CONTROLLERBUTTONDOWN ||
          (e.type==SDL_CONTROLLERAXISMOTION && std::abs(float(e.caxis.value)/32767.f)>=.15f)) {
        const auto instance=e.type==SDL_CONTROLLERBUTTONDOWN?e.cbutton.which:e.caxis.which;
        for(unsigned i=0;i<options.local_players;++i)if(auto *pad=selected_pad_for(i);
            pad && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==instance)prompt_gamepad[i]=true;
      }
      if(game_voice && e.type==SDL_KEYDOWN && !e.key.repeat && e.key.windowID==SDL_GetWindowID(platform.window)) {
        const auto selected=game_voice->snapshot();
        if(e.key.keysym.sym==SDLK_F6) {game_voice->enable(!selected.enabled);announce_voice(selected.enabled?"Voice off":"Voice on - tap N for team radio",now);continue;}
        if(e.key.keysym.sym==SDLK_F7) {game_voice->microphone_muted(!selected.microphone_muted);announce_voice(selected.microphone_muted?"Microphone on":"Microphone muted",now);continue;}
        if(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_TAB] &&
           (e.key.keysym.sym==SDLK_UP || e.key.keysym.sym==SDLK_DOWN || e.key.keysym.sym==SDLK_m)) {
          const auto &roster=options.online_match->roster();
          if(e.key.keysym.sym==SDLK_UP)mute_target=(mute_target+roster.size()-1)%roster.size();
          if(e.key.keysym.sym==SDLK_DOWN)mute_target=(mute_target+1)%roster.size();
          if(e.key.keysym.sym==SDLK_m && mute_target!=options.online_match->self())game_voice->mute(mute_target,!(selected.muted&(1u<<mute_target)));
          announce_voice(roster[mute_target].name+((game_voice->snapshot().muted&(1u<<mute_target))?" - muted":" - voice on"),now);continue;
        }
      }
      if(renderer.debug_panel().process_event(e)) {
        keyboard.release();
        pad_holds={};
        continue;
      }
      if(pause_menu && pause_menu->event(e)) {
        keyboard.release();pad_holds={};pad_pending={};pad_controls={};pad_zoom_pending={};pad_zoom_held={};
        continue;
      }
      keyboard.handle_event(e);
      if (e.type == SDL_QUIT) {
        running = false;
        exit_reason = MultiplayerExitReason::window_quit;
      } else if (e.type == SDL_WINDOWEVENT &&
               e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        renderer.resized();
      else if (e.type == SDL_CONTROLLERDEVICEADDED ||
               e.type == SDL_CONTROLLERDEVICEREMOVED) {
        pad_holds={};
        platform.refresh();
        for(unsigned i=0;i<options.local_players;++i)prompt_gamepad[i]=selected_pad_for(i)!=nullptr;
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.windowID == SDL_GetWindowID(platform.window) &&
               (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT))
        SDL_SetRelativeMouseMode(SDL_TRUE);
      else if (e.type == SDL_MOUSEMOTION && e.motion.windowID == SDL_GetWindowID(platform.window) && SDL_GetRelativeMouseMode()) {
        // Native SDL look gain retains its existing sensitivity3 baseline.
        const auto &kbm=platform.desktop->keyboard_settings;
        const float gain=.0025f*float(kbm.sensitivity)/3.f;
        yaw[0] -= e.motion.xrel * gain;
        pitch[0] = std::clamp(pitch[0] - e.motion.yrel * gain * (kbm.invert ? -1.f : 1.f), -1.5f, 1.5f);
      } else if (e.type == SDL_KEYDOWN && e.key.windowID == SDL_GetWindowID(platform.window) && e.key.keysym.sym == SDLK_ESCAPE) {
        if (SDL_GetRelativeMouseMode())
          SDL_SetRelativeMouseMode(SDL_FALSE);
        else {
          running = false;
          exit_reason = MultiplayerExitReason::escape;
        }
      }
    }
    if(pause_menu && pause_menu->quit_requested()) {
      running=false;exit_reason=MultiplayerExitReason::escape;
    }
    keyboard.settings(platform.desktop->keyboard_settings);
    if(game_voice) {
      game_voice->blocks(options.online_match->voice_blocks());
      const bool focused=(SDL_GetWindowFlags(platform.window)&SDL_WINDOW_INPUT_FOCUS)!=0 && !renderer.debug_panel().visible() &&
          !(pause_menu && pause_menu->visible());
      game_voice->focus(focused);options.online_match->update_postgame_voice(focused);
      const auto self=options.online_match->self();
      const auto &p=current.players[self];
      // Authoritative eligibility is rechecked by the host before delivery.
      const auto age=p.source_death_tick==null_datum ? std::nullopt : std::optional(float(current.tick-p.source_death_tick)/30);
      game_voice->radio(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_N],game_voice_active(p.connected,p.source_unit!=null_datum,p.source_previous_unit!=null_datum,age));
      const auto selected=game_voice->snapshot();
      if(host) {
        voice_players[self].disabled=!selected.enabled;
        voice_players[self].muted=selected.muted;
      } else if(!voice_preferences_sent || selected.enabled!=voice_previous.enabled || selected.muted!=voice_previous.muted || now-voice_preferences_at>std::chrono::seconds(1)) {
        options.online_match->voice_preferences(options.online_match->host(),selected.enabled,selected.muted);
        voice_preferences_sent=true;voice_preferences_at=now;
      }
      voice_previous=selected;
      const double dt=std::chrono::duration<double>(now-voice_credit_at).count();voice_credit_at=now;
      for(auto &credit:voice_credits)credit=std::min(10.,credit+dt*50);
      auto route=[&](GameVoiceFrame frame) {
        if(frame.source>=16 || frame.sequence<=voice_sequences[frame.source] || voice_credits[frame.source]<1)return;
        voice_sequences[frame.source]=frame.sequence;voice_credits[frame.source]-=1;
        for(unsigned id=0;id<16;++id) {
          const auto &player=match.player(id);const auto &unit=match.unit(id);
          auto &voice=voice_players[id];voice.connected=player.connected;voice.team=player.source_membership;
          const auto age=unit.source_death_tick==null_datum ? std::nullopt : std::optional(float(current.tick-unit.source_death_tick)/30);
          voice.active=game_voice_active(player.connected,player.source_current_object!=null_datum,player.source_previous_unit!=null_datum,age);
          voice.position=current.players[id].position;voice.yaw=unit.yaw;
          if(id<options.online_match->roster().size()) {
            const auto peer=online_admission_peer(options.online_match->roster()[id]);
            voice.disabled=voice_players[peer].disabled;voice.muted=voice_players[peer].muted;
          }
        }
        // Shared PC speakers receive each encoded frame once. A nearby local
        // listener may admit proximity audio even when the primary is farther
        // away; keep the strongest channel gains instead of duplicating Opus.
        std::array<std::optional<GameVoiceGains>,16> delivery;
        const auto source_peer=online_admission_peer(options.online_match->roster()[frame.source]);
        for(const auto &listener:options.online_match->roster()) {
          const auto peer=online_admission_peer(listener);if(peer==source_peer)continue;
          if(auto gains=game_voice_route(frame.source,listener.slot,voice_players,frame.radio,rules.team_mode,false,voice_range)) {
            auto &chosen=delivery[peer];if(!chosen)chosen=*gains;
            else {chosen->left=std::max(chosen->left,gains->left);chosen->right=std::max(chosen->right,gains->right);}
          }
        }
        for(unsigned peer=0;peer<delivery.size();++peer)if(delivery[peer]) {
          frame.gains=*delivery[peer];if(peer==self)game_voice->receive(frame);else options.online_match->voice(peer,frame);
        }
      };
      for(auto &packet:options.online_match->voice_packets()) {
        if(host) {
          if(packet.peer==self || packet.peer>=voice_players.size())continue;
          if(packet.preferences) {voice_players[packet.peer].disabled=!packet.enabled;voice_players[packet.peer].muted=packet.muted;}
          else {packet.frame.source=packet.peer;route(std::move(packet.frame));}
        } else if(packet.peer==options.online_match->host() && !packet.preferences)game_voice->receive(std::move(packet.frame));
      }
      for(auto &frame:game_voice->outgoing()) {frame.source=self;if(host)route(std::move(frame));else options.online_match->voice(options.online_match->host(),frame);}
      if(selected.error!=voice_error) {voice_error=selected.error;if(!voice_error.empty())announce_voice(voice_error,now);}
      if(voice_caption && now>=voice_notice_until){SDL_SetWindowTitle(platform.window,game_window_title.c_str());voice_caption=false;}
      std::string text;
      if(auto id=voice_speaker.update(selected.talking,self);id && current.players[*id].connected)text=current.players[*id].name;
      for(auto &[representation,hud]:unit_huds)hud->voice_text(text);
    }
    for (unsigned i = 0; i < options.local_players; ++i) {
      local[i] = {};
      pad_controls[i] = {};
      death_cycle_pressed[i]=false;
      if (i == 0) {
        local[i] = keyboard.snapshot(yaw[i], pitch[i]).controls;
        death_cycle_pressed[i]=local[i].jump && !keyboard_jump_held;
        keyboard_jump_held=local[i].jump;
      }
      if(renderer.debug_panel().captures_game_input() || (pause_menu && pause_menu->captures_game_input())) {
        local[i]={0,0,yaw[i],pitch[i]};
        death_cycle_pressed[i]=false;
        continue;
      }
      auto *selected_pad=selected_pad_for(i);
      bool pad_look_active=false;
      if(selected_pad) {
        auto *pad=selected_pad;
        auto &pad_input = pad_controls[i];
        const auto mapped=controller_input(sample_controller(pad),platform.desktop->controller_settings);
        pad_look_active=mapped.forward || mapped.right || mapped.look_yaw || mapped.look_pitch;
        pad_input.forward=mapped.forward;pad_input.right=mapped.right;
        local[i].forward=std::clamp(local[i].forward+pad_input.forward,-1.f,1.f);
        local[i].right=std::clamp(local[i].right+pad_input.right,-1.f,1.f);
        controller_look(mapped,platform.desktop->controller_settings,elapsed,yaw[i],pitch[i]);
        pad_zoom_pending[i]|=mapped.zoom&&!pad_zoom_held[i];pad_zoom_held[i]=mapped.zoom;
        pad_input.jump=mapped.jump;death_cycle_pressed[i]|=pad_input.jump&&!pad_previous[i].jump;
        pad_input.crouch=mapped.crouch;pad_input.reload=mapped.reload;pad_input.fire=mapped.fire;
        const bool dual=i<ids.size()&&current.players[ids[i]].secondary_weapon.datum!=null_datum;
        pad_input.fire_secondary=dual&&mapped.secondary;pad_input.grenade=!dual&&mapped.grenade;
        pad_input.melee=mapped.melee;pad_input.switch_grenade=mapped.switch_grenade;
        pad_input.pickup=mapped.pickup;pad_input.dual_wield=mapped.dual_wield;pad_input.switch_weapon=mapped.switch_weapon;
        const auto held_pad=pad_input;
        const auto input_now=SDL_GetTicks();
        pad_holds[i][0].set(pad_input.pickup,input_now);pad_holds[i][1].set(pad_input.dual_wield,input_now);
        const auto pickup=pad_holds[i][0].sample(input_now),dual_pickup=pad_holds[i][1].sample(input_now);
        pad_input.pickup=pickup.first;pad_input.pickup_milliseconds=pickup.second;
        pad_input.dual_wield=dual_pickup.first;pad_input.dual_wield_milliseconds=dual_pickup.second;
        // Original187610 defers the shared secondary/switch button
        // until release. The authority suppresses a consumed dual hold.
        pad_input.switch_weapon = !held_pad.switch_weapon && pad_previous[i].switch_weapon;
        pad_input.grenade &= !pad_previous[i].grenade;
        pad_input.melee &= !pad_previous[i].melee;
        pad_input.switch_grenade &= !pad_previous[i].switch_grenade;
        pad_previous[i]=held_pad;
        pad_pending[i].switch_grenade|=pad_input.switch_grenade;
        pad_input.switch_grenade=pad_pending[i].switch_grenade;
        pad_pending[i].melee|=pad_input.melee;
        pad_input.melee=pad_pending[i].melee;
        pad_pending[i].switch_weapon|=pad_input.switch_weapon;pad_pending[i].grenade|=pad_input.grenade;
        pad_input.switch_weapon=pad_pending[i].switch_weapon;pad_input.grenade=pad_pending[i].grenade;
        local[i].pickup|=pad_input.pickup;local[i].dual_wield|=pad_input.dual_wield;
        local[i].pickup_milliseconds=std::max(local[i].pickup_milliseconds,pad_input.pickup_milliseconds);
        local[i].dual_wield_milliseconds=std::max(local[i].dual_wield_milliseconds,pad_input.dual_wield_milliseconds);
        local[i].switch_weapon|=pad_input.switch_weapon;local[i].grenade|=pad_input.grenade;
        local[i].fire_secondary|=pad_input.fire_secondary;
        local[i].melee|=pad_input.melee;local[i].switch_grenade|=pad_input.switch_grenade;
        local[i].jump |= pad_input.jump;
        local[i].crouch |= pad_input.crouch;
        local[i].reload |= pad_input.reload;
        local[i].fire |= pad_input.fire;
      }
      const bool center=(i!=0 || pad_look_active)?platform.desktop->controller_settings.auto_center:platform.desktop->keyboard_settings.auto_center;
      const float speed=i<ids.size()?length(current.players[ids[i]].velocity):0.f;
      auto_center_look(auto_center[i],center,local[i].forward,pitch[i]-prior_input_pitch[i],
          local[i].crouch,speed,elapsed,content.auto_level_scale,content.auto_level_minimum_ticks,pitch[i]);
      prior_input_pitch[i]=pitch[i];
      yaw[i] = std::remainder(yaw[i], 6.283185307f);
      local[i].yaw = yaw[i];
      local[i].pitch = pitch[i];
      if (options.replay) {
        local[i].forward = .5f;
        local[i].yaw = yaw[i] + std::sin(float(steps) / 120) * .1f;
        local[i].fire = steps % 20 == 0;
      }
    }
    auto& panel=renderer.debug_panel();
    if(!host && !client) {
      DebugGameSettings settings;
      for(const auto id:ids)settings.players.at(id)=panel.game_settings().players.at(id);
      match.debug_settings(settings);
      while(const auto request=panel.take_request()) {
        try {
          if(std::find(ids.begin(),ids.end(),request->player)==ids.end())
            throw FormatError("debug controls require a local player");
          if(request->kind==DebugGameRequest::Teleport) {
            const auto observation=observe_debug_game(match.simulation(),cache.spawns());
            if(request->index>=observation.teleports.size())throw FormatError("debug spawn selection expired");
            match.debug_request(*request,&observation.teleports[request->index].spawn);
            const auto view=std::find(ids.begin(),ids.end(),request->player)-ids.begin();
            yaw[view]=match.unit(request->player).yaw;pitch[view]=0;
            local[view].yaw=yaw[view];local[view].pitch=0;
          } else match.debug_request(*request);
          panel.status("Debug request applied.");
        } catch(const std::exception& error){panel.status(error.what());}
      }
    } else {
      match.debug_settings({});
      while(panel.take_request())panel.status("Gameplay debug controls are available in local sessions.");
    }
    if (client) {
      client->receive();
      if (!client->error().empty())
        throw FormatError(client->error());
      if (client->ready()) {
        if (!joined) {
          ids.assign(client->slots().begin(), client->slots().end());
          joined = true;
          // Retained host events from before admission are not new local
          // effects. Baseline the journal before consuming the join snapshot.
          if(!client->state().rumble_events.empty())rumble_serial=client->state().rumble_events.back().serial;
          for (unsigned i = 0; i < ids.size(); ++i) {
            yaw[i] = client->state().players[ids[i]].yaw;
            pitch[i] = 0;
          }
          std::cout << "LAN joined slots=";
          for (auto id : ids)
            std::cout << id << ',';
          std::cout << std::endl;
        }
        for(const auto &received:client->received_states()) {
          device_event_audio(received.device_events);rumble_events(received);
          if(received.tick==current.tick && current.players[ids[0]].connected){
            current.devices=received.devices;current.device_events=received.device_events;continue;
          }
          previous = current;
          current = received;
          capture_source_body_poses();
          if(current.source_scene_roots_authoritative) {
            match.replicate_source_scene(current.source_scene_roots,current.source_body_poses);
            scene_interpolation.capture(current.tick,match.simulation().scene().objects());
          }
          update_audio_listeners();
          update_sensors();
          // A frame may drain several genuinely accepted host ticks. Consume
          // each changed shot owner once before publishing the newest state;
          // skipped network ticks never supply invented firing events.
          if(audio)for(unsigned id=0;id<multiplayer_capacity;++id) {
            const auto &player=current.players[id];
            const auto &prior_player=previous.players[id];
            if(player.connected && (!prior_player.connected ||
                prior_player.source_player!=player.source_player)) {
              // A newly observed player owns a retained last-shot stamp, not
              // a history of audible events. Establish its lifetime baseline;
              // respawning the same player does not restart that baseline.
              heard_shots[id]=player.shot_tick;
              continue;
            }
            if(player.connected && player.shot_tick && player.shot_tick!=heard_shots[id]) {
              heard_shots[id]=player.shot_tick;
              const auto &fired_weapon=content.weapons.at(player.weapon);
              const bool local_effect=std::any_of(fired_weapon.firing_effects.begin(),fired_weapon.firing_effects.end(),
                  [&](const auto &row){return world_firing_effects.contains(row.effects[0]);}) ||
                  ((fired_weapon.smg || fired_weapon.firing_presentation) && std::find(ids.begin(),ids.end(),id)!=ids.end());
              if(fired_weapon.sound!=null_datum && !local_effect)
                events.push_back({fired_weapon.sound,player.position});
            }
          }
          synchronize_zoom();
          for(unsigned id=0;id<multiplayer_capacity;++id)if(current.players[id].connected) {
            if(previous.tick && current.tick-previous.tick!=1)gait[id].reset();
            update_native_gait(id);
          }
          update_world_effects(true);
          if(!local_definitions.empty())
            for(unsigned view=0;view<ids.size();++view) {
              const auto &fp_definition=species_for(current.players[ids[view]]).fp_definition;
              const auto &player=current.players[ids[view]];
              FirstPersonBindingInput binding_input{player.source_player,player.source_unit,
                  int(multiplayer_representation(player.source_configuration)),
                  {player.source_weapon,player.secondary_weapon.datum}};
              observe_first_person_binding(client_fp_views[view],binding_input,[&](auto datum,auto representation)->const FirstPersonBindingDefinition * {
                const auto definition=datum==player.source_weapon?player.weapon:player.secondary_weapon.definition;
                const auto found=species.find({unsigned(representation),definition});
                return found==species.end()?nullptr:&*found->second.fp_definition;
              });
              client_fp_clock_available[view]=apply_replicated_first_person_clock(client_fp_origins[view],
                  client_fp_views[view],current.players[ids[view]].first_person_clock,*fp_definition);
              client_secondary_clock_available[view]=false;
              if(player.secondary_weapon.datum!=null_datum) {
                auto origin=client_fp_origins[view],binding=client_fp_views[view];
                binding.hands[0]=binding.hands[1];origin.hands[0]=origin.hands[1];
                const auto &definition=*species.at({unsigned(binding.representation),player.secondary_weapon.definition}).fp_definition;
                client_secondary_clock_available[view]=apply_replicated_first_person_clock(origin,binding,player.secondary_weapon.clock,definition);
                client_fp_origins[view].hands[1]=origin.hands[0];client_fp_views[view].hands[1]=binding.hands[0];
                if(audio && client_secondary_clock_available[view]) {
                  auto event_hand=origin.hands[0];
                  first_person_animation_clock(event_hand,definition,tick_seconds,[&](const FirstPersonAnimationEvent &event){
                    try{audio->first_person(view,binding,event);}catch(const std::exception &e){std::cerr<<"Offhand audio: "<<e.what()<<'\n';}
                  });
                }
              }
              if(audio && client_fp_clock_available[view]) {
                // The received pair independently proves this one source tick.
                // Replay its events once on a private channel copy; never run
                // a duplicate authority clock or guess across lost intervals.
                auto event_hand=client_fp_origins[view].hands[0];
                first_person_animation_clock(event_hand,*fp_definition,tick_seconds,
                    [&](const FirstPersonAnimationEvent &event){
                      try{audio->first_person(view,client_fp_views[view],event);}
                      catch(const std::exception &e){std::cerr<<"LAN first-person audio: "<<e.what()<<'\n';}
                    });
              }
              if(client_fp_views[view].unit!=client_fp_reported_units[view]) {
                client_fp_reported_units[view]=client_fp_views[view].unit;
                std::cout<<"LAN source binding view="<<view<<" player="<<current.players[ids[view]].source_player
                    <<" unit="<<client_fp_views[view].unit<<" weapon="<<client_fp_views[view].hands[0].weapon
                    <<" flags="<<client_fp_views[view].hands[0].flags<<'\n';
              }
            }
          // Each accepted snapshot owns its emitted effect. Consuming only
          // the final render state loses intermediate weapon sounds.
          consume_client_weapon_effects();
          update_aim(current,current.tick-previous.tick);
          snapshot_time = now;
        }
      }
    }
    while (accumulator >= tick_seconds) {
      if (options.capture_tick && current.tick >= options.capture_tick) break;
      bool scoreboard_requested=false;
      if (!options.replay && options.input_script.empty() && options.local_players) {
        // Consume taps only when delivering a simulation command. An idle
        // attached pad must not clear keyboard/mouse actions or movement.
        const auto input = keyboard.consume(yaw[0], pitch[0]);
        scoreboard_requested=input.actions.scoreboard;
        local[0].jump = input.controls.jump || pad_controls[0].jump;
        local[0].reload = input.controls.reload || pad_controls[0].reload;
        const bool dual=!ids.empty() && current.players[ids[0]].secondary_weapon.datum!=null_datum;
        const bool inverted=platform.desktop->keyboard_settings.dual_wield_invert;
        local[0].fire = (dual?(inverted?input.actions.dual_secondary:input.actions.dual_primary):input.controls.fire) || pad_controls[0].fire;
        local[0].grenade = input.controls.grenade || pad_controls[0].grenade;
        local[0].melee=input.controls.melee || pad_controls[0].melee;
        local[0].pickup=input.controls.pickup || pad_controls[0].pickup;
        local[0].dual_wield=input.controls.dual_wield || pad_controls[0].dual_wield;
        local[0].pickup_milliseconds=std::max(input.controls.pickup_milliseconds,pad_controls[0].pickup_milliseconds);
        local[0].dual_wield_milliseconds=std::max(input.controls.dual_wield_milliseconds,pad_controls[0].dual_wield_milliseconds);
        local[0].switch_weapon=input.controls.switch_weapon || pad_controls[0].switch_weapon;
        local[0].fire_secondary=(dual && (inverted?input.actions.dual_primary:input.actions.dual_secondary)) || pad_controls[0].fire_secondary;
        //1884E0's ordinary valid-unit path rechecks C9920 even without a
        // scope edge. Its retained view selector is separate from unit CURRENT.
        for(unsigned view=0;view<ids.size();++view) {
          const auto &player=current.players[ids[view]];
          const auto weapon=content.weapons.find(player.weapon);
          if(player.source_unit!=null_datum && weapon!=content.weapons.end() && weapon->second.smg) {
            const std::array<std::int16_t,1> modes{player.source_zoom_magazine_mode};
            const ZoomWeaponView primary{weapon->second.zoom,modes};
            if(!source_zoom_admitted({player.source_parent,player.source_action_word,{&primary,nullptr}}))
              view_zoom[view]=-1;
          }
        }
        for(unsigned view=0;view<ids.size();++view)if(((view==0 && input.actions.scope_pressed)||pad_zoom_pending[view]) && current.players[ids[view]].secondary_weapon.datum==null_datum) {
          const auto &player=current.players[ids[view]];
          const auto weapon=content.weapons.find(player.weapon);
          if(scope_constants && content.binocular_zoom && weapon!=content.weapons.end() && weapon->second.smg) {
            // C9CC0 reads the retained local-view selector. Runtime magazine
            // mode stays authoritative; producer admission is handled below.
            const std::array<std::int16_t,1> modes{player.source_zoom_magazine_mode};
            const ZoomWeaponView primary{weapon->second.zoom,modes};
            if(player.source_unit!=null_datum && source_zoom_admitted(
                {player.source_parent,player.source_action_word,{&primary,nullptr}}))
              view_zoom[view]=source_zoom_next(view_zoom[view],&primary,*content.binocular_zoom);
          }else if(!scope_unavailable_reported) {
            std::cerr<<"Scope unavailable: selected executable/weapon lacks a verified live scope owner\n";
            scope_unavailable_reported=true;
          }
        }
      }
      for(unsigned view=0;view<ids.size();++view)
        scoreboard.step(view,view==0 && scoreboard_requested,current,ids[view]);
      for(unsigned view=0;view<options.local_players;++view) {
        local[view].zoom=view_zoom[view];
        local[view].zoom_reset_serial=view_zoom_reset[view];
      }
      if (!options.input_script.empty()) {
        for (unsigned i = 0; i < options.local_players; ++i) {
          local[i] = {};
          local[i].yaw = yaw[i];
          local[i].pitch = pitch[i];
          const auto at = scripted_inputs.find(steps);
          if (at != scripted_inputs.end() && at->second[i]) {
            const auto&input=*at->second[i];
            local[i].fire=input.fire;local[i].reload=input.reload;local[i].grenade=input.grenade;
            local[i].forward=input.move.x;local[i].right=input.move.y;
            if(input.look){yaw[i]=local[i].yaw=input.look->x;pitch[i]=local[i].pitch=input.look->y;}
          }
        }
      }
      if (client)
        client->send(std::span(local).first(options.local_players));
      else {
        for (unsigned i = 0; i < ids.size(); ++i)
          controls[ids[i]] = local[i];
        if (host)
          host->receive(controls);
        previous = current;
        match.step(controls);
        for(const auto &event:match.take_prop_events())if(event.effect!=null_datum)try {
          effect_sounds.spawn(event.effect,event.handle,event.effect_position);
        }catch(const std::exception &error){std::cerr<<"Authored prop effect sound: "<<error.what()<<'\n';}
        // World-effect playback consumes each authoritative detonation once
        // on the game clock, including its sound parts. Drain the separate
        // gameplay callback bank without replaying a second audio timeline.
        (void)match.take_grenade_events();
        current = match.state();
        update_audio_listeners();
        device_event_audio(current.device_events);rumble_events(current);
        scene_interpolation.capture(current.tick,match.simulation().scene().objects());
        capture_source_body_poses();
        update_sensors();
        for(unsigned id=0;id<multiplayer_capacity;++id)
          if(current.players[id].connected)update_native_gait(id);
        update_world_effects();
        synchronize_zoom();
        update_aim(current,1);
        update_muzzle_effects();
        if (host)
          host->broadcast();
      }
      if (options.input_observer)
        options.input_observer(steps, std::span(local).first(options.local_players), current);
      for(auto &pending:pad_pending)pending={};
      pad_zoom_pending={};
      for(auto &holds:pad_holds)for(auto &hold:holds)hold.consume();
      for(auto &pad:pad_controls)pad.pickup=pad.dual_wield=pad.switch_weapon=pad.grenade=pad.melee=pad.switch_grenade=false;
      for(auto &command:local)command.pickup=command.dual_wield=command.switch_weapon=command.grenade=command.melee=command.switch_grenade=false;
      for(auto &player:rumble)player.advance(tick_seconds);
      accumulator -= tick_seconds;
      ++steps;
      if(!client && options.session_world_update && options.session_world_update(match)) {
        running=false;
        exit_reason=MultiplayerExitReason::session_world_close;
        break;
      }
    }
    if(!running)break; // The session owner has already requested world close.
    if(client && joined) {
      // The first authoritative bank establishes this client's lifetime
      // baseline. A late join must not replay earlier explosions. Later
      // requests carry the actual emitted effect and pose, not a definition
      // lookup or a guess from a missing object.
      for(const auto &event:current.prop_removals)
        if(heard_prop_effects.insert(event.handle).second && client_prop_effect_baseline &&
            event.effect!=null_datum)try {
          effect_sounds.spawn(event.effect,event.handle,event.effect_position);
        }catch(const std::exception &error){std::cerr<<"LAN prop effect sound: "<<error.what()<<'\n';}
      client_prop_effect_baseline=true;
    }
    // Source125F40/125B40 advances saved class envelopes with audio elapsed
    // wall time. A peer consumes the authority's one-shot fade flag, rather
    // than starting another countdown from local packet-arrival time.
    if (!client)
      events = match.take_sounds();
    if(!client)match.advance_audio(audio_elapsed);
    else {
      if(current.game_over_fade_started && !peer_game_over_fade_started) {
        source_game_over_fade(peer_sound_classes,true,*content.original_data->game_over_constants);
        peer_game_over_fade_started=true;
      }
      peer_sound_classes.advance(audio_elapsed);
    }
    if(audio)audio->master_volume(float(platform.desktop->display_settings.volume)/10.f);
    if(audio)audio->class_gains(client?peer_sound_classes.gains():match.simulation().sound_classes().gains());
    if(options.session_audio_update)options.session_audio_update(
        float(platform.desktop->display_settings.volume)/10.f,match.simulation().sound_classes().gains());
    if (audio)
      for (const auto &sound : events)
        try {
          if(sound.action==SoundEvent::Reset)audio->stop();
          else if(sound.action==SoundEvent::Impulse)audio->impulse_at(sound.datum,sound.position,sound.gain);
        } catch (const std::exception &e) {
          std::cerr << "Multiplayer sound: " << e.what() << '\n';
        }
    if(audio)for(const auto &[datum,owner]:weapon_effect_sounds)try {
      audio->impulse(datum,1,MultiplayerAudioKind::Impulse,owner);
    }catch(const std::exception &e){std::cerr<<"Weapon event sound: "<<e.what()<<'\n';}
    weapon_effect_sounds.clear();
    effect_sounds.advance(audio_elapsed,[&](const EffectSoundCue &cue,std::uint32_t owner,Vec3 position){
      if(audio)try{audio->impulse_at(cue.sound,position,cue.gain,MultiplayerAudioKind::Impulse,owner);}
      catch(const std::exception &error){std::cerr<<"Prop effect sound: "<<error.what()<<'\n';}
    });
    if(audio)audio->update();
    if (!joined) {
      SDL_Delay(5);
      continue;
    }
    const auto present_players = unsigned(std::count_if(
        current.players.begin(), current.players.end(),
        [](const auto &p) { return p.connected; }));
    peak_players = std::max(peak_players, present_players);
    if (!capture_ready) {
      if (present_players < options.capture_players) {
        SDL_Delay(5);
        continue;
      }
      capture_ready = true;
      std::cout << "LAN capture participants=" << present_players << std::endl;
    }
    const float blend =
        client ? std::clamp(
                     std::chrono::duration<float>(now - snapshot_time).count() /
                         tick_seconds,
                     0.f, 1.f)
               : options.reference_render_fraction.value_or(std::clamp(float(accumulator / tick_seconds),0.f,1.f));
    // Native high-refresh pose sampling follows the local fixed-step remainder.
    // Packet receipt resets the position interpolation window, but must not
    // reset the fraction of an already validated one-tick FP channel pair.
    const float animation_blend=options.reference_render_fraction.value_or(std::clamp(float(accumulator/tick_seconds),0.f,1.f));
    const double pose_tick=animation_render_tick(previous.tick,current.tick,animation_blend);
    measure(timing.input_ms);
    auto shown=multiplayer_presentation(previous,current,blend);
    scenery_render_fraction=blend;
    hud_vertices.clear();
    for(auto *banks:{&worlds,&foregrounds,&backgrounds,&reflection_backgrounds})
      for(auto &vertices:*banks)vertices.clear();
    if(!world_effect_tick || *world_effect_tick!=current.tick)update_world_effects();
    const auto world_decals=world_effects.decals(current.tick,animation_blend,tick_seconds);
    for(const auto&gap:world_effects.diagnostics().unavailable)
      if(reported_world_effect_gaps.insert(gap).second)std::cerr<<"World effect boundary: "<<gap<<'\n';
    std::vector<std::vector<ParticleDraw>> foreground_particles(options.local_players),world_particles(options.local_players);
    update_muzzle_effects();
    for(const auto &sound:muzzle_effects.take_sounds())if(audio)try {
      if(sound.position)audio->impulse_at(sound.cue.sound,*sound.position,sound.cue.gain,MultiplayerAudioKind::Impulse,sound.owner);
      else audio->impulse(sound.cue.sound,sound.cue.gain,MultiplayerAudioKind::Impulse,sound.owner);
    }catch(const std::exception &e){std::cerr<<"Ready effect sound: "<<e.what()<<'\n';}
    std::vector<std::vector<WaterFrame>> water_frames(options.local_players);
    std::vector<std::vector<ObjectLightingBinding>> light_bindings(options.local_players);
    std::vector<SourceEffectLightView> effect_light_views(options.local_players);
    std::vector<std::vector<WorldEffectLightReceiver>> effect_light_receivers(options.local_players);
    std::vector<std::vector<EffectLightRawDraw>> raw_effect_lights(options.local_players);
    std::vector<std::vector<ModelObjectDraw>> model_object_draws(options.local_players);
    std::vector<std::vector<std::vector<Vertex>>> vehicle_vertices(options.local_players);
    for(auto& bank:vehicle_vertices)bank.reserve(current.world_vehicles.size());
    std::erase_if(vehicle_light_cache,[&](const auto& entry){return std::none_of(current.world_vehicles.begin(),current.world_vehicles.end(),
        [&](const auto& v){return (v.lighting_owner==null_datum?v.handle:v.lighting_owner)==entry.first;});});
    std::erase_if(vehicle_previous_lods,[&](const auto& entry){return std::none_of(current.world_vehicles.begin(),current.world_vehicles.end(),
        [&](const auto& v){return v.handle==entry.first;});});
    // Independent geometry and per-draw constants keep a detached body from
    // acquiring its respawned owner's new pose, lighting or appearance.
    std::vector<std::vector<std::vector<Vertex>>> corpse_vertices(options.local_players);
    for(auto &bank:corpse_vertices)bank.reserve(current.corpses.size());
    std::erase_if(corpse_light_cache,[&](const auto &entry) {
      return std::none_of(current.corpses.begin(),current.corpses.end(),
          [&](const auto &corpse){return corpse.object==entry.first;});
    });
    // ModelView retains these addresses through the immediate draw below.
    std::vector<SourceBspAdmission> bsp_views(options.local_players);
    std::vector<SourceBspResidentViewPlan> bsp_vertex_plans(options.local_players);
    std::vector<ModelView> views;
    int drawable_w{}, drawable_h{};
    SDL_Vulkan_GetDrawableSize(platform.window, &drawable_w, &drawable_h);
    if (drawable_w < 1 || drawable_h < 1) {
      SDL_Delay(10);
      continue;
    }
    // Source coverage, shader detail and screen-space effects use the scene
    // resolution. An explicit internal resolution can differ from the SDL
    // presentation surface; SSAA samples do not change this logical viewport.
    const auto &render_settings=platform.desktop->display_settings;
    if(render_settings.internal_width)drawable_w=int(render_settings.internal_width);
    if(render_settings.internal_height)drawable_h=int(render_settings.internal_height);
    const double shader_time=double(current.tick)*tick_seconds+double(blend)*tick_seconds;
    const bool widescreen = std::uint64_t(drawable_w)*3>std::uint64_t(drawable_h)*4;
    // Only authoritative native tombstones remove client props. Local shot
    // counters never infer an explosion or another object's lifetime.
    if(client){
      // Before the first accepted host state, current is the local startup
      // bank. It cannot establish the replica's generation baseline.
      if(joined && current.source_scene_roots_authoritative)
        match.replicate_source_scene(current.source_scene_roots,current.source_body_poses);
      if(joined)match.replicate_devices(current.devices);
      match.replicate_prop_removals(current.prop_removals);
      match.replicate_prop_regions(current.prop_regions);
    }
    props.synchronize(match.simulation().scene());
    if(current.source_scene_roots_authoritative && options.source_scene_observer)
      options.source_scene_observer(current,match.simulation().scene());
    props.interpolate(match.simulation().scene(),scene_interpolation,blend);
    weapon_projectile_scene.synchronize(shown.weapon_projectiles);
    if(grenade_scene) {
      // Released WORLD geometry is the current admitted rendering subset.
      // Held pose consumers are measured, but their original frozen/visibility
      // ownership is not recovered. This is not an inferred Xbox hide rule.
      std::vector<NativeGrenadeProjectile> released;
      for(const auto&projectile:shown.grenade_projectiles)
        if(projectile.parent==null_datum) {
          released.push_back(projectile);

        }
      grenade_scene->synchronize(released);
    }
    std::set<std::uint32_t> current_prop_handles;
    for(const auto& object:match.simulation().scene().objects())
      if(object.handle!=null_datum && content.objects.at(object.placement).type==11)
        current_prop_handles.insert(object.handle);
    for(auto at=prop_query_handles.begin();at!=prop_query_handles.end();)
      if(!current_prop_handles.contains(*at)){prop_initial_state.destroy(*at);at=prop_query_handles.erase(at);}
      else ++at;
    machines.synchronize(match.simulation().scene(),&match.simulation().devices());
    if(current.devices.tick>previous.devices.tick)machines.interpolate(previous.devices,blend);
    if(!client) {
      source_scenery.synchronize(match.simulation().scene());
      source_scenery.interpolate(match.simulation().scene(),scene_interpolation,blend);
      std::set<std::uint32_t> current_scenery_handles;
      for(const auto &object:match.simulation().scene().objects())
        if(object.handle!=null_datum) {
          const auto type=content.objects.at(object.placement).type;
          if(type==6)current_scenery_handles.insert(object.handle);
        }
      for(auto at=scenery_query_lifetimes.begin();at!=scenery_query_lifetimes.end();)
        if(!current_scenery_handles.contains(at->first)) {
          scenery_initial_state.destroy(at->first);at=scenery_query_lifetimes.erase(at);
        } else ++at;
    }
    if(!ids.empty()) {
      machines.update_pose(current.devices.tick,std::int32_t(frames),mesh);
      props.update_pose(std::int32_t(frames),mesh);
      source_scenery.update_pose(current.tick,std::int32_t(frames),mesh);
    }
    {
      const auto packets=machines.portal_owners();
      std::set<std::uint32_t> current_handles;
      for(const auto &packet:packets)current_handles.insert(packet.owner.handle);
      for(auto at=source_portal_owners.begin();at!=source_portal_owners.end();)
        if(!current_handles.contains(at->first)) {
          source_portal_mask.destroy(source_portal_definition,at->second);
          at=source_portal_owners.erase(at);
        } else ++at;
      if(current.tick<source_portal_tick)throw FormatError("source portal clock moved backwards");
      for(const auto &packet:packets) {
        auto [at,created]=source_portal_owners.try_emplace(packet.owner.handle,packet.owner);
        if(!created) {
          // Only210DF0 BSP activation binds ranges. A recreated115190 owner
          // retains NONE/0 until that actual activation service is invoked.
          const auto first=at->second.first,count=at->second.count;
          at->second=packet.owner;at->second.first=first;at->second.count=count;
        }
        // Original1152E0 is a simulation callback. Coldtick0 retains the
        // initialized all-open mask. Local/host snapshots now supply current
        // simulation channels, target groups and106790 device flags.
        if(current.tick!=source_portal_tick)
          source_portal_mask.step(source_portal_definition,at->second,packet.door);
      }
      source_portal_tick=current.tick;
    }
    if(options.world_portal_observer)options.world_portal_observer(current.tick,source_portal_mask);
    const auto body_handle=[&](const MultiplayerState::Player &player) {
      return player.dead?player.source_previous_unit:player.source_unit;
    };
    const auto source_body_pose=[&](const MultiplayerState::Player &player)->const SourceWorldPose * {
      const auto at=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
          [&](const auto &pose){return pose.object==body_handle(player);});
      return at==current.source_body_poses.end()?nullptr:&*at;
    };
    const auto body_present=[&](const MultiplayerState::Player &player) {
      if(!player.connected || player.dead)return false; // Detached bodies have their own lifetime below.
      if(current.source_body_poses_authoritative)
        return source_body_pose(player)!=nullptr;
      return true;
    };
    for(unsigned j=0;j<multiplayer_capacity;++j) {
      const auto &root=shown.players[j];
      if(!body_present(root)){object_light_alive[j]=false;continue;}
      if(!object_light_alive[j] || object_light_units[j]!=body_handle(root)) {
        object_light_cache[j]={};object_light_reported[j]=false;object_light_alive[j]=true;
        object_light_units[j]=body_handle(root);
      }
      auto &light=object_light_cache[j];
      if(reference_player) {
        if(!reference_player->owner_sphere)
          throw FormatError("original player shader selection requires its retained owner sphere");
        object_owner_spheres[j]=*reference_player->owner_sphere;
        light.lighting=reference_player->root_lighting;
        light.lighting_valid=true;
        continue;
      }
      if(current.source_body_poses_authoritative) {
        object_owner_spheres[j]=source_body_pose(root)->sphere;
        continue;
      }
      const auto &v=species_for(root);
      const auto &player=v.player;
      const auto body_basis=authored_basis({root.yaw,0,0});
      const auto object_frame=source_object_transform(root.position,body_basis.forward,body_basis.up,1,false);
      const auto node0=pose_model_nodes_world(player.model,object_frame,
              player.idle?&*player.idle:nullptr,float(pose_tick)*tick_seconds).front();
      object_owner_spheres[j]=source_shadow_sphere(v.player_bounds,node0,1);
    }
    for(auto &s:scenery)if(s.dynamic_lighting && !s.probe) {
      const auto origin=s.query_origin;
      const auto frame=std::int32_t(frames);
      const bool due=object_lighting_query_due(s.light,frame,origin,0,true);
      ObjectLightQueryContext query_context;
      query_context.bank_valid=source_object_light_query_bank_valid(lighting);
      query_context.object_type=6;
      query_context.definition_flags=s.definition.object_flags;
      // BC280/B7B40 and the SCEN constructor leave runtime bit13 clear.
      // Script/network mutation is a separate live-state producer.
      const auto query=due?source_object_light_query(query_context,
          [&](unsigned mode){return query_object_lightmap(surface_lighting,object_light_collision,origin,
              ObjectLightMapQueryContext{mode,!(s.light.sample_valid && s.light.lighting_valid),1.f,s.light.target});},
          []{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
      if(query.query.status==ObjectLightQueryStatus::attribution_unresolved && !s.reported) {
        std::cerr<<"Object lighting unresolved: original collision leaf needed for scenery "<<s.visual.model.name<<'\n';
        s.reported=true;
      }
      // 10A1B0/B9B90 freeze authored scenery after its initial sample.
      update_object_lighting_cache(s.light,object_lighting_definition,6,frame,origin,{},due,
          query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,true);
    }
    // Native equipped world/FP banks belong to the current biped. Scalar37
    // stops at that first biped even when the biped itself has a parent;
    // object lighting's ultimate-root lookup cannot supply this input.
    std::vector<std::optional<float>> material_camouflage(mesh.materials.size());
    const auto camouflage_range=[&](unsigned first,unsigned count,float value) {
      if(first>material_camouflage.size() || count>material_camouflage.size()-first)
        throw FormatError("Camouflage owner material range outside native bank");
      for(unsigned m=first;m<first+count;++m)material_camouflage[m]=value;
    };
    if(!reference_player)for(unsigned j=0;j<object_slots;++j) {
      const auto &owner=current.players[j];
      SourceCamouflageContextServices context;
      context.initial=[&](std::uint32_t datum)->std::optional<SourceCamouflageObjectView> {
        if(datum==null_datum || owner.source_unit==null_datum)return std::nullopt;
        if(datum==owner.source_unit)return SourceCamouflageObjectView{0,owner.source_parent};
        // This is the native scene's explicit equipped-weapon attachment to
        // its current unit, not a synthesized original object-header pool.
        if(datum==owner.source_weapon || datum==owner.secondary_weapon.datum)
          return SourceCamouflageObjectView{2,owner.source_unit};
        return std::nullopt;
      };
      context.parent=[&](std::uint32_t datum)->std::optional<SourceCamouflageObjectView> {
        if(owner.source_unit==null_datum || (datum&0xffffu)!=(owner.source_unit&0xffffu))return std::nullopt;
        return SourceCamouflageObjectView{0,owner.source_parent};
      };
      context.primary=[&](std::uint32_t){return owner.source_camouflage.primary;};
      const std::array camouflage{
          source_object_camouflage(owner.source_weapon,context),
          source_object_camouflage(owner.secondary_weapon.datum,context)};
      for(const auto &[definition,bank]:body_guns)
        for(unsigned hand=0;hand<2;++hand)
          camouflage_range(bank.materials[j*2+hand],unsigned(bank.gun.model.mesh.materials.size()),camouflage[hand]);
      for(unsigned i=0;i<ids.size();++i)if(ids[i]==j)
        for(const auto &[representation,body]:species)
          for(unsigned hand=0;hand<2;++hand) {
            const auto &gun=body.guns[i*2+hand];
            const auto &view=client?client_fp_views[i]:match.first_person_view(i);
            camouflage_range(gun.material,unsigned(gun.model.mesh.materials.size()),
                source_object_camouflage(view.hands[hand].weapon,context));
          }
    }
    struct BodyFramePose {
      std::vector<Vertex> body,weapon;std::vector<Transform> nodes;
      const AnimationClip *primary{};std::optional<float> frame;
      std::vector<Transform> weapon_nodes;
    };
    // Frame-local undecorated geometry. Animation/root/time are common to all
    // views; shader choice and material offsets are still applied to each copy.
    // Single-view draws retain their direct path without an extra cache copy.
    std::array<std::optional<BodyFramePose>,multiplayer_capacity> body_frame_poses;
    const auto append_shields=[&](std::vector<Vertex> &out,std::span<const Vertex> posed,
        const auto &layers,unsigned view,unsigned player,bool first_person,
        const SourceModelShaderCollector &collector,const SourceModelShaderView &shader_view) {
      if(posed.empty())return;
      const auto &owner=current.players.at(player);
      // These are the native renderer's already accepted posed model inputs.
      // The resident body uses the native highest-detail policy; first-person
      // models use its separate fixed LOD4 policy. Original cache/queue lifecycle
      // is not inferred from this native presentation boundary.
      const ShieldOverlayGeometry geometry{true,true,first_person,1,std::uint8_t(first_person?4:5),0};
      const auto selected=select_shield_overlays(biped_for(owner).vitality,
          owner.damage_feedback.shield.visible,owner.shield,geometry);
      for(const auto &overlay:selected.selected()) {
        // This resident bank owns the authored damaged-plasma pass. The
        // separate overshield shader family retains its own pass binding.
        if(overlay.kind!=ShieldOverlayKind::Damaged)continue;
        const auto &layer=layers.at(overlay.shader);
        std::vector<Vertex> vertices(posed.begin(),posed.end());
        layer.shaders->select(vertices,collector,shader_view);
        for(auto &vertex:vertices)vertex.material+=layer.materials.at(player);
        if(options.shield_layer_observer)
          options.shield_layer_observer(view,player,first_person,overlay.shader,layer.render_model,vertices);
        out.insert(out.end(),vertices.begin(),vertices.end());
      }
    };
    std::vector<std::optional<std::span<const Vertex>>> world_weapon_poses(shown.world_weapons.size());
    std::vector<std::vector<Vertex>> owned_world_weapon_poses(shown.world_weapons.size());
    std::vector<std::vector<std::vector<Vertex>>> world_weapon_draw_vertices(options.local_players,
        std::vector<std::vector<Vertex>>(shown.world_weapons.size()));
    std::map<std::uint32_t,unsigned> world_weapon_handle_counts;
    for(const auto &item:shown.world_weapons)++world_weapon_handle_counts[item.handle];
    for(auto at=retained_world_weapon_poses.begin();at!=retained_world_weapon_poses.end();) {
        const auto count=world_weapon_handle_counts.find(at->first);
        if(at->first==null_datum || count==world_weapon_handle_counts.end() || count->second!=1)
            at=retained_world_weapon_poses.erase(at);
        else ++at;
    }
    const auto *fresh_world_weapons=std::getenv("HALO2_REFERENCE_FRESH_WORLD_WEAPON_POSES");
    const bool retain_world_weapons=!(fresh_world_weapons && *fresh_world_weapons);
    const auto *copy_world_weapons=std::getenv("HALO2_REFERENCE_COPY_WORLD_WEAPONS");
    const bool share_world_weapon_poses=options.local_players>1 && !(copy_world_weapons && *copy_world_weapons);
    for (unsigned i = 0; i < ids.size(); ++i) {
      const unsigned id = ids[i];
      auto p = shown.players[id];
      const auto &v=species_for(p);
      const auto &biped=biped_for(p);
      const auto &weapon=content.weapons.at(p.weapon==null_datum?rules.weapon:p.weapon);
      const auto &gun=v.guns[i*2];
      const auto &hands=v.hands;
      auto &hud=*huds.at({multiplayer_representation(p.source_configuration),p.weapon==null_datum?rules.weapon:p.weapon});
      auto &unit_hud=*unit_huds.at(multiplayer_representation(p.source_configuration));
      if (dead[i] && !p.dead) {
        yaw[i] = p.yaw;
        pitch[i] = 0;
      }
      dead[i] = p.dead;
      p.yaw = yaw[i];
      p.pitch = pitch[i];
      float horizontal_fov=biped.camera_field_of_view;
      if(scope_constants && content.binocular_zoom && !reference_player) {
        const auto selected=content.weapons.find(p.weapon);
        if(selected!=content.weapons.end() && selected->second.smg) {
          const std::array<std::int16_t,1> modes{current.players[id].source_zoom_magazine_mode};
          const ZoomWeaponView primary{selected->second.zoom,modes};
          const auto magnification=source_zoom_magnification(view_zoom[i],&primary,*content.binocular_zoom);
          if(!magnification)throw FormatError("Original multi-level scope math provider is unavailable");
          const auto &c=*scope_constants;
          UnitScopeCameraPacket packet{1,0,0,0,0}; // Normal23DE70 caller contract.
          const auto authored=p.source_unit==null_datum?std::optional<float>{}:std::optional{biped.camera_field_of_view};
          source_scope_camera_target(scope_producers[i],packet,source_scope_camera_fov(authored,*magnification,c),c);
          source_scope_camera_register(scope_observers[i],packet);
          source_scope_camera_adopt(scope_observers[i],packet,c);
          // Other native ordinary camera channels have no source transition
          // durations. Their six-channel coupled-cap owner remains unported.
          if(source_scope_camera_motion(scope_observers[i],source_scope_camera_dt(elapsed,c),c))
            throw FormatError("Original coupled observer-channel cancellation requires its source owner");
          horizontal_fov=source_scope_camera_render_fov(scope_observers[i],c);
        }
      }
      auto camera = player_camera(p, biped,horizontal_fov,content.player_screen_offset);
      std::optional<NativeDeathCameraView> seated_view;
      if(vehicle_camera)seated_view=vehicle_camera->update(i,p.source_unit,p.source_flags,
          p.yaw,p.pitch,elapsed,{camera.position,camera.forward,camera.up,horizontal_fov});
      else if(client)seated_view=p.source_vehicle_camera;
      const bool vehicle_view=seated_view.has_value();
      if(seated_view) {
        camera.position=seated_view->position;camera.forward=seated_view->forward;camera.up=seated_view->up;
        camera.oriented=true;horizontal_fov=seated_view->horizontal_fov;
      }
      if(death_camera) {
        if(const auto view=death_camera->update(i,p.source_player,p.source_flags,
            current.team_mode,elapsed,death_cycle_pressed[i],
            {camera.position,camera.forward,camera.up,horizontal_fov})) {
          camera.position=view->position;camera.forward=view->forward;camera.up=view->up;
          camera.oriented=true;horizontal_fov=view->horizontal_fov;
        }
      } else if(client && p.source_death_camera) {
        const auto &view=*p.source_death_camera;
        camera.position=view.position;camera.forward=view.forward;camera.up=view.up;
        camera.oriented=true;horizontal_fov=view.horizontal_fov;
      }
      if(options.capture_camera)camera=*options.capture_camera;
      if(reference_player) {
        camera.position=reference_player->camera_position;
        camera.forward=reference_player->camera_forward;
        camera.up=reference_player->camera_up;
        camera.oriented=true;
      }
      const auto layout=source_view_grid(options.local_players,i,widescreen);
      auto source_angles=source_camera_angles(horizontal_fov,1.3333333730697632f,
          layout.grid,layout.span);
      if(options.capture_projection){source_angles.projection_angle=options.capture_projection->x;
        source_angles.lighting_distance_scale=options.capture_projection->y;}
      if(reference_player){source_angles.projection_angle=reference_player->camera_vertical_angle;
        source_angles.lighting_distance_scale=reference_player->camera_scale;}
      camera.field_of_view=source_angles.projection_angle*57.295779513f;
      if(audio && i==0)try {
        //18BB80 chooses the first actual local listener. Native cameras use
        // the same submitted position and literal14A280 cluster traversal.
        const auto targets=multiplayer_ambient_targets(audio_bsp,atmosphere,camera.position,
            atmosphere.cluster_at(camera.position),match.simulation().weather().current,
            match.simulation().weather().stopped==0);
        audio->ambient(targets,audio_elapsed);
      }catch(const std::exception&e){if(!ambient_unavailable_reported){ambient_unavailable_reported=true;
        std::cerr<<"Multiplayer ambience: "<<e.what()<<'\n';}}
      const auto view_forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
      const auto fog=atmosphere.sample(camera.position,view_forward);
      const auto viewport_layout=hud_viewport(options.local_players,i,widescreen);
      const auto rect=viewport_layout.outer;
      const auto physical=[&](const UiRect &r) {
        return full_surface_viewport(RenderViewport{r.left,r.top,
            r.right-r.left,r.bottom-r.top},drawable_w,drawable_h);
      };
      const auto aim_forward=direction(p.yaw,p.pitch);
      const auto projection=source_camera_projection(source_angles.projection_angle,
          physical(rect),physical(viewport_layout.safe),false);
      // Original widescreen corrects the Xbox 640x480 pixel aspect. The
      // physical Vulkan viewport already contains that display aspect.
      camera.projection_clip_scale=projection.clip_scale;
      camera.projection_clip_offset=projection.clip_offset;
      const auto source_rectangle=[&](const UiRect &r) {
        const auto p=physical(r);
        const std::array<int,4> values{int(p.top),int(p.left),
            int(p.top+p.height),int(p.left+p.width)};
        std::array<std::int16_t,4> result{};
        for(unsigned j=0;j<4;++j) {
          if(values[j]<-32768 || values[j]>32767)
            throw FormatError("source render camera rectangle exceeds its signed-short bank");
          result[j]=std::int16_t(values[j]);
        }
        return result;
      };
      SourceFrustumProjectionInput frustum_projection_input;
      frustum_projection_input.projection_angle=source_angles.projection_angle;
      frustum_projection_input.outer=source_rectangle(rect);
      frustum_projection_input.safe=source_rectangle(viewport_layout.safe);
      frustum_projection_input.scenario_present=true;
      frustum_projection_input.scenario_type=geometry_scenario_type;
      // Native ordinary render cameras have no +58/+68 subrectangle/offset,
      // auxiliary layout state or fog override. Physical viewport aspect
      // already includes the Xbox widescreen pixel-aspect correction.
      const auto frustum_projection=source_frustum_projection(frustum_projection_input);
      const auto render_camera=source_view_projection({camera.position,view_forward,
          camera.oriented?camera.up:Vec3{0,0,1},.015625f,1024.f,{},
          frustum_projection.projection_program});
      SourceViewShaderConstants source_view_constants;
      source_view_constants.world=source_view_shader_constants(render_camera,1024.f,false);
      const auto foreground_constants=source_view_shader_constants(render_camera,1024.f,true);
      std::copy_n(foreground_constants.begin(),4,source_view_constants.foreground.begin());
      source_view_constants.reflection=source_view_constants.world;
      const auto source_camera=source_frustum_camera({camera.position,view_forward,
          camera.oriented?camera.up:Vec3{0,0,1},.015625f,1024.f,
          frustum_projection.clipping_rectangle()});
      const auto visibility=source_portal_visibility(source_portals,
          std::span{&source_camera,1},fog.cluster,source_portal_mask.words);
      //36B50 advances intensity/clock only through view0. Every local view
      // consumes that same elapsed value and its actual clipped cluster bank.
      match.advance_weather_view(weather_realtime,std::int16_t(i));
      const auto weather_view=source_weather_view_inputs(std::int16_t(i),camera.position,
          view_forward,source_angles.projection_angle,render_camera);
      weather_world.admit_view(weather_view,atmosphere,fog.cluster,visibility,
          match.simulation().weather());
      SourceWeatherWorldStep weather_step;
      weather_points[i].clear();weather_lines[i].clear();weather_plates[i].clear();
      if(!weather_world.definitions().empty()) {
        SourceWeatherExecution arithmetic;
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,true});
        const ShaderVector clip_range{0,16777215.f,depth.minimum_w,depth.maximum_w};
        SourceWeatherWorldDrawServices draws;
        draws.plate=[&](std::uint32_t datum,std::size_t index,std::span<const std::byte> plate,
            std::span<const std::byte>,unsigned slot) {
          auto submission=source_weather_plate_submission({plate,std::int16_t(slot),
              source_rectangle(rect),{.015625f,1024.f},{.015625f,1024.f}});
          //37A60 returns AL1 on its no-quad branch too. It does not suppress
          //the independent following particle draw chain.
          if(submission.result.profile==SourceWeatherPlateCallerProfile::NoDraw)return true;
          const auto &owner=weather_world.definition(datum).definition;
          auto &draw=weather_plates[i].emplace_back();draw.vertex_program=owner.vertex_programs[0];
          draw.submission=std::move(submission);draw.clip_range=clip_range;draw.rectangle=source_rectangle(rect);
          for(unsigned layer=0;layer<3;++layer)draw.layers[layer]=&weather_layers.at(owner.plates.at(index).textures[layer]);
          return true;
        };
        draws.particle=[&](std::uint32_t datum,std::size_t index,SourceWeatherParticleResource &resource,
            float elapsed,std::int16_t views,std::int16_t slot) {
          const auto &owner=weather_world.definition(datum).definition;
          const auto immediate=resource.submission(elapsed,views,slot);
          const auto fill_camera=[&](auto &draw) {
            draw.vertex_program=owner.vertex_programs[1];
            draw.camera=source_view_shader_bank(render_camera,1024.f,camera.position,physical(rect),false);
            const auto viewport=source_weather_viewport_constants(
                source_rectangle(rect)[1],source_rectangle(rect)[0],
                source_rectangle(rect)[3],source_rectangle(rect)[2]);
            std::copy(viewport.begin(),viewport.end(),draw.camera.begin()+10);draw.clip_range=clip_range;
          };
          if(immediate.lines) {
            auto &draw=weather_lines[i].emplace_back();fill_camera(draw);
            draw.pixel=source_weather_pixel_program(true);draw.line_width=immediate.line_width;
            for(const auto &vertex:immediate.vertices)draw.vertices.push_back({vertex.position,vertex.color});
          } else {
            const auto sprite=weather_sprites.find(owner.particles.at(index).sprites);
            if(sprite==weather_sprites.end())throw FormatError("weather point draw lacks its admitted authored sprite");
            auto &draw=weather_points[i].emplace_back();fill_camera(draw);
            draw.pixel=source_weather_pixel_program(false);draw.sprite=&sprite->second;
            for(const auto &point:immediate.vertices)draw.points.push_back({point.position,point.color});
          }
          return true;
        };
        weather_step=weather_world.visit_view(i,match.simulation().weather(),
            {std::int16_t(options.local_players),match.simulation().weather_clock().elapsed,
             0x023f,0x1f80,[](std::size_t,std::uint32_t speed) {
               return source_weather_rsqrt_xemu_0_8_136(speed);
             }},auxiliary_random,draws);
      }
      if(options.frames && frames==0 && !weather_world.definitions().empty()) {
        const auto admitted=weather_world.admission(i);
        std::cout<<"Source weather admission: view="<<i<<" palettes="
            <<std::count_if(admitted.begin(),admitted.end(),[](const auto &entry){return entry.visible;})
            <<" initialized="<<weather_world.initialized()
            <<" view_bytes="<<weather_world.view_inputs(i).size()<<'\n';
      }
      if(options.frames && !weather_world.definitions().empty()) {
        std::size_t vertices=0;
        std::uint64_t packet_hash=14695981039346656037ull;
        const auto admitted=weather_world.admission(i);
        for(std::size_t palette=0;palette<admitted.size();++palette)if(admitted[palette].visible) {
          const auto &owner=weather_world.definition(weather_world.bsp().palette[palette].datum);
          for(const auto &resource:owner.particles)if(resource.published()) {
            const auto packet=resource.submission(match.simulation().weather_clock().elapsed,
                std::int16_t(options.local_players),std::int16_t(i));
            vertices+=packet.vertices.size();
            for(auto byte:std::as_bytes(std::span(packet.vertices)))
              packet_hash=(packet_hash^std::to_integer<unsigned>(byte))*1099511628211ull;
          }
        }
        std::cout<<"Source weather simulation: frame="<<frames<<" view="<<i
            <<" elapsed="<<match.simulation().weather_clock().elapsed
            <<" wind="<<weather_step.wind_steps<<" plates="<<weather_step.plate_steps
            <<" systems="<<weather_step.particle_steps<<" rows="<<weather_step.particle_rows
            <<" rsqrt="<<weather_step.rsqrt_calls<<" vertices="<<vertices
            <<" packet_hash="<<packet_hash<<'\n';
      }
      const auto containing_fog=source_bsp_fog_admission(atmosphere,
          {true,true,fog.planar_datum,0});
      SourceBspAdmissionInput bsp_admission_input;
      bsp_admission_input.containing_fog=containing_fog;
      bsp_views[i]=source_bsp_admission(source_bsp_geometry,visibility,bsp_admission_input);
      const auto light_frustums=visibility.sphere_bank();
      SourceEffectLightCollectionContext light_collection{};
      light_collection.visibility.frustums=&light_frustums;
      light_collection.containing_cluster=std::int16_t(fog.cluster);light_collection.local_player=i;
      SourceEffectLightCandidateContext light_candidates{};
      light_candidates.admission={1,0,0,2,{}}; // Ordinary enabled native world view, no debug suppression.
      const auto& inverse=render_camera.inverse_view;
      light_candidates.depth_plane={inverse.basis[0].z,inverse.basis[1].z,inverse.basis[2].z,inverse.position.z};
      light_candidates.pixel_scale=render_camera.viewport_half_metrics.y;
      //2B5D0 increments the render-frame bank once before all local views.
      effect_light_views[i]=world_effects.light_view(camera.position,light_collection,
          light_candidates,std::bit_cast<std::int32_t>(std::uint32_t(frames)+1));
      effect_light_receivers[i]=world_effects.light_receivers(effect_light_views[i],
          source_bsp_geometry,containing_fog,&bsp_views[i]);
      raw_effect_lights[i]=bsp_effect_lights.draw(effect_light_views[i],effect_light_receivers[i],
          bsp_views[i],render_camera,fog,camera.position,source_rectangle(rect),
          {std::uint32_t(drawable_w),std::uint32_t(drawable_h)},options.local_players);
      const auto observer_aim=source_observer_aim(aim_forward,
          cross(aim_forward,{-std::sin(p.yaw),std::cos(p.yaw),0}),
          horizontal_fov,content.player_screen_offset);
      const auto aim_anchor=source_hud_aim_anchor({camera.position,view_forward,camera.up,
          source_angles.projection_angle,false},
          physical(rect),physical(viewport_layout.safe),observer_aim.displacement,!p.dead);
      // 3CA90 reads the original outer rectangle and raw camera angle;
      // its 1.5 widescreen factor differs from the main safe frustum.
      const float water_aspect=(rect.right-rect.left)/(rect.bottom-rect.top);
      for(unsigned d=0;d<water.definitions.size();++d) {
        water_frames[i].push_back(water_frame(water.definitions[d],camera.position,
            view_forward,source_angles.projection_angle,water_aspect,widescreen));
      }
      auto &world = worlds[i];
      world.insert(world.end(),world_decals.begin(),world_decals.end());
      {
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,true});
        MuzzleEffectView effects_view;
        effects_view.camera=source_view_shader_bank(render_camera,1024.f,camera.position,
            full_surface_viewport({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top},drawable_w,drawable_h),false);
        effects_view.fog=bsp_fog_constants(fog,camera.position,view_forward);
        const auto&basis=render_camera.camera_to_world.basis;
        effects_view.orientation={basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,basis[2].x,basis[2].y,basis[2].z};
        effects_view.position=camera.position;effects_view.forward=view_forward;effects_view.up=camera.oriented?camera.up:basis[2];
        effects_view.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
        effects_view.width=drawable_w;effects_view.height=drawable_h;
        world_particles[i]=world_effects.draw(effects_view,blend,tick_seconds);
        if(options.effect_observer)options.effect_observer(i,current.tick,world_particles[i]);
      }
      auto &bindings=light_bindings[i];
      //3DFE0 deduplicates an actual query, not a visit by an earlier view.
      // Snapshot each submitted view's bank before a later view can update it.
      for(unsigned j=0;j<multiplayer_capacity;++j)
        if(!reference_player && body_present(current.players[j])) {
          auto &light=object_light_cache[j];
          const auto origin=options.capture_root?(*options.capture_root)[0]:object_owner_spheres[j].center;
          const auto velocity=options.capture_root?(*options.capture_root)[1]:current.players[j].velocity;
          const auto frame=std::int32_t(frames);
          const auto draw_distance=length(origin-camera.position)*source_angles.lighting_distance_scale;
          const bool due=object_lighting_query_due(light,frame,origin,draw_distance,false);
          const ObjectSkyLightingDefinition *selected_render_sky=nullptr;
          if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<sky_lighting.size() && sky_lighting[fog.visible_sky])
            selected_render_sky=&*sky_lighting[fog.visible_sky];
          ObjectLightQueryContext query_context;
          query_context.bank_valid=source_object_light_query_bank_valid(lighting);
          query_context.object_type=0;query_context.definition_flags=biped_for(current.players[j]).object_flags;
          //DE4C0/E5980 cold mode; later physics/death/lunge state is separate.
          query_context.biped_state=source_biped_light_query_initial_state(biped_for(current.players[j]).physics_flags,0);
          const auto query=due?source_object_light_query(query_context,
              [&](unsigned mode){return query_object_lightmap(surface_lighting,object_light_collision,origin,
                  ObjectLightMapQueryContext{mode,!(light.sample_valid && light.lighting_valid),1.f,light.target});},
              [&]{return query_object_sky_lightmap(surface_lighting,selected_render_sky,origin);}):ObjectLightQueryAdmission{};
          if(query.query.status==ObjectLightQueryStatus::attribution_unresolved && !object_light_reported[j]) {
            std::cerr<<"Object lighting unresolved: original collision leaf needed for player "<<j<<'\n';
            object_light_reported[j]=true;
          }
          update_object_lighting_cache(light,object_lighting_definition,0,frame,origin,velocity,due,
              query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{});
        }
      for(unsigned j=0;j<multiplayer_capacity;++j)if(shown.players[j].connected) {
        const auto &v=species_for(shown.players[j]);
        bindings.push_back({v.player_materials[j],unsigned(v.player.model.mesh.materials.size()),object_light_cache[j].lighting});
        // Original3DB00/BAF80 gives attached weapons their root's84-byte bank.
        const auto &held=body_guns.at(shown.players[j].weapon==null_datum?rules.weapon:shown.players[j].weapon);
        bindings.push_back({held.materials[j*2],unsigned(held.gun.model.mesh.materials.size()),object_light_cache[j].lighting});
      }
      const auto first_person_lighting=reference_player?reference_player->first_person_lighting:object_light_cache[id].lighting;
      bindings.push_back({hands.material,unsigned(hands.model.mesh.materials.size()),first_person_lighting});
      bindings.push_back({gun.material,unsigned(gun.model.mesh.materials.size()),first_person_lighting});
      for(const auto &s:scenery)if(s.dynamic_lighting)
        bindings.push_back({s.visual.material,unsigned(s.visual.model.mesh.materials.size()),
            s.probe?s.probe->lighting:s.light.lighting});
      measure(timing.view[i].camera_lighting_ms);
      if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<sky_models.size() && sky_models[fog.visible_sky]) {
        const auto &sky_model=*sky_models[fog.visible_sky];const auto &sky=*skies[fog.visible_sky];
        append(backgrounds[i],pose_sky(sky_model.model,sky,camera.position),sky_model.material,{});
        if(!water.definitions.empty()) {
          auto eye=camera.position;eye.z=2.f*water.definitions.front().parameters[33]-eye.z;
          append(reflection_backgrounds[i],pose_sky(sky_model.model,sky,eye),sky_model.material,{});
        }
      }
      const auto detail_policy=native_detail_policy(platform.desktop->display_settings.detail,options.local_players);
      const SourceBspShaderView bsp_shader_view{render_camera,bsp_views[i],
          frustum_projection_input.outer[0],frustum_projection_input.outer[2],
          options.local_players,frustum_projection.view_modifier,detail_policy.effect_scale};
      if(bsp_shaders) {
        bsp_vertex_plans[i]=bsp_shaders->select(bsp_shader_view);
        if(bsp_reference_scene) {
          auto reference=bsp_reference_mesh->vertices;
          const auto reference_parts=bsp_reference_scene->select(reference,bsp_shader_view);
          const auto expanded=bsp_shaders->expand(bsp_vertex_plans[i].bindings);
          if(reference.size()!=expanded.size() || (!reference.empty() &&
              std::memcmp(reference.data(),expanded.data(),reference.size()*sizeof(Vertex))))
            throw FormatError("Actual BSP view changed complete source-selected vertices");
          const auto &selected=bsp_vertex_plans[i].selected_parts;
          if(reference_parts.size()!=selected.size())throw FormatError("Actual BSP view changed selected part count");
          for(std::size_t p=0;p<selected.size();++p) {
            const auto &a=reference_parts[p],&b=selected[p];
            if(std::tie(a.geometry,a.section,a.part,a.shader_lod,a.material,a.coverage_byte,a.instance_alpha)!=
                std::tie(b.geometry,b.section,b.part,b.shader_lod,b.material,b.coverage_byte,b.instance_alpha))
              throw FormatError("Actual BSP view changed ordered selected part fields");
          }
        }
        if(options.bsp_selection_observer)options.bsp_selection_observer(current.tick,i,bsp_shader_view,bsp_vertex_plans[i].selected_parts);
      } else {
        world.insert(world.end(),copy_world_vertices.begin(),copy_world_vertices.end());
        const auto selected=bsp_copy_scene->select(world,bsp_shader_view);
        if(options.bsp_selection_observer)options.bsp_selection_observer(current.tick,i,bsp_shader_view,selected);
      }
      for(unsigned d=0;d<water.definitions.size();++d)if(!water.definitions[d].geometry.materials.empty()) {
        auto vertices=water.definitions[d].geometry.vertices;
        for(auto &vertex:vertices)vertex.position=water_world_position(water_frames[i][d],vertex.position);
        append(world,std::move(vertices),water_material_bases[d],{});
      }
      for (const auto &s : scenery)
        append(world, pose_model(s.visual.model,s.visual.idle?&*s.visual.idle:nullptr,float(shader_time)), s.visual.material,
               s.transform);
      measure(timing.view[i].static_world_ms);
      SourceObjectViewLod object_view;
      object_view.view_index=i;object_view.local_players=options.local_players;
      object_view.detail_policy=detail_policy;
      // The current ordinary camera has no +58 subrectangle or +68 offset
      // feature.2F970 additionally consumes the actual primaryscnr+10 type.
      object_view.source_4ba019=frustum_projection.view_modifier;
      object_view.shader=SourceModelShaderView{
          source_shader_coverage_camera(render_camera),
          source_shader_lod_view(frustum_projection_input.outer[0],
              frustum_projection_input.outer[2],false,false),
          detail_policy.coverage_multiplier,
          detail_policy.effect_scale,
          frustum_projection.view_modifier};
      for(const auto& v:current.world_vehicles) {
        if(v.placement>=content.objects.size() || content.objects[v.placement].type!=1)throw FormatError("Vehicle snapshot lost its authored definition");
        const auto& o=content.objects[v.placement];const auto& vehicle=vehicle_visuals.at({o.definition,o.variant});
        const auto& skeleton=vehicle.lods[5];
        auto nodes=current.source_body_poses_authoritative?source_body_interpolation.sample(v.handle,blend):
            pose_model_nodes_world(skeleton,v.transform);
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        auto reference_row=o.netgame_row;
        for(auto parent=o.model_attachment;reference_row<0 && parent;parent=content.objects.at(parent->parent).model_attachment)
          reference_row=content.objects.at(parent->parent).netgame_row;
        const auto* reference=vehicle_shading_reference(o.definition,reference_row);
        if(reference)nodes=reference->nodes;
#endif
        if(nodes.empty())throw FormatError("Vehicle lost its current WORLD node0");
        auto sphere=vehicle.shaders->sphere(nodes.front(),v.transform.scale);
        if(current.source_body_poses_authoritative) {
          const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),[&](const auto& p){return p.object==v.handle;});
          if(pose==current.source_body_poses.end())throw FormatError("Vehicle lost its authoritative coverage sphere");
          sphere=pose->sphere;
        }
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        if(reference)sphere=reference->sphere;
#endif
        auto geometry_flags=v.geometry_flags;auto geometry_control=v.geometry_control_flags;
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        if(reference){geometry_flags=reference->geometry_flags;geometry_control=reference->geometry_control_flags;}
#endif
        const auto [history,inserted]=vehicle_previous_lods.try_emplace(v.handle,std::array<std::int8_t,4>{-1,-1,-1,-1});
        auto metric=source_geometry_lod_distance(sphere.center,camera.position,source_angles.lighting_distance_scale);
        if(object_view.source_4ba019)metric*=.3f;
        if(geometry_control&4u)metric*=1.7f;
        const auto choice=source_geometry_lod_choice(vehicle.geometry,metric,history->second[i]);
        const auto lod=unsigned(source_geometry_lod_cap(choice,detail_policy,geometry_flags,false));
        auto& vertices=vehicle_vertices[i].emplace_back(skin_model_world_nodes(vehicle.lods[lod],nodes));
        if(!vertices.empty())history->second[i]=std::int8_t(lod);
        vehicle.shaders->select(vertices,sphere,*object_view.shader);
        for(auto& vertex:vertices)vertex.material+=vehicle.material;
        const auto owner=v.lighting_owner==null_datum?v.handle:v.lighting_owner;
        const auto definition=v.lighting_definition==null_datum?o.definition:v.lighting_definition;
        auto& light=vehicle_light_cache[owner];
        auto origin=v.lighting_owner==null_datum?sphere.center:v.lighting_origin;
        auto velocity=v.lighting_velocity;auto flags=v.lighting_flags;auto frozen=v.lighting_frozen;
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        if(reference){origin=reference->origin;velocity=reference->velocity;flags=reference->flags;frozen=reference->frozen;}
#endif
        const auto distance=source_geometry_lod_distance(origin,camera.position,source_angles.lighting_distance_scale);
        const auto frame=std::int32_t(frames);const bool due=object_lighting_query_due(light,frame,origin,distance,frozen);
        auto context=vehicle_light_definitions.at(definition);context.bank_valid=source_object_light_query_bank_valid(lighting);context.runtime_flags=flags;
        const ObjectSkyLightingDefinition* sky=nullptr;
        if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<sky_lighting.size() && sky_lighting[fog.visible_sky])sky=&*sky_lighting[fog.visible_sky];
        const auto query=due?source_object_light_query(context,[&](unsigned mode){
          return query_object_lightmap(surface_lighting,object_light_collision,origin,
              ObjectLightMapQueryContext{mode,!(light.sample_valid && light.lighting_valid),1.f,light.target});
        },[&]{return query_object_sky_lightmap(surface_lighting,sky,origin);}):ObjectLightQueryAdmission{};
        if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)throw FormatError("Vehicle lighting contact attribution unresolved");
        update_object_lighting_cache(light,object_lighting_definition,1,frame,origin,velocity,due,
            query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,frozen);
        // Each owner supplies independent constants even when vehicle models,
        // resident images and material slots are shared with another vehicle.
        const auto colors=source_object_initial_colors(vehicle.colors,o.position,vehicle.variant_name);
        model_object_draws[i].push_back({vertices,light.lighting,colors,{}});
        if(options.vehicle_draw_observer)options.vehicle_draw_observer(i,v.handle,o.definition,light.lighting,light.lighting_valid,lod,vertices);
      }
      object_view.input=[&](std::size_t source_object,std::uint32_t handle,Vec3 center,float) {
        const auto&live=match.simulation().scene().get(handle);
        const auto&authored=content.objects.at(source_object);
        if(live.placement!=source_object || (authored.type!=6 && authored.type!=7 && authored.type!=11))
          throw FormatError("world geometry LOD requires its actual constructor identity");
        // All742 original SCEN/MACH/BLOC constructor prefixes have bit19 and
        // +10A bit2 clear. Their native handlers do not yet run the later
        // HSsetLOD or control-state producers; retain these initial inputs.
        return SourceObjectLodInput{source_geometry_lod_distance(center,camera.position,
            source_angles.lighting_distance_scale),0,0};
      };
      machines.append_view(world,bindings,object_view);
      props.append_view(world,bindings,object_view);
      source_scenery.append_view(world,bindings,object_view);
      if(grenade_scene) {
        auto grenade_view=object_view;
        grenade_view.input=[&](std::size_t index,std::uint32_t handle,Vec3 center,float) {
          const auto owner=std::find_if(current.grenade_projectiles.begin(),current.grenade_projectiles.end(),
              [&](const auto&p){return p.datum==handle && (p.datum&65535u)==index;});
          if(owner==current.grenade_projectiles.end())
            throw FormatError("grenade draw lost its actual salted snapshot owner");
          // Captured B8820/B8A30/E8F30 construction and projectile writers
          // FC666/FC6C6/FC6D1 leave the ordinary generic
          // +04bit19/+10Abit2 clear. Native projectile contact flags are a
          // different namespace and never stand in for these object fields.
          return SourceObjectLodInput{source_geometry_lod_distance(center,camera.position,
              source_angles.lighting_distance_scale),0,0};
        };
        for(const auto&draw:grenade_scene->draw_view(std::int32_t(frames),grenade_view)) {
          model_object_draws[i].push_back({draw.vertices,draw.ordinary_lighting?
              std::optional<ObjectLightingState>{draw.lighting}:std::nullopt});
          if(options.grenade_draw_observer)
            options.grenade_draw_observer(i,current.tick,draw.owner,draw.definition,draw.vertices);
        }
      }
      {
        auto projectile_view=object_view;
        projectile_view.input=[&](std::size_t index,std::uint32_t handle,Vec3 center,float) {
          const auto owner=std::find_if(current.weapon_projectiles.begin(),current.weapon_projectiles.end(),
              [&](const auto &p){return p.datum==handle && (handle&65535u)==index;});
          if(owner==current.weapon_projectiles.end())throw FormatError("weapon projectile LOD lost its actual owner");
          return SourceObjectLodInput{source_geometry_lod_distance(center,camera.position,
              source_angles.lighting_distance_scale),0,0};
        };
        for(const auto &draw:weapon_projectile_scene.draw_view(std::int32_t(frames),projectile_view)) {
          model_object_draws[i].push_back({draw.vertices,draw.ordinary_lighting?
              std::optional<ObjectLightingState>{draw.lighting}:std::nullopt,draw.colors});
          if(options.weapon_projectile_draw_observer)
            options.weapon_projectile_draw_observer(i,current.tick,draw.owner,draw.definition,draw.vertices);
        }
      }
      for(const auto &item:shown.world_weapons) {
        const auto found=body_guns.find(item.definition);if(found==body_guns.end())continue;
        const auto &gun=found->second.gun;
        const auto frame=source_object_transform(item.position,item.forward,item.up,1,false);
        const auto collector=source_model_shader_collector(item.position,.5f,*object_view.shader);
        auto &owner_vertices=world_weapon_draw_vertices[i][std::size_t(&item-shown.world_weapons.data())];
        if(share_world_weapon_poses) {
          // Current animation/root sampling runs once per weapon per frame.
          // Only a complete unchanged node bank can retain its immutable skin.
          // Every view still applies its own collector and material binding.
          const auto index=std::size_t(&item-shown.world_weapons.data());
          auto &pose=world_weapon_poses[index];
          if(!pose) {
            if(retain_world_weapons && item.handle!=null_datum && world_weapon_handle_counts[item.handle]==1) {
              const auto nodes=pose_model_nodes_world(gun.model,frame,gun.idle?&*gun.idle:nullptr,float(pose_tick)*tick_seconds);
              pose=retained_world_weapon_poses[item.handle].update(gun.model,nodes).vertices;
            } else {
              owned_world_weapon_poses[index]=pose_model_world(gun.model,frame,gun.idle?&*gun.idle:nullptr,float(pose_tick)*tick_seconds);
              pose=owned_world_weapon_poses[index];
            }
          }
          bool verify_world_pose=std::getenv("HALO2_VERIFY_WORLD_WEAPON_POSES")!=nullptr;
#ifdef HALO2_PROFILE_MATERIAL_FRAME_ORACLE
          verify_world_pose=verify_world_pose || std::getenv("HALO2_VERIFY_ACTIVE_MATERIALS");
#endif
          if(verify_world_pose) {
            const auto reference=pose_model_world(gun.model,frame,gun.idle?&*gun.idle:nullptr,float(pose_tick)*tick_seconds);
            if(reference.size()!=pose->size() || (!reference.empty() && std::memcmp(reference.data(),pose->data(),reference.size()*sizeof(Vertex))))
              throw FormatError("shared world weapon pose differs from fresh view pose");
          }
          gun.shaders->append_selected(owner_vertices,*pose,collector,*object_view.shader,gun.material);
        } else {
          auto vertices=pose_model_world(gun.model,frame,gun.idle?&*gun.idle:nullptr,float(pose_tick)*tick_seconds);
          gun.shaders->select(vertices,collector,*object_view.shader);
          append_world(owner_vertices,std::move(vertices),gun.material);
        }
        const auto *definition=&weapon_shader_definitions.at(item.definition);
        const auto tables=cache.shader_function_tables();
        const ShaderInput owner_input=[&,item,definition,tables](std::uint32_t name) {
          float value=0;
          if(const auto source=source_weapon_shader_input(name,item.shader))value=*source;
          else {
            const bool ammunition_function=std::any_of(definition->functions.begin(),definition->functions.end(),
                [&](const auto &f){return f.output==name && f.input==0x1200056au;});
            if((name==0x1200056au || ammunition_function) && definition->magazines.size()==1) {
              const std::array<SourceWeaponMagazineState,1> magazines{{{std::int16_t(item.loaded),std::int16_t(item.reserve)}}};
              if(const auto source=source_weapon_magazine_shader_input(name,definition->magazines,
                  magazines,definition->functions,*tables))value=source->value;
            } else value=weapon_ammunition_input(name,item.loaded).value_or(0.f);
          }
          if(options.world_weapon_shader_input_observer)
            options.world_weapon_shader_input_observer(item.handle,item.definition,name,value);
          return value;
        };
        model_object_draws[i].push_back({owner_vertices,object_light_cache[id].lighting,{},owner_input});
      }
      measure(timing.view[i].objects_ms);
      if(current.source_body_poses_authoritative)for(const auto &corpse:current.corpses) {
        const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
            [&](const auto &p){return p.object==corpse.object;});
        if(pose==current.source_body_poses.end())
          throw FormatError("Retained corpse lost its completed source WORLD hierarchy");
        const auto representation=multiplayer_representation(corpse.configuration);
        const auto &body=species.at({representation,rules.weapon});
        const auto &definition=content.player_representations.at(representation).biped;
        if(definition.datum!=corpse.definition)
          throw FormatError("Retained corpse appearance differs from its actual biped definition");
        auto [light_at,new_light]=corpse_light_cache.try_emplace(corpse.object);
        const auto old_slot=corpse.player&65535u;
        if(new_light && old_slot<multiplayer_capacity && object_light_units[old_slot]==corpse.object)
          light_at->second=object_light_cache[old_slot];
        auto &light=light_at->second;
        const auto origin=pose->sphere.center;
        const auto frame=std::int32_t(frames);
        const float distance=length(origin-camera.position)*source_angles.lighting_distance_scale;
        const bool due=object_lighting_query_due(light,frame,origin,distance,false);
        ObjectLightQueryContext context;
        context.bank_valid=source_object_light_query_bank_valid(lighting);
        context.object_type=0;context.definition_flags=definition.object_flags;
        context.biped_state=corpse.physical_state;
        const ObjectSkyLightingDefinition *sky=nullptr;
        if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<sky_lighting.size() && sky_lighting[fog.visible_sky])
          sky=&*sky_lighting[fog.visible_sky];
        const auto query=due?source_object_light_query(context,
            [&](unsigned mode){return query_object_lightmap(surface_lighting,object_light_collision,origin,
                ObjectLightMapQueryContext{mode,!(light.sample_valid && light.lighting_valid),1.f,light.target});},
            [&]{return query_object_sky_lightmap(surface_lighting,sky,origin);}):ObjectLightQueryAdmission{};
        update_object_lighting_cache(light,object_lighting_definition,0,frame,origin,corpse.velocity,due,
            query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{});
        const auto nodes=source_body_interpolation.sample(corpse.object,blend);
        const auto skinned=skin_model_world_nodes(body.player.model,nodes);
        auto &vertices=corpse_vertices[i].emplace_back();
        const auto collector=source_model_shader_collector(origin,pose->sphere.radius,*object_view.shader);
        body.player.shaders->append_selected(vertices,skinned,collector,*object_view.shader,body.player.material);
        auto colors=cache.load_object_colors(corpse.definition);
        if(corpse.configuration) {
          const auto emblem=source_player_emblem_colors(*corpse.configuration,player_emblem_palette);
          colors[2]=emblem[0];colors[3]=emblem[1];
        }
        model_object_draws[i].push_back({vertices,light.lighting,colors,
            [&,corpse](std::uint32_t name) {
              if(corpse.configuration)if(const auto input=source_object_emblem_input(
                  source_player_object_emblem(*corpse.configuration),player_emblem_scenario,name))return input->value;
              return damage_feedback_shader_input(corpse.damage_feedback,corpse.health,corpse.shield,true,
                  player_damage_functions.at(corpse.definition),*player_damage_function_tables,name,float(shader_time));
            }});
        ++timing.bodies_drawn;
        if(options.animation_observer)options.animation_observer(i,corpse.player&65535u,
            {current.tick,corpse.object,null_datum,body.player.model.animation,null_datum,
                blend,float(current.tick),false,false,vertices,null_datum,0,{},nodes});
      }
      for (unsigned j = 0; j < multiplayer_capacity; ++j)
        if (body_present(shown.players[j]) && (j != id || shown.players[j].dead || vehicle_view)) {
          ++timing.bodies_drawn;
          const auto &a = shown.players[j];
          const auto &body=species_for(a);
          const auto &player=body.player;
          const auto &body_attachment=body.body_attachment;
          const auto &held=body_guns.at(a.weapon);
          const auto &body_gun=held.gun;
          const auto &body_gun_materials=held.materials;
          const auto b = authored_basis({gait[j]&&!a.dead?gait[j]->facing(animation_blend):a.yaw, 0, 0});
          const auto body_frame=source_object_transform(a.position,b.forward,b.up,1,false);
          const bool same_body=previous.players[j].source_unit==a.source_unit &&
              previous.players[j].source_player==a.source_player &&
              previous.players[j].connected && !previous.players[j].dead;
          const double body_tick=same_body?pose_tick:double(current.tick);
          const float body_time=float(body_tick)*tick_seconds;
          const AnimationClip *body_clip=player.idle?&*player.idle:nullptr;
          std::optional<float> body_source_frame;
          if(gait[j]&&gait_units[j]==a.source_unit) {
            const auto presentation=gait[j]->sample(animation_blend);
            body_clip=presentation.clip;body_source_frame=presentation.frame;
          }
          if((a.source_action_word&(1u<<22))&&a.grenade_action.animation_active) {
            const auto *grenade=match.grenade_clip(a.grenade_action.graph,a.grenade_action.clip);
            if(!grenade)throw FormatError("Actual grenade action has no decoded body clip");
            // Ordinary throws occupy the authored masked overlay channel.
            // Preserve the native gait underneath its unauthored leg bones.
            if(grenade->animation_type==0) {
              body_clip=grenade;body_source_frame=a.grenade_action.frame;
            }
          }
          const auto collector=source_model_shader_collector(object_owner_spheres[j].center,
              object_owner_spheres[j].radius,*object_view.shader);
          auto &cached=body_frame_poses[j];
          std::vector<Vertex> body_vertices,gun_vertices;
          std::vector<Transform> body_nodes,held_nodes;
          const auto *copy_pose=std::getenv("HALO2_REFERENCE_COPY_BODY_VERTICES");
          const bool direct_pose=!(copy_pose && *copy_pose);
          if(options.local_players>1 && cached) {
            if(!direct_pose) {body_vertices=cached->body;gun_vertices=cached->weapon;}
            body_nodes=cached->nodes;
            held_nodes=cached->weapon_nodes;
            body_clip=cached->primary;body_source_frame=cached->frame;
          } else {
            ++timing.body_pose_builds;
            if(current.source_body_poses_authoritative)
              body_nodes=source_body_interpolation.sample(body_handle(a),blend);
            else if(body_clip && body.body_animation && !reference_player && !a.dead) {
              const float frame=body_source_frame.value_or(std::fmod(float(body_tick),float(body_clip->frame_count)));
              const float heading=gait[j]?gait[j]->facing(animation_blend):a.yaw;
              NativeLocomotionFrame composed;
              auto locals=compose_native_body(j,a,{body_clip,frame,gait[j]?gait[j]->frame().activity:0x0400000cu},body_tick,
                  std::remainder(a.yaw-heading,6.283185307179586f),&composed);
              body_clip=composed.clip;body_source_frame=composed.frame;
              body_transitions[j].apply(locals,animation_blend);
              body_nodes=pose_model_locals_nodes_world(player.model,body_frame,locals);
            } else body_nodes=pose_model_nodes_world(player.model,body_frame,body_clip,body_time);
            // D0870/B8EE0 aligns the unnamed child to the actual posed hand
            // marker; B93B0 stores parent-local fields, then BD090 composes it.
            if(!a.dead) {
            const bool source_weapon_pose=current.source_body_poses_authoritative &&
                std::any_of(current.source_body_poses.begin(),current.source_body_poses.end(),
                    [&](const auto &pose){return pose.object==a.source_weapon;});
            if(source_weapon_pose)held_nodes=source_body_interpolation.sample(a.source_weapon,blend);
            else {
            const auto parent_node=body_nodes.at(body_attachment.parent.node);
            const auto marker=compose_transform(parent_node,body_attachment.parent.local);
            auto child_marker=body_attachment.child.local;
            if(body_attachment.named_child)child_marker=compose_transform(
                pose_model_node(body_gun.model,body_attachment.child.node,
                    body_gun.idle?&*body_gun.idle:nullptr,body_time),child_marker);
            const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},
                marker,child_marker,body_attachment.named_child);
            const auto local=source_object_attachment_local_frame(aligned,parent_node);
            const auto gun_frame=source_object_transform(local.position,local.forward,local.up,1,false,&parent_node);
            held_nodes=pose_model_nodes_world(body_gun.model,gun_frame,
                body_gun.idle?&*body_gun.idle:nullptr,body_time);
            }
            //113BA0/113980's world callback aligns the support hand to the
            // actual carried weapon markers. FP arms use a separate model.
            if(!current.source_body_poses_authoritative && a.secondary_weapon.datum==null_datum)
              apply_frontend_model_weapon_ik(body.body_ik[a.native_crouching],player.model,
                  body_nodes,held_nodes,source_aim_blend_weight(aim_blends[j]));
            gun_vertices=skin_model_world_nodes(body_gun.model,held_nodes);
            }
            body_vertices=skin_model_world_nodes(player.model,body_nodes);
            if(options.local_players>1) {
              if(direct_pose)cached=BodyFramePose{std::move(body_vertices),std::move(gun_vertices),body_nodes,body_clip,body_source_frame,held_nodes};
              else cached=BodyFramePose{body_vertices,gun_vertices,body_nodes,body_clip,body_source_frame,held_nodes};
            }
          }
          const auto &posed_body=direct_pose && cached?cached->body:body_vertices;
          const auto &posed_gun=direct_pose && cached?cached->weapon:gun_vertices;
          append_shields(world,posed_body,body.body_shields,i,j,false,collector,*object_view.shader);
          if(options.animation_observer)options.animation_observer(i,j,
              {current.tick,a.source_unit,a.source_weapon,body_clip?body_clip->source_graph:player.model.animation,
                body_clip?body_clip->source_index:null_datum,animation_blend,body_source_frame.value_or(float(body_tick)),false,
                same_body && pose_tick!=double(current.tick),posed_body,null_datum,0,held_nodes,body_nodes,
                gait[j]?gait[j]->facing(animation_blend):a.yaw,gait[j]?gait[j]->frame().activity:null_datum});
          if(direct_pose) {
            player.shaders->append_selected(world,posed_body,collector,*object_view.shader,body.player_materials[j]);
            if(!a.dead)body_gun.shaders->append_selected(world,posed_gun,collector,*object_view.shader,body_gun_materials[j*2]);
          } else {
            player.shaders->select(body_vertices,collector,*object_view.shader);
            append_world(world,std::move(body_vertices),body.player_materials[j]);
            if(!a.dead) {
              body_gun.shaders->select(gun_vertices,collector,*object_view.shader);
              append_world(world,std::move(gun_vertices),body_gun_materials[j*2]);
            }
          }
          if(!a.dead && a.secondary_weapon.datum!=null_datum) {
            auto secondary_player=a;secondary_player.weapon=a.secondary_weapon.definition;
            const auto &secondary=species_for(secondary_player);const auto &attachment=*secondary.secondary_attachment;
            const auto marker=compose_transform(body_nodes.at(attachment.parent.node),attachment.parent.local);
            const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},marker,attachment.child.local,false);
            const auto &left_bank=body_guns.at(a.secondary_weapon.definition);
            const auto &left=left_bank.gun;
            auto vertices=pose_model_world(left.model,source_object_transform(aligned.position,aligned.forward,aligned.up,1,false),
                left.idle?&*left.idle:nullptr,body_time);
            left.shaders->append_selected(world,vertices,collector,*object_view.shader,left_bank.materials[j*2+1]);
            bindings.push_back({left_bank.materials[j*2+1],unsigned(left.model.mesh.materials.size()),object_light_cache[j].lighting});
          }
        }
      measure(timing.view[i].body_ms);
      const auto &registered_view=client?client_fp_views[i]:match.first_person_view(i);
      const auto primary_player=p;
      Transform primary_camera_adjustment;
      for(unsigned hand_index=0;hand_index<2;++hand_index) {
        // TU1678C1 leaves both hand registrations NONE before the genuine
        // deferred unit birth. Do not resolve a weapon/model for an absent hand.
        if(registered_view.hands[hand_index].weapon==null_datum)continue;
        if(hand_index && primary_player.secondary_weapon.datum==null_datum)continue;
        auto p=primary_player;
        if(hand_index) {
          const auto &secondary=p.secondary_weapon;
          p.weapon=secondary.definition;p.source_weapon=secondary.datum;p.loaded=secondary.loaded;p.reserve=secondary.reserve;
          p.shot_tick=secondary.shot_tick;p.reload_tick=secondary.reload_tick;
          p.reload_seconds=secondary.reload_seconds;p.reload_empty=secondary.reload_empty;
          p.ammunition_available=secondary.ammunition_available;
        }
        const auto &v=species_for(p);const auto &weapon=content.weapons.at(p.weapon);
        const auto &gun=v.guns[i*2+hand_index];const auto &hands=v.hands;
        const auto &fp_idle=v.fp_idle;const auto &fp_reload=v.fp_reload;const auto &fp_reload_empty=v.fp_reload_empty;
        const auto &fp_fire=v.fp_fire;const auto &fp_definition=v.fp_definition;const auto &fp_ik=v.fp_ik;
        auto fp_view=registered_view;fp_view.hands[0]=registered_view.hands[hand_index];
      if (!p.dead && !vehicle_view && fp_view.hands[0].weapon!=null_datum) {
        const auto *clip = &fp_idle;
        float time = float(pose_tick) * tick_seconds;
        std::optional<float> source_frame;
        bool frame_interpolated{};
        if (p.reload_seconds > 0 && fp_reload) {
          clip = p.reload_empty && fp_reload_empty ? &*fp_reload_empty : &*fp_reload;
          time = float(std::max(0.0,pose_tick-double(p.reload_tick))) * tick_seconds;
        } else if (p.shot_tick && current.tick - p.shot_tick < 4 && fp_fire) {
          clip = &*fp_fire;
          time = float(std::max(0.0,pose_tick-double(p.shot_tick))) * tick_seconds;
        }
        if(fp_definition && !reference_player) {
          const auto &binding=fp_view;
          const auto &hand=binding.hands[0];
          auto origin=client?client_fp_origins[i]:match.first_person_clock_origin(i);
          origin.hands[0]=origin.hands[hand_index];
          const auto presentation=first_person_render_frame(origin,binding,*fp_definition,animation_blend,
              client ? (hand_index?client_secondary_clock_available[i]:client_fp_clock_available[i])
                     : current.tick>previous.tick && current.tick-previous.tick==1);
          source_frame=presentation.frame;
          frame_interpolated=presentation.interpolated;
          const auto index=hand.animation_identifier>>16;
          clip=nullptr;
          const auto found=v.fp_clips.find(index);
          if(found!=v.fp_clips.end())clip=&found->second;
          if(!clip)throw FormatError("Registered first-person clock has no decoded presentation clip");
        }
        if(reference_player) {
          clip=&fp_idle;
          time=float(reference_player->pose_frame)*tick_seconds;
        }
        const auto f = view_forward;
        const auto u = camera.oriented?camera.up:cross(f,{-std::sin(camera.yaw), std::cos(camera.yaw),0});
        const auto transform = camera_transform(camera.position, f, u);
        const auto aiming=primary_player.secondary_weapon.datum!=null_datum?0.f:
            source_aim_blend_weight(reference_player?reference_player->aiming:aim_blends[id]);
        const AnimationClip *ammunition=nullptr;float ammunition_frame=0;
        if(fp_definition && fp_definition->ammunition[fp_view.hands[0].dual_wield?1:0]) {
          ammunition=&*fp_definition->ammunition[fp_view.hands[0].dual_wield?1:0];
          // Read the authoritative104CF0 inventory sum, including inactive
          // matching owners. Clients do not reconstruct the inventory here.
          const int available=p.ammunition_available;
          ammunition_frame=first_person_ammunition_frame(std::int16_t(p.loaded),
              std::bit_cast<std::int16_t>(std::uint16_t(available)),std::int16_t(weapon.capacity),
              std::bit_cast<std::int16_t>(std::uint16_t(current.tick-p.reload_tick)),weapon.weapon_type,
              fp_view.hands[0].activity,ammunition->frame_count);
        }
        auto pose=source_frame
            ? pose_first_person_models_world_frame(gun.model,hands.model,*clip,*source_frame,
                weapon.first_person_offset,transform,fp_ik,aiming,hand_index!=0,
                hand_index?&primary_camera_adjustment:nullptr,ammunition,ammunition_frame)
            : pose_first_person_models_world(gun.model,hands.model,*clip,time,
                weapon.first_person_offset,transform,fp_ik,aiming,hand_index!=0,
                hand_index?&primary_camera_adjustment:nullptr,ammunition,ammunition_frame);
        if(!hand_index)primary_camera_adjustment=pose.camera_adjustment;
        if(primary_player.secondary_weapon.datum!=null_datum) {
          //169965 keeps only authored left-arm graph nodes for the second
          //hand. Both draws retain their authored winding and tangent basis.
          std::vector<Vertex> arm;
          for(std::size_t triangle=0;triangle+2<pose.arms.size();triangle+=3) {
            bool selected=false;
            for(unsigned k=0;k<3;++k) {
              const auto &skin=hands.model.skin.at(triangle+k);
              for(unsigned n=0;n<4;++n)if(skin.weights[n]>0) {
                const auto name=hands.model.bones.at(skin.bones[n]).name;
                const auto node=std::find(clip->nodes.begin(),clip->nodes.end(),name);
                if(node==clip->nodes.end())throw FormatError("Dual arm node has no original graph mapping");
                const bool left=clip->node_flags.at(node-clip->nodes.begin())&0x20u;
                if(left==(hand_index!=0))selected=true;
              }
            }
            if(selected)arm.insert(arm.end(),pose.arms.begin()+triangle,pose.arms.begin()+triangle+3);
          }
          pose.arms=std::move(arm);
        }
        if(options.animation_observer)options.animation_observer(i,id,
            {current.tick,p.source_unit,p.source_weapon,clip->source_graph,clip->source_index,
              animation_blend,source_frame?*source_frame:time*30.f,true,frame_interpolated,pose.weapon,
              ammunition?ammunition->source_index:null_datum,ammunition_frame,pose.weapon_nodes});
        if(!v.effect_markers.empty()) {
          const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,true});
          MuzzleEffectView effects_view;
          effects_view.camera=source_view_shader_bank(render_camera,1024.f,camera.position,
              full_surface_viewport({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top},drawable_w,drawable_h),true);
          effects_view.fog=bsp_fog_constants(fog,camera.position,view_forward);
          const auto &basis=render_camera.camera_to_world.basis;
          effects_view.orientation={basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,
              basis[2].x,basis[2].y,basis[2].z};
          effects_view.position=camera.position;effects_view.forward=f;effects_view.up=u;
          effects_view.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
          effects_view.width=drawable_w;effects_view.height=drawable_h;
          auto particles=muzzle_effects.draw(p.source_weapon,effects_view,current.tick,animation_blend,tick_seconds);
          foreground_particles[i].insert(foreground_particles[i].end(),particles.begin(),particles.end());
          if(options.effect_observer)options.effect_observer(i,current.tick,foreground_particles[i]);
        }
        const auto owner=source_model_shader_collector(object_owner_spheres[id].center,
            object_owner_spheres[id].radius,*object_view.shader);
        bindings.push_back({hands.material,unsigned(hands.model.mesh.materials.size()),first_person_lighting});
        bindings.push_back({gun.material,unsigned(gun.model.mesh.materials.size()),first_person_lighting});
        // Original133AB0 shield recursion reaches133F70 with owner filtering:
        // pose.arms is the complete character FP model: arms, hands and fingers.
        // The weapon's separate object owner is excluded from this layer.
        append_shields(foregrounds[i],pose.arms,v.hand_shields,i,id,true,owner,*object_view.shader);
        const auto *reference_append=std::getenv("HALO2_REFERENCE_COPY_FIRST_PERSON");
        if(reference_append && *reference_append) {
        hands.shaders->select(pose.arms,owner,*object_view.shader);
        gun.shaders->select(pose.weapon,owner,*object_view.shader);
        append(foregrounds[i], std::move(pose.arms), hands.material, {});
        append(foregrounds[i], std::move(pose.weapon), gun.material, {});
        } else {
          const auto first=foregrounds[i].size();
          hands.shaders->append_selected(foregrounds[i],pose.arms,owner,*object_view.shader,hands.material);
          gun.shaders->append_selected(foregrounds[i],pose.weapon,owner,*object_view.shader,gun.material);
          // Keep append's original identity placement arithmetic, including
          // its signed-zero behavior. Selection and the material base are
          // already applied once per accepted part in the destination bank.
          place_vertices(std::span{foregrounds[i]}.subspan(first),0,{});
#ifdef HALO2_PROFILE_MATERIAL_FRAME_ORACLE
          if(std::getenv("HALO2_VERIFY_FIRST_PERSON_APPEND")) {
            std::vector<Vertex> expected;
            hands.shaders->select(pose.arms,owner,*object_view.shader);
            gun.shaders->select(pose.weapon,owner,*object_view.shader);
            append(expected,std::move(pose.arms),hands.material,{});
            append(expected,std::move(pose.weapon),gun.material,{});
            const auto actual=std::span{foregrounds[i]}.subspan(first);
            if(actual.size()!=expected.size() || (!actual.empty() &&
                std::memcmp(actual.data(),expected.data(),actual.size_bytes())))
                throw FormatError("direct first-person append changed complete original posed vertices");
          }
#endif
        }
      }
      }
      measure(timing.view[i].first_person_ms);
      ModelView view;
      view.camera = camera;
      if(reference_player)view.source_view_clear=reference_player->view_clear;
      view.left = rect.left;
      view.top = rect.top;
      view.width = rect.right - rect.left;
      view.height = rect.bottom - rect.top;
      view.vertices = world;
      view.full_surface = true;
      view.foreground = foregrounds[i];
      view.foreground_particles=foreground_particles[i];
      view.world_particles=world_particles[i];
      view.raw_effect_lights=raw_effect_lights[i];
      if(const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_BSP_EFFECT_LIGHTS");disabled && *disabled)
        view.raw_effect_lights={}; // Pixel-isolation control; simulation and collection still run.
      view.weather_points=weather_points[i];
      view.weather_lines=weather_lines[i];
      view.weather_plates=weather_plates[i];
      view.background = backgrounds[i];
      view.atmosphere = fog;
      view.atmosphere_source = &atmosphere;
      view.water=water_frames[i];
      view.effect_time=shader_time;
      view.object_lighting=bindings;
      view.objects=model_object_draws[i];
      view.source_render_policy=source_geometry_lod_policy(options.local_players);
      view.source_bsp_admission=&bsp_views[i];
      if(bsp_shaders) {
        view.source_bsp_resident_plan=&*bsp_shaders;
        view.source_bsp_bindings=bsp_vertex_plans[i].bindings;
      }
      if(!p.dead && weapon.first_person_scope_size.x>0 && weapon.first_person_scope_size.y>0)
        view.scope=scope_copy_frame(scope_source_rectangle(physical(rect),weapon.first_person_scope_size,
            aim_anchor,{float(drawable_w),float(drawable_h)}));
      view.water_reflection_background=reflection_backgrounds[i];
      view.water_reflection_camera=camera;
      if(!water.definitions.empty()) {
        view.water_reflection_camera.position.z=2.f*water.definitions.front().parameters[33]-camera.position.z;
        view.water_reflection_camera.oriented=true;
        const auto f=view_forward;
        view.water_reflection_camera.forward={f.x,f.y,-f.z};
        view.water_reflection_camera.up={0,0,-1};
        const auto &reflected=view.water_reflection_camera;
        const auto reflection_camera=source_view_projection({reflected.position,
            reflected.forward,reflected.up,.015625f,1024.f,{},
            frustum_projection.projection_program});
        source_view_constants.reflection=source_view_shader_constants(reflection_camera,1024.f,false);
      }
      view.source_view_shader_constants=source_view_constants;
      if(options.view_observer)options.view_observer(i,camera,horizontal_fov,view_zoom[i]);
      if(options.view_state_observer)options.view_state_observer(i,current,camera);
      view.before_ui_vertex = hud_vertices.size();
      views.push_back(view);
      hud.animate(current, id, i, current.score_limit, current.time_ticks,
                  float(current.tick) * tick_seconds);
      unit_hud.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
      if(uploaded_sensor_tick[i]!=current.tick) {
        if(const auto material=unit_hud.motion_sensor_material(i))
          renderer.update_image(*material,mesh.materials.at(*material).diffuse);
        uploaded_sensor_tick[i]=current.tick;
      }
      const auto board=scoreboard.draw(current,ids,i,drawable_w,drawable_h);
      if(board.normal_hud) {
        unit_hud.prompt_gamepad(i,prompt_gamepad[i]);
        unit_hud.prompt_boarding_buttons(i,platform.desktop->keyboard_settings,platform.desktop->controller_settings);
        unit_hud.prompt_gamepad_buttons(i,native_action_prompt_controller_buttons(platform.desktop->controller_settings));
        unit_hud.prompt_keyboard_buttons(i,native_action_prompt_keyboard_buttons(platform.desktop->keyboard_settings));
        auto vertices=unit_hud.draw(current,id,i,options.local_players,drawable_w,
            drawable_h,current.score_limit,current.time_ticks,aim_anchor);
        auto weapon_vertices=hud.draw(current,id,i,options.local_players,drawable_w,
            drawable_h,current.score_limit,current.time_ticks,aim_anchor);
        vertices.insert(vertices.end(),weapon_vertices.begin(),weapon_vertices.end());
        if(p.secondary_weapon.datum!=null_datum) {
          auto &secondary_hud=*offhand_huds.at({multiplayer_representation(p.source_configuration),p.secondary_weapon.definition});
          secondary_hud.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
          auto secondary_vertices=secondary_hud.draw(current,id,i,options.local_players,drawable_w,drawable_h,
              current.score_limit,current.time_ticks,aim_anchor);
          vertices.insert(vertices.end(),secondary_vertices.begin(),secondary_vertices.end());
        }
        if(platform.desktop->display_settings.safe_area) {
          // PC664A18: margin is five percent of view height. HUD draw vertices
          // already use the shared centered480-high presentation canvas.
          const float cx=view.left+view.width*.5f,cy=view.top+view.height*.5f;
          const float margin=view.height*.05f*(float(drawable_h)/480.f)/(float(drawable_w)/640.f);
          const float sx=1.f-2.f*margin/view.width;
          for(auto &v:vertices){v.position.x=cx+(v.position.x-cx)*sx;v.position.y=cy+(v.position.y-cy)*.9f;}
        }
        hud_vertices.insert(hud_vertices.end(),vertices.begin(),vertices.end());
      }
      // PC6278AA/62657B draws47554D's viewport black quad before the
      // separate score display. Keep it after the actual world/HUD and before
      // the score vertices, using the returned packed alpha without retiming.
      if(options.session_view_black)if(const auto color=options.session_view_black(i)) {
        const float alpha=float(*color>>24)/255.f;
        const Vec3 rgb{float((*color>>16)&255)/255.f,
            float((*color>>8)&255)/255.f,float(*color&255)/255.f};
        const auto material=*session_black_material;
        const Vertex a{{view.left,view.top,alpha},rgb,{0,0},material};
        const Vertex b{{view.left+view.width,view.top,alpha},rgb,{1,0},material};
        const Vertex c{{view.left+view.width,view.top+view.height,alpha},rgb,{1,1},material};
        const Vertex d{{view.left,view.top+view.height,alpha},rgb,{0,1},material};
        hud_vertices.insert(hud_vertices.end(),{a,b,c,a,c,d});
      }
      hud_vertices.insert(hud_vertices.end(),board.paint.draw.vertices.begin(),board.paint.draw.vertices.end());
      if(options.scoreboard_observer)options.scoreboard_observer(i,board.normal_hud,board.layout,board.paint);
      measure(timing.view[i].hud_ms);
    }
    std::vector<UiBackdrop> pause_backdrops;
    if(pause_menu && pause_menu->visible()) {
      auto overlay=pause_menu->draw(drawable_w,drawable_h);
      for(auto backdrop:overlay.backdrops) {
        backdrop.before_vertex+=hud_vertices.size();pause_backdrops.push_back(backdrop);
      }
      hud_vertices.insert(hud_vertices.end(),overlay.vertices.begin(),overlay.vertices.end());
    }
    const auto *all_materials=std::getenv("HALO2_REFERENCE_ALL_MATERIALS");
    const bool use_active_materials=!(all_materials && *all_materials);
    const auto active_roots=[&] {
    if(!use_active_materials)return std::vector<unsigned>{};
    // The initialized bank adds no new pass links in the frame loop. Color
    // and camouflage producers can detach copies of existing children only.
    const auto *recheck_graph=std::getenv("HALO2_REFERENCE_RECHECK_MATERIAL_GRAPH");
    if(recheck_graph && *recheck_graph)material_frame.begin_frame(mesh.materials);
    else material_frame.begin_frame_with_detach_only_pass_graph(mesh.materials);
    material_frame.add(hud_vertices);
    for(const auto &view:views) {
      material_frame.add(view.vertices);material_frame.add(view.foreground);
      material_frame.add(view.background);material_frame.add(view.water_reflection_background);
      for(const auto &binding:view.source_bsp_bindings)if(binding.material)material_frame.add(*binding.material);
      for(const auto &object:view.objects)material_frame.add(object.vertices);
      for(const auto &draw:view.effect_lights)material_frame.add(draw.material_slot);
      for(const auto &draw:view.raw_effect_lights)material_frame.add(draw.material_slot);
      for(const auto *bank:{&view.world_transparent,&view.foreground_transparent})if(*bank)
        for(const auto &binding:(*bank)->bindings)
          if(const auto *geometry=std::get_if<SourceTransparentGeometryDraw>(&binding))material_frame.add(geometry->vertices);
      if(view.source_shadows)for(const auto &root:view.source_shadows->roots) {
        for(const auto &draw:root.casters)material_frame.add(draw.material_slot);
        for(const auto &draw:root.receivers)material_frame.add(draw.material_slot);
      }
    }
    return material_frame.roots();
    }();
    auto animate=[&](auto &self,Material &material,const ShaderInput &input)->void {
      if(!material.forward_pass)return;
      try {
      animate_forward_material(material,shader_time,input);
      animate_material_bitmaps(material,shader_time,input);
      } catch(const FormatError &error) {
        throw FormatError("Multiplayer shader "+material.name+": "+error.what());
      }
      if(material.environment_pass)self(self,*material.environment_pass,input);
      for(auto &pass:material.world_overlays_before)self(self,*pass,input);
      for(auto &pass:material.world_overlays_after)self(self,*pass,input);
      for(auto &pass:material.world_bloom_passes)self(self,*pass,input);
    };
    const auto animate_bank=[&](bool all) {
    const auto count=all?mesh.materials.size():active_roots.size();
    for(unsigned index=0;index<count;++index) {
      const auto m=all?index:active_roots[index];
      auto &material=mesh.materials[m];
      if(material.hud_pass)continue;
      if(material_camouflage[m])bind_object_camouflage(material,*material_camouflage[m]);
      int ammunition=0;
      std::uint32_t material_weapon=null_datum,material_definition=null_datum;
      SourceWeaponShaderState shader_state;
      for(const auto &[representation,v]:species)
        for(unsigned i=0;i<ids.size();++i)
          for(unsigned hand=0;hand<2;++hand) {
            const auto &gun=v.guns[i*2+hand];
            if(m>=gun.material && m<gun.material+gun.model.mesh.materials.size()) {
              const auto &player=current.players[ids[i]];
              if((hand?player.secondary_weapon.definition:player.weapon)!=representation.second)continue;
              ammunition=hand?player.secondary_weapon.loaded:player.loaded;
              material_weapon=hand?player.secondary_weapon.datum:player.source_weapon;
              material_definition=representation.second;
              shader_state=hand?player.secondary_weapon.shader:player.weapon_shader;
            }
          }
      for(const auto &[datum,body]:body_guns)for(unsigned i=0;i<object_slots;++i)
        for(unsigned hand=0;hand<2;++hand)if(m>=body.materials[i*2+hand] &&
            m<body.materials[i*2+hand]+body.gun.model.mesh.materials.size()) {
          const auto &player=current.players[i];
          if((hand?player.secondary_weapon.definition:player.weapon)!=datum)continue;
          ammunition=hand?player.secondary_weapon.loaded:player.loaded;
          material_weapon=hand?player.secondary_weapon.datum:player.source_weapon;
          material_definition=datum;
          shader_state=hand?player.secondary_weapon.shader:player.weapon_shader;
        }
      const SourceWorldWeaponDefinition *shader_definition=material_definition==null_datum?nullptr:
          &weapon_shader_definitions.at(material_definition);
      std::vector<SourceWeaponMagazineState> magazines;
      if(shader_definition && shader_definition->magazines.size()==1)
        magazines.push_back({std::int16_t(ammunition),0});
      const ShaderInput input=[&](std::uint32_t name) {
        if(player_damage_owners[m]>=0) {
          const auto &owner=current.players[unsigned(player_damage_owners[m])];
          return damage_feedback_shader_input(owner.damage_feedback,owner.health,
              owner.shield,owner.dead,player_damage_functions.at(biped_for(owner).datum),
              *player_damage_function_tables,name,float(shader_time));
        }
        if(shader_definition) {
          if(const auto value=source_weapon_shader_input(name,shader_state))return *value;
          // Native combat currently owns magazine0 only. The recovered
          // consumer admits that actual bank and its authored identity
          // functions; unsupported heat/power producers remain separate.
          const bool ammunition_function=std::any_of(shader_definition->functions.begin(),
              shader_definition->functions.end(),[&](const auto &f){return f.output==name && f.input==0x1200056au;});
          if((name==0x1200056au || ammunition_function) &&
             shader_definition->magazines.size()==magazines.size())
            if(const auto value=source_weapon_magazine_shader_input(name,shader_definition->magazines,
                magazines,shader_definition->functions,*cache.shader_function_tables()))return value->value;
        }
        return weapon_ammunition_input(name,ammunition).value_or(0.f);
      };
      if(player_armor_owners[m]>=0) {
        if(rules.custom==CustomSlayer::infection) {
          const auto &profile=current.players[unsigned(player_armor_owners[m])].source_configuration;
          if(profile) {
            auto colors=material.object_render_colors.value_or(ObjectColors{});
            for(unsigned channel=0;channel<2;++channel) {
              const auto index=unsigned((*profile)[0x40+channel]);
              if(index<player_emblem_palette.size()) {
                const auto &rgb=player_emblem_palette[index];
                colors[channel]={std::bit_cast<float>(rgb[0]),std::bit_cast<float>(rgb[1]),std::bit_cast<float>(rgb[2])};
              }
            }
            const auto packed=source_object_render_colors(colors);colors[0]=packed[0];colors[1]=packed[1];
            apply_object_render_colors(material,colors);
          }
        }
        animate_player_armor_emblem(material,shader_time,
            current.players[unsigned(player_armor_owners[m])].source_configuration,
            player_emblem_scenario,player_emblem_palette,input);
        if(options.player_armor_observer)
          options.player_armor_observer(unsigned(player_armor_owners[m]),material);
      } else animate(animate,material,input);
#ifdef HALO2_SHIELD_MATERIAL_TRACE
      if(options.capture_tick && current.tick==options.capture_tick && player_damage_owners[m]>=0 &&
          player_armor_owners[m]<0) {
        std::cout<<"Shield material tick="<<current.tick<<" slot="<<m<<" owner="<<player_damage_owners[m]
            <<" shader="<<std::hex<<material.source_shader_datum<<std::dec
            <<" spartan_input="<<input(0x0a0012b4u)<<" elite_input="<<input(0x12001a0bu)<<" program=";
        for(const auto word:material.forward_program.words)std::cout<<std::hex<<word<<' ';
        std::cout<<std::dec<<'\n';
      }
#endif
      if(options.weapon_material_observer && material_weapon!=null_datum)
        options.weapon_material_observer(material_weapon,material_definition,material,input);
    }
    };
    animate_bank(!use_active_materials);
#ifdef HALO2_PROFILE_MATERIAL_FRAME_ORACLE
    if(use_active_materials && std::getenv("HALO2_VERIFY_ACTIVE_MATERIALS")) {
      verify_material_frame(mesh.materials,active_roots,[&]{animate_bank(true);});
      if(frames%60==0)std::cerr<<"[material-frame-oracle] frame="<<frames<<" active="<<active_roots.size()
          <<" resident="<<mesh.materials.size()<<" equal\n";
    }
#endif
    if(use_active_materials)renderer.update_material_constants(mesh.materials,active_roots);
    else renderer.update_material_constants(mesh.materials);


    {
      auto observation=observe_debug_game(match.simulation(),cache.spawns());
      observation.mode=client?"LAN client":host?"LAN host":"Local multiplayer";
      observation.available=!host && !client;
      observation.ai_available=observation.snapshots_available=observation.checkpoint_available=false;
      observation.limitation=observation.available?
          "Select a local player. Enemy targeting applies only to campaign AI. Local player count is selected before the match.":
          "Gameplay debug controls are disabled in network sessions.";
      observation.players.clear();observation.snapshots.clear();
      for(const auto id:ids) {
        const auto& player=current.players[id];
        observation.players.push_back({id,player.name,player.position,player.health,player.shield,
            player.loaded,player.reserve,player.dead});
      }
      panel.observe(std::move(observation));
    }
    const bool capture = !options.screenshot.empty() &&
        (options.capture_tick ? current.tick == options.capture_tick : frames + 1 == options.frames);
    if (capture && options.capture_tick) {
      std::cout << "Input capture tick=" << current.tick << " animation_fraction=" << animation_blend << " pose_tick=" << pose_tick;
      std::cout<<" capture_tick_cadence="<<options.reference_tick_cadence
          <<" capture_auxiliary_start="<<capture_auxiliary_start
          <<" capture_auxiliary_final="<<auxiliary_random
          <<" capture_primary_final="<<visual_event_primary;
      const auto effect_state=world_effects.diagnostics();
      std::cout<<" impacts="<<world_impact_serial<<" detonations="<<world_detonation_serial
               <<" decals="<<effect_state.decals<<" world_particles="<<effect_state.particles
               <<" world_lights="<<effect_state.lights;
      const auto light_state=world_effects.lights();
      std::cout<<" registered_lights="<<std::count_if(light_state.begin(),light_state.end(),
          [](const auto& light) {return (light.registration.flags&8)!=0;});
      for(unsigned view=0;view<effect_light_views.size();++view) {
        const auto& lights=effect_light_views[view];
        std::cout<<" light_view="<<view<<" light_epoch="<<lights.epoch
            <<" visible_lights="<<lights.visible.size()<<" selected_lights="<<lights.selected.size();
        std::cout<<" geometry_light_view="<<view
            <<" raw_light_packets="<<views.at(view).raw_effect_lights.size()
            <<" packed_light_packets="<<views.at(view).effect_lights.size();
        const auto& receivers=effect_light_receivers[view];
        std::size_t clusters=0,instances=0;
        for(const auto& receiver:receivers){clusters+=receiver.view.bsp.clusters.size();instances+=receiver.view.bsp.instances.size();}
        std::cout<<" receiver_view="<<view<<" receiver_queries="<<receivers.size()
            <<" receiver_epoch="<<(receivers.empty()?lights.epoch:receivers.back().epoch)
            <<" receiver_clusters="<<clusters<<" receiver_instances="<<instances;
        for(const auto& record:lights.records) {
          const auto& r=record.runtime;
          std::cout<<" light_record_view="<<view<<" light_identity="<<r.identity
              <<" light_cluster="<<r.cluster<<" light_radius="<<r.spatial_radius
              <<" light_fade="<<r.overall_fade<<" light_center="<<r.spatial_center.x<<','<<r.spatial_center.y<<','<<r.spatial_center.z
              <<" light_diffuse="<<r.state.diffuse.x<<','<<r.state.diffuse.y<<','<<r.state.diffuse.z;
        }
      }
      if(!current.grenade_detonations.empty()){
        const auto&d=current.grenade_detonations.back();
        std::cout<<" detonation_tick="<<d.tick<<" detonation_position="<<d.position.x<<','<<d.position.y<<','<<d.position.z;
      }
      for (unsigned view=0;view<ids.size();++view) {
        const auto id=ids[view];
        const auto &p = current.players[id];
        std::cout << " slot=" << id << " loaded=" << p.loaded << " reserve=" << p.reserve
                  << " reload_empty=" << p.reload_empty << " reload_seconds=" << p.reload_seconds
                  << " player_position="<<p.position.x<<','<<p.position.y<<','<<p.position.z;
        if(!client && weapon.smg) {
          const auto &hand=match.first_person_view(view).hands[0];
          std::cout << " fp_clip=" << (hand.animation_identifier>>16) << " fp_frame=" << hand.frame
                    << " fp_fire_ticks=" << hand.fire_ticks;
        }
      }
      std::cout << '\n';
    }
    measure(timing.common_ms);
    if(bsp_reference_scene) {
      const auto postprocess_before=renderer.bsp_postprocess_state();
      auto reference_views=views;
      std::vector<std::vector<Vertex>> reference_worlds(views.size());
      for(std::size_t i=0;i<views.size();++i) {
        auto &vertices=reference_worlds[i];
        vertices=bsp_shaders->expand(views[i].source_bsp_bindings);
        vertices.insert(vertices.end(),views[i].vertices.begin(),views[i].vertices.end());
        reference_views[i].vertices=vertices;
        reference_views[i].source_bsp_resident_plan=nullptr;
        reference_views[i].source_bsp_bindings={};
      }
      if(!renderer.draw(reference_views.front().camera,options.bsp_reference_screenshot,
          {},false,{},{},hud_vertices,reference_views))
        throw FormatError("Paired source-copy renderer did not submit its capture");
      // prepare() advances authored patchy wind even at identical elapsed time.
      // Both captures consume the same pre-frame motion/RNG; final state advances once.
      if(postprocess_before)renderer.restore_bsp_postprocess_state(*postprocess_before);
    }
    timing.submitted=renderer.draw(views.front().camera,
                      capture ? options.screenshot : std::filesystem::path{},
                      {}, false, {}, {}, hud_vertices, views,{},pause_backdrops);
    measure(timing.renderer_ms);
    if(options.frame_observer) {
      timing.renderer=renderer.frame_timing();
      timing.wall_ms=std::chrono::duration<double,std::milli>(Clock::now()-now).count();
      options.frame_observer(timing);
    }
    if (timing.submitted) {
      ++frames;
      if (capture && options.capture_tick) {
        running = false;
        exit_reason = MultiplayerExitReason::captured_tick;
      }
    }
    else
      SDL_Delay(10);
  }
  std::cout << "Multiplayer frames=" << frames << " ticks=" << current.tick
            << " players="
            << std::count_if(current.players.begin(), current.players.end(),
                             [](const auto &p) { return p.connected; })
            << " connected=" << joined << " peak_players=" << peak_players << '\n';
  retain_outcome(exit_reason, current, joined);
  for(auto *pad:platform.pads)send_controller_rumble(pad,{},false,false,true);
  if(exit_reason==MultiplayerExitReason::session_world_close)
    platform.desktop->begin_loading();
  return 0;
}
int run_multiplayer(const MultiplayerOptions &options) {
  return run_multiplayer_impl(options, nullptr);
}
int run_multiplayer(const MultiplayerOptions &options,
                    MultiplayerRuntimeOutcome &outcome) {
  return run_multiplayer_impl(options, &outcome);
}
} // namespace halo2
