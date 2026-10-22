#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_world.hpp"
#include <SDL.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <bit>
#include <string_view>

namespace {
// A bounded workload identity for paired diagnostics. Explicit fields avoid
// padding/pointers; this is not a full memory, framebuffer or parity oracle.
std::uint64_t state_signature(const halo2::MultiplayerState &state) {
  std::uint64_t hash=14695981039346656037ull;
  const auto word=[&](std::uint32_t value){for(unsigned i=0;i<4;++i){hash^=(value>>(8*i))&255u;hash*=1099511628211ull;}};
  const auto real=[&](float value){word(std::bit_cast<std::uint32_t>(value));};
  const auto vector=[&](halo2::Vec3 value){real(value.x);real(value.y);real(value.z);};
  const auto transform=[&](const halo2::Transform &value){real(value.scale);for(auto basis:value.basis)vector(basis);vector(value.position);};
  word(state.tick);
  for(const auto &p:state.players) {
    word(p.connected);word(p.dead);word(p.source_player);word(p.source_unit);word(p.source_weapon);
    vector(p.position);vector(p.velocity);real(p.yaw);real(p.pitch);real(p.health);real(p.shield);
    word(p.weapon);word(p.loaded);word(p.reserve);word(p.score);word(p.kills);word(p.deaths);
  }
  word(std::uint32_t(state.world_vehicles.size()));
  for(const auto &v:state.world_vehicles){word(v.handle);word(v.placement);transform(v.transform);}
  word(std::uint32_t(state.source_body_poses.size()));
  for(const auto &pose:state.source_body_poses){word(pose.object);word(std::uint32_t(pose.nodes.size()));for(const auto &node:pose.nodes)transform(node);}
  return hash;
}
}

