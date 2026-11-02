void OnlineMatchSession::pump_postgame_voice() {
  const auto audio=retained_voice();if(!audio)return;
  const auto phase_now=phase();
  if(phase_now==Phase::Failed){audio->enable(false);return;}
  if(phase_now!=Phase::Reviewing && phase_now!=Phase::Postgame)return;
  audio->focus(impl_->voice_focused);audio->radio(false,true);
  audio->blocks(voice_blocks());
  const auto selected=audio->snapshot();rust::Vec<bridge::NativeSessionAudio> outgoing;
  for(const auto &frame:audio->outgoing())outgoing.push_back(wire_audio(frame));
  rust::Vec<bridge::NativeSessionAudio> received;
  {std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();
    received=bridge::native_session_postgame_voice(*impl_->controller,transfer_time(),selected.enabled,selected.muted,selected.blocked,std::move(outgoing),audio->ready(impl_->audio_generation));}
  for(const auto &frame:received)audio->receive(native_audio(frame));
}
