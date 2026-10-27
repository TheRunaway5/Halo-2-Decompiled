#define SDL_MAIN_HANDLED
#include "halo2/cache.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/native_source_loading_wait.hpp"
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
Mesh scene(){
  Mesh mesh;Material material;material.name="source loading handoff control";
  material.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
  mesh.materials.push_back(std::move(material));return mesh;
}
std::vector<unsigned char> capture(DesktopSession&desktop,const std::filesystem::path&path,bool request=true){
  if(request)desktop.capture_loading(path);
  await([&]{return std::filesystem::is_regular_file(path);},"Native source wait failed to capture its actual surface");
  (void)desktop.loading_snapshot();
  std::ifstream input(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
  input>>magic>>width>>height>>maximum;
  check(magic=="P6"&&width==640&&height==480&&maximum==255,"Source wait capture has the wrong actual surface");
  check(input.get()=='\n',"Source wait capture has an invalid pixel boundary");
  std::vector<unsigned char> pixels(std::size_t(width)*height*3);
  input.read(reinterpret_cast<char*>(pixels.data()),std::streamsize(pixels.size()));
  check(bool(input),"Source wait capture did not retain its complete submitted surface");
  check(std::all_of(pixels.begin(),pixels.begin()+std::size_t(width)*200*3,[](auto p){return p==0;}),
        "Original MP wait did not clear the departing world/scoreboard to black");
  const auto lit=std::count_if(pixels.begin(),pixels.end(),[](auto p){return p!=0;});
  check(lit>100 && lit<std::ptrdiff_t(pixels.size()/4),"Actual native loading caption is absent or lost its black full surface");
  return pixels;
}
}
int main(int argc,char**argv)try{
  if(argc!=3 && argc!=4)return 77;
  if(!std::getenv("DISPLAY"))return 77;
  const bool validation=argc==3;
  check(validation || std::string_view(argv[3])=="--without-validation","Unknown GPU qualification mode");
  SDL_SetMainReady();const Cache menu(argv[1]);check(menu.is_mcc(),"Source wait production control requires MCC input");
  const auto fonts=load_gameplay_font_source(gameplay_font_directory(menu));
  NativeSourceLoadingWait text(fonts);text.update(0,0);
  check(text.state().text==u"LOADING 0%"&&!text.vertices().empty()&&text.materials().size()==1,
        "Admitted native font3 did not produce the original wait caption");
  const auto zero_vertices=text.vertices().size();text.update(0x3f800000u,0);
  check(text.state().text==u"LOADING 100%"&&text.vertices().size()>zero_vertices,
        "Source wait did not consume the actual completed-resource input");
  const auto output=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(output);
  DesktopSession desktop;desktop.configure_source_loading_wait(fonts);desktop.window(640,480,true);
  check(desktop.source_loading_wait_configured()&&!desktop.loading_active(),"Source wait invented an initial active interval");
  std::weak_ptr<Renderer> departed;
  {
    auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation);departed=renderer;
    desktop.renderer_ready(renderer);check(renderer->draw({}),"Initial actual scene failed to present");
    desktop.begin_loading();
    check(!desktop.loading_active(),"Source-only admission invented an unrelated Vista/campaign presentation");
    desktop.begin_source_loading_wait();
  }
  const auto before_map=capture(desktop,output/"before-map.ppm");
  check(!departed.expired(),"Actual wait lost the departing GPU device with its CPU scene");
  {
    auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation);
    check(!departed.expired(),"Map upload destroyed the still-presenting device before actual admission");
    const auto ready_capture=output/"map-resources-ready.ppm";
    desktop.capture_loading(ready_capture);
    desktop.renderer_ready(renderer);
    check(desktop.source_loading_wait_active()&&departed.expired(),
          "Renderer admission either ended the wait early or retained the departed device");
    // Ordinary source first-frame preparation updates these same Vulkan
    // buffers after resource admission. The retained loading frame must stay
    // visible while its submitter has yielded ownership of the new device.
    const auto admitted_frames=desktop.loading_snapshot()->submitted_frames;
    bool late_capture{};try{desktop.capture_loading(output/"late-source-capture.ppm");}
    catch(const FormatError&){late_capture=true;}
    check(late_capture,"Late capture could wake the loading submitter during ordinary GPU preparation");
    renderer->update_material_constants(mesh.materials);
    check(desktop.loading_snapshot()->submitted_frames==admitted_frames,
          "Admitted loading submitter raced first-frame material preparation");
    check(capture(desktop,ready_capture,false)!=before_map,
          "Submitted caption did not change from actual requested to admitted map resources");
    desktop.finish_loading();
    check(!desktop.loading_active()&&!desktop.loading_snapshot()->fade_active&&!desktop.loading_snapshot()->fade_complete,
          "Original MP wait borrowed an unrelated fade or failed to join");
    check(renderer->draw({}),"First prepared world could not submit after the independent wait joined");
    desktop.begin_source_loading_wait();
  }
  capture(desktop,output/"before-mainmenu.ppm");
  {
    auto mesh=scene();auto renderer=desktop.make_renderer(mesh,validation);
    const auto ready_capture=output/"mainmenu-resources-ready.ppm";
    desktop.capture_loading(ready_capture);desktop.renderer_ready(renderer);
    check(desktop.source_loading_wait_active(),"Mainmenu GPU admission bypassed its actual explicit source factory boundary");
    capture(desktop,ready_capture,false);desktop.finish_loading();
    check(renderer->draw({}),"Prepared menu could not present after reverse loading handoff");
  }
  std::cout<<"PASS admitted MCC font3 source wait, actual forward/reverse Vulkan device handoff and captures; no fixed dwell or borrowed campaign fade\n";
  return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
