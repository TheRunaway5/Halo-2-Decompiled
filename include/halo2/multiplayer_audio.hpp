#pragma once
#include "halo2/audio.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include <cstdint>
#include <functional>
#include <optional>

namespace halo2 {
// Literal BSP sound palette/paths, not an initialized Xbox sound system.
struct MultiplayerAudioBsp {
  struct Palette {std::uint32_t sound=0xffffffffu,inside_sound=0xffffffffu,flags{};
    float cutoff{},inside{},portal{},outside{},fade_seconds{};};
  struct Cluster {std::int16_t palette=-1,path=-1;};
  std::vector<Palette> palette;
  std::vector<Cluster> clusters;
  std::vector<std::vector<std::uint16_t>> paths;
  std::vector<std::uint8_t> pair_distances;
  float distance_lower{},distance_upper{};
  bool encoded_distances{};
};
struct MultiplayerAmbientTarget {
  std::uint32_t sound=0xffffffffu;
  unsigned palette{};
  float gain{},gain_db{},fade_seconds{};
};
// 18D730 ->18AE80's authored ambient target selection. Weather-modulated
// palettes require actual weather input; callers must supply it explicitly.
std::vector<MultiplayerAmbientTarget> multiplayer_ambient_targets(
    const MultiplayerAudioBsp &,const BspAtmosphere &,Vec3 listener,
    int cluster,float weather_intensity,bool weather_enabled);

enum class MultiplayerAudioKind {Impulse,FirstPerson,Ambient,Footstep,Shield,DamageVoice};
struct MultiplayerAudioSubmission {
  MultiplayerAudioKind kind{};
  std::uint32_t sound=0xffffffffu,owner=0xffffffffu;
  unsigned view{},pitch_range{},permutation{};
  std::uint64_t voice{};
  std::size_t frames{};
  std::uint32_t rng_before{},rng_after{};
  // Actual source pitch passed to the driver conversion for this packet.
  std::uint32_t source_pitch_bits{};
  // Actual caller world point, retained through selection and packet delivery.
  std::optional<Vec3> position;
};
using MultiplayerAudioObserver=std::function<void(const MultiplayerAudioSubmission &)>;

// Native playback adapter. Original authored tags, selection kernels and FP
// clock events feed a genuine SDL device. No Xbox driver, admission flag,
// object header, listener record, or successful source handle is fabricated.
class MultiplayerAudio {
public:
  using Voice = std::uint64_t;
  using Completion = std::function<void(const SdlSoundCompletion &)>;
  // Optional game-thread auxiliary word, retained through all requests.
  // Gain/pitch variation borrows it; impulses copy the post-pitch word into
  // their own permutation stream. The caller must outlive this adapter.
  MultiplayerAudio(const Cache &,std::uint32_t native_secondary_seed,
      const SdlSoundPlatformConfiguration &,MultiplayerAudioObserver={},
      std::uint32_t *shared_variation_random=nullptr);
  ~MultiplayerAudio();
  MultiplayerAudio(const MultiplayerAudio &)=delete;
  void prepare(std::uint32_t sound);
  void first_person(unsigned view,const FirstPersonViewBinding &,
                    const FirstPersonAnimationEvent &);
  // Source caller control interpolates authored gain and pitch bounds.
  void impulse(std::uint32_t sound,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0);
  // Returns the actual native voice, without narrowing to a source datum.
  // NONE requests retain their existing no-op behavior and return no handle.
  // A successful request owns its callback until its single PCM packet is
  // accounted. update()/stop() invoke it once on their caller's thread, after
  // acknowledgement and voice destruction, never under the SDL mixer lock.
  // Completion retains the platform's actual status, including Flushed or
  // Failure; stopping an already consumed packet still reports Success.
  // Callbacks may submit a subsequent request. All retired callbacks run even
  // if one throws; update()/stop() then rethrow the first exception. Destruction
  // cancels remaining packets, rejects new requests and suppresses exceptions.
  std::optional<Voice> play_impulse(std::uint32_t sound,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,Completion={});
  // Explicit world emitter and current local listener positions. A missing
  // listener makes a world voice silent; it never becomes a centered UI sound.
  // Uses authored class/playback distance bounds and original distance gain.
  // This handoff does not provide Xbox HRTF/DSP, portal or PVS processing.
  void listeners(std::span<const Vec3>);
  void impulse_at(std::uint32_t sound,Vec3 position,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu);
  std::vector<Voice> interface_loop(std::uint32_t looping_sound,
      std::uint32_t owner,unsigned view);
  void stop_voice(Voice);
  bool voice_active(Voice) const;
  std::optional<Voice> play_impulse_at(std::uint32_t sound,Vec3 position,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,std::uint32_t owner=0xffffffffu);
  void ambient(std::span<const MultiplayerAmbientTarget>,float elapsed);
  void master_volume(float);
  std::string speaker_layout(unsigned);
  void class_gains(const std::array<float,sound_class_count> &);
  void update();
  void stop();
  std::size_t voices() const;
  std::vector<float> render(std::size_t frames); // Explicit manual SDL provider.
  std::uint64_t mixed_frames() const;
private:
  struct State;
  std::unique_ptr<State> state_;
};
} // namespace halo2
