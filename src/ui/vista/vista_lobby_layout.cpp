#include "halo2/vista_lobby_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby layout service");
  return f(a...);
}
VistaLobbyLayoutWidgetState& widget(VistaLobbyLayoutState& state,VistaLobbyLayoutWidget id) {
  const auto i=unsigned(id);
  if(i>=state.widgets.size())throw FormatError("Vista lobby layout requires a constructed button");
  return state.widgets[i];
}
std::optional<VistaLobbyLayoutWidget> focused(const VistaLobbyLayoutState& state,
    const VistaLobbyLayoutServices& services) {
  //6435B5 iterates these source objects, including hidden/disabled buttons.
  for(unsigned i=0;i<4;++i) {
    const auto id=VistaLobbyLayoutWidget(i);
    if(call(services.focused,id))return id;
  }
  if(state.chat_enabled && call(services.focused,VistaLobbyLayoutWidget::chat))
    return VistaLobbyLayoutWidget::chat;
  return std::nullopt;
}
}
VistaLobbyLayoutRegistration vista_lobby_layout_registration(std::uint8_t factory,std::uint8_t chat) {
  VistaLobbyLayoutRegistration out;
  out.buttons[out.count++]=VistaLobbyLayoutWidget::start;
  out.buttons[out.count++]=VistaLobbyLayoutWidget::setup;
  if(factory) {
    out.buttons[out.count++]=VistaLobbyLayoutWidget::find_games;
    out.buttons[out.count++]=VistaLobbyLayoutWidget::players;
  }
  if(chat)out.buttons[out.count++]=VistaLobbyLayoutWidget::chat;
  return out;
}
std::optional<std::u16string_view> vista_lobby_layout_players_caption(std::uint8_t factory,unsigned language) {
  if(!factory)return std::nullopt;
  constexpr std::array<std::u16string_view,9> strings{
      u"PLAYERS",u"プレイヤー",u"SPIELER",u"JOUEURS",u"JUGADORES",
      u"GIOCATORI",u"플레이어",u"玩家",u"PLAYERS"};
  if(language>=strings.size())throw FormatError("Vista lobby caption language outside original table");
  return strings[language];
}
void vista_lobby_layout_visibility(VistaLobbyLayoutState& state,const VistaLobbyLayoutServices& s) {
  if(state.flags&2)return;
  const bool host=call(s.host)!=0;
  bool allowed=false;
  if(std::none_of(state.team_edit.begin(),state.team_edit.end(),[](auto v){return v!=0;}))
    allowed=!call(s.countdown_active);
  VistaLobbyLayoutText* prior{};
  if(state.pane==0 || state.pane==2)prior=call(s.text,std::int16_t{45});
  else if(state.pane==1)prior=call(s.text,std::int16_t{40});
  if(prior && call(s.factory_flag)) {
    prior->bounds[0]=400;prior->bounds[2]=320;
  }
  auto& players=widget(state,VistaLobbyLayoutWidget::players);
  if(call(s.factory_flag)) {
    players.visible=1;
    players.enabled=call(s.player_count)>1;
    players.bounds=call(s.host)?state.host_bounds:state.normal_bounds;
  } else {players.visible=0;players.enabled=0;}
  state.widgets[0].visible=host;state.widgets[1].visible=host;
  state.widgets[2].visible=host && call(s.factory_flag) && call(s.host);
  for(unsigned i=0;i<3;++i)state.widgets[i].enabled=host;
  if(prior)prior->visible=!host;
  const auto current=focused(state,s);
  if(current && !widget(state,*current).enabled) {
    call(s.focus_change,*current,std::int32_t{},std::uint8_t{});
    call(s.focus_change,host?VistaLobbyLayoutWidget::start:VistaLobbyLayoutWidget::screen,
        std::int32_t{},std::uint8_t{1});
  }
  if(allowed!=state.buttons_allowed) {
    if(call(s.host)) {
      if(allowed) {
        if(!state.remembered_focus)throw FormatError("Vista lobby host has no source remembered focus");
        call(s.activate_focus,*state.remembered_focus);
      }
    } else {
      bool check_chat=true;
      if(allowed) {
        if(state.remembered_focus) {
          call(s.activate_focus,*state.remembered_focus);check_chat=false;
        }
      } else {
        state.remembered_focus=state.chat_enabled && call(s.focused,VistaLobbyLayoutWidget::chat)
            ?std::optional{VistaLobbyLayoutWidget::chat}:std::nullopt;
      }
      if(check_chat && !call(s.focused,VistaLobbyLayoutWidget::chat))
        call(s.focus_change,VistaLobbyLayoutWidget::screen,std::int32_t{},std::uint8_t{1});
    }
    state.buttons_allowed=allowed;
  }
}
void vista_lobby_layout_pane(VistaLobbyLayoutState& state,const VistaLobbyLayoutServices& s) {
  const auto e=call(s.session_enum);
  const auto pane=e==0 || e==2 || e==4?std::int16_t{1}:
      e==1 || e==3 || e==5?std::int16_t{}:e==6?std::int16_t{2}:state.pane;
  if(pane!=state.pane) {call(s.select_pane,pane);state.pane=pane;}
  if(auto* image=call(s.bitmap,std::int16_t{41}))image->visible=call(s.favorite);
}
} // namespace halo2
