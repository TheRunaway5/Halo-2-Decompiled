#pragma once
// Native TU1.5 tolerance checks, entered at AA200 and AA270 (death cycle,
// AI frame and physics live support units): whether a value is within
// 46A180 of where it should be.
//
//   tolerance::unit_length   AA200(EAX = vector) -> EAX
//   tolerance::near          AA270(XMM0 = value; target) -> EAX
//
// AA200: the vector's squared length less one; AA270: the value less the
// target. A difference whose exponent is all ones (infinite or NaN) is not
// within tolerance; otherwise the result is whether 46A180 is above its
// magnitude, compared on the x87 (FLD, FABS, FLD, FCOMIP).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_tolerance_checks.hpp) maps the role to the original
// instruction and runs the x87 comparison on the logical x87.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_tolerance_checks {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t one=0x46a0d0u,tolerance=0x46a180u;   // floats
}
inline constexpr std::uint32_t exponent_bits=0x7f800000u;

// Frame (both checks):
//   local(Local)           a local's address now
//   within(difference)     the x87 test (the loads ran it): whether the
//                          tolerance is above |difference|
namespace unit_length {
enum class Local {difference,difference_copy};
enum class Access {y,z,x,one,difference_store,difference_bits,difference_copy_store,difference_x87,tolerance};
// The SSE registers the check leaves.
struct Squares {std::uint32_t difference,z,z_squared,y_squared;};
template<class Memory,class Frame> std::uint32_t check(Memory &memory,Frame &frame,std::uint32_t vector) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto vy=memory.load32(y,at(vector,4u));
    const auto vz=memory.load32(z,at(vector,8u));
    const auto vx=memory.load32(x,vector);
    const auto y_squared=sse::multiply(vy,vy),z_squared=sse::multiply(vz,vz);
    const auto squared=sse::add(sse::add(sse::multiply(vx,vx),y_squared),z_squared);
    const auto difference=sse::subtract(squared,memory.load32(one,globals::one));
    frame.squares({difference,vz,z_squared,y_squared});
    memory.store32(difference_store,local(Local::difference),difference);
    const auto bits=memory.load32(difference_bits,local(Local::difference));
    memory.store32(difference_copy_store,local(Local::difference_copy),difference);
    if((bits&exponent_bits)==exponent_bits)return 0u;
    (void)memory.load32(difference_x87,local(Local::difference_copy));
    (void)memory.load32(tolerance,globals::tolerance);
    return frame.within()?1u:0u;
}
} // namespace unit_length

namespace near {
enum class Local {target,slot};
enum class Access {target,difference_store,difference_bits,slot_store,difference_x87,tolerance};
template<class Memory,class Frame> std::uint32_t check(Memory &memory,Frame &frame,std::uint32_t value) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto difference=sse::subtract(value,memory.load32(target,local(Local::target)));
    frame.difference(difference);
    memory.store32(difference_store,local(Local::target),difference);
    const auto bits=memory.load32(difference_bits,local(Local::target));
    memory.store32(slot_store,local(Local::slot),difference);
    if((bits&exponent_bits)==exponent_bits)return 0u;
    (void)memory.load32(difference_x87,local(Local::slot));
    (void)memory.load32(tolerance,globals::tolerance);
    return frame.within()?1u:0u;
}
} // namespace near
} // namespace halo2::native_tolerance_checks
