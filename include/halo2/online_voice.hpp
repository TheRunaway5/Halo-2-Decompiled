#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace halo2 {
class OnlineAccount;
inline constexpr unsigned voice_frame_samples = 960; // 20ms, 48kHz mono.
using VoicePcm = std::array<float, voice_frame_samples>;
using VoiceStereo = std::array<float, voice_frame_samples*2>;
struct VoiceAuthorization { std::string origin, bearer, clan, player; bool allow_loopback_http{}, party{}; };
struct OnlineVoiceSnapshot {
  bool joined{}, connecting{}, microphone_muted{}, transmitting{};
  std::string message;
  std::vector<std::string> talking, muted;
  std::uint64_t revision{};
};
// Codec is a native Opus wire format, not the opaque Xbox XHV format.
class VoiceCodec {
public:
  explicit VoiceCodec(bool discontinuous_transmission = false);
  ~VoiceCodec();
  std::vector<std::uint8_t> encode(const VoicePcm &);
  std::optional<VoicePcm> decode(std::span<const std::uint8_t>);
  bool speech() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
// Device abstraction permits synthetic speech in real transport tests without
// ever opening the operator's microphone. The default implementation uses SDL.
class VoiceDevices {
public:
  virtual ~VoiceDevices() = default;
  virtual void start() = 0;
  virtual void transmit(bool) = 0;
  virtual bool capture(VoicePcm &) = 0;
  virtual void playback(const VoicePcm &) = 0;
  virtual void playback_stereo(const VoiceStereo &pcm) {
    VoicePcm mono{};for(unsigned i=0;i<voice_frame_samples;++i)mono[i]=(pcm[i*2]+pcm[i*2+1])*.5f;
    playback(mono);
  }
  // Worker-only: discard queued capture and playback without reopening the
  // devices. No samples from before this call may remain queued afterward.
  virtual void discard() = 0;
  virtual void stop() = 0;
};
std::unique_ptr<VoiceDevices> sdl_voice_devices();
class OnlineVoice {
public:
  explicit OnlineVoice(std::shared_ptr<OnlineAccount>, bool party = false);
  using AuthorizationSource = std::function<std::optional<VoiceAuthorization>()>;
  OnlineVoice(AuthorizationSource, std::unique_ptr<VoiceDevices>);
  ~OnlineVoice();
  void join();
  void leave();
  void push_to_talk(bool);
  void mute_microphone(bool);
  void mute(const std::string &player, bool);
  OnlineVoiceSnapshot snapshot() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
