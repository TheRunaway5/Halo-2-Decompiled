  void fence_audio(){
    const auto generation=audio?audio->fence():audio_generation;
    bridge::native_session_fence_audio(*controller,generation);refresh();
  }
  void sync_audio(){if(audio&&projection.sync_audio)(void)audio->resume(audio_generation);}
