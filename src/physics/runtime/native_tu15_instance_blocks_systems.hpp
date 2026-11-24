#pragma once
// Native TU1.5 instance blocks, entered at 16BBC0 (physics live support and
// death cycle units): whether an instanced geometry piece holds a point.
//
//   instance_blocks::query   16BBC0(EAX = instance, EBX = point*; flags) -> AL, RET 4
//
// The structure BSP's ([4ECC18]) instance (0x58 bytes at +0x144) names its
// definition (int16 +0x34; 0xC8 bytes at +0x13C). One that 16B810 (EDX the
// definition; the flags pushed) accepts and whose bounding sphere (centre
// +0x3C, radius +0x48; COMISS on the squares, the original's order) holds
// the point is turned to: 145200 (ECX the instance, ESI the point, EDX the
// frame) brings the point into the instance's space, and it holds the point
// when 14CDB0 (ECX the definition's BSP +0x70, EDX the frame, EAX 0) finds
// no cluster there.
//
// The single-precision arithmetic is the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads;
// every access names its role (Access); the boundary adapter
// (native_tu15_instance_blocks.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_instance_blocks {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t structure_bsp=0x4ecc18u;}
inline constexpr std::uint32_t instance_size=0x58u,definition_size=0xc8u;
namespace bsp {inline constexpr std::uint32_t definitions=0x13cu,instances=0x144u;}
namespace instance {inline constexpr std::uint32_t definition=0x34u,x=0x3cu,y=0x40u,z=0x44u,radius=0x48u;}
namespace definition {inline constexpr std::uint32_t bsp=0x70u;}
namespace argument {inline constexpr std::uint32_t flags=0u;}
// The frame's slot, by offset from the return address: the point in the instance's space.
namespace local {inline constexpr std::int32_t point=-0xc;}

namespace instance_blocks {
enum class Access {bsp,instances,definitions,flags_argument,definition,centre_x,point_x,centre_z,point_z,centre_y,point_y,radius};
enum class Outcome {refused,far,inside,holds};
// The original's EBX (the point), EBP (the definition) and EDI (the instance) across the children.
struct Kept {std::uint32_t point,definition,instance;};
// Frame: saving(), indexed(...), accepted(flags, kept) [16B810], sphere(...), transformed(kept) [145200],
// located(kept) [14CDB0], restoring(); each names the original instructions it stands for.
template<class Memory,class Frame> Outcome query(Memory &memory,Frame &frame,std::uint32_t instance_index,Kept kept) {
    using enum Access;
    frame.saving();
    const auto bsp_address=memory.load32(Access::bsp,globals::structure_bsp);
    const auto instances_address=memory.load32(instances,at(bsp_address,bsp::instances));
    const auto definitions_address=memory.load32(definitions,at(bsp_address,bsp::definitions));
    const auto flags=memory.load32(flags_argument,frame.argument(argument::flags));
    const auto scaled=frame.scaled(instance_index);
    const auto definition_index=widen16(memory.load16(Access::definition,scaled+instances_address+instance::definition));
    kept.instance=frame.instance(scaled,instances_address);
    kept.definition=frame.definition(definition_index,definitions_address);
    if(!(frame.accepted(flags,kept)&0xffu))return Outcome::refused;
    // The bounding sphere: the squares of the point's offsets against the radius's.
    const auto cx=memory.load32(centre_x,at(kept.instance,instance::x));
    const auto dx=sse::subtract(cx,memory.load32(point_x,kept.point));
    const auto cz=memory.load32(centre_z,at(kept.instance,instance::z));
    const auto dz=sse::subtract(cz,memory.load32(point_z,kept.point+8u));
    const auto cy=memory.load32(centre_y,at(kept.instance,instance::y));
    const auto dy=sse::subtract(cy,memory.load32(point_y,kept.point+4u));
    const auto r=memory.load32(radius,at(kept.instance,instance::radius));
    const auto distance=sse::add(sse::add(sse::multiply(dx,dx),sse::multiply(dz,dz)),sse::multiply(dy,dy));
    const auto reach=sse::multiply(r,r);
    frame.sphere(reach,dy,dz,r,distance);
    if(sse::below(reach,distance))return Outcome::far;
    frame.transformed(kept);
    return frame.located(kept)==none?Outcome::holds:Outcome::inside;
}
} // namespace instance_blocks
} // namespace halo2::native_instance_blocks
