    if(mode_==3 && std::any_of(stack_.begin(),stack_.end(),[](const auto &frame){return frame.screen==14;})) {
      // Original0x8B callbacks25132E/251345 leave the sessions, not merely
      // their widgets. A submitted command is acknowledged before UI return.
      if(online_lobby_leave_pending_)return;
      if(account_.party && (!online_account_ || !online_account_->party_command({"leave_lobby"}))) {
        if(pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::Error);
        return;
      }
      online_lobby_leave_pending_=true;
      if(online_matchmaking_)online_matchmaking_->cancel_queue();
      quickmatch_pending_=false;update_online_account();update_online_lobby_leave();return;
    }
