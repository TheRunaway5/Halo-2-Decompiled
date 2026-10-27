#pragma once
// Native TU1.5 look update (NETCODE_ENHANCED_GOAL item 1, seated look
// prediction). One call of the original 18A300 on a retained 0x94-byte look
// input record, given its 189FD0 context and world operands:
//   - yaw += dy; a seated view with authored yaw limits (seat +88/+8C) is
//     clamped around the seat marker's world yaw (BAD60 + FPATAN), keeping
//     the nearer bound; yaw is then wrapped into [0, 2pi];
//   - pitch constraints (seat +60 block, or unit +D4 on foot) are shifted by
//     the vehicle's tilt along the look direction (11F7E0 with CRT acos
//     32CCD8) while the seat's object stands (up.z > 0.2), and clamped to the
//     global +/-1.4922565;
//   - auto-level moves pitch toward the level pitch at a rate proportional
//     to the object's speed (or when the idle flag +30 is set);
//   - the soft pitch limits +80/+84 chase the constraint limits by at most
//     pi/256 per call; pitch += dp is clamped to them.
// CD4A0/BBF20 (also reached by 189FD0) fill a camera field 18A300 never
// reads. Native code only; the translated route appears only in the oracle
// (tests/native_tu15_seat_look_tu15_tests.cpp).
#include "halo2/cache.hpp"
#include "halo2/physics_body_factory.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <optional>

namespace halo2 {
inline constexpr float native_tu15_look_pitch_bound = 1.49225652217865f; // 0x46A58C (and -0x46A588)
inline constexpr float native_tu15_look_two_pi = 6.2831854820251465f;    // 0x44923C
// The 0x94-byte record fields 18A300 reads and writes.
struct NativeSeatLookRecord {
  float yaw{}, pitch{};                                // +10, +14
  bool idle{};                                         // +30 (1898A0 local idle flag)
  float minimum = -native_tu15_look_pitch_bound, maximum = native_tu15_look_pitch_bound; // +80, +84
  bool operator==(const NativeSeatLookRecord &) const = default;
};
// 189FD0 context and the world operands of one call.
struct NativeSeatLookOperands {
  bool seated{};                       // unit +1FC != -1: the context object is its parent
  float yaw_minimum{}, yaw_maximum{};  // seat +88/+8C; both zero: no yaw limit
  Vec3 marker_forward{1, 0, 0};        // BAD60 world forward of seat marker +8 (yaw limits only)
  bool constraints = true;             // context +8 present
  float level_pitch{}, pitch_minimum{}, pitch_maximum{}; // constraints +8/+C/+10
  Vec3 up{0, 0, 1}, velocity;          // context object +7C, +88
  float level_scale{};                 // [[0x4ECC1C]+F4]+5C
  float seconds = 1.f / 30.f;          // [0x51DF54]+4
  // Startup-computed single-precision constants (0x5564C8, 0x5564C4, 0x556554).
  float pi = 3.14159274101257324f, two_pi = native_tu15_look_two_pi, acos_pi = 3.14159274101257324f;
};
// One 18A300 call (advance=true). advance=false is not an original call: it
// applies dy/dp, the yaw limits, wrap and the current soft limits without the
// per-call auto-level and soft-limit motion (presentation between ticks).
void native_tu15_seat_look(NativeSeatLookRecord &, const NativeSeatLookOperands &, float dy, float dp,
                           SourcePhysicsFactoryFloatingPoint, bool advance = true);
// 11F7E0: angle between two vectors in [0, pi] (acos(2cos^2 - 1) / 2 with
// the obtuse fold); 0 when either is zero.
float native_tu15_vector_angle(Vec3 a, Vec3 b, float pi, SourcePhysicsFactoryFloatingPoint);
// The deltas the host's look adapter delivers between two raw samples
// (PhysicsLiveWorldTu15::prepare_player_look): yaw remainder by 2pi.
float native_tu15_look_yaw_delta(float raw, float previous);

// ---- Host bank readers (oracle; the host still runs the original) ----
NativeSeatLookRecord native_tu15_seat_look_record(const SourcePhysicsContactMemory &, std::uint32_t record);
struct NativeSeatLookContext {
  std::uint32_t object = 0xffffffffu; // context object (the unit, or its parent when seated)
  std::int16_t seat = -1;
  std::uint32_t constraints{};        // address of the constraint block, 0 when absent
  std::uint32_t seat_definition{};    // address of the 0xB0 seat block when seated
  std::uint32_t marker{};             // seat +8
};
NativeSeatLookContext native_tu15_seat_look_context(const SourcePhysicsContactMemory &, std::uint32_t unit);
// Every operand except marker_forward, which the caller supplies (BAD60).
NativeSeatLookOperands native_tu15_seat_look_operands(const SourcePhysicsContactMemory &, std::uint32_t unit,
                                                      std::optional<Vec3> marker_forward = {});

// ---- Authored operands for a client (map tags only) ----
struct NativeSeatLookDefinition {
  bool constraints{};
  float level_pitch{}, pitch_minimum{}, pitch_maximum{};
  float yaw_minimum{}, yaw_maximum{};
  std::uint32_t marker{};
  bool yaw_limited() const { return yaw_minimum != 0 || yaw_maximum != 0; }
};
// Seat `seat` of vehicle/unit definition `definition` (seat +60.. and
// +88/+8C), or with seat -1 the unit's own constraints (+D4..).
NativeSeatLookDefinition native_seat_look_definition(const Cache &, std::uint32_t definition, int seat);
} // namespace halo2
