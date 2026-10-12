#pragma once
#include "halo2/cache.hpp"
#include "halo2/math.hpp"
#include <functional>
#include <map>

namespace halo2 {
struct EffectSoundCue {
  float elapsed{},gain=1;
  std::uint32_t sound=0xffffffffu;
  unsigned event{}; // Original event identity; equal start times do not merge parts.
};
// Literal fixed, non-looping effe event/part sound path. Original179D10,
// 17AD20,17BDF0 and17C310 select the cue and scale. Random skip/delay,
// environment transitions and nested effect owners require their own path.
std::vector<EffectSoundCue> fixed_effect_sound_cues(const Cache&,std::uint32_t,
    float effect_a=1,float effect_b=1);

// Native game-clock adapter for the above authored timeline. Its pending
// records are host records, not fabricated original effect/audio handles.
class NativeEffectSounds {
public:
  explicit NativeEffectSounds(const Cache &cache):cache_(cache){}
  void spawn(std::uint32_t effect,std::uint32_t owner,Vec3 position);
  using Submit=std::function<void(const EffectSoundCue&,std::uint32_t,Vec3)>;
  void advance(float seconds,const Submit&);
private:
  const Cache& cache_;
  std::map<std::uint32_t,std::vector<EffectSoundCue>> definitions_;
  struct Pending{EffectSoundCue cue;std::uint32_t owner{};Vec3 position;};
  std::vector<Pending> pending_;
};
}
