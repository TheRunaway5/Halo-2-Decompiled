#pragma once
#include "halo2/multiplayer_audio.hpp"
#include "halo2/vista_announcer_events.hpp"
#include "halo2/source_multiplayer_event_tu15.hpp"
#include <memory>

namespace halo2 {
struct NativeVistaAnnouncerConfiguration {
  std::filesystem::path pc_maps;
  std::string scenario;
  std::shared_ptr<const OriginalData> native_audio_constants;
  std::uint32_t language;
  std::int16_t tick_rate;
  VistaLoadingFloatingPoint floating_point;
  bool mute;
  std::uint32_t native_secondary_seed;
  SdlSoundPlatformConfiguration platform{
      SdlSoundPlatformConfiguration::Delivery::DeviceCallback,512,{}};
  bool source_tu15=false;
};
struct NativeVistaAnnouncerVoice {
  std::uint32_t queue_token;
  MultiplayerAudio::Voice native_voice;
};
struct NativeVistaAnnouncerCompletion {
  std::uint32_t queue_token;
  SdlSoundCompletion packet;
};
struct NativeVistaAnnouncerSnapshot {
  VistaAnnouncerEventState source;
  std::vector<NativeVistaAnnouncerVoice> voices;
  std::vector<NativeVistaAnnouncerCompletion> completions;
  bool game_over_published;
  bool pending_start_boundary;
  bool muted;
  std::uint32_t next_token{};
  std::uint8_t published_byte{};
};

// Native join of the separately verified PC event/queue bodies and actual SDL
// voices. The retained PC Cache owns its distinct sound datum namespace. The
// 32-bit queue token is an explicit native adapter, never a narrowed voice ID.
class NativeVistaAnnouncer {
  struct State;
  struct SnapshotData;
  struct PreparedData;
public:
  class Checkpoint {
  public:
    Checkpoint()=default;
  private:
    std::shared_ptr<const SnapshotData> data_;
    friend class NativeVistaAnnouncer;
  };
  class PreparedReplacement {
  public:
    PreparedReplacement()=default;
    ~PreparedReplacement();
    PreparedReplacement(PreparedReplacement&&) noexcept;
    PreparedReplacement& operator=(PreparedReplacement&&) noexcept;
    PreparedReplacement(const PreparedReplacement&)=delete;
  private:
    std::shared_ptr<PreparedData> data_;
    friend class NativeVistaAnnouncer;
  };
  class HeldPublication {
  public:
    ~HeldPublication();
    HeldPublication(const HeldPublication&)=delete;
    HeldPublication(HeldPublication&&)=delete;
  private:
    struct Impl;
    explicit HeldPublication(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
    friend class NativeVistaAnnouncer;
  };
  static std::unique_ptr<NativeVistaAnnouncer> open(NativeVistaAnnouncerConfiguration);
  ~NativeVistaAnnouncer();
  NativeVistaAnnouncer(const NativeVistaAnnouncer&)=delete;
  NativeVistaAnnouncer& operator=(const NativeVistaAnnouncer&)=delete;
  // One actual admitted simulation tick: source475924 ->4D0FF4, before fade.
  void pre_world_tick();
  // Actual Slayer510C2B or registered King slot6 publication from a verified
  // birth/activation transition, after
  // the real player active bit is written and before its first native spawn.
  // The caller supplies the original event and genuine local output bindings.
  std::vector<VistaAnnouncerAudioSelection> publish_player_activation(
      const VistaAnnouncerEvent&,std::uint8_t actual_world_mode,bool game_over,
      std::uint32_t actual_tick,std::span<const VistaAnnouncerLocalPlayer>);
  // Actual category6 publication from the King engine. This consumes the
  // original event fields and real local bindings; it does not establish
  // player activation, caption delivery or hill rendering.
  std::vector<VistaAnnouncerAudioSelection> publish_king_event(
      const VistaAnnouncerEvent&,std::uint8_t actual_world_mode,bool game_over,
      std::uint32_t actual_tick,std::span<const VistaAnnouncerLocalPlayer>);
  // Shared original event audio dispatch for the recovered general/Slayer/King
  // banks, including score-change publications. Captions and network delivery
  // have separate owners; this consumes actual local output bindings.
  std::vector<VistaAnnouncerAudioSelection> publish_engine_event(
      const VistaAnnouncerEvent&,std::uint8_t actual_world_mode,bool game_over,
      std::uint32_t actual_tick,std::span<const VistaAnnouncerLocalPlayer>);
  // Actual Xbox dispatch input. Local view assignments are presentation
  // bindings; recipient flags/membership and RNG come from that event's bank.
  // The copied RNG is never published into the authoritative game world.
  std::vector<VistaAnnouncerAudioSelection> publish_source_event(
      const SourceMultiplayerEventTu15&,std::span<const unsigned> local_slots);
  // Source47156A's engine/mode/game-over/C48 guards, after world simulation.
  // Local users must be actual occupied-player iterator order and bindings.
  // C48 is stored before event delivery.
  std::vector<VistaAnnouncerAudioSelection> post_world_tick(bool engine_present,
      std::uint8_t actual_world_mode,bool game_over,std::uint32_t actual_tick,
      std::span<const VistaAnnouncerLocalPlayer>);
  // Genuine packet accounting and gains. Ordinary playback permits a render
  // producer. Explicit continuation enrollment routes all producers through
  // the constructing executor, including this accounting call.
  void audio_update(float actual_master_volume,
      const std::array<float,sound_class_count>& actual_class_gains);
  void stop();
  NativeVistaAnnouncerSnapshot snapshot() const;
  const VistaAnnouncerAssets& assets() const;
  const Cache& audio_cache() const;
  // Only an explicitly Manual SDL configuration admits this test provider.
  std::vector<float> render_manual(std::size_t frames);
  // Opt in before any playback. The actual MCC producer route is synchronous
  // with the native simulation/runtime loop; ordinary owners keep their prior
  // mutex-serialized producer behavior until this succeeds.
  void enroll_continuation();
  Checkpoint checkpoint() const;
  PreparedReplacement prepare_restore(const Checkpoint&) const;
  // Takes announcer producer exclusion before its actual Core/SDL endpoint.
  // Normal methods must not be called while this nontransferable hold exists.
  HeldPublication hold_publication(PreparedReplacement&);
  bool ready(const PreparedReplacement&,const HeldPublication&) const noexcept;
  bool commit(PreparedReplacement&,HeldPublication&) noexcept;
private:
  explicit NativeVistaAnnouncer(std::shared_ptr<State>);
  std::shared_ptr<State> state_;
};
} // namespace halo2
