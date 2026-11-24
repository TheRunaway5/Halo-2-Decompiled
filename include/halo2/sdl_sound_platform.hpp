#pragma once

#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include "halo2/sound_i3dl2_reverb.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace halo2 {

// Native platform services, not an Xbox voice/sound datum or scheduler.
// The decoder supplies stereo float PCM at 48 kHz. Its source-byte/frame
// mapping, selected permutation and cache pin remain owned by the caller.
struct SdlSoundPacket {
  std::shared_ptr<const std::vector<float>> stereo;
  std::size_t first_frame;
  std::size_t frame_count;
  std::uint64_t packet_context;
  // Optional native packet attenuation. A queued permutation keeps its own
  // volume while sharing its stream's current rate, pause and output routes.
  std::optional<std::int32_t> volume_hundredths_db;
};

// Supported host routing is an explicitly supplied stereo matrix. There is
// deliberately no implicit pan, mix-bin translation or effects substitute.
// One PCM channel's route to a speaker: 0 front left, 1 front right,
// 2 back left, 3 back right (mixbins 0, 1, 4 and 5).
struct SdlSoundRoute {
  std::uint8_t pcm_channel;
  std::uint8_t output_channel;
  float linear_gain;
};

// TU1.5 effect send (2290C0 -> 128CA2 -> driver +12 -> 2B96B0): the voice
// keeps its dry routes and adds a 0 dB send to mixbin 13, the in-place input
// of DSP record 1 (mono distortion). Creating the voice installs its record-1
// parameters, as the voice's SetEffectData does; 1973E0/2275A0's mix-in voice
// returns mixbin 13 to the front speakers at 0 dB.
struct SdlSoundEffectSend {
  tu15_sound_mono_distortion::Parameters parameters;
};

// The Xbox APU's per-voice HRTF stage for a 3D voice (see
// source_sound_hrtf_tu15.hpp): a 31-tap filter per ear and that ear's delay
// in 48 kHz frames (the voice processor holds one signed delay, so at most
// one ear's is nonzero). The voice's mono input (its two PCM channels
// averaged) feeds both ears. Each ear plays into its side's front and back
// speakers (mixbins 6/8 and 7/9) at its output channel's route gains plus
// DirectSound's front and rear levels (37DFE1, millibels). A stereo setup
// has front 0 and rear -10000, which silences the back pair. When the
// filter changes, the voice processor moves its taps and its delay from the
// last frame's to the new ones linearly across one 32-sample frame.
struct SdlSoundHrtf {
  static constexpr unsigned taps = 31;
  std::array<float, taps> left{}, right{};
  std::uint8_t left_delay{}, right_delay{};
  std::int32_t front{}, rear = -10000;
  friend bool operator==(const SdlSoundHrtf &, const SdlSoundHrtf &) = default;
};

// One path of a 3D voice through the voice processor's I3DL2 filter
// (DirectSound's filter mode 3, from 38368B): the path's linear level and
// the pole of its one-pole low-pass y = b*y + (1-b)*x, b = pole/65536,
// rounded like the voice processor's. A zero pole leaves the path open. The direct path feeds the voice's routes;
// the room path is its send to mixbin 10, the listener reverbs' input.
struct SdlSoundI3dl2Path {
  float gain{};
  std::uint16_t pole{};
  friend bool operator==(const SdlSoundI3dl2Path &, const SdlSoundI3dl2Path &) = default;
};

// The voice processor's DLS2 filter on a 3D voice, as its coefficient words
// (SoundDls2FilterTu15): a state-variable low-pass on the whole voice, ahead
// of its I3DL2 direct and room paths, with the voice processor's coefficient
// steps and rounding (sound_dls2_frequency_tu15, sound_filter_round_tu15).
struct SdlSoundDls2 {
  std::uint16_t cutoff{}, resonance{};
  friend bool operator==(const SdlSoundDls2 &, const SdlSoundDls2 &) = default;
};

