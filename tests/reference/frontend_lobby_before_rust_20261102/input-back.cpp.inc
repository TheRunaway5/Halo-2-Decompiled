    if(frame().screen==14 && mode_==3 && account_.party && account_.party->leader && online_lobby_countdown_>=0) {
      // 250B1E param1/0xD: an active leader's B runs25122F/19A7E9 and
      // stays in the lobby. The later idle B may ask with0x8B.
      if(!online_account_ || !online_account_->party_command({"countdown_cancel"}))
        if(pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::Error);
      return;
    }
    if(frame().screen==14 && mode_==3 && account_.party && !account_.party->leader && online_lobby_countdown_>=0)
      // 2511B6's member route runs25122F before its0x8B: the real mode1
      // request can be observed even when the later confirmation is canceled.
      if(online_account_)online_account_->party_command({"countdown_request"});
