#include "halo2/source_sound_hrtf_tu15.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_sdk_spatial.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
// The game's sound-driver floating-point state (all exceptions masked).
constexpr SoundDriverFloatingPointContext sdk_fp{0x027f, 0x1f80};
// 3812E0 folds the rear kernels only with rear speakers.
constexpr std::uint32_t stereo_fold = 0, surround_fold = 1;
// 37DFE1's distance factor: the listener's (3.048, which 21E4B0 sets with
// IDirectSound_SetDistanceFactor) times the buffer's, DirectSound's default
// 1.0 (the DS3DBUFFER template at 385850), which Halo never changes.
constexpr std::uint32_t front_rear_distance_factor = 0x4043126fu;
// DirectSound 3D head-relative mode, the frame 129790 supplies.
constexpr std::uint32_t head_relative = 1;

std::array<std::uint32_t, 3> words(Vec3 v) {
  return {std::bit_cast<std::uint32_t>(v.x), std::bit_cast<std::uint32_t>(v.y),
          std::bit_cast<std::uint32_t>(v.z)};
}
} // namespace

float source_sound_hrtf_coefficient_tu15(std::uint8_t code) {
  const unsigned exponent = (code >> 4) & 7u, mantissa = code & 15u;
  const unsigned magnitude = exponent ? (16u + mantissa) << (exponent - 1u) : mantissa;
  return code & 0x80u ? -float(magnitude) : float(magnitude);
}

std::optional<Vec3> source_sound_head_position_tu15(Vec3 listener, Vec3 forward, Vec3 up, Vec3 point) {
  // Halo frames are forward/left/up; the SDK's head-relative axes are
  // right/up/forward.
  const auto ahead = normalized(forward);
  const auto right = normalized(cross(ahead, up));
  if (dot(ahead, ahead) == 0.f || dot(right, right) == 0.f) return {};
  const auto above = cross(right, ahead);
  const auto offset = point - listener;
  return Vec3{dot(offset, right), dot(offset, above), dot(offset, ahead)};
}

SourceSoundHrtfTu15::SourceSoundHrtfTu15(const SoundSdkSpatialData &data, SourceSoundHrtfScaleTu15 scale)
    : data_(data) {
  if (scale == SourceSoundHrtfScaleTu15::VoiceProcessor) {
    scale_ = 1.f / 2048.f;
    return;
  }
  // Straight ahead, both ears share one kernel; scale it to unity power.
  const auto ahead = source_sound_sdk_kernel(data_.constants.zero_bits, stereo_fold, data_.constants, sdk_fp);
  const auto kernel = data_.kernel(ahead[0]);
  double energy = 0;
  for (unsigned n = 0; n < SdlSoundHrtf::taps; ++n) {
    const double tap = source_sound_hrtf_coefficient_tu15(std::to_integer<std::uint8_t>(kernel[n]));
    energy += tap * tap;
  }
  if (!(energy > 0)) throw std::runtime_error("Original HRTF straight-ahead kernel is silent");
  scale_ = float(1.0 / std::sqrt(energy));
}

float SourceSoundHrtfTu15::azimuth(Vec3 head) const {
  const auto &c = data_.constants;
  // 380168 ignores the listener in head-relative mode and drops height.
  const auto position = source_sound_sdk_position(head_relative, {}, words(head), c, sdk_fp);
  const auto angles = source_sound_sdk_angles(head_relative, c.mode_one_forward, c.mode_one_side,
                                              position.direction, position.distance_bits, c, sdk_fp);
  return std::bit_cast<float>(angles.azimuth_bits);
}

SdlSoundHrtf SourceSoundHrtfTu15::filter(Vec3 head, bool surround) const {
  if (!std::isfinite(head.x) || !std::isfinite(head.y) || !std::isfinite(head.z))
    throw std::invalid_argument("HRTF voice lost its finite head-relative position");
  const auto &c = data_.constants;
  const auto position = source_sound_sdk_position(head_relative, {}, words(head), c, sdk_fp);
  const auto angles = source_sound_sdk_angles(head_relative, c.mode_one_forward, c.mode_one_side,
                                              position.direction, position.distance_bits, c, sdk_fp);
  const auto angle = std::bit_cast<float>(angles.azimuth_bits);
  const auto kernels = source_sound_sdk_kernel(angles.azimuth_bits, surround ? surround_fold : stereo_fold,
                                               c, sdk_fp);
  const auto left = data_.kernel(kernels[0]), right = data_.kernel(kernels[1]);
  SdlSoundHrtf out;
  for (unsigned n = 0; n < SdlSoundHrtf::taps; ++n) {
    out.left[n] = scale_ * source_sound_hrtf_coefficient_tu15(std::to_integer<std::uint8_t>(left[n]));
    out.right[n] = scale_ * source_sound_hrtf_coefficient_tu15(std::to_integer<std::uint8_t>(right[n]));
  }
  // 3812E0 packs one signed delay: the left kernel's for a source on the
  // right (or ahead), the negated right kernel's for one on the left. A
  // positive delay holds back the left ear.
  const auto delay = angle < 0.f ? -std::int32_t(std::to_integer<std::uint8_t>(right[31]))
                                 : std::int32_t(std::to_integer<std::uint8_t>(left[31]));
  if (delay < -63 || delay > 63) throw std::runtime_error("Original HRTF delay escapes the voice processor's field");
  out.left_delay = std::uint8_t(delay > 0 ? delay : 0);
  out.right_delay = std::uint8_t(delay < 0 ? -delay : 0);
  // 37DFE1: the share behind the listener goes to the back pair; a source
  // within half a metre spreads toward both. Stereo leaves the rear off.
  const auto split = source_sound_sdk_front_rear(front_rear_distance_factor, position.distance_bits,
                                                 angles.azimuth_bits, surround ? 1u : 0u, c, sdk_fp);
  out.front = split.front;
  out.rear = split.rear;
  return out;
}
} // namespace halo2