// One of the two listener reverbs (the DSP's I3DL2 records 9 and 8, run
// natively by tu15_sound_ring_effect with DirectSound's parameter words)
// and the volumes of its four return voices: front left, front right, back
// left, back right. Each return plays its reverb output one 32-sample DSP
// block late, as the voice processor reads the previous block; the EP's
// stereo encoder adds each back channel to its side at -3 dB.
struct SdlSoundReverb {
  SoundI3dl2Parameters parameters;
  std::array<float, 4> speaker_gains{};
};

// The voice processor's own view of a voice's rate: its sound's sample rate
// and the pitch register DirectSound writes (4096ths of an octave relative
// to 48 kHz; 221B90's driver pitch). The PCM arrives at 48 kHz. With this set
// and the voice processor on, the mixer rebuilds the sound's native-rate
// samples from it (a band-limited windowed sinc at the native Nyquist) and
// steps through them as the voice processor does: 2^(pitch/4096) native
// samples per output sample in 1/65536, through its 8-point Lagrange filter.
// relative_rate_bits stays the 48 kHz rate for every other path.
struct SdlSoundNativeRate {
  std::uint32_t sample_rate{};
  std::int32_t pitch{};
  friend bool operator==(const SdlSoundNativeRate &, const SdlSoundNativeRate &) = default;
};

struct SdlSoundVoiceParameters {
  std::uint32_t relative_rate_bits;
  std::int32_t volume_hundredths_db;
  bool paused;
  std::vector<SdlSoundRoute> routes;
  std::optional<SdlSoundEffectSend> effect{};
  std::optional<SdlSoundHrtf> hrtf{};
  std::optional<SdlSoundI3dl2Path> room{};
  std::optional<SdlSoundI3dl2Path> direct{};
  std::optional<SdlSoundDls2> dls2{};
  std::optional<SdlSoundNativeRate> native{};
};

struct SdlSoundPlatformConfiguration {
  enum class Delivery { DeviceCallback, Manual };
  Delivery delivery;
  std::uint16_t callback_frames;
  std::optional<std::string> device_name;
  // DSP record 10's crosstalk stage on the 3D mixbins (6..9), on as the
  // DSPImage installs it. HALO2_REFERENCE_NO_CROSSTALK also turns it off.
  bool crosstalk = true;
  // The voice processor's measured level and resampling behaviour (xemu
  // pull request 3105's retail-console measurements): each route's level is
  // DirectSound's attenuation register (380B97) through the hardware's curve
  // in 1/16384, moving linearly across each 32-sample frame and fading a new
  // voice in over its first; the cursor steps in 1/65536 with the pitch
  // moving across each frame in its log form, through 8-point Lagrange
  // interpolation. Off, levels are exact and immediate and the resampler
  // linear. HALO2_REFERENCE_IDEAL_VOICES also turns it off.
  bool voice_processor = true;
};

enum class SdlSoundCompletionStatus : std::uint32_t {
  Success = 0,
  Flushed = 0x80004004u, // XMP_STATUS_FLUSHED / E_ABORT
  Failure = 0x80004005u, // XMP_STATUS_FAILURE / E_FAIL
};

struct SdlSoundCompletion {
  std::uint64_t voice;
  std::uint64_t channel_context;
  std::uint64_t packet_context;
  std::uint64_t submission;
  SdlSoundCompletionStatus status;
  std::size_t pcm_frames;
};

struct SdlSoundVoiceStatus {
  bool accepts_input;
  bool paused;
  bool device_removed;
  std::size_t pending_packets;
  std::size_t completed_packets;
};

// Observed native consumption, not a submitted-packet or wall-clock estimate.
// The first frames identify the callback/manual mixer sample that consumed
// this voice, including an authored zero-valued PCM sample.
struct SdlSoundPlaybackPosition {
  std::optional<std::uint64_t> first_device_frame;
  std::optional<std::uint64_t> first_presentation_frame;
  // Cumulative decoded 48 kHz input frames traversed, including relative
  // rate, bounded by supplied PCM. Underruns and canceled PCM add nothing.
  double source_frames{};
  std::uint64_t output_frames{};
  // A restored prefix is historical observed consumption in this publication
  // branch. Physical device frames never rewind. A resumed-frame observation
  // is absent until this branch actually consumes PCM (even zero-valued PCM).
  std::uint64_t publication_epoch{1};
  std::optional<std::uint64_t> first_resumed_device_frame;
};