// Actual interactive renderer timings, with no alternate simulation or pose
// provider. No screenshot, vertex hashing or GPU query is performed here.
int main(int argc,char **argv) {
  try {
    if(argc<8 || argc>11)throw halo2::FormatError("Supply MAP SHARED UPDATE_XBE VIEWS FRAMES WIDTH OUTPUT_TSV [reference|resident [UI_ASSETS [fixed]]]");
    const bool fixed=argc==11 && std::string_view(argv[10])=="fixed";
    if(argc==11 && !fixed)throw halo2::FormatError("Unknown workload; use fixed for one source tick per rendered frame");
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)throw halo2::FormatError(SDL_GetError());
    const auto verify_controllers=[] {
      for(int index=0;index<SDL_NumJoysticks();++index)
        if(SDL_IsGameController(index))throw halo2::FormatError("Profiling requires zero admitted controllers");
    };
    verify_controllers();
    halo2::MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=unsigned(std::stoul(argv[4]));
    options.frames=unsigned(std::stoul(argv[5]));
    const auto render_width=unsigned(std::stoul(argv[6]));
    // Compositors can clamp oversized windows to the connected display.
    // Use the ordinary internal-resolution path to measure the requested
    // render workload while presenting through a visible 1280x720 window.
    options.width=1280;options.height=720;
    options.desktop=std::make_shared<halo2::DesktopSession>();
    options.desktop->display_settings.internal_width=render_width;
    options.desktop->display_settings.internal_height=render_width*9/16;
    options.starting_weapon=5;options.mute=true;
    if(argc>=9) {
      const std::string mode=argv[8];
      if(mode!="reference" && mode!="resident")throw halo2::FormatError("Unknown geometry mode");
      options.source_bsp_copy_reference=mode=="reference";
    }
    std::uint32_t observed_tick{};std::uint64_t observed_state{};
    options.input_observer=[&](std::uint32_t,std::span<const halo2::Controls> input,const halo2::MultiplayerState &state){
      verify_controllers();
      for(const auto &control:input)
        if(control.forward!=0 || control.right!=0 || control.jump || control.crouch || control.fire || control.reload)
          throw halo2::FormatError("Profiling received external gameplay input");
      observed_tick=state.tick;observed_state=state_signature(state);
    };
    if(argc>=10)options.ui_assets=argv[9];
    if(options.frames<60 || (options.local_players!=1 && options.local_players!=2 && options.local_players!=4))
      throw halo2::FormatError("Use one/two/four views and at least sixty frames");
    if(fixed) {
      options.starting_weapon=0;options.session_random_seed=0x1376c0;
      options.desktop->auxiliary_random()=1;
      options.source_object_world=true;options.variant.values.set(76,1);
      options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
      options.prepared_world=std::make_shared<halo2::MultiplayerPreparedWorld>(options);
      auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
      std::array<halo2::Controls,halo2::multiplayer_capacity> controls{};
      const auto initial=world.match.state();
      for(auto slot:world.local_slots)controls[slot].yaw=initial.players[slot].yaw;
      for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
      std::cout<<"Fixed diagnostic workload: session seed=1275584, auxiliary seed=1, "
          "60 real preparation ticks, one original30Hz tick per rendered frame, interpolation=.5; "
          "timings are CPU-cost comparisons, not ordinary playback FPS acceptance.\n";
    }
    std::ofstream output(argv[7]);
    if(!output)throw halo2::FormatError("Open CPU timing output");
    output<<"frame\tviews\tsubmitted\tbodies_drawn\tbody_pose_builds\tinput_ms\tcommon_ms\trenderer_ms\twall_ms";
    for(unsigned i=0;i<4;++i)for(const auto *name:{"camera_lighting","static_world","objects","body","first_person","hud"})
      output<<"\tv"<<i<<"_"<<name<<"_ms";
    output<<"\trenderer_measured\trenderer_materials\trenderer_views\trenderer_dynamic_vertices\trenderer_width\trenderer_height";
    for(const auto *name:{"fence","global_constants","view_constants","geometry","acquire","commands","submit","present","screenshot"})
      output<<"\trenderer_"<<name<<"_ms";
    output<<"\trenderer_geometry_pack_ms\trenderer_geometry_upload_ms\trenderer_geometry_diagnostics_ms";
    output<<"\trenderer_geometry_plan_ms\trenderer_geometry_bind_ms";
    output<<"\tthread_cpu_available\tinput_thread_cpu_ms\tpreparation_renderer_thread_cpu_ms\titeration_thread_cpu_ms";
    output<<"\tobserved_tick\tobserved_state_signature";
    output<<"\tunit_skin_builds\tunit_skin_reuses\tvehicle_skin_builds\tvehicle_skin_reuses";
    output<<'\n'<<std::setprecision(12);
    options.frame_observer=[&](const halo2::MultiplayerFrameTiming &sample) {
      if(sample.frame==0) {
        auto *window=options.desktop->window(options.width,options.height,false);
        SDL_SetWindowTitle(window,"Halo 2 Performance Benchmark");
        SDL_SetWindowResizable(window,SDL_FALSE);
        std::cout<<"SDL video backend: "<<SDL_GetCurrentVideoDriver()<<'\n';
      }
      output<<sample.frame<<'\t'<<sample.views<<'\t'<<sample.submitted<<'\t'<<sample.bodies_drawn<<'\t'<<sample.body_pose_builds
          <<'\t'<<sample.input_ms<<'\t'<<sample.common_ms<<'\t'<<sample.renderer_ms<<'\t'<<sample.wall_ms;
      for(const auto &v:sample.view)output<<'\t'<<v.camera_lighting_ms<<'\t'<<v.static_world_ms<<'\t'<<v.objects_ms
          <<'\t'<<v.body_ms<<'\t'<<v.first_person_ms<<'\t'<<v.hud_ms;
      const auto &r=sample.renderer;
      output<<'\t'<<r.measured<<'\t'<<r.materials<<'\t'<<r.views<<'\t'<<r.dynamic_vertices<<'\t'<<r.width<<'\t'<<r.height;
      for(const auto elapsed:{r.fence_ms,r.global_constants_ms,r.view_constants_ms,r.geometry_ms,
          r.acquire_ms,r.commands_ms,r.submit_ms,r.present_ms,r.screenshot_ms})output<<'\t'<<elapsed;
      output<<'\t'<<r.geometry_pack_ms<<'\t'<<r.geometry_upload_ms<<'\t'<<r.geometry_diagnostics_ms;
      output<<'\t'<<r.geometry_plan_ms<<'\t'<<r.geometry_bind_ms;
      // Same-thread stamps already supplied by the ordinary frame observer.
      // Host scheduling/presentation waits therefore do not masquerade as CPU
      // work. Keep unavailable platforms explicit instead of reporting zero.
      const auto &schedule=sample.schedule;
      const bool cpu=schedule.measured && schedule.timed_start.thread_cpu_available &&
          schedule.input_end.thread_cpu_available && schedule.renderer_end.thread_cpu_available &&
          schedule.iteration_begin.thread_cpu_available;
      output<<'\t'<<cpu;
      if(cpu)output<<'\t'<<schedule.input_end.thread_cpu_ms-schedule.timed_start.thread_cpu_ms
          <<'\t'<<schedule.renderer_end.thread_cpu_ms-schedule.input_end.thread_cpu_ms
          <<'\t'<<schedule.renderer_end.thread_cpu_ms-schedule.iteration_begin.thread_cpu_ms;
      else output<<"\tNA\tNA\tNA";
      output<<'\t'<<observed_tick<<'\t'<<observed_state<<'\t'<<sample.unit_skin_builds<<'\t'<<sample.unit_skin_reuses
          <<'\t'<<sample.vehicle_skin_builds<<'\t'<<sample.vehicle_skin_reuses<<'\n';
    };
    const auto start=std::chrono::steady_clock::now();
    const auto result=halo2::run_multiplayer(options);
    if(!output)throw halo2::FormatError("Write CPU timing output");
    std::cout<<"Actual renderer profiling completed in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
        <<" seconds (includes initial asset/device setup).\n";
    return result;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
