#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace halo2 {
// Values imported from the owned original executable and its executed static
// initializers. No guessed scope timing, FOV limit or interpolation constants.
struct UnitScopeCameraConstants {
  float default_fov, packet_minimum, packet_maximum, render_minimum, render_maximum;
  float magnified_minimum, magnified_maximum, one, packet_multiplier;
  float transition_seconds, frame_dt_multiplier;
  float half, three, five, fifteen, six, ten, twenty, two, four, twelve, seven;
  float acceleration_cap;
  double reciprocal_numerator;
};
struct UnitScopeCameraProducer {
  float persistent_fov{}; // Source normal-camera persistent value; initial zero.
};
// FOV fields extracted from an actual valid AC-byte normal observer packet.
// The caller owns all other fields and supplies the packet flags/global time.
struct UnitScopeCameraPacket {
  std::uint32_t flags;
  float fov, global_seconds;
  std::uint8_t fov_flags;
  float fov_seconds;
};
struct UnitScopeCameraState {
  bool registered{};
  std::uint32_t registered_flags{}, flags{};
  std::uint8_t fov_flags{};
  float current{}, target{}, remaining{}, velocity{}, acceleration{}, delta{};
  // Original remaining-time polynomial: fifth, fourth, third, quadratic,
  // linear, constant. These persist after expiry as in the original observer.
  std::array<float,6> coefficients{};
  float rendered_fov{};
};
// Fresh source BSS plus original170F30/170E40 constructor. This is not a reset
// of an already running observer, whose original constructor retains derivatives.
UnitScopeCameraState source_scope_camera_initial(const UnitScopeCameraConstants &);
// Original188B60. Missing unit uses its executed static fallback; otherwise the
// caller supplies real bipd+CC and C9BE0 magnification of its retained view
// selector. This leaf does not check admission or the unit current selector:
// ordinary1884E0 owns admission and clearing the retained selector beforehand.
float source_scope_camera_fov(std::optional<float> authored_unit_fov,
    float magnification,const UnitScopeCameraConstants &);
// Original23E210 scalar producer. Supply the caller-cleared packet FOV fields;
// it changes the flag/time only when the persistent target actually changes.
void source_scope_camera_target(UnitScopeCameraProducer &,UnitScopeCameraPacket &,
    float fov,const UnitScopeCameraConstants &);
// Original170C10 first registration, including forced immediate first adoption.
void source_scope_camera_register(UnitScopeCameraState &,UnitScopeCameraPacket &);
// Original170FF0 scalar adoption. Returns false when packet bit0 is clear.
bool source_scope_camera_adopt(UnitScopeCameraState &,UnitScopeCameraPacket &,
    const UnitScopeCameraConstants &);
// Original170D00 source dt writer. The caller supplies the actual frame clock.
float source_scope_camera_dt(float frame_seconds,const UnitScopeCameraConstants &);
// Original171910 FOV channel4. No position/basis reconstruction is included.
// True signals the original acceleration-cap cancellation: the caller must also
// cancel other observer channel durations equal to the pre-call FOV duration.
// A source cancellation by an earlier channel must already be reflected in
// state.remaining. The frozen stock traces never take either coupled branch.
bool source_scope_camera_motion(UnitScopeCameraState &,float source_dt,
    const UnitScopeCameraConstants &);
// Original172A50 horizontal-FOV clamp/write. Feed this into the existing source
// camera-angle projection consumer; no alternate tangent interpolation is used.
float source_scope_camera_render_fov(UnitScopeCameraState &,
    const UnitScopeCameraConstants &);
} // namespace halo2
