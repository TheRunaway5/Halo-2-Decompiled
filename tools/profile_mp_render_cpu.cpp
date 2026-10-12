#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include <SDL.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

// Actual interactive renderer timings, with no alternate simulation or pose
// provider. No screenshot, vertex hashing or GPU query is performed here.
int main(int argc,char **argv) {
  try {
    if(argc!=8 && argc!=9 && argc!=10)throw halo2::FormatError("Supply MAP SHARED UPDATE_XBE VIEWS FRAMES WIDTH OUTPUT_TSV [reference|resident [UI_ASSETS]]");
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
    options.input_observer=[&](std::uint32_t,std::span<const halo2::Controls> input,const halo2::MultiplayerState &){
      verify_controllers();
      for(const auto &control:input)
        if(control.forward!=0 || control.right!=0 || control.jump || control.crouch || control.fire || control.reload)
          throw halo2::FormatError("Profiling received external gameplay input");
    };
    if(argc==10)options.ui_assets=argv[9];
    if(options.frames<60 || (options.local_players!=1 && options.local_players!=4))
      throw halo2::FormatError("Use one/four views and at least sixty frames");
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
      output<<'\t'<<r.geometry_plan_ms<<'\t'<<r.geometry_bind_ms<<'\n';
    };
    const auto start=std::chrono::steady_clock::now();
    const auto result=halo2::run_multiplayer(options);
    if(!output)throw halo2::FormatError("Write CPU timing output");
    std::cout<<"Actual renderer profiling completed in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
        <<" seconds (includes initial asset/device setup).\n";
    return result;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
