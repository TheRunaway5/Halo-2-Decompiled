  if(f.screen==14 && (online_lobby_countdown_>=0 || entry.destination==-17)) {
    // 0x25106C ->0x19A78E(controller,10,3).
    if(const auto error=online_lobby_start_error();!error.empty()) {
      pending_ui_sounds_.push_back(FrontendUiSound::Error);unavailable(error);return true;
    }
    const auto playlist=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),
        [&](const auto&p){return p.id==selected_playlist_;});
    if(!online_account_->party_command({"countdown_start",{},playlist->id,playlist->revision})) {
      pending_ui_sounds_.push_back(FrontendUiSound::Error);
      unavailable("The ONLINE operation changed before Start Game was accepted. Wait for it to finish, then try again.");
    } else update_online_account();
    return true;
  }
