#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include "halo2/native_vista_loading_audio.hpp"
#include <SDL_vulkan.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace {
using namespace halo2;
void require(bool value,const std::string& message){if(!value)throw FormatError(message);}
template<class Predicate> void await(Predicate done,const char *message) {
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!done()) {
    require(std::chrono::steady_clock::now()<deadline,message);
    SDL_PumpEvents();std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}
Mesh scene() {
  Mesh mesh;Material material;material.name="bridge CPU scene";
  material.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
  mesh.materials.push_back(std::move(material));return mesh;
}
void capture(DesktopSession& desktop,const std::filesystem::path& path,unsigned width,unsigned height) {
  std::filesystem::remove(path);
  desktop.capture_loading(path);
  await([&]{return std::filesystem::is_regular_file(path);},"source loading thread did not capture");
  // The snapshot lock joins the in-progress write after filesystem creation.
  (void)desktop.loading_snapshot();
  std::ifstream file(path,std::ios::binary);std::string magic;unsigned w{},h{},maximum{};
  file>>magic>>w>>h>>maximum;
  require(magic=="P6" && w==width && h==height && maximum==255,"loading capture retained the wrong surface extent");
}
// The departed renderer deliberately retains this address. After the CPU bank
// dies, a Linux guard page makes any accidental loading resize dereference a
// hard failure even when the renderer library is built without sanitizers.
class GuardedAtmosphere {
public:
  GuardedAtmosphere() {
#if defined(__linux__)
    const auto page=std::size_t(sysconf(_SC_PAGESIZE));
    bytes_=((sizeof(BspAtmosphere)+page-1)/page)*page;
    allocation_=mmap(nullptr,bytes_,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(allocation_==MAP_FAILED)throw FormatError("cannot allocate guarded CPU atmosphere");
    value_=new(allocation_)BspAtmosphere;
#else
    value_=new BspAtmosphere;
#endif
    value_->gradient={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
  }
  ~GuardedAtmosphere() {
    if(value_)value_->~BspAtmosphere();
#if defined(__linux__)
    if(allocation_ && allocation_!=MAP_FAILED)munmap(allocation_,bytes_);
#else
    if(value_)::operator delete(value_);
#endif
  }
  BspAtmosphere& value(){return *value_;}
  void depart() {
    value_->~BspAtmosphere();
#if !defined(__linux__)
    ::operator delete(value_);
#endif
    value_=nullptr;
#if defined(__linux__)
    if(mprotect(allocation_,bytes_,PROT_NONE)!=0)throw FormatError("cannot guard departed CPU atmosphere");
#endif
  }
private:
  BspAtmosphere *value_{};
#if defined(__linux__)
  void *allocation_{};std::size_t bytes_{};
#endif
};
}

int main(int argc,char** argv) {
  try {
    require(argc==3,"genuine loading assets and output directory required");
    const auto output=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(output);
    std::mutex gains_mutex;std::vector<std::uint32_t> gains;
    unsigned audio_begins{},audio_ends{};bool audio_active{};
    auto begin_audio=[&] {
      std::lock_guard lock(gains_mutex);
      require(!audio_active,"loading audio admission overlapped its previous interval");
      audio_active=true;++audio_begins;
    };
    auto end_audio=[&] {
      std::lock_guard lock(gains_mutex);
      require(audio_active,"loading audio shutdown lost its admitted interval");
      audio_active=false;++audio_ends;
    };
    auto gain_audio=[&](std::uint32_t gain) {
      std::lock_guard lock(gains_mutex);
      require(audio_active,"graphics gain reached an unadmitted or stopped audio owner");
      gains.push_back(gain);
    };
    DesktopSession desktop;desktop.configure_loading(argv[1]);
    auto *window=desktop.window(640,480,true);
    GuardedAtmosphere bank;
    std::weak_ptr<Renderer> departed;
    {
      auto mesh=scene();auto renderer=desktop.make_renderer(mesh,true);
      renderer->scene_bsp_atmosphere(bank.value());desktop.renderer_ready(renderer);
      require(renderer->draw({}),"initial ordinary scene did not present");
      departed=renderer;desktop.begin_loading();
    } // Both root Mesh and the local Renderer owner have departed.
    bank.depart();
    await([&]{return desktop.loading_snapshot()->submitted_frames>=2;},"loading did not continue after CPU scene departure");
    require(!departed.expired(),"loading lost its retained GPU device");
    capture(desktop,output/"departed-menu.ppm",640,480);
    // This recreates loading attachments while the ordinary BSP bank is
    // inaccessible, proving that the dedicated path does not borrow it.
    SDL_SetWindowSize(window,960,540);SDL_PumpEvents();
    int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);
    require(width==960 && height==540,"bridge test window did not resize");
    capture(desktop,output/"departed-menu-wide.ppm",960,540);
    // Binding is normally performed before begin. Complete this first handoff
    // without an assumed audio consumer, then exercise the explicit service.
    std::weak_ptr<Renderer> world;
    {
      auto mesh=scene();auto renderer=desktop.make_renderer(mesh,true);world=renderer;
      require(!departed.expired(),"old GPU device died before presentation admission");
      desktop.renderer_ready(renderer);
      const auto state=*desktop.loading_snapshot();
      require(!state.active && state.fade_complete && state.gain_bits==0 && state.fade_active,
          "actual source fade/join did not complete");
      require(departed.expired(),"old Vulkan device survived completed handoff");
      require(renderer->draw({}),"prepared world did not present after source join");
      bool incomplete=false;
      try{desktop.set_loading_audio_services({begin_audio,gain_audio,{}});}
      catch(const FormatError&){incomplete=true;}
      require(incomplete,"incomplete loading audio lifetime services accepted");
      desktop.set_loading_audio_services({begin_audio,gain_audio,end_audio});
      desktop.begin_loading();
      require(audio_active && audio_begins==1 && !audio_ends,
          "audio admission did not precede return from graphics-thread creation");
      bool late=false;
      try{desktop.set_loading_audio_services({begin_audio,gain_audio,end_audio});}
      catch(const FormatError&){late=true;}
      require(late,"active loading audio services could be replaced");
    }
    await([&]{return desktop.loading_snapshot()->submitted_frames>=2;},"reverse loading did not retain its GPU owner");
    capture(desktop,output/"departed-world.ppm",960,540);
    {
      auto mesh=scene();auto renderer=desktop.make_renderer(mesh,true);
      desktop.renderer_ready(renderer);
      require(world.expired(),"world device was not released on menu return");
      require(desktop.loading_snapshot()->fade_complete && !desktop.loading_active(),"reverse source fade did not join");
      require(renderer->draw({}),"returned menu did not present after source join");
      require(!audio_active && audio_begins==1 && audio_ends==1,
          "completed graphics join did not close its audio owner exactly once");
      {
        std::lock_guard lock(gains_mutex);
        require(gains.size()>1 && gains.back()==0,"original782052 gain argument service was not retained");
      }
      desktop.begin_loading();desktop.cancel_loading();
      require(!desktop.loading_active() && !desktop.loading_snapshot()->fade_complete,
          "native cancellation fabricated fade readiness");
      require(!audio_active && audio_begins==2 && audio_ends==2,
          "cancellation did not stop its independently admitted audio owner");
      desktop.cancel_loading();
      require(audio_ends==2,"repeated cancellation closed loading audio twice");
      // Combined GPU/device boundary: the same adapter used by main consumes
      // the original four voice banks while actual loading frames present.
      // A dummy device exercises callbacks and shutdown without speaker proof.
      require(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"cannot select loading test audio device");
      auto native_audio=std::make_shared<NativeVistaLoadingAudio>(
          std::filesystem::path(argv[1])/"loading-tone.xbox-adpcm");
      desktop.set_loading_audio_services({
          [native_audio]{native_audio->begin(false);},
          [native_audio](std::uint32_t gain){native_audio->gain(gain);},
          [native_audio]{native_audio->end();}});
      desktop.begin_loading();
      await([&] {
        const auto audio=native_audio->snapshot();
        require(audio.failure.empty(),"native loading audio failed: "+audio.failure);
        return audio.device_open && audio.worker_running && audio.mixed_output_frames>=1024;
      },"native loading voices did not consume PCM during graphics loading");
      capture(desktop,output/"native-audio-loading.ppm",960,540);
      desktop.finish_loading();
      const auto stopped_audio=native_audio->snapshot();
      require(!stopped_audio.device_open && !stopped_audio.worker_running &&
          !stopped_audio.source.device && !stopped_audio.source.worker &&
          stopped_audio.source.attenuation==-5000 && stopped_audio.failure.empty(),
          "graphics fade/join did not forward final gain and close native loading voices");
      desktop.set_loading_audio_services({[&] {
        begin_audio();throw FormatError("injected loading audio admission failure");
      },gain_audio,end_audio});
      bool admission_failure=false;
      try{desktop.begin_loading();}
      catch(const FormatError& error){admission_failure=std::string(error.what())=="injected loading audio admission failure";}
      require(admission_failure && !desktop.loading_active() && !audio_active &&
          audio_begins==3 && audio_ends==3,
          "partial audio admission failure did not release its source interval");
      desktop.set_loading_audio_services({begin_audio,gain_audio,end_audio});
      desktop.set_loading_gain_service([](std::uint32_t){throw FormatError("injected loading gain service failure");});
      desktop.begin_loading();bool propagated=false;
      try{desktop.finish_loading();}
      catch(const FormatError& error){propagated=std::string(error.what())=="injected loading gain service failure";}
      require(propagated && !desktop.loading_active(),"loading worker failure was not joined and propagated");
      require(!audio_active && audio_begins==4 && audio_ends==4,
          "graphics worker failure retained its audio owner");
      desktop.cancel_loading();
    }
    std::cout<<"PASS live Vulkan loading bridge: CPU bank guard, resize, explicit swapchain handoff, forward/reverse source fade/join, independent audio lifetime, original four-voice SDL dummy callbacks, admission failure, cancellation and worker exception\n";
  } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
