#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include <SDL.h>
#include <fstream>
#include <iostream>

// Capture an ordinary first multiplayer frame. Observers reject external input
// and state advancement; they never supply controls, cameras or admission.
int main(int argc,char **argv) {
  try {
    if(argc!=8 && argc!=9)throw halo2::FormatError("Supply MAP SHARED UPDATE_XBE VIEWS WIDTH reference|resident|paired OUTPUT_PREFIX [UI_ASSETS]");
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)throw halo2::FormatError(SDL_GetError());
    const auto verify_controllers=[] {
      for(int i=0;i<SDL_NumJoysticks();++i)
        if(SDL_IsGameController(i))throw halo2::FormatError("Comparison requires zero admitted controllers");
    };
    verify_controllers();
    halo2::MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=unsigned(std::stoul(argv[4]));options.width=unsigned(std::stoul(argv[5]));
    options.height=options.width*9/16;options.frames=1;options.starting_weapon=5;
    options.mute=true;options.hidden=true;options.validation=true;
    if(argc==9)options.ui_assets=argv[8];
    const std::string mode=argv[6],prefix=argv[7];
    if(mode!="reference" && mode!="resident" && mode!="paired")throw halo2::FormatError("Unknown geometry mode");
    options.source_bsp_copy_reference=mode=="reference";
    options.screenshot=prefix+".ppm";
    if(mode=="paired")options.bsp_reference_screenshot=prefix+".reference.ppm";
    std::ofstream output(prefix+".selection",std::ios::binary);
    if(!output)throw halo2::FormatError("Open selection capture");
    const auto scalar=[&]<class T>(T value){output.write(reinterpret_cast<const char *>(&value),sizeof value);};
    const auto records=[&](const auto &values) {
      scalar(std::uint32_t(values.size()));
      for(const auto &v:values){scalar(v.id);scalar(v.flags);scalar(v.association);scalar(v.other);}
    };
    unsigned observed{};
    options.bsp_selection_observer=[&](std::uint32_t tick,unsigned view,const halo2::SourceBspShaderView &input,
        std::span<const halo2::SourceBspShaderPart> parts) {
      if(tick!=0 || view!=observed++)throw halo2::FormatError("Comparison did not capture the ordinary ordered initial frame");
      if(view==0)std::cout<<"SDL video backend: "<<SDL_GetCurrentVideoDriver()<<'\n';
      scalar(tick);scalar(view);
      output.write(reinterpret_cast<const char *>(&input.camera),sizeof input.camera);
      const auto &a=input.admission;
      output.write(reinterpret_cast<const char *>(a.cluster_entries.data()),a.cluster_entries.size());
      for(const auto v:a.marked_clusters)scalar(v);
      records(a.clusters);records(a.instances);
      scalar(std::uint32_t(a.associations.size()));
      for(const auto &group:a.associations)for(const auto v:group)scalar(v);
      scalar(std::uint8_t(a.association_capacity_reached));
      scalar(input.outer_top);scalar(input.outer_bottom);scalar(input.local_players);scalar(std::uint8_t(input.view_modifier));
      scalar(std::uint32_t(parts.size()));
      for(const auto &p:parts) {
        scalar(std::int32_t(p.geometry));scalar(p.section);scalar(p.part);scalar(p.shader_lod);scalar(p.material);
        scalar(p.coverage_byte);scalar(p.instance_alpha);
      }
    };
    options.input_observer=[&](std::uint32_t,std::span<const halo2::Controls> controls,const halo2::MultiplayerState &state) {
      verify_controllers();
      if(state.tick!=0)throw halo2::FormatError("Initial-frame comparison advanced gameplay");
      for(const auto &c:controls)
        if(c.forward!=0 || c.right!=0 || c.jump || c.crouch || c.fire || c.reload)
          throw halo2::FormatError("Comparison received external gameplay input");
    };
    const auto result=halo2::run_multiplayer(options);
    if(observed!=options.local_players || !output)throw halo2::FormatError("Incomplete selection capture");
    std::cout<<"Captured "<<observed<<" ordinary initial views using "<<mode<<" geometry.\n";
    return result;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
