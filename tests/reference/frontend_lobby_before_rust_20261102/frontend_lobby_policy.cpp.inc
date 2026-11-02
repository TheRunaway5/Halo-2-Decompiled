void MenuState::enter_online_matchmaking_lobby() {
  online_lobby_matchmaking_=true;
  online_lobby_countdown_party_.clear();
  cancel_online_lobby_countdown();
  if(account_.party && account_.party->countdown && account_.party->countdown->complete)
    online_lobby_countdown_consumed_=account_.party->countdown->serial;
  if(frame().screen==14 && mode_==3){stack_.back().entries.clear();populate_online_party(stack_.back());++revision;}
}

void MenuState::cancel_online_lobby_countdown() {
  online_lobby_countdown_=online_lobby_countdown_stage_=-1;
}

bool MenuState::update_online_lobby_countdown() {
  const bool lobby=mode_==3 && std::any_of(stack_.begin(),stack_.end(),[](const auto &f){return f.screen==14;});
  if(online_lobby_leave_pending_)return false;
  if(!lobby || !account_.party || !account_.party->countdown_supported) {
    online_lobby_countdown_party_.clear();
    if(online_lobby_countdown_>=0){cancel_online_lobby_countdown();++revision;}
    return false;
  }
  const auto &published=account_.party->countdown;
  if(published && published->seconds && *published->seconds>0 && online_lobby_countdown_party_.empty())
    for(const auto &m:account_.party->members)online_lobby_countdown_party_.push_back(m.handle);
  if(!published && !online_lobby_countdown_party_.empty()) {
    const auto counted=std::exchange(online_lobby_countdown_party_,{});
    const bool departed=std::any_of(counted.begin(),counted.end(),[&](const auto &handle){
      return std::none_of(account_.party->members.begin(),account_.party->members.end(),[&](const auto &m){return m.handle==handle;});
    });
    if(departed) {
      // Actual accepted member departure invalidated party A's parameter.
      // 250804 presents its departure states6/14/19 as the original0xAE.
      cancel_online_lobby_countdown();quickmatch_pending_=false;
      if(online_matchmaking_)online_matchmaking_->cancel_queue();
      reset_online_matchmaking_progress();
      while(stack_.size()>1 && frame().screen!=14)stack_.pop_back();
      dialog(0xae,-5,true);++revision;return true;
    }
  }
  const auto seconds=published && published->seconds &&
    (!published->complete || published->serial!=online_lobby_countdown_consumed_)?*published->seconds:-1;
  if(seconds!=online_lobby_countdown_){online_lobby_countdown_=seconds;++revision;}
  // 0x250A8B: UI sound10 for each new stage 0..3.
  if(matchmaking_lobby_countdown_beeps(online_lobby_countdown_stage_,online_lobby_countdown_) &&
     pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::PregameCountdown);
  online_lobby_countdown_stage_=online_lobby_countdown_;
  if(!published || !published->complete || published->serial==online_lobby_countdown_consumed_)return false;
  // 0x6E410: a zero countdown stops and a matchmaking party enters mode6,
  // which 0x19B1C9 presents as screen210 for the retained playlist.
  online_lobby_countdown_consumed_=published->serial;
  online_lobby_countdown_party_.clear();
  cancel_online_lobby_countdown();
  while(stack_.size()>1 && frame().screen!=14)stack_.pop_back();
  open(210);
  return true;
}

void MenuState::update_online_lobby_leave() {
  if(!online_lobby_leave_pending_ || account_.busy)return;
  if(account_.party) {
    // A refused mutation leaves the actual party and its confirmation intact.
    if(!account_.party_message.empty())online_lobby_leave_pending_=false;
    return;
  }
  // 25132E/251345 ->199E3C ->199E2E leaves both A and B; profile/login
  // remains signed in. Wait for the accepted-party reply before disposing UI.
  online_lobby_leave_pending_=false;
  if(online_matchmaking_)online_matchmaking_->sign_out();
  matchmaking_active_=quickmatch_pending_=false;matchmaking_={};
  selected_playlist_.clear();online_lobby_matchmaking_=online_lobby_playlist_pending_=false;
  online_lobby_countdown_party_.clear();cancel_online_lobby_countdown();
  reset_online_matchmaking_progress();launch.reset();
  while(stack_.size()>1 && frame().screen!=6)stack_.pop_back();
  ++revision;
}

std::string MenuState::online_lobby_start_error() const {
  if(!online_account_ || !account_.authenticated)return "Sign in to ONLINE before starting matchmaking.";
  if(!account_.party)return "Create or join a party before starting matchmaking.";
  if(!account_.party->leader)return "Only the party leader can start matchmaking.";
  if(!online_lobby_matchmaking_ || selected_playlist_.empty())return "Choose a matchmaking playlist in Game Setup first.";
  if(!online_matchmaking_)return "No matchmaking service is configured.";
  if(!account_.party->countdown_supported)
    return "This ONLINE service does not support the shared lobby countdown. Update the server before starting matchmaking.";
  if(account_.busy)return "Wait for the current ONLINE operation to finish, then start matchmaking.";
  const auto playlist=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),
      [&](const auto&p){return p.id==selected_playlist_;});
  if(playlist==matchmaking_.playlists.end() || !playlist->eligible)
    return "The selected playlist is unavailable. Choose a playlist in Game Setup.";
  if(!std::all_of(account_.party->members.begin(),account_.party->members.end(),
      [&](const auto&m){return m.ready_playlist==playlist->id;}))
    return "Every party member must select this playlist before matchmaking can start.";
  return {};
}
