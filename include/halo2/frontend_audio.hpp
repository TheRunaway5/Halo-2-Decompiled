#pragma once
#include "halo2/audio.hpp"
#include "halo2/frontend_timing.hpp"
#include "halo2/menu_music.hpp"
#include <mutex>
#include <optional>

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
  SoundClassFades transition_classes;
  std::optional<std::uint32_t> class_clock;
  std::mutex class_mutex;
private:
  void class_update_locked(std::uint32_t clock) {
    if(class_clock) {
      const auto elapsed=clock-*class_clock;
      // UI and loading threads can query the same monotonic clock before
      // either obtains this owner. An older query cannot rewind its fade or
      // turn a small scheduling delay into an unsigned49-day advance.
      if(elapsed<=0x7fffffffu)transition_classes.advance(float(elapsed)/1000.f);
      else clock=*class_clock;
    }
    class_clock=clock;
    if(engine)engine->class_gains(transition_classes.gains());
  }
public:
  void class_update(std::uint32_t clock) {
    const std::lock_guard lock(class_mutex);
    class_update_locked(clock);
  }
  // TU1.5 258565 -> 228390("", -64 dB, 1 second) on screen231 creation.
  // The independent loader services this clock while the UI thread loads.
  void transition_begin(std::uint32_t clock) {
    const std::lock_guard lock(class_mutex);
    class_update_locked(clock);transition_classes.set("",-64.f,1.f,true);
  }
  void transition_end(std::uint32_t clock) {
    const std::lock_guard lock(class_mutex);
    class_update_locked(clock);
    // Map activation restores classes over two seconds. Its literal immediate
    // selector is scripted_dialog_player, which matches none of the54 names
    // in TU1.5 (native_tu15_operands.generated.cpp). The new world's audio owns
    // its own activation; these are
    // the retained menu voices that survive the renderer handoff.
    transition_classes.set("",-64.f,0.f,true);
    transition_classes.set("",0.f,2.f,true);
    transition_classes.set("scripted_dialog_player",0.f,0.f,true);
    class_update_locked(clock);
  }
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