class SdlSoundPlatform {
  struct State;
  struct SnapshotData;
  struct PreparedData;
public:
  using Voice = std::uint64_t;
  enum class Submission { Accepted, Backpressure };

  class PreparedReplacement;
  class HeldPublication;
  class VoiceReceipt {
  public:
    std::uint64_t id() const noexcept { return id_; }
  private:
    std::weak_ptr<State> owner_;
    std::shared_ptr<const unsigned char> record_;
    std::uint64_t id_{},epoch_{};
    friend class SdlSoundPlatform;
    friend struct State;
    friend class PreparedReplacement;
  };
  class CompletionReceipt {
  public:
    const SdlSoundCompletion &value() const noexcept { return value_; }
  private:
    VoiceReceipt voice_;
    std::shared_ptr<const unsigned char> record_;
    SdlSoundCompletion value_{};
    friend class SdlSoundPlatform;
  };
  class Checkpoint {
  public:
    Checkpoint() = default;
  private:
    std::shared_ptr<const SnapshotData> data_;
    friend class SdlSoundPlatform;
  };
  class PreparedReplacement {
  public:
    PreparedReplacement() = default;
    ~PreparedReplacement();
    PreparedReplacement(PreparedReplacement&&) noexcept;
    PreparedReplacement &operator=(PreparedReplacement&&) noexcept;
    PreparedReplacement(const PreparedReplacement&) = delete;
    // Authenticated saved-target handles may stage paired owners before commit.
    // They have fresh preparation records and a future epoch: ordinary calls
    // refuse until exactly that target publishes. Sibling/canceled preparations
    // cannot confer authority. No live raw-ID refresh exists.
    std::optional<VoiceReceipt> voice(Voice) const noexcept;
    std::optional<std::size_t> voices() const noexcept;
  private:
    std::shared_ptr<PreparedData> data_;
    friend class SdlSoundPlatform;
  };
  struct Clocks {
    std::uint64_t physical{},elapsed{},presentation{},publication_epoch{};
  };
  class HeldPublication {
  public:
    // Non-transferable: foreign destruction terminates before unlocking SDL
    // or touching retained storage. Prepared cancellation alone is transferable.
    ~HeldPublication();
    HeldPublication(const HeldPublication&) = delete;
    HeldPublication(HeldPublication&&) = delete;
    std::optional<Clocks> clocks() const noexcept;
    // LIVE preimage witness; after commit this observes the installed target.
    // It does not prove a prospective preparation's target state.
    std::optional<SdlSoundVoiceStatus> status(const VoiceReceipt&) const noexcept;
  private:
    HeldPublication(std::shared_ptr<State>,std::shared_ptr<PreparedData>);
    std::shared_ptr<State> state_;
    std::shared_ptr<PreparedData> data_;
    bool locked_{};
    friend class SdlSoundPlatform;
  };

  explicit SdlSoundPlatform(const SdlSoundPlatformConfiguration &);
  ~SdlSoundPlatform();
  SdlSoundPlatform(const SdlSoundPlatform &) = delete;
  SdlSoundPlatform &operator=(const SdlSoundPlatform &) = delete;

  // SDL must actually open the requested native device before construction
  // succeeds. No source availability flag is written by this class.
  std::uint32_t device_id() const;
  std::string speaker_layout(unsigned);
  unsigned output_channels() const;
  Voice create_voice(std::uint64_t channel_context,
                     const SdlSoundVoiceParameters &);
  // Uses the shared 48 kHz presentation clock for this voice's first PCM frame.
  // A late voice starts at the next available sample; PCM is never discarded
  // to conceal lateness. Its two packet slots retain ordinary stream behavior.
  Voice create_voice_at(std::uint64_t channel_context,
                       const SdlSoundVoiceParameters &,
                       std::uint64_t presentation_frame);
  void parameters(Voice, const SdlSoundVoiceParameters &);
  void packet_volume(Voice,std::uint64_t packet_context,std::int32_t hundredths_db);
  Submission submit(Voice, SdlSoundPacket);
  SdlSoundVoiceStatus status(Voice) const;
  SdlSoundPlaybackPosition playback_position(Voice) const;

