#include "halo2/desktop_session.hpp"
#include "halo2/native_loading_presentation.hpp"
#include "halo2/native_source_loading_wait.hpp"
#include "halo2/renderer.hpp"
#include "halo2/vista_loading_thread.hpp"
#include <SDL_vulkan.h>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

namespace halo2 {
namespace {
struct LoadingCancelled {};
constexpr VistaLoadingFloatingPoint loading_fp{0x027f,0x1f80};
}

// This owner survives the departing menu/world CPU scene. Only immutable GPU
// images and an independently owned loading draw bank cross that boundary.
class DesktopLoadingBridge {
public:
  DesktopLoadingBridge(SDL_Window *window,const VistaLoadingAssets& assets,bool preserve_logo_proportions)
      :window_(window),presentation_(std::in_place,assets,preserve_logo_proportions) {}
  DesktopLoadingBridge(SDL_Window *window,const GameplayFontSource& source)
      :window_(window),source_wait_presentation_(std::in_place,source) {}
  DesktopLoadingBridge(SDL_Window *window,std::shared_ptr<FrontendNetworkTransitionPainter> painter)
      :window_(window),transition_presentation_(std::move(painter)) {
    if(!transition_presentation_ || transition_presentation_->materials().empty())
      throw FormatError("Network transition dialog has no resident owner");
  }
  ~DesktopLoadingBridge(){cancel();}
  void source_wait_assets(const GameplayFontSource& source) {
    std::lock_guard lock(mutex_);rethrow();
    if(current_ || pending_ || snapshot_.active || worker_.joinable())
      throw FormatError("Source loading wait must be configured before its first renderer");
    if(source_wait_presentation_)throw FormatError("Source loading wait is already configured");
    source_wait_presentation_.emplace(source);
  }
  void network_transition_assets(std::shared_ptr<FrontendNetworkTransitionPainter> painter) {
    std::lock_guard lock(mutex_);rethrow();
    if(current_ || pending_ || snapshot_.active || worker_.joinable())
      throw FormatError("Network transition dialog must be configured before its first renderer");
    if(transition_presentation_)throw FormatError("Network transition dialog is already configured");
    if(!painter || painter->materials().empty())throw FormatError("Network transition dialog has no resident owner");
    transition_presentation_=std::move(painter);
  }
  void network_transition_status(FrontendNetworkTransitionStatus status) {
    std::lock_guard lock(mutex_);transition_status_=status;
    if(transition_active_ && snapshot_.active)snapshot_.network_transition=status;
  }

  std::shared_ptr<Renderer> make_renderer(SDL_Window *window,Mesh& mesh,bool validation,
      std::span<const SourceShadowPlanMaterial> shadows,const SourceBspResidentPlan *bsp,
      const std::filesystem::path &shader_directory,
      std::span<const SourceShadowRawPlanMaterial> raw_shadow_materials) {
    unsigned base{},source_base{},transition_base{};
    std::shared_ptr<Renderer> previous;
    {
      std::lock_guard lock(mutex_);rethrow();
      if(!window || (window_ && window_!=window))throw FormatError("loading bridge window changed");
      window_=window;
      if(pending_)throw FormatError("a loading renderer handoff is already pending");
      previous=current_;
      base=unsigned(mesh.materials.size());
      if(presentation_) {
        const auto bank=presentation_->materials();
        mesh.materials.insert(mesh.materials.end(),bank.begin(),bank.end());
      }
      source_base=unsigned(mesh.materials.size());
      if(source_wait_presentation_) {
        const auto bank=source_wait_presentation_->materials();
        mesh.materials.insert(mesh.materials.end(),bank.begin(),bank.end());
      }
      transition_base=unsigned(mesh.materials.size());
      if(transition_presentation_) {
        const auto bank=transition_presentation_->materials();
        mesh.materials.insert(mesh.materials.end(),bank.begin(),bank.end());
      }
      // CPU scene/resource preparation is complete at this boundary. The old
      // device can present the original fade while the new device uploads and
      // compiles. Keep the original fade clock/curve and join before gameplay.
      start_completion_locked();
    }
    // The old device remains usable throughout new texture/geometry uploads.
    // Presentation transfer occurs immediately before the new swapchain.
    auto renderer=std::make_shared<Renderer>(window_,mesh,validation,shadows,bsp,[this] {
      std::lock_guard lock(mutex_);rethrow();
      if(current_) {
        current_->release_presentation();
        retired_=std::move(current_);
      }
    },previous.get(),shader_directory,raw_shadow_materials);
    {
      std::lock_guard lock(mutex_);rethrow();
      pending_=renderer;pending_base_=base;pending_source_base_=source_base;
      pending_transition_base_=transition_base;
    }
    return renderer;
  }

