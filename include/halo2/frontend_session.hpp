#pragma once
#include "halo2/frontend.hpp"
#include <chrono>
#include <memory>
#include <future>

namespace halo2 {
struct FrontendAudioState;
class NativeVistaWidgetManagers;
class NativeVistaWidgetCache;
class NativeVistaWidgetMemory;
class MultiplayerMapAssets;
struct MultiplayerOptions;
// Native application lifetime only. This owns the actual frontend content and
// MenuState across renderer/world teardown; it is not a Vista session, return
// request, PCR builder, or permission to resume an old screen. A frontend
// invocation populates this owner once. Later invocations require an explicit
// new screen factory; they retain configuration, never old navigation history.
class FrontendSession {
public:
  FrontendSession();
  ~FrontendSession();
  FrontendSession(const FrontendSession &) = delete;
  FrontendSession &operator=(const FrontendSession &) = delete;
  FrontendSession(FrontendSession &&) = delete;
  FrontendSession &operator=(FrontendSession &&) = delete;
  // The current session survives world/renderer teardown. Steam refresh
  // credentials are scoped to a local profile in the OS credential store.
  std::shared_ptr<DiscordLogin> discord_login;
  std::shared_ptr<DiscordSocial> discord_social;
  // Account, party and queue ownership spans renderer/world invocations.
  // Runtime adapters borrow this container; explicit account/profile signout
  // or destruction of the retained session ends the corresponding workers.
  std::shared_ptr<NativeTu15OnlineServices> online_services;

  bool has_content() const noexcept;
  // Complete content setup before creating the state which references it.
  FrontendContent &retain_content(FrontendContent);
  const FrontendContent &content() const;
  FrontendContent &content();
  // Actual retained source32 UI manager banks. Screen allocation/class
  // dispatch and live Vista factory admission are separate required owners.
  NativeVistaWidgetManagers &vista_widget_managers();
  const NativeVistaWidgetManagers &vista_widget_managers() const;
  // Bind an explicit source32 mapping once. This retains writable loaded UI
  // bytes across world handoffs; it does not publish cache globals/readiness.
  NativeVistaWidgetCache &retain_vista_widget_cache(std::uint32_t source_base);
  NativeVistaWidgetCache &vista_widget_cache();
  const NativeVistaWidgetCache &vista_widget_cache() const;
  // Registry views borrow the actual retained owners; there are no copied
  // manager/cache shadows. Other regions require explicit bytes and lifetime.
  NativeVistaWidgetMemory &vista_widget_memory();
  const NativeVistaWidgetMemory &vista_widget_memory() const;
  MenuState &create_state(const std::filesystem::path &preferences = {});
  bool has_state() const noexcept;
  MenuState &state();
  const MenuState &state() const;
  // Actual60CA7D retains its truncated millisecond accumulator at D71900.
  // Native observations use real steady elapsed, including renderer/world
  // gaps, with truncation and uint32 wrap; entering a renderer never resets it.
  std::uint32_t ui_clock();
  // Lazy so CPU-only frontend/session fixtures open no audio device.
  FrontendAudioState &audio_state();
  void world_audio_update();
  std::shared_future<std::shared_ptr<MultiplayerMapAssets>> prepare_multiplayer_assets(const MultiplayerOptions&);
  // Source factory admission is supplied by the lifecycle/menu-return owner.
  // This native resource owner does not infer a factory from match completion.
  MenuState &enter(FrontendOptions::Entry,
      std::shared_ptr<const VistaPcrScreenOwner> = {});

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::chrono::steady_clock::time_point ui_clock_previous_;
  // Before the source UI bank is constructed only. D71900 then owns the value.
  std::uint32_t ui_clock_ms_{};
  std::unique_ptr<FrontendAudioState> audio_;
};

// Runtime adapters borrow a particular frontend invocation's resources. Scope
// exit detaches them before those resources die, including exceptional exits.
class FrontendRuntimeAdapterScope {
public:
  explicit FrontendRuntimeAdapterScope(MenuState &state) noexcept : state_(state) {}
  ~FrontendRuntimeAdapterScope() { state_.clear_runtime_adapters(); }
  FrontendRuntimeAdapterScope(const FrontendRuntimeAdapterScope &) = delete;
  FrontendRuntimeAdapterScope &operator=(const FrontendRuntimeAdapterScope &) = delete;
private:
  MenuState &state_;
};
} // namespace halo2
