#pragma once
// Native TU1.5 light admits, entered at C5170 (light portal unit): whether a
// light takes part.
//
//   light_admits::admits   C5170(EAX = light handle) -> AL
//
// A light (0x110-byte datums of the data array at [4ECBDC]) without flag
// bit 1 (byte +2) does not. One whose two intensities (+0x24, +0x100) both
// lie above the epsilon at 46A0EC, and one of whose two vectors (+0xD4,
// +0xE0) is longer than the threshold at 46A12C (squared), does; the
// second vector is measured only when the first is not long enough.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), and the boundary
// adapter (native_tu15_light_admits.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_light_admits {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t lights=0x4ecbdcu;      // DataArray* of 0x110-byte lights
inline constexpr std::uint32_t epsilon=0x46a0ecu,threshold=0x46a12cu;
}
inline constexpr std::uint32_t datum_data=0x44u,light_size=0x110u;
namespace light {inline constexpr std::uint32_t flags=2u,intensity=0x24u,second_intensity=0x100u,first_vector=0xd4u,second_vector=0xe0u;}
inline constexpr std::uint32_t portal_flag=2u;

namespace light_admits {
enum class Access {lights,data,flags,intensity,epsilon,second_intensity,x,y,z,threshold};
enum class Outcome {unflagged,admitted,refused};
struct Length {std::uint32_t z,zz,yy,sum;};
// Frame: measuring(k) before each vector; measured(length) its registers;
// verdict(long_enough) once the lengths are measured.
// A vector's squared length; the first also reads the threshold, after its z.
template<class Memory,class Frame> std::uint32_t squared(Memory &memory,Frame &frame,std::uint32_t vector,std::uint32_t *threshold_out) {
    using enum Access;
    const auto x_value=memory.load32(x,at(vector,0u));
    const auto y_value=memory.load32(y,at(vector,4u));
    const auto z_value=memory.load32(z,at(vector,8u));
    if(threshold_out)*threshold_out=memory.load32(Access::threshold,globals::threshold);
    const auto xx=sse::multiply(x_value,x_value);
    const auto yy=sse::multiply(y_value,y_value);
    const auto xy=sse::add(xx,yy);
    const auto zz=sse::multiply(z_value,z_value);
    const Length length{z_value,zz,yy,sse::add(xy,zz)};
    frame.measured(length);
    return length.sum;
}
template<class Memory,class Frame> Outcome admits(Memory &memory,Frame &frame,std::uint32_t handle,bool &answer) {
    using enum Access;
    const auto light_address=memory.load32(data,at(memory.load32(Access::lights,globals::lights),datum_data))+light_size*(handle&0xffffu);
    answer=false;
    if(!(memory.load8(flags,at(light_address,light::flags))&portal_flag))return Outcome::unflagged;
    const auto intensity_value=memory.load32(intensity,at(light_address,light::intensity));
    const auto epsilon_value=memory.load32(Access::epsilon,globals::epsilon);
    const bool bright=!sse::not_above(intensity_value,epsilon_value) &&
        !sse::not_above(memory.load32(second_intensity,at(light_address,light::second_intensity)),epsilon_value);
    frame.measuring(0u);
    std::uint32_t threshold_value{};
    const auto length=squared(memory,frame,at(light_address,light::first_vector),&threshold_value);
    bool long_enough=!sse::not_above(length,threshold_value);
    if(!long_enough) {
        frame.measuring(1u);
        long_enough=!sse::not_above(squared(memory,frame,at(light_address,light::second_vector),nullptr),threshold_value);
    }
    frame.verdict(long_enough);
    answer=bright && long_enough;
    return answer?Outcome::admitted:Outcome::refused;
}
} // namespace light_admits
} // namespace halo2::native_light_admits