  void ready(const std::shared_ptr<Renderer>& renderer) {
    bool finish{},source_wait{};
    {
      std::lock_guard lock(mutex_);rethrow();
      if(!renderer || renderer!=pending_)
        throw FormatError("loading handoff does not name its prepared renderer");
      finish=snapshot_.active;
      source_wait=source_wait_active_;
      if(finish)renderer->prepare_loading();
      current_=std::move(pending_);current_base_=pending_base_;
      current_source_base_=pending_source_base_;current_transition_base_=pending_transition_base_;
      if(source_wait_active_) {
        // The actual new renderer has completed its GPU resource admission.
        // A later explicit first-draw boundary owns world/UI completion.
        source_wait_progress_=0x3f800000u;source_ready_presented_=false;
        source_wait_quiesced_=source_surface_unavailable_=false;
      }
      // The old swapchain was explicitly released before the new one existed.
      // Its remaining device resources are now safe to destroy on this thread.
      retired_.reset();condition_.notify_all();
    }
    if(finish && source_wait) {
      // The new device first presents its actual admitted-resource frame,
      // then the loader stops submitting. Source bootstrap may update images
      // and material buffers before its first world draw; it cannot race a
      // second queue/debug-buffer owner. The last loading surface stays up.
      std::unique_lock lock(mutex_);
      condition_.wait(lock,[this]{return cancelled_ || failure_ || source_wait_quiesced_;});
      rethrow();
    }
    if(finish && !source_wait)join_with_fade();
  }

  void begin(bool source_wait=false,std::optional<FrontendNetworkTransitionStatus> transition={}) {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active || worker_.joinable())
      throw FormatError("Vista loading thread is already active");
    if(!current_ || pending_)
      throw FormatError("Vista loading requires a ready presentation owner");
    if(transition?!transition_presentation_:source_wait?!source_wait_presentation_:!presentation_)
      throw FormatError("Requested loading presentation has no admitted resource owner");
    // The screen231 dialog shares the map-wait lifecycle: black clear, the
    // new device's admitted-resource frame, then a join at the first draw.
    source_wait=source_wait || transition;
    transition_active_=bool(transition);
    if(transition){transition_status_=*transition;transition_origin_=SDL_GetTicks();}
    source_wait_active_=source_wait;source_wait_progress_=0;
    source_completion_requested_=false;source_ready_presented_=false;
    source_wait_quiesced_=source_surface_unavailable_=false;
    current_->prepare_loading();
    // Original66B9B1/66B9BB initializes deadline from one clock query;
    //66BA84/66BA96 initializes origin from a later clock query. E4AFE8's sweep
    // input starts at its known original PE .data raw value zero. Its
    // indirect producer remains unresolved, so no map percentage is invented.
    fade_={0,0,0x3f800000};
    const auto deadline=SDL_GetTicks();
    snapshot_={};snapshot_.active=true;snapshot_.clock_origin=SDL_GetTicks();
    snapshot_.network_transition=transition;
    snapshot_.deadline=deadline;snapshot_.gain_bits=fade_.gain_bits;
    cancelled_=false;
    completion_started_=false;
    try {
      //66BAAD precedes the original graphics-thread creation66BAB2..66BAC8.
      // Mark ownership first so a partially failed native admission is closed.
      if(!source_wait && audio_.begin){audio_active_=true;audio_.begin();}
      if(transition && transition_audio_.begin) {
        transition_audio_active_=true;transition_audio_.begin(transition_origin_);
      }
      if(transition)std::cout<<"Multiplayer loading: screen=231 status="
          <<(*transition==FrontendNetworkTransitionStatus::LeavingGame?"network_leaving":
             *transition==FrontendNetworkTransitionStatus::LoadingMap?"network_loading_map":"network_joining")<<'\n';
      worker_=std::thread([this,source_wait] {
        try {
          if(source_wait) {source_wait_loop();condition_.notify_all();return;}
          const VistaLoadingThreadServices services{
            [this](std::uint32_t milliseconds) {
              std::unique_lock lock(mutex_);
              if(condition_.wait_for(lock,std::chrono::milliseconds(milliseconds),[this]{return cancelled_;}))
                throw LoadingCancelled{};
              ++snapshot_.sleeps;
            },
            []{return std::uint8_t(0);},
            [this](std::uint8_t mode){return present(mode);},
            [this]{std::lock_guard lock(mutex_);return fade_.active;}};
          (void)vista_loading_thread_run(services);
        } catch(const LoadingCancelled&) {
          // Native cancellation is a lifetime/exception path, not an original
          // readiness signal or a successful fade completion.
        } catch(...) {
          std::lock_guard lock(mutex_);failure_=std::current_exception();
        }
        condition_.notify_all();
      });
    } catch(...) {
      const auto failure=std::current_exception();
      snapshot_.active=false;
      try{end_audio();}catch(...){}
      std::rethrow_exception(failure);
    }
  }

