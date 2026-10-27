#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
// Presentation journal of completed TU1.5 point impulses (1D94CE -> 324F80)
// on loose physics objects. The host only observes the original call; the
// event lets a non-host replica start moving on the first presented frame
// after the snapshot that carries it, before the pose budget catches up.
// It never feeds a simulation and is not an Xbox System Link record.
//
// An event carries what a replica applies: the object's root-body linear
// and angular velocity after its last impulse that tick. The impulse vector
// and application point stay with the original call; no replica reads them.
// Every vector is held at its published value (a shared power-of-two
// exponent and three signed 8-bit mantissas, 1/127 of its largest
// component), so the host's journal and a decoded replica journal are
// identical. Serials are contiguous. On the wire the journal is the newest
// serial plus 32 fixed slots (serial mod 32), each one byte when empty or
// 17 bytes when occupied: tick, object, then linear and angular velocity at
// 4 bytes each. Fixed slots keep retained events byte-identical, and a new
// event usually overwrites an older one in place, so the delta codec pays
// only for the new event's own bytes: no list shift, no new anchor key.
struct PhysicsImpulseEvent {
  std::uint32_t serial{},tick{},object=0xffffffffu;
  Vec3 linear_velocity;  // Root body after the impulse, world units / s.
  Vec3 angular_velocity; // Root body after the impulse, world radians / s.
};
// Bitwise equality of every published field.
bool same_physics_impulse(const PhysicsImpulseEvent &,const PhysicsImpulseEvent &);
// Bounded retention, as the rumble journal: the newest 32 events, none
// older than 10 s. Replicas apply only events newer than the snapshot they
// already hold, so retention tolerates loss and costs nothing per tick.
inline constexpr std::size_t physics_impulse_journal_capacity=32;
inline constexpr std::uint32_t physics_impulse_retention_ticks=300;
// Published vector form: byte 0 is the signed exponent e (-24..24), bytes
// 1..3 the signed mantissas c (-127..127); each component is c * 2^e.
// Rounds to nearest, saturating; non-finite inputs are rejected.
std::array<std::uint8_t,4> physics_impulse_vector_bits(Vec3);
Vec3 physics_impulse_vector_value(const std::array<std::uint8_t,4> &);
Vec3 physics_impulse_quantize(Vec3);
// Serials are nonzero and contiguous, ticks never decrease and lie within
// retention of the snapshot tick, each (object, tick) appears once, objects
// are full salted datums and every vector is exactly a published value.
void validate_physics_impulse_journal(std::span<const PhysicsImpulseEvent>,std::uint32_t tick);
// Host retention helper: append coalesced events with fresh serials and
// published vectors, then drop events older than the retention window and
// the oldest beyond capacity.
void retain_physics_impulses(std::vector<PhysicsImpulseEvent> &journal,std::uint32_t &serial,
    std::span<const PhysicsImpulseEvent> fresh,std::uint32_t tick);
}
