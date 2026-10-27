#include "halo2/pstencil/player_detail.hpp"
#include "halo2/source_projectile_effect_observer.hpp"
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
#include "halo2/held_pose_shading_reference.hpp"
#endif
#include "rendering/renderer/material_frame_selection.hpp"
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
#include "halo2/vehicle_shading_reference.hpp"
#endif
#include "rendering/models/model_world_pose_cache.hpp"
#include "rendering/models/model_frame_pose_cache.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include "gameplay/objects/source_object_geometry.hpp"
#ifdef HALO2_PROFILE_MATERIAL_FRAME_ORACLE
#include "rendering/renderer/material_frame_oracle.hpp"
#endif
#include "halo2/audio.hpp"
#include "halo2/rumble.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_map_effects.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_model_build.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/unit_damage_audio.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/model_skin_submission.hpp"
#include <cfenv>
#include <cerrno>
#include "halo2/source_model_regions.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/native_vista_announcer.hpp"
#include "halo2/source_king_draw.hpp"
#include "halo2/source_transparent_queue.hpp"
#include "halo2/source_device_event.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_receipt_continuity.hpp"
#include "halo2/multiplayer_pose_continuity.hpp"
#include "halo2/multiplayer_scene_continuity.hpp"
#include "halo2/multiplayer_locomotion_continuity.hpp"
#include "halo2/multiplayer_scene_runtime_values.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/source_foreground_world_handoff.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include <iterator>
#include "halo2/source_lens_presentation_handoff_tu15.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/source_weapon_material_input.hpp"
#include "halo2/damage_feedback_names.hpp"
#include "halo2/source_weapon_firing_material_owner.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/native_locomotion.hpp"
#include "halo2/native_unit_interaction.hpp"
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
#include "halo2/gameplay_fov.hpp"
#include "halo2/texture.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/source_admitted_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_weather_world.hpp"
#include "halo2/source_weather_sprite_profile.hpp"
#include "halo2/source_weather_rsqrt.hpp"
#include "halo2/source_weather_execution.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_character_shaders.hpp"
#include "halo2/source_shield_model_shaders.hpp"
#include "halo2/shield_overlay.hpp"
#include "halo2/source_camouflage_context.hpp"
#include "halo2/source_camouflage_material.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_netgame_weapon_scene.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/source_first_person_packet_tu15.hpp"
#include "halo2/source_first_person_draws_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif
#include "halo2/scenery.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_live_shadow_phase_inputs_tu15.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/source_grenade_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/native_lens_presentation_tu15.hpp"
#include "halo2/source_combat_attachments.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/pstencil/point_lights.hpp"
#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/path_tracing/sky_lighting.hpp"
#include "halo2/source_contrail_draw.hpp"
#include "halo2/source_prop_initial_state.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_scenery_initial_state.hpp"
#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/hud.hpp"
#include "halo2/objective_waypoints.hpp"
#include "halo2/objective_waypoint_memory_tu15.hpp"
#include "halo2/math.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/multiplayer_presentation.hpp"
#include "halo2/replica_motion.hpp"
#include "halo2/adaptive_interpolation.hpp"
#include "halo2/predicted_impacts.hpp"
#include "halo2/nonhost_movement_client.hpp"
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
#include "halo2/system_link_session.hpp"
#include "halo2/game_voice.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/pc_mouse_look.hpp"
#include "halo2/gameplay_mouse_capture.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/sky.hpp"
#include "halo2/transform.hpp"
#include "halo2/water.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <thread>
#include <ctime>
#include <type_traits>
#include <atomic>
#ifdef __linux__
#include <time.h>
#endif
namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
// Joined PreparedWorlds deliberately have no native gameplay World. This
// receiver-only owner supplies genuine constructors/assets for FP sampling;
// it is never installed into Match or advanced as a simulation authority.
struct NativeFirstPersonRecipientAssets {
  struct Storage {
    std::filesystem::path path;
    Storage() {
      static std::atomic<std::uint64_t> serial{};
      const auto base=std::filesystem::temp_directory_path();
      for(unsigned attempt=0;attempt<32;++attempt) {
        const auto name="halo2-fp-assets-"+std::to_string(Clock::now().time_since_epoch().count())+
            "-"+std::to_string(serial.fetch_add(1,std::memory_order_relaxed));
        auto candidate=base/name;
        if(std::filesystem::create_directory(candidate)){path=std::move(candidate);return;}
      }
      throw FormatError("Native FP recipient could not allocate owned asset storage");
    }
    ~Storage(){if(!path.empty()){std::error_code error;std::filesystem::remove_all(path,error);}}
  };
  std::shared_ptr<Storage> storage;
  std::shared_ptr<MultiplayerObjectRuntime> runtime;
  NativeFirstPersonRecipientAssets(const Cache &cache,std::size_t bsp,
      const SourceGameVariant &variant,std::uint32_t seed)
      :storage(std::make_shared<Storage>()),runtime(
          new MultiplayerObjectRuntime(cache,bsp,storage->path),
          [files=storage](MultiplayerObjectRuntime *owner){delete owner;}) {
    runtime->configure_session(variant,1,30,seed);
  }
};
std::optional<double> observed_thread_cpu_ms() noexcept {
#ifdef __linux__
  timespec value{};
  if(::clock_gettime(CLOCK_THREAD_CPUTIME_ID,&value)==0 && value.tv_sec>=0 &&
      value.tv_nsec>=0 && value.tv_nsec<1000000000)
    return double(value.tv_sec)*1000.+double(value.tv_nsec)/1000000.;
#endif
  return std::nullopt;
}
MultiplayerFrameTiming::Stamp observed_schedule_stamp(Clock::time_point wall)noexcept {
  MultiplayerFrameTiming::Stamp stamp;
  stamp.wall_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(wall.time_since_epoch()).count();
  if(const auto cpu=observed_thread_cpu_ms();cpu&&*cpu>=0){stamp.thread_cpu_ms=*cpu;stamp.thread_cpu_available=true;}
  return stamp;
}
// Null observations do not read clocks, allocate, increment counts or log.
// Scope exit also records failed work without changing exception propagation.
class ObservedReceiptWork {
  MultiplayerFrameTiming::Work *work_{};
  Clock::time_point wall_{};
  std::optional<double> cpu_;
public:
  explicit ObservedReceiptWork(MultiplayerFrameTiming::Work *work) noexcept:work_(work) {
    if(work_){wall_=Clock::now();cpu_=observed_thread_cpu_ms();}
  }
  ~ObservedReceiptWork() noexcept {
    if(!work_)return;
    const auto cpu=observed_thread_cpu_ms();
    const auto wall=Clock::now();
    work_->wall_ms+=std::chrono::duration<double,std::milli>(wall-wall_).count();
    ++work_->samples;
    if(cpu_&&cpu&&*cpu>=*cpu_){work_->thread_cpu_ms+=*cpu-*cpu_;++work_->thread_cpu_samples;}
  }
  ObservedReceiptWork(const ObservedReceiptWork &)=delete;
  ObservedReceiptWork &operator=(const ObservedReceiptWork &)=delete;
};
bool peer_handoff_paused(OnlineMatchSession::Phase phase) {
  return phase==OnlineMatchSession::Phase::AwaitingWorld||phase==OnlineMatchSession::Phase::Migrating||
      phase==OnlineMatchSession::Phase::Rebinding;
}
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
  std::shared_ptr<const std::array<RenderModel,6>> world_lods;
  std::uint32_t definition=null_datum;
  std::uint32_t render_model=null_datum;
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
                     const BipedDefinition &d,float horizontal_fov,Vec2 screen_offset={},float aim_scale=1.f) {
  Camera c;
  c.position = ordinary_unit_eye(p.position, d.camera_height,
                                 d.crouch_camera_height, p.crouch);
  c.yaw = p.yaw;
  c.pitch = p.pitch;
  c.field_of_view = source_camera_angles(horizontal_fov).projection_angle * 57.295779513f;
  const auto f=direction(c.yaw,c.pitch);
  const auto aim=source_observer_aim(f,cross(f,{-std::sin(c.yaw),std::cos(c.yaw),0}),horizontal_fov,screen_offset,
      1.3333333730697632f,aim_scale);
  c.oriented=true;c.forward=aim.forward;c.up=aim.up;
  if(p.source_player_camera) {
    //245060 ->CD4A0 ->E18D0 supplies the actual ordinary eye/head correction.
    // Keep the explicit native FOV/aim presentation while sharing this source
    // eye between the main view and first-person effect frames.
    c.position=p.source_player_camera->position;
  }
  return c;
}
} // namespace
static int run_multiplayer_impl(const MultiplayerOptions &request,
                               MultiplayerRuntimeOutcome *outcome,
                               std::shared_ptr<const SourceAuthorityRetainedWorld> *recovery=nullptr) {
  auto options=request;
  std::optional<std::uint64_t> advertised_fingerprint;
  if((options.session_world_update || options.session_world_loaded || options.session_view_black ||
      options.session_audio_update) &&
      (!options.join.empty() || options.online_match))
    throw FormatError("Local session world owner cannot control a remote or ONLINE match");
  if ((!options.input_script.empty() || options.capture_tick) &&
      ((options.input_script.empty() && !options.input_observer) || !options.capture_tick || options.frames ||
       options.capture_tick > 1000000 || options.screenshot.empty() ||
       options.host_port || !options.join.empty() || options.dedicated || options.replay))
    throw FormatError("Input-script capture requires a local finite tick capture");
  if(options.reference_live_shadow_gpu_tick &&
      (options.reference_live_shadow_gpu_tick!=61 || options.capture_tick!=61 || options.frames || options.ticks ||
       !options.input_observer || options.screenshot.empty() || options.width!=640 || options.height!=480 ||
       options.local_players!=1 || !options.source_object_world || !options.reference_tick_cadence ||
       options.capture_root || !options.reference_player.empty() || !options.bsp_reference_screenshot.empty() ||
       options.host_port || !options.join.empty() || options.dedicated || options.replay || options.online_match))
    throw FormatError("Live shadow GPU reference requires an explicit local tick61 single-view640x480 source capture");
  const bool finite_observer=options.input_observer && (options.frames || options.capture_tick) && !options.ticks &&
      !options.host_port && options.join.empty() && !options.dedicated && !options.replay && !options.online_match;
  if(!options.capture_shader_directory.empty() &&
      (!options.capture_tick || options.frames || options.ticks || !options.input_observer ||
       options.screenshot.empty() || options.host_port || !options.join.empty() ||
       options.dedicated || options.replay || options.online_match ||
       !std::filesystem::is_directory(options.capture_shader_directory)))
    throw FormatError("Pinned shader assets require an isolated finite local screenshot capture");
  if(options.effect_light_capture_request &&
      (!options.capture_tick || options.frames || options.ticks || !options.input_observer ||
       options.screenshot.empty() || options.host_port || !options.join.empty() ||
       options.dedicated || options.replay || options.online_match))
    throw FormatError("Source light capture requests require an isolated finite local screenshot run");
  if((options.reference_auxiliary_seed || options.reference_tick_cadence) &&
      !(options.reference_tick_cadence && !options.reference_auxiliary_seed && finite_observer) &&
      (options.input_script.empty() || !options.capture_tick || options.frames ||
       options.screenshot.empty() || options.host_port || !options.join.empty() ||
       options.dedicated || options.replay || options.online_match))
    throw FormatError("Explicit capture RNG/cadence requires an isolated finite local input-script capture");
  if((options.capture_camera || options.capture_root) && ((!options.frames && !options.capture_tick) || options.screenshot.empty() || options.local_players!=1))
    throw FormatError("Source view replay requires a finite one-player screenshot capture");
  if(options.capture_third_person && !options.capture_camera)
    throw FormatError("Third-person pose capture requires a finite capture camera");
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
    throw FormatError("Choose --host PORT or --join ADDRESS:PORT#HOST_KEY");
  if (options.capture_players &&
      (!options.frames || options.join.empty() ||
       options.capture_players > multiplayer_capacity ||
       options.capture_players < options.local_players))
    throw FormatError("Participant-gated capture requires a finite LAN client run");
  if(options.system_link_session && !options.system_link_session->host() && options.join.empty())
    options.join=options.system_link_session->join_address();
  if(!options.join.empty() && !(options.system_link_session && options.system_link_session->state())) {
    const auto description=query_lan_session(options.join);
    options.variant=description.variant;
    // The host supplied its effective override as an ordered variant write.
    // Local saved starting-weapon and vehicle preferences cannot replace it.
    options.starting_weapon=0;advertised_fingerprint=description.fingerprint;
    if(options.prepared_world && !options.prepared_world->matches(options))options.prepared_world.reset();
    const auto settings=source_game_variant(options.variant);
    std::cout<<"Native host configuration received: writes="<<options.variant.values.size()
        <<" primary_light="<<source_game_variant_get(settings,76)
        <<" secondary_light="<<source_game_variant_get(settings,77)
        <<" primary_heavy="<<source_game_variant_get(settings,78)
        <<" starting_weapon="<<source_game_variant_get(settings,89)<<std::endl;
  }
  if(!options.system_link_session && !options.online_match && !options.frames && !options.ticks && !options.dedicated &&
      (options.host_port || !options.join.empty())) {
    const auto engine=source_game_variant_type(source_game_variant(options.variant));
    if(engine==1 || engine==9)options.system_link_session=std::make_shared<SystemLinkSession>(bool(options.host_port));
  }
  // An actual retained frontend device presents the original screen231
  // "PLEASE WAIT" dialog while this runtime admits its new MCC map/source
  // world ("Loading map..."), then "Setting up game..." at renderer handoff.
  // The universal percentage wait remains only for a cache without that
  // screen. Direct or headless callers without a presentation owner do not
  // invent one.
  if(options.online_match && options.desktop && !options.desktop->loading_active()) {
    if(options.desktop->network_transition_configured())
      options.desktop->begin_network_transition(FrontendNetworkTransitionStatus::LoadingMap);
    else if(options.desktop->source_loading_wait_configured())
      options.desktop->begin_source_loading_wait();
  }
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
  const auto fingerprint = options.host_port || !options.join.empty() || options.online_match || options.system_link_session
      ? custom_slayer_fingerprint(multiplayer_fingerprint(options.map, cache.shared_directory(),
                                std::uint32_t(options.bsp), world->network_starting_weapon_identity(), options.gameplay_xbe),variant)
      : 0;
  if(advertised_fingerprint && *advertised_fingerprint!=fingerprint)
    throw FormatError("Host map, shared game data or executable differs after configuration adoption");
  std::unique_ptr<LanHost> host;
  std::unique_ptr<LanClient> client;
  MultiplayerRuntimeReceiptValues receipt_values;
  auto &ids=receipt_values.local_slots;
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
      host=std::make_unique<LanHost>(match,options.online_match,fingerprint,online.roster(),variant);
    } else client=std::make_unique<LanClient>(options.online_match,online.host(),fingerprint,options.selected_teams,options.selected_profiles);
  } else if(options.system_link_session) {
    if(options.system_link_session->host()) {
      ids=world->local_slots;
      host=options.system_link_session->enter_host(match,options.host_port,fingerprint,variant,options.map.stem().string());
    } else client=options.system_link_session->enter_client(options.join,fingerprint,options.local_players,options.selected_teams,options.selected_profiles);
  } else if (!options.join.empty())
    client = std::make_unique<LanClient>(options.join, fingerprint,
                                         options.local_players,options.selected_teams,options.selected_profiles);
  else {
    ids=world->local_slots;
    if (options.host_port)
      host = std::make_unique<LanHost>(match, options.host_port, fingerprint,variant);
  }
  std::cout << "Native Slayer: " << options.map.filename().string()
            << " local_players=" << options.local_players
            << " primary=" << (rules.weapon==null_datum?"NONE":content.weapons.at(rules.weapon).name) << '\n';
  if (!options.gameplay_xbe.empty())
    std::cout << "Original multiplayer balance: " << cache.original_data()->source.string()
              << " stores=" << cache.multiplayer_balance_writes()
              << " netgame_stores=" << cache.multiplayer_netgame_writes() << '\n';
  if (host)
    std::cout << "LAN host UDP port=" << host->port()
              << " capacity=" << rules.maximum_players << std::endl;
  if(host && !options.online_match) {
    const auto token=host->join_address("HOST_ADDRESS");
    std::cout << "LAN join token: " << token << "\nShare the full host key through an independent trusted channel; replace HOST_ADDRESS with this host's IPv4 address.\n" << std::flush;
  }
  if (client)
    std::cout << "Connecting to LAN host " << options.join << std::endl;
  std::unique_ptr<SourceAuthorityReplayWorker> source_receiver;
  if(MultiplayerRuntimeAuthority::recovery_preparation_enabled(options.online_match)&&client&&cache.original_data()&&
      cache.original_data()->image==OriginalData::Image::TitleUpdate15&&rules.custom==CustomSlayer::none&&!rules.defusal)
    source_receiver=std::make_unique<SourceAuthorityReplayWorker>(options);
  MultiplayerRuntimeAuthority authority(options,world,std::move(host),std::move(client),std::move(source_receiver));
  const auto live_match=[&]() -> MultiplayerMatch & {return authority.match();};
  const auto live_world=[&]{return &authority.world();};
  const auto live_host=[&]{return authority.host();};
  const auto live_client=[&]{return authority.client();};
  std::array<Controls, multiplayer_capacity> controls{};
  for (auto id : ids)
    controls[id].yaw = live_match().unit(id).yaw;
  if(live_host() && !options.online_match && options.session_lan_host_ready)options.session_lan_host_ready(live_host()->join_address());
  if(options.session_world_loaded)options.session_world_loaded(live_match(),ids);
  std::unique_ptr<NativeVistaAnnouncer> objective_announcer;
  const auto consume_objective_audio=[&](const MultiplayerState& state) {
    // The native source journal is shared by every original engine. An
    // offline frontend callback still owns its separate legacy Slayer/King
    // queue; retain that explicit owner without duplicating its deliveries.
    if(!cache.is_mcc() || ids.empty() ||
        (options.session_audio_update && state.source_engine!=1 && state.source_engine!=9))return;
    if(!objective_announcer) {
      std::uint32_t crt=1;
      const auto seed=source_secondary_random_seed(std::uint32_t(std::time(nullptr)),SDL_GetTicks(),source_crt_random(crt));
      NativeVistaAnnouncerConfiguration configuration{cache.shared_directory(),cache.scenario(),
          cache.original_data(),0,30,{0x27f,0x1f80},options.mute,seed};
      configuration.source_tu15=true;configuration.audio_observer=options.audio_observer;
      configuration.selected_cache_path=cache.path();
      objective_announcer=NativeVistaAnnouncer::open(std::move(configuration));
    }
    auto& tick=receipt_values.objective_audio_tick;auto& serial=receipt_values.objective_audio_serial;
    const bool initial=!tick;
    if(initial)tick=live_client()?state.tick:0u;
    const auto advance=[&](std::uint32_t target) {
      if(std::bit_cast<std::int32_t>(target-*tick)<0)return;
      while(*tick!=target){objective_announcer->pre_world_tick();++*tick;}
    };
    for(const auto& event:state.source_engine_events)if(std::bit_cast<std::int32_t>(event.serial-serial)>0) {
      serial=event.serial;
      // A new peer starts at its actual snapshot. Preserve its own targeted
      // admission publication without replaying earlier broadcast history.
      if(initial && live_client() && event.tick!=state.tick) {
        const auto target=event.words[2];
        if(target==null_datum || std::ranges::none_of(ids,[&](auto slot){return state.players[slot].source_player==target;}))continue;
      }
      advance(event.tick);
      const auto selections=objective_announcer->publish_source_event(event,ids);
      if(options.objective_audio_observer) {
        std::vector<std::uint32_t> sounds;for(const auto& choice:selections)if(choice.sound!=null_datum)sounds.push_back(choice.sound);
        options.objective_audio_observer(event,sounds);
      }
    }
    advance(state.tick);
  };
  if(!live_client())consume_objective_audio(live_match().state());
  bool online_gameplay_started{};
  SourceGameplayFadePresentationTu15 gameplay_fade;
  std::optional<MultiplayerSnapshotOrigin> terminal_display_origin;
  std::optional<SourceGameplayFadePresentationTu15> terminal_display_fade;
  const auto retain_recovery=[&](MultiplayerExitReason reason,const MultiplayerSnapshotOrigin *origin){
    if(options.online_match&&options.online_match->terminal_recovery_enabled()&&!online_gameplay_started)return;
    if(!recovery||reason!=MultiplayerExitReason::online_disconnected||!authority.has_receiver())return;
    try {
      std::shared_ptr<const SourceAuthorityRetainedWorld> retained;
      for(;;){
        if(authority.recovery_eligibility()==MultiplayerRuntimeAuthority::RecoveryEligibility::Unavailable){
          authority.cancel_recovery();return;
        }
        if((retained=authority.recover()))break;
        // Native replay remains on its own executor. Keep OS events moving
        // during this completed-world barrier; a window quit cancels export.
        if(SDL_WasInit(SDL_INIT_VIDEO)){
          SDL_PumpEvents();
          if(SDL_HasEvent(SDL_QUIT)){authority.cancel_recovery();return;}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if(options.online_match&&options.online_match->terminal_recovery_enabled()) {
        // A historical native cut can be internally valid while preceding the
        // actual adopted display state. Its old score/round/clock may not be
        // shown as the current ending. Require the exact opaque body/sequence
        // receipt, including same-tick snapshots, before exporting this cut.
        if(!origin||!retained->host||!live_client())
          throw FormatError("Terminal source recovery lacks its actual adopted snapshot receipt");
        const auto proof=live_client()->prove_snapshot(*retained->host,retained->cut,*origin);
        if(!proof.matches(*origin))throw FormatError("Terminal source recovery differs from the adopted snapshot");
      }
      *recovery=std::move(retained);
      if(outcome)outcome->local_gameplay_fade=terminal_display_fade.value_or(gameplay_fade);
    }catch(const std::exception &error){
      std::cerr<<"Source recovery unavailable: "<<error.what()<<'\n';
    }
  };
  const auto retain_outcome = [&](MultiplayerExitReason reason,
                                  const MultiplayerState &provided_state,
                                  bool client_snapshot_received = true,
                                  const MultiplayerSnapshotOrigin *origin = nullptr) {
    const bool terminal_display=reason==MultiplayerExitReason::online_disconnected&&terminal_display_origin.has_value();
    const auto &state=terminal_display?terminal_display_origin->accepted_state():provided_state;
    if(terminal_display)origin=&*terminal_display_origin;
    if(std::getenv("H2_EOS_DIAGNOSTICS")){
      std::cerr<<std::dec<<"ONLINE runtime exit reason="<<unsigned(reason)<<" tick="<<state.tick
          <<" phase="<<(options.online_match?unsigned(options.online_match->phase()):255u);
      if(const auto *client=live_client()){
        const auto stats=client->snapshot_diagnostics();
        std::cerr<<" client_error="<<unsigned(stats.error)<<" accepted_sequence="
            <<(stats.accepted_sequence?std::int64_t(*stats.accepted_sequence):-1)
            <<" accepted_tick="<<(stats.accepted_tick?std::int64_t(*stats.accepted_tick):-1)
            <<" accepted_age_ms="<<(stats.accepted_age_ms?std::int64_t(*stats.accepted_age_ms):-1)
            <<" rejected_packets="<<stats.rejected_packets<<" buffered_bytes="<<stats.buffered_bytes;
      }
      if(const auto *host=live_host()){
        const auto stats=host->snapshot_diagnostics();
        std::cerr<<" host_snapshot_bytes="<<stats.body_bytes<<" host_snapshot_fragments="<<stats.fragments
            <<" host_snapshot_peers="<<stats.peers;
        if(const auto removal=host->last_peer_removal())std::cerr<<" host_removal_reason="<<unsigned(removal->reason)
            <<" host_input_acceptances="<<removal->input_acceptances<<" host_idle_age_ns="<<removal->idle_age_ns;
      }
      std::cerr<<std::endl;
    }
    const auto handoff_system_link=[&]{
      if(reason!=MultiplayerExitReason::system_link_postgame && reason!=MultiplayerExitReason::system_link_lobby)return;
      if(!options.system_link_session)throw FormatError("System Link return lost its real session owner");
      auto transports=authority.release_system_link();
      options.system_link_session->retain(std::move(transports.first),std::move(transports.second));
      if(outcome)outcome->system_link_session=options.system_link_session;
    };
    if (!outcome) {handoff_system_link();return;}
    if (live_client()) {
      *outcome = retain_multiplayer_outcome(reason,
          MultiplayerOutcomeAuthority::client_snapshot,
          client_snapshot_received ? &state : nullptr);
      retain_recovery(reason,origin);handoff_system_link();
      return;
    }
    const auto recording = live_match().result_recording();
    *outcome = retain_multiplayer_outcome(reason,
        live_host() ? MultiplayerOutcomeAuthority::host : MultiplayerOutcomeAuthority::local,
        &state, MultiplayerOutcomeBanksView{
            &live_match().source_match_statistics(), live_match().recorded_statistics(),
            &recording,live_match().recorded_events()});
    handoff_system_link();
  };
  // The peer can leave between the frontend handoff and this world owner.
  // Retain only the actual initialized host state, or no client snapshot.
  if(options.online_match && options.online_match->phase()==OnlineMatchSession::Phase::Failed) {
    retain_outcome(MultiplayerExitReason::online_disconnected,live_match().state(),false);return 0;
  }
  if (options.dedicated || options.ticks) {
    if (live_client())
      throw FormatError("Headless client mode is unavailable");
    const auto limit = options.ticks ? options.ticks : options.frames;
    auto next = Clock::now();
    bool hosted_session_started=!ids.empty();
    auto exit_reason=MultiplayerExitReason::headless_limit;
    for (unsigned t = 0; !limit || t < limit;) {
      if(options.online_match){const auto phase=options.online_match->phase();
        if(phase==OnlineMatchSession::Phase::Failed){exit_reason=MultiplayerExitReason::online_disconnected;break;}
        if(peer_handoff_paused(phase)){
          if(phase==OnlineMatchSession::Phase::Migrating)authority.prepare_takeover();
          else if(phase==OnlineMatchSession::Phase::Rebinding)authority.prepare_client_rebind();
          next=Clock::now();std::this_thread::sleep_for(std::chrono::milliseconds(2));continue;
        }
      }
      authority.service();
      auto source_frame=authority.begin_frame();
      if(!source_frame)continue;
      if (live_host())
        live_host()->receive(controls);
      authority.admit_players();
      if(options.system_link_session && options.system_link_session->state() &&
          options.system_link_session->state()->phase!=SystemLinkPhase::running) {
        if(options.system_link_session->world_update(live_match().state())) {exit_reason=MultiplayerExitReason::system_link_postgame;break;}
        if(live_host())live_host()->broadcast();
        next=Clock::now();std::this_thread::sleep_for(std::chrono::milliseconds(2));continue;
      }
      // A native dedicated host has no active session before its first
      // admitted participant. Calling the original active-engine end query
      // on that empty roster would finish it before the first UDP hello.
      // Once started, preserve genuine empty-roster/departure ending rules.
      hosted_session_started=hosted_session_started || (live_host() && live_host()->peers());
      if(!live_host() || hosted_session_started) {
        const auto before=live_match().simulation().ticks();
        live_match().step(controls);
        // A headless outer iteration admits one source tick. Keep its frame
        // effect owners inside the same authority recording scope, without a GPU.
        const auto accepted=live_match().simulation().ticks()==before+1u?tick_seconds:0.f;
        if(live_world()->source_objects && live_world()->source_objects->owns_physics())
          live_world()->source_objects->advance_effect_frame(accepted);
      }
      consume_objective_audio(live_match().state());
      if(objective_announcer)objective_announcer->audio_update(1.f,live_match().simulation().sound_classes().gains());
      if(options.system_link_session && options.system_link_session->world_update(live_match().state())) {
        exit_reason=MultiplayerExitReason::system_link_postgame;break;
      }
      if(options.session_world_update && options.session_world_update(live_match())) {
        exit_reason=MultiplayerExitReason::session_world_close;
        break;
      }
      if (live_host())
        authority.broadcast_candidate_frame(source_frame);
      if(!source_frame)continue;
      if(source_frame)source_frame->finish();
      if (live_host()) {
        next += std::chrono::microseconds(33333);
        std::this_thread::sleep_until(next);
      }
      ++t;
    }
    const auto s = live_match().state();
    std::cout << "Multiplayer ticks=" << s.tick << " players="
              << std::count_if(s.players.begin(), s.players.end(),
                               [](const auto &p) { return p.connected; })
              << " finished=" << s.finished << '\n';
    retain_outcome(exit_reason, s);
    return 0;
  }
  Platform platform(options.desktop, options.width, options.height, options.hidden);
  // This scope ends before Platform can release the retained desktop/window.
  // ONLINE failures and exceptions must restore the menu pointer too.
  GameplayMouseCapture mouse_capture(platform.window);
  std::shared_ptr<GameVoice> game_voice;
  if(options.online_match) {
    game_voice=options.online_match->retained_voice();
    if(!game_voice){game_voice=std::make_shared<GameVoice>();game_voice->enable(options.match_voice);options.online_match->retain_voice(game_voice);}
  }
  auto mesh = world->map_assets?world->map_assets->take_bsp_mesh(options):cache.load_bsp(options.bsp);
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
      if(source_weather_sprite_resource_profile(sprite.width,sprite.height,unsigned(sprite.mips.size()),
          sprite.format,sprite.layers,sprite.depth))
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
    water_material_bases.push_back(unsigned(mesh.materials.size()));
    for(auto material:definition.geometry.materials) {
      material.water_definition=int(d);mesh.materials.push_back(std::move(material));
    }
  }
  auto visual = [&](std::uint32_t datum, const ObjectColors *colors=nullptr, const ScenarioObject *object=nullptr,
                    bool dynamic_lighting=false, std::optional<float> camouflage=std::nullopt,
                    bool prepare_world_lods=false) {
    Visual v;
    v.definition=datum;
    v.model = cache.load_model(datum,true,object?object->variant:0);
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
    if(const auto* reference=held_pose_shading_reference(datum);reference && reference->colors)
      colors=&*reference->colors;
#endif
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
      v.render_model=mode;
      if(prepare_world_lods) {
        const auto initial=cache.load_initial_model_definition(datum);
        auto banks=std::make_shared<std::array<RenderModel,6>>();
        auto bindings=v.model;bindings.mesh.vertices.clear();bindings.vertex_origins.clear();
        for(unsigned lod=0;lod<6;++lod) {
          std::vector<unsigned> sections;
          for(const auto& region:initial.render_sections)for(const auto& permutation:region) {
            const auto section=permutation[lod];
            if(section>=0 && std::find(sections.begin(),sections.end(),unsigned(section))==sections.end())
              sections.push_back(unsigned(section));
          }
          auto& model=(*banks)[lod];
          model=cache.load_model_sections(datum,false,object?object->variant:0,lod,sections);
          if(model.bones.size()!=v.model.bones.size() || model.checksum!=v.model.checksum)
            throw FormatError("Third-person resident LOD changed the original model hierarchy");
          bindings.mesh.vertices.insert(bindings.mesh.vertices.end(),model.mesh.vertices.begin(),model.mesh.vertices.end());
          bindings.vertex_origins.insert(bindings.vertex_origins.end(),model.vertex_origins.begin(),model.vertex_origins.end());
          model.mesh.materials.clear();
        }
        v.shaders.emplace(cache,bindings,mode,2,camouflage);
        v.model.mesh.materials=std::move(bindings.mesh.materials);
        v.world_lods=std::move(banks);
      } else v.shaders.emplace(cache,v.model,mode,2,camouflage);
    }
    if(dynamic_lighting && camouflage)for(auto &material:v.model.mesh.materials)
      prepare_camouflage_materials(cache,material);
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
    // Explicit storage control for retained render-input experiments. The
    // original GL capture's decoded cube bytes are compared independently;
    // ordinary gameplay never selects this diagnostic transformation.
    if(held_pose_shading_reference(datum) && std::getenv("HALO2_HELD_POSE_EXPAND_BC")) {
      std::function<void(Material&)> expand=[&](Material& material) {
        const auto texture=[](Texture& image) {
          if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
          held_pose_shading_reference_texture(image);
        };
        texture(material.diffuse);texture(material.detail);texture(material.bump);texture(material.auxiliary);
        for(auto& stage:material.animated_textures)for(auto& frame:stage)texture(frame);
        const auto child=[&](std::shared_ptr<Material>& value) {
          if(value){value=std::make_shared<Material>(*value);expand(*value);}
        };
        child(material.environment_pass);
        for(auto& value:material.world_overlays_before)child(value);
        for(auto& value:material.world_overlays_after)child(value);
      };
      for(auto& material:v.model.mesh.materials)expand(material);
    }
#endif
    v.material = unsigned(mesh.materials.size());
    mesh.materials.insert(mesh.materials.end(), v.model.mesh.materials.begin(),
                          v.model.mesh.materials.end());
    return v;
  };
  struct VehicleVisual {
    SourceInitialModelDefinition initial;
    bool requires_source_regions{};
    std::array<RenderModel,6> lods;
    std::array<std::vector<unsigned>,6> initial_sections;
    unsigned material{};
    std::unique_ptr<SourceCharacterShaders> shaders;
    std::unique_ptr<SourceCharacterShaderInputs> prt_inputs;
    std::unique_ptr<SourceObjectGeometry> prt;
    const SourceCharacterShaderInputs &owner()const{return prt?*prt_inputs:*shaders;}
    SourceGeometryLodDefinition geometry;
    SourceObjectColorDefinition colors;
    std::uint32_t variant_name{};
  };
  std::map<std::pair<std::uint32_t,std::uint32_t>,VehicleVisual> vehicle_visuals;
  std::map<std::uint32_t,ObjectLightQueryContext> vehicle_light_definitions;
  std::optional<SourcePrtLighting> vehicle_prt_lighting;
  std::set<std::uint32_t> vehicle_render_models;
  std::map<std::uint32_t,SourcePrtDefinition> vehicle_prt_definitions;
  for(const auto &o:content.objects)if(o.type==1 && o.definition!=null_datum) {
    const auto key=std::pair{o.definition,o.variant};if(vehicle_visuals.contains(key))continue;
    VehicleVisual vehicle;
    const auto initial=cache.load_initial_model_definition(o.definition);
    vehicle_render_models.insert(initial.render_model);
    const auto selected=cache.load_initial_model_selection(o.definition,o.variant);
    vehicle.initial=initial;vehicle.requires_source_regions=!selected.resolved();
    vehicle.variant_name=selected.variant_index<0?0u:initial.variants.at(unsigned(selected.variant_index)).name;
    vehicle.colors=cache.load_source_object_color_definition(o.definition);
    vehicle.geometry=cache.load_source_geometry_lod_definition(o.definition);
    if(const auto prt=cache.load_source_prt_definition(initial.render_model)) {
      vehicle_prt_definitions.emplace(initial.render_model,*prt);
      if(!vehicle_prt_lighting)vehicle_prt_lighting=cache.load_source_prt_lighting(options.bsp);
      vehicle.prt_inputs=std::make_unique<SourceCharacterShaderInputs>(cache,o.definition);
      vehicle.prt=std::make_unique<SourceObjectGeometry>();
      if(selected.resolved())vehicle.prt->load(cache,o,initial,selected,initial.render_model,true);
      else vehicle.prt->load_union(cache,o,initial,initial.render_model);
      const auto basis=authored_basis(o.rotation);
      const auto transform=source_object_transform(o.position,basis.forward,basis.up,o.scale,false);
      const auto nodes=pose_model_nodes_world(vehicle.prt->model(),transform);
      vehicle.prt->pose_world([&](const RenderModel &model){return skin_model_world_nodes(model,nodes);});
      vehicle.prt->prepare(cache,cache.load_object_colors(o.definition,o.variant),nodes,*vehicle_prt_lighting,mesh,0.f);
      vehicle.material=vehicle.prt->first_material;
      vehicle_light_definitions.emplace(o.definition,vehicle.owner().light_query_context());
      vehicle_visuals.emplace(key,std::move(vehicle));continue;
    }
    // Prepare every literal source geometry bank. Resident materials are shared;
    // the union of section origins binds compiled inputs for every bank once.
    RenderModel bindings;
    for(unsigned lod=0;lod<vehicle.lods.size();++lod) {
      auto& model=vehicle.lods[lod];
      if(selected.resolved())vehicle.initial_sections[lod]=source_initial_model_sections(initial,selected,lod);
      std::vector<unsigned> resident;
      for(const auto &region:initial.render_sections)for(const auto &permutation:region) {
        const auto section=permutation[lod];
        if(section>=0 && std::find(resident.begin(),resident.end(),unsigned(section))==resident.end())resident.push_back(unsigned(section));
      }
      model=cache.load_model_sections(o.definition,lod==5,o.variant,lod,resident);
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
  if(reference_player && (reference_player->unit_definition!=content.player.datum ||
                         reference_player->weapon_definition!=rules.weapon))
    throw FormatError("Original player fixture has different unit or weapon definitions");
  struct WeaponWorldVisual {
    Visual gun;
    std::array<unsigned,multiplayer_capacity*2> materials;
  };
  std::map<std::uint32_t,WeaponWorldVisual> body_guns;
  std::map<std::uint32_t,SourceWorldWeaponDefinition> weapon_shader_definitions;
  const auto weapon_material_names=SourceWeaponMaterialNames::from_cache(cache);
  for(const auto &[datum,definition]:content.weapons)
    weapon_shader_definitions.emplace(datum,cache.load_source_world_weapon_definition(datum));
  unsigned object_slots=live_host() || live_client() ? multiplayer_capacity:
      (ids.empty()?0u:*std::max_element(ids.begin(),ids.end())+1u);
  if(!live_host() && !live_client()) {
    // A prepared offline world can retain nonlocal participants too. Their
    // ordinary and override materials must have real owner slots, rather
    // than falling through to the default BSP material offset.
    const auto roster=live_match().state();
    for(unsigned player=0;player<roster.players.size();++player)
      if(roster.players[player].connected)object_slots=std::max(object_slots,player+1);
  }
  const auto player_emblem_palette=cache.load_full_scoreboard_assets().palette;
  const auto player_emblem_scenario=cache.load_source_geometry_scenario_type();
  for(const auto &[datum,definition]:content.weapons)if(!definition.first_person_models.empty() && definition.first_person_models.front().model!=null_datum && definition.first_person_models.front().animation!=null_datum) {
    auto &world=body_guns[datum];world.gun=visual(datum,nullptr,nullptr,true,0.f,true);
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
    Visual first_person_body;
    std::uint32_t first_person_body_model=null_datum;
    std::array<unsigned,multiplayer_capacity> first_person_body_materials{};
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
  const int source_game_type=options.campaign_difficulty?1:2;
  const auto representations=source_session_player_representations(content,source_game_type);
  std::map<std::pair<unsigned,std::uint32_t>,SpeciesVisual> species;
  std::vector<FirstPersonBindingDefinition> local_definitions;
  const auto prepare_player_body=[&](unsigned representation,std::uint32_t weapon_datum,SpeciesVisual &v) {
    const auto &authored=content.player_representations[representation];
    {
      //168EB0..168EBB reads the separate representation+0C body model.
      const auto globals=cache.query_tag({.group=tag_groups::globals,.require_address=true});
      if(!globals)throw FormatError("First-person body requires its actual globals definition");
      const auto metadata=cache.load_source_tag_metadata(globals->datum);
      const Bytes root(metadata.bytes_at(metadata.source_address(),0x140));
      const auto rows=read_tag_block_descriptor(root,0x138);
      const auto extent=rows.extent(0xbc,256);
      if(representation>=unsigned(rows.count))throw FormatError("First-person body representation is absent");
      const auto owner=cache.query_tag({.contains=TagAddressRange{rows.address,extent},.require_address=true});
      if(!owner)throw FormatError("First-person body representation bank has no retained owner");
      (void)cache.tag_payload(owner->handle());
      v.first_person_body_model=Bytes(cache.read_tag_bytes(rows.address,extent)).u32(representation*0xbc+0xc);
    }
    const auto colors=reference_player?reference_player->colors:cache.load_object_colors(authored.biped.datum);
    const auto common=std::find_if(species.begin(),species.end(),[&](const auto &entry) {
      return entry.first.first==representation && entry.first.second!=weapon_datum;
    });
    if(common!=species.end()) {
      v.player=common->second.player;v.player_materials=common->second.player_materials;
      v.hands=common->second.hands;
      v.first_person_body=common->second.first_person_body;
      v.first_person_body_materials=common->second.first_person_body_materials;
    } else {
      v.player=visual(authored.biped.datum,&colors,nullptr,true,0.f,true);
      v.hands=visual(authored.hands,&colors,nullptr,true,0.f);
      if(v.first_person_body_model!=null_datum) {
        v.first_person_body=visual(v.first_person_body_model,&colors,nullptr,true,0.f);
        v.first_person_body_materials[0]=v.first_person_body.material;
        for(unsigned i=1;i<object_slots;++i) {
          v.first_person_body_materials[i]=unsigned(mesh.materials.size());
          mesh.materials.insert(mesh.materials.end(),v.first_person_body.model.mesh.materials.begin(),v.first_person_body.model.mesh.materials.end());
        }
      }
      v.player_materials[0]=v.player.material;
      for(unsigned i=1;i<object_slots;++i) {
        v.player_materials[i]=unsigned(mesh.materials.size());
        mesh.materials.insert(mesh.materials.end(),v.player.model.mesh.materials.begin(),v.player.model.mesh.materials.end());
      }
    }
    v.player_bounds=cache.load_object_shadow_definition(authored.biped.datum);
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
  };
  // NONE has its own unit/body owner, without a weapon, hand graph or guessed idle.
  for(unsigned representation:representations) {
    if(representation>=content.player_representations.size() ||
        content.player_representations[representation].biped.datum==null_datum)continue;
    auto &body=species[{representation,null_datum}];
    prepare_player_body(representation,null_datum,body);
  }
  for(const auto &[weapon_datum,weapon]:content.weapons) {
  if(weapon.first_person_models.empty())continue;
  const auto weapon_animation=cache.load_weapon_animation_identifiers(weapon.datum);
  for(unsigned representation:representations) {
    if(representation>=content.player_representations.size() ||
        content.player_representations[representation].biped.datum==null_datum)continue;
    const auto preferred_record=representation==1 || representation==3 ? 1u:0u;
    const auto &selected_fp=weapon.first_person_models.at(weapon.first_person_models.size()>preferred_record?preferred_record:0u);
    if(selected_fp.model==null_datum || selected_fp.animation==null_datum)continue;
    const auto &authored=content.player_representations[representation];
    auto &v=species[{representation,weapon_datum}];
    prepare_player_body(representation,weapon_datum,v);
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
    const auto record=(representation==1 || representation==3) && weapon.first_person_models.size()>1 ? 1u:0u;
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
      const auto identifier=first_person_clip_identifier(clock);
      auto clip=cache.load_animation_identifier(definition.graph,identifier);
      v.fp_clips.emplace(identifier,clip);
      switch(clock.activity) {
      case 0x05000024:v.fp_ready=clip;break;
      case 0x06000006:v.fp_fire=clip;break;
      case 0x0b000065:v.fp_reload=clip;break;
      case 0x0c000064:v.fp_reload_empty=clip;break;
      case 0x0d000021:v.fp_throw=clip;break;
      }
    }

  }
  }
  const auto make_shields=[&](const RenderModel &model,std::uint32_t mode,
      const VitalityDefinition &vitality,bool first_person) {
    std::map<std::uint32_t,SpeciesVisual::ShieldVisual> result;
    const std::array shaders{
        first_person?vitality.shield_damaged_first_person_shader:vitality.shield_damaged_shader,
        first_person?vitality.overshield_first_person_shader:vitality.overshield_shader};
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
      auto shield_model=body.player.model;
      if(body.player.world_lods) {
        // The source collector can select any authored world LOD. Prepare
        // override part/input banks from the same complete geometry catalog
        // used by ordinary materials; keep submitted geometry view-owned.
        shield_model.mesh.vertices.clear();shield_model.vertex_origins.clear();
        for(const auto& model:*body.player.world_lods) {
          shield_model.mesh.vertices.insert(shield_model.mesh.vertices.end(),model.mesh.vertices.begin(),model.mesh.vertices.end());
          shield_model.vertex_origins.insert(shield_model.vertex_origins.end(),model.vertex_origins.begin(),model.vertex_origins.end());
        }
      }
      body.body_shields=make_shields(shield_model,mode,representation.biped.vitality,false);
      body.hand_shields=make_shields(body.hands.model,representation.hands,representation.biped.vitality,true);
    }
  }
  const auto species_for=[&](const MultiplayerState::Player &p)->SpeciesVisual& {
    return species.at({source_session_player_representation(p.source_configuration,source_game_type),p.weapon});
  };
  const auto biped_for=[&](const MultiplayerState::Player &p)->const BipedDefinition& {
    return multiplayer_biped(content,source_session_player_representation(p.source_configuration,source_game_type));
  };
  if(!live_client())live_match().configure_local_views(ids,local_definitions);
  MultiplayerPoseContinuity pose_continuity;
  auto &pose_values=pose_continuity.state();
  auto &client_fp_views=pose_values.client_fp_views;
  auto &client_fp_origins=pose_values.client_fp_origins;
  auto &client_fp_clock_available=pose_values.client_fp_clock_available;
  auto &client_secondary_clock_available=pose_values.client_secondary_clock_available;
  auto &client_fp_reported_units=pose_values.client_fp_reported_units;
  // Retain the existing native visual-event primary channel until the
  // original game-tick dispatch owner is connected. Borrowing gameplay's
  // deterministic word from this render-batched owner would make gameplay
  // RNG depend on frame batching. All proven auxiliary consumers share the
  // actual application word; this legacy primary channel is not Xbox history.
  std::uint32_t visual_event_primary=0x12345678u;
  const SourceEffectRandom effect_random{visual_event_primary,auxiliary_random};
  NativeMuzzleEffects muzzle_effects(cache,effect_random);
  muzzle_effects.collision_query([&](auto flags,Vec3 origin,Vec3 delta)->std::optional<ParticleCollisionHit> {
    if(live_world()->source_objects && live_world()->source_objects->owns_physics())
      return live_world()->source_objects->particle_collision(flags,origin,delta);
    const auto distance=length(delta);if(!(distance>0))return {};
    const auto hit=live_match().simulation().collision().raycast(origin,delta,distance);if(!hit)return {};
    return ParticleCollisionHit{hit->distance/distance,hit->position,hit->normal,true,std::int16_t(hit->material)};
  });
  NativeWorldEffects world_effects(cache,mesh,live_match().simulation().collision(),effect_random);
  SourceWorldParticleDrawsTu15 source_particle_draws(cache,content.objects);
  auto* source_particle_world=!live_client() && !reference_player && !options.capture_root &&
      live_world()->source_objects && live_world()->source_objects->owns_physics()?live_world()->source_objects.get():nullptr;
  // A gated migration candidate must admit authored source resources before
  // Renderer freezes material residency, even while it is still a client.
  // Resource loading does not authorize native simulation/render producers.
  const bool source_migration_resources=
      options.online_match && options.online_match->direct_udp() && options.online_match->peer_migration().has_value() &&
       !reference_player && !options.capture_root && rules.custom==CustomSlayer::none && !rules.defusal &&
       cache.original_data() && cache.original_data()->image==OriginalData::Image::TitleUpdate15;
  const bool source_particle_resources=source_particle_world || source_migration_resources;
  const bool native_lens_resources=(live_client()||source_migration_resources) &&
      !reference_player && !options.capture_root && cache.original_data() &&
      cache.original_data()->image==OriginalData::Image::TitleUpdate15;
  const bool source_lens_resources=source_particle_resources||native_lens_resources;
  std::unique_ptr<SourceLensDrawFrameTu15> source_lens_draws;
  if(source_lens_resources)source_lens_draws=std::make_unique<SourceLensDrawFrameTu15>(cache,mesh);
#if HALO2_SOURCE_DECALS_AVAILABLE
  SourceDecalScene cached_decals(cache);
#endif
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
  SourceContrailScene source_contrails(cache);
#endif
  if(live_world()->source_objects && live_world()->source_objects->owns_physics())
    world_effects.collision_query([&](auto flags,Vec3 origin,Vec3 delta) {
      return live_world()->source_objects->particle_collision(flags,origin,delta);
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
  auto &world_impact_serial=receipt_values.world_impact_serial;
  auto &world_melee_serial=receipt_values.world_melee_serial;
  auto &world_detonation_serial=receipt_values.world_detonation_serial;
  auto &world_unit_effect_serial=receipt_values.world_unit_effect_serial;
  auto &world_voice_serial=receipt_values.world_voice_serial;
  auto &world_effect_baseline=receipt_values.world_effect_baseline;
  world_effect_baseline=!live_client();
  std::set<std::string> reported_world_effect_gaps;
  NativeEffectSounds effect_sounds(cache);
  auto &weapon_effect_sounds=receipt_values.weapon_sounds;
  auto &client_effect_serial=receipt_values.client_effect_serial;
  auto &client_effect_baseline=receipt_values.client_effect_baseline;
  client_effect_baseline=!live_client();
  // Native effect slots are bounded; retain the full salt alongside the
  // source instance so retired/reused slots cannot inherit an old refresh.
  // Each authored effect keeps its own binding across alternating barrels.
  auto &source_firing_instances=receipt_values.firing_instances;
  std::set<std::uint32_t> world_firing_effects;
  std::map<std::uint32_t,std::map<std::string,std::vector<ObjectAttachmentMarker>>> firing_world_markers;
  auto &world_weapon_effect_serial=receipt_values.world_weapon_effect_serial;
  auto &client_prop_effect_baseline=receipt_values.client_prop_effect_baseline;
  auto &device_event_cursor=receipt_values.device_events;
  device_event_cursor=NativeDeviceEventCursor(!live_client());
  auto &heard_prop_effects=receipt_values.heard_prop_effects;
  std::uint32_t muzzle_effect=null_datum;
  const auto stock_smg=std::find_if(content.weapons.begin(),content.weapons.end(),
      [](const auto &entry){return entry.second.smg.has_value();});
  if(stock_smg!=content.weapons.end() && !stock_smg->second.firing_effects.empty()) {
    muzzle_effect=stock_smg->second.firing_effects[0].effects[0];
    for(auto &[key,v]:species)if(key.second!=null_datum && content.weapons.at(key.second).smg) {
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
  std::set<std::uint32_t> source_weapon_resource_definitions;
  const auto preload_source_weapon_effect=[&](std::uint32_t effect) {
    if(!source_particle_resources || effect==null_datum)return false;
    if(!source_weapon_resource_definitions.insert(effect).second)
      return source_particle_draws.world_effect(effect);
    // FP and WORLD admit different authored camera systems. An unavailable
    // system in one pass must not hide supported resources in the other.
    try{source_particle_draws.preload_first_person(effect);}
    catch(const FormatError &e){std::cerr<<"Source first-person weapon particles "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
    try{return source_particle_draws.preload_world(effect);}
    catch(const FormatError &e){std::cerr<<"Source world weapon particles "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
    return false;
  };
  for(const auto &[datum,d]:content.weapons) {
  std::set<std::uint32_t> barrel_effects;
  const auto add_bank=[&](const WeaponDefinition &barrel) {
    // The specialized stock SMG owner has an authored effect bank without
    // the generic barrel presenter. Resource admission follows that bank.
    (void)preload_source_weapon_effect(barrel.ready_effect);
    for(const auto &entry:barrel.firing_effects)
      barrel_effects.insert(entry.effects.begin(),entry.effects.end());
  };
  add_bank(d);for(const auto &barrel:d.barrel_views)add_bank(*barrel);
  for(const auto effect:barrel_effects) {
    if(effect==null_datum || !cache.query_tag({.handle=TagHandle{effect},.group=tag_groups::effect,.require_address=true}))continue;
    firing_sound_effects.insert(effect);
    // Keep the client's existing CPU particle owner until a complete prepared
    // adoption changes producers. Residency alone cannot suppress its effects.
    if(preload_source_weapon_effect(effect) && source_particle_world)
      world_effects.source_particle_effect(effect);
    try {
      preload_weapon_world_effect(datum,effect);
    }catch(const FormatError &e){std::cerr<<"World firing effect "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
    try {
    if(cache.load_muzzle_effect(effect).light_parts && !world_firing_effects.contains(effect))
      throw FormatError("Firing light parts have no completed live world owner");
    const auto names=muzzle_effects.marker_names(effect);
    for(auto &[key,v]:species)if(key.second==datum)for(auto name:names) {
      auto markers=source_model_markers(cache.load_source_tag_metadata(v.fp_definition->weapon_model),name);
      v.effect_markers.emplace(name,std::move(markers));
    }
    firing_effects.insert(effect);
    }catch(const FormatError &e){std::cerr<<"Firing effect "<<std::hex<<effect<<std::dec<<": "<<e.what()<<'\n';}
  }
  }
  std::set<std::uint32_t> ready_effects;
  for(const auto &[datum,d]:content.weapons)if(d.ready_effect!=null_datum) {
    (void)preload_source_weapon_effect(d.ready_effect);
    try {
    if(cache.load_muzzle_effect(d.ready_effect).light_parts)
      preload_weapon_world_effect(datum,d.ready_effect);
    const auto names=muzzle_effects.marker_names(d.ready_effect);
    for(auto &[key,v]:species)if(key.second==datum)for(auto name:names) {
      auto markers=source_model_markers(cache.load_source_tag_metadata(v.fp_definition->weapon_model),name);
      v.effect_markers.emplace(name,std::move(markers));
    }
    ready_effects.insert(d.ready_effect);
    }catch(const FormatError &e){std::cerr<<"Ready effect "<<std::hex<<d.ready_effect<<std::dec<<": "<<e.what()<<'\n';}
  }
  for(auto &[key,v]:species)if(!v.muzzle_markers.empty())
    v.effect_markers.emplace(muzzle_effects.marker_name(muzzle_effect),v.muzzle_markers);
  struct PresentationOwnerLifetime {
    MultiplayerMatch *borrowed_match{};
    ~PresentationOwnerLifetime(){if(borrowed_match)borrowed_match->release_presentation_owners();}
  } presentation_owner_lifetime{live_client()?nullptr:&live_match()};
  // The TU1.5 source scheduler already publishes actual weapon effects.
  // Renderer-local fallback handles belong only to the software weapon path.
  if(!live_client() && !(options.online_match && live_world()->source_objects && live_world()->source_objects->owns_physics()))
    live_match().configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &input) {
      if(input.effect==null_datum)return null_datum;
      const auto tag=cache.query_tag({.handle=TagHandle{input.effect}});
      if(tag && tag->group=="snd!") {
        weapon_effect_sounds.emplace_back(input.effect,input.weapon);return null_datum;
      }
      std::optional<SourceWeaponEffectPresentation> source;
      Vec3 velocity=unit.body.velocity;
      auto &binding=source_firing_instances[{std::uint16_t(input.weapon),input.effect}];
      const auto source_previous=binding.weapon==input.weapon && binding.presentation==input.previous_handle
          ?binding.source:null_datum;
      if(tag && tag->group=="effe" && live_world()->source_objects && live_world()->source_objects->owns_physics()) {
        auto original=input;original.previous_handle=source_previous;
        source=live_world()->source_objects->request_weapon_effect(original);
        velocity=live_world()->source_objects->object_linear_velocity(input.weapon);
        if(!source->colors_observed)throw FormatError("Prepared shot has no completed original effect-color owner");
        live_match().weapon_effect_presentation(unit.source_datum,input.weapon,*source,velocity);
      }
      if(source && source->instance==null_datum)return null_datum;
      if(source && source_particle_world && source_particle_draws.first_person_effect(input.effect)) {
        // The original instance owns its particles and RNG. Retain its actual
        // handle rather than creating another native emission history.
        binding={input.weapon,source->instance,source->instance};
        return source->instance;
      }
      const bool local=std::any_of(ids.begin(),ids.end(),[&](unsigned id){return live_match().player(id).source_datum==unit.source_player;});
      const bool fp=input.effect==muzzle_effect || firing_effects.contains(input.effect) || ready_effects.contains(input.effect);
      if(local && fp) {
        auto presentation=input;
        if(source && (source->instance==null_datum || source->instance!=source_previous))presentation.previous_handle=null_datum;
        const auto handle=source?muzzle_effects.request(presentation,std::uint32_t(live_match().simulation().ticks()+1),
            source->colors,velocity,world_firing_effects.contains(input.effect),true):
            muzzle_effects.request(presentation,std::uint32_t(live_match().simulation().ticks()+1));
        binding={input.weapon,handle,source?source->instance:null_datum};return handle;
      }
      if(!world_firing_effects.contains(input.effect) && firing_sound_effects.contains(input.effect))
        effect_sounds.spawn(input.effect,input.weapon,unit.body.position,std::bit_cast<float>(input.a_bits),std::bit_cast<float>(input.b_bits));
      const auto handle=source?source->instance:null_datum;binding={input.weapon,handle,handle};return handle;
    });
  struct FpWeaponAttachment {std::uint32_t effect{},scale_a{},scale_b{},marker_alias{};};
  std::map<std::uint32_t,std::vector<FpWeaponAttachment>> fp_weapon_attachments;
  for(const auto &[datum,weapon]:content.weapons) {
    const auto metadata=cache.load_source_tag_metadata(datum);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
    const auto count=root.u32(0x94);
    std::optional<SourceTagMetadata> attachment_owner;
    if(count) {
      const auto *owner=cache.query_tag({.contains=TagAddressRange{root.u32(0x98),std::size_t(count)*0x18},.require_address=true});
      if(!owner)throw FormatError("Weapon attachment bank has no resident source owner");
      attachment_owner.emplace(cache.load_source_tag_metadata(owner->datum));
    }
    const Bytes attachments(count?attachment_owner->bytes_at(root.u32(0x98),std::size_t(count)*0x18):std::span<const std::byte>{});
    for(unsigned n=0;n<count;++n) {
      const auto at=n*0x18;
      if(attachments.u32(at)!=0x65666665u || attachments.u32(at+4)==null_datum)continue;
      const auto effect=attachments.u32(at+4),a=attachments.u32(at+0x10),b=attachments.u32(at+0x14);
      // Material/bitmap admission needs no object-function/color execution.
      // The native and CPU producers keep their existing independent gates.
      (void)preload_source_weapon_effect(effect);
      try {
        const auto admitted=[&](auto name) {
          return !name || source_weapon_shader_input(name,{}) ||
              (name==0x0f000574u && !weapon.triggers.empty());
        };
        if(attachments.u32(at+0xc) || !admitted(a) || !admitted(b))
          throw FormatError("Weapon attachment requires its actual object-function/color owner");
        const auto markers=muzzle_effects.marker_names(effect);
        for(auto &[key,visual]:species)if(key.second==datum)
          for(const auto name:markers) {
            const auto selected=attachments.u32(at+8)?attachments.u32(at+8):name;
            visual.effect_markers.try_emplace(selected,source_model_markers(
                cache.load_source_tag_metadata(visual.fp_definition->weapon_model),selected));
          }
        fp_weapon_attachments[datum].push_back({effect,a,b,attachments.u32(at+8)});
      }catch(const FormatError &error) {std::cerr<<"Authored weapon attachment: "<<error.what()<<'\n';}
    }
  }
  MultiplayerSceneRuntimeValues scene_runtime_values;
  auto &scenery_initial_state=scene_runtime_values.scenery_initial_state;
  using SceneryQueryLifetime=MultiplayerSceneRuntimeValues::SceneryQueryLifetime;
  auto &scenery_query_lifetimes=scene_runtime_values.scenery_query_lifetimes;
  const bool source_scenery_regions=cache.original_data() && cache.original_data()->image==OriginalData::Image::TitleUpdate15 &&
      ((live_world()->source_objects && live_world()->source_objects->owns_physics()) || (live_client() && rules.custom==CustomSlayer::none && !rules.defusal));
  const SourceSceneryScene::LifetimeSelection scenery_regions=source_scenery_regions?
      SourceSceneryScene::LifetimeSelection{[&](std::size_t index,std::uint32_t handle,std::uint64_t born) {
        const auto &live=live_match().simulation().scene().get(handle);
        if(live.placement!=index || live.created_tick!=born)
          throw FormatError("SCEN actual regions lost their full source lifetime");
        if(live_world()->source_objects && live_world()->source_objects->owns_physics())
          return live_world()->source_objects->object_model_regions(handle).selection;
        if(live.source_model_regions)return live.source_model_regions->selection;
        throw FormatError("SCEN replica has no completed source model regions");
      }}:SourceSceneryScene::LifetimeSelection{};
  SourceSceneryScene source_scenery(cache,content.objects,options.bsp,mesh,object_light_collision,{},
      [&](std::size_t,const ScenarioObject &object) {
        // Only the four already identified Zanzibar RNG-dependent pillars
        // retain their old preview. An additional unresolved owner is an error.
        const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
        return tag &&
            tag->name=="scenarios\\objects\\multi\\zanzibar\\pillar\\pillar" &&
            object.placement>=19 && object.placement<=22;
      },source_scenery_regions);
  source_scenery.birth_query_capture([&](std::size_t source_object,std::uint32_t handle,
      std::uint64_t created_tick,std::uint32_t definition,const Transform &node0,float scale) {
    const auto &live=live_match().simulation().scene().get(handle);
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
  auto &scenery_render_fraction=scene_runtime_values.scenery_render_fraction;
  source_scenery.animation_frames([&](std::size_t source_object,std::uint32_t handle,
      std::uint64_t created_tick,std::uint64_t simulation_tick,const AnimationClip &clip) -> AnimationFrameWindow {
    const auto &live=live_match().simulation().scene().get(handle);
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
    const auto &live=live_match().simulation().scene().get(handle);
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
  if(!live_client())source_scenery.synchronize(live_match().simulation().scene(),scenery_regions);
  for(const auto &boundary:source_scenery.deferred_resource_owners()) {
    const auto &authored=content.objects.at(boundary.source_object);
    std::cout<<"SCEN inactive resource boundary: placement="<<boundary.source_object
        <<" origin_bsp="<<authored.origin_bsp<<" definition="<<authored.definition
        <<" error="<<boundary.error<<'\n';
  }
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
  SourceMachineScene machines(cache,content.objects,options.bsp,mesh,object_light_collision,source_scenery_regions);
  if(!live_client())machines.synchronize(live_match().simulation().scene(),&live_match().simulation().devices());
  if(reference_player && !reference_player->devices.empty())
    machines.set_reference_devices(reference_player->devices,mesh);
  auto &source_portal_mask=scene_runtime_values.source_portal_mask;
  auto &source_portal_owners=scene_runtime_values.source_portal_owners;
  {
    std::vector<SourcePortalOwnerState> owners;
    if(!live_client())for(const auto &packet:machines.portal_owners())owners.push_back(packet.owner);
    source_portal_mask.reset_acoustic();
    source_portal_mask.initialize(source_portal_definition,owners);
    for(const auto &owner:owners)source_portal_owners.emplace(owner.handle,owner);
  }
  auto &source_portal_tick=scene_runtime_values.source_portal_tick;
  source_portal_tick=live_match().state().tick;
  const auto &machine_counts=machines.stats();
  std::cout<<"Source machines: "<<machine_counts.placements<<" placements, "
           <<machine_counts.render_models<<" render models, "<<machine_counts.prt_materials
           <<" PRT material uses, "<<machine_counts.initial_state_only
           <<" door/platform placements; authoritative simulation/replica device state\n";
  auto &prop_initial_state=scene_runtime_values.prop_initial_state;
  const auto geometry_scenario_type=cache.load_source_geometry_scenario_type();
  std::function<std::optional<SourcePropQueryState>(std::uint32_t)> live_prop_query;
  auto &prop_query_handles=scene_runtime_values.prop_query_handles;
  SourcePropScene props(cache,content.objects,options.bsp,mesh,object_light_collision,source_scenery_regions);
  SourceDynamicObjectScene dynamic_objects(cache,options.bsp,mesh,object_light_collision);
  // SourcePropScene consumes this borrowed selection before its next callback.
  auto &actual_prop_regions=scene_runtime_values.actual_prop_regions;
  props.region_state([&](std::uint32_t handle)->const SourceInitialModelSelection * {
    if(!source_scenery_regions)return live_match().simulation().props().region_selection(handle);
    const auto &live=live_match().simulation().scene().get(handle);
    if(content.objects.at(live.placement).type!=11)
      throw FormatError("Prop region readback crossed its full source owner");
    if(live_world()->source_objects && live_world()->source_objects->owns_physics())
      actual_prop_regions=live_world()->source_objects->object_model_regions(handle).selection;
    else {
      if(!live.source_model_regions)throw FormatError("Prop replica has no completed source model regions");
      actual_prop_regions=live.source_model_regions->selection;
    }
    return &*actual_prop_regions;
  });
  SourceMapEffects map_effects(cache,content.objects,world_effects);
  {
    const auto state=map_effects.stats();
    std::cout<<"Map effect attachments: "<<state.prepared<<'/'<<state.attachments<<" prepared\n";
    for(const auto &gap:state.unavailable)std::cerr<<"Map effect attachment boundary: "<<gap<<'\n';
  }
  std::optional<SourceGrenadeScene> grenade_scene;
  std::vector<std::uint32_t> projectile_definitions;
  for(const auto &[datum,weapon]:content.weapons) {
    if(weapon.projectile!=null_datum)projectile_definitions.push_back(weapon.projectile);
    for(const auto &barrel:weapon.barrel_views)
      if(barrel->projectile!=null_datum)projectile_definitions.push_back(barrel->projectile);
    // barrel_views contains the charged-weapon presentation views, rather
    // than every native barrel. The original source can also publish an
    // authored secondary projectile (Scorpion coaxial input1/barrel1).
    // Prepare resources from its actual resident definition before drawing.
    const auto *tag=cache.query_tag({.handle=TagHandle{datum},.group=tag_groups::weapon,
        .require_address=true,.minimum_size=0x2d8});
    if(!tag)throw FormatError("Projectile preparation lost its resident weapon definition");
    const Bytes root(cache.read_tag_bytes(tag->address+0x2d0,8));
    const auto barrels=read_tag_block_descriptor(root,0);
    const auto extent=barrels.extent(0xec,4095);
    const Bytes rows(barrels.count?cache.read_tag_bytes(barrels.address,extent):std::span<const std::byte>{});
    for(unsigned barrel=0;barrel<unsigned(barrels.count);++barrel) {
      const auto projectile=rows.u32(barrel*0xecu+0x90u);
      if(projectile!=null_datum)projectile_definitions.push_back(projectile);
    }
  }
  std::ranges::sort(projectile_definitions);
  projectile_definitions.erase(std::unique(projectile_definitions.begin(),projectile_definitions.end()),projectile_definitions.end());
  NativeWeaponProjectileScene weapon_projectile_scene(cache,options.bsp,mesh,
      object_light_collision,projectile_definitions,live_match().native_object_capacity());
  auto combat_attachment_definitions=projectile_definitions;
  for(const auto &[datum,weapon]:content.weapons)combat_attachment_definitions.push_back(datum);
  for(const auto &projectile:cache.select_tags({.group=tag_groups::projectile,.require_address=true}))
    combat_attachment_definitions.push_back(projectile.datum);
  pstencil::PointLights pstencil_point_lights(cache,options.bsp);
  for(const auto& gap:pstencil_point_lights.unavailable())std::cerr<<"pStencil direct light boundary: "<<gap<<'\n';
  SourceCombatAttachments combat_attachments(cache,world_effects,combat_attachment_definitions);
  for(const auto &gap:combat_attachments.unavailable())std::cerr<<"Combat attachment boundary: "<<gap<<'\n';
  if(cache.original_data() && cache.original_data()->grenade_constants &&
     cache.original_data()->object_construction_operands)
    grenade_scene.emplace(cache,options.bsp,mesh,object_light_collision,live_match().native_object_capacity(),
        cache.original_data()->object_construction_operands->one_bits);
  // SceneObjects currently has no type11 physics advance. Retain the original
  // first BDEF0 output, proven before B8890 registers physics; later transform
  // changes require the actual subsequent physics/cache producer.
  if(!live_client())props.synchronize(live_match().simulation().scene());
  props.observe_source_draws(options.prop_source_draw_observer);
  props.observe_draws(options.prop_draw_observer);
  props.lighting_query_state([&](std::size_t source_object,std::uint32_t handle,
                                 const Transform& node0,float scale) {
    if(live_prop_query)if(const auto live=live_prop_query(handle))return *live;
    const auto &authored=content.objects.at(source_object);
    if(!prop_initial_state.contains(handle)) {
      const auto &live=live_match().simulation().scene().get(handle);
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
  path_tracing::SkyLighting path_tracing_sky_lighting;
  std::vector<std::optional<Visual>> sky_models;
  for(unsigned i=0;i<atmosphere.skies.size();++i) {
    skies.push_back(cache.load_sky(i));sky_models.emplace_back();
    sky_lighting.push_back(cache.object_sky_lighting_available(i)?
        cache.load_object_sky_lighting(i):std::optional<ObjectSkyLightingDefinition>{});
    if(skies.back() && skies.back()->model!=null_datum)sky_models.back()=visual(skies.back()->model);
  }
  std::map<std::pair<unsigned,std::uint32_t>,std::unique_ptr<MultiplayerHud>> huds,offhand_huds,vehicle_huds;
  std::map<unsigned,std::unique_ptr<MultiplayerHud>> unit_huds;
  std::map<std::pair<unsigned,std::uint32_t>,std::unique_ptr<MultiplayerHud>> parent_huds;
  const auto font_directory=gameplay_font_directory(cache);
  const auto font_bank=world->gameplay_fonts?world->gameplay_fonts:std::make_shared<GameplayFontBank>(font_directory);
  const auto load_hud=[&](std::uint32_t biped,std::uint32_t weapon) {
    return world->map_assets?world->map_assets->take_hud(biped,weapon):cache.load_hud(biped,weapon);
  };
  const auto voice_range=world->map_assets?world->map_assets->hud_sensor_range(content.player.datum,rules.weapon):
      cache.load_hud(content.player.datum,rules.weapon).sensor.range;
  for(unsigned representation:representations) {
    auto unit=load_hud(multiplayer_biped(content,representation).datum,null_datum);
    auto empty_weapon=unit;
    std::erase_if(unit.widgets,[](const auto &widget){return widget.weapon_type==1;});
    unit_huds.emplace(representation,std::make_unique<MultiplayerHud>(std::move(unit),
        font_directory,mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,0,font_bank,rules.defusal.has_value()));
    std::erase_if(empty_weapon.widgets,[](const auto &widget){return widget.weapon_type!=1;});
    empty_weapon.action_messages.enabled=false;
    empty_weapon.messages.enabled=false;
    huds.emplace(std::pair{representation,null_datum},std::make_unique<MultiplayerHud>(std::move(empty_weapon),
        font_directory,mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,0,font_bank));
  }
  for(unsigned representation:representations)for(const auto &binding:handheld_weapon_hud_bindings(cache,representation)) {
    const auto key=std::pair{representation,binding.definition};
    auto authored_hud=load_hud(multiplayer_biped(content,key.first).datum,key.second);
    if(!unit_huds.contains(key.first)) {
      auto unit_hud=authored_hud;
      std::erase_if(unit_hud.widgets,[](const auto &widget){return widget.weapon_type==1;});
      unit_huds.emplace(key.first,std::make_unique<MultiplayerHud>(std::move(unit_hud),
          font_directory,mesh,options.local_players,options.variant.name,
          cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,0,font_bank,rules.defusal.has_value()));
    }
    std::erase_if(authored_hud.widgets,[](const auto &widget){return widget.weapon_type!=1;});
    authored_hud.action_messages.enabled=false;
    authored_hud.messages.enabled=false;
    huds.emplace(key,std::make_unique<MultiplayerHud>(authored_hud,
        font_directory,mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,0,font_bank));
    if(binding.dual_wieldable)
      offhand_huds.emplace(key,std::make_unique<MultiplayerHud>(std::move(authored_hud),
          font_directory,mesh,options.local_players,options.variant.name,
          cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,1,font_bank));
  }
  std::set<std::uint32_t> vehicle_weapon_definitions,parent_definitions;
  if(content.unit_interactions)for(const auto &[id,unit]:content.unit_interactions->units) {
    const auto tag=cache.query_tag({.handle=TagHandle{id},.require_address=true});
    if(!tag || tag->size<0x1c8)throw FormatError("Vehicle HUD preparation lost its authored unit");
    const auto metadata=cache.load_source_tag_metadata(id);
    const Bytes descriptor(metadata.bytes_at(tag->address+0x1c0u,8));
    const auto count=descriptor.u32(0);
    if(count>4096)throw FormatError("Excessive authored unit weapon list");
    if(count) {
      const Bytes rows(metadata.bytes_at(descriptor.u32(4),count*8u));
      for(unsigned n=0;n<count;++n)if(const auto weapon=rows.u32(n*8u+4u);weapon!=null_datum)
        vehicle_weapon_definitions.insert(weapon);
    }
    for(auto hud:cache.load_unit_seat_huds(id))parent_definitions.insert(hud);
  }
  for(unsigned representation:representations)for(auto id:vehicle_weapon_definitions) {
    // This is a resource union, before any player occupies a seat. Cinematic
    // maps retain typed vehicle weapons with genuinely pruned HUD payloads.
    // Actual seated submission below still requires its prepared authored HUD.
    const Bytes weapon(cache.tag_payload(TagHandle{id}));
    const auto hud=weapon.u32(0x2b4);
    // A null weapon interface is authored absence, handled by load_hud as
    // an empty weapon widget bank (the Spectre turret uses this path).
    if(hud!=null_datum && !cache.resident_hud_definition(hud))continue;
    auto authored=load_hud(multiplayer_biped(content,representation).datum,id);
    std::erase_if(authored.widgets,[](const auto &w){return w.weapon_type!=1;});
    for(auto &w:authored.widgets)w.weapon_type=4;
    authored.action_messages.enabled=false;
    authored.messages.enabled=false;
    vehicle_huds.emplace(std::pair{representation,id},std::make_unique<MultiplayerHud>(std::move(authored),
        font_directory,mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,2,font_bank));
  }
  for(unsigned representation:representations)for(auto id:parent_definitions) {
    if(!cache.resident_hud_definition(id))continue;
    parent_huds.emplace(std::pair{representation,id},std::make_unique<MultiplayerHud>(
        cache.load_hud_definition(multiplayer_biped(content,representation).datum,id),
        font_directory,mesh,options.local_players,options.variant.name,
        cache.original_data()->hud_bitmap_geometry_constants,options.ui_assets,0,font_bank));
  }
  std::unique_ptr<MultiplayerScoreboard> scoreboard;
  if(source_game_type==2)scoreboard=std::make_unique<MultiplayerScoreboard>(cache,font_directory,mesh,variant,options.ui_assets);
  std::unique_ptr<GamePauseMenu> pause_menu;
  const bool trace_game_pause=std::getenv("HALO2_TRACE_GAME_PAUSE")!=nullptr;
  if(cache.is_mcc() || !options.ui_assets.empty()) {
    std::unique_ptr<char,decltype(&SDL_free)> pref(SDL_GetPrefPath("Halo2Native","Halo2"),SDL_free);
    pause_menu=std::make_unique<GamePauseMenu>(cache.shared_directory(),options.ui_assets,mesh,false,
        pref?std::filesystem::path(pref.get())/"menu.cfg":std::filesystem::path{},options.menu_ui_assets,cache.path());
    pause_menu->party_notices(options.party_notice,options.party_notice_action);
  }
  std::optional<unsigned> session_black_material;
  std::unique_ptr<ObjectiveWaypoints> objective_waypoints;
  std::optional<SourceRuntimeMemory> objective_replica_memory;
  if((rules.source_engine==1 || rules.source_engine==9) && cache.is_mcc())
    objective_waypoints=std::make_unique<ObjectiveWaypoints>(cache,mesh,options.local_players);
  std::uint32_t objective_goal_shader=null_datum,objective_goal_material=null_datum;
  SourceTransparentPart objective_goal_metadata;
  if(rules.source_engine==9 && cache.is_mcc()) {
    objective_goal_shader=cache.load_multiplayer_hill_shader();
    auto material=cache.load_material(objective_goal_shader);
    if(!prepare_simple_world_material(cache,material))throw FormatError("Authored Assault goal shader has no native material owner");
    objective_goal_metadata=cache.load_source_transparent_shader(objective_goal_shader);
    if(objective_goal_metadata.template_lod_layers.size()!=1)
      throw FormatError("Assault goal shader requires its actual multiple-LOD selector");
    objective_goal_material=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(material));
  }
  std::array<SourceTransparentQueue,4> objective_goal_queues;
  std::array<std::vector<std::array<Vertex,192>>,4> objective_goal_vertices;
  std::array<std::vector<SourceTransparentDrawBinding>,4> objective_goal_bindings;
  for(auto& vertices:objective_goal_vertices)vertices.reserve(18);
  if(options.session_view_black || cache.is_mcc()) {
    session_black_material=unsigned(mesh.materials.size());
    mesh.materials.push_back({});
    mesh.materials.back().ui_full_surface=true;
  }
  std::cout<<"Source resident material roots: "<<mesh.materials.size()<<'\n';
  auto source_model_roots=vehicle_render_models;
  for(const auto model:props.source_render_models())source_model_roots.insert(model);
  for(const auto& [definition,weapon]:body_guns)if(weapon.gun.render_model!=null_datum)
    source_model_roots.insert(weapon.gun.render_model);
  for(const auto& representation:content.player_representations)if(representation.biped.model!=null_datum) {
    const auto model=cache.load_source_tag_metadata(representation.biped.model);
    source_model_roots.insert(Bytes(model.bytes_at(model.source_address(),model.size())).u32(4));
  }
  auto* source_model_world=!live_client() && !reference_player && !options.capture_root &&
      !source_model_roots.empty() && live_world()->source_objects && live_world()->source_objects->owns_physics()?live_world()->source_objects.get():nullptr;
  bool source_model_resources=source_model_world || (source_migration_resources && !source_model_roots.empty());
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
  source_model_world=nullptr; // This isolated fixture supplies separate matched-reference owners.
  source_model_resources=false;
#endif
  if(const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_LIVE_VEHICLE_MODELS");disabled && *disabled) {
    source_model_world=nullptr;
    source_model_resources=false;
  }
  const std::vector source_resident_models(source_model_roots.begin(),source_model_roots.end());
  // The original4C190/4C2B0 model builder requests only current selected
  // sections through130980. Eagerly retaining every vehicle LOD exhausts
  // the original512-record cache (Waterworks heavy1 fails at section513).
  auto all_effect_light_definitions=world_effects.light_definitions();
  for(const auto id:pstencil_point_lights.definitions())
    if(std::find(all_effect_light_definitions.begin(),all_effect_light_definitions.end(),id)==all_effect_light_definitions.end())
      all_effect_light_definitions.push_back(id);
  SourceBspEffectLights bsp_effect_lights(cache,mesh,options.bsp,all_effect_light_definitions,
      source_model_resources?std::span<const std::uint32_t>(source_resident_models):std::span<const std::uint32_t>{});
  for(const auto& gap:bsp_effect_lights.unavailable())
    if(reported_world_effect_gaps.insert(gap).second)
      std::cerr<<"BSP effect light material boundary: "<<gap<<'\n';
  //Reference-only raw residency is immutable and must exist before the
  //Renderer uploads its Mesh. It does not activate source GPU readiness.
  std::unique_ptr<SourceShadowScene> source_shadow_scene;
  if(options.reference_live_shadow_gpu_tick) {
    if(!source_model_world)throw FormatError("Live shadow GPU reference lacks its actual native model world");
    source_shadow_scene=std::make_unique<SourceShadowScene>(cache,mesh,options.bsp,source_resident_models);
  }
  bind_water_pool_materials(water,mesh.materials);
  const auto source_materials_at_upload=mesh.materials.size();
  const auto renderer_owner=platform.desktop->make_renderer(mesh,options.validation,{},bsp_shaders?&*bsp_shaders:nullptr,
      options.capture_shader_directory,source_shadow_scene?source_shadow_scene->resident():std::span<const SourceShadowRawPlanMaterial>{});
  auto& renderer=*renderer_owner;
  renderer.path_tracing_source(mesh);
  // All source model/geometry levels above and the mesh are synchronously
  // resident before this constructor returns. Original130980 increments its
  // unresolved count only for a still-unavailable requested cache block. The
  // native resident graph has no outstanding request at this point.
  const std::int32_t gameplay_geometry_unresolved=0;
  // Admit resident scene/history owners before loaded() releases the real
  // peers. Constructing this graph after admission can exhaust the normal
  // input-idle window before the first actual client command is delivered.
  // The renderer has now admitted its own material roots; every borrower and
  // generated radar page below already has its fixed resident shape.
  const bool retain_effect_history=options.online_match && options.online_match->direct_udp() &&
      options.online_match->peer_migration().has_value();
  std::optional<MaterialPresentationContinuity> material_continuity;
  std::optional<MultiplayerSceneContinuity> scene_continuity;
  std::optional<MultiplayerHudContinuity> hud_continuity;
  if(retain_effect_history) {
    std::vector<unsigned> sensor_materials;
    for(const auto &[representation,hud]:unit_huds)
      for(unsigned view=0;view<options.local_players;++view)
        if(const auto material=hud->motion_sensor_material(view))sensor_materials.push_back(*material);
    material_continuity.emplace(mesh,sensor_materials);
    scene_continuity.emplace(MultiplayerSceneContinuity::Targets{scene_runtime_values,
        source_scenery,machines,props,dynamic_objects,weapon_projectile_scene,
        grenade_scene?&*grenade_scene:nullptr,pstencil_point_lights,*material_continuity});
    hud_continuity.emplace(MultiplayerHudContinuity::Targets{
        huds,offhand_huds,vehicle_huds,unit_huds,parent_huds,scoreboard});
  }
  // The query owner retires GPU resources through this renderer and must be
  // destroyed before renderer_owner, including captures and early exits.
  std::unique_ptr<SourceLensQueryFrameTu15> source_lens_queries;
  if(source_lens_resources)source_lens_queries=std::make_unique<SourceLensQueryFrameTu15>(renderer);
  if(source_lens_draws)source_lens_draws->bind(renderer);
  std::unique_ptr<NativeLensPresentationTu15> native_lens;
  if(native_lens_resources) {
    native_lens=std::make_unique<NativeLensPresentationTu15>(cache,*source_lens_draws);
    native_lens->bind(renderer);
  }
  if(pause_menu)pause_menu->bind(platform.window,*platform.desktop,renderer);
  if(trace_game_pause)std::cerr<<"Native pause owner mcc="<<cache.is_mcc()<<" present="<<bool(pause_menu)
    <<" window="<<SDL_GetWindowID(platform.window)<<'\n';
  renderer.share_display_settings(platform.desktop->display_settings);
  renderer.measure_frames(bool(options.frame_observer));
  // The existing timing-file diagnostic can measure preparation directly;
  // it does not install an observer or enable receipt/schedule observations.
  const bool write_host_timings=renderer.writes_frame_timings();
  const bool measure_host_timings=bool(options.frame_observer) || write_host_timings;
  renderer.scene_bsp_atmosphere(atmosphere);
  std::cout << "Multiplayer GPU: " << renderer.device_name()
            << " split_views=" << options.local_players << std::endl;
  std::unique_ptr<MultiplayerAudio> audio;
  struct SourceWeaponAudio {
    struct Request {std::uint32_t sound;std::array<std::byte,160> raw;};
    MultiplayerObjectRuntime *world{};
    std::vector<Request> pending;
    std::vector<SourceLoopSoundRequestTu15> loops;
    ~SourceWeaponAudio(){if(world){world->observe_sound_requests({});world->tu15_world().observe_loop_sound_requests({});}}
  } source_weapon_audio;
  const auto audio_bsp=cache.load_multiplayer_audio_bsp(options.bsp);
  bool ambient_unavailable_reported{};
  if (!options.mute)
    try {
      const auto seed=platform.desktop->fresh_secondary_random();
      audio = std::make_unique<MultiplayerAudio>(cache_owner,seed,
          SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,512,{}},
          options.audio_observer,&auxiliary_random);
      if(!live_client() && cache.is_mcc() && live_world()->source_objects &&
          live_world()->source_objects->owns_physics()) {
        // Native playback consumes Xbox predicates and sound-class defaults.
        // Cache still owns the raw MCC routing bits and PCM resources. Bind
        // the actual converted World before any authored bank is prepared.
        audio->enroll_continuation();
        audio->bind_native_sound_metadata(
            live_world()->source_objects->tu15_world().sound_variation_provider(cache));
        audio->bind_object_loop_random_source([desktop=platform.desktop]{return desktop->fresh_secondary_random();});
      }
      if(live_client() && cache.is_mcc()) {
        audio->enroll_continuation();
        audio->bind_native_loop_metadata(MccSoundMetadataPublication::create(cache_owner));
        audio->bind_object_loop_random_source([desktop=platform.desktop]{return desktop->fresh_secondary_random();});
      }
      if(auto error=audio->speaker_layout(platform.desktop->display_settings.speakers);!error.empty())std::cerr<<"Audio layout: "<<error<<'\n';
      if(pause_menu)pause_menu->audio_layout([&](unsigned layout){return audio?audio->speaker_layout(layout):std::string{};});
      for(const auto &[representation,v]:species)if(v.fp_definition)for(const auto &clip:v.fp_definition->clock_clips)
        for(const auto &event:clip.events)if(event.type==1)try{audio->prepare(event.tag);}
          catch(const std::exception &e){std::cerr<<"Authored sound "<<std::hex<<event.tag<<std::dec<<": "<<e.what()<<'\n';}
      if(!live_client() && !(live_world()->source_objects.get() && live_world()->source_objects.get()->owns_physics()))
        live_match().configure_first_person_event_owner(
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
  if(audio && !live_client() && live_world()->source_objects &&
      live_world()->source_objects->owns_physics()) {
    source_weapon_audio.world=live_world()->source_objects.get();
    source_weapon_audio.world->tu15_world().observe_loop_sound_requests([&](const auto& request) {
      source_weapon_audio.loops.push_back(request);
    });
    source_weapon_audio.world->observe_sound_requests([&](std::uint32_t sound,std::span<const std::byte,160> raw) {
      const Bytes request(raw);
      // Original caller has already selected the sound, owner, control and
      // local audibility. Retain that intent once; never rerun its clock or
      // substitute an original admission/voice result. Controllers retain
      // their separate source loop/stop owner.
      if(!(request.u32(0)&8u) || (request.u32(0)&0x10u) ||
          (std::to_integer<unsigned>(raw[0xf])&15u)>1u)return;
      SourceWeaponAudio::Request event{sound,{}};
      std::copy(raw.begin(),raw.end(),event.raw.begin());
      source_weapon_audio.pending.push_back(std::move(event));
    });
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
  std::unique_ptr<NativeUnitDamageAudio> damage_audio;
  if(audio)damage_audio=std::make_unique<NativeUnitDamageAudio>(cache,*audio,auxiliary_random);
  auto &client_state_bound=receipt_values.client_state_bound;
  client_state_bound=live_client() && live_client()->ready();
  MultiplayerState current = client_state_bound ? live_client()->state() : live_match().state(), previous = current;
  if(options.initial_gameplay_fade)gameplay_fade=*options.initial_gameplay_fade;
  bool gameplay_fade_available{};
  const auto local_vehicle_sound_view=[&](std::uint32_t owner,bool source_owner)->std::optional<unsigned> {
    if(owner==null_datum || !source_weapon_audio.world)return {};
    for(unsigned view=0;view<ids.size();++view) {
      const auto &p=current.players[ids[view]];const auto &hud=p.source_vehicle_hud;
      const auto weapon=hud.ownership.vehicle_weapon;
      if(!p.connected || p.dead || !hud.ownership.seated || !hud.weapon.present ||
          weapon==null_datum || hud.weapon.datum!=weapon || p.source_parent==null_datum)continue;
      if(!source_owner) {if(owner==weapon)return view;continue;}
      // The original effect caller owns its sound on the armed unit, not
      // necessarily on the weapon object. Scorpion's cannon is a child of
      // the occupied chassis; a Warthog gunner occupies the turret itself.
      // Require the full weapon ancestry to reach that actual seated parent.
      bool matched=false;auto object=weapon;
      for(unsigned depth=0;depth<0x800 && object!=null_datum;++depth) {
        matched|=object==owner;
        if(object==p.source_parent){if(matched)return view;break;}
        object=source_weapon_audio.world->object_parent(object);
      }
    }
    return {};
  };
  const auto local_vehicle_input_definition=[&](unsigned view)->const WeaponDefinition* {
    if(view>=ids.size())return nullptr;
    const auto &p=current.players[ids[view]];const auto &hud=p.source_vehicle_hud;
    if(!hud.ownership.seated || !hud.weapon.present ||
        hud.ownership.vehicle_weapon==null_datum || hud.weapon.datum!=hud.ownership.vehicle_weapon)return nullptr;
    const auto found=content.weapons.find(hud.definition);
    return found==content.weapons.end()?nullptr:&found->second;
  };
  const auto deliver_source_weapon_audio=[&]{
    for(const auto &event:source_weapon_audio.pending) {
      const auto owner=Bytes(event.raw).u32(0x50);
      if(owner==null_datum)continue;
      const auto local=std::find_if(ids.begin(),ids.end(),[&](auto id){
        const auto &p=current.players[id];
        return p.connected && (p.source_weapon==owner || p.secondary_weapon.datum==owner);
      });
      const auto view=local!=ids.end()?std::optional<unsigned>(unsigned(local-ids.begin())):
          local_vehicle_sound_view(owner,true);
      if(!view)continue;
      const auto kind=(std::to_integer<unsigned>(event.raw[0xf])&15u)==1u?
          MultiplayerAudioKind::Impulse:MultiplayerAudioKind::FirstPerson;
      try{audio->source_impulse(event.sound,event.raw,kind,*view);}
      catch(const std::exception &error){std::cerr<<"Source weapon audio: "<<error.what()<<'\n';}
    }
    source_weapon_audio.pending.clear();
  };
  MultiplayerSnapshotOrigin current_receipt=live_client()?live_client()->state_origin():MultiplayerSnapshotOrigin{};
  const bool terminal_certified_playout=options.online_match&&options.online_match->terminal_recovery_enabled()&&live_client();
  std::deque<MultiplayerSnapshotOrigin> terminal_snapshots;
  std::optional<MultiplayerSnapshotOrigin> terminal_bootstrap_snapshot;
  std::shared_ptr<const SourceAuthorityEndpoint> terminal_projection_endpoint,terminal_current_endpoint;
  std::uint64_t terminal_snapshot_peak{},terminal_snapshot_bytes{},terminal_snapshot_bytes_peak{},terminal_adoptions{},terminal_lag_peak{};
  const auto drop_terminal_snapshot=[&]{
    terminal_snapshot_bytes-=terminal_snapshots.front().encoded_body_bytes();terminal_snapshots.pop_front();
  };
  const auto queue_terminal_snapshots=[&](std::span<const MultiplayerState> states,
      std::span<const MultiplayerSnapshotOrigin> origins){
    if(!terminal_certified_playout)return;
    if(states.size()!=origins.size())throw FormatError("Terminal snapshot lost its authenticated origin");
    for(std::size_t i=0;i<states.size();++i){
      if(!origins[i].context())continue;
      terminal_snapshots.push_back(origins[i]);terminal_snapshot_bytes+=origins[i].encoded_body_bytes();
    }
    terminal_snapshot_peak=std::max<std::uint64_t>(terminal_snapshot_peak,terminal_snapshots.size());
    terminal_snapshot_bytes_peak=std::max(terminal_snapshot_bytes_peak,terminal_snapshot_bytes);
    // Eight seconds at the normal source snapshot rate. Exceeding this
    // qualification budget fails closed; no unproven state is presented and
    // no missing event prefix is silently discarded.
    if(terminal_snapshots.size()>256||terminal_snapshot_bytes>64ull*1024*1024)throw FormatError("Terminal certified snapshot queue exceeded its qualification budget");
  };
  const auto terminal_match=[&](const std::shared_ptr<const SourceAuthorityEndpoint> &endpoint,
      const MultiplayerSnapshotOrigin &origin){
    if(!endpoint)return false;
    try{return endpoint->proof().prove_snapshot(*live_client(),origin).matches(origin);}
    catch(const FormatError &){return false;}
  };
  auto &previous_receipt=receipt_values.previous;
  previous_receipt=current_receipt;
  if(options.capture_tick && current.tick>options.capture_tick)
    throw FormatError("Capture tick precedes the current authoritative world tick");
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
  const auto consume_journal_weapon_effects=[&]{
    if(!client_effect_baseline) {
      if(!current.weapon_effects.empty())client_effect_serial=current.weapon_effects.back().serial;
      client_effect_baseline=true;return;
    }
    for(const auto &effect:current.weapon_effects)if(effect.serial>client_effect_serial) {
      client_effect_serial=effect.serial;
      // Legacy host callbacks already submit their first-person presentation.
      // Original scheduled firing reaches this journal after its source owner completes.
      if(!live_client() && !effect.source_scheduled)continue;
      if(effect.source_presentation && effect.source_instance==null_datum)continue;
      const auto local=std::find_if(ids.begin(),ids.end(),[&](auto id) {
        const auto &p=current.players[id];return p.connected && p.source_unit==effect.unit &&
            (p.source_weapon==effect.weapon || p.secondary_weapon.datum==effect.weapon);
      });
      if(local==ids.end())continue;
      const auto &p=current.players[*local];
      if(effect.source_scheduled && source_particle_world &&
          source_particle_draws.first_person_effect(effect.effect))continue;
      if(effect.effect==muzzle_effect || firing_effects.contains(effect.effect) || ready_effects.contains(effect.effect)) {
        auto &binding=source_firing_instances[{std::uint16_t(effect.weapon),effect.effect}];
        const auto previous=binding.weapon==effect.weapon && effect.source_instance!=null_datum && binding.source==effect.source_instance
            ?binding.presentation:null_datum;
        const WeaponEffectInput input{effect.effect,effect.weapon,previous,effect.a_bits,effect.b_bits};
        const auto handle=effect.source_presentation?muzzle_effects.request(input,effect.tick,effect.colors,
            effect.velocity,world_firing_effects.contains(effect.effect),effect.source_presentation):muzzle_effects.request(input,effect.tick);
        binding={effect.weapon,handle,effect.source_instance};
      }else {
        const auto tag=cache.query_tag({.handle=TagHandle{effect.effect}});
        if(tag && tag->group=="snd!")weapon_effect_sounds.emplace_back(effect.effect,effect.weapon);
        else if(!world_firing_effects.contains(effect.effect) && firing_sound_effects.contains(effect.effect))
          effect_sounds.spawn(effect.effect,effect.weapon,p.position,std::bit_cast<float>(effect.a_bits),std::bit_cast<float>(effect.b_bits));
      }
    }
  };
  auto &scene_interpolation=pose_values.scene_interpolation;
  scene_interpolation.reset(current.tick,live_match().simulation().scene().objects());
  auto &source_body_interpolation=pose_values.source_body_interpolation;
  auto &source_render_interpolation=pose_values.source_render_interpolation;
  // Immutable model-order hierarchy only; no geometry, channel evaluation or
  // GPU admission is added when a joined WORLD owner first appears.
  std::map<std::uint32_t,RenderModel> source_world_hierarchies;
  // Non-host dead reckoning of physics objects/projectiles and immediate
  // replicated impulses. Presentation only; the host never runs it.
  std::unordered_map<std::uint32_t,float> replica_projectile_gravity;
  for(const auto &[datum,weapon]:content.weapons)
    if(weapon.projectile!=null_datum && std::isfinite(weapon.projectile_gravity))
      replica_projectile_gravity.emplace(weapon.projectile,weapon.projectile_gravity*retail_gravity);
  // Non-host movement prediction (NETCODE_ENHANCED_GOAL item 1): the own
  // on-foot biped answers on the input tick and reconciles with every
  // snapshot. H2_NET_PREDICTION=0 disables it (draws host motion).
  std::unique_ptr<NonHostMovementClient> nonhost_movement;
  const bool nonhost_prediction_enabled=[] {
    const auto *value=std::getenv("H2_NET_PREDICTION");return !(value && std::string_view(value)=="0");
  }();
  const auto ensure_nonhost_movement=[&]()->NonHostMovementClient* {
    if(!nonhost_movement && nonhost_prediction_enabled && live_client() && live_client()->ready())
      nonhost_movement=std::make_unique<NonHostMovementClient>(live_client()->state().netcode_mode,content.movement,
          cache.original_data()?cache.original_data()->unit_locomotion_deadzone_bits:std::nullopt);
    return nonhost_movement.get();
  };
  ReplicaMotion replica_motion({},[&replica_projectile_gravity](std::uint32_t definition)->std::optional<float> {
    const auto found=replica_projectile_gravity.find(definition);
    return found==replica_projectile_gravity.end()?std::nullopt:std::optional{found->second};
  },[&content](const SourceSceneRoot &root) {
    // Physics props only: machines, controls, light fixtures and sound
    // scenery keep their own device/static presentation.
    if(root.placement>=content.objects.size())return false;
    const auto type=content.objects[root.placement].type;
    return type!=7 && type!=8 && type!=9 && type!=10;
  });
  const auto replica_seconds=[](Clock::time_point at){return std::chrono::duration<double>(at.time_since_epoch()).count();};
  // Predicted impact sparks for the local player's own precision hitscan
  // shots (NETCODE_ENHANCED_GOAL item 6; Balanced/Full non-hosts only).
  // Presentation only: world surfaces, never a player hit. A replicated host
  // contact that is the copy of a predicted one is not spawned again.
  std::array<PredictedShotGate,4> predicted_shot_gates{};
  PredictedImpactLedger predicted_impact_ledger;
  std::optional<PredictedImpactTracer> predicted_impact_tracer;
  std::uint32_t predicted_impact_random=0x5d1c7a39u;
  const auto predicted_impact_copy=[&](const PredictedImpactLedger::HostImpact &host) {
    return predicted_impact_ledger.pending() && live_client() &&
        predicted_impact_ledger.suppress(host,replica_seconds(Clock::now()));
  };
  const auto capture_source_body_poses=[&](bool reset=false,bool refresh=false) {
    if(refresh)refresh_multiplayer_body_poses(source_body_interpolation,previous,current);
    else capture_multiplayer_body_poses(source_body_interpolation,previous,current,reset);
    if(reset)source_render_interpolation.clear();
    std::set<std::uint32_t> cuts;
    for(unsigned n=0;n<current.players.size();++n) {
      const auto &a=previous.players[n],&b=current.players[n];
      if(a.source_weapon!=b.source_weapon || a.secondary_weapon.datum!=b.secondary_weapon.datum)
        for(auto owner:{a.source_weapon,b.source_weapon,a.secondary_weapon.datum,b.secondary_weapon.datum})cuts.insert(owner);
      if(a.connected==b.connected && a.dead==b.dead && a.source_player==b.source_player &&
         a.source_unit==b.source_unit && a.source_parent==b.source_parent && a.source_seat==b.source_seat &&
         a.source_teleport_serial==b.source_teleport_serial &&
         length(a.position-b.position)<5)continue;
      for(auto owner:{a.source_unit,b.source_unit,a.source_weapon,b.source_weapon,
          a.secondary_weapon.datum,b.secondary_weapon.datum})cuts.insert(owner);
    }
    std::erase_if(source_render_interpolation,[&](const auto &entry) {
      return cuts.contains(entry.first.first) || std::none_of(current.source_body_poses.begin(),
          current.source_body_poses.end(),[&](const auto &pose){return pose.object==entry.first.first;});
    });
  };
  capture_source_body_poses(true);
  if(live_client() && live_client()->ready())ids.assign(live_client()->slots().begin(),live_client()->slots().end());
  auto update_sensors=[&] {
    for(unsigned view=0;view<ids.size();++view) {
      const auto id=ids[view];
      unit_huds.at(source_session_player_representation(current.players[id].source_configuration,source_game_type))->update_motion_sensor(current,id,view);
    }
  };
  update_sensors();
  std::array<std::uint32_t,4> uploaded_sensor_tick{~0u,~0u,~0u,~0u};
  auto &aim_blends=pose_values.aim_blends;
  auto &body_aim_blends=pose_values.body_aim_blends;
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
      const auto *clip=live_match().grenade_clip(a.grenade_action.graph,a.grenade_action.clip);
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
  auto &body_transitions=pose_values.body_transitions;
  std::array<std::unique_ptr<NativeLocomotion>,multiplayer_capacity> gait;
  std::array<bool,multiplayer_capacity> gait_dual{};
  std::array<std::uint32_t,multiplayer_capacity> gait_units{},gait_weapons{};
  std::array<bool,multiplayer_capacity> gait_unavailable{};
  std::optional<MultiplayerLocomotionContinuity> locomotion_continuity;
  if(retain_effect_history)locomotion_continuity.emplace(MultiplayerLocomotionContinuity::Targets{
      gait,gait_dual,gait_units,gait_weapons,gait_unavailable});
  const auto update_native_gait=[&](unsigned id,bool advance=true) {
    if(locomotion_continuity)locomotion_continuity->mutating();
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
        gait[id]=std::make_unique<NativeLocomotion>(cache_owner,player.model.animation,p.weapon,
            std::array<std::uint32_t,2>{rules.random_seed,p.source_unit},dual);
        gait[id]->bind_facing(biped_for(p).datum,p.yaw);
      }
      // A newer packet may replace roster/owners at this completed tick.
      // Resolve/reset the lifetime above without ticking its source channel.
      if(!advance)return;
      // Only a type0 throw owns the base. Masked throws retain the running
      // gait clock and its current foot events beneath the authored overlay.
      if((p.source_action_word&(1u<<22))&&p.grenade_action.animation_active) {
        const auto *clip=live_match().grenade_clip(p.grenade_action.graph,p.grenade_action.clip);
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
  auto &world_effect_tick=receipt_values.world_effect_tick;
  auto &attached_effect_objects=receipt_values.attached_effect_objects;
  const auto source_decal_consumer_owned=[&] {
#if HALO2_SOURCE_DECALS_AVAILABLE
    return cached_decals.active() && current.source_decals.has_value();
#else
    return false;
#endif
  };
  const auto update_world_effects=[&](bool accepted_replica=false) {
    world_effects.source_decal_cache(source_decal_consumer_owned());
    // Read actual occupied source owners and their current WORLD pose. The
    // optional direct-light bank has no simulation or Original-render writes.
    {
      std::vector<std::vector<Transform>> point_nodes;
      std::vector<pstencil::PointLightOwner> point_owners;
      std::vector<std::vector<pstencil::AttachedLight>> native_point_lights;
      if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics()) {
      const auto objects=live_world()->source_objects->live_objects();point_nodes.reserve(objects.size());
      native_point_lights.reserve(objects.size());
      for(const auto& object:objects)if(pstencil_point_lights.has(object.definition)) {
        const auto body=live_world()->source_objects->source_object_body(object.object);
        const Bytes fields(live_world()->source_objects->memory().view(body+0x64u,36));
        const auto vector=[&](unsigned at){return Vec3{fields.f32(at),fields.f32(at+4),fields.f32(at+8)};};
        auto root=source_object_transform(vector(0),vector(12),vector(24),1,false);
        point_nodes.push_back(live_world()->source_objects->world_nodes(object.object));
        if(object.parent!=null_datum && !point_nodes.back().empty())root=point_nodes.back().front();
        native_point_lights.push_back(pstencil::attached_lights_tu15(
            live_world()->source_objects->memory(),object.object,object.definition));
        point_owners.push_back({object.object,object.definition,root,point_nodes.back(),{},native_point_lights.back()});
      }
      }
      if(options.capture_tick)for(const auto& projectile:current.grenade_projectiles)
        std::cout<<"pStencil grenade input: tick="<<current.tick<<" definition="<<projectile.definition
          <<" object="<<projectile.datum<<" type="<<projectile.type<<" admitted="<<pstencil_point_lights.has(projectile.definition)<<'\n';
      for(const auto& projectile:current.grenade_projectiles)
        if(pstencil_point_lights.has(projectile.definition) &&
           std::none_of(point_owners.begin(),point_owners.end(),[&](const auto& p){return p.object==projectile.datum;})) {
          const auto root=source_object_transform(projectile.position,projectile.forward,projectile.up,1,false);
          point_owners.push_back({projectile.datum,projectile.definition,root,{},{}});
        }
      pstencil_point_lights.synchronize(point_owners,current.tick);
      if(options.capture_tick && !point_owners.empty())for(const auto& light:pstencil_point_lights.lights())
        std::cout<<"pStencil point lifetime: tick="<<current.tick<<" definition="<<light.definition
          <<" owner="<<light.effect_owner<<" position="<<light.position.x<<','<<light.position.y<<','<<light.position.z
          <<" diffuse="<<light.state.diffuse.x<<','<<light.state.diffuse.y<<','<<light.state.diffuse.z<<'\n';
    }
    if(live_client() && !accepted_replica && !world_effect_baseline)return;
    if(live_client() && accepted_replica) {
      live_match().replicate_devices(current.devices);
      if(current.source_scene_roots_authoritative)
        live_match().replicate_source_scene(current.source_scene_roots,current.source_body_poses);
      live_match().replicate_prop_removals(current.prop_removals);
      live_match().replicate_prop_regions(current.prop_regions);
    }
    //Publish the actual current scene pose on the simulation tick before
    //emission. The later render interpolation is a separate read-only sample.
    machines.synchronize(live_match().simulation().scene(),&live_match().simulation().devices());
    props.synchronize(live_match().simulation().scene());
    source_scenery.synchronize(live_match().simulation().scene(),scenery_regions);
    const auto render_fraction=scenery_render_fraction;scenery_render_fraction=1;
    machines.update_pose(current.devices.tick,std::int32_t(current.tick),mesh);
    props.update_pose(std::int32_t(current.tick),mesh);
    source_scenery.update_pose(current.tick,std::int32_t(current.tick),mesh);
    scenery_render_fraction=render_fraction;
    auto map_owners=source_scenery.effect_owners();
    for(const auto &owner:machines.effect_owners())map_owners.push_back(owner);
    for(const auto &owner:props.effect_owners())map_owners.push_back(owner);
    map_effects.synchronize(map_owners,current.tick,current.source_scene_roots);
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
    // Vehicle and AI child weapons have genuine source WORLD nodes, but do
    // not appear in the native player-held weapon bank above.
    if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics())
      for(const auto &object:live_world()->source_objects->live_objects()) {
        if(object.type!=2 || present.contains(object.object))continue;
        const auto authored=firing_world_markers.find(object.definition);
        if(authored==firing_world_markers.end())continue;
        const auto nodes=live_world()->source_objects->world_nodes(object.object);
        if(nodes.empty())continue;
        NativeWorldEffects::ObjectMarkers markers;
        for(const auto &[name,bank]:authored->second)for(const auto &marker:pose_model_markers(bank,nodes)) {
          const auto &t=marker.world;markers[name].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
        }
        if(!markers.contains("root")) {
          const auto &t=nodes.front();markers["root"].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
        }
        world_effects.object_markers(object.object,std::move(markers));present.insert(object.object);
      }
    if(current.source_body_poses_authoritative)for(const auto &corpse:current.corpses) {
      if(present.contains(corpse.object))continue;
      const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
          [&](const auto &p){return p.object==corpse.object;});
      if(pose==current.source_body_poses.end())continue;
      const auto &visual=species.at({source_session_player_representation(corpse.configuration,source_game_type),rules.weapon});
      publish_markers(corpse.object,visual,pose->nodes);
    }
    if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics()) {
      const auto combat=combat_attachments.synchronize(*live_world()->source_objects,current.tick);
      present.insert(combat.begin(),combat.end());
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
    std::map<std::uint32_t,NativeWorldEffects::ObjectMarkers> firing_locations;
    for(const auto &effect:current.weapon_effects) {
      if(!effect.source_locations_observed || firing_locations.contains(effect.source_instance) ||
         !world_firing_effects.contains(effect.effect))continue;
      auto &locations=firing_locations[effect.source_instance];
      for(const auto &location:effect.source_locations) {
        const auto &t=location.frame;
        locations[cache.string_id(location.name)].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});
      }
      world_effects.update_weapon_locations(effect.weapon,effect.source_instance,locations);
    }
    for(const auto &effect:current.weapon_effects)if(effect.serial>world_weapon_effect_serial) {
      world_weapon_effect_serial=effect.serial;
      if(!world_firing_effects.contains(effect.effect) ||
         (effect.source_presentation && effect.source_instance==null_datum))continue;
      const bool local=std::any_of(ids.begin(),ids.end(),[&](auto id) {
        const auto &player=current.players[id];return player.connected && player.source_unit==effect.unit &&
            (player.source_weapon==effect.weapon || player.secondary_weapon.datum==effect.weapon);
      });
      const bool fp=effect.effect==muzzle_effect || firing_effects.contains(effect.effect) || ready_effects.contains(effect.effect);
      world_effects.request_weapon(effect.effect,effect.weapon,effect.tick,effect.colors,effect.velocity,
          std::bit_cast<float>(effect.a_bits),std::bit_cast<float>(effect.b_bits),effect.source_instance,!local || !fp,
          firing_locations.contains(effect.source_instance)?&firing_locations.at(effect.source_instance):nullptr);
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
      if(contact.bullet && predicted_impact_copy(predicted_host_impact(*contact.bullet,[&] {
          const auto weapon=content.weapons.find(contact.bullet->weapon);
          return weapon==content.weapons.end()?null_datum:weapon->second.projectile;}())))continue;
      try{
        if(contact.bullet)world_effects.impact(*contact.bullet);
        else world_effects.melee_impact(*contact.melee);
      }catch(const FormatError&error){
        if(reported_world_effect_gaps.insert(error.what()).second)std::cerr<<"Authored impact visual: "<<error.what()<<'\n';
      }
    }
    for(const auto&event:current.grenade_detonations)if(event.serial>world_detonation_serial){
      world_detonation_serial=event.serial;
      if(predicted_impact_copy(predicted_host_impact(event)))continue;
      try{
        if(!event.source_locations.empty())world_effects.spawn_source(event.effect,event.projectile,event.tick,event.source_locations,event.colors,event.controls,event.replica_render);
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
      if(audio && !local_vehicle_sound_view(sound.owner,false))try{audio->impulse_at(sound.sound,sound.position,sound.gain,
          MultiplayerAudioKind::Impulse,sound.owner);}
      catch(const FormatError &error){
        if(reported_world_effect_gaps.insert(error.what()).second)
          std::cerr<<"Authored world effect audio: "<<error.what()<<'\n';
      }
    world_effect_tick=current.tick;
  };
  auto &object_light_cache=scene_runtime_values.object_light_cache;
  auto &object_light_units=scene_runtime_values.object_light_units;
  auto &corpse_light_cache=scene_runtime_values.corpse_light_cache;
  auto &vehicle_light_cache=scene_runtime_values.vehicle_light_cache;
  auto &vehicle_previous_lods=scene_runtime_values.vehicle_previous_lods;
  auto &object_owner_spheres=scene_runtime_values.object_owner_spheres;
  auto &aim_alive=pose_values.aim_alive;
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
  auto &muzzle_pose_tick=receipt_values.muzzle_pose_tick;
  auto &muzzle_pose_owners=receipt_values.muzzle_pose_owners;
  auto &source_observer_ready=pose_values.source_observer_ready;
  using FpPoseKey=std::array<std::uint32_t,3>; // full owner, render model, WORLD/FP mode
  auto &source_fp_interpolation=pose_values.source_fp_interpolation;
  auto &source_fp_lifetimes=pose_values.source_fp_lifetimes;
  std::unique_ptr<NativeFirstPersonRecipientAssets> native_fp_recipient_assets;
  std::shared_ptr<const SourceRuntimeResources> native_fp_resources;
  std::array<std::unique_ptr<SourceFirstPersonPoseReceiverTu15>,4> native_fp_receivers;
  const auto native_fp_presentation=[&] {
    if(reference_player || options.capture_root)return false;
    // A native stock-game replica must consume its actual recipient packet.
    // The absence of a gameplay World must never choose the retired clip path.
    if(live_client() && rules.requires_source_engine)return true;
    return live_world()->source_objects && live_world()->source_objects->owns_physics();
  };
  const auto prepare_native_fp_recipient=[&] {
    if(!live_client() || !cache.is_mcc() || !cache.original_data() ||
        cache.original_data()->image!=OriginalData::Image::TitleUpdate15 || !rules.requires_source_engine)
      throw FormatError("Joined native FP presentation has no admitted TU1.5/MCC recipient role");
    if(!native_fp_recipient_assets)native_fp_recipient_assets=std::make_unique<NativeFirstPersonRecipientAssets>(
        cache,options.bsp,source_game_variant(variant),rules.random_seed);
    if(!native_fp_resources) {
      const auto &owner=native_fp_recipient_assets->runtime;
      native_fp_resources=std::shared_ptr<const SourceRuntimeResources>(owner,&owner->tu15_world().runtime_resources());
    }
    for(unsigned view=0;view<options.local_players;++view)if(!native_fp_receivers.at(view))
      native_fp_receivers[view]=std::make_unique<SourceFirstPersonPoseReceiverTu15>(cache,native_fp_resources);
  };
  // Complete resource preparation before ONLINE loaded/admission publication,
  // using only the genuine local Cache/role; live packets own later bindings.
  if(live_client() && native_fp_presentation())prepare_native_fp_recipient();
  const auto prepare_native_fp_models=[&](unsigned view,const MultiplayerState::Player &player,
      Vec3 position,Vec3 forward,Vec3 up) {
    if(!live_client()) {
      auto *objects=live_world()->source_objects.get();
      if(!objects || !objects->owns_physics())throw FormatError("Native FP presentation has no admitted authority World");
      return objects->prepare_first_person_models(view,position,forward,up);
    }
    prepare_native_fp_recipient();
    if(!player.first_person_pose)throw FormatError("Joined native FP presentation has no completed recipient input packet");
    const auto &packet=*player.first_person_pose;
    SourceFirstPersonContextIdentityTu15 expected;
    expected.player=player.source_player;expected.unit=player.source_unit;
    expected.unit_definition=biped_for(player).datum;
    expected.representation=source_session_player_representation(player.source_configuration,source_game_type);
    expected.weapons={player.source_weapon,player.secondary_weapon.datum};
    expected.weapon_definitions={player.source_weapon==null_datum?null_datum:player.weapon,player.secondary_weapon.definition};
    if(packet.temporal.source_tick+1u!=current.tick)
      throw FormatError("Joined native FP presentation lost its completed snapshot phase");
    auto &receiver=native_fp_receivers.at(view);
    if(!receiver)throw FormatError("Native FP recipient view was not prepared before admission");
    SourcePhysicsWorldRuntimeServices cpu;
#if defined(__i386__) || defined(__x86_64__)
    cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
        _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
    cpu.timestamp_counter=[](){return __rdtsc();};
#else
    throw FormatError("Native FP presentation requires actual CPU instruction services");
#endif
    auto &source=native_fp_recipient_assets->runtime->tu15_world();
    const auto caller=source.prepare_source_call();
    return receiver->sample(source.memory(),caller,packet,expected,position,forward,up,cpu);
  };
  const auto retain_fp_poses=[&](unsigned view,const auto &binding,const auto &player,
      const std::vector<SourceFirstPersonModelTu15> &models,const Transform &camera) {
    const std::array lifetime{binding.unit,binding.hands[0].weapon,binding.hands[1].weapon,
        player.source_parent,std::uint32_t(player.dead)};
    auto &history=source_fp_interpolation.at(view);
    if(lifetime!=source_fp_lifetimes[view])history.clear();
    source_fp_lifetimes[view]=lifetime;
    std::set<FpPoseKey> present;
    for(const auto &model:models) {
      const FpPoseKey key{model.owner,model.render_model,model.mode};present.insert(key);
      history[key].capture(current.tick,model.nodes,model.mode==1?std::optional{camera}:std::nullopt);
    }
    std::erase_if(history,[&](const auto &entry){return !present.contains(entry.first);});
  };
  const auto native_fp_draws=[&](const FirstPersonViewBinding &view,const MultiplayerState::Player &player,
      const std::vector<SourceFirstPersonModelTu15> &models) {
    SourceFirstPersonDrawBindingTu15 binding;binding.unit=view.unit;
    for(unsigned hand=0;hand<2;++hand)if(view.hands[hand].weapon!=null_datum) {
      auto owner=player;if(hand)owner.weapon=player.secondary_weapon.definition;
      const auto &definition=species_for(owner).fp_definition;
      if(!definition)throw FormatError("Native FP record has no actual authored binding");
      if(binding.arms_model!=null_datum && binding.arms_model!=definition->arms_model)
        throw FormatError("Native FP hands disagree on their representation's shared arms model");
      binding.arms_model=definition->arms_model;binding.weapons[hand]=view.hands[hand].weapon;
      binding.weapon_models[hand]=definition->weapon_model;
    }
    return source_first_person_draws_tu15(models,binding);
  };
  const auto update_muzzle_effects=[&](bool refresh=false) {
    std::vector<std::array<std::uint32_t,3>> owners;
    for(unsigned view=0;view<ids.size();++view) {
      const auto &binding=live_client()?client_fp_views[view]:live_match().first_person_view(view);
      owners.push_back({binding.unit,binding.hands[0].weapon,binding.hands[1].weapon});
    }
    if(!refresh && muzzle_pose_tick==current.tick && owners==muzzle_pose_owners)return;
    muzzle_pose_tick=current.tick;muzzle_pose_owners=std::move(owners);
    std::vector<std::uint32_t> attached_weapons;
    for(unsigned view=0;view<ids.size();++view) {
      const auto &primary=current.players[ids[view]];
      if(!primary.connected || primary.dead || primary.source_unit==null_datum)continue;
      const auto &registered=live_client()?client_fp_views[view]:live_match().first_person_view(view);
      const auto &biped=biped_for(primary);
      const auto camera=options.capture_camera.value_or(player_camera(primary,biped,biped.camera_field_of_view,content.player_screen_offset));
      const auto transform=camera_transform(camera.position,camera.forward,camera.up);
      MuzzleEffectView effects_view{};
      effects_view.position=camera.position;effects_view.forward=camera.forward;effects_view.up=camera.up;
      const bool source_fp=native_fp_presentation();
      if(source_fp && source_particle_world && !source_observer_ready[view]) {
        source_particle_world->render_model_view([&](auto &source) {
          source_particle_draws.publish_observer(source,view,
              camera.position,camera.forward,camera.up,biped.camera_field_of_view);
        });
        source_observer_ready[view]=true;
      }
      const auto source_models=source_fp ? prepare_native_fp_models(
          view,primary,camera.position,camera.forward,camera.up) : std::vector<SourceFirstPersonModelTu15>{};
      if(source_fp)retain_fp_poses(view,registered,primary,source_models,transform);
      const auto native_draws=source_fp?native_fp_draws(registered,primary,source_models):SourceFirstPersonDrawsTu15{};
      Transform primary_adjustment;
      for(unsigned hand_index=0;hand_index<2;++hand_index) {
        const auto &hand=registered.hands[hand_index];
        if(hand.weapon==null_datum)continue;
        // A live registration does not imply a168DED draw: its native flag2
        // can omit either weapon while the shared arms record remains.
        if(source_fp && !native_draws.weapons[hand_index])continue;
        auto player=primary;
        if(hand_index) {
          const auto &secondary=primary.secondary_weapon;
          if(secondary.datum!=hand.weapon)throw FormatError("FP emission hand differs from its authoritative weapon");
          player.weapon=secondary.definition;player.source_weapon=secondary.datum;
          player.loaded=secondary.loaded;player.ammunition_available=secondary.ammunition_available;
          player.reload_tick=secondary.reload_tick;
        }
        auto &visual=species_for(player);const auto &weapon=content.weapons.at(player.weapon);
        if(!primary.dead)if(const auto attached=fp_weapon_attachments.find(player.weapon);attached!=fp_weapon_attachments.end()) {
          const auto &shader=hand_index?primary.secondary_weapon.shader:primary.weapon_shader;
          const auto &trigger=hand_index?primary.secondary_weapon.trigger:primary.weapon_trigger;
          const auto function=[&](std::uint32_t name,float absent) -> FunctionValue {
            if(!name)return {absent,true};
            if(name==0x0f000574u && !weapon.triggers.empty())
              return source_weapon_primary_charge(trigger.mode,trigger.ticks,
                  weapon.triggers[0].charging_seconds,tick_seconds);
            if(name==0x09000568u)return source_weapon_turned_on(shader);
            const auto value=source_weapon_shader_input(name,shader);
            if(!value)throw FormatError("Live FP attachment lost its admitted weapon function");
            return {*value,*value>0.f};
          };
          for(const auto &attachment:attached->second) {
            const auto a=function(attachment.scale_a,0.f),b=function(attachment.scale_b,1.f);
            muzzle_effects.attachment(attachment.effect,hand.weapon,current.tick,{a.value,b.value},a.enabled,attachment.marker_alias,live_world()->source_objects && live_world()->source_objects->owns_physics());
          }
          attached_weapons.push_back(hand.weapon);
        }
        if(!visual.fp_definition || visual.effect_markers.empty())continue;
        if(source_fp && source_models.empty())continue;
        const auto clip_index=hand.animation_identifier;
        // The original manager can choose another authored variant of an
        // action. Decode that actual graph/index, not just the first clip
        // returned by the native action table's preload.
        if(!source_fp && !visual.fp_clips.contains(clip_index)) {
          if(hand.graph!=visual.fp_definition->graph || hand.animation_identifier==null_datum)
            throw FormatError("FP emission clock lost its actual animation graph/index");
          visual.fp_clips.emplace(clip_index,cache.load_animation_identifier(hand.graph,clip_index));
        }
        const auto clip=visual.fp_clips.find(clip_index);
        auto binding=registered;binding.hands[0]=hand;
        auto origin=binding;
        const auto frame=source_fp?hand.frame:first_person_render_frame(origin,binding,*visual.fp_definition,1,false).frame;
        const AnimationClip *ammunition=nullptr;float ammunition_frame{};
        if(!source_fp && visual.fp_definition->ammunition[hand.dual_wield?1:0]) {
          ammunition=&*visual.fp_definition->ammunition[hand.dual_wield?1:0];
          ammunition_frame=first_person_ammunition_frame(std::int16_t(player.loaded),
              std::bit_cast<std::int16_t>(std::uint16_t(player.ammunition_available)),std::int16_t(weapon.capacity),
              std::bit_cast<std::int16_t>(std::uint16_t(current.tick-player.reload_tick)),weapon.weapon_type,
              hand.activity,ammunition->frame_count);
        }
        const float aiming=primary.secondary_weapon.datum==null_datum?source_aim_blend_weight(aim_blends[ids[view]]):0.f;
        FirstPersonPose pose;
        if(source_fp) {
          pose.weapon_nodes=native_draws.weapons[hand_index]->nodes;
        } else pose=pose_first_person_models_world_frame(visual.guns[view*2+hand_index].model,visual.hands.model,
            clip->second,frame,weapon.first_person_offset,transform,visual.fp_ik,aiming,hand_index!=0,
            hand_index?&primary_adjustment:nullptr,ammunition,ammunition_frame);
        if(!hand_index)primary_adjustment=pose.camera_adjustment;
        muzzle_effects.pose(hand.weapon,[&](std::uint32_t name) {
          const auto authored=visual.effect_markers.find(name);
          if(authored==visual.effect_markers.end())throw FormatError("FP emission has no authored marker");
          const auto markers=pose_model_markers(authored->second,pose.weapon_nodes);
          std::vector<Transform> frames;frames.reserve(markers.size());
          //178360 grants only literal root a missing-marker fallback. Other
          //missing groups produce zero locations, preserving the rest of the
          //effect (the authored Magnum muzzle_flash group is one such case).
          if(markers.empty() && cache.string_id(name)=="root")
            frames.push_back(compose_transform(inverse_transform(transform),pose.weapon_nodes.front()));
          for(const auto &marker:markers)frames.push_back(compose_transform(inverse_transform(transform),marker.world));
          return frames;
        },effects_view,current.tick,tick_seconds);
      }
    }
    muzzle_effects.retire_attachments_except(attached_weapons);
    muzzle_effects.advance(current.tick,tick_seconds);
  };
  const auto deliver_muzzle_sounds=[&] {
    for(const auto &sound:muzzle_effects.take_sounds())if(audio && !local_vehicle_sound_view(sound.owner,false))try {
      if(sound.position)audio->impulse_at(sound.cue.sound,*sound.position,sound.cue.gain,MultiplayerAudioKind::Impulse,sound.owner);
      else audio->impulse(sound.cue.sound,sound.cue.gain,MultiplayerAudioKind::Impulse,sound.owner);
    }catch(const std::exception &e){std::cerr<<"Ready effect sound: "<<e.what()<<'\n';}
  };
  EffectPresentationContinuity effect_continuity(muzzle_effects,world_effects,map_effects,combat_attachments,effect_random);
  // Native CPU retention policy under the existing internal migration gate.
  // Count bounded only; memory/catch-up and full adoption still need qualification.
  EffectPresentationHistory effect_history(effect_continuity,effect_sounds,live_match().simulation(),128);
  MultiplayerReceiptContinuity receipt_continuity(
      {receipt_values,current,previous,current_receipt},effect_continuity,effect_sounds,pose_continuity);
  std::unique_ptr<SourceForegroundWorldHandoff> foreground_handoff;
  if(source_migration_resources)
    foreground_handoff=std::make_unique<SourceForegroundWorldHandoff>(cache_owner,mesh,source_particle_draws,
        bsp_effect_lights,source_materials_at_upload,
        SourceForegroundWorldHandoff::Policy{source_particle_resources,source_model_resources},
        source_particle_world,source_model_world);
  struct PreparedForeground {
    PeerMigrationDecision decision;
    SourceForegroundWorldHandoff::Prepared bindings;
    MultiplayerReceiptContinuity::Prepared receipt;
    MultiplayerSceneContinuity::PreparedRestore scene;
    MultiplayerHudContinuity::PreparedRestore hud;
    MultiplayerLocomotionContinuity::PreparedRestore locomotion;
    std::optional<SourceLensPresentationHandoffTu15::Prepared> lens;
    const SourceRuntimeMemory *lens_world{};
  };
  std::optional<PreparedForeground> prepared_foreground;
  std::string foreground_handoff_error;
  // A real partial preparation, kept private. Missing camera,
  // HUD image uploads, physical input, remaining render owners or playing audio still prohibit
  // full Ready/activation. Native/transport publication is not attempted here.
  const auto prepare_foreground_handoff=[&] {
    if(!foreground_handoff||!scene_continuity||!hud_continuity||!locomotion_continuity)return;
    const auto migration=options.online_match->peer_migration();
    if(!migration||!migration->decision){prepared_foreground.reset();return;}
    const auto &decision=*migration->decision;
    const auto checkpoint=authority.common_cut_checkpoint(decision.previous,decision.value.cut);
    if(!checkpoint){prepared_foreground.reset();return;}
    const auto same_preparation=[&] {
      return prepared_foreground&&prepared_foreground->decision==decision&&
          foreground_handoff->ready(prepared_foreground->bindings)&&
          receipt_continuity.ready(prepared_foreground->receipt,
              ValidatedAuthorityCut{options.online_match->self(),decision.previous,decision.value.cut})&&
          scene_continuity->ready(prepared_foreground->scene,
              checkpoint->proof())&&
          hud_continuity->ready(prepared_foreground->hud,checkpoint->proof())&&
          locomotion_continuity->ready(prepared_foreground->locomotion,checkpoint->proof())&&
          (!prepared_foreground->lens||(prepared_foreground->lens_world&&source_lens_queries&&source_lens_draws&&
            SourceLensPresentationHandoffTu15::ready(*source_lens_queries,*source_lens_draws,
              *prepared_foreground->lens,*prepared_foreground->lens_world)));
    };
    if(same_preparation())return;
    prepared_foreground.reset();
    try {
      const bool becoming_host=decision.value.host==options.online_match->self();
      auto *takeover=becoming_host?authority.pending_takeover():nullptr;
      if(becoming_host&&!takeover)return;
      if(!becoming_host&&authority.pending_client_rebind()!=std::optional{decision})return;
      auto &target=becoming_host?takeover->world():authority.world();
      auto bindings=becoming_host?foreground_handoff->prepare_host(*takeover,*checkpoint):
          foreground_handoff->prepare_client(target,*checkpoint);
      auto receipt=receipt_continuity.prepare(*checkpoint,bindings.muzzle_collision(),bindings.world_collision());
      auto scene=scene_continuity->prepare_restore(checkpoint->frontier().scene,checkpoint->proof());
      auto hud=hud_continuity->prepare_restore(checkpoint->frontier().hud,checkpoint->proof());
      auto locomotion=locomotion_continuity->prepare_restore(checkpoint->frontier().locomotion,checkpoint->proof());
      std::optional<SourceLensPresentationHandoffTu15::Prepared> lens;
      const SourceRuntimeMemory *lens_world{};
      if(becoming_host&&(source_lens_queries||source_lens_draws)) {
        if(!source_lens_queries||!source_lens_draws||!target.source_objects)
          throw FormatError("Foreground handoff lacks its paired lens/native owners");
        lens_world=&target.source_objects->memory();
        lens.emplace(SourceLensPresentationHandoffTu15::prepare(*source_lens_queries,*source_lens_draws,*lens_world));
      }
      prepared_foreground.emplace(PreparedForeground{decision,std::move(bindings),std::move(receipt),
          std::move(scene),std::move(hud),std::move(locomotion),std::move(lens),lens_world});
      foreground_handoff_error.clear();
    }catch(const std::exception &error){
      if(foreground_handoff_error!=error.what())std::cerr<<"Foreground handoff preparation unavailable: "<<error.what()<<'\n';
      foreground_handoff_error=error.what();
    }
  };
  std::optional<EffectPresentationHistory::RuntimeCheckpoint> retained_effect_cut;
  std::optional<ValidatedAuthorityCut> attempted_effect_cut;
  std::uint64_t attempted_effect_captures{};
  const LanClient *effect_binding_client{};
  std::optional<PeerCommonCut> reserved_common_cut;
  std::optional<EffectPresentationHistory::RuntimeReservation> common_reservation;
  std::optional<EffectPresentationHistory::RuntimeCandidate> common_candidate;
  std::shared_ptr<PeerSnapshotCandidate> candidate_delivery;
  // A candidate nonce is not a common seal id. Remember this lineage only
  // after the real foreground/native association succeeds.
  std::optional<MultiplayerRuntimeAuthority::SnapshotCandidateCut> common_candidate_cut;
  bool common_history_mode{};
  std::optional<AuthorityTransferContext> common_history_context;
  std::string effect_history_error;
  const auto report_effect_history=[&](const std::exception &error) {
    if(effect_history_error!=error.what())std::cerr<<"Effect cut history unavailable: "<<error.what()<<'\n';
    effect_history_error=error.what();
  };
  const auto bind_effect_history=[&](bool bind=true) {
    const auto context=retain_effect_history?options.online_match->authority_context():std::nullopt;
    effect_history.context(context);
    if(common_history_context!=context){common_history_mode=false;common_history_context=context;}
    if(options.online_match&&options.online_match->snapshot_candidates_enabled())common_history_mode=true;
    if(context){const auto common=options.online_match->common_cut();
      if(common&&(common->pending||common->committed))common_history_mode=true;}
    if(context && live_client() && current_receipt.context()==context && previous_receipt.same(current_receipt) &&
        current_receipt.same(live_client()->state_origin()) && !current_receipt.local_slots().empty())
        try{effect_history.begin_runtime(*live_client());}
        catch(const std::exception &error){report_effect_history(error);}
    if((retained_effect_cut && retained_effect_cut->cut().context!=context) || effect_binding_client!=live_client()){
      retained_effect_cut.reset();attempted_effect_cut.reset();effect_binding_client=live_client();
    }
    if(!bind){
      // The network may negotiate/abort its first common round at any instant.
      // No legacy observer alias may span the quiescent service that ACKs it.
      retained_effect_cut.reset();attempted_effect_cut.reset();return;
    }
    if(!context || !live_client())return;
    if(common_history_mode){
      // Recovery pin lifetime belongs to the common owner alone. A latest-cut
      // observer alias must not keep an aborted foreground alive after ACK.
      retained_effect_cut.reset();attempted_effect_cut.reset();return;
    }
    if(const auto shadow=authority.shadow_state();shadow && shadow->host_proof)try {
      const auto &cut=shadow->host_proof->cut();
      const auto same_cut=[&](const auto &saved){return saved.peer==cut.peer && saved.context==cut.context && saved.seal==cut.seal;};
      if(retained_effect_cut && same_cut(retained_effect_cut->cut()))return;
      const auto captured=effect_history.state().captured;
      if(attempted_effect_cut && same_cut(*attempted_effect_cut) && attempted_effect_captures==captured)return;
      attempted_effect_cut=cut;attempted_effect_captures=captured;
      if(auto bound=effect_history.bind_runtime(*shadow->host_proof,*live_client()))retained_effect_cut=std::move(bound);
    }catch(const std::exception &error){report_effect_history(error);}
    // Historical association only. Readiness and native/GPU/audio activation
    // require the enclosing complete owner transaction; do not advertise here.
  };
  const auto reserve_common_history=[&] {
    if(options.online_match&&options.online_match->snapshot_candidates_enabled()){
      common_reservation.reset();reserved_common_cut.reset();return;
    }
    if(!retain_effect_history||!live_client()){common_reservation.reset();reserved_common_cut.reset();return;}
    const auto agreement=options.online_match->common_cut();
    if(!agreement||!agreement->pending||agreement->abort||
        effect_history.state().context!=agreement->pending->context){
      common_reservation.reset();reserved_common_cut.reset();return;
    }
    if(reserved_common_cut==agreement->pending&&common_reservation&&common_reservation->valid())return;
    common_reservation.reset();reserved_common_cut.reset();
    try{
      common_reservation=effect_history.reserve_runtime(agreement->pending->context,agreement->pending->seal);
      reserved_common_cut=agreement->pending;
    }catch(const std::exception &error){report_effect_history(error);}
  };
  // Only run at the completed-frame boundary. Binding in the receipt observer
  // above is allowed during a frame; native pin publication is not.
  const auto associate_common_history=[&] {
    if(options.online_match&&options.online_match->snapshot_candidates_enabled()){
      if(!common_candidate||!common_candidate->captured()||!live_client())return;
      const auto agreement=options.online_match->common_cut();
      if(!agreement||!agreement->pending||agreement->abort||
          agreement->pending->context!=common_candidate->token().context||
          (agreement->associated&(std::uint16_t(1)<<options.online_match->self())))return;
      const auto pin=authority.pending_native_cut();
      if(!pin||pin->cut().context!=agreement->pending->context||pin->cut().seal!=agreement->pending->seal)return;
      try{
        if(auto checkpoint=effect_history.bind_runtime(pin->host_proof(),*live_client(),*common_candidate)){
          if(authority.associate_common_cut(pin,*checkpoint))
            common_candidate_cut=MultiplayerRuntimeAuthority::SnapshotCandidateCut{
                common_candidate->token(),agreement->pending->seal};
        }
        else options.online_match->reject_common_cut(*agreement->pending);
      }catch(const std::exception &error){
        report_effect_history(error);options.online_match->reject_common_cut(*agreement->pending);
      }
      return;
    }
    if(!reserved_common_cut||!common_reservation||!live_client())return;
    const auto agreement=options.online_match->common_cut();
    if(!agreement||agreement->pending!=reserved_common_cut||agreement->abort||
        (agreement->associated&(std::uint16_t(1)<<options.online_match->self())))return;
    const auto pin=authority.pending_native_cut();
    if(!pin||pin->cut().context!=reserved_common_cut->context||pin->cut().seal!=reserved_common_cut->seal)return;
    try{
      if(auto checkpoint=effect_history.bind_runtime(pin->host_proof(),*live_client(),*common_reservation)){
        authority.associate_common_cut(pin,*checkpoint);
      }else if(const auto history=effect_history.state();common_reservation->overflowed()||
          (history.newest_tick&&*history.newest_tick>reserved_common_cut->seal.tick)){
        // Native evidence cannot manufacture a missed foreground receipt.
        // A full reservation cannot accept a later same-tick candidate either.
        // Abort and retry a later cut while keeping the old committed pin.
        options.online_match->reject_common_cut(*reserved_common_cut);
      }
    }catch(const std::exception &error){report_effect_history(error);}
  };
  const auto observed_effect_cut=[&]() -> std::optional<ValidatedAuthorityCut> {
    if(!common_history_mode)return retained_effect_cut?std::optional{retained_effect_cut->cut()}:std::nullopt;
    const auto common=options.online_match->common_cut();if(!common)return {};
    const auto bound=[&](const PeerCommonCut &cut)->std::optional<ValidatedAuthorityCut>{
      if(const auto checkpoint=authority.common_cut_checkpoint(cut.context,cut.seal))return checkpoint->cut();
      return {};
    };
    if(common->pending&&!common->abort&&(common->associated&(std::uint16_t(1)<<options.online_match->self())))
      if(auto pending=bound(*common->pending))return pending;
    return common->committed?bound(*common->committed):std::nullopt;
  };
  auto &object_light_alive=scene_runtime_values.object_light_alive;
  auto &object_light_reported=scene_runtime_values.object_light_reported;
  std::array<float, 4> yaw{}, pitch{};
  std::array<Controls, 4> local{};
  std::array<Controls, 4> pad_controls{},pad_previous{},pad_pending{};
  std::array<bool,4> pad_zoom_held{},pad_zoom_pending{};
  std::array<bool,4> pad_scoreboard_held{};
  std::array<std::int16_t,4> pad_scoreboard_scroll{};
  std::array<bool,4> death_cycle_pressed{};
  std::array<bool,4> pending_camera_cycle{};
  bool keyboard_jump_held{};
  std::array<std::array<NativeInteractionHold,2>,4> pad_holds{};
  MultiplayerKbm keyboard(SDL_GetWindowID(platform.window), platform.desktop->keyboard_settings);
  PcMouseLook mouse_look(.0025f);
  auto &view_zoom=pose_values.view_zoom;
  auto &view_zoom_unit=pose_values.view_zoom_unit;
  auto &view_zoom_reset=pose_values.view_zoom_reset;
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
  auto &scope_producers=pose_values.scope_producers;
  auto &scope_observers=pose_values.scope_observers;
  std::optional<NativeVehicleCamera> initial_vehicle_camera;
  // Online hosts own the same seat/third-person and death cameras as offline.
  if(live_world()->source_objects && !live_client() && live_world()->source_objects->owns_physics())
    initial_vehicle_camera.emplace(*live_world()->source_objects,*content.original_data);
  NativeVehicleCamera *local_camera_owner{};
  SourceAdmittedCameraBank *local_admitted_camera_owner{};
  std::optional<std::uint32_t> local_camera_tick;
  const auto complete_local_vehicle_cameras=[&] {
    auto *admitted=authority.admitted_cameras();
    auto *camera=authority.promoted_camera();
    if(!camera && initial_vehicle_camera)camera=&*initial_vehicle_camera;
    if(live_client() || (!camera&&!admitted))return;
    if(camera==local_camera_owner && admitted==local_admitted_camera_owner && local_camera_tick==current.tick)return;
    const float seconds=camera==local_camera_owner && admitted==local_admitted_camera_owner && local_camera_tick && current.tick>*local_camera_tick?
        float(current.tick-*local_camera_tick)*tick_seconds:tick_seconds;
    // The original observer consumes completed 30 Hz world/look inputs. Run
    // it once per completed step (including catch-up steps), then let render
    // sampling share the body's endpoints rather than chasing a stepped root
    // once per submitted frame.
    for(unsigned view=0;view<ids.size();++view) {
      auto &player=current.players[ids[view]];
      std::optional<NativeDeathCameraView> completed;
      const auto result=admitted?admitted->update(admitted->context().host,view,seconds,pending_camera_cycle[view],&completed):
          camera->update_player(view,player.source_player,player.source_flags,seconds,pending_camera_cycle[view],&completed);
      const bool death=admitted?admitted->death_view(admitted->context().host,view):camera->death_view(view);
      player.source_player_camera=!death && !result?completed:std::nullopt;
      player.source_vehicle_camera=death?std::nullopt:result;
      player.source_death_camera=death?result:std::nullopt;
      pending_camera_cycle[view]=false;
    }
    local_camera_owner=camera;local_admitted_camera_owner=admitted;local_camera_tick=current.tick;
  };
  std::optional<NativeDeathCamera> death_camera;
  if(live_world()->source_objects && !live_world()->source_objects->owns_physics())
    death_camera.emplace(*live_world()->source_objects,*content.original_data,auxiliary_random);
  if(scope_constants)
    for(auto &observer:scope_observers)observer=source_scope_camera_initial(*scope_constants);
  bool scope_unavailable_reported{};
  auto &dead=pose_values.view_was_dead;
  auto &heard_shots=receipt_values.heard_shots;
  auto &heard_teleports=receipt_values.heard_teleports;
  if(client_state_bound)for(unsigned id=0;id<multiplayer_capacity;++id)
    heard_teleports[id]=current.players[id].source_teleport_serial;
  if(client_state_bound)for(unsigned id=0;id<multiplayer_capacity;++id)heard_shots[id]=current.players[id].shot_tick;
  for (unsigned i = 0; i < ids.size(); ++i) {
    yaw[i] = current.players[ids[i]].yaw;
    pitch[i] = current.players[ids[i]].pitch;
  }
  const auto source_relative_look=[&] {
    if(!live_client())return live_world()->source_objects && live_world()->source_objects->owns_physics();
    return current.source_body_poses_authoritative && content.original_data &&
      content.original_data->image==OriginalData::Image::TitleUpdate15;
  };
  bool running = true;
  auto &joined=receipt_values.joined;
  joined=!live_client() || live_client()->ready();
  auto exit_reason = MultiplayerExitReason::frame_limit;
  MaterialFrameSelection material_frame(mesh.materials);
  std::vector<int> player_armor_owners(mesh.materials.size(),-1);
  std::vector<int> player_damage_owners(mesh.materials.size(),-1);
  std::map<std::uint32_t,std::vector<ObjectFunction>> player_damage_functions;
  const auto player_damage_function_tables=cache.shader_function_tables();
  const auto player_damage_names=DamageFeedbackNames::from_cache(cache);
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
  struct ScriptedEffectInput {bool fire{},reload{},grenade{};std::optional<Vec2> look;Vec2 move;bool switch_grenade{},jump{},crouch{};};
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
        while(row>>extra){
          if(extra=="switch-grenade" && !input.switch_grenade)input.switch_grenade=true;
          else if(extra=="jump" && !input.jump)input.jump=true;
          else if(extra=="crouch" && !input.crouch)input.crouch=true;
          else if(extra=="move") {
            if(!(row>>input.move.x>>input.move.y) || !std::isfinite(input.move.x) || !std::isfinite(input.move.y) ||
               std::abs(input.move.x)>1.f || std::abs(input.move.y)>1.f)
              throw FormatError("Invalid scripted movement input");
          }else throw FormatError("Extra multiplayer input-script columns");
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
  auto &peer_sound_classes=receipt_values.peer_sound_classes;
  auto &peer_game_over_fade_started=receipt_values.peer_game_over_fade_started;
  // Retain staging capacity; rebuild every frame from the current source state.
  std::vector<Vertex> hud_vertices;
  std::vector<std::vector<Vertex>> worlds(options.local_players),
      foregrounds(options.local_players),backgrounds(options.local_players),
      reflection_backgrounds(options.local_players);
  std::vector<std::vector<SourceWeatherPointDraw>> weather_points(options.local_players);
  std::vector<std::vector<SourceWeatherLineDraw>> weather_lines(options.local_players);
  std::vector<std::vector<SourceWeatherPlateDraw>> weather_plates(options.local_players);
  // The map's resources are resident. The dialog's admitted-resource frame on
  // the new device reads network_joining until the first gameplay draw.
  platform.desktop->network_transition_status(FrontendNetworkTransitionStatus::SettingUpGame);
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
  unsigned keyboard_view=options.local_players?0u:~0u;
  if(!options.input_instances.empty()) {
    keyboard_view=~0u;
    for(unsigned view=0;view<options.input_instances.size();++view)if(options.input_instances[view]==-1) {
      if(keyboard_view!=~0u)throw FormatError("Local input bindings duplicate the keyboard");
      keyboard_view=view;
    }
  }
  std::array<SDL_JoystickID,4> local_pad_instances{-1,-1,-1,-1};
  std::array<bool,4> controller_views{};
  for(unsigned view=0;view<options.local_players;++view) {
    controller_views[view]=options.input_instances.empty()?(options.local_players==1 || view>0):options.input_instances[view]>=0;
    if(!options.input_instances.empty()) {
      local_pad_instances[view]=options.input_instances[view];
    } else {
      const int index=options.local_players==1?0:int(view)-1;
      if(index>=0 && unsigned(index)<platform.pads.size())
        local_pad_instances[view]=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(platform.pads[index]));
    }
  }
  const auto selected_pad_for=[&](unsigned view)->SDL_GameController* {
    for(auto *pad:platform.pads)
      if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==local_pad_instances[view])return pad;
    return nullptr;
  };
  std::array<RumblePlayer,4> rumble;
  std::array<PlayerScreenFlashPlayer,4> screen_flashes;
  std::uint64_t screen_flash_tick=current.tick;
  std::map<std::uint32_t,std::vector<PlayerScreenResponse>> screen_responses;
  std::map<std::uint32_t,std::shared_ptr<const RumbleDefinition>> rumble_definitions;
  const auto rumble_tables=cache.shader_function_tables();
  auto &rumble_serial=receipt_values.rumble_serial;
  rumble_serial=live_client() && !current.rumble_events.empty()?current.rumble_events.back().serial:0;
  const auto rumble_events=[&](const MultiplayerState &state) {
    const auto advance_flashes=[&](std::uint64_t tick) {
      if(tick<screen_flash_tick)return;
      for(auto &flash:screen_flashes)flash.advance(tick-screen_flash_tick);
      screen_flash_tick=tick;
    };
    for(const auto &row:state.rumble_events)if(row.serial>rumble_serial) {
      rumble_serial=row.serial;
      advance_flashes(row.tick);
      for(unsigned view=0;view<ids.size();++view)if(ids[view]==unsigned(row.event.owner+1)) {
        auto &definition=rumble_definitions[row.event.definition];
        if(!definition)definition=std::make_shared<const RumbleDefinition>(load_rumble_definition(cache,row.event.definition));
        rumble[view].play(definition,row.event.shielded,row.event.scale);
        auto [responses,inserted]=screen_responses.try_emplace(row.event.definition);
        if(inserted)responses->second=load_player_screen_responses(cache,row.event.definition);
        // Skip an already expired response if a newly bound journal predates
        // this presentation clock. Ordinary packet gaps retain exact hit age.
        if(row.tick>=screen_flash_tick)
          screen_flashes[view].play(responses->second,row.event.shielded,row.event.seated,row.event.flash_scale,*rumble_tables);
      }
    }
    advance_flashes(state.tick);
  };
  PlayerPromptDevices prompt_devices;
  for(unsigned i=0;i<options.local_players;++i)prompt_devices.bind(i,selected_pad_for(i));
  bool voice_caption{};
  auto announce_voice=[&](std::string text,Clock::time_point at) {
    voice_notice=std::move(text);voice_notice_until=at+std::chrono::seconds(3);
    SDL_SetWindowTitle(platform.window,(game_window_title+" | "+voice_notice).c_str());voice_caption=true;
    std::cout<<voice_notice<<'\n';
  };
  std::string voice_error;
  bool postgame_published{};
  std::map<std::uint32_t,ModelWorldPoseCache> retained_world_weapon_poses;
  bool handoff_paused{};
  std::optional<Uint32> handoff_input_paused_at;
  std::array<SDL_JoystickID,4> handoff_pad_instances{-1,-1,-1,-1};
  const auto suspend_handoff_input=[&] {
    if(handoff_input_paused_at)return;
    handoff_input_paused_at=SDL_GetTicks();keyboard.suspend(*handoff_input_paused_at);
    for(unsigned view=0;view<options.local_players;++view){
      auto *pad=selected_pad_for(view);
      handoff_pad_instances[view]=pad?SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)):-1;
    }
    // These device-edge queues have not been delivered to the native world.
    // The admitted client's serialized bank independently owns carried input.
    pad_pending={};pad_controls={};pad_zoom_pending={};pad_scoreboard_held={};death_cycle_pressed={};pending_camera_cycle={};local={};
  };
  const auto resume_handoff_input=[&] {
    if(!handoff_input_paused_at)return;
    const auto now=SDL_GetTicks(),elapsed=now-*handoff_input_paused_at;
    for(unsigned view=0;view<options.local_players;++view){
      auto *pad=selected_pad_for(view);
      const auto instance=pad?SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)):-1;
      if(instance==handoff_pad_instances[view])for(auto &hold:pad_holds[view])hold.exclude_elapsed(elapsed);
      else{pad_holds[view]={};pad_previous[view]={};pad_zoom_held[view]=false;}
    }
    keyboard.resume(now);handoff_input_paused_at.reset();
    auto_center={};prior_input_pitch=pitch;
    int discard_x,discard_y;SDL_GetRelativeMouseState(&discard_x,&discard_y);
  };
  const auto capture_runtime_frontier=[&](const MultiplayerSnapshotOrigin &origin,std::span<const SoundEvent> pending_sounds,
      MultiplayerFrameTiming::ReceiptCapture *observation){
    MultiplayerRuntimeReceiptFrontier frontier;
    {ObservedReceiptWork work(observation?&observation->values:nullptr);
      static_cast<MultiplayerRuntimeReceiptValues &>(frontier)=receipt_values;}
    {ObservedReceiptWork work(observation?&observation->replica:nullptr);
      frontier.replica=live_match().replica_checkpoint(origin);}
    {ObservedReceiptWork work(observation?&observation->pose:nullptr);
      frontier.pose=pose_continuity.capture(origin);}
    if(scene_continuity){ObservedReceiptWork work(observation?&observation->scene:nullptr);
      frontier.scene=scene_continuity->capture(origin);}
    if(hud_continuity){ObservedReceiptWork work(observation?&observation->hud:nullptr);
      frontier.hud=hud_continuity->capture(origin);}
    if(locomotion_continuity){ObservedReceiptWork work(observation?&observation->locomotion:nullptr);
      frontier.locomotion=locomotion_continuity->capture(origin);}
    {ObservedReceiptWork work(observation?&observation->sounds:nullptr);
      frontier.sounds.assign(pending_sounds.begin(),pending_sounds.end());}
    return frontier;
  };
  // Release ONLINE admission only after all resident resources, the loading
  // renderer handoff and local input bindings have completed. Keep the actual
  // admission bodies below; the first main receive may have no newer packet.
  if(options.online_match) {
    options.online_match->loaded(fingerprint,variant);
    const auto deadline=Clock::now()+std::chrono::seconds(185);
    std::cout<<"ONLINE waiting for matched players to load"<<std::endl;
    while(options.online_match->phase()!=OnlineMatchSession::Phase::Running) {
      if(options.online_match->phase()==OnlineMatchSession::Phase::Failed || Clock::now()>deadline) {
        retain_outcome(MultiplayerExitReason::online_disconnected,live_match().state(),false);return 0;
      }
      SDL_Event event;while(SDL_PollEvent(&event)) {
        if(event.type==SDL_QUIT){retain_outcome(MultiplayerExitReason::window_quit,live_match().state(),false);return 0;}
        if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){retain_outcome(MultiplayerExitReason::escape,live_match().state(),false);return 0;}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const auto admission_deadline=Clock::now()+std::chrono::seconds(15);
    std::array<Controls,4> neutral{};
    // No world ticks before every real participant has been admitted.
    for(;;) {
      if(live_host()) { live_host()->receive(controls);authority.admit_players();live_host()->broadcast(); }
      if(live_client()) { live_client()->send(std::span(neutral).first(options.local_players));live_client()->receive();authority.receive(); }
      const auto admission_state=live_client()?live_client()->state():live_match().state();
      const bool admitted=(!live_client() || live_client()->ready()) &&
          std::count_if(admission_state.players.begin(),admission_state.players.end(),[](const auto &p){return p.connected;})==int(options.online_match->roster().size());
      if(admitted)break;
      if(options.online_match->phase()==OnlineMatchSession::Phase::Failed || Clock::now()>admission_deadline || (live_client() && !live_client()->error().empty())) {
        retain_outcome(MultiplayerExitReason::online_disconnected,live_client()?live_client()->state():live_match().state(),live_client() && live_client()->ready());return 0;
      }
      SDL_Event event;while(SDL_PollEvent(&event)) {
        if(event.type==SDL_QUIT){retain_outcome(MultiplayerExitReason::window_quit,live_match().state(),false);return 0;}
        if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){retain_outcome(MultiplayerExitReason::escape,live_match().state(),false);return 0;}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(options.online_match->terminal_recovery_enabled()) {
      std::cout<<"ONLINE validating initial paired source checkpoint before gameplay"<<std::endl;
      const auto bootstrap_began=Clock::now();auto heartbeat=Clock::time_point{};
      for(;;) {
        const auto now=Clock::now();
        auto bootstrap_frame=authority.begin_frame();
        if(live_host())live_host()->receive(controls);
        if(now>=heartbeat) {
          // These are ordinary authenticated native snapshot/input packets.
          // They keep the existing ten-second connection liveness owner fed
          // while private source validation runs, without advancing its world.
          if(live_host())live_host()->broadcast();
          if(live_client())live_client()->send(std::span(neutral).first(options.local_players));
          heartbeat=now+std::chrono::milliseconds(100);
        }
        if(live_client()){
          live_client()->receive();
          queue_terminal_snapshots(live_client()->received_states(),live_client()->received_origins());
        }
        const auto admission_state=live_client()?live_client()->state():live_match().state();
        if(bootstrap_frame){bootstrap_frame->finish();bootstrap_frame.reset();}
        authority.service();
        if(options.source_recovery_observer)if(const auto observed=authority.shadow_state())options.source_recovery_observer(*observed);
        const bool seats_intact=std::count_if(admission_state.players.begin(),admission_state.players.end(),
            [](const auto &p){return p.connected;})==int(options.online_match->roster().size());
        if(options.online_match->phase()==OnlineMatchSession::Phase::Failed || now>deadline || !seats_intact ||
            (live_client()&&!live_client()->error().empty())) {
          std::cerr<<"ONLINE initial source checkpoint admission failed before gameplay"<<std::endl;
          retain_outcome(MultiplayerExitReason::online_disconnected,admission_state,live_client()&&live_client()->ready());return 0;
        }
        if(authority.terminal_bootstrap_ready()){
          if(!live_client())break;
          const auto endpoint=authority.terminal_endpoint();
          const auto found=std::find_if(terminal_snapshots.begin(),terminal_snapshots.end(),
              [&](const auto &snapshot){return terminal_match(endpoint,snapshot);});
          if(found!=terminal_snapshots.end()){
            authority.adopt_terminal_endpoint(endpoint,*found);
            terminal_bootstrap_snapshot=*found;
            const auto count=std::size_t(std::distance(terminal_snapshots.begin(),found))+1;
            for(std::size_t i=0;i<count;++i)drop_terminal_snapshot();
            ++terminal_adoptions;break;
          }
        }
        SDL_Event event;while(SDL_PollEvent(&event)) {
          if(event.type==SDL_QUIT){retain_outcome(MultiplayerExitReason::window_quit,admission_state,false);return 0;}
          if(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_ESCAPE){retain_outcome(MultiplayerExitReason::escape,admission_state,false);return 0;}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      std::cout<<"ONLINE initial paired source checkpoint validated before first gameplay tick; validation_ms="
          <<std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-bootstrap_began).count()<<" loading_deadline_seconds=185"<<std::endl;
    }
    online_gameplay_started=true;
    std::cout<<"ONLINE match started with "<<options.online_match->roster().size()<<" real players"<<std::endl;
  }
  std::unique_ptr<MultiplayerRuntimeAuthority::Frame> initial_source_frame;
  if(options.online_match&&options.online_match->terminal_recovery_enabled())initial_source_frame=authority.begin_frame();
  if(options.online_match) {
    client_state_bound=live_client() && live_client()->ready();
    current=client_state_bound?live_client()->state():live_match().state();
    previous=current;
    current_receipt=live_client()?live_client()->state_origin():MultiplayerSnapshotOrigin{};
    if(terminal_bootstrap_snapshot){
      current=terminal_bootstrap_snapshot->accepted_state();previous=current;
      current_receipt=*terminal_bootstrap_snapshot;
      terminal_display_origin=current_receipt;terminal_display_fade=gameplay_fade;
      terminal_bootstrap_snapshot.reset();
    }
    previous_receipt=current_receipt;
    if(options.capture_tick && current.tick>options.capture_tick)
      throw FormatError("Capture tick precedes the current authoritative world tick");
    if(live_client() && live_client()->ready())
      ids.assign(live_client()->slots().begin(),live_client()->slots().end());
    joined=!live_client() || live_client()->ready();
    update_audio_listeners();
    scene_interpolation.reset(current.tick,live_match().simulation().scene().objects());
    capture_source_body_poses(true);
    update_sensors();
    synchronize_zoom();
    if(live_world()->source_objects && !live_client() && live_world()->source_objects->owns_physics())
      initial_vehicle_camera.emplace(*live_world()->source_objects,*content.original_data);
    if(client_state_bound)for(unsigned id=0;id<multiplayer_capacity;++id)
      heard_shots[id]=current.players[id].shot_tick;
    for(unsigned view=0;view<ids.size();++view) {
      yaw[view]=current.players[ids[view]].yaw;
      pitch[view]=current.players[ids[view]].pitch;
    }
    rumble_serial=live_client() && !current.rumble_events.empty()?current.rumble_events.back().serial:0;
    screen_flash_tick=current.tick;
    for(auto &flash:screen_flashes)flash.clear();
    // Loading and admission do not advance operating frame/audio/voice clocks.
    last=snapshot_time=Clock::now();accumulator=0;
    voice_credit_at=voice_preferences_at=voice_notice_until=last;
  }
  if(initial_source_frame){initial_source_frame->finish();initial_source_frame.reset();}
  // A receive owns a bounded batch. Finish each authentic body before
  // admitting another batch, and return to real input/render work between
  // completed bodies. The origin is an owning identity witness even for LAN
  // receipts that do not carry a continuity-bound decoded state.
  struct PendingReplicaBatch {
    const LanClient *client{};
    std::size_t next{},count{};
    MultiplayerSnapshotOrigin tail;
    std::optional<AuthorityTransferContext> context;
  };
  std::optional<PendingReplicaBatch> pending_replica_batch;
  const bool paced_replica_batches=options.online_match&&options.online_match->snapshot_candidates_enabled();
  const auto service_snapshot_candidate=[&]{
    if(!options.online_match||!options.online_match->snapshot_candidates_enabled()||!live_client()||pending_replica_batch)return;
    if(!candidate_delivery)candidate_delivery=options.online_match->snapshot_candidates();
    if(!candidate_delivery)return;
    const auto state=candidate_delivery->state();
    if(!state.pending)return;
    if(state.closing){
      if(common_candidate){
        if(effect_history.retire_runtime_candidate(*common_candidate)){
          common_candidate_cut.reset();common_candidate.reset();candidate_delivery.reset();
        }
      }else{
        // No foreground owner/fence was installed for this aborted Arm.
        // Retire only the module's real unchecked-out delivery, if present.
        std::optional<PeerSnapshotCandidate::Delivery> none;
        if(candidate_delivery->acknowledge_retired(*state.pending,std::move(none)))candidate_delivery.reset();
      }
      return;
    }
    if(common_candidate)return;
    const auto history=effect_history.state();
    if(history.context!=state.pending->context)return;
    // Binding continuity does not certify the already-decoded admission
    // receipt. Wait for a real completed history entry before attempting Arm;
    // ordinary input/receive/projection continues without an adoption fence.
    if(!history.retained)return;
    try{common_candidate=effect_history.arm_runtime(*live_client(),candidate_delivery,*state.pending);}
    catch(const std::exception &error){report_effect_history(error);candidate_delivery->reject(*state.pending);}
  };
  // Prepared collision/lens bindings borrow a staged native target. Retire
  // them on this executor before the authority can destroy that target or
  // acknowledge Abort. Registration is scoped after every captured holder.
  auto retire_outer_handoff=[&](const MultiplayerRuntimeAuthority::OuterRetirement &retirement)noexcept{
    const bool whole_context=retirement.reason==MultiplayerRuntimeAuthority::OuterRetirement::Reason::ContextRetired;
    const auto matches=[&](const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
      return context==retirement.context&&(whole_context||seal==retirement.seal);
    };
    if(prepared_foreground&&matches(prepared_foreground->decision.previous,prepared_foreground->decision.value.cut))
      prepared_foreground.reset();
    if(retained_effect_cut&&matches(retained_effect_cut->cut().context,retained_effect_cut->cut().seal))
      retained_effect_cut.reset();
    if(attempted_effect_cut&&matches(attempted_effect_cut->context,attempted_effect_cut->seal))attempted_effect_cut.reset();
    if(reserved_common_cut&&matches(reserved_common_cut->context,reserved_common_cut->seal)){
      common_reservation.reset();reserved_common_cut.reset();
    }
    if(common_candidate_cut&&matches(common_candidate_cut->token.context,common_candidate_cut->seal))
      common_candidate_cut.reset();
    // The provisional candidate owns copied history and a separate delivery
    // lease. Its actual retirement requires SnapshotClosing, which follows
    // CommonAborted; waiting for it here would deadlock that handshake.
    return true;
  };
  auto outer_retirement=authority.hold_outer_retirement_gate(
      [](void *owner,const MultiplayerRuntimeAuthority::OuterRetirement &retirement)noexcept{
        return (*static_cast<decltype(retire_outer_handoff) *>(owner))(retirement);
      },&retire_outer_handoff);
  std::uint64_t observed_loops{},observed_input_callbacks{};
  // Frame-advance state shared by the tick block (advance) and preparation.
  // With render overlap the tick block runs for the next frame while the
  // renderer draws this one, so these outlive one iteration.
  enum class Advance{proceed,skip,stop};
  Clock::time_point now{};auto measured_at=now;
  const auto measure=[&](double &value) {
    if(measure_host_timings) {
      const auto end=Clock::now();value+=std::chrono::duration<double,std::milli>(end-measured_at).count();measured_at=end;
    }
  };
  // Native callbacks can consume host FP status and errno. The new nested
  // timers use integer nanoseconds only; preserve errno around each clock
  // read and leave the actual call and its ambient environment untouched.
  const auto host_stamp=[&]()noexcept {
    static_assert(std::is_integral_v<Clock::duration::rep>);
    if(!measure_host_timings)return Clock::time_point{};
    const auto saved_errno=errno;const auto stamp=Clock::now();errno=saved_errno;
    return stamp;
  };
  const auto host_returned=[&](Clock::time_point begin,
      RendererHostPreparationTiming::ReturnedCall &work)noexcept {
    if(measure_host_timings) {
      work.nanoseconds+=std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(host_stamp()-begin).count());
      ++work.calls;
    }
  };
  float audio_elapsed=0,elapsed=0,blend=0,animation_blend=0;double pose_tick=0;
  // Non-host adaptive interpolation buffer (NETCODE_ENHANCED_GOAL item 3a).
  // Balanced/Full: accepted states wait in replica_playout and the playout
  // clock releases them into the ordinary projection; the frame presents
  // (and every input reports through LanClient::set_view) that clock's time.
  // Original keeps the fixed delay byte for byte, as do the peer-migration
  // qualification sessions (retained effect history, paced candidates).
  AdaptiveInterpolationClock interpolation_clock;
  std::deque<std::pair<MultiplayerState,MultiplayerSnapshotOrigin>> replica_playout;
  std::vector<MultiplayerState> playout_states;
  std::vector<MultiplayerSnapshotOrigin> playout_origins;
  bool replica_playout_active=false;
  auto playout_report_at=Clock::now();
  static const bool playout_stats=std::getenv("H2_NET_STATS")!=nullptr;
  // H2_NET_INTERP=fixed keeps the fixed delay in every mode (diagnostics).
  static const bool playout_fixed=[]{const auto *v=std::getenv("H2_NET_INTERP");return v && std::string_view(v)=="fixed";}();
  // The newest accepted state; the presented `current` lags it while buffering.
  const auto newest_replica=[&]()->const MultiplayerState& {
    return replica_playout_active && live_client()?live_client()->state():current;
  };
  const auto replica_playout_blend=[&] {
    const double span=double(current.tick)-double(previous.tick);
    return span>0?float(std::clamp((interpolation_clock.playback()-double(previous.tick))/span,0.0,1.0)):1.f;
  };
  // H2_NET_STATS: the presentation delay, next to the host's stream lines.
  const auto report_playout=[&](double at) {
    if(!playout_stats || now-playout_report_at<std::chrono::seconds(5))return;
    playout_report_at=now;
    const auto mode=live_client()->ready()?live_client()->state().netcode_mode:NetcodeMode::original;
    if(!replica_playout_active) {
      std::fprintf(stderr,"H2 net view: interpolation delay fixed (one tick, %.1f ms), mode %s\n",
          double(tick_seconds)*1000,netcode_mode_name(mode));
      return;
    }
    const auto s=interpolation_clock.statistics(at);
    const auto &impacts=predicted_impact_ledger.statistics();
    std::fprintf(stderr,"H2 net view: interpolation delay %.1f ms (target %.1f, jitter %.1f, peak %.1f, deficit %.1f, "
        "frame %.1f ms), underruns %llu (%.0f ms held), snaps %llu, buffered %zu, mode %s; predicted impacts %llu, "
        "host copies suppressed %llu, expired %llu\n",
        s.delay*1000,s.target*1000,s.jitter*1000,s.peak*1000,s.deficit*1000,s.frame*1000,
        (unsigned long long)s.underruns,s.underrun_seconds*1000,(unsigned long long)s.snaps,replica_playout.size(),
        netcode_mode_name(mode),(unsigned long long)impacts.predicted,(unsigned long long)impacts.suppressed,
        (unsigned long long)impacts.expired);
  };
  const auto release_terminal_snapshots=[&](std::span<const MultiplayerState> &states,
      std::span<const MultiplayerSnapshotOrigin> &origins)->bool {
    if(!terminal_certified_playout)return false;
    // Reconcile newly delivered movement once, while preserving every source
    // event/state in the owning queue until a real endpoint certifies its tail.
    auto *movement=ensure_nonhost_movement();const auto inputs=live_client()->received_inputs();
    for(std::size_t i=0;i<states.size();++i)if(movement&&i<inputs.size())
      for(unsigned view=0;view<ids.size();++view)
        movement->reconcile(view,states[i].players[ids[view]],inputs[i],live_match().simulation().collision());
    queue_terminal_snapshots(states,origins);
    playout_states.clear();playout_origins.clear();terminal_projection_endpoint.reset();
    const auto endpoint=authority.terminal_endpoint();
    const auto found=std::find_if(terminal_snapshots.begin(),terminal_snapshots.end(),
        [&](const auto &snapshot){return terminal_match(endpoint,snapshot);});
    if(found!=terminal_snapshots.end()){
      const auto count=std::size_t(std::distance(terminal_snapshots.begin(),found))+1;
      for(std::size_t i=0;i<count;++i){
        playout_states.push_back(terminal_snapshots.front().accepted_state());
        playout_origins.push_back(terminal_snapshots.front());drop_terminal_snapshot();
      }
      terminal_projection_endpoint=endpoint;
      terminal_lag_peak=std::max<std::uint64_t>(terminal_lag_peak,
          live_client()->state().tick-playout_states.back().tick);
    }
    states=playout_states;origins=playout_origins;return true;
  };
  // Buffer this receive's accepted states and substitute the bank of those
  // the playout clock releases now (all of them when buffering stops). True
  // when the bank was substituted.
  const auto release_replica_playout=[&](std::span<const MultiplayerState> &states,
      std::span<const MultiplayerSnapshotOrigin> &origins)->bool {
    const double at=replica_seconds(now);
    const bool adaptive=live_client()->ready() && client_state_bound &&
        adaptive_interpolation_enabled(live_client()->state().netcode_mode) &&
        !paced_replica_batches && !retain_effect_history && !playout_fixed &&
        (!options.online_match || options.online_match->phase()==OnlineMatchSession::Phase::Running);
    if(!adaptive && replica_playout.empty()) {
      if(replica_playout_active){replica_playout_active=false;interpolation_clock.reset();}
      report_playout(at);
      return false;
    }
    // Movement prediction reconciles at receipt, in delivery order.
    auto *movement=ensure_nonhost_movement();
    const auto inputs=live_client()->received_inputs();
    for(std::size_t index=0;index<states.size();++index) {
      if(movement && index<inputs.size())
        for(unsigned view=0;view<ids.size();++view)
          movement->reconcile(view,states[index].players[ids[view]],inputs[index],live_match().simulation().collision());
      if(adaptive)interpolation_clock.receive(states[index].tick,at);
      replica_playout.emplace_back(states[index],origins[index]);
    }
    if(!adaptive)interpolation_clock.reset();
    replica_playout_active=adaptive;
    const double playback=adaptive?interpolation_clock.advance(at):0.0;
    playout_states.clear();playout_origins.clear();
    auto presented=current.tick;
    while(!replica_playout.empty()) {
      auto &[state,origin]=replica_playout.front();
      // Superseded by a direct adoption (the online result path).
      if(state.tick<presented){replica_playout.pop_front();continue;}
      // The next endpoint is released once playback reaches the current one.
      if(adaptive && state.tick!=presented && playback<double(presented))break;
      presented=state.tick;
      playout_states.push_back(std::move(state));playout_origins.push_back(std::move(origin));
      replica_playout.pop_front();
    }
    states=playout_states;origins=playout_origins;
    report_playout(at);
    return true;
  };
  // One delivered input per local view: the press gate of predicted impacts.
  const auto predicted_shot_input=[&] {
    if(!live_client() || !live_client()->ready())return;
    const auto &newest=live_client()->state();
    if(!predicted_impacts_enabled(newest.netcode_mode))return;
    const double at=replica_seconds(now);
    for(unsigned view=0;view<ids.size() && view<predicted_shot_gates.size();++view) {
      const auto &player=newest.players[ids[view]];
      const auto found=content.weapons.find(player.weapon);
      const auto weapon=found==content.weapons.end()?std::nullopt:predicted_shot_weapon(found->second);
      const auto &c=local[view];
      predicted_shot_gates[view].input(at,{c.fire,c.reload,c.melee,c.grenade,c.switch_weapon||c.pickup||c.dual_wield},
          predicted_shot_player(player,newest.tick),weapon?&*weapon:nullptr);
    }
  };
  // This view's rounds due this frame, traced from the camera this frame
  // draws (predicted look and position) and spawned at once. Bodies on the
  // line of fire suppress the prediction: the host would hit them instead.
  const auto predict_view_impacts=[&](unsigned view,const MultiplayerState::Player &p,
      const BipedDefinition &biped,const MultiplayerState &shown) {
    if(view>=predicted_shot_gates.size() || view>=ids.size())return;
    const double at=replica_seconds(now);
    const auto rounds=predicted_shot_gates[view].take_due(at);
    const auto weapon=content.weapons.find(p.weapon);
    if(rounds.empty() || weapon==content.weapons.end() || p.dead || p.source_parent!=null_datum)return;
    if(!predicted_impact_tracer)predicted_impact_tracer.emplace(cache,content,surface_lighting);
    std::vector<PredictedImpactOccluder> bodies;
    for(unsigned id=0;id<multiplayer_capacity;++id) {
      const auto &other=shown.players[id];
      if(other.connected && !other.dead && id!=ids[view])bodies.push_back({other.position,other.crouch>.5f?.55f:.75f,.35f});
    }
    for(const auto &vehicle:shown.world_vehicles)bodies.push_back({vehicle.transform.position,0.f,2.f});
    const auto origin=ordinary_unit_eye(p.position,biped.camera_height,biped.crouch_camera_height,p.crouch);
    for(const auto &round:rounds) {
      if(round.weapon!=p.weapon)continue;
      const auto predicted=predicted_impact_tracer->trace(weapon->second,origin,direction(p.yaw,p.pitch),round.range,
          bodies,(world_effect_tick?std::uint64_t(*world_effect_tick):std::uint64_t(current.tick))+1,predicted_impact_random);
      if(!predicted)continue;
      try{world_effects.impact(predicted->impact);}
      catch(const FormatError &error){
        if(reported_world_effect_gaps.insert(error.what()).second)std::cerr<<"Predicted impact visual: "<<error.what()<<'\n';
        continue;
      }
      predicted_impact_ledger.predict({round.player,round.unit,round.weapon,round.projectile,round.tick_floor,at,
          predicted->impact.position,predicted->distance,predicted->impact.material,predicted->effects});
    }
  };
  decltype(current.tick) host_tick_begin{};
  std::unique_ptr<MultiplayerRuntimeAuthority::Frame> source_frame;
  bool ticked_ahead=false,skip_next=false,stop_next=false;
  MultiplayerFrameTiming pending_timing;
  // Overlap only where every borrower is known: no observers, no network
  // session (its frame recording would publish before the draw), and not
  // HALO2_REFERENCE_SYNC_RENDER. The choice is constant for the run.
  static const bool reference_sync_render=[]{const auto *p=std::getenv("HALO2_REFERENCE_SYNC_RENDER");return p && *p;}();
  const bool overlap_allowed=!reference_sync_render && !options.online_match && !live_client() && !options.animation_observer && !options.audio_observer && !options.bsp_selection_observer && !options.bsp_submission_observer && !options.camouflage_layer_observer && !options.completed_bsp_draw_observer && !options.completed_material_upload_observer && !options.dynamic_draw_observer && !options.effect_history_observer && !options.effect_light_command_observer && !options.effect_observer && !options.frame_observer && !options.frame_capture && !options.grenade_draw_observer && !options.input_observer && !options.node_observer && !options.model_skin_gpu_owner_observer && !options.objective_audio_observer && !options.objective_goal_draw_observer && !options.objective_waypoint_draw_observer && !options.player_armor_observer && !options.prop_draw_observer && !options.prop_source_draw_observer && !options.reference_live_shadow_gpu_observer && !options.replica_observer && !options.scoreboard_observer && !options.shadow_admission_observer && !options.shadow_root_frame_observer && !options.shadow_selection_observer && !options.shadow_synchronization_observer && !options.shield_layer_observer && !options.source_dynamic_observer && !options.source_particle_phase_observer && !options.source_scene_observer && !options.unit_source_draw_observer && !options.vehicle_draw_observer && !options.vehicle_projection_observer && !options.view_observer && !options.view_rectangle_observer && !options.view_state_observer && !options.weapon_contrail_observer && !options.weapon_light_volume_observer && !options.weapon_material_observer && !options.weapon_projectile_draw_observer && !options.world_portal_observer && !options.world_weapon_shader_input_observer && !options.world_weapon_source_draw_observer;
  std::cerr<<"Render overlap: "<<(overlap_allowed?"enabled":"disabled")<<'\n';
  while (running && (!options.frames || frames < options.frames)) {
    MultiplayerFrameTiming timing;
    // The tick block. With render overlap it also runs for the next frame
    // while the renderer draws this one (see the draw call below).
    const auto advance=[&](MultiplayerFrameTiming &timing)->Advance {
    // The previous iteration's authority frame ended at the iteration
    // boundary before; keep that order ahead of publication service.
    source_frame.reset();
    timing=MultiplayerFrameTiming{};timing.frame=frames;timing.views=options.local_players;
    host_tick_begin=current.tick;
    if(options.frame_observer){timing.schedule.measured=true;timing.schedule.loop=++observed_loops;
      timing.schedule.iteration_begin=observed_schedule_stamp(Clock::now());}
    bind_effect_history(false);
    reserve_common_history();
    service_snapshot_candidate();
    // No native frame/recording or presentation caller may run while the
    // session fences a predecessor generation. Keep only OS/device events
    // moving until a separately completed native role resumes the session.
    if(options.online_match){const auto phase=options.online_match->phase();
      if(phase==OnlineMatchSession::Phase::Failed){exit_reason=MultiplayerExitReason::online_disconnected;return Advance::stop;}
      if(peer_handoff_paused(phase)){
        if(!handoff_paused){
          // The explicit generation fence retires the old connection bank.
          // An approved replacement resumes from its independently bound cut.
          pending_replica_batch.reset();
          replica_playout.clear();interpolation_clock.reset();replica_playout_active=false;
          predicted_impact_ledger.reset();for(auto &gate:predicted_shot_gates)gate.reset();
          authority.suspend_publication();
          suspend_handoff_input();
          if(game_voice)game_voice->focus(false);
          for(auto *pad:platform.pads)send_controller_rumble(pad,{},false,false,true);
          handoff_paused=true;
        }
        SDL_Event event;
        while(SDL_PollEvent(&event)){
          if(event.type==SDL_QUIT){running=false;exit_reason=MultiplayerExitReason::window_quit;}
          else if(event.type==SDL_KEYDOWN&&event.key.windowID==SDL_GetWindowID(platform.window)&&event.key.keysym.sym==SDLK_ESCAPE){
            if(SDL_GetRelativeMouseMode())mouse_capture.release();
            else{running=false;exit_reason=MultiplayerExitReason::escape;}
          }
          else if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)renderer.resized();
          else if(event.type==SDL_CONTROLLERDEVICEADDED||event.type==SDL_CONTROLLERDEVICEREMOVED)platform.refresh();
          // Suspended admission tracks physical/focus changes without queuing
          // fresh presses or inflating an old interaction's held duration.
          keyboard.handle_event(event);
        }
        int discard_x,discard_y;SDL_GetRelativeMouseState(&discard_x,&discard_y);
        // Abort/context retirement must still progress while gameplay is
        // fenced. The scoped gate above removes outer borrowers first.
        if(running)authority.service();
        const auto serviced_phase=options.online_match->phase();
        if(serviced_phase==OnlineMatchSession::Phase::Failed){exit_reason=MultiplayerExitReason::online_disconnected;return Advance::stop;}
        if(running&&serviced_phase==OnlineMatchSession::Phase::Migrating)authority.prepare_takeover();
        else if(running&&serviced_phase==OnlineMatchSession::Phase::Rebinding){
          const auto agreement=options.online_match->peer_migration();
          if(agreement&&agreement->decision){
            const auto checkpoint=authority.common_cut_checkpoint(agreement->decision->previous,agreement->decision->value.cut);
            if(checkpoint)authority.prepare_client_rebind(checkpoint->proof(),checkpoint->origin(),checkpoint->frontier().replica);
          }
        }
        if(running)prepare_foreground_handoff();
        last=Clock::now();accumulator=0;SDL_Delay(2);return Advance::skip;
      }
      if(handoff_paused){
        prepared_foreground.reset();
        last=snapshot_time=Clock::now();accumulator=0;
        voice_preferences_sent=false;voice_speaker=GameVoiceSpeaker{};voice_sequences={};voice_players={};
        for(auto &p:voice_players)p.disabled=true;
        voice_credits.fill(10);voice_credit_at=last;handoff_paused=false;
      }
    }
    pose_continuity.mutating();
    if(scene_continuity)scene_continuity->mutating();
    if(hud_continuity)hud_continuity->mutating();
    if(locomotion_continuity)locomotion_continuity->mutating();
    if(material_continuity)material_continuity->mutating();
    authority.service();
    associate_common_history();
    if(options.source_recovery_observer)if(const auto observed=authority.shadow_state())options.source_recovery_observer(*observed);
    source_frame.reset();source_frame=authority.begin_frame();
    if(!source_frame)return Advance::skip;
    if(authority.local_input_pending())suspend_handoff_input();
    else resume_handoff_input();
    if(options.menu_audio_update)options.menu_audio_update();
    if(options.system_link_session && live_host() && (options.system_link_session->state()->phase==SystemLinkPhase::starting ||
          options.system_link_session->state()->phase==SystemLinkPhase::reviewing)) {
      live_host()->receive(controls);live_host()->broadcast();
      SDL_PumpEvents();if(SDL_HasEvent(SDL_QUIT)){exit_reason=MultiplayerExitReason::window_quit;return Advance::stop;}
      last=snapshot_time=Clock::now();accumulator=0;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));return Advance::skip;
    }
    if(options.system_link_session && live_client() && !live_client()->ready() && options.system_link_session->state() &&
        options.system_link_session->state()->phase==SystemLinkPhase::lobby) {
      exit_reason=MultiplayerExitReason::system_link_lobby;return Advance::stop;
    }
    if(options.system_link_session && !pending_replica_batch && options.system_link_session->world_update(current)) {
      exit_reason=MultiplayerExitReason::system_link_postgame;return Advance::stop;
    }
    bool terminal_replica_drain=false;
    if(options.online_match) {
      const auto phase=options.online_match->phase();
      terminal_replica_drain=pending_replica_batch &&
          (phase==OnlineMatchSession::Phase::Reviewing || phase==OnlineMatchSession::Phase::Postgame);
      if(phase==OnlineMatchSession::Phase::Failed){exit_reason=MultiplayerExitReason::online_disconnected;return Advance::stop;}
      // Finish already accepted bodies before the result path performs
      // its separate final receive. Never overwrite a pending live batch.
      if(phase==OnlineMatchSession::Phase::Reviewing && !pending_replica_batch) {
        if(live_client()){live_client()->receive();current=live_client()->state();}
        try {
          online_postgame_validate(current,options.variant,options.online_match->roster(),options.online_match->postgame());
          options.online_match->confirm_result();
        } catch(const FormatError &error) {
          std::cerr<<error.what()<<'\n';exit_reason=MultiplayerExitReason::online_disconnected;return Advance::stop;
        }
      }
      if(phase==OnlineMatchSession::Phase::Postgame && !pending_replica_batch) {
        if(live_client()){live_client()->receive();current=live_client()->state();}
        exit_reason=MultiplayerExitReason::online_postgame;return Advance::stop;
      }
      if(!live_client() && !postgame_published && current.game_over_complete() && current.results_finalized) {
        const auto report=online_postgame_publish(live_match(),options.variant,options.online_match->roster());
        options.online_match->complete(report.source);postgame_published=true;
      }
    }
    if(options.frame_observer)timing.schedule.terminal_drain=terminal_replica_drain;
    for(unsigned view=0;view<options.local_players;++view)if(auto *pad=selected_pad_for(view)) {
      const auto motors=rumble[view].evaluate(*rumble_tables);
      send_controller_rumble(pad,motors,platform.desktop->controller_settings.vibration,pause_menu && pause_menu->visible(),
          !handoff_input_paused_at && (SDL_GetWindowFlags(platform.window)&SDL_WINDOW_INPUT_FOCUS));
    }
    std::vector<SoundEvent> events;
    events.swap(receipt_values.sounds);
    now = Clock::now();
    if(options.frame_observer)timing.schedule.timed_start=observed_schedule_stamp(now);
    measured_at=now;
    audio_elapsed=options.reference_tick_cadence?tick_seconds:
        std::chrono::duration<float>(now-last).count();
    elapsed=std::min(.1f,audio_elapsed);
    last = now;
    if(options.reference_tick_cadence)accumulator=tick_seconds;
    else accumulator += elapsed;
    weather_realtime+=double(elapsed);

    weather_world.begin_frame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      // Keep each viewport's labels on its most recently used input device.
      // Stick drift inside the gameplay dead zone must not steal KBM labels.
      prompt_devices.event(e,SDL_GetWindowID(platform.window),keyboard_view);
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
      const bool pause_was_visible=pause_menu && pause_menu->visible();
      const bool pause_consumed=pause_menu && pause_menu->event(e);
      if(trace_game_pause && e.type==SDL_KEYDOWN && (e.key.keysym.sym==SDLK_ESCAPE ||
          e.key.keysym.sym==SDLK_RETURN || e.key.keysym.sym==SDLK_UP || e.key.keysym.sym==SDLK_DOWN))
        std::cerr<<"Native pause event key="<<e.key.keysym.sym<<" window="<<e.key.windowID
          <<" expected="<<SDL_GetWindowID(platform.window)<<" before="<<pause_was_visible<<" consumed="<<pause_consumed
          <<" after="<<(pause_menu && pause_menu->visible())<<" quit="<<(pause_menu && pause_menu->quit_requested())
          <<" screen="<<(pause_menu?pause_menu->state().frame().screen:0)
          <<" row="<<(pause_menu?pause_menu->state().frame().selected:0)<<'\n';
      if(pause_consumed) {
        keyboard.release();pad_holds={};pad_pending={};pad_controls={};pad_zoom_pending={};pad_zoom_held={};pad_scoreboard_held={};
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
        if(!handoff_input_paused_at)pad_holds={};
        platform.refresh();
        // Retain connected players by instance, rather than shifting every
        // viewport when SDL compacts its device list. Replacements enter only
        // vacant controller-owned views; the KBM player's slot stays theirs.
        for(unsigned i=0;i<options.local_players;++i)if(!selected_pad_for(i))local_pad_instances[i]=-1;
        for(auto *pad:platform.pads) {
          const auto instance=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
          if(std::find(local_pad_instances.begin(),local_pad_instances.end(),instance)!=local_pad_instances.end())continue;
          for(unsigned i=0;i<options.local_players;++i)if(controller_views[i] && local_pad_instances[i]<0) {
            local_pad_instances[i]=instance;break;
          }
        }
        for(unsigned i=0;i<options.local_players;++i)prompt_devices.bind(i,selected_pad_for(i));
      }
      else if (e.type == SDL_MOUSEBUTTONDOWN &&
               e.button.windowID == SDL_GetWindowID(platform.window) &&
               (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT))
        mouse_capture.capture();
      else if (e.type == SDL_MOUSEMOTION && e.motion.windowID == SDL_GetWindowID(platform.window) && SDL_GetRelativeMouseMode() && keyboard_view<options.local_players && !handoff_input_paused_at) {
        mouse_look.motion(e.motion.xrel,e.motion.yrel,platform.desktop->keyboard_settings,
            yaw[keyboard_view],pitch[keyboard_view]);
        if(!options.source_object_world && !source_relative_look())pitch[keyboard_view]=std::clamp(pitch[keyboard_view],-1.5f,1.5f);
      } else if (e.type == SDL_KEYDOWN && e.key.windowID == SDL_GetWindowID(platform.window) && e.key.keysym.sym == SDLK_ESCAPE) {
        if (SDL_GetRelativeMouseMode())
          mouse_capture.release();
        else {
          running = false;
          exit_reason = MultiplayerExitReason::escape;
        }
      }
    }
    if(pause_menu && pause_menu->quit_requested()) {
      running=false;exit_reason=MultiplayerExitReason::escape;
    }
    if(handoff_input_paused_at){int discard_x,discard_y;SDL_GetRelativeMouseState(&discard_x,&discard_y);}
    keyboard.settings(platform.desktop->keyboard_settings);
    if(game_voice) {
      game_voice->blocks(options.online_match->voice_blocks());
      const bool focused=(SDL_GetWindowFlags(platform.window)&SDL_WINDOW_INPUT_FOCUS)!=0 &&
          !(pause_menu && pause_menu->visible());
      game_voice->focus(focused);options.online_match->update_postgame_voice(focused);
      const auto self=options.online_match->self();
      const auto &p=current.players[self];
      // Authoritative eligibility is rechecked by the host before delivery.
      const auto age=p.source_death_tick==null_datum ? std::nullopt : std::optional(float(current.tick-p.source_death_tick)/30);
      const auto selected=game_voice->snapshot();
      // Review/postgame voice has its own session pump and may select a new
      // passive voice leader after the completed world's host leaves. The
      // live-world routing owner must not drain that same microphone/packet
      // queue or use its former fixed gameplay role during those phases.
      const auto voice_leader=options.online_match->host();
      if(options.online_match->phase()==OnlineMatchSession::Phase::Running) {
        game_voice->radio(SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_N],game_voice_active(p.connected && !(p.source_flags&2u),p.source_unit!=null_datum,p.source_previous_unit!=null_datum,age));
        if(live_host()) {
          voice_players[self].disabled=!selected.enabled;
          voice_players[self].muted=selected.muted;
        } else if(!voice_preferences_sent || selected.enabled!=voice_previous.enabled || selected.muted!=voice_previous.muted || now-voice_preferences_at>std::chrono::seconds(1)) {
          options.online_match->voice_preferences(voice_leader,selected.enabled,selected.muted);
          voice_preferences_sent=true;voice_preferences_at=now;
        }
        voice_previous=selected;
        const double dt=std::chrono::duration<double>(now-voice_credit_at).count();voice_credit_at=now;
        for(auto &credit:voice_credits)credit=std::min(10.,credit+dt*50);
        auto route=[&](GameVoiceFrame frame) {
          if(frame.source>=16 || frame.sequence<=voice_sequences[frame.source] || voice_credits[frame.source]<1)return;
          voice_sequences[frame.source]=frame.sequence;voice_credits[frame.source]-=1;
          for(unsigned id=0;id<16;++id) {
            const auto &player=live_match().player(id);const auto &unit=live_match().unit(id);
            auto &voice=voice_players[id];voice.connected=player.connected && !(player.source_flags&2u);voice.team=player.source_membership;
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
          if(live_host()) {
            if(packet.peer==self || packet.peer>=voice_players.size())continue;
            if(packet.preferences) {voice_players[packet.peer].disabled=!packet.enabled;voice_players[packet.peer].muted=packet.muted;}
            else {route(std::move(packet.frame));}
          } else if(packet.peer==voice_leader && !packet.preferences)game_voice->receive(std::move(packet.frame));
        }
        for(auto &frame:game_voice->outgoing()) {frame.source=self;if(live_host())route(std::move(frame));else options.online_match->voice(voice_leader,frame);}
      }
      if(selected.error!=voice_error) {voice_error=selected.error;if(!voice_error.empty())announce_voice(voice_error,now);}
      if(voice_caption && now>=voice_notice_until){SDL_SetWindowTitle(platform.window,game_window_title.c_str());voice_caption=false;}
      std::string text;
      if(auto id=voice_speaker.update(selected.talking,self);id && current.players[*id].connected)text=current.players[*id].name;
      for(auto &[representation,hud]:unit_huds)hud->voice_text(text);
    }
    for (unsigned i = 0; i < options.local_players; ++i) {
      local[i] = {};
      pad_controls[i] = {};
      pad_scoreboard_held[i]=false;
      pad_scoreboard_scroll[i]=0;
      death_cycle_pressed[i]=false;
      if(handoff_input_paused_at)return Advance::skip;
      if (i == keyboard_view) {
        local[i] = keyboard.snapshot(yaw[i], pitch[i]).controls;
        death_cycle_pressed[i]=local[i].jump && !keyboard_jump_held;
        keyboard_jump_held=local[i].jump;
      }
      if((pause_menu && pause_menu->captures_game_input())) {
        local[i]={0,0,yaw[i],pitch[i]};
        death_cycle_pressed[i]=false;
        // Original14A0D9 keeps multiplayer simulation running under its
        // overlay. Suppress this view's controls while the complete loop
        // still receives snapshots, advances ticks and services the menu.
        continue;
      }
      auto *selected_pad=selected_pad_for(i);
      bool pad_look_active=false;
      if(selected_pad) {
        auto *pad=selected_pad;
        auto &pad_input = pad_controls[i];
        const auto mapped=controller_input(sample_controller(pad),platform.desktop->controller_settings);
        pad_scoreboard_held[i]=mapped.scoreboard;
        // TU1.5 1279B5/15E7F3 use physical right-Y independently of the
        // gameplay stick preset and look inversion; retain source scaling.
        pad_scoreboard_scroll[i]=multiplayer_scoreboard_scroll_axis(
            SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_RIGHTY));
        pad_look_active=mapped.forward || mapped.right || mapped.look_yaw || mapped.look_pitch;
        pad_input.forward=mapped.forward;pad_input.right=mapped.right;
        local[i].forward=std::clamp(local[i].forward+pad_input.forward,-1.f,1.f);
        local[i].right=std::clamp(local[i].right+pad_input.right,-1.f,1.f);
        controller_look(mapped,platform.desktop->controller_settings,elapsed,yaw[i],pitch[i],!options.source_object_world);
        pad_zoom_pending[i]|=mapped.zoom&&!pad_zoom_held[i];pad_zoom_held[i]=mapped.zoom;
        pad_input.jump=mapped.jump;death_cycle_pressed[i]|=pad_input.jump&&!pad_previous[i].jump;
        pad_input.crouch=mapped.crouch;pad_input.reload=mapped.reload;pad_input.fire=mapped.fire;
        const bool armed_vehicle=local_vehicle_input_definition(i)!=nullptr;
        const bool dual=!armed_vehicle && i<ids.size()&&current.players[ids[i]].secondary_weapon.datum!=null_datum;
        const bool seated=i<ids.size() && current.players[ids[i]].source_parent!=null_datum;
        pad_input.fire_secondary=(dual || seated)&&mapped.secondary;
        pad_input.grenade=!dual&&mapped.grenade;
        if(armed_vehicle)pad_input.crouch|=mapped.secondary;
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
      const bool center=(i!=keyboard_view || pad_look_active)?platform.desktop->controller_settings.auto_center:platform.desktop->keyboard_settings.auto_center;
      const float speed=i<ids.size()?length(current.players[ids[i]].velocity):0.f;
      //189BF7 clears generic auto-centering when the unit has a parent. The
      // command-tick18A300 owner supplies authored seated automatic pitch.
      if(i<ids.size() && current.players[ids[i]].source_parent!=null_datum)auto_center[i]={};
      else auto_center_look(auto_center[i],center,local[i].forward,pitch[i]-prior_input_pitch[i],
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
      pending_camera_cycle[i]|=death_cycle_pressed[i];
    }
    if(pending_replica_batch && live_client()!=pending_replica_batch->client)
      throw FormatError("Pending replica batch changed its native connection without a generation fence");
    if(pending_replica_batch && options.online_match &&
        options.online_match->authority_context()!=pending_replica_batch->context)
      throw FormatError("Pending replica batch changed its admitted authority context without a generation fence");
    if (live_client()) {
      // received_states/origins are borrowed from this client. A subsequent
      // receive clears them, so acquire only after the preceding batch drained.
      if(options.frame_observer)timing.schedule.had_pending=bool(pending_replica_batch);
      {ObservedReceiptWork work(options.frame_observer?&timing.schedule.receive:nullptr);
        if(!pending_replica_batch){live_client()->receive();if(options.frame_observer)++timing.schedule.receive_calls;}
        authority.receive();}
      if (!live_client()->error().empty()) {
        if(options.frame_observer && live_client()->rejected_packet_count())
          std::cerr<<"LAN snapshot admission rejected "<<live_client()->rejected_packet_count()
              <<" packets; last: "<<live_client()->last_packet_rejection()<<std::endl;
        if(options.online_match&&(live_client()->ready()||options.online_match->phase()==OnlineMatchSession::Phase::Failed)){
          exit_reason=MultiplayerExitReason::online_disconnected;return Advance::stop;
        }
        throw FormatError(live_client()->error());
      }
      if (live_client()->ready()) {
        if (!joined) {
          ids.assign(live_client()->slots().begin(), live_client()->slots().end());
          joined = true;
          // Retained host events from before admission are not new local
          // effects. Baseline the journal before consuming the join snapshot.
          if(!live_client()->state().rumble_events.empty())rumble_serial=live_client()->state().rumble_events.back().serial;
          for(unsigned id=0;id<multiplayer_capacity;++id)heard_shots[id]=live_client()->state().players[id].shot_tick;
          for(unsigned id=0;id<multiplayer_capacity;++id)heard_teleports[id]=live_client()->state().players[id].source_teleport_serial;
          for (unsigned i = 0; i < ids.size(); ++i) {
            const auto &player=live_client()->state().players[ids[i]];
            // This frame built local[] before receiving admission. Rebase
            // that pending packet as well as the next frame's accumulator;
            // otherwise its stale zero yaw becomes the authority's raw
            // baseline, and the authored facing is applied again as look.
            yaw[i]=local[i].yaw=player.yaw;
            pitch[i]=local[i].pitch=player.pitch;
          }
          std::cout << "LAN joined slots=";
          for (auto id : ids)
            std::cout << id << ',';
          std::cout << std::endl;
        }
        auto received_states=live_client()->received_states();
        auto received_origins=live_client()->received_origins();
        // Balanced/Full: the adaptive playout releases the bank projected below.
        const bool playout_released=!pending_replica_batch && received_states.size()==received_origins.size() &&
            (terminal_certified_playout?release_terminal_snapshots(received_states,received_origins):
             release_replica_playout(received_states,received_origins));
        if(options.frame_observer){timing.schedule.bank_size=received_origins.size();
          timing.schedule.cursor_begin=pending_replica_batch?pending_replica_batch->next:0;}
        if(received_states.size()!=received_origins.size())throw FormatError("Accepted replica state lost its origin receipt");
        if(pending_replica_batch) {
          if(received_origins.size()!=pending_replica_batch->count || received_origins.empty() ||
              pending_replica_batch->next>=pending_replica_batch->count ||
              !received_origins.back().same(pending_replica_batch->tail))
            throw FormatError("Pending replica batch lost its exact accepted origin bank");
        } else if(!received_origins.empty()) {
          pending_replica_batch=PendingReplicaBatch{live_client(),0,received_origins.size(),received_origins.back(),
              options.online_match?options.online_match->authority_context():std::nullopt};
        }
        while(pending_replica_batch) {
          const std::size_t received_index=pending_replica_batch->next;
          const auto &received=received_states[received_index];
          const auto &origin=received_origins[received_index];
          std::optional<ObservedReceiptWork> projection;
          if(options.frame_observer)projection.emplace(&timing.schedule.projection);
          if(origin.tick()!=received.tick)throw FormatError("Accepted replica origin has a different tick");
          const bool same_tick=client_state_bound && received.tick==current.tick;
          const bool refresh_scene=same_tick && current.source_scene_roots_authoritative;
          const auto elapsed_ticks=same_tick?0u:received.tick-current.tick;
          std::array<bool,multiplayer_capacity> new_players{};
          for(unsigned id=0;id<multiplayer_capacity;++id)
            new_players[id]=received.players[id].connected && (!current.players[id].connected ||
                current.players[id].source_player!=received.players[id].source_player);
          device_event_audio(received.device_events);rumble_events(received);
          // Retain the previous completed-tick endpoint on a same-tick refresh.
          // Discrete ownership comes from the latest body; time does not move.
          if(!same_tick){previous = current;previous_receipt=current_receipt;}
          current = received;
          current_receipt=origin;
          client_state_bound=true;
          capture_source_body_poses(false,same_tick);
          replica_motion.receive(current,replica_seconds(now));
          if(auto *movement=playout_released?nullptr:ensure_nonhost_movement();movement && received_index<live_client()->received_inputs().size()) {
            // Reconcile every accepted tick in order; borrow collision afresh.
            const auto &receipt=live_client()->received_inputs()[received_index];
            for(unsigned view=0;view<ids.size();++view)
              movement->reconcile(view,received.players[ids[view]],receipt,live_match().simulation().collision());
          }
          if(current.source_scene_roots_authoritative) {
            // Accepted tick batches bind the same device bank before adopting
            // source handles, just as the later render/publication paths do.
            live_match().replicate_devices(current.devices);
            live_match().replicate_source_scene(current.source_scene_roots,current.source_body_poses);
            if(refresh_scene)scene_interpolation.refresh(current.tick,live_match().simulation().scene().objects());
            else scene_interpolation.capture(current.tick,live_match().simulation().scene().objects());
          }
          update_audio_listeners();
          update_sensors();
          // A frame may drain several genuinely accepted host ticks. Consume
          // each changed shot owner once before publishing the newest state;
          // skipped network ticks never supply invented firing events.
          if(audio)for(unsigned id=0;id<multiplayer_capacity;++id) {
            const auto &player=current.players[id];
            if(new_players[id]) {
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
          for(unsigned id=0;id<multiplayer_capacity;++id) {
            if(!same_tick && previous.tick && current.tick-previous.tick!=1){
              if(locomotion_continuity)locomotion_continuity->mutating();
              gait[id].reset();}
            update_native_gait(id,!same_tick);
          }
          update_world_effects(true);
          if(!local_definitions.empty())
            for(unsigned view=0;view<ids.size();++view) {
              const auto &fp_definition=species_for(current.players[ids[view]]).fp_definition;
              const auto &player=current.players[ids[view]];
              FirstPersonBindingInput binding_input{player.source_player,player.source_unit,
                  int(source_session_player_representation(player.source_configuration,source_game_type)),
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
                if(audio && !same_tick && client_secondary_clock_available[view]) {
                  auto event_hand=origin.hands[0];
                  first_person_animation_clock(event_hand,definition,tick_seconds,[&](const FirstPersonAnimationEvent &event){
                    try{audio->first_person(view,binding,event);}catch(const std::exception &e){std::cerr<<"Offhand audio: "<<e.what()<<'\n';}
                  });
                }
              }
              if(audio && !same_tick && client_fp_clock_available[view]) {
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
              if(player.first_person_pose)
                client_fp_views[view]=source_first_person_packet_view_tu15(*player.first_person_pose);
              if(client_fp_views[view].unit!=client_fp_reported_units[view]) {
                client_fp_reported_units[view]=client_fp_views[view].unit;
                std::cout<<"LAN source binding view="<<view<<" player="<<current.players[ids[view]].source_player
                    <<" unit="<<client_fp_views[view].unit<<" weapon="<<client_fp_views[view].hands[0].weapon
                    <<" flags="<<client_fp_views[view].hands[0].flags<<'\n';
              }
            }
          // Each accepted snapshot owns its emitted effect. Consuming only
          // the final render state loses intermediate weapon sounds.
          consume_journal_weapon_effects();
          consume_objective_audio(current);
          update_aim(current,elapsed_ticks);
          // Finish this actual accepted CPU tick before another receipt can
          // replace it. Refresh poses at the same tick without advancing time;
          // drain queued cues before retaining the local presentation cut.
          update_muzzle_effects(true);
          deliver_muzzle_sounds();
          bool scene_frontier_retained{};
          bool hud_frontier_retained{};
          bool locomotion_frontier_retained{};
          std::optional<MultiplayerSceneContinuity::Statistics> scene_statistics;
          std::optional<MultiplayerHudContinuity::Statistics> hud_statistics;
          std::optional<MultiplayerLocomotionContinuity::Statistics> locomotion_statistics;
          projection.reset();
          if(retain_effect_history && origin.context())try {
            auto *observation=options.frame_observer?&timing.receipt_capture:nullptr;
            // Declared before the frontier so total includes its actual local
            // retirement, including the displaced owners after remember().
            ObservedReceiptWork total(observation?&observation->total:nullptr);
            if(observation)++observation->attempts;
            auto frontier=capture_runtime_frontier(origin,events,observation);
            if(observation)++observation->completed;
            const auto statistics=frontier.scene.statistics();
            const auto hud_stats=frontier.hud.statistics();
            const auto locomotion_stats=frontier.locomotion.statistics();
            bool retained{};
            {ObservedReceiptWork work(observation?&observation->retention:nullptr);
              retained=effect_history.remember(origin,frontier);}
            if(observation&&retained)++observation->retained;
            scene_frontier_retained=retained&&statistics.has_value();
            hud_frontier_retained=retained&&hud_stats.has_value();
            locomotion_frontier_retained=retained&&locomotion_stats.has_value();
            if(scene_frontier_retained)scene_statistics=statistics;
            if(hud_frontier_retained)hud_statistics=hud_stats;
            if(locomotion_frontier_retained)locomotion_statistics=locomotion_stats;
            effect_history_error.clear();
          }catch(const std::exception &error){report_effect_history(error);}
          if(options.frame_observer)projection.emplace(&timing.schedule.projection);
          bind_effect_history();
          if(options.effect_history_observer){
            const auto history=effect_history.state();
            const auto resources=source_particle_draws.resource_state();
            const auto bound=observed_effect_cut();
            const auto bound_frontier=common_history_mode&&bound?
                authority.common_cut_checkpoint(bound->context,bound->seal):retained_effect_cut;
            const bool scene_frontier_bound=bound_frontier&&scene_continuity&&
                scene_continuity->owns_snapshot(bound_frontier->frontier().scene)&&
                bound_frontier->frontier().scene.matches(bound_frontier->origin())&&
                bound_frontier->proof().matches(bound_frontier->origin());
            const bool hud_frontier_bound=bound_frontier&&hud_continuity&&
                hud_continuity->owns_snapshot(bound_frontier->frontier().hud)&&
                bound_frontier->frontier().hud.matches(bound_frontier->origin())&&
                bound_frontier->proof().matches(bound_frontier->origin());
            const bool locomotion_frontier_bound=bound_frontier&&locomotion_continuity&&
                locomotion_continuity->owns_snapshot(bound_frontier->frontier().locomotion)&&
                bound_frontier->frontier().locomotion.matches(bound_frontier->origin())&&
                bound_frontier->proof().matches(bound_frontier->origin());
            std::uint16_t current_locomotion_unavailable{};
            for(unsigned id=0;id<multiplayer_capacity;++id)if(gait_unavailable[id])
              current_locomotion_unavailable|=std::uint16_t(1u<<id);
            options.effect_history_observer(origin,EffectPresentationObservation{
                history,world_effect_tick,muzzle_pose_tick,
                bound,history.newest_sequence==origin.sequence(),bound.has_value(),
                events.size(),weapon_effect_sounds.size(),effect_sounds.checkpoint().pending_count(),
                {source_migration_resources,bool(source_particle_world),bool(source_model_world),
                 resources.materials,resources.first_person_systems,resources.world_systems,
                 bsp_effect_lights.resident_model_sections(),source_materials_at_upload,mesh.materials.size()},
                scene_frontier_retained,scene_frontier_bound,scene_statistics,
                hud_frontier_retained,hud_frontier_bound,hud_statistics,
                locomotion_frontier_retained,locomotion_frontier_bound,locomotion_statistics,current_locomotion_unavailable});
          }
          if(!same_tick)snapshot_time = now;
          if(options.replica_observer)options.replica_observer(current,origin,!same_tick);
          if(options.frame_observer){++timing.schedule.projected_states;
            timing.schedule.cursor_end=pending_replica_batch->next+1;}
          // Advance only after the entire original projection, capture and
          // observer sequence completed. The original command tail below still
          // owns consumption, serials, pulse clearing and clock advancement.
          if(++pending_replica_batch->next==pending_replica_batch->count){
            pending_replica_batch.reset();
            if(terminal_projection_endpoint){
              terminal_current_endpoint=std::move(terminal_projection_endpoint);
            }
          }
          projection.reset();
          if(paced_replica_batches&&!terminal_certified_playout)break;
        }
      }
    }
    if(options.frame_observer)timing.schedule.has_pending=bool(pending_replica_batch);
    // Adaptive playout: inputs report this frame's presented time.
    if(replica_playout_active)blend=replica_playout_blend();
    while (accumulator >= tick_seconds && !terminal_replica_drain) {
      if (options.capture_tick && current.tick >= options.capture_tick) break;
      const bool carried_input=authority.continue_local_input(controls);
      if(carried_input)for(unsigned view=0;view<ids.size();++view){
        local[view]=controls[ids[view]];yaw[view]=local[view].yaw;pitch[view]=local[view].pitch;
        view_zoom[view]=local[view].zoom;view_zoom_reset[view]=local[view].zoom_reset_serial;
      }
      bool scoreboard_requested=false;
      if (!carried_input && !options.replay && options.input_script.empty() && options.local_players) {
        // Consume taps only when delivering a simulation command. An idle
        // attached pad must not clear keyboard/mouse actions or movement.
        const auto input = keyboard.consume(keyboard_view<options.local_players?yaw[keyboard_view]:0.f,
            keyboard_view<options.local_players?pitch[keyboard_view]:0.f);
        if(keyboard_view<options.local_players) {
          scoreboard_requested=input.actions.scoreboard;
          local[keyboard_view].jump = input.controls.jump || pad_controls[keyboard_view].jump;
          local[keyboard_view].reload = input.controls.reload || pad_controls[keyboard_view].reload;
          const bool dual=keyboard_view<ids.size() && current.players[ids[keyboard_view]].secondary_weapon.datum!=null_datum;
          const bool inverted=platform.desktop->keyboard_settings.dual_wield_invert;
          const bool seated=keyboard_view<ids.size() && current.players[ids[keyboard_view]].source_parent!=null_datum;
          const auto gameplay=input.gameplay_controls(dual,seated,inverted);
          local[keyboard_view].crouch = gameplay.crouch || pad_controls[keyboard_view].crouch;
          local[keyboard_view].fire = gameplay.fire || pad_controls[keyboard_view].fire;
          local[keyboard_view].grenade = input.controls.grenade || pad_controls[keyboard_view].grenade;
          local[keyboard_view].melee=input.controls.melee || pad_controls[keyboard_view].melee;
          local[keyboard_view].pickup=input.controls.pickup || pad_controls[keyboard_view].pickup;
          local[keyboard_view].dual_wield=input.controls.dual_wield || pad_controls[keyboard_view].dual_wield;
          local[keyboard_view].pickup_milliseconds=std::max(input.controls.pickup_milliseconds,pad_controls[keyboard_view].pickup_milliseconds);
          local[keyboard_view].dual_wield_milliseconds=std::max(input.controls.dual_wield_milliseconds,pad_controls[keyboard_view].dual_wield_milliseconds);
          local[keyboard_view].switch_weapon=input.controls.switch_weapon || pad_controls[keyboard_view].switch_weapon;
          local[keyboard_view].fire_secondary=gameplay.fire_secondary || pad_controls[keyboard_view].fire_secondary;
        }
        //1884E0's ordinary valid-unit path rechecks C9920 even without a
        // scope edge. Its retained view selector is separate from unit CURRENT.
        for(unsigned view=0;view<ids.size();++view) {
          const auto &player=current.players[ids[view]];
          const auto weapon=content.weapons.find(player.weapon);
          if(player.source_unit!=null_datum && weapon!=content.weapons.end()) {
            const std::array<std::int16_t,1> modes{player.source_zoom_magazine_mode};
            const ZoomWeaponView primary{weapon->second.zoom,modes};
            if(!source_zoom_admitted({player.source_parent,player.source_action_word,{&primary,nullptr}}))
              view_zoom[view]=-1;
          }
        }
        for(unsigned view=0;view<ids.size();++view)if(((view==keyboard_view && input.actions.scope_pressed)||pad_zoom_pending[view]) && current.players[ids[view]].secondary_weapon.datum==null_datum) {
          const auto &player=current.players[ids[view]];
          const auto weapon=content.weapons.find(player.weapon);
          if(scope_constants && content.binocular_zoom && weapon!=content.weapons.end()) {
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
      for(unsigned view=0;view<options.local_players;++view) {
        local[view].zoom=view_zoom[view];
        local[view].zoom_reset_serial=view_zoom_reset[view];
      }
      if (!carried_input && !options.input_script.empty()) {
        for (unsigned i = 0; i < options.local_players; ++i) {
          local[i] = {};
          local[i].yaw = yaw[i];
          local[i].pitch = pitch[i];
          const auto at = scripted_inputs.find(steps);
          if (at != scripted_inputs.end() && at->second[i]) {
            const auto&input=*at->second[i];
            local[i].fire=input.fire;local[i].reload=input.reload;local[i].grenade=input.grenade;local[i].switch_grenade=input.switch_grenade;
            local[i].forward=input.move.x;local[i].right=input.move.y;
            local[i].jump=input.jump;local[i].crouch=input.crouch;
            if(input.look){yaw[i]=local[i].yaw=input.look->x;pitch[i]=local[i].pitch=input.look->y;}
          }
        }
      }
      // Preserve the displayed source-selected vehicle action through delivery.
      // Authority must validate this intent rather than select a different seat.
      if(!carried_input)for(unsigned view=0;view<ids.size();++view) {
        const auto &target=current.players[ids[view]].action_prompt.target;
        if(target.action==0 || (target.action>=4 && target.action<=6))
          local[view].vehicle_interaction_target=target.action?target:SourcePlayerActionTarget{};
      }
      if (live_client()) {
        // Lag-compensated hosts rewind hitscan targets to the displayed time.
        live_client()->set_view(previous.tick,current.tick,blend);
        if(auto *movement=joined?ensure_nonhost_movement():nullptr) {
          // Predict this input's motion before it leaves: same tick, as the
          // 2004 distributed client stepped its own biped.
          const auto sequence=live_client()->next_input_sequence();
          for(unsigned view=0;view<ids.size();++view) {
            const auto &latest=newest_replica().players[ids[view]];
            float predicted_yaw=local[view].yaw,predicted_pitch=local[view].pitch;
            if(const auto consumed=live_client()->consumed_look(view)) {
              predicted_yaw=std::remainder(latest.yaw+std::remainder(local[view].yaw-(*consumed)[0],6.283185307f),6.283185307f);
              predicted_pitch=std::clamp(latest.pitch+(local[view].pitch-(*consumed)[1]),-1.54f,1.54f);
            }
            live_client()->set_motion(view,movement->predict(view,sequence,local[view],predicted_yaw,predicted_pitch,
                latest,biped_for(latest),live_match().simulation().collision()));
          }
        }
        live_client()->send(std::span(local).first(options.local_players));
        predicted_shot_input();
        if(options.frame_observer)++timing.schedule.client_send_calls;
      }
      else {
        for (unsigned i = 0; i < ids.size(); ++i)
          controls[ids[i]] = local[i];
        if (live_host())
          live_host()->receive(controls);
        authority.admit_players();
        previous = current;
        const auto step_begin=host_stamp();
        live_match().step(controls);
        host_returned(step_begin,timing.match_step);
        for(const auto &event:live_match().take_prop_events())if(event.effect!=null_datum)try {
          effect_sounds.spawn(event.effect,event.handle,event.effect_position);
        }catch(const std::exception &error){std::cerr<<"Authored prop effect sound: "<<error.what()<<'\n';}
        // World-effect playback consumes each authoritative detonation once
        // on the game clock, including its sound parts. Drain the separate
        // gameplay callback bank without replaying a second audio timeline.
        (void)live_match().take_grenade_events();
        const auto projection_begin=host_stamp();
        current = live_match().state();
        host_returned(projection_begin,timing.state_projection);
        const auto post_step_begin=host_stamp();
        deliver_source_weapon_audio();
        complete_local_vehicle_cameras();
        consume_journal_weapon_effects();
        consume_objective_audio(current);
        update_audio_listeners();
        device_event_audio(current.device_events);rumble_events(current);
        scene_interpolation.capture(current.tick,live_match().simulation().scene().objects());
        capture_source_body_poses();
        update_sensors();
        for(unsigned id=0;id<multiplayer_capacity;++id)
          if(current.players[id].connected)update_native_gait(id);
        update_world_effects();
        synchronize_zoom();
        update_aim(current,1);
        update_muzzle_effects();
        host_returned(post_step_begin,timing.post_step);
        if (live_host())
          authority.broadcast_candidate_frame(source_frame);
        if(!source_frame)break;
      }
      if(scoreboard) {
        // 15EBA0 visits occupied player slots, while the inactivity byte is
        // shared across views. Preserve that order for nonconsecutive seats.
        std::array<unsigned,4> order{0,1,2,3};
        std::sort(order.begin(),order.begin()+ids.size(),[&](unsigned a,unsigned b) {
          return (current.players[ids[a]].source_player&0xffffu)<
              (current.players[ids[b]].source_player&0xffffu);
        });
        for(unsigned index=0;index<ids.size();++index) {
          const auto view=order[index];
          scoreboard->step(view,(view==keyboard_view && scoreboard_requested) || pad_scoreboard_held[view],
              current,ids[view],pad_scoreboard_scroll[view]);
        }
      }
      if(options.frame_observer){const auto stamp=observed_schedule_stamp(Clock::now());
        if(!timing.schedule.command_tails)timing.schedule.first_command_tail=stamp;
        timing.schedule.last_command_tail=stamp;++timing.schedule.command_tails;}
      if (options.input_observer) {
        if(options.frame_observer){const auto ordinal=++observed_input_callbacks;
          if(!timing.schedule.input_observer_calls)timing.schedule.first_observer=ordinal;
          timing.schedule.last_observer=ordinal;++timing.schedule.input_observer_calls;}
        options.input_observer(steps, std::span(local).first(options.local_players), current);
      }
      if(!carried_input){
        for(auto &pending:pad_pending)pending={};pad_zoom_pending={};
        for(auto &holds:pad_holds)for(auto &hold:holds)hold.consume();
        for(auto &pad:pad_controls)pad.pickup=pad.dual_wield=pad.switch_weapon=pad.grenade=pad.melee=pad.switch_grenade=false;
        for(auto &command:local)command.pickup=command.dual_wield=command.switch_weapon=command.grenade=command.melee=command.switch_grenade=false;
      }
      for(auto &player:rumble)player.advance(tick_seconds);
      accumulator -= tick_seconds;
      ++steps;
      if(!live_client() && options.session_world_update && options.session_world_update(live_match())) {
        running=false;
        exit_reason=MultiplayerExitReason::session_world_close;
        break;
      }
      // Reacquire physical input on the next render frame. Never let another
      // catch-up step in this frame turn the frozen device ledger into input.
      if(carried_input&&!authority.local_input_pending())break;
    }
    if(!running)return Advance::stop; // The session owner has already requested world close.
    if(!source_frame)return Advance::skip; // A replacement generation fenced this frame.
    if(live_client() && joined) {
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
    if (!live_client()) {
      auto emitted=live_match().take_sounds();
      // Restored, not-yet-submitted receipt cues precede newly admitted host
      // requests. Preserve Reset/order semantics instead of discarding them.
      events.insert(events.end(),std::make_move_iterator(emitted.begin()),std::make_move_iterator(emitted.end()));
    }
    if(!live_client())live_match().advance_audio(audio_elapsed);
    else {
      if(current.game_over_fade_started && !peer_game_over_fade_started) {
        source_game_over_fade(peer_sound_classes,true,*content.original_data->game_over_constants);
        peer_game_over_fade_started=true;
      }
      peer_sound_classes.advance(audio_elapsed);
    }
    if(audio)audio->master_volume(float(platform.desktop->display_settings.volume)/10.f);
    if(audio)audio->class_gains(live_client()?peer_sound_classes.gains():live_match().simulation().sound_classes().gains());
    if(options.session_audio_update)options.session_audio_update(
        float(platform.desktop->display_settings.volume)/10.f,live_match().simulation().sound_classes().gains());
    if(objective_announcer)objective_announcer->audio_update(float(platform.desktop->display_settings.volume)/10.f,
        live_client()?peer_sound_classes.gains():live_match().simulation().sound_classes().gains());
    for(const auto id:ids) {
      const auto &player=current.players[id];
      if(heard_teleports[id]==player.source_teleport_serial)continue;
      heard_teleports[id]=player.source_teleport_serial;
      if(audio && player.source_teleport_serial && player.source_teleport_sound!=null_datum)try {
        audio->impulse(player.source_teleport_sound,1,MultiplayerAudioKind::Impulse,player.source_unit);
      }catch(const std::exception &error){std::cerr<<"Original teleporter audio: "<<error.what()<<'\n';}
    }
    if (audio)
      for (const auto &sound : events)
        try {
          if(sound.action==SoundEvent::Reset)audio->stop();
          else if(sound.action==SoundEvent::Impulse)audio->impulse_at(sound.datum,sound.position,sound.gain);
          else if(sound.action==SoundEvent::NonSpatialImpulse)audio->impulse(sound.datum,sound.gain);
        } catch (const std::exception &e) {
          std::cerr << "Multiplayer sound: " << e.what() << '\n';
        }
    if(audio)for(const auto &[datum,owner]:weapon_effect_sounds)try {
      if(local_vehicle_sound_view(owner,false))continue;
      audio->impulse(datum,1,MultiplayerAudioKind::Impulse,owner);
    }catch(const std::exception &e){std::cerr<<"Weapon event sound: "<<e.what()<<'\n';}
    weapon_effect_sounds.clear();
    effect_sounds.advance(audio_elapsed,[&](const EffectSoundCue &cue,std::uint32_t owner,Vec3 position){
      if(local_vehicle_sound_view(owner,false))return;
      if(audio)try{audio->impulse_at(cue.sound,position,cue.gain,MultiplayerAudioKind::Impulse,owner);}
      catch(const std::exception &error){std::cerr<<"Prop effect sound: "<<error.what()<<'\n';}
    });
    if(audio)audio->update();
    if (!joined) {
      SDL_Delay(5);
      return Advance::skip;
    }
    const auto present_players = unsigned(std::count_if(
        current.players.begin(), current.players.end(),
        [](const auto &p) { return p.connected; }));
    peak_players = std::max(peak_players, present_players);
    if (!capture_ready) {
      if (present_players < options.capture_players) {
        SDL_Delay(5);
        return Advance::skip;
      }
      capture_ready = true;
      std::cout << "LAN capture participants=" << present_players << std::endl;
    }
    blend =
        live_client() ? replica_playout_active ? replica_playout_blend() : std::clamp(
                     std::chrono::duration<float>(now - snapshot_time).count() /
                         tick_seconds,
                     0.f, 1.f)
               : options.reference_render_fraction.value_or(std::clamp(float(accumulator / tick_seconds),0.f,1.f));
    // Native high-refresh pose sampling follows the local fixed-step remainder.
    // Packet receipt resets the position interpolation window, but must not
    // reset the fraction of an already validated one-tick FP channel pair.
    animation_blend=options.reference_render_fraction.value_or(std::clamp(float(accumulator/tick_seconds),0.f,1.f));
    pose_tick=animation_render_tick(previous.tick,current.tick,animation_blend);
    measure(timing.input_ms);
    if(options.frame_observer)timing.schedule.input_end=observed_schedule_stamp(Clock::now());
    return Advance::proceed;
    };
    if(!ticked_ahead) {
      const auto step=advance(timing);
      if(step==Advance::stop)break;
      if(step==Advance::skip)continue;
    } else {
      ticked_ahead=false;timing=std::move(pending_timing);timing.frame=frames;
      if(stop_next){stop_next=false;break;}
      if(skip_next){skip_next=false;continue;}
      // The ahead tick stamped now during the previous draw; this frame's
      // pre-render span starts here.
      now=Clock::now();
    }
    complete_local_vehicle_cameras();
    gameplay_fade_available=cache.is_mcc() && !ids.empty() && (!live_client() || client_state_bound) && current.source_gameplay_fade.has_value();
    if(gameplay_fade_available) {
      std::vector<std::uint32_t> local_fade_players;local_fade_players.reserve(ids.size());
      for(const auto slot:ids)local_fade_players.push_back(current.players.at(slot).source_player);
      gameplay_fade.consume(*current.source_gameplay_fade,local_fade_players);
    }
    auto shown=multiplayer_presentation(previous,current,blend);
    if(live_client() && joined) {
      // Dead-reckoned roots replace the interpolated ones; posed objects
      // carry their newest WORLD nodes rigidly with the same motion.
      const auto at=replica_seconds(now);
      replica_motion.present(shown,at);
      const auto motions=replica_motion.node_motions(at);
      source_body_interpolation.present(motions);scene_interpolation.present(motions);
    }else if(replica_motion.active()) {
      replica_motion.reset();source_body_interpolation.present({});scene_interpolation.present({});
    }
    // The own on-foot biped is drawn at its prediction (local tick blend).
    if(nonhost_movement && live_client() && joined)nonhost_movement->present(shown,ids,animation_blend);
    // Its third-person body (other local views, shadows) and everything it
    // carries sit at that same predicted origin, keeping the replicated
    // pose. No movement client (H2_NET_PREDICTION=0, host): no roots.
    std::vector<SourceWorldPredictedRoot> predicted_bodies;
    if(nonhost_movement && live_client() && joined) {
      std::array<std::optional<Vec3>,4> presented_positions{};
      const auto views=std::min<std::size_t>(ids.size(),presented_positions.size());
      for(unsigned view=0;view<views;++view)presented_positions[view]=nonhost_movement->presented(view,animation_blend);
      predicted_bodies=predicted_body_roots(previous,current,shown,ids,std::span(presented_positions).first(views));
    }
    source_body_interpolation.predict(predicted_bodies);
    scenery_render_fraction=blend;
    hud_vertices.clear();
    for(auto *banks:{&worlds,&foregrounds,&backgrounds,&reflection_backgrounds})
      for(auto &vertices:*banks)vertices.clear();
    if(!world_effect_tick || *world_effect_tick!=current.tick)update_world_effects();
    const auto world_decals=source_decal_consumer_owned()?std::vector<Vertex>{}:
        world_effects.decals(current.tick,animation_blend,tick_seconds);
    for(const auto&gap:world_effects.diagnostics().unavailable)
      if(reported_world_effect_gaps.insert(gap).second)std::cerr<<"World effect boundary: "<<gap<<'\n';
    std::vector<std::vector<ParticleDraw>> foreground_particles(options.local_players),world_particles(options.local_players);
    std::vector<std::vector<SourceLensQueryDraw>> lens_queries(options.local_players);
    std::vector<std::vector<SourceLensDraw>> lens_draws(options.local_players);
    update_muzzle_effects();
    deliver_muzzle_sounds();
    std::vector<std::vector<WaterFrame>> water_frames(options.local_players);
    std::vector<std::vector<SourceDecalDraw>> source_decals(options.local_players);
    std::vector<std::vector<SourceContrailDraw>> contrail_draws(options.local_players);
    std::vector<std::vector<ObjectLightingBinding>> light_bindings(options.local_players);
    std::vector<std::vector<CamouflageBinding>> camouflage_bindings(options.local_players);
    std::vector<std::vector<std::vector<std::uint32_t>>> camouflage_groups(options.local_players);
    std::vector<SourceEffectLightView> effect_light_views(options.local_players);
    std::vector<std::vector<path_tracing::Light>> path_tracing_lights;
    const bool path_tracing_active=platform.desktop->display_settings.graphics_mode==pstencil::GraphicsMode::PathTracing;
    path_tracing_sky_lighting.set_active(path_tracing_active);
    if(path_tracing_active)
      path_tracing_lights.resize(options.local_players);
    std::vector<std::vector<WorldEffectLightReceiver>> effect_light_receivers(options.local_players);
    std::vector<std::vector<EffectLightRawDraw>> raw_effect_lights(options.local_players);
    std::vector<std::vector<ModelObjectDraw>> model_object_draws(options.local_players);
    std::vector<std::vector<pstencil::LightmapCaster>> pstencil_lightmap_casters(options.local_players);
    std::vector<std::vector<pstencil::PointCaster>> pstencil_point_casters(options.local_players);
    const auto collect_pstencil_lightmap=[&](unsigned view,std::span<const Vertex> vertices,const ObjectLightingCache& light) {
      if(platform.desktop->display_settings.graphics_mode!=pstencil::GraphicsMode::pStencil || vertices.empty() ||
          !light.sample_valid || !light.lighting_valid || dot(light.current.incident,light.current.incident)<1.e-8f)return;
      // Only the actual biped/vehicle call sites use this owner. The 2003
      // composite uses the raw incident, independently of retail angle limits.
      pstencil_lightmap_casters[view].push_back({{vertices.begin(),vertices.end()},light.current.incident});
    };
    std::vector<SourceLiveShadowSelectionTu15> live_shadow_selections(options.local_players);
    std::vector<std::vector<SourceLiveShadowRootTu15>> live_shadow_roots(options.local_players);
    const bool reference_shadow_frame=source_shadow_scene && current.tick==options.reference_live_shadow_gpu_tick;
    //These owners outlive every span/pointer in ModelView through draw().
    //Each Scene root copies current palettes before secondary query reuse.
    std::optional<RendererTargets::Use> reference_shadow_targets;
    std::optional<SourceLiveShadowPhaseInputsTu15> reference_shadow_phases;
    std::vector<SourceShadowSceneRoot> reference_shadow_roots;
    std::vector<SourceShadowRawPlanRoot> reference_shadow_root_views;
    std::optional<SourceShadowRawPlan> reference_shadow_plan;
    SourceShadowRawPlanStats reference_shadow_stats;
    std::vector<std::map<std::uint32_t,std::vector<SourceLiveModelBaseDrawTu15>>> live_vehicle_base(options.local_players);
    std::vector<std::map<std::uint32_t,std::vector<SourceLiveModelBaseDrawTu15>>> live_prop_base(options.local_players);
    std::vector<std::map<std::uint32_t,std::vector<SourceLiveModelBaseDrawTu15>>> live_unit_base(options.local_players);
    std::vector<std::map<std::uint32_t,std::vector<Vertex>>> live_unit_vertices(options.local_players);
    std::vector<std::map<std::uint32_t,std::shared_ptr<const ModelSkinGeometry>>> live_unit_gpu(options.local_players);
    std::vector<std::vector<ModelSkinSubmission>> unit_skin_submissions(options.local_players);
    std::vector<std::vector<ModelSkinSubmission>> foreground_skin_submissions(options.local_players);

    std::vector<std::set<std::uint32_t>> submitted_unit_base(options.local_players);
    std::vector<std::map<std::uint32_t,std::vector<Transform>>> live_vehicle_nodes(options.local_players);
    std::vector<std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const VertexProgramBank>>> live_vehicle_prt(options.local_players);
    std::vector<std::vector<std::vector<Vertex>>> vehicle_vertices(options.local_players);
    for(auto& bank:vehicle_vertices)bank.reserve(current.world_vehicles.size());
    std::vector<std::vector<std::vector<SourcePrtMaterialBank>>> vehicle_prt_banks(options.local_players);
    for(auto& bank:vehicle_prt_banks)bank.reserve(current.world_vehicles.size());
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
    SourceObjectAudioViewsTu15 object_audio_views;
    int drawable_w{}, drawable_h{};
    SDL_Vulkan_GetDrawableSize(platform.window, &drawable_w, &drawable_h);
    if(reference_shadow_frame && (drawable_w!=640 || drawable_h!=480))
      throw FormatError("Live shadow GPU reference requires the actual640x480 drawable window");
    if (drawable_w < 1 || drawable_h < 1) {
      SDL_Delay(10);
      continue;
    }
    // Source coverage, shader detail and screen-space effects use the scene
    // resolution. An explicit internal resolution can differ from the SDL
    // presentation surface; temporal input scaling does not change this logical viewport.
    const auto &render_settings=platform.desktop->display_settings;
    if(render_settings.internal_width)drawable_w=int(render_settings.internal_width);
    if(render_settings.internal_height)drawable_h=int(render_settings.internal_height);
    if(reference_shadow_frame && (drawable_w!=640 || drawable_h!=480 || render_settings.upscaler!=Upscaler::Off))
      throw FormatError("Live shadow GPU reference requires actual unscaled640x480 single-sample presentation");
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
    const double shader_time=held_pose_shading_reference_time();
#else
    const double shader_time=double(current.tick)*tick_seconds+double(blend)*tick_seconds;
#endif
    props.begin_source_frame();
    // Actual12E142 calls the game update13AA50 before12E151 calls the
    // per-frame13A9B0 owner. Its13AA28 advances particles with this frame's
    // elapsed input. Views share one phase and then publish their union.
    if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics()) {
      if(source_particle_world)
        source_particle_world->render_model_view([&](auto &source){source_particle_draws.prepare_storage(source);});
      live_world()->source_objects->advance_effect_frame(elapsed,options.source_particle_phase_observer);
      if(source_particle_world)
        source_particle_world->render_model_view([&](auto &source) {
          // Admit actual completed effect/system records, including impacts,
          // detonations and children created by the shared particle phase.
          for(const auto effect:source_particle_draws.preload_live_world(source.memory()))
            if(source_particle_draws.world_effect(effect))
              world_effects.source_particle_effect(effect);
          source_particle_draws.prepare_storage(source);
        });
    }
    const bool replica_lens_frame=native_lens && live_client() && !source_particle_world;
    if(native_lens && !replica_lens_frame)native_lens->suspend();
    if(replica_lens_frame)native_lens->begin(options.local_players);
    if(source_particle_world && source_lens_queries)
      source_particle_world->render_model_view([&](auto &source){source_lens_queries->begin(source);});
    if(source_particle_world && source_lens_draws)source_lens_draws->begin();
    std::array<std::optional<std::vector<SourceFirstPersonModelTu15>>,4> cached_native_fp_groups;
    if(source_model_world)source_model_world->begin_model_frame();
    if(source_particle_world && source_particle_world!=source_model_world)
      source_particle_world->begin_model_frame();
    const bool widescreen = std::uint64_t(drawable_w)*3>std::uint64_t(drawable_h)*4;
    // Only authoritative native tombstones remove client props. Local shot
    // counters never infer an explosion or another object's lifetime.
    if(live_client() && joined){
      // Only an accepted host bank can establish replica identities. Bind
      // device ownership before scene adoption rebuilds dependent collision.
      live_match().replicate_devices(current.devices);
      if(current.source_scene_roots_authoritative)
        live_match().replicate_source_scene(current.source_scene_roots,current.source_body_poses);
      live_match().replicate_prop_removals(current.prop_removals);
      live_match().replicate_prop_regions(current.prop_regions);
    }
    if(!live_client() || joined)props.synchronize(live_match().simulation().scene());
    dynamic_objects.synchronize(current.source_dynamic_objects,current.source_body_poses);
    if(options.source_dynamic_observer)options.source_dynamic_observer(current,dynamic_objects);
    if(current.source_scene_roots_authoritative && options.source_scene_observer)
      options.source_scene_observer(current,live_match().simulation().scene());
    props.interpolate(live_match().simulation().scene(),scene_interpolation,blend);
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
    for(const auto& object:live_match().simulation().scene().objects())
      if(object.handle!=null_datum && content.objects.at(object.placement).type==11)
        current_prop_handles.insert(object.handle);
    for(auto at=prop_query_handles.begin();at!=prop_query_handles.end();)
      if(!current_prop_handles.contains(*at)){prop_initial_state.destroy(*at);at=prop_query_handles.erase(at);}
      else ++at;
    if(!live_client() || joined)machines.synchronize(live_match().simulation().scene(),&live_match().simulation().devices());
    if(current.devices.tick>previous.devices.tick)machines.interpolate(previous.devices,blend);
    machines.interpolate(live_match().simulation().scene(),scene_interpolation,blend);
    if(!live_client()) {
      source_scenery.synchronize(live_match().simulation().scene(),scenery_regions);
      source_scenery.interpolate(live_match().simulation().scene(),scene_interpolation,blend);
      std::set<std::uint32_t> current_scenery_handles;
      for(const auto &object:live_match().simulation().scene().objects())
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
      const auto packets=(!live_client() || joined)?machines.portal_owners():std::vector<SourceMachinePortalState>{};
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
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
      if(const auto* reference=held_pose_shading_reference(biped_for(root).datum)) {
        object_owner_spheres[j]=reference->sphere;light.lighting=reference->lighting;
        light.lighting_valid=true;continue;
      }
#endif
      if(reference_player) {
        if(!reference_player->owner_sphere)
          throw FormatError("original player shader selection requires its retained owner sphere");
        object_owner_spheres[j]=*reference_player->owner_sphere;
        light.lighting=reference_player->root_lighting;
        light.lighting_valid=true;
        continue;
      }
      if(current.source_body_poses_authoritative) {
        object_owner_spheres[j]=predicted_body_sphere(predicted_bodies,body_handle(root),source_body_pose(root)->sphere);
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
      const auto unit_camouflage=source_object_camouflage(owner.source_unit,context);
      for(const auto &[representation,body]:species)
        for(const auto *layers:{&body.body_shields,&body.hand_shields})
          for(const auto &[shader,layer]:*layers)
            camouflage_range(layer.materials[j],layer.count,unit_camouflage);
      for(const auto &[definition,bank]:body_guns)
        for(unsigned hand=0;hand<2;++hand)
          camouflage_range(bank.materials[j*2+hand],unsigned(bank.gun.model.mesh.materials.size()),camouflage[hand]);
      for(unsigned i=0;i<ids.size();++i)if(ids[i]==j)
        for(const auto &[representation,body]:species)
          for(unsigned hand=0;hand<2;++hand) {
            if(body.guns.empty())continue;
            const auto &gun=body.guns.at(i*2+hand);
            const auto &view=live_client()?client_fp_views[i]:live_match().first_person_view(i);
            camouflage_range(gun.material,unsigned(gun.model.mesh.materials.size()),
                source_object_camouflage(view.hands[hand].weapon,context));
          }
    }
    struct BodyFramePose {
      std::vector<Vertex> body,weapon;std::vector<Transform> nodes;
      const AnimationClip *primary{};std::optional<float> frame;
      std::vector<Transform> weapon_nodes;
      bool body_deferred{},weapon_deferred{};
    };
    // Frame-local undecorated geometry. Animation/root/time are common to all
    // views; shader choice and material offsets are still applied to each copy.
    // Single-view draws retain their direct path without an extra cache copy.
    std::array<std::optional<BodyFramePose>,multiplayer_capacity> body_frame_poses;
    std::map<std::uint32_t,std::vector<Transform>> completed_render_nodes;
    ModelFramePoseCache vehicle_frame_poses;
    ModelFramePoseCache unit_frame_poses;
    const auto *repeat_unit_skins=std::getenv("HALO2_REFERENCE_REPEAT_UNIT_SKINS");
    SourceLiveModelPoseProviderTu15 unit_pose_provider;
    if(!(repeat_unit_skins && *repeat_unit_skins))
      unit_pose_provider=[&](std::uint32_t owner,unsigned lod,const RenderModel &model,
          std::span<const Transform> nodes){return unit_frame_poses.update(owner,lod,model,nodes);};
    SourceLiveModelPoseProviderTu15 selected_unit_pose_provider;
    const auto *repeat_selected_skins=std::getenv("HALO2_REFERENCE_REPEAT_SELECTED_SKINS");
    if(!(repeat_unit_skins && *repeat_unit_skins) && !(repeat_selected_skins && *repeat_selected_skins))
      selected_unit_pose_provider=[&](std::uint32_t owner,unsigned lod,const RenderModel &model,
          std::span<const Transform> nodes){return unit_frame_poses.update_preserving_effects(owner,lod,model,nodes);};
    const SourceLiveModelDeferredPoseProviderTu15 deferred_unit_pose_provider=
      [&](std::uint32_t owner,unsigned lod,const RenderModel &model,std::span<const Transform> nodes) {
        return unit_frame_poses.update_deferred(owner,lod,model,nodes).pose;
      };
    const SourceLiveModelPoseProviderTu15 deferred_selected_pose_provider=
      [&](std::uint32_t owner,unsigned lod,const RenderModel &model,std::span<const Transform> nodes) {
        return unit_frame_poses.update_deferred(owner,lod,model,nodes,true).pose.vertices;
      };
    const auto *repeat_vehicle_skins=std::getenv("HALO2_REFERENCE_REPEAT_VEHICLE_SKINS");
    const bool reuse_vehicle_skins=!(repeat_vehicle_skins && *repeat_vehicle_skins);
    // select() reads only the LOD it chooses; the default poses that bank
    // alone. This switch poses all six resident LOD banks as before.
    const auto *object_lod_skins=std::getenv("HALO2_REFERENCE_OBJECT_LOD_SKINS");
    const bool reference_object_lod_skins=object_lod_skins && *object_lod_skins;
    const auto completed_model=[&](std::uint32_t model)->const RenderModel* {
      for(const auto &[key,body]:species)if(body.player.render_model==model)return &body.player.model;
      for(const auto &[key,weapon]:body_guns)if(weapon.gun.render_model==model)return &weapon.gun.model;
      return nullptr;
    };
    const SourceWorldInterpolation::ModelProvider source_world_model=[&](std::uint32_t owner)->const RenderModel* {
      std::uint32_t definition=null_datum;
      const auto admit=[&](std::uint32_t object,std::uint32_t value) {
        if(object!=owner)return;
        if(definition!=null_datum && definition!=value)
          throw FormatError("Native WORLD owner has conflicting admitted model definitions");
        definition=value;
      };
      for(const auto &player:current.players)if(player.connected) {
        if(!player.dead)admit(player.source_unit,biped_for(player).datum);
        admit(player.source_weapon,player.weapon);
        admit(player.secondary_weapon.datum,player.secondary_weapon.definition);
      }
      for(const auto &corpse:current.corpses)admit(corpse.object,corpse.definition);
      for(const auto &weapon:current.world_weapons)admit(weapon.handle,weapon.definition);
      for(const auto &vehicle:current.world_vehicles) {
        if(vehicle.placement>=content.objects.size())throw FormatError("Native WORLD vehicle lost its authored definition");
        admit(vehicle.handle,content.objects[vehicle.placement].definition);
      }
      for(const auto &root:current.source_scene_roots)admit(root.object,root.definition);
      for(const auto &object:current.source_dynamic_objects)admit(object.object,object.definition);
      if(definition==null_datum)return nullptr;
      auto found=source_world_hierarchies.find(definition);
      if(found==source_world_hierarchies.end())found=source_world_hierarchies.emplace(
          definition,cache.load_model_hierarchy(definition)).first;
      return &found->second;
    };
    const auto replica_world_nodes=[&](std::uint32_t owner) {
      return source_body_interpolation.sample(owner,blend,source_world_model);
    };
    struct CompletedWorldGeometry {
      std::uint32_t model{};unsigned lod{};std::vector<unsigned> sections;
      // Diagnostic provenance copied from the completed collector, not read
      // again from live source memory: context, record address and model row.
      std::optional<std::array<std::uint32_t,3>> collector_debug;
    };
    std::array<std::map<std::uint32_t,CompletedWorldGeometry>,4> completed_world_geometry;
    const auto selected_world_vertices=[&](const Visual& visual,std::uint32_t owner,unsigned view,
                                          std::span<const Transform> nodes,std::string_view caller)
        ->std::optional<std::vector<Vertex>> {
      const auto found=completed_world_geometry.at(view).find(owner);
      CompletedWorldGeometry selected;
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
      if(const auto* reference=held_pose_shading_reference(visual.definition))
        selected={reference->render_model,reference->lod,reference->sections};
      else
#endif
      {
        if(found==completed_world_geometry.at(view).end())return std::nullopt;
        selected=found->second;
      }
      if(!visual.world_lods || visual.render_model!=selected.model || selected.lod>=6) {
        std::ostringstream diagnostic;
        diagnostic<<"Third-person draw lost the original selected model/LOD"
            <<" frame="<<frames<<" tick="<<current.tick<<" view="<<view<<" owner="<<owner
            <<" caller="<<caller<<" visual_definition="<<visual.definition
            <<" visual_render_model="<<visual.render_model<<" selected_model="<<selected.model
            <<" selected_lod="<<selected.lod<<" has_world_lods="<<bool(visual.world_lods);
        if(selected.collector_debug)diagnostic<<" collector_context="<<selected.collector_debug->at(0)
            <<" collector_address="<<selected.collector_debug->at(1)<<" collector_row="<<selected.collector_debug->at(2);
        else diagnostic<<" collector_context=unavailable collector_address=unavailable collector_row=unavailable";
        throw FormatError(diagnostic.str());
      }
      // The collector retains region order and can repeat a section. Its NONE
      // entries were omitted while copying; no preview LOD supplies geometry.
      const auto reused=unit_frame_poses.reuses();
      auto vertices=source_live_model_selected_vertices_tu15(owner,selected.lod,
          visual.world_lods->at(selected.lod),nodes,selected.sections,
          live_unit_gpu.at(view).contains(owner)?deferred_selected_pose_provider:selected_unit_pose_provider);
      ++timing.selected_skin_requests;
      timing.selected_skin_reuses+=unit_frame_poses.reuses()-reused;
      return vertices;
    };
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
    if(measure_host_timings)
      timing.shared_pre_view_ns=std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(host_stamp()-measured_at).count());
    for (unsigned i = 0; i < ids.size(); ++i) {
      const unsigned id = ids[i];
      auto p = shown.players[id];
      const auto &v=species_for(p);
      const auto &biped=biped_for(p);
      const auto selected_weapon=content.weapons.find(p.weapon);
      const auto *weapon=selected_weapon==content.weapons.end()?nullptr:&selected_weapon->second;
      const auto *gun=p.weapon==null_datum?nullptr:&v.guns.at(i*2);
      const auto &hands=v.hands;
      auto &hud=*huds.at({source_session_player_representation(p.source_configuration,source_game_type),p.weapon});
      auto &unit_hud=*unit_huds.at(source_session_player_representation(p.source_configuration,source_game_type));
      // TU1.5 consumes deltas between raw PC samples. A new source unit owns
      // its look initialization; rebasing raw input to that processed angle
      // would deliver another turn on the following command.
      if (dead[i] && !p.dead && !source_relative_look()) {
        yaw[i] = p.yaw;
        pitch[i] = 0;
      }
      dead[i] = p.dead;
      if((!options.source_object_world && !source_relative_look()) || p.source_unit==null_datum) {p.yaw=yaw[i];p.pitch=pitch[i];}
      // The original client owned its look. Draw the host's processed angles
      // plus raw look it has not consumed yet, so aiming answers this frame
      // instead of a round trip later. Seats keep host angles: 18A300's seat
      // limits and auto-level run only on the host.
      else if(const auto consumed=live_client()&&!p.dead&&p.source_parent==null_datum?
          live_client()->consumed_look(i):std::nullopt) {
        const auto &latest=newest_replica().players[id];
        p.yaw=std::remainder(latest.yaw+std::remainder(yaw[i]-(*consumed)[0],6.283185307f),6.283185307f);
        p.pitch=std::clamp(latest.pitch+(pitch[i]-(*consumed)[1]),-1.54f,1.54f);
      }
      const auto &fov_settings=platform.desktop->display_settings;
      const float unzoomed_fov=gameplay_field_of_view(fov_settings.field_of_view);
      float horizontal_fov=biped.camera_field_of_view;
      if(scope_constants && content.binocular_zoom && !reference_player) {
        const auto selected=content.weapons.find(p.weapon);
        if(selected!=content.weapons.end()) {
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
      predict_view_impacts(i,p,biped,shown);
      const auto &seated_view=p.source_vehicle_camera;
      const bool vehicle_view=seated_view.has_value();
      const bool third_person_vehicle_view=vehicle_view && seated_view->camera_mode!=0;
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
      } else if(p.source_death_camera) {
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
      const auto split=fov_settings.two_player_split;
      const auto layout=source_view_grid(options.local_players,i,
          hud_layout_widescreen(options.local_players,widescreen,split));
      if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics()) {
        if(source_particle_world)source_particle_world->render_model_view([&](auto &source) {
          source_particle_draws.publish_observer(source,i,
              camera.position,camera.forward,camera.up,horizontal_fov);
          source_observer_ready[i]=true;
        });
        else live_world()->source_objects->publish_native_observer_position(i,camera.position);
      }
      auto source_angles=source_camera_angles(horizontal_fov,1.3333333730697632f,
          layout.grid,layout.span);
      float presentation_fov=horizontal_fov;
      const bool user_fov=!p.dead && !options.capture_camera && !options.capture_projection && !reference_player;
      if(user_fov) {
        presentation_fov=vehicle_view?vehicle_observer_field_of_view(horizontal_fov,
            content.original_data->death_camera_constants->default_fov,fov_settings.vehicle_field_of_view):
            gameplay_observer_field_of_view(horizontal_fov,biped.camera_field_of_view,fov_settings.field_of_view);
        if(!vehicle_view) {
          // MCC 6F1700/7DFF10 also uses the modern scale for the authored
          // screen-offset rotation and its HUD aim displacement.
          const auto aim=player_camera(p,biped,presentation_fov,content.player_screen_offset,gameplay_camera_angle_scale);
          camera.forward=aim.forward;camera.up=aim.up;
        }
        const auto view_layout=hud_viewport(options.local_players,i,widescreen,split);
        const auto &r=view_layout.outer;
        const auto view_extent=full_surface_viewport({r.left,r.top,r.right-r.left,r.bottom-r.top},drawable_w,drawable_h);
        const float aspect=view_extent.width/view_extent.height;
        source_angles=vehicle_view?vehicle_camera_angles(horizontal_fov,
            content.original_data->death_camera_constants->default_fov,fov_settings.vehicle_field_of_view,
            layout.grid,layout.span):gameplay_camera_angles(presentation_fov,unzoomed_fov,aspect);
        if(i==keyboard_view && !vehicle_view)
          mouse_look.projection(source_angles.projection_angle,
              gameplay_camera_angles(unzoomed_fov,unzoomed_fov,aspect).projection_angle);
      }
      if(i==keyboard_view && (!user_fov || vehicle_view))mouse_look.reset();
      if(options.capture_projection){source_angles.projection_angle=options.capture_projection->x;
        source_angles.lighting_distance_scale=options.capture_projection->y;}
      if(reference_player){source_angles.projection_angle=reference_player->camera_vertical_angle;
        source_angles.lighting_distance_scale=reference_player->camera_scale;}
      const float foreground_projection_angle=user_fov?
          gameplay_first_person_camera_angle(biped.camera_field_of_view,layout):source_angles.projection_angle;
      camera.field_of_view=source_angles.projection_angle*57.295779513f;
      object_audio_views[i]=SourceObjectAudioViewTu15{camera.position,std::int16_t(atmosphere.cluster_at(camera.position))};
      if(audio && i==0)try {
        //18BB80 chooses the first actual local listener. Native cameras use
        // the same submitted position and literal14A280 cluster traversal.
        const auto targets=multiplayer_ambient_targets(audio_bsp,atmosphere,camera.position,
            atmosphere.cluster_at(camera.position),live_match().simulation().weather().current,
            live_match().simulation().weather().stopped==0);
        audio->ambient(targets,audio_elapsed);
      }catch(const std::exception&e){if(!ambient_unavailable_reported){ambient_unavailable_reported=true;
        std::cerr<<"Multiplayer ambience: "<<e.what()<<'\n';}}
      const auto view_forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
      const auto fog=atmosphere.sample(camera.position,view_forward);
      auto viewport_layout=hud_viewport(options.local_players,i,widescreen,split);
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
      const auto retained_safe=held_pose_shading_reference_safe_rectangle();
      const auto retained_outer=held_pose_shading_reference_outer_rectangle();
      const float scale_x=640.f/float(drawable_w),scale_y=480.f/float(drawable_h);
      viewport_layout.outer={.top=retained_outer.top*scale_y,.left=retained_outer.left*scale_x,
          .bottom=(retained_outer.top+retained_outer.height)*scale_y,
          .right=(retained_outer.left+retained_outer.width)*scale_x};
      viewport_layout.safe={.top=retained_safe.top*scale_y,.left=retained_safe.left*scale_x,
          .bottom=(retained_safe.top+retained_safe.height)*scale_y,
          .right=(retained_safe.left+retained_safe.width)*scale_x};
#endif
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
      const SourceViewProjectionInput render_camera_input{camera.position,view_forward,
          camera.oriented?camera.up:Vec3{0,0,1},.015625f,1024.f,{},
          frustum_projection.projection_program};
      const auto render_camera=source_view_projection(render_camera_input);
      SourceViewShaderConstants source_view_constants;
      source_view_constants.world=source_view_shader_constants(render_camera,1024.f,false);
      auto foreground_render_camera=render_camera;
      if(user_fov) {
        auto foreground_input=frustum_projection_input;
        foreground_input.projection_angle=foreground_projection_angle;
        const auto foreground_projection=source_frustum_projection(foreground_input);
        foreground_render_camera=source_view_projection({camera.position,view_forward,
            camera.oriented?camera.up:Vec3{0,0,1},.015625f,1024.f,{},
            foreground_projection.projection_program});
      }
      const auto foreground_constants=source_view_shader_constants(foreground_render_camera,1024.f,true);
      std::copy_n(foreground_constants.begin(),4,source_view_constants.foreground.begin());
      source_view_constants.reflection=source_view_constants.world;
      const auto source_camera=source_frustum_camera({camera.position,view_forward,
          camera.oriented?camera.up:Vec3{0,0,1},.015625f,1024.f,
          frustum_projection.clipping_rectangle()});
      const auto visibility=source_portal_visibility(source_portals,
          std::span{&source_camera,1},fog.cluster,source_portal_mask.words);
      bool source_main_commands_prepared{};
      if(source_particle_world && source_lens_queries)
        source_particle_world->render_model_view([&](auto &source){source_lens_queries->prepare_view(source,i);});
      if(replica_lens_frame)
        native_lens->prepare_view(i,camera.position,view_forward,camera.oriented?camera.up:render_camera.camera_to_world.basis[2],
            source_angles.projection_angle,render_camera,source_rectangle(rect),drawable_w,drawable_h);
      if(source_model_world && source_model_world==source_particle_world) {
        MuzzleEffectView source_view;
        source_view.position=camera.position;source_view.forward=view_forward;
        source_view.up=camera.oriented?camera.up:Vec3{0,0,1};
        source_particle_world->render_model_view([&](auto &source) {
          source_particle_draws.prepare_view(source,visibility,source_view,i,
              source_angles.projection_angle,source_angles.lighting_distance_scale);
        });
      } else if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics())
        live_world()->source_objects->publish_render_visibility(visibility);
      //36B50 advances intensity/clock only through view0. Every local view
      // consumes that same elapsed value and its actual clipped cluster bank.
      live_match().advance_weather_view(weather_realtime,std::int16_t(i));
      const auto weather_view=source_weather_view_inputs(std::int16_t(i),camera.position,
          view_forward,source_angles.projection_angle,render_camera);
      weather_world.admit_view(weather_view,atmosphere,fog.cluster,visibility,
          live_match().simulation().weather());
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
        weather_step=weather_world.visit_view(i,live_match().simulation().weather(),
            {std::int16_t(options.local_players),live_match().simulation().weather_clock().elapsed,
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
            const auto packet=resource.submission(live_match().simulation().weather_clock().elapsed,
                std::int16_t(options.local_players),std::int16_t(i));
            vertices+=packet.vertices.size();
            for(auto byte:std::as_bytes(std::span(packet.vertices)))
              packet_hash=(packet_hash^std::to_integer<unsigned>(byte))*1099511628211ull;
          }
        }
        std::cout<<"Source weather simulation: frame="<<frames<<" view="<<i
            <<" elapsed="<<live_match().simulation().weather_clock().elapsed
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
#if HALO2_SOURCE_DECALS_AVAILABLE
      if(cached_decals.active() && ((!live_client() && live_world()->source_objects) || current.source_decals)) {
        std::array<ShaderVector,192> constants{};
        const auto view_constants=source_view_shader_bank(render_camera,1024.f,camera.position,physical(rect),false);
        std::copy(view_constants.begin(),view_constants.end(),constants.begin());
        const auto fog_constants=bsp_fog_constants(fog,camera.position,view_forward);
        std::copy(fog_constants.begin(),fog_constants.end(),constants.begin()+12);
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
        const ShaderVector viewport{float(physical(rect).left),float(physical(rect).top),
            float(physical(rect).width),float(physical(rect).height)};
        const ShaderVector clip{0,16777215.f,depth.minimum_w,depth.maximum_w};
        if(!live_client() && live_world()->source_objects)
          source_decals[i]=cached_decals.draw(live_world()->source_objects->memory(),bsp_views[i],constants,viewport,clip);
        else if(current.source_decals)
          source_decals[i]=cached_decals.draw(*current.source_decals,bsp_views[i],constants,viewport,clip);

        if(options.frames || options.capture_tick) {
          std::size_t quads{};for(const auto &draw:source_decals[i])quads+=draw.quads.size();
          std::cout<<"Source cached decals: frame="<<frames<<" view="<<i
              <<" draws="<<source_decals[i].size()<<" quads="<<quads<<'\n';
        }
      }
#endif
#if HALO2_SOURCE_CONTRAILS_AVAILABLE
      if(current.source_contrails.authoritative) {
        std::array<ShaderVector,192> constants{};
        const auto view_constants=source_view_shader_bank(render_camera,1024.f,camera.position,physical(rect),false);
        std::copy(view_constants.begin(),view_constants.end(),constants.begin());
        const auto fog_constants=bsp_fog_constants(fog,camera.position,view_forward);
        std::copy(fog_constants.begin(),fog_constants.end(),constants.begin()+12);
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
        contrail_draws[i]=source_contrails.draw(current.source_contrails,camera.position,view_forward,constants,
            {float(physical(rect).left),float(physical(rect).top),float(physical(rect).width),float(physical(rect).height)},
            {0,16777215.f,depth.minimum_w,depth.maximum_w});
        if((options.frames || options.capture_tick) && !contrail_draws[i].empty())
          std::cout<<"Source contrails: frame="<<frames<<" view="<<i<<" draws="<<contrail_draws[i].size()<<'\n';
      }
#endif
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
      const bool native_light_pool=source_model_world &&
          (render_settings.graphics_mode==pstencil::GraphicsMode::Original ||
           render_settings.graphics_mode==pstencil::GraphicsMode::PathTracing);
      if(native_light_pool) {
        // Selection follows this view's actualC35D0 in the main collector.
        // Do not advance the separate fallback cache before that producer.
        effect_light_views[i]={};effect_light_receivers[i].clear();
      }else {
        effect_light_views[i]=world_effects.light_view(camera.position,light_collection,
            light_candidates,std::bit_cast<std::int32_t>(std::uint32_t(frames)+1));
        effect_light_receivers[i]=world_effects.light_receivers(effect_light_views[i],
            source_bsp_geometry,containing_fog,&bsp_views[i]);
      }
      const auto collect_lights=[&](SourceEffectLightModelViewTu15* models) {
        raw_effect_lights[i]=bsp_effect_lights.draw(effect_light_views[i],effect_light_receivers[i],
            bsp_views[i],render_camera,fog,camera.position,source_rectangle(rect),
            {std::uint32_t(drawable_w),std::uint32_t(drawable_h)},options.local_players,models,
            render_settings.graphics_mode==pstencil::GraphicsMode::pStencil?&pstencil_point_casters[i]:nullptr);
        if(render_settings.graphics_mode==pstencil::GraphicsMode::pStencil) {
          auto point_view=pstencil_point_lights.view(camera.position,light_collection,light_candidates,
              std::bit_cast<std::int32_t>(std::uint32_t(frames)+1));
          auto point_receivers=pstencil_point_lights.receivers(point_view,source_bsp_geometry,containing_fog,&bsp_views[i]);
          std::vector<EffectLightRawDraw> point_draws;
          if(!point_receivers.empty()) {
            std::optional<SourceEffectLightModelViewTu15> point_models;
            if(models) {
              point_models.emplace(*models);point_models->main_draws={};point_models->main_collected={};
              point_models->context.built={};
            }
            point_draws=bsp_effect_lights.draw(point_view,point_receivers,bsp_views[i],render_camera,fog,
                camera.position,source_rectangle(rect),{std::uint32_t(drawable_w),std::uint32_t(drawable_h)},
                options.local_players,point_models?&*point_models:nullptr,&pstencil_point_casters[i],1);
          }
          for(auto& draw:point_draws)draw.pstencil_light_domain=1;
          raw_effect_lights[i].insert(raw_effect_lights[i].end(),
              std::make_move_iterator(point_draws.begin()),std::make_move_iterator(point_draws.end()));
          if(options.capture_tick && !point_draws.empty())
            for(const auto& light:pstencil_point_lights.lights())
              std::cout<<"pStencil direct light: definition="<<light.definition<<" owner="<<light.effect_owner
                <<" position="<<light.position.x<<','<<light.position.y<<','<<light.position.z
                <<" diffuse="<<light.state.diffuse.x<<','<<light.state.diffuse.y<<','<<light.state.diffuse.z
                <<" draws="<<point_draws.size()<<'\n';
        }
      };
      if(source_model_world)source_model_world->render_model_view([&](PhysicsLiveWorldTu15& source) {
        std::vector<std::uint32_t> detail_players;
        for(const auto& player:current.players)if(body_present(player))detail_players.push_back(body_handle(player));
        pstencil::PlayerDetail player_detail(source,detail_players,
            render_settings.graphics_mode==pstencil::GraphicsMode::pStencil);
        // This is the actual local render-user identity already supplied to
        // selection below. Main40E80/3E9B0/177160 consume it before selection.
        source_live_shadow_main_user_tu15(source,std::int32_t(i));
        source_live_shadow_main_location_tu15(source,fog.cluster);
        source_live_shadow_main_collection_fog_tu15(source,fog);
        source_live_shadow_main_camera_tu15(source,render_camera_input,source_rectangle(rect),0,false);
        SourceLiveModelBaseSelectionTu15 selected;
        SourceLiveModelDrawContextTu15 context;
        context.entry.distance_scale=source_angles.lighting_distance_scale;
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
        context.entry.coverage_multiplier=held_pose_shading_reference_policy().coverage_multiplier;
#else
        context.entry.coverage_multiplier=native_detail_policy(render_settings.detail,options.local_players).coverage_multiplier;
#endif
        context.forward=view_forward;context.view=i;context.local_players=options.local_players;
        context.view_modifier=frustum_projection.view_modifier;
        context.built=[&](const auto& collector){
          selected.remember(collector);
          const Bytes c(collector.bytes);
          const auto count=std::bit_cast<std::int16_t>(c.u16(0x166u));
          if(count<=0)return;
          if(count>5)throw FormatError("Completed TU1.5 render pose exceeds five model owners");
          // Cached first-person rows can share a body or weapon owner. They
          // still belong to selected's original main draw handoff above, but
          // only WORLD output owns completed WORLD nodes and section choices.
          if(std::to_integer<unsigned>(c.slice(0x16cu,1).front()))return;
          for(unsigned n=0;n<unsigned(count);++n) {
            const auto owner=c.u32(0x34u+n*4u);
            if(auto nodes=source.rendered_world_nodes(owner)) {
              const auto model=c.u32(0x5cu+n*4u);
              if(options.node_observer)options.node_observer(i,current.tick,
                  {MultiplayerNodeStage::world_completed,owner,model,0,blend,*nodes});
              std::uint32_t observed_parent=null_datum;
              unsigned observed_parent_node{};bool observed_parent_known{};
              if(const auto *skeleton=completed_model(model)) {
                auto &history=source_render_interpolation[{owner,model}];
                std::optional<Transform> source_parent,shown_parent;
                const auto object=source.source_object_body(owner);
                const Bytes fields(source.memory().view(object,0x19));
                const auto parent=fields.u32(0x14);
                const auto parent_node=std::to_integer<unsigned>(fields.slice(0x18,1).front());
                observed_parent=parent;observed_parent_node=parent_node;observed_parent_known=true;
                // B93B0 retains the exact parent and selected node. Interpolate
                // carried models in that hand's frame so a rotating wrist
                // cannot leave its independently lerped weapon behind.
                if(std::any_of(current.players.begin(),current.players.end(),
                    [&](const auto &p){return parent!=null_datum && p.source_unit==parent;})) {
                  const auto shown=completed_render_nodes.find(parent);
                  const auto raw=source.rendered_world_nodes(parent);
                  if(raw && shown!=completed_render_nodes.end() && parent_node<raw->size() && parent_node<shown->second.size()) {
                    source_parent=raw->at(parent_node);shown_parent=shown->second.at(parent_node);
                  }
                }
                history.capture(current.tick,*nodes,source_parent);
                *nodes=history.sample(*skeleton,blend,shown_parent);
              }
              if(options.node_observer)options.node_observer(i,current.tick,
                  {MultiplayerNodeStage::world_interpolated,owner,model,0,blend,*nodes,
                      observed_parent,observed_parent_node,observed_parent_known});
              completed_render_nodes.insert_or_assign(owner,std::move(*nodes));
            }
            const auto regions=c.u32(0x18u+n*4u);
            const auto lod=c.u16(0x168u);
            if(regions>16 || lod>=6)throw FormatError("Completed TU1.5 geometry exceeds its model/LOD bank");
            CompletedWorldGeometry selected_geometry{c.u32(0x5cu+n*4u),lod,{}};
            selected_geometry.collector_debug=std::array<std::uint32_t,3>{c.u32(0x48u+n*4u),collector.address,n};
            for(unsigned region=0;region<regions;++region) {
              const auto section=std::to_integer<unsigned>(c.slice(0x75u+n*16u+region,1).front());
              if(section!=255)selected_geometry.sections.push_back(section);
            }
            completed_world_geometry.at(i).insert_or_assign(owner,std::move(selected_geometry));
          }
        };
        if(options.capture_third_person && body_present(current.players[id]))
          for(const auto owner:{body_handle(current.players[id]),current.players[id].source_weapon,
              current.players[id].secondary_weapon.datum})if(owner!=null_datum) {
            const auto object=source.source_object_body(owner);
            const Bytes b(source.memory().view(object,0x40));
            const Vec3 center{b.f32(0x30),b.f32(0x34),b.f32(0x38)};
            const auto metric=source_geometry_lod_distance(center,camera.position,source_angles.lighting_distance_scale);
            const auto coverage=source_shader_object_coverage(center,b.f32(0x3c),
                context.entry.coverage_multiplier,source_shader_coverage_camera(render_camera));
            context.built(source.build_world_model(owner,i,
                options.local_players,frustum_projection.view_modifier,metric,coverage));
          }
        SourceObjectAdmissionInput input;input.containing_fog=containing_fog;
        SourceEffectLightModelViewTu15 models{source,source.admit_render_objects(visibility,input,bsp_views[i].associations),context,containing_fog};
        models.visibility=&visibility;
        if(native_light_pool)models.current_lights=[&,i] {
          auto lights=world_effects.source_light_view(source,options.bsp,light_collection,light_candidates,
              std::bit_cast<std::int32_t>(std::uint32_t(frames)+1));
          auto receivers=world_effects.source_light_receivers(source,options.bsp,lights,
              source_bsp_geometry,containing_fog,&bsp_views[i]);
          effect_light_views[i]=lights;effect_light_receivers[i]=receivers;
          if(render_settings.graphics_mode==pstencil::GraphicsMode::PathTracing)
            path_tracing_lights[i]=path_tracing::source_lights(source,lights.records);
          return std::pair{std::move(lights),std::move(receivers)};
        };
        models.diagnostic_native_light_seed=options.diagnostic_native_light_seed;
        models.view_calls=[&,i](bool before,auto entry,
            const auto& abi,const auto& memory) {
          if(!before && entry==0x3d2b0u && !cached_native_fp_groups.at(i)) {
            // Original2BABC caches this actual camera's FP group before the
            // main WORLD collector.3D4E0 later reads those exact matrices;
            // re-running168DED after BF640 would copy its mutated SIM bank.
            cached_native_fp_groups[i]=source_cached_first_person_models_tu15(cache,memory,current.players[id].source_unit);
          }
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(before,i,current.tick,0u,entry,abi,memory);
        };
        if(options.shadow_admission_observer)models.light_commands=[&,i](bool before,auto light,
            const auto& abi,const auto& memory) {
          options.shadow_admission_observer(before,i,current.tick,light,0x40e80u,abi,memory);
        };
        if(options.effect_light_command_observer)models.light_command_observer=[&,i](const auto& observed) {
          options.effect_light_command_observer(i,current.tick,observed);
        };
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
        const auto source_policy=held_pose_shading_reference_policy();
#else
        const auto source_policy=native_detail_policy(render_settings.detail,options.local_players);
#endif
        const SourceModelShaderView source_shader_view{source_shader_coverage_camera(render_camera),
          source_shader_lod_view(frustum_projection_input.outer[0],frustum_projection_input.outer[2],false,false),
          source_policy.coverage_multiplier,source_policy.effect_scale,frustum_projection.view_modifier};
        models.main_collected=[&](const auto& bsp_main,const auto& main,const auto& main_context) {
          // Original ordinary queues precede main culling/command production.
          // Publishing them only inside selection leaves dead prior-frame
          // owners visible to40E80's attachment producer.
          source_live_shadow_main_queues_tu15(source,bsp_main,main);
          // Ordinary44710 consumes the now published collected main bank
          // before335D0 selection and secondary shadow/light queries.
          auto main_cull=source.prepare_source_call();
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(true,i,current.tick,0u,0x44710u,main_cull,source.memory());
          const auto cull_begin=host_stamp();
          const auto translated_cull=[&]{source.run_actual(0x44710u,main_cull);};
          if(models.main_cull)models.main_cull(translated_cull);else translated_cull();
          host_returned(cull_begin,timing.native_view[i].main_cull);
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(false,i,current.tick,0u,0x44710u,main_cull,source.memory());

          // Normal13768→2BCB0 supplies argument5=0 to the whole40E80
          // producer after main44710 and before335D0. Only bank0 resets in
          // this actual mode0 context; retained bank2 belongs to later owners.
          auto main_commands=source.prepare_source_call();main_commands.gp[0]=0;
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(true,i,current.tick,0u,0x40e80u,main_commands,source.memory());
          const auto commands_begin=host_stamp();
          const auto translated_commands=[&]{source.run_actual(0x40e80u,main_commands);};
          if(models.main_commands)models.main_commands(translated_commands);else translated_commands();
          host_returned(commands_begin,timing.native_view[i].main_commands);
          source_main_commands_prepared=source_model_world==source_particle_world;
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(false,i,current.tick,0u,0x40e80u,main_commands,source.memory());

          if(options.shadow_synchronization_observer)options.shadow_synchronization_observer(i,current.tick,source);
          if(reference_shadow_frame) {
            if(i || reference_shadow_targets || reference_shadow_phases)
              throw FormatError("Live shadow GPU reference repeated or split its ordinary source view");
            reference_shadow_targets.emplace(renderer.target_resources().acquire());
            //Explicit UNUSED finite residue only. The private phase accessor
            //rejects every selected program that could consume these words.
            const SourceLiveShadowPhaseResidueTu15 residue{MenuWorldSourceConstants{},std::array<float,2>{0,0}};
            reference_shadow_phases.emplace(source_live_shadow_phase_inputs_tu15(
                source.memory(),*reference_shadow_targets,residue));
          }
          const auto selection_begin=host_stamp();
          live_shadow_selections[i]=source_live_shadow_selection_tu15(source,bsp_main,main,main_context,
              std::int32_t(i),options.shadow_selection_observer?SourceShadowSelectionObserverTu15{
                [&](bool before,const auto& abi,const auto& memory) {
                  options.shadow_selection_observer(before,i,current.tick,abi,memory);
                }}:SourceShadowSelectionObserverTu15{});
          host_returned(selection_begin,timing.native_view[i].shadow_selection);
          const auto roots_begin=host_stamp();
          live_shadow_roots[i]=source_live_shadow_root_frames_tu15(source,live_shadow_selections[i].selected,
              options.shadow_admission_observer?SourceShadowAdmissionObserverTu15{
                [&](bool before,auto entry,auto owner,const auto& abi,const auto& memory) {
                  options.shadow_admission_observer(before,i,current.tick,owner,entry,abi,memory);
                }}:SourceShadowAdmissionObserverTu15{},
              (options.shadow_root_completed || reference_shadow_frame)?SourceShadowRootCompletedTu15{
                [&](auto& world,const auto& root) {
                  if(reference_shadow_frame && root.route==SourceLiveShadowRootRouteTu15::projected && root.commands) {
                    if(!reference_shadow_targets || !reference_shadow_phases || !root.uniforms)
                      throw FormatError("Live shadow GPU root lost its actual phase/lease/uniform owner");
                    reference_shadow_roots.push_back(source_shadow_scene->draw(world,root,*root.uniforms,
                        *reference_shadow_phases,*reference_shadow_targets));
                  }
                  //Observer runs after the owned draw copied actual current
                  //palettes; it cannot make a later query's palette current.
                  if(options.shadow_root_completed)options.shadow_root_completed(i,current.tick,world,root);
                }}:SourceShadowRootCompletedTu15{},models.secondary_commands);
          host_returned(roots_begin,timing.native_view[i].shadow_roots);
          if(reference_shadow_frame) {
            reference_shadow_root_views.reserve(reference_shadow_roots.size());
            for(const auto& root:reference_shadow_roots)reference_shadow_root_views.push_back(root.view());
            reference_shadow_plan=SourceShadowRawPlan{reference_shadow_phases->camera(*reference_shadow_targets),
                reference_shadow_root_views};
            reference_shadow_stats=validate_source_shadow_raw_plan(*reference_shadow_plan,source_shadow_scene->resident());
          }
          if(options.shadow_root_frame_observer)
            options.shadow_root_frame_observer(i,current.tick,live_shadow_roots[i]);
          if(options.frames || options.capture_tick) {
            const auto& selected=live_shadow_selections[i];
            std::cout<<"Source shadow selection: frame="<<selected.frame<<" tick="<<current.tick
                <<" view="<<i<<" queue="<<main.objects.size()<<" roots="<<selected.selected.size()
                <<" history="<<selected.history_after.count<<" fixed_enabled="<<selected.fixed_object_shadows_enabled
                <<" lightmap="<<selected.lightmap_enabled<<" cinematic="<<selected.cinematic_enabled<<'\n';
          }
        };
        models.main_draws=[&](const auto& units) {
          const auto snapshots=selected.copy(source,units,[&](const auto& unit){return source_model_roots.contains(unit.words[1]);});
          std::vector<SourceTransparentCommand> groups;
          source_transparent_append_groups(std::span(units.sections).first(units.section_highwater),groups,0);
          for(const auto ordinal:source_transparent_sort(groups,camera.position)) {
            const auto &group=groups[ordinal];if(group.callback!=0x41cc0u)continue;
            auto &owners=camouflage_groups[i].emplace_back();
            for(unsigned member=0;member<group.payload[0];++member)for(const auto &draw:snapshots) {
              if(((draw.unit.words[3]>>4)&31u)!=group.payload[member+1])continue;
              for(const auto &player:current.players)if(draw.object==player.source_unit ||
                  draw.object==player.source_weapon || draw.object==player.secondary_weapon.datum)
                if(std::find(owners.begin(),owners.end(),player.source_unit)==owners.end())owners.push_back(player.source_unit);
            }
          }
          std::map<std::uint32_t,std::vector<SourceLiveModelBaseDrawTu15>> owners;
          for(const auto& draw:snapshots) {
            owners[draw.object].push_back(draw);
          }
          for(auto& [owner,draws]:owners) {
            if(const auto* audit=std::getenv("HALO2_CAPTURE_UNIT_ADMISSION_OWNER");
               options.capture_tick && audit && owner==std::stoul(audit))
              for(const auto& draw:draws)
                std::cout<<"Source unit admission audit: tick="<<current.tick<<" owner="<<owner
                    <<" class="<<draw.lighting_class<<" override_slot="<<unsigned(draw.override_slot)
                    <<" override_shader="<<draw.shader_override<<" flags="<<draw.unit.words[0]
                    <<" lod="<<draw.lod<<" section="<<(draw.unit.words[3]>>9&511u)<<'\n';
            const auto vehicle=std::find_if(current.world_vehicles.begin(),current.world_vehicles.end(),
                [&](const auto& v){return v.handle==owner;});
            const Visual* unit_visual=nullptr;
            bool gpu_character_owner{};
            std::set<std::uint32_t> separate_unit_overrides;
            const auto select_visual=[&](const Visual& candidate) {
              if(unit_visual && unit_visual!=&candidate)
                throw FormatError("Source unit base has more than one live presentation owner");
              unit_visual=&candidate;
            };
            for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
              const auto& player=shown.players[slot];
              if(!body_present(player) || !(options.capture_third_person || slot!=id || player.dead || third_person_vehicle_view))continue;
              if(owner==body_handle(player)) {
                const auto& body=species_for(player);select_visual(body.player);
                gpu_character_owner=true;
                // The completed main collector contains ordinary body units
                // and independent damage/overshield units. Overrides already
                // submitted by append_shields use that separate path.
                // Unknown overrides still reject the complete ordinary base.
                {
                  const auto& actual=current.players[slot];
                  const ShieldOverlayGeometry geometry{true,true,false,1,5,0};
                  const auto overlays=select_shield_overlays(biped_for(actual).vitality,
                      actual.damage_feedback.shield.visible,actual.shield,geometry);
                  for(const auto& overlay:overlays.selected()) {
                    const auto layer=body.body_shields.find(overlay.shader);
                    if(layer!=body.body_shields.end() && layer->second.shaders)
                      separate_unit_overrides.insert(overlay.shader);
                  }
                }
              }
              if(owner==player.source_weapon && player.weapon!=null_datum) {
                select_visual(body_guns.at(player.weapon).gun);gpu_character_owner=true;
              }
              if(owner==player.secondary_weapon.datum && player.secondary_weapon.definition!=null_datum) {
                select_visual(body_guns.at(player.secondary_weapon.definition).gun);gpu_character_owner=true;
              }
            }
            if(current.source_body_poses_authoritative)for(const auto& corpse:current.corpses)
              if(owner==corpse.object)
                select_visual(species.at({source_session_player_representation(corpse.configuration,source_game_type),rules.weapon}).player);
            for(const auto &weapon:current.world_weapons)if(weapon.handle==owner) {
              const auto visual=body_guns.find(weapon.definition);
              if(visual!=body_guns.end())select_visual(visual->second.gun);
            }
            if(const auto *audit=std::getenv("HALO2_MODEL_SKIN_DIAGNOSTICS");audit && *audit && !frames) {
              std::cout<<"GPU source unit classification: view="<<i<<" owner="<<owner
                  <<" character="<<gpu_character_owner<<" visual="<<(unit_visual?unit_visual->model.name:std::string{})
                  <<" draws="<<draws.size()<<" shield_overrides="<<separate_unit_overrides.size()<<'\n';
            }
            if(unit_visual) {
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
              // This offline experiment supplies captured WORLD nodes and
              // ordered LOD sections below. The live source continuation has
              // a different simulated frame; submitting it would silently
              // replace the checked retained inputs with another pose/LOD.
              // Ordinary gameplay continues through the live owner unchanged.
              if(held_pose_shading_reference(unit_visual->definition))continue;
#endif
              if(std::any_of(draws.begin(),draws.end(),[&](const auto& draw) {
                  return draw.lighting_class!=2 || !(draw.unit.words[0]&1u) ||
                      (draw.override_slot!=255 && !separate_unit_overrides.contains(draw.shader_override));
              }))continue;
              std::vector<SourceLiveModelBaseDrawTu15> ordinary;
              for(const auto& draw:draws)if(draw.override_slot==255)ordinary.push_back(draw);
              const auto pose=completed_render_nodes.find(owner);
              if(!unit_visual->world_lods || !unit_visual->shaders || pose==completed_render_nodes.end())continue;
              // Posed CPU consumers retain the original skin. Presentation
              // packets are admitted only before ordinary redundant skins and
              // never substitute local records for shields/shadows/observers.
              const bool gpu_allowed=model_skin_gpu_requested() && renderer.can_skin_models() &&
                  gpu_character_owner &&
                  separate_unit_overrides.empty() && !options.animation_observer &&
                  !options.unit_source_draw_observer && !options.shield_layer_observer &&
                  !options.bsp_submission_observer && !bsp_reference_scene && !reference_shadow_frame &&
                  render_settings.graphics_mode!=pstencil::GraphicsMode::pStencil && render_settings.upscaler==Upscaler::Off &&
                  !std::getenv("HALO2_GROUP_INPUT_DIAGNOSTICS") && !std::getenv("HALO2_VIEW_DIAGNOSTICS") &&
                  !(repeat_unit_skins && *repeat_unit_skins) && !(repeat_selected_skins && *repeat_selected_skins);
              std::shared_ptr<const ModelSkinGeometry> gpu;
              auto vertices=source_live_model_base_vertices_tu15(owner,unit_visual->definition,
                  unit_visual->render_model,ordinary,*unit_visual->world_lods,*unit_visual->shaders,
                  pose->second,source_shader_view,unit_pose_provider,gpu_allowed?&gpu:nullptr,
                  gpu_allowed?deferred_unit_pose_provider:SourceLiveModelDeferredPoseProviderTu15{});
              if(const auto *audit=std::getenv("HALO2_MODEL_SKIN_DIAGNOSTICS");audit && *audit && !frames)
                std::cout<<"GPU source unit result: view="<<i<<" owner="<<owner
                    <<" allowed="<<gpu_allowed<<" deferred="<<bool(gpu)<<" records="<<(vertices?vertices->size():0)
                    <<" lod="<<(ordinary.empty()?0:ordinary.front().lod)<<'\n';
              if(!vertices)continue;
              if(gpu)live_unit_gpu[i].emplace(owner,std::move(gpu));
              live_unit_vertices[i].emplace(owner,std::move(*vertices));
              live_unit_base[i].emplace(owner,std::move(ordinary));
              continue;
            }
            // Prop/vehicle banks have no separate admitted override path.
            // Their ordinary admission remains atomic for the complete owner.
            if(std::any_of(draws.begin(),draws.end(),[](const auto& draw){
                return draw.lighting_class!=2 || draw.override_slot!=255 || !(draw.unit.words[0]&1u);
            }))continue;
            auto nodes=source_model_world->world_nodes(owner);
            if(std::any_of(current.source_body_poses.begin(),current.source_body_poses.end(),
                [&](const auto &pose){return pose.object==owner;}))
              nodes=source_body_interpolation.sample(owner,blend);
            else {
              const auto &objects=live_match().simulation().scene().objects();
              const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.handle==owner;});
              if(object!=objects.end() && object->solved_nodes)nodes=scene_interpolation.sample_nodes(*object,blend);
            }
            if(vehicle==current.world_vehicles.end()) {
              if(props.retain_source_view(i,draws,nodes,source_shader_view))
                live_prop_base[i].emplace(owner,std::move(draws));
              continue;
            }
            live_vehicle_nodes[i].emplace(owner,nodes);
            for(const auto& draw:draws)if(const auto prt=vehicle_prt_definitions.find(draw.unit.words[1]);prt!=vehicle_prt_definitions.end())
              live_vehicle_prt[i].emplace(std::pair{owner,draw.unit.words[3]>>9&511u},source_live_model_prt_bank_tu15(source,draw.unit,prt->second));
            live_vehicle_base[i].emplace(owner,std::move(draws));
          }
        };
        models.model_available=[&](const auto& unit) {
          const auto owner=Bytes(source.memory().view(unit.words[4],4)).u32(0);
          for(const auto* bank:{&live_vehicle_base[i],&live_prop_base[i],&live_unit_base[i]}) {
            const auto at=bank->find(owner);if(at==bank->end())continue;
            return std::any_of(at->second.begin(),at->second.end(),[&](const auto& base) {
              return source_live_model_base_matches_tu15(base,source,unit);
            });
          }
          return false;
        };
        const auto collection_begin=host_stamp();
        collect_lights(&models);
        host_returned(collection_begin,timing.native_view[i].model_collection);
      });
      else collect_lights(nullptr);
      if(options.effect_light_capture_request &&
         options.effect_light_capture_request(i,current.tick,raw_effect_lights[i],world_effects.lights()))
        options.capture_tick=current.tick?current.tick:options.capture_tick;
      if(source_model_world && (options.frames || options.capture_tick)) {
        std::size_t units{},packets{};
        for(const auto& [owner,draws]:live_vehicle_base[i])units+=draws.size();
        std::size_t prop_units{},prop_packets{};
        for(const auto& [owner,draws]:live_prop_base[i])prop_units+=draws.size();
        for(const auto& packet:raw_effect_lights[i])if(packet.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object) {
          const auto owner=packet.source.owner().generation.value();
          packets+=live_vehicle_base[i].contains(owner);prop_packets+=live_prop_base[i].contains(owner);
        }
        std::cout<<"Source vehicle base/light: frame="<<frames<<" view="<<i
            <<" owners="<<live_vehicle_base[i].size()<<" units="<<units<<" model_light_packets="<<packets<<'\n';
        std::cout<<"Source prop base/light: frame="<<frames<<" view="<<i
            <<" owners="<<live_prop_base[i].size()<<" units="<<prop_units<<" model_light_packets="<<prop_packets<<'\n';
        std::size_t unit_draws{},unit_packets{};
        for(const auto& [owner,draws]:live_unit_base[i])unit_draws+=draws.size();
        for(const auto& packet:raw_effect_lights[i])if(packet.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object)
          unit_packets+=live_unit_base[i].contains(packet.source.owner().generation.value());
        std::cout<<"Source unit base/light: frame="<<frames<<" view="<<i
            <<" owners="<<live_unit_base[i].size()<<" units="<<unit_draws<<" model_light_packets="<<unit_packets<<'\n';
      }
      const auto observer_aim=source_observer_aim(aim_forward,
          cross(aim_forward,{-std::sin(p.yaw),std::cos(p.yaw),0}),
          presentation_fov,content.player_screen_offset,1.3333333730697632f,
          user_fov?gameplay_camera_angle_scale:1.f);
      const auto aim_anchor=source_hud_aim_anchor({camera.position,view_forward,camera.up,
          source_angles.projection_angle,false},
          physical(rect),physical(viewport_layout.safe),
          seated_view?seated_view->aim_displacement:observer_aim.displacement,!p.dead);
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
        // Ordinary2B5B0 passes0 through2B770 to the132B0/12F80 camera.
        // Keep these World bounds unscaled with the camera bank below.
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
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
        if(source_particle_world)
          source_particle_world->render_model_view([&](auto &source) {
            auto draws=source_particle_draws.draw(source,visibility,effects_view,i,
                source_angles.projection_angle,source_angles.lighting_distance_scale,
                bool(source_lens_queries),source_main_commands_prepared);
            if(source_lens_queries)
              for(const auto &request:source_standalone_light_lenses_tu15(effect_light_views[i]))
                source_request_light_lens_tu15(source,request);
            if(source_lens_queries)
              lens_queries[i]=source_lens_queries->view(source,i,camera.position,view_forward,
                  effects_view.up,source_angles.projection_angle,render_camera,source_rectangle(rect),
                  drawable_w,drawable_h);
            if(source_lens_draws)
              lens_draws[i]=source_lens_draws->view(source,i,camera.position,view_forward,
                  effects_view.up,source_angles.projection_angle,render_camera,source_rectangle(rect),
                  drawable_w,drawable_h);
            if(options.frames || options.capture_tick)
              std::cout<<"Source world particles: frame="<<frames<<" view="<<i<<" draws="<<draws.size()<<'\n';
            if(source_lens_queries && (options.frames || options.capture_tick)) {
              const auto &memory=source.memory();const auto count=Bytes(memory.view(0x531fbcu,4)).u32(0);
              unsigned admitted{},alpha_sum{};
              for(unsigned slot=0;slot<std::min(count,511u);++slot) {
                const Bytes record(memory.view(0x52dfdcu+slot*32,32));
                if(((record.u32(0)>>1)&3u)==i && (record.u32(4)&2u)) {
                  ++admitted;alpha_sum+=record.u16(2)>>8;
                }
              }
              std::cout<<"Source lens queries: frame="<<frames<<" view="<<i<<" issued="<<lens_queries[i].size()
                  <<" flares="<<lens_draws[i].size()
                  <<" admitted="<<admitted<<" alpha_sum="<<alpha_sum
                  <<" stamp="<<Bytes(memory.view(0x4922c0u,4)).u32(0);
              if(i==0)for(const auto &result:source_lens_queries->completed())
                std::cout<<" completed="<<result.id<<':'<<result.generation<<':'<<result.samples;
              std::cout<<'\n';
            }
            world_particles[i].insert(world_particles[i].end(),std::make_move_iterator(draws.begin()),std::make_move_iterator(draws.end()));
          });
        if(replica_lens_frame) {
          for(const auto &request:source_standalone_light_lenses_tu15(effect_light_views[i]))
            native_lens->standalone_light(request);
          for(const auto &request:world_effects.replica_lenses()) {
            if(request.source.attached_owner==null_datum)
              native_lens->unattached_effect(request.source,request.lens,request.ordinal,
                  request.position,request.forward,request.opacity);
            else native_lens->nonbiped_attached_effect(request.source,request.lens,request.ordinal,
                  request.position,request.forward,request.opacity);
          }
          for(const auto &request:weapon_projectile_scene.free_lens_view()) {
            const auto &node=request.first_world_node;
            native_lens->free_projectile(request.owner,request.definition,request.ordinal,
                node.position,node.basis[0],request.use_marker_scale?node.scale:1.f,1.f,{1,1,1});
          }
          lens_queries[i]=native_lens->queries();lens_draws[i]=native_lens->draws();
          if(options.frames||options.capture_tick) {
            std::cout<<"Native replica lens queries: frame="<<frames<<" view="<<i
                <<" issued="<<lens_queries[i].size()<<" flares="<<lens_draws[i].size();
            if(i==0)for(const auto &result:native_lens->completed_queries())
              std::cout<<" completed="<<result.id<<':'<<result.generation<<':'<<result.samples;
            std::cout<<'\n';
          }
        }
        if(options.effect_observer)options.effect_observer(i,current.tick,world_particles[i]);
      }
      auto &bindings=light_bindings[i];
      auto &camouflage=camouflage_bindings[i];
      const auto camouflage_range=[&](unsigned first,unsigned count,const auto &owner) {
        if(count)camouflage.push_back({first,count,std::clamp(owner.source_camouflage.primary,0.f,1.f),owner.source_unit});
      };
      for(unsigned j=0;j<multiplayer_capacity;++j)if(shown.players[j].connected) {
        const auto &owner=current.players[j];const auto &body=species_for(shown.players[j]);
        camouflage_range(body.player_materials[j],unsigned(body.player.model.mesh.materials.size()),owner);
        camouflage_range(body.first_person_body_materials[j],unsigned(body.first_person_body.model.mesh.materials.size()),owner);
        if(owner.weapon!=null_datum) {
          const auto &held=body_guns.at(owner.weapon);
          camouflage_range(held.materials[j*2],unsigned(held.gun.model.mesh.materials.size()),owner);
        }
        if(owner.secondary_weapon.definition!=null_datum) {
          const auto &held=body_guns.at(owner.secondary_weapon.definition);
          camouflage_range(held.materials[j*2+1],unsigned(held.gun.model.mesh.materials.size()),owner);
        }
      }
      const auto &local_camouflage=current.players[id];
      camouflage_range(hands.material,unsigned(hands.model.mesh.materials.size()),local_camouflage);
      for(const auto &[key,body]:species)for(unsigned hand=0;hand<2;++hand)if(!body.guns.empty()) {
        const auto &held=body.guns.at(i*2+hand);
        camouflage_range(held.material,unsigned(held.model.mesh.materials.size()),local_camouflage);
      }
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
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
        if(const auto* reference=held_pose_shading_reference(v.player.definition))
          object_light_cache[j].lighting=reference->lighting;
#endif
        bindings.push_back({v.player_materials[j],unsigned(v.player.model.mesh.materials.size()),object_light_cache[j].lighting});
        // Original3DB00/BAF80 gives attached weapons their root's84-byte bank.
        // An unarmed source unit has no attached weapon or lighting recipient.
        if(shown.players[j].weapon!=null_datum) {
          const auto &held=body_guns.at(shown.players[j].weapon);
          bindings.push_back({held.materials[j*2],unsigned(held.gun.model.mesh.materials.size()),object_light_cache[j].lighting});
        }
      }
      const auto first_person_lighting=reference_player?reference_player->first_person_lighting:object_light_cache[id].lighting;
      bindings.push_back({hands.material,unsigned(hands.model.mesh.materials.size()),first_person_lighting});
      if(gun)bindings.push_back({gun->material,unsigned(gun->model.mesh.materials.size()),first_person_lighting});
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
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
      const auto detail_policy=held_pose_shading_reference_policy();
#else
      const auto detail_policy=native_detail_policy(platform.desktop->display_settings.detail,options.local_players);
#endif
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
      object_view.in_frustum=[&](Vec3 center,float radius) {
        return source_sphere_frustum_visibility(
            {source_camera.full_frustum.bounds,source_camera.full_frustum.planes},center,radius)!=SourceSphereVisibility::outside;
      };
      if(current.source_body_poses_authoritative)object_view.source_bound=[&](std::uint32_t owner) {
        const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
            [&](const auto& p){return p.object==owner;});
        if(pose==current.source_body_poses.end())
          throw FormatError("Source machine lost its authoritative retained render bound");
        return pose->sphere;
      };
      const auto machine_frustums=visibility.sphere_bank();
      if(current.source_scene_roots_authoritative)object_view.source_visible=[&](std::uint32_t owner,ShadowCasterSphere sphere) {
        const auto root=std::find_if(current.source_scene_roots.begin(),current.source_scene_roots.end(),
            [&](const auto& r){return r.object==owner;});
        if(root==current.source_scene_roots.end())
          throw FormatError("Source machine lost its authoritative cluster membership owner");
        for(std::size_t ordinal=0;ordinal<machine_frustums.clusters.size();++ordinal)
          if(std::binary_search(root->render_clusters.begin(),root->render_clusters.end(),
                machine_frustums.clusters[ordinal].source_cluster) &&
             source_cluster_frustum_visibility(machine_frustums,ordinal,sphere.center,sphere.radius).visible)return true;
        return false;
      };
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
      const auto vehicle_lighting=[&](const auto& v,const auto& o,ShadowCasterSphere sphere)->const ObjectLightingCache& {
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        auto reference_row=o.netgame_row;
        for(auto parent=o.model_attachment;reference_row<0 && parent;parent=content.objects.at(parent->parent).model_attachment)
          reference_row=content.objects.at(parent->parent).netgame_row;
        const auto* reference=vehicle_shading_reference(o.definition,reference_row);
#endif
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
        return light;
      };
      for(const auto& v:current.world_vehicles) {
        if(v.placement>=content.objects.size() || content.objects[v.placement].type!=1)throw FormatError("Vehicle snapshot lost its authored definition");
        const auto& o=content.objects[v.placement];auto& vehicle=vehicle_visuals.at({o.definition,o.variant});
        if(vehicle.requires_source_regions && !v.regions)throw FormatError("Vehicle rendering requires its original live model region bank");
        if(v.regions && vehicle.prt)vehicle.prt->regions(v.regions->selection);
        if(const auto source=live_vehicle_base[i].find(v.handle);source!=live_vehicle_base[i].end()) {
          const auto& nodes=live_vehicle_nodes[i].at(v.handle);
          const auto& light=vehicle_lighting(v,o,source->second.front().sphere);
          if(vehicle.prt && !reuse_vehicle_skins)
            vehicle.prt->pose_world([&](const auto& model){return skin_model_world_nodes(model,nodes);});
          for(const auto& draw:source->second) {
            if(draw.definition!=o.definition)throw FormatError("live vehicle base changed its full definition owner");
            const auto section=draw.unit.words[3]>>9&511u;
            const SourceModelShaderCollector collector{0,draw.coverage};
            auto& vertices=vehicle_vertices[i].emplace_back();
            auto& prt_banks=vehicle_prt_banks[i].emplace_back();
            if(vehicle.prt) {
              if(reuse_vehicle_skins) {
                const auto& model=vehicle.prt->at(draw.lod).model;
                const auto posed=vehicle_frame_poses.update(v.handle,draw.lod,model,nodes);
                vertices=vehicle.prt->selected_section_pose(draw.lod,section,collector,*object_view.shader,posed).vertices;
              } else vertices=vehicle.prt->selected_section(draw.lod,section,collector,*object_view.shader).vertices;
              prt_banks=vehicle.prt->selected_prt_banks(draw.lod,section,live_vehicle_prt[i].at({v.handle,section}));
            } else {
              const auto& model=vehicle.lods.at(draw.lod);
              std::vector<Vertex> reference_skin;
              std::span<const Vertex> posed;
              if(reuse_vehicle_skins)posed=vehicle_frame_poses.update(v.handle,draw.lod,model,nodes);
              else {reference_skin=skin_model_world_nodes(model,nodes);posed=reference_skin;}
              for(const auto& vertex:posed)if(vertex.source_section==section)vertices.push_back(vertex);
              if(vertices.empty())
                throw FormatError("live vehicle selected an unprepared original section/LOD");
              vehicle.shaders->select(vertices,collector,*object_view.shader);
              for(auto& vertex:vertices)vertex.material+=vehicle.material;
            }
            const auto colors=source_live_model_base_colors_tu15(draw,
                source_object_initial_colors(vehicle.colors,o.position,vehicle.variant_name));
            model_object_draws[i].push_back({vertices,light.lighting,colors,{},prt_banks});
            collect_pstencil_lightmap(i,vertices,light);
            if(options.vehicle_draw_observer)options.vehicle_draw_observer(i,v.handle,o.definition,
                light.lighting,light.lighting_valid,draw.lod,vertices);
            if(options.vehicle_projection_observer)options.vehicle_projection_observer(i,v.handle,
                vertices,source_view_constants.world,Vec2{float(physical(rect).width),float(physical(rect).height)});
          }
          continue;
        }
        const auto& skeleton=vehicle.prt?vehicle.prt->model():vehicle.lods[5];
        auto nodes=current.source_body_poses_authoritative?replica_world_nodes(v.handle):
            pose_model_nodes_world(skeleton,v.transform);
#ifdef HALO2_VEHICLE_SHADING_REFERENCE
        auto reference_row=o.netgame_row;
        for(auto parent=o.model_attachment;reference_row<0 && parent;parent=content.objects.at(parent->parent).model_attachment)
          reference_row=content.objects.at(parent->parent).netgame_row;
        const auto* reference=vehicle_shading_reference(o.definition,reference_row);
        if(reference)nodes=reference->nodes;
#endif
        if(nodes.empty())throw FormatError("Vehicle lost its current WORLD node0");
        auto sphere=vehicle.owner().sphere(nodes.front(),v.transform.scale);
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
        auto& vertices=vehicle_vertices[i].emplace_back();
        auto& prt_banks=vehicle_prt_banks[i].emplace_back();
        unsigned lod{};bool ordinary_lighting=true;
        if(vehicle.prt) {
          auto selector=object_view;
          selector.input=[&](std::size_t,std::uint32_t,Vec3 center,float) {
            return SourceObjectLodInput{source_geometry_lod_distance(center,camera.position,
                source_angles.lighting_distance_scale),geometry_flags,geometry_control};
          };
          const auto pose_lod=[&](unsigned current_lod,const RenderModel &model) {
            if(!reuse_vehicle_skins)return skin_model_world_nodes(model,nodes);
            const auto posed=vehicle_frame_poses.update(v.handle,current_lod,model,nodes);
            return std::vector<Vertex>(posed.begin(),posed.end());
          };
          if(reference_object_lod_skins) {
            unsigned resident_lod{};
            // pose_world visits the six resident banks in ascending LOD order.
            vehicle.prt->pose_world([&](const RenderModel &model){return pose_lod(resident_lod++,model);});
          } else {
            const auto chosen=vehicle.prt->choose_lod(v.placement,v.handle,sphere.center,sphere.radius,selector,history->second);
            vehicle.prt->pose_world_lod(chosen,[&](const RenderModel &model){ModelSkinWorldPose pose;pose.vertices=pose_lod(chosen,model);return pose;});
          }
          lod=vehicle.prt->select(v.placement,v.handle,sphere.center,sphere.radius,selector,history->second).lod;
          const auto &draw=vehicle.prt->view(i);vertices=draw.vertices;ordinary_lighting=draw.ordinary_lighting;
          prt_banks=vehicle.prt->owner_banks(cache,nodes,*vehicle_prt_lighting,lod);
        } else {
          auto metric=source_geometry_lod_distance(sphere.center,camera.position,source_angles.lighting_distance_scale);
          if(object_view.source_4ba019)metric*=.3f;
          if(geometry_control&4u)metric*=1.7f;
          const auto choice=source_geometry_lod_choice(vehicle.geometry,metric,history->second[i]);
          lod=unsigned(source_geometry_lod_cap(choice,detail_policy,geometry_flags,false));
          std::vector<Vertex> reference_skin;
          std::span<const Vertex> posed;
          if(reuse_vehicle_skins)posed=vehicle_frame_poses.update(v.handle,lod,vehicle.lods[lod],nodes);
          else {reference_skin=skin_model_world_nodes(vehicle.lods[lod],nodes);posed=reference_skin;}
          // Actual source choices retain region order and duplicate collector
          // entries. The resident resource union never selects active geometry.
          const auto sections=v.regions?source_initial_model_sections(vehicle.initial,v.regions->selection,lod):vehicle.initial_sections[lod];
          for(auto section:sections)for(const auto& vertex:posed)
            if(vertex.source_section==section)vertices.push_back(vertex);
          if(!vertices.empty())history->second[i]=std::int8_t(lod);
          vehicle.shaders->select(vertices,sphere,*object_view.shader);
          for(auto& vertex:vertices)vertex.material+=vehicle.material;
        }
        const auto& light=vehicle_lighting(v,o,sphere);
        // Each owner supplies independent constants even when vehicle models,
        // resident images and material slots are shared with another vehicle.
        const auto colors=source_object_initial_colors(vehicle.colors,o.position,vehicle.variant_name);
        model_object_draws[i].push_back({vertices,ordinary_lighting?std::optional{light.lighting}:std::nullopt,colors,{},prt_banks});
        if(ordinary_lighting)collect_pstencil_lightmap(i,vertices,light);
        if(options.vehicle_draw_observer)options.vehicle_draw_observer(i,v.handle,o.definition,light.lighting,light.lighting_valid,lod,vertices);
        if(options.vehicle_projection_observer)options.vehicle_projection_observer(i,v.handle,
            vertices,source_view_constants.world,Vec2{float(physical(rect).width),float(physical(rect).height)});
      }
      object_view.input=[&](std::size_t source_object,std::uint32_t handle,Vec3 center,float) {
        const auto&live=live_match().simulation().scene().get(handle);
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
      props.append_source_models(model_object_draws[i],object_view);
      props.append_view(world,bindings,object_view);
      source_scenery.append_view(world,bindings,object_view);
      auto dynamic_view=object_view;
      dynamic_view.input=[&](std::size_t,std::uint32_t handle,Vec3 center,float) {
        const auto &o=dynamic_objects.get(handle);
        return SourceObjectLodInput{source_geometry_lod_distance(center,camera.position,
            source_angles.lighting_distance_scale),o.runtime_flags,o.control_flags};
      };
      for(const auto &draw:dynamic_objects.draw_view(std::int32_t(current.tick),dynamic_view,
          [&](std::uint32_t owner){return source_body_interpolation.sample(owner,blend);})) {
        model_object_draws[i].push_back({draw.vertices,draw.ordinary_lighting?
            std::optional{draw.lighting}:std::nullopt,draw.colors,{},draw.prt,draw.camouflage,draw.skins});
        if(options.dynamic_draw_observer && !draw.vertices.empty())
          options.dynamic_draw_observer(i,draw.object,draw.definition,draw.vertices);
      }
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
        std::vector<Transform> source_nodes;
        const std::vector<SourceLiveModelBaseDrawTu15>* source_draws=nullptr;
        std::optional<ObjectLightingState> source_lighting;
        if(current.source_body_poses_authoritative) {
          if(source_model_world) {
            const auto admitted=live_unit_base[i].find(item.handle);
            if(admitted==live_unit_base[i].end())continue;
            source_draws=&admitted->second;
            const auto pose=completed_render_nodes.find(item.handle);
            if(pose==completed_render_nodes.end())throw FormatError("Loose weapon lost its completed original render nodes");
            source_nodes=pose->second;
            const auto vertices=live_unit_vertices[i].find(item.handle);
            if(vertices==live_unit_vertices[i].end())throw FormatError("Loose weapon admission has no matching source geometry");
            append_world(owner_vertices,vertices->second,gun.material);
            if(!submitted_unit_base[i].insert(item.handle).second)
              throw FormatError("Loose weapon source base was submitted twice");
            source_lighting=source_draws->front().lighting;
          }else {
            source_nodes=source_body_interpolation.sample(item.handle,blend);
            if(source_nodes.empty())throw FormatError("Loose weapon snapshot omitted its original WORLD nodes");
            auto vertices=skin_model_world_nodes(gun.model,source_nodes);
            gun.shaders->select(vertices,collector,*object_view.shader);
            append_world(owner_vertices,std::move(vertices),gun.material);
          }
        }else if(share_world_weapon_poses) {
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
        std::optional<std::uint16_t> damage_flags;
        std::optional<SourceWeaponFiringMaterialState> firing_material;
        if(!live_client() && live_world()->source_objects && live_world()->source_objects->owns_physics() && item.handle!=null_datum) {
          auto &source=*live_world()->source_objects;
          if(source.object_definition(item.handle)!=item.definition)
            throw FormatError("World weapon material flags lost their actual source definition");
          damage_flags=Bytes(source.memory().view(source.object_body(item.handle)+0x10au,2)).u16(0);
          firing_material=source_weapon_firing_material_owner(source,item.handle,*definition);
        }
        const ShaderInput owner_input=[&,item,definition,tables,damage_flags,firing_material](std::uint32_t name) {
          if(!name)return 0.f;
          const std::array<SourceWeaponMagazineState,1> storage{{{std::int16_t(item.loaded),std::int16_t(item.reserve)}}};
          const std::span<const SourceWeaponMagazineState> magazines=definition->magazines.size()==1?
              std::span<const SourceWeaponMagazineState>(storage):std::span<const SourceWeaponMagazineState>{};
          const auto source=source_weapon_material_input(name,item.shader,definition->magazines,
              magazines,definition->functions,*tables,float(shader_time),{},weapon_material_names,item.handle,damage_flags,firing_material);
          const float value=source?source->value:weapon_ammunition_input(name,item.loaded).value_or(0.f);
          if(options.world_weapon_shader_input_observer)
            options.world_weapon_shader_input_observer(item.handle,item.definition,name,value);
          return value;
        };
        if(options.world_weapon_source_draw_observer)
          options.world_weapon_source_draw_observer(i,current.tick,item.handle,item.definition,
              source_nodes,owner_vertices,source_draws?std::span<const SourceLiveModelBaseDrawTu15>(*source_draws):
                  std::span<const SourceLiveModelBaseDrawTu15>{});
        model_object_draws[i].push_back({owner_vertices,source_lighting.value_or(object_light_cache[id].lighting),{},owner_input});
      }
      measure(timing.view[i].objects_ms);
      const auto retain_unit_submission=[&](std::uint32_t owner)->const std::vector<Vertex>* {
        const auto at=live_unit_vertices[i].find(owner);if(at==live_unit_vertices[i].end())return nullptr;
        if(!submitted_unit_base[i].insert(owner).second)
          throw FormatError("Source unit base was submitted twice in one view");
        return &at->second;
      };
      const auto observe_unit_submission=[&](std::uint32_t owner,const ObjectLightingCache& light,
                                              std::span<const Vertex> vertices) {
        if(!options.unit_source_draw_observer)return;
        const auto& draw=live_unit_base[i].at(owner).front();
        options.unit_source_draw_observer(i,owner,draw.definition,light.lighting,light.lighting_valid,draw.lod,vertices);
      };
      const auto append_unit_base=[&](std::uint32_t owner,unsigned material,const ObjectLightingCache& light) {
        const auto* vertices=retain_unit_submission(owner);if(!vertices)return false;
        const auto first=world.size();append_world(world,*vertices,material);
        if(const auto gpu=live_unit_gpu[i].find(owner);gpu!=live_unit_gpu[i].end())
          unit_skin_submissions[i].push_back({first,gpu->second});
        observe_unit_submission(owner,light,std::span<const Vertex>(world).subspan(first));
        return true;
      };
      if(current.source_body_poses_authoritative)for(const auto &corpse:current.corpses) {
        const auto pose=std::find_if(current.source_body_poses.begin(),current.source_body_poses.end(),
            [&](const auto &p){return p.object==corpse.object;});
        if(pose==current.source_body_poses.end())
          throw FormatError("Retained corpse lost its completed source WORLD hierarchy");
        const auto representation=source_session_player_representation(corpse.configuration,source_game_type);
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
        const auto nodes=replica_world_nodes(corpse.object);
        const auto skinned=skin_model_world_nodes(body.player.model,nodes);
        auto &vertices=corpse_vertices[i].emplace_back();
        const auto collector=source_model_shader_collector(origin,pose->sphere.radius,*object_view.shader);
        if(const auto* retained=retain_unit_submission(corpse.object)) {
          vertices=*retained;
          for(auto& vertex:vertices)vertex.material+=body.player.material;
          observe_unit_submission(corpse.object,light,vertices);
        } else body.player.shaders->append_selected(vertices,skinned,collector,*object_view.shader,body.player.material);
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
                  player_damage_functions.at(corpse.definition),*player_damage_function_tables,name,float(shader_time),player_damage_names);
            }});
        ++timing.bodies_drawn;
        if(options.animation_observer)options.animation_observer(i,corpse.player&65535u,
            {current.tick,corpse.object,null_datum,body.player.model.animation,null_datum,
                blend,float(current.tick),false,false,vertices,null_datum,0,{},nodes});
      }
      for (unsigned j = 0; j < multiplayer_capacity; ++j)
        if (body_present(shown.players[j]) && (options.capture_third_person || j != id || shown.players[j].dead || third_person_vehicle_view)) {
          ++timing.bodies_drawn;
          const auto &a = shown.players[j];
          const auto &body=species_for(a);
          const auto &player=body.player;
          const auto &body_attachment=body.body_attachment;
          const auto *held=a.weapon==null_datum?nullptr:&body_guns.at(a.weapon);
          // The original seat flags already decide handheld visibility for
          // local and replicated players. Hidden carried weapons retain their
          // last WORLD bank; the fallback below must not draw that stale pose.
          const bool handheld_visible=a.source_vehicle_hud.ownership.handheld_visible;
          const auto *body_gun=held && handheld_visible?&held->gun:nullptr;
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
            const auto *grenade=live_match().grenade_clip(a.grenade_action.graph,a.grenade_action.clip);
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
          bool body_deferred{},weapon_deferred{};
          std::vector<Transform> body_nodes,held_nodes;
          const auto *copy_pose=std::getenv("HALO2_REFERENCE_COPY_BODY_VERTICES");
          const bool direct_pose=!(copy_pose && *copy_pose);
          const bool gpu_body=live_unit_gpu[i].contains(body_handle(a));
          const bool gpu_weapon=!a.dead && body_gun && live_unit_gpu[i].contains(a.source_weapon);
          if(options.local_players>1 && cached) {
            // A prior view may have consumed only local GPU records. A later
            // view with a CPU observer/override needs its own real posed bank.
            if(!gpu_body && cached->body_deferred) {
              cached->body=skin_model_world_nodes(player.model,cached->nodes);cached->body_deferred=false;
            }
            if(!gpu_weapon && !a.dead && body_gun && cached->weapon_deferred) {
              cached->weapon=skin_model_world_nodes(body_gun->model,cached->weapon_nodes);cached->weapon_deferred=false;
            }
            if(!direct_pose) {body_vertices=cached->body;gun_vertices=cached->weapon;}
            body_nodes=cached->nodes;
            held_nodes=cached->weapon_nodes;
            body_clip=cached->primary;body_source_frame=cached->frame;
          } else {
            ++timing.body_pose_builds;
            if(const auto rendered=completed_render_nodes.find(body_handle(a));rendered!=completed_render_nodes.end())
              body_nodes=rendered->second;
            else if(current.source_body_poses_authoritative)
              body_nodes=replica_world_nodes(body_handle(a));
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
            if(!a.dead && body_gun) {
            const bool source_weapon_pose=current.source_body_poses_authoritative &&
                std::any_of(current.source_body_poses.begin(),current.source_body_poses.end(),
                    [&](const auto &pose){return pose.object==a.source_weapon;});
            if(const auto rendered=completed_render_nodes.find(a.source_weapon);rendered!=completed_render_nodes.end())
              held_nodes=rendered->second;
            else if(source_weapon_pose)held_nodes=replica_world_nodes(a.source_weapon);
            else {
            const auto parent_node=body_nodes.at(body_attachment.parent.node);
            const auto marker=compose_transform(parent_node,body_attachment.parent.local);
            auto child_marker=body_attachment.child.local;
            if(body_attachment.named_child)child_marker=compose_transform(
                pose_model_node(body_gun->model,body_attachment.child.node,
                    body_gun->idle?&*body_gun->idle:nullptr,body_time),child_marker);
            const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},
                marker,child_marker,body_attachment.named_child);
            const auto local=source_object_attachment_local_frame(aligned,parent_node);
            const auto gun_frame=source_object_transform(local.position,local.forward,local.up,1,false,&parent_node);
            held_nodes=pose_model_nodes_world(body_gun->model,gun_frame,
                body_gun->idle?&*body_gun->idle:nullptr,body_time);
            }
            //113BA0/113980's world callback aligns the support hand to the
            // actual carried weapon markers. FP arms use a separate model.
            if(!current.source_body_poses_authoritative && a.secondary_weapon.datum==null_datum)
              apply_frontend_model_weapon_ik(body.body_ik[a.native_crouching],player.model,
                  body_nodes,held_nodes,source_aim_blend_weight(aim_blends[j]));
            if(gpu_weapon) {
              auto prepared=model_skin_world_pose(body_gun->model,held_nodes,true);
              gun_vertices=std::move(prepared.vertices);weapon_deferred=prepared.gpu;
            } else gun_vertices=skin_model_world_nodes(body_gun->model,held_nodes);
            }
            if(gpu_body) {
              auto prepared=model_skin_world_pose(player.model,body_nodes,true);
              body_vertices=std::move(prepared.vertices);body_deferred=prepared.gpu;
            } else body_vertices=skin_model_world_nodes(player.model,body_nodes);
            if(options.local_players>1) {
              if(direct_pose)cached=BodyFramePose{std::move(body_vertices),std::move(gun_vertices),body_nodes,body_clip,body_source_frame,held_nodes,body_deferred,weapon_deferred};
              else cached=BodyFramePose{body_vertices,gun_vertices,body_nodes,body_clip,body_source_frame,held_nodes,body_deferred,weapon_deferred};
            }
          }
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
          if(const auto* reference=held_pose_shading_reference(player.definition))body_nodes=reference->nodes;
          if(body_gun)if(const auto* reference=held_pose_shading_reference(body_gun->definition))held_nodes=reference->nodes;
#endif
          auto selected_body=selected_world_vertices(player,body_handle(a),i,body_nodes,"body");
          auto selected_weapon=a.dead || !body_gun?std::optional<std::vector<Vertex>>{}:
              selected_world_vertices(*body_gun,a.source_weapon,i,held_nodes,"primary");
          const auto &posed_body=selected_body?*selected_body:direct_pose && cached?cached->body:body_vertices;
          const auto &posed_gun=selected_weapon?*selected_weapon:direct_pose && cached?cached->weapon:gun_vertices;
          if(options.node_observer && options.observe_submitted_nodes) {
            options.node_observer(i,current.tick,{MultiplayerNodeStage::world_submitted,
                body_handle(a),player.render_model,0,blend,body_nodes,null_datum,0,false,frames,~0u});
            if(!a.dead && body_gun)
              options.node_observer(i,current.tick,{MultiplayerNodeStage::world_submitted,
                  a.source_weapon,body_gun->render_model,0,blend,held_nodes,null_datum,0,false,frames,0});
          }
          append_shields(world,posed_body,body.body_shields,i,j,false,collector,*object_view.shader);
          if(options.animation_observer)options.animation_observer(i,j,
              {current.tick,a.source_unit,a.source_weapon,body_clip?body_clip->source_graph:player.model.animation,
                body_clip?body_clip->source_index:null_datum,animation_blend,body_source_frame.value_or(float(body_tick)),false,
                same_body && pose_tick!=double(current.tick),posed_body,null_datum,0,held_nodes,body_nodes,
                gait[j]?gait[j]->facing(animation_blend):a.yaw,gait[j]?gait[j]->frame().activity:null_datum,{},
                gait[j]?std::optional{gait[j]->frame().frame}:std::nullopt,posed_gun});
          const auto shadow_body_first=world.size();
          if(direct_pose) {
            if(!append_unit_base(body_handle(a),body.player_materials[j],object_light_cache[j]))
              player.shaders->append_selected(world,posed_body,collector,*object_view.shader,body.player_materials[j]);
            collect_pstencil_lightmap(i,std::span<const Vertex>(world).subspan(shadow_body_first),object_light_cache[j]);
            if(!a.dead && body_gun && !append_unit_base(a.source_weapon,held->materials[j*2],object_light_cache[j]))
              body_gun->shaders->append_selected(world,posed_gun,collector,*object_view.shader,held->materials[j*2]);
          } else {
            if(selected_body)body_vertices=std::move(*selected_body);
            if(selected_weapon)gun_vertices=std::move(*selected_weapon);
            if(!append_unit_base(body_handle(a),body.player_materials[j],object_light_cache[j])) {
              player.shaders->select(body_vertices,collector,*object_view.shader);
              append_world(world,std::move(body_vertices),body.player_materials[j]);
            }
            collect_pstencil_lightmap(i,std::span<const Vertex>(world).subspan(shadow_body_first),object_light_cache[j]);
            if(!a.dead && body_gun) {
              if(!append_unit_base(a.source_weapon,held->materials[j*2],object_light_cache[j])) {
                body_gun->shaders->select(gun_vertices,collector,*object_view.shader);
                append_world(world,std::move(gun_vertices),held->materials[j*2]);
              }
            }
          }
          if(!a.dead && handheld_visible && a.secondary_weapon.datum!=null_datum) {
            const auto &left_bank=body_guns.at(a.secondary_weapon.definition);
            const auto &left=left_bank.gun;
            std::vector<Transform> nodes;
            if(const auto rendered=completed_render_nodes.find(a.secondary_weapon.datum);rendered!=completed_render_nodes.end())
              nodes=rendered->second;
            else if(current.source_body_poses_authoritative) {
              if(std::none_of(current.source_body_poses.begin(),current.source_body_poses.end(),
                  [&](const auto& pose){return pose.object==a.secondary_weapon.datum;}))
                throw FormatError("Source left-hand weapon lost its completed WORLD pose");
              nodes=replica_world_nodes(a.secondary_weapon.datum);
            } else {
              auto secondary_player=a;secondary_player.weapon=a.secondary_weapon.definition;
              const auto &secondary=species_for(secondary_player);const auto &attachment=*secondary.secondary_attachment;
              const auto marker=compose_transform(body_nodes.at(attachment.parent.node),attachment.parent.local);
              const auto aligned=source_object_attachment_frame({{}, {1,0,0}, {0,0,1}},marker,attachment.child.local,false);
              nodes=pose_model_nodes_world(left.model,source_object_transform(aligned.position,aligned.forward,aligned.up,1,false),
                  left.idle?&*left.idle:nullptr,body_time);
            }
            auto selected=selected_world_vertices(left,a.secondary_weapon.datum,i,nodes,"secondary");
            if(options.node_observer && options.observe_submitted_nodes)options.node_observer(i,current.tick,
                {MultiplayerNodeStage::world_submitted,a.secondary_weapon.datum,left.render_model,
                  0,blend,nodes,null_datum,0,false,frames,1});
            auto vertices=selected?std::move(*selected):live_unit_gpu[i].contains(a.secondary_weapon.datum)?
                model_skin_world_pose(left.model,nodes,true).vertices:skin_model_world_nodes(left.model,nodes);
            if(!append_unit_base(a.secondary_weapon.datum,left_bank.materials[j*2+1],object_light_cache[j]))
              left.shaders->append_selected(world,vertices,collector,*object_view.shader,left_bank.materials[j*2+1]);
            bindings.push_back({left_bank.materials[j*2+1],unsigned(left.model.mesh.materials.size()),object_light_cache[j].lighting});
          }
        }
      measure(timing.view[i].body_ms);
      const auto &registered_view=live_client()?client_fp_views[i]:live_match().first_person_view(i);
      const auto primary_player=p;
      const bool source_fp= !options.capture_third_person && native_fp_presentation();
      const auto source_up=camera.oriented?camera.up:cross(view_forward,{-std::sin(camera.yaw),std::cos(camera.yaw),0});
      std::vector<SourceFirstPersonModelTu15> source_models;
      if(source_fp && !p.dead && !third_person_vehicle_view) {
        if(source_model_world) {
          if(!cached_native_fp_groups.at(i))throw FormatError("Native FP draw lost its actual pre-collector cache producer");
          source_models=std::move(*cached_native_fp_groups[i]);
        } else source_models=prepare_native_fp_models(i,p,camera.position,view_forward,source_up);
      }
      const auto completed_fp_models=options.animation_observer?source_models:std::vector<SourceFirstPersonModelTu15>{};
      if(source_fp) {
        if(options.node_observer)for(const auto &model:source_models)
          options.node_observer(i,current.tick,{MultiplayerNodeStage::first_person_completed,
              model.owner,model.render_model,model.mode,animation_blend,model.nodes});
        const auto frame=camera_transform(camera.position,view_forward,source_up);
        retain_fp_poses(i,registered_view,p,source_models,frame);
        const auto &body=species_for(p);
        for(auto &model:source_models) {
          const RenderModel *skeleton=nullptr;
          if(model.mode==0 && model.render_model==body.first_person_body_model)skeleton=&body.first_person_body.model;
          for(unsigned hand=0;hand<2 && !skeleton;++hand) {
            if(registered_view.hands[hand].weapon==null_datum)continue;
            auto owner=p;
            if(hand)owner.weapon=p.secondary_weapon.definition;
            const auto &visual=species_for(owner);
            if(!visual.fp_definition)continue;
            if(model.owner==registered_view.unit && model.render_model==visual.fp_definition->arms_model)
              skeleton=&visual.hands.model;
            else if(model.owner==registered_view.hands[hand].weapon && model.render_model==visual.fp_definition->weapon_model)
              skeleton=&visual.guns[i*2+hand].model;
          }
          if(!skeleton)throw FormatError("Original FP interpolation lost its authored model hierarchy");
          const auto &history=source_fp_interpolation[i].at({model.owner,model.render_model,model.mode});
          model.nodes=history.sample(*skeleton,animation_blend,model.mode==1?std::optional{frame}:std::nullopt);
        }
        if(options.node_observer)for(const auto &model:source_models)
          options.node_observer(i,current.tick,{MultiplayerNodeStage::first_person_interpolated,
              model.owner,model.render_model,model.mode,animation_blend,model.nodes});
      }
      if(source_fp && !source_models.empty()) {
        const auto &body=species_for(p);
        for(const auto &model:source_models)if(model.mode!=1 && source_foreground_models_admitted(view_zoom[i])) {
          //168DED can additionally submit the looking-down body in WORLD
          //mode. Its original output supplies every node; never substitute
          //an idle pose or reject it as an unexpected third FP model.
          if(model.mode!=0 || model.owner!=registered_view.unit || model.render_model!=body.first_person_body_model)
            throw FormatError("Original FP body draw lost its authored unit/model binding");
          const auto vertices=skin_model_world_nodes(body.first_person_body.model,model.nodes);
          const auto collector=source_model_shader_collector(object_owner_spheres[id].center,
              object_owner_spheres[id].radius,*object_view.shader);
          body.first_person_body.shaders->append_selected(world,vertices,collector,*object_view.shader,body.first_person_body_materials[id]);
          bindings.push_back({body.first_person_body_materials[id],unsigned(body.first_person_body.model.mesh.materials.size()),object_light_cache[id].lighting});
          if(options.animation_observer)options.animation_observer(i,id,
              {current.tick,p.source_unit,p.source_weapon,body.player.model.animation,null_datum,
                animation_blend,float(current.tick),false,false,vertices,null_datum,0,{},model.nodes});
        }
      }
      const auto native_draws=source_fp?native_fp_draws(registered_view,primary_player,source_models):SourceFirstPersonDrawsTu15{};
      Transform primary_camera_adjustment;
      bool native_arms_consumed{};
      for(unsigned hand_index=0;hand_index<2 && !options.capture_third_person;++hand_index) {
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
        auto &v=species_for(p);const auto &weapon=content.weapons.at(p.weapon);
        const auto &gun=v.guns[i*2+hand_index];const auto &hands=v.hands;
        const auto &fp_idle=v.fp_idle;const auto &fp_reload=v.fp_reload;const auto &fp_reload_empty=v.fp_reload_empty;
        const auto &fp_fire=v.fp_fire;const auto &fp_definition=v.fp_definition;const auto &fp_ik=v.fp_ik;
        auto fp_view=registered_view;fp_view.hands[0]=registered_view.hands[hand_index];
      if (!p.dead && !third_person_vehicle_view && fp_view.hands[0].weapon!=null_datum) {
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
        if(source_fp && source_models.empty())continue;
        if(fp_definition && !reference_player && !source_fp) {
          const auto &binding=fp_view;
          const auto &hand=binding.hands[0];
          auto origin=live_client()?client_fp_origins[i]:live_match().first_person_clock_origin(i);
          origin.hands[0]=origin.hands[hand_index];
          const auto presentation=first_person_render_frame(origin,binding,*fp_definition,animation_blend,
              live_client() ? (hand_index?client_secondary_clock_available[i]:client_fp_clock_available[i])
                     : current.tick>previous.tick && current.tick-previous.tick==1);
          source_frame=presentation.frame;
          frame_interpolated=presentation.interpolated;
          const auto index=hand.animation_identifier;
          clip=nullptr;
          const auto found=v.fp_clips.find(index);
          if(found!=v.fp_clips.end())clip=&found->second;
          if(!clip)throw FormatError("Registered first-person clock has no decoded presentation clip");
        }
        if(source_fp) {
          const auto &hand=fp_view.hands[0];const auto index=hand.animation_identifier;
          // Native banks already contain the complete pose. Only an observer
          // for an actual gun record needs decoded clip metadata; an omitted
          // gun must not introduce a separate clip-admission dependency.
          if(options.animation_observer && native_draws.weapons[hand_index]) {
            auto selected=v.fp_clips.find(index);
            if(selected==v.fp_clips.end())
              selected=v.fp_clips.emplace(index,cache.load_animation_identifier(hand.graph,index)).first;
            clip=&selected->second;
          }
          source_frame=fp_view.hands[0].frame;
          const auto pose=source_fp_interpolation[i].find({p.source_weapon,fp_definition->weapon_model,1});
          frame_interpolated=pose!=source_fp_interpolation[i].end() && pose->second.paired() && animation_blend<1;
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
        if(!source_fp && fp_definition && fp_definition->ammunition[fp_view.hands[0].dual_wield?1:0]) {
          ammunition=&*fp_definition->ammunition[fp_view.hands[0].dual_wield?1:0];
          // Read the authoritative104CF0 inventory sum, including inactive
          // matching owners. Clients do not reconstruct the inventory here.
          const int available=p.ammunition_available;
          ammunition_frame=first_person_ammunition_frame(std::int16_t(p.loaded),
              std::bit_cast<std::int16_t>(std::uint16_t(available)),std::int16_t(weapon.capacity),
              std::bit_cast<std::int16_t>(std::uint16_t(current.tick-p.reload_tick)),weapon.weapon_type,
              fp_view.hands[0].activity,ammunition->frame_count);
        }
        FirstPersonPose pose;
        ModelSkinWorldPose deferred_gun,deferred_arms;
        if(source_fp) {
          const bool gpu_allowed=model_skin_gpu_requested() && renderer.can_skin_models() &&
              !options.animation_observer &&
              !options.shield_layer_observer && !options.effect_observer &&
              !options.unit_source_draw_observer && !options.bsp_submission_observer &&
              !bsp_reference_scene && !reference_shadow_frame &&
              !std::getenv("HALO2_REFERENCE_COPY_FIRST_PERSON") &&
              !std::getenv("HALO2_VERIFY_FIRST_PERSON_APPEND") &&
              !(repeat_unit_skins && *repeat_unit_skins) && !(repeat_selected_skins && *repeat_selected_skins);
          bool arms_gpu_allowed=gpu_allowed;
          if(arms_gpu_allowed && !native_arms_consumed) {
            // Query the exact shield owner without moving its FP effects ahead
            // of the original weapon/arms skin and later overlay selection.
            const auto &actual=current.players.at(id);
            const auto &vitality=biped_for(actual).vitality;
            const ShieldOverlayGeometry geometry{true,true,true,1,4,0};
            std::fenv_t saved,held;const auto error=errno;
            if(std::fegetenv(&saved)!=0)arms_gpu_allowed=false;
            else {
              if(std::feholdexcept(&held)!=0)arms_gpu_allowed=false;
              else arms_gpu_allowed=select_shield_overlays(vitality,
                  actual.damage_feedback.shield.visible,actual.shield,geometry).selected().empty();
              const auto restored=std::fesetenv(&saved);errno=error;
              if(restored!=0)throw FormatError("first-person shield GPU admission could not restore the CPU floating-point owner");
            }
            errno=error;
          }
          if(!fp_definition)throw FormatError("Original FP draw has no native authored model binding");
          if(const auto *draw=native_draws.weapons[hand_index]) {
            pose.weapon_nodes=draw->nodes;
            deferred_gun=model_skin_world_pose(gun.model,pose.weapon_nodes,gpu_allowed);
            pose.weapon=std::move(deferred_gun.vertices);
          }
          if(!native_arms_consumed) {
            if(const auto *arms=native_draws.arms) {
              pose.arms_nodes=arms->nodes;
              deferred_arms=model_skin_world_pose(hands.model,pose.arms_nodes,arms_gpu_allowed);
              pose.arms=std::move(deferred_arms.vertices);native_arms_consumed=true;
            }
          }
        } else pose=source_frame
            ? pose_first_person_models_world_frame(gun.model,hands.model,*clip,*source_frame,
                weapon.first_person_offset,transform,fp_ik,aiming,hand_index!=0,
                hand_index?&primary_camera_adjustment:nullptr,ammunition,ammunition_frame)
            : pose_first_person_models_world(gun.model,hands.model,*clip,time,
                weapon.first_person_offset,transform,fp_ik,aiming,hand_index!=0,
                hand_index?&primary_camera_adjustment:nullptr,ammunition,ammunition_frame);
        if(!hand_index)primary_camera_adjustment=pose.camera_adjustment;
        if(!source_fp && primary_player.secondary_weapon.datum!=null_datum) {
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
        if(options.node_observer && options.observe_submitted_nodes) {
          if(!pose.weapon_nodes.empty())options.node_observer(i,current.tick,{MultiplayerNodeStage::first_person_submitted,
              p.source_weapon,fp_definition?fp_definition->weapon_model:gun.render_model,
              1,animation_blend,pose.weapon_nodes,null_datum,0,false,frames,hand_index});
          if(!pose.arms_nodes.empty() && !pose.arms.empty())
            options.node_observer(i,current.tick,{MultiplayerNodeStage::first_person_submitted,
                registered_view.unit,fp_definition?fp_definition->arms_model:hands.render_model,
                1,animation_blend,pose.arms_nodes,null_datum,0,false,frames,source_fp?~0u:hand_index});
        }
        if(options.animation_observer && (!source_fp || !pose.weapon_nodes.empty())) {
          AnimationRenderObservation observation{current.tick,p.source_unit,p.source_weapon,clip->source_graph,clip->source_index,
              animation_blend,source_frame?*source_frame:time*30.f,true,frame_interpolated,pose.weapon,
              ammunition?ammunition->source_index:null_datum,ammunition_frame,pose.weapon_nodes};
          if(source_fp) {
            const auto raw=std::find_if(completed_fp_models.begin(),completed_fp_models.end(),
                [&](const auto &model){return model.mode==1 && model.owner==p.source_weapon;});
            if(raw!=completed_fp_models.end())observation.completed_weapon_nodes=raw->nodes;
          }
          options.animation_observer(i,id,observation);
        }
        if(!v.effect_markers.empty()) {
          const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,true});
          MuzzleEffectView effects_view;
          effects_view.camera=source_view_shader_bank(foreground_render_camera,1024.f,camera.position,
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
        if(source_foreground_models_admitted(view_zoom[i])) {
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
          } else if(deferred_gun.gpu || deferred_arms.gpu) {
            const auto deferred_first=foregrounds[i].size();
            const auto append_model=[&](const Visual &visual,std::vector<Vertex> &vertices,
                const ModelSkinWorldPose &deferred,std::uint32_t model_owner) {
              const auto first=foregrounds[i].size();
              if(!deferred.gpu) {
                visual.shaders->append_selected(foregrounds[i],vertices,owner,*object_view.shader,visual.material);
              } else {
                auto geometry=std::make_shared<ModelSkinGeometry>();
                geometry->owner=model_owner;geometry->definition=visual.definition;
                geometry->render_model=visual.render_model;geometry->lod=4;
                geometry->vertices=vertices;geometry->source_vertices.resize(vertices.size());
                for(std::size_t v=0;v<vertices.size();++v)geometry->source_vertices[v]=std::uint32_t(v);
                visual.shaders->select_indexed(geometry->vertices,geometry->source_vertices,owner,*object_view.shader);
                geometry->bindings.reserve(geometry->source_vertices.size());
                for(const auto ordinal:geometry->source_vertices)geometry->bindings.push_back(visual.model.skin.at(ordinal));
                geometry->palette=deferred.palette;geometry->post_identity_placement=true;
                // model_skin_world_pose admitted this whole local bank with
                // this palette; select_indexed only filtered it and the
                // material add touches no admitted field, so every record
                // subset is admitted and the renderer need not re-evaluate.
                geometry->arithmetic_admitted=true;
                for(auto &vertex:geometry->vertices)vertex.material+=visual.material;
                foregrounds[i].insert(foregrounds[i].end(),geometry->vertices.begin(),geometry->vertices.end());
                if(!geometry->vertices.empty())foreground_skin_submissions[i].push_back({first,std::move(geometry)});
              }
              return std::pair{first,foregrounds[i].size()-first};
            };
            const auto arms=append_model(hands,pose.arms,deferred_arms,registered_view.unit);
            const auto weapon=append_model(gun,pose.weapon,deferred_gun,p.source_weapon);
            // Preserve selection before placement, and the original arms→gun
            // order for CPU consumers. GPU records run this graph after skin.
            if(!deferred_arms.gpu)place_vertices(std::span{foregrounds[i]}.subspan(arms.first,arms.second),0,{});
            if(!deferred_gun.gpu)place_vertices(std::span{foregrounds[i]}.subspan(weapon.first,weapon.second),0,{});
            if(std::getenv("HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS")) {
              struct SavedEnvironment {
                std::fenv_t fp;int error=errno;
                SavedEnvironment(){std::feholdexcept(&fp);std::fesetround(FE_TONEAREST);}
                ~SavedEnvironment(){std::fesetenv(&fp);errno=error;}
              } saved;
              auto expected_arms=pose.arms_nodes.empty()?std::vector<Vertex>{}:skin_model_world_nodes(hands.model,pose.arms_nodes);
              auto expected_gun=pose.weapon_nodes.empty()?std::vector<Vertex>{}:skin_model_world_nodes(gun.model,pose.weapon_nodes);
              hands.shaders->select(expected_arms,owner,*object_view.shader);
              gun.shaders->select(expected_gun,owner,*object_view.shader);
              std::vector<Vertex> expected;
              append(expected,std::move(expected_arms),hands.material,{});
              append(expected,std::move(expected_gun),gun.material,{});
              auto actual=std::vector<Vertex>(foregrounds[i].begin()+deferred_first,foregrounds[i].end());
              for(const auto &submission:foreground_skin_submissions[i])if(submission.first>=deferred_first) {
                const auto &geometry=*submission.geometry;
                auto vertices=std::span{actual}.subspan(submission.first-deferred_first,geometry.vertices.size());
                skin_model_vertices(vertices,geometry.bindings,geometry.palette);
                if(geometry.post_identity_placement)place_vertices(vertices,0,{});
              }
              if(actual.size()!=expected.size())throw FormatError("GPU first-person source selection changed the original CPU vertex extent");
              if(!actual.empty() && std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(Vertex))) {
                const auto a=std::as_bytes(std::span{actual}),b=std::as_bytes(std::span{expected});
                std::size_t byte{};while(byte<a.size() && a[byte]==b[byte])++byte;
                throw FormatError("GPU first-person source bank differs from original CPU selection/placement at vertex "+
                    std::to_string(byte/sizeof(Vertex))+" byte "+std::to_string(byte%sizeof(Vertex)));
              }
            }
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
      }
      if(source_particle_world && !options.capture_third_person && !p.dead && !third_person_vehicle_view) {
        const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,true});
        MuzzleEffectView effects_view;
        effects_view.camera=source_view_shader_bank(foreground_render_camera,1024.f,camera.position,
            full_surface_viewport({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top},drawable_w,drawable_h),true);
        effects_view.fog=bsp_fog_constants(fog,camera.position,view_forward);
        const auto &basis=foreground_render_camera.camera_to_world.basis;
        effects_view.orientation={basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,basis[2].x,basis[2].y,basis[2].z};
        effects_view.position=camera.position;effects_view.forward=view_forward;effects_view.up=camera.oriented?camera.up:basis[2];
        effects_view.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
        effects_view.width=drawable_w;effects_view.height=drawable_h;
        source_particle_world->render_model_view([&](auto &source) {
          auto particles=source_particle_draws.draw_first_person(source,visibility,effects_view,i,
              foreground_projection_angle,
              source_angles.lighting_distance_scale);
          if(options.frames || options.capture_tick)
            std::cout<<"Source FP particles: frame="<<frames<<" view="<<i<<" draws="<<particles.size()<<'\n';
          foreground_particles[i].insert(foreground_particles[i].end(),std::make_move_iterator(particles.begin()),std::make_move_iterator(particles.end()));
        });
        if(options.effect_observer)options.effect_observer(i,current.tick,foreground_particles[i]);
      }
      measure(timing.view[i].first_person_ms);
      ModelView view;
      if(options.model_skin_gpu_owner_observer)
        view.model_skin_gpu_owner_observer=[&,i,tick=current.tick](const auto &sample) {
          options.model_skin_gpu_owner_observer(i,tick,sample);
        };
      view.camera = camera;
      view.screen_flash=screen_flashes[i].evaluate(*rumble_tables);
      if(reference_player)view.source_view_clear=reference_player->view_clear;
      view.left = rect.left;
      view.top = rect.top;
      view.width = rect.right - rect.left;
      view.height = rect.bottom - rect.top;
      view.vertices = world;
      if(objective_goal_material!=null_datum) {
        auto& queue=objective_goal_queues[i];auto& geometry=objective_goal_vertices[i];auto& bindings=objective_goal_bindings[i];
        queue.reset_world();geometry.clear();bindings.clear();
        for(const auto& goal:current.players[id].source_objective_presentation.goals) {
          if(goal.shader!=objective_goal_shader)throw FormatError("Assault goal changed its actual resident shader owner");
          const auto boundary=source_objective_goal_boundary_tu15(goal,camera.position,std::bit_cast<std::int32_t>(current.tick),30);
          if(!boundary)continue;
          SourceTransparentShaderInput input;input.selected_template_layers=objective_goal_metadata.template_lod_layers.front();
          input.shader=goal.shader;input.context=std::bit_cast<std::uint32_t>(goal.designator);input.callback=0x24d760u;
          input.shader_priority_flags=objective_goal_metadata.shader_priority_flags;
          input.center=goal.center;input.camera_position=camera.position;input.camera_forward=view_forward;
          if(!queue.append_shader(input))throw FormatError("Authored Assault goal did not enter its selected source layer");
          geometry.push_back(source_king_boundary_triangles(*boundary,objective_goal_material));
          const auto& vertices=geometry.back();
          bindings.emplace_back(SourceTransparentShaderGeometryDraw{0x24d760u,{goal.shader,input.context,vertices,{},0}});
          if(options.objective_goal_draw_observer)options.objective_goal_draw_observer(i,current.tick,vertices);
        }
        view.world_transparent=SourceTransparentDrawBank{queue.commands(),0,bindings};
      }
      view.deferred_skins=unit_skin_submissions[i];
      view.full_surface = true;
      view.foreground = foregrounds[i];
      view.deferred_foreground_skins=foreground_skin_submissions[i];
      view.camouflage=camouflage;
      view.camouflage_groups=camouflage_groups[i];
      if(options.camouflage_layer_observer)view.completed_camouflage_draw_observer=
          [&,i](auto owner,auto layer,auto implementation,auto first_person,auto primary,auto count) {
            options.camouflage_layer_observer(i,owner,layer,implementation,first_person,primary,count);
          };
      //12E5E0's patchy bank occupies+6C..98 of the published fog state:
      //datum, density pair and distance pair feed41CC0/3385D/33EF9.
      if(fog.patchy_datum!=null_datum && fog.patchy &&
          std::max(fog.patchy->density[0],fog.patchy->density[1])>0.f) {
        if(!(fog.patchy->distance[1]>fog.patchy->distance[0]))
          throw FormatError("Camouflage patchy fog has no source finite interval");
        view.camouflage_fog=SourceShaderFogContext{fog.patchy_datum,
            fog.patchy->distance[0],fog.patchy->distance[1],{view_forward.x,view_forward.y,view_forward.z},
            {}}; // Valid interval does not consume the retained default-color pointer.
      }
      if(user_fov)view.foreground_projection_vertical_angle=foreground_projection_angle;
      if(!options.capture_third_person)view.foreground_particles=foreground_particles[i];
      view.world_particles=world_particles[i];
      view.lens_queries=lens_queries[i];
      view.lens_draws=lens_draws[i];
      if(const auto *hide=std::getenv("HALO2_REFERENCE_LENS_DRAWS_HIDE");hide && *hide)
        view.lens_draws={}; // Pixel isolation; original callbacks and query/RNG state still run.
      std::vector<SourceProjectileGlowOwner> attached_projectile_glows;
      for(const auto &object:current.source_dynamic_objects)if(object.type==5 && object.visible && object.has_world_nodes) {
        const auto nodes=source_body_interpolation.sample(object.object,blend);
        if(nodes.empty())throw FormatError("Attached projectile glow has no completed WORLD node");
        attached_projectile_glows.push_back({object.object,object.definition,nodes.front()});
      }
      view.light_volumes=weapon_projectile_scene.light_volume_view(i,view_forward,render_camera,physical(rect),1024.f,attached_projectile_glows);
      view.light_volume_observer=[&,i](const SourceLightVolumeDraw &draw) {
        if(options.weapon_light_volume_observer)options.weapon_light_volume_observer(i,current.tick,draw);
      };
      if(const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_MODEL_EFFECT_LIGHTS");disabled && *disabled)
        std::erase_if(raw_effect_lights[i],[](const auto& draw){return draw.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object;});
      if(const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_UNIT_EFFECT_LIGHTS");disabled && *disabled) {
        // Finite comparison input: remove only the occupied source identities
        // observed in the positive run, preserving unrelated unit lights.
        std::set<std::uint32_t> identities;std::istringstream list(disabled);
        for(std::string text;std::getline(list,text,',');) {
          std::size_t end{};const auto identity=std::stoull(text,&end,10);
          if(end!=text.size() || identity>0xffffffffull)
            throw FormatError("Unit light comparison requires complete source light identities");
          identities.insert(std::uint32_t(identity));
        }
        if(identities.empty())throw FormatError("Unit light comparison has no source light identities");
        std::optional<std::uint32_t> receiver;
        if(const auto* text=std::getenv("HALO2_REFERENCE_DISABLE_UNIT_EFFECT_LIGHT_OWNER");text && *text) {
          std::size_t end{};const auto value=std::stoull(text,&end,10);
          if(end!=std::strlen(text) || value>=0xffffffffull)
            throw FormatError("Unit light comparison requires a complete source receiver identity");
          receiver=std::uint32_t(value);
        }
        std::erase_if(raw_effect_lights[i],[&](const auto& draw) {
          const auto& owner=draw.source.owner();
          return owner.kind==MenuWorldSourceOwnerKind::dynamic_object && owner.generation &&
              live_unit_base[i].contains(*owner.generation) && identities.contains(draw.light_identity) &&
              (!receiver || *receiver==*owner.generation);
        }); // Pixel-isolation control; original collection and base draws still run.
      }
      view.raw_effect_lights=raw_effect_lights[i];
      if(render_settings.graphics_mode==pstencil::GraphicsMode::PathTracing) {
        if(!source_model_world) {
          const auto effect_emitters=world_effects.lights();
          path_tracing_lights[i]=path_tracing::source_lights(effect_emitters);
          const auto direct_emitters=pstencil_point_lights.lights();
          const auto direct=path_tracing::source_lights(direct_emitters);
          path_tracing_lights[i].insert(path_tracing_lights[i].end(),direct.begin(),direct.end());
        }
        if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<atmosphere.skies.size())
          if(const auto* sky=path_tracing_sky_lighting.select(cache,fog.visible_sky)) {
            view.path_tracing_environment=sky->outdoor_ambient;
            if(const auto light=path_tracing::source_sky_light(*sky))path_tracing_lights[i].push_back(*light);
          }
        view.path_tracing_lights=path_tracing_lights[i];
      }
      if(const auto* disabled=std::getenv("HALO2_REFERENCE_DISABLE_BSP_EFFECT_LIGHTS");disabled && *disabled)
        view.raw_effect_lights={}; // Pixel-isolation control; simulation and collection still run.
      view.source_decals=source_decals[i];
      view.contrails=contrail_draws[i];
      view.contrail_observer=[&,i](const SourceContrailDraw &draw) {
        if(options.weapon_contrail_observer)options.weapon_contrail_observer(i,current.tick,draw);
      };
      if(const auto* hide=std::getenv("HALO2_REFERENCE_CACHE_DECALS_HIDE");hide && *hide)view.source_decals={};
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
      view.pstencil_lightmap_casters=pstencil_lightmap_casters[i];
      view.pstencil_point_casters=pstencil_point_casters[i];
      view.pstencil_native_point_casters=source_model_world!=nullptr;
#ifdef HALO2_HELD_POSE_SHADING_REFERENCE
      view.source_render_policy=held_pose_shading_reference_policy();
#else
      view.source_render_policy=source_geometry_lod_policy(options.local_players);
#endif
      view.source_bsp_admission=&bsp_views[i];
      if(bsp_shaders) {
        view.source_bsp_resident_plan=&*bsp_shaders;
        view.source_bsp_bindings=bsp_vertex_plans[i].bindings;
      }
      if(!p.dead && weapon && weapon->first_person_scope_size.x>0 && weapon->first_person_scope_size.y>0)
        view.scope=scope_copy_frame(scope_source_rectangle(physical(rect),weapon->first_person_scope_size,
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
      if(reference_shadow_frame) {
        if(i || !reference_shadow_plan || !reference_shadow_targets || !reference_shadow_targets->valid())
          throw FormatError("Live shadow GPU view lacks its copied synchronous plan and live attachment use");
        view.raw_source_shadows=*reference_shadow_plan;
        view.raw_source_shadow_targets=&*reference_shadow_targets;
      }
      if(options.view_rectangle_observer)options.view_rectangle_observer(i,
          {view.top,view.left,view.top+view.height,view.left+view.width});
      if(options.view_observer)options.view_observer(i,camera,horizontal_fov,view_zoom[i]);
      if(options.view_state_observer)options.view_state_observer(i,shown,camera);
      if(options.completed_bsp_draw_observer && options.completed_bsp_draw_tick==current.tick) {
        const auto tick=current.tick;
        view.completed_bsp_draw_observer=[&,i,tick](const RendererObservedBspDraw& draw) {
          options.completed_bsp_draw_observer(i,tick,draw);
        };
      }
      if(options.completed_material_upload_observer && options.completed_material_upload_tick &&
          options.completed_material_upload_tick==current.tick) {
        const auto tick=current.tick;
        view.completed_material_upload_observer=[&,i,tick](const RendererCompletedMaterialUpload& upload) {
          options.completed_material_upload_observer(i,tick,upload);
        };
      }
      view.before_ui_vertex = hud_vertices.size();
      for(const auto& [owner,draws]:live_unit_base[i])if(!submitted_unit_base[i].contains(owner))
        throw FormatError("Source unit model-light admission has no submitted matching base");
      views.push_back(view);
      if(source_particle_world && source_lens_queries)
        source_particle_world->render_model_view([&](auto &source){source_lens_queries->finish_view(source,i);});
      if(replica_lens_frame)native_lens->finish_view();
      hud.animate(current, id, i, current.score_limit, current.time_ticks,
                  float(current.tick) * tick_seconds);
      unit_hud.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
      if(uploaded_sensor_tick[i]!=current.tick) {
        if(const auto material=unit_hud.motion_sensor_material(i))
          renderer.update_image(*material,mesh.materials.at(*material).diffuse);
        uploaded_sensor_tick[i]=current.tick;
      }
      const auto board=scoreboard?scoreboard->draw(current,ids,i,drawable_w,drawable_h,split):MultiplayerScoreboard::Draw{};
      if(board.normal_hud && !options.capture_third_person) {
        unit_hud.prompt_device(i,prompt_devices.device(i));
        unit_hud.prompt_boarding_buttons(i,platform.desktop->keyboard_settings,platform.desktop->controller_settings);
        unit_hud.prompt_gamepad_buttons(i,native_action_prompt_controller_buttons(platform.desktop->controller_settings));
        unit_hud.prompt_keyboard_buttons(i,native_action_prompt_keyboard_buttons(platform.desktop->keyboard_settings));
        auto vertices=unit_hud.draw(current,id,i,options.local_players,drawable_w,
            drawable_h,current.score_limit,current.time_ticks,aim_anchor,split,
            SourceHudAimCamera{camera.position,view_forward,camera.up,source_angles.projection_angle,false});
        if(p.source_vehicle_hud.weapon.present) {
          const auto key=std::pair{source_session_player_representation(p.source_configuration,source_game_type),p.source_vehicle_hud.definition};
          const auto found=vehicle_huds.find(key);
          if(found==vehicle_huds.end())throw FormatError("Source vehicle weapon has no prepared authored HUD");
          auto &vehicle_hud=*found->second;
          vehicle_hud.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
          auto vehicle_vertices=vehicle_hud.draw(current,id,i,options.local_players,drawable_w,drawable_h,
              current.score_limit,current.time_ticks,aim_anchor,split,
              SourceHudAimCamera{camera.position,view_forward,camera.up,source_angles.projection_angle,false});
          vertices.insert(vertices.end(),vehicle_vertices.begin(),vehicle_vertices.end());
        }
        if(p.source_vehicle_hud.ownership.handheld_visible) {
          auto weapon_vertices=hud.draw(current,id,i,options.local_players,drawable_w,
              drawable_h,current.score_limit,current.time_ticks,aim_anchor,split,
              SourceHudAimCamera{camera.position,view_forward,camera.up,source_angles.projection_angle,false});
          vertices.insert(vertices.end(),weapon_vertices.begin(),weapon_vertices.end());
          if(p.secondary_weapon.datum!=null_datum) {
            auto &secondary_hud=*offhand_huds.at({source_session_player_representation(p.source_configuration,source_game_type),p.secondary_weapon.definition});
            secondary_hud.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
            auto secondary_vertices=secondary_hud.draw(current,id,i,options.local_players,drawable_w,drawable_h,
              current.score_limit,current.time_ticks,aim_anchor,split,
              SourceHudAimCamera{camera.position,view_forward,camera.up,source_angles.projection_angle,false});
            vertices.insert(vertices.end(),secondary_vertices.begin(),secondary_vertices.end());
          }
        }
        if(p.source_vehicle_hud.ownership.parent_hud!=null_datum) {
          auto &parent=*parent_huds.at({source_session_player_representation(p.source_configuration,source_game_type),p.source_vehicle_hud.ownership.parent_hud});
          parent.animate(current,id,i,current.score_limit,current.time_ticks,float(current.tick)*tick_seconds);
          auto parent_vertices=parent.draw(current,id,i,options.local_players,drawable_w,drawable_h,
              current.score_limit,current.time_ticks,aim_anchor,split,
              SourceHudAimCamera{camera.position,view_forward,camera.up,source_angles.projection_angle,false});
          vertices.insert(vertices.end(),parent_vertices.begin(),parent_vertices.end());
        }
        if(scoreboard)if(const auto status=scoreboard->status_text(current,id)) {
          // 25B53A draws the status independently of current unit ownership.
          // The native HUD currently supplies ordinary opacity one; session
          // black/fade overlays retain their existing later draw position.
          const auto &constants=cache.original_data()->full_scoreboard_paint_constants;
          if(!constants)throw FormatError("Player status lost its original text paint constants");
          auto status_vertices=unit_hud.draw_player_status(*status,current,id,i,options.local_players,
              drawable_w,drawable_h,*constants,split,1.f);
          vertices.insert(vertices.end(),status_vertices.begin(),status_vertices.end());
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
        if(objective_waypoints && p.connected) {
          auto* source=live_world()->source_objects.get();
          const SourceRuntimeMemory* memory{};
          SourceObjectiveOcclusionTu15 occlusion;
          if(source && source->owns_physics()) {
            memory=&source->memory();
            occlusion=[&](Vec3 from,Vec3 to,std::uint32_t flags,std::uint32_t viewer,std::uint32_t target){
              return source->tu15_world().objective_waypoint_occluded(from,to,flags,viewer,target);
            };
          } else {
            if(!live_client())throw FormatError("Objective waypoint lost its local source world");
            if(!objective_replica_memory)objective_replica_memory.emplace(native_objective_waypoint_memory_tu15(cache));
            memory=&*objective_replica_memory;
          }
          const SourceObjectiveViewTu15 objective_view{camera.position,render_camera,
              frustum_projection_input.outer,frustum_projection_input.safe,1.f,objective_waypoints->bitmap_views(),options.local_players};
          const auto quads=source_objective_waypoint_quads_tu15(*memory,p.source_player,
              current.players[id].source_objective_presentation.waypoints,objective_view,occlusion,{0x27f,0x1f80});
          auto waypoint_vertices=objective_waypoints->draw(quads,current,id,i,drawable_w,drawable_h);
          if(options.objective_waypoint_draw_observer)options.objective_waypoint_draw_observer(i,current.tick,waypoint_vertices);
          hud_vertices.insert(hud_vertices.end(),waypoint_vertices.begin(),waypoint_vertices.end());
        }
      }
      // TU1.5's15C2A0 draws the viewport black AFTER world/HUD and BEFORE
      // its separate full scoreboard. Consume the exact binary32 source
      // opacity; loading's later global compositor owns covering both.
      if(gameplay_fade_available) {
        const auto alpha=gameplay_fade.render(i,gameplay_geometry_unresolved);
        if(options.gameplay_fade_observer)options.gameplay_fade_observer(i,current.tick,alpha,gameplay_fade.state());
        if(alpha) {
          const auto material=*session_black_material;const Vec3 rgb{};
          const Vertex a{{view.left,view.top,*alpha},rgb,{0,0},material};
          const Vertex b{{view.left+view.width,view.top,*alpha},rgb,{1,0},material};
          const Vertex c{{view.left+view.width,view.top+view.height,*alpha},rgb,{1,1},material};
          const Vertex d{{view.left,view.top+view.height,*alpha},rgb,{0,1},material};
          hud_vertices.insert(hud_vertices.end(),{a,b,c,a,c,d});
        }
      }
      // PC6278AA/62657B draws47554D's viewport black quad before the
      // separate score display. Keep it after the actual world/HUD and before
      // the score vertices, using the returned packed alpha without retiming.
      if(!gameplay_fade_available && options.session_view_black)if(const auto color=options.session_view_black(i)) {
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
      if(!options.capture_third_person)hud_vertices.insert(hud_vertices.end(),board.paint.draw.vertices.begin(),board.paint.draw.vertices.end());
      views.back().screen_flash_after_ui_vertex=hud_vertices.size();
      if(options.scoreboard_observer)options.scoreboard_observer(i,board.normal_hud,board.layout,board.paint);
      measure(timing.view[i].hud_ms);
    }
    if(audio && source_weapon_audio.world) {
      // Every input above is the actual submitted local camera. The original
      //18EE50 owner performs object/PVS/distance decisions; copied requests
      //are consumed only after its read-only observer boundary has closed.
      std::vector<Vec3> submitted_audio_positions;
      for(const auto& view:object_audio_views)if(view)submitted_audio_positions.push_back(view->position);
      audio->listeners(submitted_audio_positions);
      source_weapon_audio.world->tu15_world().step_native_object_audio(object_audio_views,audio_elapsed);
      for(const auto& request:source_weapon_audio.loops)try{audio->object_loop_request(request);}
        catch(const std::exception& error){
          const auto key="Object loop "+std::to_string(request.arguments[0])+": "+error.what();
          if(reported_world_effect_gaps.insert(key).second)std::cerr<<key<<'\n';
        }
      source_weapon_audio.loops.clear();audio->object_loop_frame(audio_elapsed);
    }
    if(audio && live_client() && current.source_object_loop_emitters) {
      std::vector<Vec3> submitted_audio_positions;
      for(const auto& view:object_audio_views)if(view)submitted_audio_positions.push_back(view->position);
      audio->listeners(submitted_audio_positions);
      // Peers publish completed emitters before their own camera/PAS gate.
      // This machine supplies its genuine BSP and submitted local cameras;
      // the compiled native kernels own audibility and private controllers.
      for(const auto& request:native_object_loop_requests_tu15(*current.source_object_loop_emitters,audio_bsp,object_audio_views))
          try{audio->object_loop_request(request);}catch(const std::exception& error) {
            const auto key="Replicated object loop "+std::to_string(request.arguments[0])+": "+error.what();
            if(reported_world_effect_gaps.insert(key).second)std::cerr<<key<<'\n';
          }
      audio->object_loop_frame(audio_elapsed);
    }
    timing.vehicle_skin_builds=vehicle_frame_poses.builds();
    timing.vehicle_skin_reuses=vehicle_frame_poses.reuses();
    timing.unit_skin_builds=unit_frame_poses.builds();
    timing.unit_skin_reuses=unit_frame_poses.reuses();
    std::vector<UiBackdrop> pause_backdrops;
    if(pause_menu) {
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
      for(const auto &draw:view.light_volumes)for(const auto &binding:draw.bitmap_materials)material_frame.add(binding.material);
      for(const auto &draw:view.effect_lights)material_frame.add(draw.material_slot);
      for(const auto &draw:view.raw_effect_lights)material_frame.add(draw.material_slot);
      for(const auto *bank:{&view.world_transparent,&view.foreground_transparent})if(*bank)
        for(const auto &binding:(*bank)->bindings)
          if(const auto *geometry=std::get_if<SourceTransparentGeometryDraw>(&binding))material_frame.add(geometry->vertices);
      if(view.source_shadows)for(const auto &root:view.source_shadows->roots) {
        for(const auto &draw:root.casters)material_frame.add(draw.material_slot);
        for(const auto &draw:root.receivers)material_frame.add(draw.material_slot);
      }
      if(view.raw_source_shadows)for(const auto& root:view.raw_source_shadows->roots) {
        for(const auto& draw:root.casters)material_frame.add(draw.material_slot);
        for(const auto& draw:root.receivers)material_frame.add(draw.material_slot);
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
      std::optional<FunctionValue> primary_trigger;
      for(const auto &[representation,v]:species)
        for(unsigned i=0;i<ids.size();++i)
          for(unsigned hand=0;hand<2;++hand) {
            if(v.guns.empty())continue;
            const auto &gun=v.guns.at(i*2+hand);
            if(m>=gun.material && m<gun.material+gun.model.mesh.materials.size()) {
              const auto &player=current.players[ids[i]];
              if((hand?player.secondary_weapon.definition:player.weapon)!=representation.second)continue;
              ammunition=hand?player.secondary_weapon.loaded:player.loaded;
              material_weapon=hand?player.secondary_weapon.datum:player.source_weapon;
              material_definition=representation.second;
              shader_state=hand?player.secondary_weapon.shader:player.weapon_shader;
              primary_trigger.reset();
              if(const auto weapon=content.weapons.find(material_definition);weapon!=content.weapons.end() && !weapon->second.triggers.empty()) {
                const auto &trigger=hand?player.secondary_weapon.trigger:player.weapon_trigger;
                primary_trigger=source_weapon_primary_charge(trigger.mode,trigger.ticks,
                    weapon->second.triggers[0].charging_seconds,tick_seconds);
              }
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
          primary_trigger.reset();
          if(const auto weapon=content.weapons.find(material_definition);weapon!=content.weapons.end() && !weapon->second.triggers.empty()) {
            const auto &trigger=hand?player.secondary_weapon.trigger:player.weapon_trigger;
            primary_trigger=source_weapon_primary_charge(trigger.mode,trigger.ticks,
                weapon->second.triggers[0].charging_seconds,tick_seconds);
          }
        }
      const SourceWorldWeaponDefinition *shader_definition=material_definition==null_datum?nullptr:
          &weapon_shader_definitions.at(material_definition);
      std::vector<SourceWeaponMagazineState> magazines;
      if(shader_definition && shader_definition->magazines.size()==1)
        magazines.push_back({std::int16_t(ammunition),0});
      std::optional<std::uint16_t> damage_flags;
      std::optional<SourceWeaponFiringMaterialState> firing_material;
      if(shader_definition && material_weapon!=null_datum && !live_client() &&
          live_world()->source_objects && live_world()->source_objects->owns_physics()) {
        auto &source=*live_world()->source_objects;
        if(source.object_definition(material_weapon)!=material_definition)
          throw FormatError("Carried weapon material flags lost their actual source definition");
        damage_flags=Bytes(source.memory().view(source.object_body(material_weapon)+0x10au,2)).u16(0);
        firing_material=source_weapon_firing_material_owner(source,material_weapon,*shader_definition);
      }
      const ShaderInput input=[&](std::uint32_t name) {
        if(!name)return 0.f;
        if(player_damage_owners[m]>=0) {
          const auto &owner=current.players[unsigned(player_damage_owners[m])];
          return damage_feedback_shader_input(owner.damage_feedback,owner.health,
              owner.shield,owner.dead,player_damage_functions.at(biped_for(owner).datum),
              *player_damage_function_tables,name,float(shader_time),player_damage_names);
        }
        if(shader_definition) {
          if(const auto value=source_weapon_material_input(name,shader_state,shader_definition->magazines,
              magazines,shader_definition->functions,*cache.shader_function_tables(),float(shader_time),primary_trigger,weapon_material_names,
              material_weapon==null_datum?std::optional<std::uint32_t>{}:material_weapon,damage_flags,firing_material))return value->value;
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


    const bool capture = !options.screenshot.empty() &&
        (options.capture_tick ? current.tick == options.capture_tick : frames + 1 == options.frames);
    if (capture && options.capture_tick) {
      if(source_particle_world) {
        const auto &memory=source_particle_world->memory();const auto rng=Bytes(memory.view(0x4f3cd8u,4)).u32(0);
        std::cout<<"Source swarm RNG: primary="<<Bytes(memory.view(rng,4)).u32(0)
            <<" auxiliary="<<Bytes(memory.view(rng+4u,4)).u32(0)<<'\n';
      }
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
        if(!live_client() && p.weapon!=null_datum && content.weapons.at(p.weapon).smg) {
          const auto &hand=live_match().first_person_view(view).hands[0];
          std::cout << " fp_clip=" << (hand.animation_identifier>>16) << " fp_frame=" << hand.frame
                    << " fp_fire_ticks=" << hand.fire_ticks;
        }
      }
      std::cout << '\n';
    }
    SourceFogDrawControls source_fog_control{};
    if(live_world()->source_objects && live_world()->source_objects->owns_physics()) {
      const auto& bank=live_world()->source_objects->memory();
      source_fog_control.suppressed=std::to_integer<unsigned>(bank.view(0x492295u,1)[0])!=0;
      source_fog_control.force_atmospheric=std::to_integer<unsigned>(bank.view(0x492080u,1)[0])!=0;
    }
    // Counterfactual GPU isolation only; original source globals stay read-only.
    if(options.capture_tick || options.frames)
      if(const auto *forced=std::getenv("HALO2_REFERENCE_FORCE_ATMOSPHERIC_FOG");forced && *forced)
        source_fog_control.force_atmospheric=true;
    renderer.source_fog_controls(source_fog_control);
    measure(timing.common_ms);
    // All first-frame source simulation and geometry inputs are now ready.
    // Join the independent loading submitter before ordinary rendering uses
    // this same device; renderer_ready alone precedes source-world bootstrap.
    if(platform.desktop->source_loading_wait_active())platform.desktop->finish_loading();
    if(bsp_reference_scene) {
      const auto postprocess_before=renderer.bsp_postprocess_state();
      auto reference_views=views;
      // The paired diagnostic renderer shares the same real query resources;
      // only the actual frame submits the original query identities.
      for(auto &reference_view:reference_views) {
        reference_view.lens_queries={};
        // The completed observer belongs to the actual submission below,
        // after its synchronous Mesh identity observation, not this paired draw.
        reference_view.completed_material_upload_observer={};
        reference_view.completed_bsp_draw_observer={};
      }
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
    if(options.bsp_submission_observer && bsp_shaders)
      for(unsigned view=0;view<views.size();++view)
        options.bsp_submission_observer(view,current.tick,bsp_vertex_plans[view],mesh.materials);
    if(reference_shadow_frame && (!capture || !reference_shadow_targets || !reference_shadow_targets->valid()))
      throw FormatError("Live shadow GPU reference lost its explicitly requested capture tick or attachment lease");
    std::optional<RendererHostPreparationTiming> host_preparation;
    if(write_host_timings) {
      auto &host=host_preparation.emplace();
      host.frame=timing.frame;host.views=timing.views;
      host.tick_begin=host_tick_begin;host.tick_end=current.tick;
      host.input_ms=timing.input_ms;host.common_ms=timing.common_ms;
      host.pre_render_ms=std::chrono::duration<double,std::milli>(Clock::now()-now).count();
      host.match_step=timing.match_step;host.state_projection=timing.state_projection;host.post_step=timing.post_step;
      host.shared_pre_view_ns=timing.shared_pre_view_ns;host.native_view=timing.native_view;
      for(unsigned view=0;view<host.view.size();++view) {
        const auto &source=timing.view[view];
        host.view[view]={source.camera_lighting_ms,source.static_world_ms,source.objects_ms,
            source.body_ms,source.first_person_ms,source.hud_ms};
      }
    }
    // Render overlap: the renderer draws this frame on its own thread while
    // the next frame's tick block runs here. The tick does not touch renderer
    // inputs (iteration locals that stay alive through the call) or Mesh
    // state; the authority frame closes first because frames cannot nest
    // (offline sessions record nothing, so that is only a flag).
    const bool captured_frame=capture;
    std::function<void()> overlap;
    if(overlap_allowed) {
      source_frame.reset();
      overlap=[&]{
        pending_timing=MultiplayerFrameTiming{};
        const auto saved=measured_at;
        const auto step=advance(pending_timing);
        measured_at=saved;
        ticked_ahead=true;skip_next=step==Advance::skip;stop_next=step==Advance::stop;
      };
    }
    timing.submitted=renderer.draw(views.front().camera,
                      options.frame_capture?options.frame_capture(current):capture?options.screenshot:std::filesystem::path{},
                      {}, false, {}, {}, hud_vertices, views,{},pause_backdrops,
                      nullptr,nullptr,false,host_preparation?&*host_preparation:nullptr,
                      overlap?&overlap:nullptr);
    if(reference_shadow_frame && options.reference_live_shadow_gpu_observer)
      options.reference_live_shadow_gpu_observer(0,current.tick,timing.submitted,reference_shadow_stats);
    if(source_particle_world && source_lens_queries)
      source_particle_world->render_model_view([&](auto &source){source_lens_queries->finish(source,timing.submitted);});
    if(source_particle_world && source_lens_draws)source_lens_draws->finish(timing.submitted);
    if(replica_lens_frame)native_lens->finish(timing.submitted);
    measure(timing.renderer_ms);
    if(options.frame_observer) {
      timing.schedule.renderer_end=observed_schedule_stamp(Clock::now());
      timing.renderer=renderer.frame_timing();
      timing.wall_ms=std::chrono::duration<double,std::milli>(Clock::now()-now).count();
      options.frame_observer(timing);
    }
    if (timing.submitted) {
      if(terminal_current_endpoint){
        authority.adopt_terminal_endpoint(terminal_current_endpoint,current_receipt);
        terminal_current_endpoint.reset();terminal_display_origin=current_receipt;
        terminal_display_fade=gameplay_fade;++terminal_adoptions;
      }else if(terminal_certified_playout&&terminal_display_origin&&terminal_display_origin->same(current_receipt))
        terminal_display_fade=gameplay_fade;
      ++frames;
      if (captured_frame && options.capture_tick) {
        running = false;
        exit_reason = MultiplayerExitReason::captured_tick;
      }
    }
    else
      SDL_Delay(10);
  }
  source_frame.reset();
  mouse_capture.release();
  std::cout << "Multiplayer frames=" << frames << " ticks=" << current.tick
            << " players="
            << std::count_if(current.players.begin(), current.players.end(),
                             [](const auto &p) { return p.connected; })
            << " connected=" << joined << " peak_players=" << peak_players << '\n';
  if(terminal_certified_playout)std::cout<<"Terminal certified playout adoptions="<<terminal_adoptions
      <<" queue_peak="<<terminal_snapshot_peak<<" queue_wire_bytes_peak="<<terminal_snapshot_bytes_peak<<" lag_peak_ticks="<<terminal_lag_peak<<std::endl;
  retain_outcome(exit_reason, current, joined,&current_receipt);
  for(auto *pad:platform.pads)send_controller_rumble(pad,{},false,false,true);
  if(exit_reason==MultiplayerExitReason::session_world_close) {
    // The original logo/sweep screen covers the mainmenu reload; the
    // percentage map-copy wait is only a fallback without that bank.
    if(platform.desktop->vista_loading_configured())platform.desktop->begin_loading();
    else if(platform.desktop->source_loading_wait_configured())platform.desktop->begin_source_loading_wait();
  } else if(options.online_match &&
      (exit_reason==MultiplayerExitReason::online_postgame ||
       exit_reason==MultiplayerExitReason::online_disconnected ||
       exit_reason==MultiplayerExitReason::escape)) {
    // A departing network world reads network_leaving (TU1.5 0x87AC0 6/13).
    if(platform.desktop->network_transition_configured())
      platform.desktop->begin_network_transition(FrontendNetworkTransitionStatus::LeavingGame);
    else if(platform.desktop->source_loading_wait_configured())
      platform.desktop->begin_source_loading_wait();
  }
  return 0;
}
int run_multiplayer(const MultiplayerOptions &options) {
  return run_multiplayer_impl(options, nullptr);
}
int run_multiplayer(const MultiplayerOptions &options,
                    MultiplayerRuntimeOutcome &outcome) {
  return run_multiplayer_impl(options, &outcome);
}
int run_multiplayer(const MultiplayerOptions &options,MultiplayerRuntimeOutcome &outcome,
                    std::shared_ptr<const SourceAuthorityRetainedWorld> &recovery) {
  recovery.reset();return run_multiplayer_impl(options,&outcome,&recovery);
}
} // namespace halo2