  void join_with_fade() {
    {
      std::lock_guard lock(mutex_);
      if(!snapshot_.active){rethrow();return;}
      if(!failure_) {
        if(!current_ || pending_)throw FormatError("Vista loading fade requires the prepared presentation owner");
        if(source_wait_active_) {
          if(source_wait_progress_!=0x3f800000u)
            throw FormatError("Source loading wait requires actual next-renderer admission before completion");
          source_completion_requested_=true;
        }
        else start_completion_locked();
      }
      condition_.notify_all();
    }
    // The original joins its graphics thread after fade begin. There is no
    // estimated map duration, fixed loading dwell, or synthetic completion.
    if(worker_.joinable())worker_.join();
    std::lock_guard lock(mutex_);snapshot_.active=false;capture_.clear();
    //66A0AB shuts down the independent ambience after the graphics join,
    // including a native presentation failure. Preserve the first failure.
    try{end_audio();}catch(...){if(!failure_)failure_=std::current_exception();}
    if(snapshot_.network_transition)std::cout<<"Multiplayer loading: graphics joined before first world draw; submitted_frames="
        <<snapshot_.submitted_frames<<'\n';
    rethrow();
  }

  void cancel() noexcept {
    {
      std::lock_guard lock(mutex_);cancelled_=true;condition_.notify_all();
    }
    if(worker_.joinable())worker_.join();
    std::lock_guard lock(mutex_);snapshot_.active=false;capture_.clear();
    try{end_audio();}catch(...){if(!failure_)failure_=std::current_exception();}
  }
  DesktopLoadingSnapshot snapshot() const {
    std::lock_guard lock(mutex_);return snapshot_;
  }
  bool source_wait_configured() const {
    std::lock_guard lock(mutex_);return bool(source_wait_presentation_);
  }
  bool vista_configured() const {
    std::lock_guard lock(mutex_);return bool(presentation_);
  }
  bool source_wait_active() const {
    std::lock_guard lock(mutex_);return snapshot_.active && source_wait_active_;
  }
  bool network_transition_configured() const {
    std::lock_guard lock(mutex_);return bool(transition_presentation_);
  }
  void capture(const std::filesystem::path& path) {
    if(path.empty())throw FormatError("loading capture path is empty");
    std::lock_guard lock(mutex_);rethrow();
    if(!snapshot_.active)throw FormatError("no active loading frame to capture");
    if(source_wait_active_ && source_wait_quiesced_)
      throw FormatError("Source loading capture must be queued before next-renderer admission");
    if(!capture_.empty())throw FormatError("a loading capture is already pending");
    capture_=path;condition_.notify_all();
  }
  void gain_service(std::function<void(std::uint32_t)> service) {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active)throw FormatError("loading gain service must be bound before loading");
    audio_.gain=std::move(service);
  }
  void audio_services(DesktopLoadingAudioServices services) {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active || worker_.joinable() || audio_active_)
      throw FormatError("loading audio services must be bound before loading");
    if(!services.begin || !services.gain || !services.end)
      throw FormatError("loading audio requires begin, gain and end owners");
    audio_=std::move(services);
  }
  void transition_audio_services(DesktopNetworkTransitionAudioServices services) {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active || worker_.joinable() || transition_audio_active_)
      throw FormatError("Network transition audio must be bound before loading");
    if(!services.begin || !services.update || !services.end)
      throw FormatError("Network transition audio requires begin, update and end owners");
    transition_audio_=std::move(services);
  }
  void update_service(std::function<void()> service) {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active || worker_.joinable())
      throw FormatError("Loading update service must be bound before loading");
    update_=std::move(service);
  }

