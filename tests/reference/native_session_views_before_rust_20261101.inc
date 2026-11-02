std::optional<std::uint16_t> OnlineMatchSession::postgame_presence() const {
  std::lock_guard lock(impl_->mutex);
  return !impl_->postgame_presence_closed&&impl_->postgame_presence_observed&&(impl_->phase==Phase::Reviewing||impl_->phase==Phase::Postgame)?
      std::optional(impl_->voice_members):std::nullopt;
}
std::shared_ptr<PeerSnapshotCandidate> OnlineMatchSession::snapshot_candidates()const{
  std::lock_guard lock(impl_->mutex);
  return impl_->common_phase()&&impl_->snapshot_supported()?impl_->snapshot_candidate:nullptr;
}
std::vector<unsigned> OnlineMatchSession::local_slots() const {
  std::vector<unsigned> slots;
  for(const auto &player:impl_->roster)
    if(online_admission_peer(player)==impl_->self)slots.push_back(player.slot);
  return slots;
}
std::optional<OnlineContinuityState> OnlineMatchSession::continuity() const {
  std::lock_guard lock(impl_->mutex);if(!impl_->epoch_packets())return std::nullopt;return impl_->continuity_state;
}
