#pragma once
// Native TU1.5 object motion setters for host adoption of non-host motion
// (NETCODE_ENHANCED_GOAL item 1). These reproduce the original owners over
// the live TU1.5 bank (MultiplayerObjectRuntime::memory()); they never run
// translated code. Correspondence (TU1.5 / retail / TU1.1):
//   B9800 / B77D0 / B86C0  set linear+angular velocity and wake
//     B9770  object +88/+94 stores, B7E50 mode-4 network dirty mark
//     1CD6A0 component velocity delivery (TU1.1 1C8430)
//       1D9080/1D9100 body linear/angular (TU1.1 1D47C0/1D4840), motion
//       virtuals +50/+54 = 3239F0/323A00 (TU1.1 31FE90/31FEA0)
//       1D8EB0/1D9EC0, 315C90 island query, 315CB0 activation (3165D0
//       deferred-body removal, 316D40 island activation, 322660 listeners)
//     BBBC0 (TU1.1 BAA80) wake, B9390 update propagation, BDA50/BCFB0/BE7E0
//   B4F00 / AACA0          nudge correction: BC190 transform, 144BF0 basis,
//     144090 inverse, 145560 product, 144A60 quaternion, 120150 axis/angle
//   B44C0 / AA260 decision 2 (velocity nudge): B4BD0 hold predicate (E65A0,
//     CE8F0), BC200 current velocities, 0.06/0.01 dead bands, B4F00, B4C40
//     (vehicles), B9800.
//   B4C40                  vehicle rider bookkeeping: D2B60/D2BD0 unit
//     iterator, unit +270 and +134 bit 28.
// The teleport (decision 1, B4DA0) needs B95D0 placement, which is not
// native; see the adoption owner.
#include "halo2/math.hpp"
#include "halo2/physics_body_factory.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace halo2 {
// Original 0x34-byte transform: scale, forward, left, up, position.
struct NativeTu15Transform {
  float scale = 1;
  Vec3 forward{1, 0, 0}, left{0, 1, 0}, up{0, 0, 1}, position;
};
struct NativeTu15Velocities {
  Vec3 linear, angular;
};
// BC190: the object's world transform (parent node product when attached).
NativeTu15Transform native_tu15_object_transform(const SourcePhysicsContactMemory &, std::uint32_t object);
// BC200: current velocities (root body motion, or object +88/+94).
NativeTu15Velocities native_tu15_object_velocities(const SourcePhysicsContactMemory &, std::uint32_t object);
// B4BD0: true when the original keeps a biped's physics motion (no nudge).
bool native_tu15_motion_held(const SourcePhysicsContactMemory &, std::uint32_t object,
                             SourcePhysicsFactoryFloatingPoint);
// B4F00: velocities that steer the object toward the desired pose.
NativeTu15Velocities native_tu15_nudge_correction(const SourcePhysicsContactMemory &, std::uint32_t object,
                                                  Vec3 position, Vec3 forward, Vec3 up, Vec3 linear,
                                                  Vec3 angular, SourcePhysicsFactoryFloatingPoint);
// Read-only admission for B9800: empty when the native owner covers this
// call, otherwise the unported owner it would reach. The known case is a
// deactivated Havok island: 316D40 activation runs the world's island
// listener (TU1.5 282870, collision-agent re-creation through virtual
// dispatch), which is not ported. Callers skip the write in that case; the
// host's own TU1.5 step wakes a biped that its input moves.
std::string native_tu15_object_velocities_unsupported(const SourcePhysicsContactMemory &, std::uint32_t object,
                                                     const Vec3 *linear, const Vec3 *angular);
// B9800: set velocities (either may be absent) and wake. Throws before any
// write when native_tu15_object_velocities_unsupported() refuses.
void native_tu15_set_object_velocities(SourcePhysicsContactMemory &, std::uint32_t object, const Vec3 *linear,
                                       const Vec3 *angular, SourcePhysicsFactoryFloatingPoint);
// D2B60 / D2BD0: the units the original rider iterator yields for `object`,
// in its order. It starts at the highest unit ancestor reached through
// parents whose object +4 bit 26 is set, yields that root's unit children,
// then the unit children of each root child unit with +4 bit 26 (two levels;
// the root itself is not yielded).
std::vector<std::uint32_t> native_tu15_vehicle_riders(const SourcePhysicsContactMemory &, std::uint32_t object);
// B4C40: rider bookkeeping before the original moves a vehicle on its own
// (B44C0 decision 2 with the corrected linear velocity, B4DA0 with the
// placement and linear velocity). No effect unless `object` is a vehicle
// (header +3 == 1). With `position`, the vehicle's +270 accumulates
// (+64 - position); with `linear`, every rider's +270 accumulates
// (linear - vehicle +88) * game seconds per tick (time globals +4). Every
// rider's +134 gains bit 28 (0x10000000). Never refuses.
void native_tu15_vehicle_child_update(SourcePhysicsContactMemory &, std::uint32_t object, const Vec3 *position,
                                      const Vec3 *linear, SourcePhysicsFactoryFloatingPoint);
// B44C0 decision 2 (mode 2: client motion). Absent pose fields keep the
// object's own. Returns true when B9800 wrote velocities. A vehicle runs
// B4C40 with the corrected linear velocity between B4F00 and B9800 when the
// linear velocity was taken. Refused B9800 calls throw before any write.
bool native_tu15_nudge_object(SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t mode,
                              const Vec3 *position, const Vec3 *forward, const Vec3 *up, const Vec3 *linear,
                              const Vec3 *angular, SourcePhysicsFactoryFloatingPoint);
} // namespace halo2
