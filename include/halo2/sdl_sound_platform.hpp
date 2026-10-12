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

class SdlSoundPlatform {
public:
  using Voice = std::uint64_t;
  enum class Submission { Accepted, Backpressure };

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
  void parameters(Voice, const SdlSoundVoiceParameters &);
  Submission submit(Voice, SdlSoundPacket);
  SdlSoundVoiceStatus status(Voice) const;

  // A stream has exactly two retained packet slots (2215F0). Completion alone
  // does not release its PCM: acknowledge only after the source completion
  // owner has accounted for the chunk. The oldest completed packet is first.
  std::optional<SdlSoundCompletion> completion(Voice) const;
  void acknowledge(Voice, std::uint64_t submission);
  void flush(Voice);
  void destroy_voice(Voice); // Requires both retained slots to be accounted.

  // Called by the actual SDL_AUDIODEVICEREMOVED event owner. Events for other
  // devices are ignored. Pending packets fail, not complete successfully.
  bool device_removed(std::uint32_t which);

  // Uses the same mixer as the real callback, only for explicitly Manual
  // devices. It does not report physical speaker consumption or audibility.
  std::vector<float> render(std::size_t frames);
  std::uint64_t rendered_frames() const;

private:
  struct State;
  std::unique_ptr<State> state_;
};

} // namespace halo2
