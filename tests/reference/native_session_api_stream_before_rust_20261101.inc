void OnlineMatchSession::send(std::span<const std::byte> packet,unsigned peer){
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();bridge::native_session_send(*impl_->controller,peer,wire(packet));
}
bool OnlineMatchSession::receive(std::vector<std::byte> &packet,unsigned &peer){
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();auto received=bridge::native_session_receive(*impl_->controller);
  if(received.empty())return false;
  peer=received[0].peer;packet=native_bytes(received[0].bytes);return true;
}
void OnlineMatchSession::voice(unsigned target,const GameVoiceFrame &frame){
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();impl_->sync_audio();
  bridge::native_session_voice(*impl_->controller,target,wire_audio(frame),!impl_->audio||impl_->audio->ready(impl_->audio_generation));
}
void OnlineMatchSession::voice_preferences(unsigned target,bool enabled,std::uint16_t muted){
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();bridge::native_session_preferences(*impl_->controller,target,enabled,muted);
}
std::uint16_t OnlineMatchSession::voice_blocks() const {std::lock_guard lock(impl_->mutex);return impl_->voice_blocked;}
std::vector<MatchVoicePacket> OnlineMatchSession::voice_packets(){
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();std::vector<MatchVoicePacket> result;
  for(const auto &v:bridge::native_session_voices(*impl_->controller))result.push_back({v.peer,v.preferences,v.enabled,v.muted,native_audio(v.audio)});
  return result;
}
void OnlineMatchSession::retain_voice(std::shared_ptr<GameVoice> audio) {
  std::lock_guard lock(impl_->mutex);require(!impl_->audio || impl_->audio==audio);
  if(impl_->audio==audio)return;
  impl_->audio=std::move(audio);if(impl_->audio)bridge::native_session_bind_audio(*impl_->controller);impl_->fence_audio();impl_->sync_audio();
}
