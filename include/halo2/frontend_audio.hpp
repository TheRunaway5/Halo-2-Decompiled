#pragma once
#include "halo2/audio.hpp"
#include "halo2/frontend_timing.hpp"
#include "halo2/menu_music.hpp"

namespace halo2 {
class DesktopSession;
// Native resource lifetime, not a substitute Vista sound driver. Predecoded
// music and its transition state survive frontend/world renderer changes.
// No Cache, Mesh, renderer, or invocation callback is retained here.
struct FrontendAudioState {
  // Destroy the device before the last platform owner can call SDL_Quit.
  std::shared_ptr<DesktopSession> desktop;
  std::unique_ptr<AudioEngine> engine;
  FrontendMusicTransitions transitions;
  bool enabled = true;
  std::vector<AudioEngine::VoiceId> music_voices;
  std::unique_ptr<MenuMusic> music;
  // Actual PC638C60 sets desired=0 outside scenario kind2. Continue servicing
  // the source debounce on real world presentation updates after a handoff;
  // leaving the frontend invocation is not itself a forced stop.
  void world_update(std::uint32_t ui_clock) {
    enabled=false;
    if(!engine || !transitions.request(false,ui_clock))return;
    if(music && !music_voices.empty())music->stop(*engine,music_voices);
    else for(const auto voice:music_voices)engine->stop(voice);
    music_voices.clear();
  }
};
} // namespace halo2
