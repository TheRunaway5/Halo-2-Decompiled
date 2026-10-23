#pragma once
#include "halo2/audio.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include <cstdint>
#include <functional>
#include <optional>

namespace halo2 {
class MccSoundVariationProvider;
class MccScriptSoundWorldAccess;
class NativeVistaAnnouncer;
class NativeUnitDamageAudio;
class EffectAudioAuxiliary;
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

// One pending actual-Audio/clock admission. Destruction only requests
// cancellation, including on a foreign thread; the Audio producer drains it.
// Moving transfers that request lifetime. Copies/numeric reconstructions are
// unavailable. Captured sound/selection are immutable, not a playback result.
class MultiplayerPreparedScriptImpulseTu15 {
public:
  ~MultiplayerPreparedScriptImpulseTu15();
  MultiplayerPreparedScriptImpulseTu15(MultiplayerPreparedScriptImpulseTu15&&) noexcept;
  MultiplayerPreparedScriptImpulseTu15&operator=(MultiplayerPreparedScriptImpulseTu15&&) noexcept;
  MultiplayerPreparedScriptImpulseTu15(const MultiplayerPreparedScriptImpulseTu15&)=delete;
  MultiplayerPreparedScriptImpulseTu15&operator=(const MultiplayerPreparedScriptImpulseTu15&)=delete;
  std::uint32_t sound() const;
  SourceScriptSoundClockSelectionTu15 selection() const;
private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  explicit MultiplayerPreparedScriptImpulseTu15(std::shared_ptr<Impl>);
  void request_cancel() noexcept;
  friend class MultiplayerAudio;
};

// Native playback adapter. Original authored tags, selection kernels and FP
// clock events feed a genuine SDL device. No Xbox driver, admission flag,
// object header, listener record, or successful source handle is fabricated.
class MultiplayerAudio {
  struct ContinuationSnapshot;
  struct ContinuationPrepared;
  struct PublicationHold;
public:
  using Voice = std::uint64_t;
  using Completion = std::function<void(const SdlSoundCompletion &)>;
  class VoiceReceipt {
  public:
    Voice id() const noexcept { return id_; }
    bool same(const VoiceReceipt&) const noexcept;
  private:
    std::weak_ptr<const void> owner_;
    const void* owner_pointer_{};
    std::shared_ptr<const unsigned char> record_;
    Voice id_{};
    std::uint64_t epoch_{};
    friend class MultiplayerAudio;
  };
private:
  struct CompanionCompletion {
    std::weak_ptr<void> owner;
    std::shared_ptr<void> branch;
    std::uint64_t token{};
    bool queued{};
    void (*dispatch)(const std::shared_ptr<void>&,const std::shared_ptr<void>&,
        std::uint64_t,bool,const VoiceReceipt&,const SdlSoundCompletion&){};
  };
  struct CompanionVoice { VoiceReceipt voice; CompanionCompletion completion; };
  using CompanionBinder=std::function<CompanionCompletion(
      const CompanionCompletion&,const VoiceReceipt&)>;
public:
  class Checkpoint {
  public:
    Checkpoint() = default;
    std::optional<std::size_t> voices() const noexcept;
  private:
    std::vector<CompanionVoice> companion_completions() const;
    std::shared_ptr<const ContinuationSnapshot> data_;
    friend class MultiplayerAudio;
    friend class NativeVistaAnnouncer;
  };
  class PreparationIdentity {
  public:
    PreparationIdentity() = default;
  private:
    std::weak_ptr<const void> owner_;
    const void* pointer_{};
    friend class MultiplayerAudio;
  };
  class PreparedReplacement {
  public:
    PreparedReplacement() = default;
    ~PreparedReplacement();
    PreparedReplacement(PreparedReplacement&&) noexcept;
    PreparedReplacement& operator=(PreparedReplacement&&) noexcept;
    PreparedReplacement(const PreparedReplacement&) = delete;
    std::optional<VoiceReceipt> voice(const VoiceReceipt&) const noexcept;
    std::optional<std::size_t> voices() const noexcept;
    bool published() const noexcept;
    bool for_audio(const MultiplayerAudio&) const noexcept;
    PreparationIdentity identity() const noexcept;
    bool matches(const PreparationIdentity&) const noexcept;
    // Once-only transfer of the captured external pending caller. Before
    // publication its destruction cancels this preparation; after a reserved
    // commit intent it queues a request for the published producer to drain.
    std::optional<MultiplayerPreparedScriptImpulseTu15> script(
        const MultiplayerPreparedScriptImpulseTu15&);
  private:
    bool auxiliary_agrees(const std::uint32_t*,std::uint32_t) const noexcept;
    std::shared_ptr<ContinuationPrepared> data_;
    friend class MultiplayerAudio;
    friend class NativeUnitDamageAudio;
  };
  class HeldPublication {
  public:
    // Foreign destruction terminates before touching SDL or retained storage.
    ~HeldPublication();
    HeldPublication(const HeldPublication&) = delete;
    HeldPublication(HeldPublication&&) = delete;
  private:
    explicit HeldPublication(std::unique_ptr<PublicationHold>);
    std::unique_ptr<PublicationHold> data_;
    friend class MultiplayerAudio;
  };
  // Optional game-thread auxiliary word, retained through all requests.
  // Gain/pitch variation borrows it; impulses copy the post-pitch word into
  // their own permutation stream. The caller must outlive this adapter.
  MultiplayerAudio(const Cache &,std::uint32_t native_secondary_seed,
      const SdlSoundPlatformConfiguration &,MultiplayerAudioObserver={},
      std::uint32_t *shared_variation_random=nullptr);
  // Retains the actual Cache owner. Ownership alone preserves the legacy
  // producer policy; it does not enroll playback for continuation.
  MultiplayerAudio(std::shared_ptr<const Cache>,std::uint32_t native_secondary_seed,
      const SdlSoundPlatformConfiguration &,MultiplayerAudioObserver={},
      std::uint32_t *shared_variation_random=nullptr);
  ~MultiplayerAudio();
  MultiplayerAudio(const MultiplayerAudio &)=delete;
  // Explicit creator-thread, externally quiescent enrollment, before any
  // voice or script-clock binding. Requires the retaining Cache constructor.
  // Existing owning/borrowed users keep their prior producer behavior until
  // this succeeds. Enrolled playback uses exact typed SDL receipts internally;
  // enrollment supplies no receipt/global runtime readiness.
  void enroll_continuation();
  bool continuation_enrolled() const noexcept;
  // Complete local playback, not a receipt/global adoption proof. External
  // native/Aux/companion owners must join their genuine aggregate separately.
  Checkpoint checkpoint() const;
  PreparedReplacement prepare_restore(const Checkpoint&) const;
  PreparedReplacement prepare_restore(const Checkpoint&,const EffectAudioAuxiliary&) const;
  // Actual prospective native target, minted by a real retained publication
  // scope. Keep that SAME creator-owned scope alive through the preparation
  // wait and final publication. This value retains only its weak scope binding;
  // it supplies no freeze, cut selection or full native/device readiness.
  // Authored/PCM compatibility is validated without resetting saved Audio RNG,
  // variation histories, voices or clocks. Timing/SDL holds are acquired only
  // for final publication, in native -> timing -> SDL order.
  PreparedReplacement prepare_restore(const Checkpoint&,
      const MccSoundVariationProvider::NativeBinding&) const;
  PreparedReplacement prepare_restore(const Checkpoint&,const EffectAudioAuxiliary&,
      const MccSoundVariationProvider::NativeBinding&) const;
  HeldPublication hold_publication(PreparedReplacement&);
  bool ready(const PreparedReplacement&,const HeldPublication&) const noexcept;
  // Reserve the exact local transaction before another owner swaps. Pending
  // wrapper cancellation then queues for the published branch; it cannot
  // invalidate this reserved image between aggregate commits.
  // Keep this preparation and all companion wrappers stable until every
  // aggregate swap completes. Abandoning Held cancels an unpublished intent.
  bool begin_commit(PreparedReplacement&,HeldPublication&) noexcept;
  bool commit(PreparedReplacement&,HeldPublication&) noexcept;
  // Bind converted native metadata before playback, or explicitly rebind
  // after all playback, pending admissions and completion dispatch quiesce.
  // Rebinding clears cached definitions and invalidates old script clocks;
  // callers create a fresh clock afterward. Stop Audio before World restore.
  // The raw Cache continues to own DAT/PCM. A stale provider never falls back.
  void bind_native_sound_metadata(const MccSoundVariationProvider&);
  void prepare(std::uint32_t sound);
  void first_person(unsigned view,const FirstPersonViewBinding &,
                    const FirstPersonAnimationEvent &);
  // Native stereo delivery of an actual completed source caller's raw
  // one-shot request (18CEFC ->223B20). Does not change its original admission
  // result, manufacture a source voice, or implement controller/loop ownership.
  std::optional<Voice> source_impulse(std::uint32_t sound,
      std::span<const std::byte,160> request,MultiplayerAudioKind,unsigned view);
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
  std::optional<VoiceReceipt> play_impulse_receipt(std::uint32_t sound,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,Completion={});
  // Native output deadline only: selection and shared RNG still occur at
  // this call, exactly as for play_impulse. This does not evaluate a future
  // script request or create/reserve an Xbox sound handle.
  std::optional<Voice> schedule_impulse(std::uint32_t sound,
      std::uint64_t presentation_frame,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,Completion={});
  std::optional<VoiceReceipt> schedule_impulse_receipt(std::uint32_t sound,
      std::uint64_t presentation_frame,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,Completion={});
  // Additive script handoff. All producer operations use the creating thread;
  // callbacks are qualified original policy providers, not PCM clocks. The
  // stop request remains explicit: current immediate stop_voice is not an
  // implicit128E70 adapter. No-slot/NONE requests create no native voice.
  // Once a script clock is bound, ordinary Audio operations and destruction
  // also use its creating thread. Prepared destruction alone is transferable.
  std::shared_ptr<SourceScriptSoundClockNative64Tu15> make_script_sound_clock();
  // Restore qualified metadata read from the actual World clock table. This
  // needs quiescent Audio and a fresh script epoch (normally after metadata
  // rebind). No serialized source voice is imported or converted to a host ID.
  std::shared_ptr<SourceScriptSoundClockNative64Tu15> make_script_sound_clock(
      const SourceScriptSoundClockRowsTu15& qualified_restored_metadata);
  std::optional<MultiplayerPreparedScriptImpulseTu15> prepare_script_impulse(
      const std::shared_ptr<SourceScriptSoundClockNative64Tu15>&,
      std::uint32_t sound,const SourceScriptSoundClockPolicyTu15&,
      const SourceScriptSoundClockNative64Tu15::StopRequest&,
      const SoundDriverFloatingPointContext&);
  std::optional<Voice> play_prepared_script_impulse(
      MultiplayerPreparedScriptImpulseTu15&,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,
      std::optional<Vec3> position={},
      std::optional<std::uint64_t> presentation_frame={},Completion={});
  // Initial FF selection uses the actual World's shared range/used/RNG
  // owners. The valid forced-selector path retains its existing bypass.
  // The capability is retained for this call. After a phase failure, quiesce
  // and restore World/Audio; do not overwrite partial World RNG publication
  // with a caller's earlier local seed. Later packets remain separate.
  std::optional<Voice> play_prepared_script_impulse(
      MultiplayerPreparedScriptImpulseTu15&,const MccScriptSoundWorldAccess&,
      float control=1,MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu,unsigned view=0,
      std::optional<Vec3> position={},
      std::optional<std::uint64_t> presentation_frame={},Completion={});
  void cancel_prepared_script_impulse(MultiplayerPreparedScriptImpulseTu15&);
  // Genuine128E70 playback-state request on this published script voice.
  // Uses the existing native Audio millisecond clock and300ms fade. Retired
  // receipts return false; foreign Audio owners reject before backend reads.
  bool request_script_stop(const SourceScriptSoundClockVoice64Tu15&);
  // Explicit world emitter and current local listener positions. A missing
  // listener makes a world voice silent; it never becomes a centered UI sound.
  // Uses authored class/playback distance bounds and original distance gain.
  // This handoff does not provide Xbox HRTF/DSP, portal or PVS processing.
  void listeners(std::span<const Vec3>);
  // Refresh a live spatial voice from its owning source object's completed
  // WORLD point. This changes only spatial parameters, never PCM or selection.
  // Centered voices and retired identities cannot acquire an emitter here.
  void emitter(Voice,Vec3);
  void emitter(const VoiceReceipt&,Vec3);
  void impulse_at(std::uint32_t sound,Vec3 position,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,
      std::uint32_t owner=0xffffffffu);
  std::vector<Voice> interface_loop(std::uint32_t looping_sound,
      std::uint32_t owner,unsigned view);
  std::vector<VoiceReceipt> interface_loop_receipts(std::uint32_t looping_sound,
      std::uint32_t owner,unsigned view);
  void stop_voice(Voice);
  void stop_voice(const VoiceReceipt&);
  bool voice_active(Voice) const;
  bool voice_active(const VoiceReceipt&) const;
  std::optional<Voice> play_impulse_at(std::uint32_t sound,Vec3 position,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,std::uint32_t owner=0xffffffffu);
  std::optional<VoiceReceipt> play_impulse_at_receipt(std::uint32_t sound,Vec3 position,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,std::uint32_t owner=0xffffffffu);
  std::optional<Voice> schedule_impulse_at(std::uint32_t sound,Vec3 position,
      std::uint64_t presentation_frame,float control=1,
      MultiplayerAudioKind=MultiplayerAudioKind::Impulse,std::uint32_t owner=0xffffffffu);
  void ambient(std::span<const MultiplayerAmbientTarget>,float elapsed);
  void master_volume(float);
  void paused(bool);
  SdlSoundVoiceParameters voice_parameters(Voice) const;
  SdlSoundVoiceParameters voice_parameters(const VoiceReceipt&) const;
  SdlSoundPlaybackPosition playback_position(Voice) const; // Live native voice only.
  SdlSoundPlaybackPosition playback_position(const VoiceReceipt&) const;
  std::string speaker_layout(unsigned);
  void class_gains(const std::array<float,sound_class_count> &);
  void update();
  void stop();
  std::size_t voices() const;
  std::vector<float> render(std::size_t frames); // Explicit manual SDL provider.
  std::uint64_t mixed_frames() const;
  std::uint64_t presentation_frames() const;
private:
  void stop_voice_impl(Voice);
  void emitter_impl(Voice,Vec3);
  PreparedReplacement prepare_restore(const Checkpoint&,const CompanionBinder&) const;
  PreparedReplacement prepare_restore(const Checkpoint&,const CompanionBinder&,const EffectAudioAuxiliary*) const;
  PreparedReplacement prepare_restore(const Checkpoint&,const CompanionBinder&,const EffectAudioAuxiliary*,
      const MccSoundVariationProvider::NativeBinding*) const;
  std::optional<VoiceReceipt> play_companion_impulse(std::uint32_t,float,
      MultiplayerAudioKind,std::uint32_t,unsigned,CompanionCompletion);
  friend class NativeVistaAnnouncer;
  std::optional<Voice> play_prepared_script_impulse_impl(
      MultiplayerPreparedScriptImpulseTu15&,const MccScriptSoundWorldAccess*,
      float,MultiplayerAudioKind,std::uint32_t,unsigned,std::optional<Vec3>,
      std::optional<std::uint64_t>,Completion);
  struct State;
  std::unique_ptr<State> state_;
};
} // namespace halo2
