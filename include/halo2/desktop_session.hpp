#pragma once
#include "halo2/display_settings.hpp"
#include "halo2/pc_settings.hpp"
#include "halo2/controller_settings.hpp"
#include <SDL.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>

namespace halo2 {
class Renderer;
class SourceBspResidentPlan;
struct Mesh;
struct SourceShadowPlanMaterial;
class DesktopLoadingBridge;
struct DesktopLoadingSnapshot {
  bool active{},fade_active{},fade_complete{};
  std::uint32_t clock_origin{},deadline{},gain_bits{},sleeps{},present_calls{},submitted_frames{};
};
struct DesktopLoadingAudioServices {
  // Normal initializer66BAAD starts782297 before the graphics thread;
  // shutdown66A0AB reaches782173 after that graphics thread has joined.
  std::function<void()> begin;
  std::function<void(std::uint32_t)> gain;
  std::function<void()> end;
};
// The application owns the SDL window across menu/game renderer lifetimes.
// Construction is lazy so inspection and dedicated-server paths need no video.
class DesktopSession {
public:
  DesktopSession() = default;
  ~DesktopSession();
  DesktopSession(const DesktopSession &) = delete;
  DesktopSession &operator=(const DesktopSession &) = delete;
  SDL_Window *window(unsigned width, unsigned height, bool hidden);
  // UI/game-thread owner, retained across BSP and renderer handoffs. Native
  // time()/SDL ticks supply146240/1462B0's OS inputs; the native CRT stream
  // starts at1. This does not import an Xbox startup history or captured seed.
  std::uint32_t &auxiliary_random();
  std::uint32_t fresh_secondary_random();
  std::string apply_video(unsigned width, unsigned height, bool fullscreen);
  // Configure genuine exported Vista loading images. Only explicit offline
  // session resource requests activate the source normal-mode presentation.
  void configure_loading(const std::filesystem::path&);
  std::shared_ptr<Renderer> make_renderer(Mesh&,bool validation,
      std::span<const SourceShadowPlanMaterial> shadow_materials={},
      const SourceBspResidentPlan *bsp_resident_plan=nullptr,
      const std::filesystem::path &shader_directory={});
  // Called after actual GPU/audio initialization, before an ordinary frame.
  void renderer_ready(const std::shared_ptr<Renderer>&);
  void begin_loading();
  void finish_loading();
  void cancel_loading() noexcept;
  bool loading_active() const;
  std::optional<DesktopLoadingSnapshot> loading_snapshot() const;
  void capture_loading(const std::filesystem::path&);
  // Original782052 argument intent. Its consumer is not assumed to be a
  // linear master-volume setter. The optional service runs on the loader.
  void set_loading_gain_service(std::function<void(std::uint32_t)>);
  // Retained independent ambience device/worker, never borrowed menu music.
  // All three services must be bound before a loading interval starts.
  void set_loading_audio_services(DesktopLoadingAudioServices);
  DisplaySettings display_settings;
  PcKeyboardSettings keyboard_settings;
  ControllerSettings controller_settings;

private:
  SDL_Window *window_{};
  bool initialized_{};
  std::uint32_t crt_random_state_=1;
  std::optional<std::uint32_t> auxiliary_random_;
  std::shared_ptr<DesktopLoadingBridge> loading_;
};
} // namespace halo2
