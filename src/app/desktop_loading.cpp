#include "halo2/desktop_session.hpp"
#include "halo2/native_loading_presentation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/vista_loading_thread.hpp"
#include <SDL_vulkan.h>
#include <chrono>
#include <condition_variable>
#include <exception>
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
  DesktopLoadingBridge(SDL_Window *window,const VistaLoadingAssets& assets)
      :window_(window),presentation_(assets) {}
  ~DesktopLoadingBridge(){cancel();}

  std::shared_ptr<Renderer> make_renderer(SDL_Window *window,Mesh& mesh,bool validation,
      std::span<const SourceShadowPlanMaterial> shadows,const SourceBspResidentPlan *bsp,
      const std::filesystem::path &shader_directory,
      std::span<const SourceShadowRawPlanMaterial> raw_shadow_materials) {
    unsigned base{};
    std::shared_ptr<Renderer> previous;
    {
      std::lock_guard lock(mutex_);rethrow();
      if(!window || (window_ && window_!=window))throw FormatError("loading bridge window changed");
      window_=window;
      if(pending_)throw FormatError("a loading renderer handoff is already pending");
      previous=current_;
      base=unsigned(mesh.materials.size());
      const auto bank=presentation_.materials();
      mesh.materials.insert(mesh.materials.end(),bank.begin(),bank.end());
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
      pending_=renderer;pending_base_=base;
    }
    return renderer;
  }

  void ready(const std::shared_ptr<Renderer>& renderer) {
    bool finish{};
    {
      std::lock_guard lock(mutex_);rethrow();
      if(!renderer || renderer!=pending_)
        throw FormatError("loading handoff does not name its prepared renderer");
      finish=snapshot_.active;
      if(finish)renderer->prepare_loading();
      current_=std::move(pending_);current_base_=pending_base_;
      // The old swapchain was explicitly released before the new one existed.
      // Its remaining device resources are now safe to destroy on this thread.
      retired_.reset();condition_.notify_all();
    }
    if(finish)join_with_fade();
  }

  void begin() {
    std::lock_guard lock(mutex_);rethrow();
    if(snapshot_.active || worker_.joinable())
      throw FormatError("Vista loading thread is already active");
    if(!current_ || pending_)
      throw FormatError("Vista loading requires a ready presentation owner");
    current_->prepare_loading();
    // Original66B9B1/66B9BB initializes deadline from one clock query;
    //66BA84/66BA96 initializes origin from a later clock query. E4AFE8's sweep
    // input starts at its known original PE .data raw value zero. Its
    // indirect producer remains unresolved, so no map percentage is invented.
    fade_={0,0,0x3f800000};
    const auto deadline=SDL_GetTicks();
    snapshot_={};snapshot_.active=true;snapshot_.clock_origin=SDL_GetTicks();
    snapshot_.deadline=deadline;snapshot_.gain_bits=fade_.gain_bits;
    cancelled_=false;
    completion_started_=false;
    try {
      //66BAAD precedes the original graphics-thread creation66BAB2..66BAC8.
      // Mark ownership first so a partially failed native admission is closed.
      if(audio_.begin){audio_active_=true;audio_.begin();}
      worker_=std::thread([this] {
        try {
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
        start_completion_locked();
      }
      condition_.notify_all();
    }
    // The original joins its graphics thread after fade begin. There is no
    // estimated map duration, fixed loading dwell, or synthetic completion.
    if(worker_.joinable())worker_.join();
    std::lock_guard lock(mutex_);snapshot_.active=false;
    //66A0AB shuts down the independent ambience after the graphics join,
    // including a native presentation failure. Preserve the first failure.
    try{end_audio();}catch(...){if(!failure_)failure_=std::current_exception();}
    rethrow();
  }

  void cancel() noexcept {
    {
      std::lock_guard lock(mutex_);cancelled_=true;condition_.notify_all();
    }
    if(worker_.joinable())worker_.join();
    std::lock_guard lock(mutex_);snapshot_.active=false;
    try{end_audio();}catch(...){if(!failure_)failure_=std::current_exception();}
  }
  DesktopLoadingSnapshot snapshot() const {
    std::lock_guard lock(mutex_);return snapshot_;
  }
  void capture(const std::filesystem::path& path) {
    if(path.empty())throw FormatError("loading capture path is empty");
    std::lock_guard lock(mutex_);rethrow();
    if(!snapshot_.active)throw FormatError("no active loading frame to capture");
    if(!capture_.empty())throw FormatError("a loading capture is already pending");
    capture_=path;
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

private:
  void start_completion_locked() {
    if(!snapshot_.active || completion_started_ || failure_)return;
    if(!current_ || pending_)throw FormatError("Vista loading fade requires its retained presentation owner");
    (void)vista_loading_fade_begin(fade_,snapshot_.clock_origin,SDL_GetTicks(),loading_fp);
    snapshot_.fade_active=true;completion_started_=true;condition_.notify_all();
  }
  void end_audio() {
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
    presentation_.update(draws,unsigned(width),unsigned(height));
    if(current_->draw_loading(current_base_,presentation_.materials(),presentation_.vertices(),capture_)) {
      ++snapshot_.submitted_frames;capture_.clear();
    }
    return update.complete?0:1;
  }
  SDL_Window *window_{};
  NativeLoadingPresentation presentation_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::thread worker_;
  std::shared_ptr<Renderer> current_,pending_,retired_;
  unsigned current_base_{},pending_base_{};
  DesktopLoadingSnapshot snapshot_;
  VistaLoadingFadeState fade_{0,0,0x3f800000};
  std::uint32_t sweep_progress_bits_{};
  bool cancelled_{};
  bool completion_started_{};
  std::exception_ptr failure_;
  std::filesystem::path capture_;
  DesktopLoadingAudioServices audio_;
  bool audio_active_{};
};

void DesktopSession::configure_loading(const std::filesystem::path& directory) {
  // Loading assets do not require a platform window until the first renderer.
  const auto assets=load_vista_loading_assets(directory);
  configure_loading(assets);
}
void DesktopSession::configure_loading(const VistaLoadingAssets& assets) {
  if(loading_)throw FormatError("Loading assets are already configured");
  auto admitted=std::make_shared<DesktopLoadingBridge>(window_,assets);
  loading_=std::move(admitted);
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
void DesktopSession::begin_loading(){if(loading_)loading_->begin();}
void DesktopSession::finish_loading(){if(loading_)loading_->join_with_fade();}
void DesktopSession::cancel_loading() noexcept {if(loading_)loading_->cancel();}
bool DesktopSession::loading_active() const {return loading_ && loading_->snapshot().active;}
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
} // namespace halo2
