#define SDL_MAIN_HANDLED
#include "halo2/cache.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_network_transition.hpp"
#include "halo2/frontend_audio.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/mcc_loading_presentation.hpp"
#include "halo2/renderer.hpp"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw FormatError(why);}
template<class F>void await(F done,const char*why){
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
  while(!done()){
    check(std::chrono::steady_clock::now()<end,why);
    SDL_PumpEvents();std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}
void pause(unsigned milliseconds){
  const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(milliseconds);
  while(std::chrono::steady_clock::now()<end){SDL_PumpEvents();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
}
Mesh scene(){
  Mesh mesh;Material material;material.name="original loading handoff control";
  material.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
  mesh.materials.push_back(std::move(material));return mesh;
}
struct Image {unsigned width{},height{};std::vector<unsigned char> rgb;};
Image capture(DesktopSession&desktop,const std::filesystem::path&path,bool request=true){
  if(request)desktop.capture_loading(path);
  await([&]{return std::filesystem::is_regular_file(path);},"Loading presentation failed to capture its actual surface");
  (void)desktop.loading_snapshot();
  std::ifstream input(path,std::ios::binary);std::string magic;Image image;unsigned maximum{};
  input>>magic>>image.width>>image.height>>maximum;
  check(magic=="P6"&&image.width&&image.height&&maximum==255&&input.get()=='\n',"Loading capture is not a complete P6 surface");
  image.rgb.resize(std::size_t(image.width)*image.height*3);
  input.read(reinterpret_cast<char*>(image.rgb.data()),std::streamsize(image.rgb.size()));
  check(bool(input),"Loading capture lost its submitted surface");
  return image;
}
std::size_t lit(const Image&image){return std::count_if(image.rgb.begin(),image.rgb.end(),[](auto p){return p>8;});}
void backdrop(const Image&image){
  // TU1.5 23B971 packs the authored wigl(.85,0,20/255,44/255)
  // overlay as D900142C. Its blend over the cleared surface is RGB0,17,37.
  for(unsigned y=0;y<image.height/8;++y)for(unsigned x=0;x<image.width;++x){
    const auto i=(std::size_t(y)*image.width+x)*3;
    check(image.rgb[i]==0 && image.rgb[i+1]==17 && image.rgb[i+2]==37,
        "Multiplayer loading lost the source full-window blue backdrop");
  }
}
// Original logo/sweep presentation from the staged H2EK bank: blue
// background art, a bright logo/glow at the centre and its ambience owner.
void logo_screen(DesktopSession&desktop,bool validation,const std::filesystem::path&output){
  // The title's real retained audio engine must keep servicing235B9C after
  // the frontend invocation ends, rather than looping underneath loading.
  const auto menu_audio=std::make_shared<FrontendAudioState>();
  menu_audio->engine=std::make_unique<AudioEngine>(false);
  menu_audio->transitions=FrontendMusicTransitions(true,1);
  auto pcm=std::make_shared<PcmSound>();pcm->stereo.assign(512,.1f);pcm->sound_class=32;
  menu_audio->music_voices.push_back(menu_audio->engine->play(pcm,1,0,true));
  desktop.set_loading_update_service([menu_audio]{menu_audio->world_update(7002);});
  auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation,{},nullptr,HALO2_TEST_LOADING_SHADER_DIR);
  desktop.renderer_ready(renderer);check(renderer->draw({}),"Initial actual scene failed to present");
  desktop.begin_loading();
  check(desktop.loading_active() && !desktop.source_loading_wait_active(),"Logo loading did not start its own presentation");
  // The spinners grow and the sweep reveals the masked background over the
  // first seconds; the logo is drawn from the first frame.
  pause(1500);
  check(menu_audio->music_voices.empty() && !menu_audio->transitions.playing(),
        "Retained title music survived campaign loading instead of servicing its source stop rule");
  const auto early=capture(desktop,output/"logo-loading-early.ppm");
  pause(4500);
  const auto frame=capture(desktop,output/"logo-loading.ppm");
  check(frame.rgb!=early.rgb,"Original logo loading animation did not advance");
  std::uint64_t red{},blue{};
  for(std::size_t i=0;i<frame.rgb.size();i+=3){red+=frame.rgb[i];blue+=frame.rgb[i+2];}
  check(blue>red,"Original H2EK loading background lost its blue art");
  // Logo quad rows .4333-.5667 (normal-mode draw 10): bright HALO 2 glyphs.
  std::size_t logo{};
  for(unsigned y=frame.height*45/100;y<frame.height*55/100;++y)
    for(unsigned x=0;x<frame.width;++x) {
      const auto at=(std::size_t(y)*frame.width+x)*3;
      logo+=frame.rgb[at]+frame.rgb[at+1]+frame.rgb[at+2]>450;
    }
  check(logo>std::size_t(frame.width)/4,"Original HALO 2 logo is absent from its authored band");
  unsigned left=frame.width,right{},top=frame.height,bottom{};
  for(unsigned y=frame.height*40/100;y<frame.height*60/100;++y)
    for(unsigned x=0;x<frame.width;++x) {
      const auto i=(std::size_t(y)*frame.width+x)*3;
      if(frame.rgb[i]+frame.rgb[i+1]+frame.rgb[i+2]>600) {
        left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
      }
    }
  const float ink_aspect=float(right-left+1)/float(bottom-top+1);
  // The authenticated kit DDS's >600 stroke bounds are924x102 pixels.
  // Allow resampling at the window's resolution, never an aspect stretch.
  check(right>left && bottom>top && std::abs(ink_aspect-924.f/102.f)<.2f,
      "The imported classic logo is stretched across the widescreen loading surface");
  auto next=scene();auto prepared=desktop.make_renderer(next,validation,{},nullptr,HALO2_TEST_LOADING_SHADER_DIR);
  desktop.renderer_ready(prepared);
  check(!desktop.loading_active(),"Logo loading did not join its original fade at renderer admission");
  check(prepared->draw({}),"Prepared scene could not present after the logo loading fade");
  desktop.set_loading_update_service({});
}
// The application uses network_joining for resident imported maps, then
// network_leaving for the menu reload. No logo ambience may enter either.
void network_screen(DesktopSession&desktop,bool validation,const std::filesystem::path&output){
  struct AudioControl {FrontendAudioState audio;unsigned starts{},updates{},ends{};};
  const auto control=std::make_shared<AudioControl>();
  desktop.set_network_transition_audio_services({
    [control](auto clock){++control->starts;control->audio.transition_begin(clock);},
    [control](auto clock){++control->updates;control->audio.class_update(clock);},
    [control](auto clock){++control->ends;control->audio.transition_end(clock);}});
  desktop.begin_multiplayer_loading();
  check(desktop.source_loading_wait_active(),"Network transition did not share the map-wait lifecycle");
  check(desktop.loading_snapshot()->network_transition==FrontendNetworkTransitionStatus::SettingUpGame,
        "Multiplayer setup selected the campaign logo or the blocking-map-copy status");
  desktop.begin_multiplayer_loading();
  pause(1200);
  const auto loading=capture(desktop,output/"multiplayer-setting-up.ppm");
  backdrop(loading);
  check(lit(loading)>200,"Network transition dialog is absent");
  auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation,{},nullptr,HALO2_TEST_LOADING_SHADER_DIR);
  desktop.network_transition_status(FrontendNetworkTransitionStatus::SettingUpGame);
  const auto ready=output/"network-setting-up-game.ppm";
  desktop.capture_loading(ready);desktop.renderer_ready(renderer);
  check(desktop.source_loading_wait_active(),"Renderer admission ended the network transition early");
  const auto setting=capture(desktop,ready,false);
  backdrop(setting);
  check(lit(setting)>200,"Admitted renderer lost the multiplayer setup dialog");
  check(desktop.loading_snapshot()->network_transition==FrontendNetworkTransitionStatus::SettingUpGame,
        "Resident map setup changed its original status at renderer admission");
  desktop.finish_loading();
  check(!desktop.loading_active(),"Network transition did not join at the first gameplay draw");
  check(renderer->draw({}),"First prepared world could not submit after the network transition joined");
  check(control->starts==1 && control->updates>1 && control->ends==1,"Multiplayer sound-class fade lost its single loading lifetime");
  desktop.begin_multiplayer_loading(FrontendNetworkTransitionStatus::LeavingGame);
  check(desktop.loading_active() &&
      desktop.loading_snapshot()->network_transition==FrontendNetworkTransitionStatus::LeavingGame,
      "Match departure did not select the original Leaving game dialog");
  pause(600);
  const auto leaving=capture(desktop,output/"network-leaving-game.ppm");
  // The source channel's750ms intro must finish before its settled tint.
  pause(200);
  const auto settled_leaving=capture(desktop,output/"network-leaving-settled.ppm");
  backdrop(settled_leaving);
  check(lit(leaving)>200,"Leaving game lost its original dialog");
  auto menu=scene();auto returned=desktop.make_renderer(menu,validation,{},nullptr,HALO2_TEST_LOADING_SHADER_DIR);
  const auto menu_ready=output/"network-leaving-menu-ready.ppm";
  desktop.capture_loading(menu_ready);desktop.renderer_ready(returned);
  check(desktop.loading_active() &&
      desktop.loading_snapshot()->network_transition==FrontendNetworkTransitionStatus::LeavingGame,
      "Menu renderer admission replaced or prematurely dismissed Leaving game");
  capture(desktop,menu_ready,false);desktop.finish_loading();
  check(!desktop.loading_active(),"Leaving game did not finish at the returned menu's first draw");
  check(returned->draw({}),"Menu could not present after the leaving transition");
  check(control->starts==2 && control->ends==2,"Leaving transition did not close its independent audio lifetime");
  desktop.begin_multiplayer_loading();desktop.cancel_loading();
  check(control->starts==3 && control->ends==3 && !desktop.loading_active(),"Cancellation leaked the multiplayer fade owner");
  // Release this test's callbacks before leaving the interval owner.
  desktop.set_network_transition_audio_services({[](auto){},[](auto){},[](auto){}});
}
}
int main(int argc,char**argv)try{
  if(argc!=3 && argc!=4)return 77;
  if(!std::getenv("DISPLAY"))return 77;
  const bool validation=argc==3;
  check(validation || std::string_view(argv[3])=="--without-validation","Unknown GPU qualification mode");
  const std::filesystem::path cache=argv[1];
  const auto menu_path=cache/"halo2/h2_maps_win64_dx11/mainmenu.map";
  if(!std::filesystem::is_regular_file(menu_path) ||
     !std::filesystem::is_regular_file(cache/"presentation/original-loading-v2/manifest.json"))return 77;
  SDL_SetMainReady();
  check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Loading audio control could not select its private dummy device");
  const Cache menu(menu_path);check(menu.is_mcc(),"Original loading control requires an imported MCC menu");
  const auto content=menu.load_frontend();
  auto painter=std::make_shared<FrontendNetworkTransitionPainter>(content,gameplay_font_directory(menu));
  check(!painter->materials().empty() && painter->materials().size()<=8,
        "Network transition dialog retained more than its own widget owners");
  for(const auto& material:painter->materials())
    check(material.name.starts_with("network-transition/"),"Network transition owner lost its admission name");
  const auto loading=painter->draw(FrontendNetworkTransitionStatus::LoadingMap,5,0);
  const auto setting=painter->draw(FrontendNetworkTransitionStatus::SettingUpGame,5,0);
  const auto leaving=painter->draw(FrontendNetworkTransitionStatus::LeavingGame,5,0);
  // Original screen231's upper-left bitmap is128x128 at(-144,109).
  // MCC's256px raster must occupy that logical quad, with no PC1200 transform.
  unsigned corner_vertices{};
  for(const auto &vertex:setting)
    if(painter->materials()[vertex.material].name=="network-transition/ui\\global_bitmaps\\ul_07") {
      check((vertex.position.x==176 || vertex.position.x==304) &&
          (vertex.position.y==131 || vertex.position.y==259),
          "Multiplayer dialog retained the MCC PC geometry instead of TU1.5 screen231");
      ++corner_vertices;
    }
  check(corner_vertices==6,"Original screen231 corner quad is missing");
  check(!loading.empty() && !setting.empty() && loading.size()!=setting.size(),
        "Network transition status strings did not reach the pane text");
  check(!leaving.empty() && leaving.size()!=setting.size(),
        "Leaving game reused the setup status text");
  const auto output=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(output);
  DesktopSession desktop;
  check(configure_h2ek_original_loading_presentation(desktop,cache,true),"Staged H2EK loading bank was not admitted");
  desktop.configure_network_transition(painter);
  check(desktop.vista_loading_configured() && desktop.network_transition_configured() && !desktop.loading_active(),
        "Original loading owners invented an initial active interval");
  desktop.window(1280,std::getenv("HALO2_TEST_LOADING_4_3")?960:720,true);
  {
    FrontendAudioState audio;
    audio.transition_begin(1000);audio.class_update(1500);
    check(audio.transition_classes.gains()[32]==-32.f,"TU1.5 all-class one-second fade lost its midpoint");
    audio.class_update(1400);
    check(audio.transition_classes.gains()[32]==-32.f,
        "An older UI clock query completed the concurrent loading fade");
    audio.class_update(2000);
    check(audio.transition_classes.gains()[32]==-64.f,"TU1.5 transition did not reach -64 dB");
    audio.transition_end(2000);audio.class_update(3000);
    check(audio.transition_classes.gains()[32]==-32.f && audio.transition_classes.gains()[19]==-32.f,
          "Map activation lost its two-second restore or widened the original scripted_dialog_player selector");
  }
  logo_screen(desktop,validation,output);
  network_screen(desktop,validation,output);
  std::cout<<"PASS H2EK logo/sweep loading and original screen231 network transition: captures, status change and forward/reverse handoff\n";
  return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
