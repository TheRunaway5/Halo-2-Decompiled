#pragma once
#include "halo2/cache.hpp"
#include "halo2/math.hpp"
#include <functional>
#include <map>
#include <memory>
#include <thread>

namespace halo2 {
struct EffectSoundCue {
  float elapsed{},gain=1;
  std::uint32_t sound=0xffffffffu;
  unsigned event{}; // Original event identity; equal start times do not merge parts.
  bool scale_a{},scale_b{}; // Actual part+30/+34 bit5 gain controls.
};
// Literal fixed, non-looping effe event/part sound path. Original179D10,
// 17AD20,17BDF0 and17C310 select the cue and scale. Random skip/delay,
// environment transitions and nested effect owners require their own path.
std::vector<EffectSoundCue> fixed_effect_sound_cues(const Cache&,std::uint32_t,
    float effect_a=1,float effect_b=1);
// Event identities only. The caller's original event clock owns selection
// and start time, including random skips/ranges and repeat admission.
std::vector<EffectSoundCue> authored_effect_sound_parts(const Cache&,std::uint32_t);

// Native game-clock adapter for the above authored timeline. Its pending
// records are host records, not fabricated original effect/audio handles.
class NativeEffectSounds {
  struct PreparedData;
public:
  explicit NativeEffectSounds(const Cache &cache):cache_(cache){}
  NativeEffectSounds(const NativeEffectSounds &)=delete;
  NativeEffectSounds &operator=(const NativeEffectSounds &)=delete;
  // Immutable owning copy of unsubmitted host cues, not already-playing SDL
  // voices or a network/native-world checkpoint. The enclosing coordinator
  // binds this state to its accepted world receipt at a completed-frame barrier.
  class Checkpoint {
  public:
    Checkpoint()=default;
    bool valid()const noexcept;
    std::size_t pending_count()const;
  private:
    friend class NativeEffectSounds;
    struct Data;
    std::shared_ptr<const Data> data_;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();
    PreparedRestore(PreparedRestore &&)noexcept;
    PreparedRestore &operator=(PreparedRestore &&)noexcept;
  private:
    friend class NativeEffectSounds;
    PreparedRestore();
    std::unique_ptr<PreparedData> data_;
  };
  Checkpoint checkpoint()const;
  // Past checkpoints stay eligible after later cue mutations. Preparation
  // copies their exact pending values; resident authored definitions stay here.
  bool owns_checkpoint(const Checkpoint &)const noexcept;
  PreparedRestore prepare_restore(const Checkpoint &)const;
  bool restore_ready(const PreparedRestore &)const noexcept;
  // Caller keeps this owner quiescent on its executor. Publication only swaps
  // pending storage and consumes eligibility: no allocation, callback, or
  // destruction of old cues. The caller token retains them until later disposal.
  bool commit_restore(PreparedRestore &&)noexcept;
  void spawn(std::uint32_t effect,std::uint32_t owner,Vec3 position,
      float effect_a=1,float effect_b=1);
  using Submit=std::function<void(const EffectSoundCue&,std::uint32_t,Vec3)>;
  void advance(float seconds,const Submit&);
  // Retire only unsubmitted presentation cues from a discarded world lifetime.
  // Keep authored definitions; already playing audio and accepted-world active
  // effect reconstruction remain with their enclosing owners.
  void retire_pending()noexcept;
private:
  void own()const;
  void invalidate()noexcept;
  const Cache& cache_;
  const std::thread::id executor_=std::this_thread::get_id();
  const std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
  std::uint64_t revision_{};
  bool advancing_{};
  std::map<std::uint32_t,std::vector<EffectSoundCue>> definitions_;
  struct Pending{EffectSoundCue cue;std::uint32_t owner{};Vec3 position;};
  std::vector<Pending> pending_;
};
}