private:
  void start_completion_locked() {
    if(source_wait_active_)return;
    if(!snapshot_.active || completion_started_ || failure_)return;
    if(!current_ || pending_)throw FormatError("Vista loading fade requires its retained presentation owner");
    (void)vista_loading_fade_begin(fade_,snapshot_.clock_origin,SDL_GetTicks(),loading_fp);
    snapshot_.fade_active=true;completion_started_=true;condition_.notify_all();
  }
  void end_audio() {
    if(transition_audio_active_) {
      transition_audio_active_=false;transition_audio_.end(SDL_GetTicks());
    }
    if(!audio_active_)return;
    audio_active_=false;
    audio_.end();
  }
  void rethrow() const {if(failure_)std::rethrow_exception(failure_);}
  std::uint8_t present(std::uint8_t mode) {
    if(mode!=0)throw FormatError("unsupported original Vista loading presentation mode");
    std::unique_lock lock(mutex_);
    condition_.wait(lock,[this]{return cancelled_ || bool(current_);});
    if(cancelled_)throw LoadingCancelled{};
    ++snapshot_.present_calls;
    const auto now=SDL_GetTicks();
    // Literal source unsigned comparison, including its retained wrap behavior.
    if(now<snapshot_.deadline)return 1;
    snapshot_.deadline=now+33;
    if(update_)update_();
    const auto update=vista_loading_fade_update(fade_,snapshot_.clock_origin,now,loading_fp);
    snapshot_.gain_bits=fade_.gain_bits;snapshot_.fade_complete=update.complete;
    // The original passes every active gain to782052, whose DirectSound owner
    // is distinct from native linear master volume. An unbound service is an
    // explicit unresolved consumer, not an invented music-volume mapping.
    if(update.gain_service_bits && audio_.gain)audio_.gain(*update.gain_service_bits);
    int width{},height{};SDL_Vulkan_GetDrawableSize(window_,&width,&height);
    // A native zero-sized drawable suppresses GPU work, not the live source
    // clock/fade owner. This also lets a minimized handoff join safely.
    if(width<=0 || height<=0)return update.complete?0:1;
    const auto draws=vista_loading_geometry(update.elapsed_seconds_bits,unsigned(width),unsigned(height),
        sweep_progress_bits_,update.packed_argb,loading_fp);
    presentation_->update(draws,unsigned(width),unsigned(height));
    if(current_->draw_loading(current_base_,presentation_->materials(),presentation_->vertices(),capture_)) {
      ++snapshot_.submitted_frames;capture_.clear();
    }
    return update.complete?0:1;
  }
  void source_wait_loop() {
    for(;;) {
      std::unique_lock lock(mutex_);
      // The admitted surface may be quiescent during first-frame bootstrap.
      // Sound-class fades keep their independent wall clock in that interval.
      condition_.wait_for(lock,std::chrono::milliseconds(16),[this]{return cancelled_ || (current_ &&
          (!source_wait_quiesced_ || source_completion_requested_));});
      if(cancelled_)throw LoadingCancelled{};
      if(transition_audio_active_)transition_audio_.update(SDL_GetTicks());
      if(source_completion_requested_ && (source_ready_presented_ || source_surface_unavailable_))return;
      if(!current_ || source_wait_quiesced_)continue;
      int width{},height{};SDL_Vulkan_GetDrawableSize(window_,&width,&height);
      if(source_wait_progress_==0x3f800000u && (width<=0 || height<=0 ||
          (SDL_GetWindowFlags(window_)&SDL_WINDOW_MINIMIZED))) {
        source_wait_quiesced_=source_surface_unavailable_=true;
        condition_.notify_all();continue;
      }
      ++snapshot_.present_calls;
      bool submitted{};
      if(transition_active_) {
        // 0x258605 updates the status each frame; its pane intro and the
        // pulsing status text run from the screen's own open time.
        const auto elapsed=SDL_GetTicks()-transition_origin_;
        transition_vertices_=transition_presentation_->draw(transition_status_,float(elapsed)/1000.f,elapsed);
        submitted=!transition_vertices_.empty() && current_->draw_network_transition(current_transition_base_,
            unsigned(transition_presentation_->materials().size()),transition_vertices_,capture_);
      } else {
        source_wait_presentation_->update(source_wait_progress_,0);
        submitted=current_->draw_source_loading_wait(current_source_base_,source_wait_presentation_->vertices(),capture_);
      }
      if(submitted) {
        ++snapshot_.submitted_frames;capture_.clear();
        if(source_wait_progress_==0x3f800000u) {
          source_ready_presented_=source_wait_quiesced_=true;
          condition_.notify_all();
        }
      }
      lock.unlock();
      // Original12E200's wait yields then presents/vblanks. Native FIFO owns
      // actual frame pacing; this is cooperative scheduling, not a dwell.
      std::this_thread::yield();
    }
  }
  SDL_Window *window_{};
  std::optional<NativeLoadingPresentation> presentation_;
  std::optional<NativeSourceLoadingWait> source_wait_presentation_;
  std::shared_ptr<FrontendNetworkTransitionPainter> transition_presentation_;
  std::vector<Vertex> transition_vertices_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::thread worker_;
  std::shared_ptr<Renderer> current_,pending_,retired_;
  unsigned current_base_{},pending_base_{};
  unsigned current_source_base_{},pending_source_base_{};
  unsigned current_transition_base_{},pending_transition_base_{};
  DesktopLoadingSnapshot snapshot_;
  VistaLoadingFadeState fade_{0,0,0x3f800000};
  std::uint32_t sweep_progress_bits_{};
  bool cancelled_{};
  bool completion_started_{};
  std::exception_ptr failure_;
  std::filesystem::path capture_;
  DesktopLoadingAudioServices audio_;
  bool audio_active_{};
  std::function<void()> update_;
  DesktopNetworkTransitionAudioServices transition_audio_;
  bool transition_audio_active_{};
  bool source_wait_active_{},source_completion_requested_{},source_ready_presented_{};
  bool source_wait_quiesced_{},source_surface_unavailable_{};
  std::uint32_t source_wait_progress_{};
  bool transition_active_{};
  FrontendNetworkTransitionStatus transition_status_{};
  std::uint32_t transition_origin_{};
};

