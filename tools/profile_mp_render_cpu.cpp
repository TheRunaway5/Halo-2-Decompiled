#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_shadow_native_prefix_tu15.hpp"
#include <SDL.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <bit>
#include <cstdlib>
#include <optional>
#include <string_view>

namespace {
struct FixedCaptureDirectory {
  std::filesystem::path path;
  FixedCaptureDirectory() {
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    if(!scratch || !*scratch)throw halo2::FormatError("Fixed profiling requires owned HALO2_TEST_SCRATCH");
    const auto parent=std::filesystem::weakly_canonical(scratch);
    if(!std::filesystem::is_directory(parent))throw halo2::FormatError("Fixed profiling scratch owner is absent");
    for(unsigned index=0;index<1024;++index) {
      const auto candidate=parent/("profile-mp-render-fixed-"+std::to_string(index));
      if(std::filesystem::create_directory(candidate)){path=candidate;return;}
    }
    throw halo2::FormatError("Allocate fixed profiling capture directory");
  }
  FixedCaptureDirectory(const FixedCaptureDirectory&)=delete;
  FixedCaptureDirectory &operator=(const FixedCaptureDirectory&)=delete;
  void cleanup() {
    if(path.empty())return;
    std::filesystem::remove_all(path);
    if(std::filesystem::exists(path))throw halo2::FormatError("Fixed profiling capture storage was not removed");
    path.clear();
  }
  ~FixedCaptureDirectory(){if(!path.empty()){std::error_code error;std::filesystem::remove_all(path,error);}}
};
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

// Actual renderer timings, with no alternate simulation or pose provider.
// Ordinary profiling remains interactive. Fixed diagnostics deliver a finite
// neutral script with pinned source RNG and capture the final frame; its
// screenshot cost and any explicitly enabled GPU readback remain separate.
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
    // Declared before the world so exceptional teardown releases source file
    // owners before this directory's last-resort cleanup.
    std::optional<FixedCaptureDirectory> fixed_capture;
    halo2::MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=unsigned(std::stoul(argv[4]));
    options.frames=unsigned(std::stoul(argv[5]));
    const auto requested_frames=options.frames;
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
    unsigned fixed_input_callbacks{},fixed_rendered_frames{},source_rng_receipts{};
    std::array<float,4> fixed_yaws{};
    std::uint32_t source_rng_initial_primary{},source_rng_initial_auxiliary{};
    const halo2::SourceRuntimeMemory *source_rng_memory{};
    options.input_observer=[&](std::uint32_t step,std::span<const halo2::Controls> input,const halo2::MultiplayerState &state){
      verify_controllers();
      for(const auto &control:input)
        if(control.forward!=0 || control.right!=0 || control.jump || control.crouch || control.fire || control.reload)
          throw halo2::FormatError("Profiling received external gameplay input");
      if(fixed) {
        if(step!=fixed_input_callbacks || step>=requested_frames || input.size()!=options.local_players)
          throw halo2::FormatError("Fixed profiling omitted or exceeded declared real commands");
        for(unsigned view=0;view<input.size();++view)
          if(std::bit_cast<std::uint32_t>(input[view].yaw)!=std::bit_cast<std::uint32_t>(fixed_yaws[view]) || input[view].pitch!=0)
            throw halo2::FormatError("Fixed profiling delivered another prepared-player yaw/pitch");
        ++fixed_input_callbacks;
      }
      observed_tick=state.tick;observed_state=state_signature(state);
    };
    if(argc>=10)options.ui_assets=argv[9];
    if(options.frames<60 || (options.local_players!=1 && options.local_players!=2 && options.local_players!=4))
      throw halo2::FormatError("Use one/two/four views and at least sixty frames");
    if(fixed) {
      if(requested_frames>1000000u-60u)throw halo2::FormatError("Fixed profiling capture exceeds its finite tick bound");
      fixed_capture.emplace();
      options.input_script=fixed_capture->path/"neutral-inputs.tsv";options.screenshot=fixed_capture->path/"final.ppm";
      options.source_object_savegame_directory=fixed_capture->path/"source";
      options.frames=0;options.capture_tick=60+requested_frames;options.reference_auxiliary_seed=1;
      options.starting_weapon=0;options.session_random_seed=0x1376c0;
      options.desktop->auxiliary_random()=1;
      options.source_object_world=true;options.variant.values.set(76,1);
      options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
      options.source_world_observer=[&](const halo2::SourceRuntimeMemory &memory) {
        const auto owner=halo2::Bytes(memory.view(0x4f3cd8u,4)).u32(0);
        if(!owner)throw halo2::FormatError("Fixed profiling has no constructed source RNG owner");
        const auto bank=halo2::Bytes(memory.view(owner,8));
        source_rng_initial_primary=bank.u32(0);source_rng_initial_auxiliary=bank.u32(4);
        if(source_rng_initial_auxiliary!=1)throw halo2::FormatError("Fixed profiling source auxiliary seed was not pinned before scenario factories");
        ++source_rng_receipts;source_rng_memory=&memory;
      };
      options.prepared_world=std::make_shared<halo2::MultiplayerPreparedWorld>(options);
      if(source_rng_receipts!=1)throw halo2::FormatError("Fixed profiling omitted its pre-scenario source RNG receipt");
      auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
      std::array<halo2::Controls,halo2::multiplayer_capacity> controls{};
      const auto initial=world.match.state();
      halo2::Vec3 center{};
      for(auto slot:world.local_slots) {
        const auto &p=initial.players[slot].position;
        center.x+=p.x/options.local_players;center.y+=p.y/options.local_players;
      }
      for(auto slot:world.local_slots) {
        const auto &p=initial.players[slot].position;
        controls[slot].yaw=std::atan2(center.y-p.y,center.x-p.x);
      }
      for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
      const auto prepared=world.match.state();
      if(prepared.tick>1000000u-requested_frames)throw halo2::FormatError("Prepared fixed profiling capture exceeds its finite tick bound");
      options.capture_tick=prepared.tick+requested_frames;
      for(unsigned view=0;view<options.local_players;++view) {
        const auto &player=prepared.players[world.local_slots.at(view)];
        if(!player.connected || player.dead || player.source_unit==halo2::null_datum)
          throw halo2::FormatError("Fixed profiling prepared player has no actual living source unit");
        fixed_yaws[view]=player.yaw;
      }
      std::ofstream input(options.input_script);input<<std::setprecision(9);
      for(unsigned step=0;step<requested_frames;++step)for(unsigned view=0;view<options.local_players;++view)
        input<<step<<' '<<view<<" 0 0 0 "<<fixed_yaws[view]<<" 0\n";
      if(!input)throw halo2::FormatError("Write finite fixed profiling neutral input script");
      std::cout<<"Fixed diagnostic workload: session seed=1275584, desktop/source auxiliary seeds=1, source construction primary="
          <<source_rng_initial_primary<<", auxiliary="<<source_rng_initial_auxiliary
          <<", 60 real preparation ticks facing the spawn center, "<<requested_frames<<" real neutral commands per view using completed player yaws, "
          "one original30Hz tick per rendered frame, interpolation=.5, final capture tick="<<options.capture_tick<<"; "
          "timings are CPU-cost comparisons, not ordinary playback FPS acceptance. The final-frame screenshot and optional GPU readback costs must be excluded or reported separately.\n";
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
    output<<"\tpresentation_copy_jobs\tpresentation_copy_bytes\tpresentation_copy_separate_thread\tpresentation_copy_ns\tpresentation_copy_overlap_ns\tpresentation_copy_wait_ns\tmodel_skin_gpu_jobs\tmodel_skin_gpu_vertices\tmodel_skin_gpu_deformed_vertices\tmodel_skin_gpu_verified_vertices\tmodel_skin_gpu_post_identity_vertices"
        <<"\tmodel_skin_cpu_fallback_jobs\tmodel_skin_cpu_fallback_vertices\tmodel_skin_cpu_fallback_deformed_vertices\tmodel_skin_cpu_fallback_post_identity_vertices"
        <<"\tmodel_skin_cpu_fallback_quota_jobs\tmodel_skin_cpu_fallback_allocation_jobs\tmodel_skin_cpu_fallback_mapping_jobs\tmodel_skin_cpu_fallback_vulkan_result";
    output<<"\tmodel_skin_input_allocations\tmodel_skin_input_maps\tmodel_skin_input_unmaps\tmodel_skin_record_pack_bytes\tmodel_skin_record_upload_bytes\tmodel_skin_palette_upload_bytes\tmodel_skin_descriptor_writes\tmodel_skin_record_cache_hits";
    output<<"\tworld_native_prepare_calls\tworld_native_prepare_completed_calls\tworld_native_prepare_nonempty_calls\tworld_native_prepared_draws\tworld_native_record_calls"
        <<"\tworld_native_title_prepared_draws\tworld_native_shadow_generate_prepared_draws\tworld_native_shadow_apply_prepared_draws\tworld_native_light_prepared_draws"
        <<"\tworld_storage_upload_calls\tworld_storage_upload_completed_calls\tworld_storage_uploaded_draws\tworld_native_prepare_ms\tworld_storage_upload_ms";
    output<<"\tthread_cpu_available\tinput_thread_cpu_ms\tpreparation_renderer_thread_cpu_ms\titeration_thread_cpu_ms";
    output<<"\tobserved_tick\tobserved_state_signature";
    output<<"\tunit_skin_builds\tunit_skin_reuses\tvehicle_skin_builds\tvehicle_skin_reuses";
    output<<"\tselected_skin_requests\tselected_skin_reuses";
    output<<"\tshadow_prefix_attempts\tshadow_prefix_admitted\tshadow_prefix_declined_reference"
        "\tshadow_prefix_declined_instruction\tshadow_prefix_declined_view\tshadow_prefix_declined_numeric";
    if(fixed)output<<"\tfixed_requested_frames\tfixed_input_callbacks\tfixed_capture_tick\tfixed_final_capture_frame"
        "\tsource_rng_initial_primary\tsource_rng_initial_auxiliary\tsource_rng_primary\tsource_rng_auxiliary";
    output<<'\n'<<std::setprecision(12);
    auto previous_prefix=halo2::source_shadow_native_prefix_tu15_stats();
    options.frame_observer=[&](const halo2::MultiplayerFrameTiming &sample) {
      if(fixed && !sample.submitted)
        throw halo2::FormatError("Fixed CPU comparison requires every scene frame to submit");
      std::uint32_t source_rng_primary{},source_rng_auxiliary{};
      if(fixed) {
        ++fixed_rendered_frames;
        if(!source_rng_memory)throw halo2::FormatError("Fixed profiling lost its source RNG memory owner");
        const auto owner=halo2::Bytes(source_rng_memory->view(0x4f3cd8u,4)).u32(0);
        if(!owner)throw halo2::FormatError("Fixed profiling lost its source RNG bank owner");
        const auto bank=halo2::Bytes(source_rng_memory->view(owner,8));source_rng_primary=bank.u32(0);source_rng_auxiliary=bank.u32(4);
      }
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
      output<<'\t'<<r.presentation_copy_jobs<<'\t'<<r.presentation_copy_bytes<<'\t'<<r.presentation_copy_separate_thread
          <<'\t'<<r.presentation_copy_ns<<'\t'<<r.presentation_copy_overlap_ns<<'\t'<<r.presentation_copy_wait_ns
          <<'\t'<<r.model_skin_gpu_jobs<<'\t'<<r.model_skin_gpu_vertices<<'\t'<<r.model_skin_gpu_deformed_vertices<<'\t'<<r.model_skin_gpu_verified_vertices<<'\t'<<r.model_skin_gpu_post_identity_vertices
          <<'\t'<<r.model_skin_cpu_fallback_jobs<<'\t'<<r.model_skin_cpu_fallback_vertices
          <<'\t'<<r.model_skin_cpu_fallback_deformed_vertices<<'\t'<<r.model_skin_cpu_fallback_post_identity_vertices
          <<'\t'<<r.model_skin_cpu_fallback_quota_jobs<<'\t'<<r.model_skin_cpu_fallback_allocation_jobs
          <<'\t'<<r.model_skin_cpu_fallback_mapping_jobs<<'\t'<<r.model_skin_cpu_fallback_vulkan_result;
      output<<'\t'<<r.model_skin_input_allocations<<'\t'<<r.model_skin_input_maps<<'\t'<<r.model_skin_input_unmaps<<'\t'<<r.model_skin_record_pack_bytes<<'\t'<<r.model_skin_record_upload_bytes<<'\t'<<r.model_skin_palette_upload_bytes<<'\t'<<r.model_skin_descriptor_writes<<'\t'<<r.model_skin_record_cache_hits;
      output<<'\t'<<r.world_native_prepare_calls<<'\t'<<r.world_native_prepare_completed_calls
          <<'\t'<<r.world_native_prepare_nonempty_calls<<'\t'<<r.world_native_prepared_draws
          <<'\t'<<r.world_native_record_calls<<'\t'<<r.world_native_title_prepared_draws
          <<'\t'<<r.world_native_shadow_generate_prepared_draws<<'\t'<<r.world_native_shadow_apply_prepared_draws
          <<'\t'<<r.world_native_light_prepared_draws<<'\t'<<r.world_storage_upload_calls
          <<'\t'<<r.world_storage_upload_completed_calls<<'\t'<<r.world_storage_uploaded_draws
          <<'\t'<<r.world_native_prepare_ms<<'\t'<<r.world_storage_upload_ms;
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
          <<'\t'<<sample.vehicle_skin_builds<<'\t'<<sample.vehicle_skin_reuses
          <<'\t'<<sample.selected_skin_requests<<'\t'<<sample.selected_skin_reuses;
      // Actual same-executor kernel admissions, independent of frame/skin
      // work counts. The first row includes work since this observer started.
      const auto prefix=halo2::source_shadow_native_prefix_tu15_stats();
      output<<'\t'<<prefix.attempts-previous_prefix.attempts
          <<'\t'<<prefix.admitted-previous_prefix.admitted
          <<'\t'<<prefix.declined_reference-previous_prefix.declined_reference
          <<'\t'<<prefix.declined_instruction-previous_prefix.declined_instruction
          <<'\t'<<prefix.declined_view-previous_prefix.declined_view
          <<'\t'<<prefix.declined_numeric-previous_prefix.declined_numeric;
      previous_prefix=prefix;
      if(fixed)output<<'\t'<<requested_frames<<'\t'<<fixed_input_callbacks<<'\t'<<options.capture_tick<<'\t'<<(observed_tick==options.capture_tick)
          <<'\t'<<source_rng_initial_primary<<'\t'<<source_rng_initial_auxiliary<<'\t'<<source_rng_primary<<'\t'<<source_rng_auxiliary;
      output<<'\n';
    };
    const auto start=std::chrono::steady_clock::now();
    const auto result=halo2::run_multiplayer(options);
    if(!output)throw halo2::FormatError("Write CPU timing output");
    if(fixed) {
      if(result || fixed_input_callbacks!=requested_frames || fixed_rendered_frames!=requested_frames)
        throw halo2::FormatError("Fixed profiling did not complete exactly its declared source commands and submitted frames");
      options.prepared_world.reset();source_rng_memory=nullptr;
      fixed_capture->cleanup();std::cout<<"Fixed profiling scratch script, screenshot and source savegames removed; no raw frame retained.\n";
    }
    std::cout<<"Actual renderer profiling completed in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
        <<" seconds (includes initial asset/device setup).\n";
    return result;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
