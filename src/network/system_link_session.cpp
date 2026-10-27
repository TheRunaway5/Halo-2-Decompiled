#include "halo2/system_link_session.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_vista_pcr.hpp"
#include "halo2/vista_pcr_owner_state.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <cstring>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
SystemLinkSession::SystemLinkSession(bool host):host_(host){}
SystemLinkSession::~SystemLinkSession()=default;
const SystemLinkState* SystemLinkSession::state() const {
  if(live_host_)return &live_host_->system_link_state();
  if(live_client_)return live_client_->system_link_state();
  return nullptr;
}
std::unique_ptr<LanHost> SystemLinkSession::enter_host(MultiplayerMatch& match,unsigned port,
    std::uint64_t fingerprint,const UiVariant& variant,std::string map) {
  if(!host_ || closed_ || (live_host_ && !retained_host_))throw FormatError("System Link host already owns a world");
  if(retained_host_)retained_host_->resume_system_link_world(match,fingerprint,variant,std::move(map));
  auto host=std::move(retained_host_);
  if(!host){host=std::make_unique<LanHost>(match,port,fingerprint,variant);host->enable_system_link_return(std::move(map));}
  live_host_=host.get();return host;
}
std::unique_ptr<LanClient> SystemLinkSession::enter_client(const std::string& address,
    std::uint64_t fingerprint,unsigned players,std::span<const std::int8_t> teams,
    std::span<const SourcePlayerConfiguration> profiles) {
  if(host_ || closed_ || (live_client_ && !retained_client_))throw FormatError("System Link client already owns a world");
  if(retained_client_) {
    const auto& client=retained_client_;
    const auto next=client->system_link_state();
    if(!next || next->phase!=SystemLinkPhase::starting || next->fingerprint!=fingerprint || client->slots().size()!=players ||
        profiles.size()!=players || teams.size()!=players)
      throw FormatError("System Link client preparation differs from the authenticated next world");
    for(unsigned i=0;i<players;++i) {
      const auto slot=client->slots()[i];const auto admitted=std::ranges::find(next->roster,slot,&OnlineAdmission::slot);
      if(next->profiles[slot]!=profiles[i] || admitted==next->roster.end() || admitted->team!=teams[i])
        throw FormatError("System Link client preparation changed its actual selected profile/team");
    }
    client->resume_system_link_world();
  }
  auto client=std::move(retained_client_);
  if(!client){client=std::make_unique<LanClient>(address,fingerprint,players,teams,profiles);address_=address;}
  live_client_=client.get();return client;
}
void SystemLinkSession::retain(std::unique_ptr<LanHost> host,std::unique_ptr<LanClient> client) {
  if(closed_ || bool(host)==bool(client) || (host && host.get()!=live_host_) || (client && client.get()!=live_client_))
    throw FormatError("System Link return lost its actual transport owner");
  const auto current=state();
  if(!current || (current->phase!=SystemLinkPhase::postgame && !(client && current->phase==SystemLinkPhase::lobby)))
    throw FormatError("System Link return requires actual lobby admission or confirmed native results");
  if(host)host->retire_system_link_world();
  retained_host_=std::move(host);retained_client_=std::move(client);
}
bool SystemLinkSession::world_update(const MultiplayerState& actual) {
  if(closed_)return false;
  const auto current=state();if(!current)return false;
  if(host_ && current->phase==SystemLinkPhase::running && actual.game_over_complete() && actual.results_finalized)
    live_host_->publish_system_link_results();
  else if(!host_ && current->phase==SystemLinkPhase::reviewing && actual.game_over_complete() && actual.results_finalized)
    live_client_->confirm_system_link_results();
  return state()->phase==SystemLinkPhase::postgame;
}
void SystemLinkSession::pump() {
  if(closed_)return;
  if(retained_host_)retained_host_->service_system_link_lobby();
  else if(retained_client_) {
    retained_client_->service_system_link_lobby();
    if(!retained_client_->error().empty())close();
  }
}
void SystemLinkSession::close() {
  // Destroying a client sends its ordinary authenticated leave. A host closes
  // its actual socket; no successor authority or offline membership is made.
  retained_client_.reset();retained_host_.reset();live_host_=nullptr;live_client_=nullptr;
  pending_start_.reset();closed_=true;
}
void SystemLinkSession::done() {
  if(!host_ || !retained_host_)throw FormatError("Only the retained System Link host can continue the session");
  retained_host_->continue_system_link_lobby();
}
std::shared_ptr<const VistaPcrScreenOwner> SystemLinkSession::report(FrontendSession& frontend) {
  const auto current=state();
  if(!current || current->phase!=SystemLinkPhase::postgame || current->report.size()!=vista_pcr_source_size)
    throw FormatError("System Link PCR requires confirmed retained original results");
  VistaLoadingFloatingPoint fp{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fnstcw %0":"=m"(fp.x87_control_word));fp.mxcsr=_mm_getcsr();
#else
  throw FormatError("System Link PCR requires the actual supported floating-point context");
#endif
  VistaPcrConsumedSnapshot source;std::copy(current->report.begin(),current->report.end(),source.source.begin());
  const auto weak=weak_from_this();
  auto pcr=std::make_shared<NativeVistaPcr>(std::move(source),current->roster,frontend.content(),fp,[weak]()->std::optional<std::uint16_t>{
    if(const auto active=weak.lock())if(const auto observed=active->state())return observed->active_players;
    return {};
  });
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->results=pcr->results();owner->rows=pcr->rows(16);
  owner->appearance=[pcr,&frontend](unsigned row){return pcr->appearance(row,16,frontend.state().frame().context);};
  const auto self=shared_from_this();
  owner->input.host=[self]()->std::uint8_t{return !self->closed_ && self->host_;};
  owner->input.details_available=[]()->std::uint8_t{return 0;};
  owner->input.continue_session=[self,&frontend]{self->done();frontend.state().set_session_countdown(std::nullopt);frontend.state().open_vista_lobby();};
  owner->input.dialog=[&frontend](const auto& request){frontend.state().vista_pcr_dialog(request);};
  owner->leave_session=[self]{self->close();};
  owner->select_row=[pcr](unsigned row){pcr->select_row(row);};
  owner->elapsed=[&frontend](std::uint32_t creation){return frontend.ui_clock()-creation;};
  owner->floating_point=fp;owner->controller=0;owner->creation_clock=frontend.ui_clock();
  const auto focus=vista_pcr_initial_focus(*owner->results);owner->pane=focus.pane;owner->selected_row=focus.selected_row;
  pcr->select_row(focus.selected_row);return owner;
}
void SystemLinkSession::update_frontend(MenuState& menu,std::uint32_t clock,
    const std::function<void(FrontendUiSound)>& sound) {
  pump();
  if(closed_){menu.leave_system_link_lobby();return;}
  const auto current=state();if(!current)return;
  std::vector<FrontendLobbyRosterPlayer> players;std::set<unsigned> machines;
  for(const auto& row:current->roster)if(current->active_players&(1u<<row.slot)) {
    FrontendLobbyRosterPlayer p;p.session_index=int(row.slot);p.name=row.name;p.team=row.team;
    p.appearance_available=current->profiles[row.slot].has_value();
    if(p.appearance_available)std::memcpy(&p.appearance,current->profiles[row.slot]->data()+0x40,sizeof(p.appearance));
    players.push_back(std::move(p));machines.insert(row.peer);
  }
  if(players.empty()){close();menu.leave_system_link_lobby();return;}
  menu.set_system_link_lobby(host_,std::move(players),unsigned(machines.size()));
  if(!host_)menu.adopt_system_link_configuration(current->map,current->variant);
  const auto self=shared_from_this();
  menu.set_system_link_leave([self,&menu]{self->close();menu.leave_system_link_lobby();});
  if(current->phase==SystemLinkPhase::lobby && menu.frame().vista_pcr)menu.open_vista_lobby();
  menu.set_session_countdown(current->countdown<0?std::nullopt:std::optional(current->countdown));
  menu.set_lobby_session_action([self,&menu](FrontendLobbySessionAction action) {
    if(action==FrontendLobbySessionAction::Back) {
      if(self->host_ && self->state()->countdown>=0) {
        self->retained_host_->system_link_countdown(-1);self->pending_start_.reset();menu.set_session_countdown(std::nullopt);
      } else {self->close();menu.leave_system_link_lobby();}
      return true;
    }
    if(!self->host_)return true;
    if(action==FrontendLobbySessionAction::Start) {
      if(self->state()->phase!=SystemLinkPhase::lobby)return true;
      const auto stage=self->state()->countdown;
      const auto selected=menu.selected_multiplayer_configuration();
      const auto type=source_game_variant_type(source_game_variant(selected.variant));
      if(type!=1 && type!=9){menu.system_link_configuration_error();return true;}
      const auto result=vista_session_countdown_start({0,10,3},{stage<0?0xa:0xb,0,true,stage,true});
      for(const auto& call:result.call_intents())if(call.service==VistaSessionCountdownService::write) {
        self->pending_start_=selected;
        self->retained_host_->system_link_countdown(std::bit_cast<std::int32_t>(call.arguments[1]));
      }
      return true;
    }
    if(self->state()->countdown>=0) {
      self->retained_host_->system_link_countdown(-1);self->pending_start_.reset();menu.set_session_countdown(std::nullopt);
    }
    return false;
  });
  if(host_ && current->phase==SystemLinkPhase::lobby) {
    const auto timer=vista_session_countdown_timer(countdown_timer_,{current->countdown,0,clock,clock,clock});
    for(const auto& call:timer.call_intents())if(call.service==VistaSessionCountdownService::write)
      retained_host_->system_link_countdown(std::bit_cast<std::int32_t>(call.arguments[1]));
  }
  const auto stage=state()->countdown;
  const auto audio=vista_lobby_countdown_consume(countdown_sound_,{stage>=0,stage,clock});
  for(const auto& call:audio.call_intents())if(call.service==VistaLobbyCountdownService::ui_sound && sound)
    sound(FrontendUiSound::PregameCountdown);
  menu.set_session_countdown(stage<0?std::nullopt:std::optional(stage));
  if(host_ && stage==0 && pending_start_) {menu.launch=std::move(pending_start_);pending_start_.reset();}
  if(!host_ && current->phase==SystemLinkPhase::starting && !menu.launch) {
    auto launch=menu.selected_multiplayer_configuration();launch.host_port=0;launch.map=current->map+".map";
    launch.variant=current->variant;menu.launch=std::move(launch);
  }
}
}
