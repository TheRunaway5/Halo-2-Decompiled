#pragma once
#include "halo2/content.hpp"
#include "halo2/sound_classes.hpp"
#include "halo2/sound_fade.hpp"
#include <SDL.h>
#include <atomic>
#include <functional>
#include <memory>
namespace halo2 {
struct PcmSound {
  std::string name;
  std::vector<float> stereo;
  unsigned rate = 48000;
  unsigned sound_class = sound_class_count;
  struct ClassSpan { std::size_t end_frame; unsigned sound_class; };
  std::vector<ClassSpan> class_spans;
  std::size_t frames() const { return stereo.size() / 2; }
  unsigned class_at(std::size_t) const;
};
PcmSound decode_sound(const EncodedSound &);
void write_wav(const PcmSound &, const std::filesystem::path &);
class AudioEngine {
public:
  explicit AudioEngine(bool realtime = true);
  ~AudioEngine();
  AudioEngine(const AudioEngine &) = delete;
  using VoiceId = std::uint64_t;
  // Invoked only at a loop boundary under the mixer lock. Providers must use
  // prepared PCM and perform no decoding, device calls or blocking work.
  using LoopNext = std::function<std::shared_ptr<const PcmSound>()>;
  VoiceId play(std::shared_ptr<const PcmSound>, float gain = 1, float pan = 0,
               bool loop = false);
  VoiceId play_sequence(std::shared_ptr<const PcmSound> start,
                        std::shared_ptr<const PcmSound> loop,
                        std::shared_ptr<const PcmSound> end, float gain = 1,
                        float pan = 0, LoopNext choose_loop = {});
  void stop(VoiceId, bool immediate = false);
  void fade_out(VoiceId, float seconds, std::uint16_t source_curve);
  void parameters(VoiceId, float gain, float pan);
  void next_loop(VoiceId, std::shared_ptr<const PcmSound>);
  std::size_t active_voices() const;
  void stop();
  void master_volume(float);
  std::string speaker_layout(unsigned);
  unsigned output_channels() const {return channels_;}
  void paused(bool value) { paused_.store(value); }
  void class_gains(const std::array<float,sound_class_count> &);
  std::vector<float> render(std::size_t frames); // Paused offline mixer only.
  std::uint64_t mixed_frames() const { return mixed_frames_.load(); }
  float peak() const { return peak_.load(); }

private:
  struct Voice {
    std::shared_ptr<const PcmSound> sound;
    std::shared_ptr<const PcmSound> next, end;
    VoiceId id{};
    std::size_t cursor{};
    float left{}, right{};
    float gain{};
    bool looping{};
    LoopNext choose_loop;
    std::optional<SoundPlaybackFade> fade;
  };
  SDL_AudioDeviceID device_{};
  bool realtime_{};
  unsigned speaker_layout_=5,channels_=2;
  std::array<float,sound_class_count> class_gains_{};
  VoiceId next_id_ = 1;
  std::vector<Voice> voices_;
  std::atomic<std::uint64_t> mixed_frames_{};
  std::atomic<float> peak_{};
  std::atomic<float> master_volume_{1.f};
  std::atomic<bool> paused_{};
  static void callback(void *, Uint8 *, int);
};
} // namespace halo2
