#pragma once
#include "halo2/frontend_session.hpp"
#include "halo2/native_vista_session.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/vista_session_start_transition.hpp"
#include "halo2/vista_menu_return_observer.hpp"
#include "halo2/vista_pcr_publication.hpp"
#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/vista_session_readiness.hpp"
#include "halo2/vista_frontend_readiness.hpp"
#include "halo2/vista_lobby_countdown.hpp"
#include "halo2/multiplayer_world.hpp"

namespace halo2 {
// Native resource/task adapter around the recovered PC lifecycle. This first
// live binding admits genuinely signed-in local offline FFA Slayer players.
// It preserves source phase writes and request ordering; the protected Windows
// loader is an explicit native Cache/task boundary, not a recovered loader.
class NativeVistaOfflineFlow final {
public:
  // The optional presentation service starts immediately before the real
  // synchronous native resource load. CPU-only callers leave it unbound.
  NativeVistaOfflineFlow(FrontendSession&,std::filesystem::path maps,const MultiplayerOptions&,
      std::function<void()> resource_load_begin = {});
  ~NativeVistaOfflineFlow();
  static bool supports(const FrontendLaunch&);
  void begin(const FrontendLaunch&,const UiLobbyMap&);
  void pump();
  bool launch_ready() const noexcept;
  bool finished() const noexcept;
  FrontendLaunch take_launch();
  void world_loaded(const MultiplayerMatch&,std::span<const unsigned>);
  std::optional<std::uint32_t> world_view_black(unsigned actual_view);
  void world_audio_update(float actual_master_volume,
      const std::array<float,sound_class_count>& actual_class_gains);
  bool world_update(const MultiplayerMatch&);
  void world_unloaded();
  std::shared_ptr<MultiplayerPreparedWorld> prepared_world() const;
  // Called after actual world teardown and the new mainmenu resource load.
  // The report consumes the retained native statistics and genuine PC XUID.
  void mainmenu_loaded();
  std::shared_ptr<const VistaPcrScreenOwner> report();
  void update_frontend(MenuState&,std::uint32_t ui_clock,
      const std::function<void(FrontendUiSound)>&);
  std::int32_t countdown() const;
  unsigned lifecycle_kind() const noexcept{return manager_.current_kind;}
  bool has_report() const noexcept{return bool(report_results_);}
  unsigned player_activation_count() const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  FrontendSession& frontend_;
  std::filesystem::path maps_;
  NativeVistaSession session_;
  std::array<std::byte,0xc0> manager_payload_{};
  VistaLifecycleManager manager_;
  std::array<VistaLifecycleHandler,7> handlers_;
  VistaSessionStartTransitionState start_;
  VistaSessionTransitionState in_game_{};
  VistaSessionStartTransitionServices services_;
  VistaMenuReturnState menu_return_;
  VistaSessionMapReadinessState map_readiness_{};
  std::optional<FrontendLaunch> launch_;
  VistaPcrPcVariant variant_;
  std::optional<VistaPcrConsumedSnapshot> report_source_;
  std::shared_ptr<const VistaPcrResults> report_results_;
  std::uint32_t selected_row_{},ui_clock_{};
  std::optional<unsigned> native_slot_;
  std::vector<unsigned> native_slots_;
  bool begun_{},world_ready_{},world_mainmenu_=true,load_requested_{},menu_requested_{};
  void network_update();
  VistaFrontendReadinessQueries readiness() const;
  void observe_menu(MenuState*);
  void publish(const MultiplayerMatch&);
  void engine_presentation_tick(const MultiplayerMatch&);
  void player_activation_tick(MultiplayerMatch&);
  bool lobby_action(FrontendLobbySessionAction);
  void configure_lobby(const FrontendLaunch&,const UiLobbyMap&);
};
} // namespace halo2