void DesktopSession::configure_loading(const std::filesystem::path& directory) {
  // Loading assets do not require a platform window until the first renderer.
  const auto assets=load_vista_loading_assets(directory);
  configure_loading(assets);
}
void DesktopSession::configure_loading(const VistaLoadingAssets& assets,bool preserve_logo_proportions) {
  if(loading_)throw FormatError("Loading assets are already configured");
  auto admitted=std::make_shared<DesktopLoadingBridge>(window_,assets,preserve_logo_proportions);
  loading_=std::move(admitted);
}
void DesktopSession::configure_source_loading_wait(const GameplayFontSource& source) {
  if(loading_)loading_->source_wait_assets(source);
  else loading_=std::make_shared<DesktopLoadingBridge>(window_,source);
}
void DesktopSession::configure_network_transition(std::shared_ptr<FrontendNetworkTransitionPainter> painter) {
  if(loading_)loading_->network_transition_assets(std::move(painter));
  else loading_=std::make_shared<DesktopLoadingBridge>(window_,std::move(painter));
}
bool DesktopSession::network_transition_configured() const {
  return loading_ && loading_->network_transition_configured();
}
void DesktopSession::begin_network_transition(FrontendNetworkTransitionStatus status) {
  if(!loading_)throw FormatError("Network transition dialog is not configured");
  loading_->begin(true,status);
}
void DesktopSession::begin_multiplayer_loading(FrontendNetworkTransitionStatus status) {
  if(loading_active())return;
  if(network_transition_configured())begin_network_transition(status);
  else if(source_loading_wait_configured())begin_source_loading_wait();
}
void DesktopSession::network_transition_status(FrontendNetworkTransitionStatus status) {
  if(loading_)loading_->network_transition_status(status);
}
std::shared_ptr<Renderer> DesktopSession::make_renderer(Mesh& mesh,bool validation,
    std::span<const SourceShadowPlanMaterial> shadows,const SourceBspResidentPlan *bsp,
      const std::filesystem::path &shader_directory,
      std::span<const SourceShadowRawPlanMaterial> raw_shadow_materials) {
  if(!window_)throw FormatError("renderer requires the desktop window");
  auto renderer=loading_?loading_->make_renderer(window_,mesh,validation,shadows,bsp,shader_directory,raw_shadow_materials):
      std::make_shared<Renderer>(window_,mesh,validation,shadows,bsp,std::function<void()>{},nullptr,shader_directory,raw_shadow_materials);
  renderer->share_display_settings(display_settings);
  return renderer;
}
void DesktopSession::renderer_ready(const std::shared_ptr<Renderer>& renderer) {
  if(loading_)loading_->ready(renderer);
}
void DesktopSession::begin_loading(){if(loading_ && loading_->vista_configured())loading_->begin();}
void DesktopSession::begin_source_loading_wait(){
  if(!loading_)throw FormatError("Source loading wait assets are not configured");
  loading_->begin(true);
}
void DesktopSession::finish_loading(){if(loading_)loading_->join_with_fade();}
void DesktopSession::cancel_loading() noexcept {if(loading_)loading_->cancel();}
bool DesktopSession::loading_active() const {return loading_ && loading_->snapshot().active;}
bool DesktopSession::vista_loading_configured() const {
  return loading_ && loading_->vista_configured();
}
bool DesktopSession::source_loading_wait_configured() const {
  return loading_ && loading_->source_wait_configured();
}
bool DesktopSession::source_loading_wait_active() const {
  return loading_ && loading_->source_wait_active();
}
std::optional<DesktopLoadingSnapshot> DesktopSession::loading_snapshot() const {
  return loading_?std::optional{loading_->snapshot()}:std::nullopt;
}
void DesktopSession::capture_loading(const std::filesystem::path& path) {
  if(!loading_)throw FormatError("Vista loading assets are not configured");
  loading_->capture(path);
}
void DesktopSession::set_loading_gain_service(std::function<void(std::uint32_t)> service) {
  if(!loading_)throw FormatError("Vista loading assets are not configured");
  loading_->gain_service(std::move(service));
}
void DesktopSession::set_loading_audio_services(DesktopLoadingAudioServices services) {
  if(!loading_)throw FormatError("Vista loading assets are not configured");
  loading_->audio_services(std::move(services));
}
void DesktopSession::set_network_transition_audio_services(DesktopNetworkTransitionAudioServices services) {
  if(!loading_)throw FormatError("Network transition assets are not configured");
  loading_->transition_audio_services(std::move(services));
}
void DesktopSession::set_loading_update_service(std::function<void()> service) {
  if(!loading_)throw FormatError("Loading assets are not configured");
  loading_->update_service(std::move(service));
}
} // namespace halo2
