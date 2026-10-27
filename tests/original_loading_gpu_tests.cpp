#define SDL_MAIN_HANDLED
#include "halo2/cache.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_network_transition.hpp"
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
// Original logo/sweep presentation from the staged H2EK bank: blue
// background art, a bright logo/glow at the centre and its ambience owner.
void logo_screen(DesktopSession&desktop,bool validation,const std::filesystem::path&output){
  auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation);
  desktop.renderer_ready(renderer);check(renderer->draw({}),"Initial actual scene failed to present");
  desktop.begin_loading();
  check(desktop.loading_active() && !desktop.source_loading_wait_active(),"Logo loading did not start its own presentation");
  // The spinners grow and the sweep reveals the masked background over the
  // first seconds; the logo is drawn from the first frame.
  pause(1500);
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
  auto next=scene();auto prepared=desktop.make_renderer(next,validation);
  desktop.renderer_ready(prepared);
  check(!desktop.loading_active(),"Logo loading did not join its original fade at renderer admission");
  check(prepared->draw({}),"Prepared scene could not present after the logo loading fade");
}
// Original screen231 dialog: Loading map... while the map world prepares,
// Setting up game... on the new device's admitted-resource frame.
void network_screen(DesktopSession&desktop,bool validation,const std::filesystem::path&output){
  desktop.begin_network_transition(FrontendNetworkTransitionStatus::LoadingMap);
  check(desktop.source_loading_wait_active(),"Network transition did not share the map-wait lifecycle");
  pause(1200);
  const auto loading=capture(desktop,output/"network-loading-map.ppm");
  const auto rows=std::size_t(loading.width)*3*(loading.height/8);
  check(std::all_of(loading.rgb.begin(),loading.rgb.begin()+rows,[](auto p){return p==0;}),
        "Network transition dialog is not drawn over the original black clear");
  check(lit(loading)>200 && lit(loading)<loading.rgb.size()/3,"Network transition dialog is absent or covers the surface");
  auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation);
  desktop.network_transition_status(FrontendNetworkTransitionStatus::SettingUpGame);
  const auto ready=output/"network-setting-up-game.ppm";
  desktop.capture_loading(ready);desktop.renderer_ready(renderer);
  check(desktop.source_loading_wait_active(),"Renderer admission ended the network transition early");
  const auto setting=capture(desktop,ready,false);
  check(setting.rgb!=loading.rgb,"Network transition status did not change at renderer admission");
  desktop.finish_loading();
  check(!desktop.loading_active(),"Network transition did not join at the first gameplay draw");
  check(renderer->draw({}),"First prepared world could not submit after the network transition joined");
  desktop.begin_network_transition(FrontendNetworkTransitionStatus::LeavingGame);
  pause(600);
  capture(desktop,output/"network-leaving-game.ppm");
  auto menu=scene();auto returned=desktop.make_renderer(menu,validation);
  const auto menu_ready=output/"network-leaving-menu-ready.ppm";
  desktop.capture_loading(menu_ready);desktop.renderer_ready(returned);
  capture(desktop,menu_ready,false);desktop.finish_loading();
  check(returned->draw({}),"Menu could not present after the leaving transition");
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
  const Cache menu(menu_path);check(menu.is_mcc(),"Original loading control requires an imported MCC menu");
  const auto content=menu.load_frontend();
  auto painter=std::make_shared<FrontendNetworkTransitionPainter>(content,gameplay_font_directory(menu));
  check(!painter->materials().empty() && painter->materials().size()<=8,
        "Network transition dialog retained more than its own widget owners");
  for(const auto& material:painter->materials())
    check(material.name.starts_with("network-transition/"),"Network transition owner lost its admission name");
  const auto loading=painter->draw(FrontendNetworkTransitionStatus::LoadingMap,5,0);
  const auto setting=painter->draw(FrontendNetworkTransitionStatus::SettingUpGame,5,0);
  check(!loading.empty() && !setting.empty() && loading.size()!=setting.size(),
        "Network transition status strings did not reach the pane text");
  const auto output=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(output);
  DesktopSession desktop;
  check(configure_h2ek_original_loading_presentation(desktop,cache,true),"Staged H2EK loading bank was not admitted");
  desktop.configure_network_transition(painter);
  check(desktop.vista_loading_configured() && desktop.network_transition_configured() && !desktop.loading_active(),
        "Original loading owners invented an initial active interval");
  desktop.window(1280,720,true);
  logo_screen(desktop,validation,output);
  network_screen(desktop,validation,output);
  std::cout<<"PASS H2EK logo/sweep loading and original screen231 network transition: captures, status change and forward/reverse handoff\n";
  return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
