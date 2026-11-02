void OnlineMatchSession::close_postgame() {
  {std::lock_guard lock(impl_->mutex);bridge::native_session_close_postgame(*impl_->controller);impl_->refresh();}
  if(const auto audio=retained_voice())audio->close();
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
}
