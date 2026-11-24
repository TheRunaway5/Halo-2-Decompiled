#pragma once
#include "halo2/math.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include "halo2/sound_sdk_spatial_data.hpp"
#include <array>
#include <cstdint>

namespace halo2 {
// A software stand-in for the Xbox APU's HRTF stage of a 3D voice, built
// from the selected executable's own HRTF kernel table.
//
// TU1.5 129790 hands each world-located voice to the driver as a position
// in its listener's head frame. The SDK then projects that position onto the
// horizontal plane (380168), measures its azimuth (37DEF9) and selects that
// azimuth's kernel pair in 3-degree steps (37E098). The HRTF owner (3812E0)
// loads both ears' 31 coefficients and the signed interaural delay into the
// voice processor. On a stereo speaker setup nothing is folded: the rear
// kernels supply the rear cues. With rear speakers (surround or Dolby
// Digital), a rear azimuth takes its mirrored front kernel and DirectSound
// splits the voice between the front and back pairs instead (37DFE1, in
// Halo's "light HRTF, 4 channel" mode). Those steps run here with the
// original numerical owners; only the voice processor's filter itself is
// replaced, by a plain 31-tap FIR per ear (SdlSoundHrtf).
//
// Each coefficient byte is a sign bit, a 3-bit exponent and a 4-bit
// mantissa (source_sound_hrtf_coefficient_tu15). It is the only reading
// under which the table behaves like a head: both ears flat straight ahead,
// and at the side a flat near ear and a head-shadowed far ear. The byte
// after the coefficients is that ear's delay in 48 kHz frames; it follows
// the spherical-head delay (12 frames at 30 degrees, 30 at 90).
//
// 3812E0 writes the bytes to the voice processor unchanged, and the voice
// processor applies them as written: a coefficient is its signed magnitude
// over 2048, so 7Fh passes 31/32 (retail-console measurements of DC through
// single taps, xemu pull request 3105). At that scale the loudest kernel
// peaks at -3.8 dB and a source straight ahead plays about 8.7 dB below an
// unfiltered voice. UnityPower instead scales the straight-ahead kernel to
// unity power, this port's level before the scale was known.
enum class SourceSoundHrtfScaleTu15 { VoiceProcessor, UnityPower };

class SourceSoundHrtfTu15 {
public:
  explicit SourceSoundHrtfTu15(const SoundSdkSpatialData &,
                               SourceSoundHrtfScaleTu15 = SourceSoundHrtfScaleTu15::VoiceProcessor);
  // head: the voice's position in its listener's head frame, in the SDK's
  // head-relative axes (x right, y up, z forward). surround: the output has
  // a back speaker pair.
  SdlSoundHrtf filter(Vec3 head, bool surround = false) const;
  // The SDK azimuth of that position in degrees: positive to the right,
  // +-180 behind.
  float azimuth(Vec3 head) const;

private:
  SoundSdkSpatialData data_;
  float scale_{};
};

// One HRTF coefficient byte as a signed magnitude: (16 + mantissa) shifted
// by exponent - 1, or the bare mantissa at exponent 0. The voice processor
// divides it by 2048.
float source_sound_hrtf_coefficient_tu15(std::uint8_t code);

// The head-relative position source_sound_hrtf_tu15 expects, for a world
// point heard by a listener at listener facing forward with the given up.
// Nothing when the listener's frame is degenerate.
std::optional<Vec3> source_sound_head_position_tu15(Vec3 listener, Vec3 forward, Vec3 up, Vec3 point);
} // namespace halo2
