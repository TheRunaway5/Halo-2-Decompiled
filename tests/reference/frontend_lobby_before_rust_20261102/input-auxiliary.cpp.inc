  if(frame().screen==14 && mode_==3) {
    // Authored host status 0x200001F0: X cancels a running start countdown.
    if(online_lobby_countdown_>=0) {
      if(!online_account_ || !account_.party || !online_account_->party_command({account_.party->leader?"countdown_cancel":"countdown_request"}))
        if(pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::Error);
      return;
    }
    open(216);return;
  }
