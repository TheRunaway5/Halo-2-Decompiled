#pragma once

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
};

// Supported host routing is an explicitly supplied stereo matrix. There is
// deliberately no implicit pan, mix-bin translation or effects substitute.
struct SdlSoundRoute {
  std::uint8_t pcm_channel;
  std::uint8_t output_channel;
  float linear_gain;
};

struct SdlSoundVoiceParameters {
  std::uint32_t relative_rate_bits;
  std::int32_t volume_hundredths_db;
  bool paused;
  std::vector<SdlSoundRoute> routes;
};

struct SdlSoundPlatformConfiguration {
  enum class Delivery { DeviceCallback, Manual };
  Delivery delivery;
  std::uint16_t callback_frames;
  std::optional<std::string> device_name;
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
  Submission submit_impl(Voice,SdlSoundPacket);
  void acknowledge_impl(Voice,std::uint64_t);
  void flush_impl(Voice);
  void destroy_voice_impl(Voice);
  std::shared_ptr<State> state_;
};

} // namespace halo2
