#include "halo2/frontend_matchmaking_lobby.hpp"
#include "halo2/frontend_lobby_setup.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <stdexcept>
namespace halo2 {
namespace bridge = online_protocol_bridge;
namespace {
FrontendLobbySetupRow setup_row(const bridge::FrontendSetupRow &row) {
  using Route = bridge::FrontendSetupRoute;
  using Action = FrontendLobbySetupAction;
  switch (row.route) {
  case Route::Open: return {row.semantic, row.label, row.destination, Action::Open};
  case Route::SwitchCoop: return {row.semantic, row.label, row.destination, Action::SwitchCoop};
  case Route::SwitchCustom: return {row.semantic, row.label, row.destination, Action::SwitchCustom};
  case Route::AlternateSession: return {row.semantic, row.label, row.destination, Action::AlternateSession};
  case Route::OpenPlaylist: return {row.semantic, row.label, row.destination, Action::OpenPlaylist};
  }
  throw std::logic_error("Invalid Rust Game Setup route");
}
}
std::uint32_t frontend_lobby_setup_label(int semantic) {
  return bridge::frontend_setup_label(semantic);
}
std::vector<FrontendLobbySetupRow> frontend_lobby_setup_rows(int session_enum) {
  const auto rows = bridge::frontend_setup_rows(session_enum);
  std::vector<FrontendLobbySetupRow> out;
  out.reserve(rows.size());
  for (const auto &row : rows) out.push_back(setup_row(row));
  return out;
}
int frontend_lobby_setup_focused_semantic(int session_enum, int row) {
  return bridge::frontend_setup_focus(session_enum, row);
}
FrontendLobbySetupSelection frontend_lobby_setup_accept(int session_enum, unsigned selected, bool online, bool leader) {
  const auto effect = bridge::frontend_setup_accept(session_enum, selected, online, leader);
  return {effect.selected ? std::optional(setup_row(effect.row)) : std::nullopt, std::string(effect.error)};
}
FrontendLobbySetupPrivacy frontend_lobby_setup_privacy(bool online, std::optional<int> session_enum,
    bool privacy_present, bool leader, bool busy) {
  const auto effect = bridge::frontend_setup_privacy(online, session_enum.has_value(), session_enum.value_or(0),
      privacy_present, leader, busy);
  return {effect.open, std::string(effect.error)};
}
struct FrontendMatchmakingLobby::Impl {
  rust::Box<bridge::FrontendLobby> state;
  Impl() : state(bridge::new_frontend_lobby()) {}
  explicit Impl(const bridge::FrontendLobby &other) : state(bridge::clone_frontend_lobby(other)) {}
};
FrontendMatchmakingLobby::FrontendMatchmakingLobby() : impl_(std::make_unique<Impl>()) {}
FrontendMatchmakingLobby::~FrontendMatchmakingLobby() = default;
FrontendMatchmakingLobby::FrontendMatchmakingLobby(const FrontendMatchmakingLobby &o)
    : impl_(std::make_unique<Impl>(*o.impl_->state)) {}
FrontendMatchmakingLobby &FrontendMatchmakingLobby::operator=(const FrontendMatchmakingLobby &o) {
  if (this != &o) { auto copy = std::make_unique<Impl>(*o.impl_->state); impl_.swap(copy); }
  return *this;
}
FrontendMatchmakingLobby::FrontendMatchmakingLobby(FrontendMatchmakingLobby &&) noexcept = default;
FrontendMatchmakingLobby &FrontendMatchmakingLobby::operator=(FrontendMatchmakingLobby &&) noexcept = default;
std::int32_t FrontendMatchmakingLobby::seconds() const { return bridge::frontend_lobby_seconds(*impl_->state); }
bool FrontendMatchmakingLobby::matchmaking() const { return bridge::frontend_lobby_matchmaking(*impl_->state); }
bool FrontendMatchmakingLobby::leave_pending() const { return bridge::frontend_lobby_leave_pending(*impl_->state); }
void FrontendMatchmakingLobby::cancel() { bridge::frontend_lobby_cancel(*impl_->state); }
void FrontendMatchmakingLobby::exit() { bridge::frontend_lobby_exit(*impl_->state); }
void FrontendMatchmakingLobby::enter(const bridge::FrontendLobbyFacts &f) { bridge::frontend_lobby_enter(*impl_->state,f); }
bridge::FrontendLobbyObservation FrontendMatchmakingLobby::observe(const bridge::FrontendLobbyFacts &f) {
  return bridge::frontend_lobby_observe(*impl_->state,f);
}
std::string FrontendMatchmakingLobby::start_error(const bridge::FrontendLobbyFacts &f) const {
  return std::string(bridge::frontend_lobby_start_error(*impl_->state,f));
}
bridge::FrontendLobbyInputEffects FrontendMatchmakingLobby::input(
    const bridge::FrontendLobbyFacts &f,unsigned event,int destination,const Submit &submit) {
  auto plan=bridge::frontend_lobby_plan(*impl_->state,f,std::uint32_t(event),destination);
  const auto command=bridge::frontend_lobby_plan_command(*plan);
  const auto playlist=std::string(bridge::frontend_lobby_plan_playlist(*plan));
  const auto revision=std::string(bridge::frontend_lobby_plan_revision(*plan));
  // The actual native command is invoked after the Rust planning borrow ends.
  // A native exception propagates unchanged; no completion is fabricated.
  const bool accepted=command==0 || submit(command,playlist,revision);
  return bridge::frontend_lobby_finish_input(*impl_->state,std::move(plan),accepted);
}
bool FrontendMatchmakingLobby::observe_leave(const bridge::FrontendLobbyFacts &f) {
  return bridge::frontend_lobby_observe_leave(*impl_->state,f);
}
void FrontendMatchmakingLobby::left() { bridge::frontend_lobby_left(*impl_->state); }

}