  // A stream has exactly two retained packet slots (2215F0). Completion alone
  // does not release its PCM: acknowledge only after the source completion
  // owner has accounted for the chunk. The oldest completed packet is first.
  std::optional<SdlSoundCompletion> completion(Voice) const;
  void acknowledge(Voice, std::uint64_t submission);
  void flush(Voice);
  void destroy_voice(Voice); // Requires both retained slots to be accounted.

  // Called on the creator by the actual SDL_AUDIODEVICEREMOVED event owner. Events for other
  // devices are ignored. Pending packets fail, not complete successfully.
  bool device_removed(std::uint32_t which);

  // Uses the same mixer as the real callback, only for explicitly Manual
  // devices. It does not report physical speaker consumption or audibility.
  std::vector<float> render(std::size_t frames);
  std::uint64_t rendered_frames() const;
  // Global presentation pause holds this clock and every stream. The device
  // clock still advances while SDL consumes silence. A per-voice pause alone
  // does not move another voice's deadline or this shared clock.
  void paused(bool);
  std::uint64_t presentation_frames() const;
  // The two listener reverbs every room send feeds. Speaker gains glide to
  // new values over about 10 ms.
  void reverb(const std::array<SdlSoundReverb, 2> &);

  // Checkpoint or typed creation enrolls exclusive constructing-executor use.
  // Enrollment rejects prior foreign producer writes; unpublished legacy raw
  // calls retain their original SDL-lock behavior. Once enrolled, all producer
  // calls (including observations) use the constructing executor.
  // Do not call ordinary methods while HeldPublication excludes the callback;
  // use its read-only witnesses. Preparation tolerates normal callback progress.
  // Explicit enrollment performs no allocation or capture. Failed admission
  // leaves enrollment unchanged; success binds the creating producer once.
  void enroll_continuation();
  Checkpoint checkpoint() const;
  PreparedReplacement prepare_restore(const Checkpoint&) const;
  HeldPublication hold_publication(PreparedReplacement&);
  bool ready(const PreparedReplacement&,const HeldPublication&) const noexcept;
  bool commit(PreparedReplacement&,HeldPublication&) noexcept;
  std::uint64_t elapsed_frames() const; // Restorable, advances through global pause.
  std::uint64_t publication_epoch() const;

  // Continuable PCM must have a real retaining control block and no escaped
  // mutable writer. Const shared ownership does not authenticate sample bytes.
  VoiceReceipt create_voice_receipt(std::uint64_t,const SdlSoundVoiceParameters&,
                                   std::optional<std::uint64_t> deadline = {});
  void parameters(const VoiceReceipt&,const SdlSoundVoiceParameters&);
  void packet_volume(const VoiceReceipt&,std::uint64_t packet_context,std::int32_t hundredths_db);
  Submission submit(const VoiceReceipt&,SdlSoundPacket);
  SdlSoundVoiceStatus status(const VoiceReceipt&) const;
  SdlSoundPlaybackPosition playback_position(const VoiceReceipt&) const;
  std::optional<CompletionReceipt> completion(const VoiceReceipt&) const;
  void acknowledge(const CompletionReceipt&);
  void flush(const VoiceReceipt&);
  void destroy_voice(const VoiceReceipt&);
  // After first publication, legacy raw voice creators/mutators/ACK reject.
  // This module alone does not enable receipt/global/MultiplayerAudio adoption.

private:
  Voice create_voice_impl(std::uint64_t, const SdlSoundVoiceParameters &,
                          std::optional<std::uint64_t>,bool raw = true);
  void parameters_impl(Voice,const SdlSoundVoiceParameters&);
  void packet_volume_impl(Voice,std::uint64_t,std::int32_t);
  Submission submit_impl(Voice,SdlSoundPacket);
  void acknowledge_impl(Voice,std::uint64_t);
  void flush_impl(Voice);
  void destroy_voice_impl(Voice);
  std::shared_ptr<State> state_;
};

} // namespace halo2
