#pragma once
#include "halo2/multiplayer_network.hpp"
#include "halo2/system_link_state.hpp"
#include "halo2/vista_lobby_countdown.hpp"
#include "halo2/vista_session_countdown.hpp"
#include <chrono>

namespace halo2 {
class FrontendSession;
// Actual native raw-LAN lifetime, separate from a source gameplay world and
// from the local offline/ONLINE session adapters.
class SystemLinkSession final:public std::enable_shared_from_this<SystemLinkSession> {
public:
  explicit SystemLinkSession(bool host);
  ~SystemLinkSession();
  bool host() const noexcept{return host_;}
  const std::string& join_address()const noexcept{return address_;}
  const SystemLinkState* state() const;
  std::unique_ptr<LanHost> enter_host(MultiplayerMatch&,unsigned,std::uint64_t,const UiVariant&,std::string);
  std::unique_ptr<LanClient> enter_client(const std::string&,std::uint64_t,unsigned,
      std::span<const std::int8_t>,std::span<const SourcePlayerConfiguration>);
  void retain(std::unique_ptr<LanHost>,std::unique_ptr<LanClient>);
  bool world_update(const MultiplayerState&);
  void pump();
  void close();
  bool closed()const noexcept{return closed_;}
  void done();
  std::shared_ptr<const VistaPcrScreenOwner> report(FrontendSession&);
  void update_frontend(MenuState&,std::uint32_t,const std::function<void(FrontendUiSound)>&);
private:
  bool host_{},closed_{};
  std::string address_;
  std::unique_ptr<LanHost> retained_host_;
  std::unique_ptr<LanClient> retained_client_;
  LanHost* live_host_{};
  LanClient* live_client_{};
  VistaSessionCountdownTaskState countdown_timer_;
  VistaLobbyCountdownState countdown_sound_;
  std::optional<FrontendLaunch> pending_start_;
};
}
