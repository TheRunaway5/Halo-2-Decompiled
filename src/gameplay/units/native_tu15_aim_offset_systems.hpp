#pragma once
// Native TU1.5 aim offset, entered at E06E0 (physics live support unit): a
// unit's smoothed offset from its aiming node to its marker, in its own
// frame.
//
//   aim_offset::update   E06E0(EAX = unit)
//
// A unit (through the object headers at [4ECBD0]) with a parent (+0x13C not
// NONE) that 111F00 does not hold back (EAX the unit) finds its marker
// (BAD60: the unit, the name 0xB0000DD and 1 pushed, CL 0, EBX a marker
// frame on the stack; failing that the name 0x4000095). The marker's
// position (+0x60 of that frame) less the aiming node's position (+0x28 of
// the node at the unit's int16 offset +0x116) is the offset. CDF30 (EAX the
// unit; its +0x168 vector copied to the stack, and 0) and E05D0 (ESI the
// unit's +0x7C, EDX that vector, ECX and EDI two axes on the stack) give the
// unit's frame; the offset in it is (fy*dy + fx*dx, ly*dy + lx*dx, dz). The
// first time (byte +0x3C8 clear) it is taken as it is and the byte set;
// afterwards each axis of the kept offset (+0x3CC, +0x3D0, +0x3D4) moves
// toward it by at most the steps at 46A558 (low) and 46A18C (high).
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), including the
// original's frame; the boundary adapter (native_tu15_aim_offset.hpp) maps
// the role to the original instruction and owns the ABI of the children.
// The values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_aim_offset {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;
inline constexpr std::uint32_t step_low=0x46a558u,step_high=0x46a18cu;   // floats
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
// String ids: the marker the offset aims at, and its fallback.
inline constexpr std::uint32_t marker_name=0xb0000ddu,fallback_name=0x4000095u,marker_mode=1u;
namespace unit {inline constexpr std::uint32_t frame_source=0x7cu,node=0x116u,parent=0x13cu,facing=0x168u,started=0x3c8u,offset=0x3ccu;}
namespace node {inline constexpr std::uint32_t position=0x28u;}
namespace marker {inline constexpr std::uint32_t position=0x60u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t forward=-0x9c,offset=-0x90,facing=-0x84,left=-0x78,marker=-0x70;}

namespace aim_offset {
enum class Access {headers,datums,object_address,parent,node_offset,marker_x,node_x,facing_x,facing_y,facing_z,offset_x_store,
    marker_y,node_y,offset_y_store,marker_z,node_z,facing_x_store,facing_z_store,offset_z_store,facing_y_store,offset_y,forward_y,
    forward_x,offset_x,started,left_y,left_x,offset_z,local_x_store,local_y_store,local_z_store,kept_x,low,high,kept_x_again,
    kept_x_store,kept_y,kept_y_again,kept_y_store,kept_z,kept_z_again,kept_z_store,first_x,first_y,first_x_store,first_z,
    first_y_store,first_z_store,started_store};
enum class Outcome {no_parent,held,started,smoothed};
// The original's EBX (the marker frame), EBP (the unit's object), ESI (its node) and EDI (the unit) across the children.
struct Kept {std::uint32_t marker_frame,object,node,unit;};
// Frame:
//   local(offset)                           the frame's slots now
//   framing(unit), parent_tested(parent)    SUB ESP,9C; the saved registers; CMP [EBP+13C],-1
//   held(kept) -> AL                        111F00(EAX = unit), refreshes kept
//   saving(), restoring()                   EBX around the marker and the frame
//   marker(name, kept) -> AX                BAD60(the unit, name, 1; CL 0, EBX the marker frame, ESI the node), refreshes kept
//   facing(dz, kept), axes(kept)            CDF30(EAX = unit, XMM0 dz; the vector, 0) and E05D0(ESI, EDI, ECX, EDX), refresh kept
//   pushing(value)                          CDF30's arguments as the original pushes them, between its stores
//   local_offset(x, y, z, dx, dy)            the frame's MULSS/ADDSS results in XMM0..3
//   started_tested(started)                 TEST AL,AL
//   clamped(axis, delta, below_low, above_high, step), moved(axis, value)   one axis's SUBSS, COMISS and ADDSS
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.framing(kept.unit);
    kept.object=memory.load32(object_address,datums_address+header_size*(kept.unit&0xffffu)+header_address);
    const auto parent_value=memory.load32(parent,at(kept.object,unit::parent));
    frame.parent_tested(parent_value);
    if(parent_value==none)return Outcome::no_parent;
    if(frame.held(kept)&0xffu)return Outcome::held;
    frame.saving();
    kept.node=kept.object+widen16(memory.load16(node_offset,at(kept.object,unit::node)));
    kept.marker_frame=frame.local(local::marker);
    if(!(frame.marker(marker_name,kept)&0xffffu))frame.marker(fallback_name,kept);
    // The offset from the aiming node to the marker, and the unit's facing vector, on the stack.
    const auto marker_x_value=memory.load32(marker_x,frame.local(local::marker)+marker::position);
    const auto dx=sse::subtract(marker_x_value,memory.load32(node_x,at(kept.node,node::position)));
    const auto facing_x_value=memory.load32(facing_x,at(kept.object,unit::facing));
    const auto facing_y_value=memory.load32(facing_y,at(kept.object,unit::facing+4u));
    const auto facing_z_value=memory.load32(facing_z,at(kept.object,unit::facing+8u));
    memory.store32(offset_x_store,frame.local(local::offset),dx);
    const auto marker_y_value=memory.load32(marker_y,frame.local(local::marker)+marker::position+4u);
    const auto dy=sse::subtract(marker_y_value,memory.load32(node_y,at(kept.node,node::position+4u)));
    memory.store32(offset_y_store,frame.local(local::offset)+4u,dy);
    const auto marker_z_value=memory.load32(marker_z,frame.local(local::marker)+marker::position+8u);
    const auto dz=sse::subtract(marker_z_value,memory.load32(node_z,at(kept.node,node::position+8u)));
    memory.store32(facing_x_store,frame.local(local::facing),facing_x_value);
    frame.pushing(0u);
    memory.store32(facing_z_store,frame.local(local::facing)+8u,facing_z_value);
    frame.pushing(frame.local(local::facing));
    memory.store32(offset_z_store,frame.local(local::offset)+8u,dz);
    memory.store32(facing_y_store,frame.local(local::facing)+4u,facing_y_value);
    frame.facing(dz,kept);
    frame.axes(kept);
    // The offset in the unit's frame.
    const auto dy_value=memory.load32(offset_y,frame.local(local::offset)+4u);
    const auto forward_y_value=memory.load32(forward_y,frame.local(local::forward)+4u);
    const auto forward_x_value=memory.load32(forward_x,frame.local(local::forward));
    const auto dx_value=memory.load32(offset_x,frame.local(local::offset));
    const auto started_value=memory.load8(started,at(kept.object,unit::started));
    const auto x_forward=sse::multiply(forward_y_value,dy_value);
    const auto x_side=sse::multiply(forward_x_value,dx_value);
    const auto x=sse::add(x_forward,x_side);
    const auto y_forward=sse::multiply(memory.load32(left_y,frame.local(local::left)+4u),dy_value);
    const auto y_side=sse::multiply(memory.load32(left_x,frame.local(local::left)),dx_value);
    const auto y=sse::add(y_forward,y_side);
    const auto z=memory.load32(offset_z,frame.local(local::offset)+8u);
    frame.local_offset(x,y,z,dx_value,y_side);
    memory.store32(local_x_store,frame.local(local::forward),x);
    memory.store32(local_y_store,frame.local(local::forward)+4u,y);
    memory.store32(local_z_store,frame.local(local::forward)+8u,z);
    frame.restoring();
    frame.started_tested(started_value);
    if(!started_value) {
        const auto first=memory.load32(first_x,frame.local(local::forward));
        const auto second=memory.load32(first_y,frame.local(local::forward)+4u);
        memory.store32(first_x_store,at(kept.object,unit::offset),first);
        const auto third=memory.load32(first_z,frame.local(local::forward)+8u);
        memory.store32(first_y_store,at(kept.object,unit::offset+4u),second);
        memory.store32(first_z_store,at(kept.object,unit::offset+8u),third);
        memory.store8(started_store,at(kept.object,unit::started),1u);
        return Outcome::started;
    }
    // Each axis moves toward the new offset by at most a step.
    const auto clamp=[&](unsigned axis,std::uint32_t delta,std::uint32_t low_step,std::uint32_t high_step) {
        const bool below_low=!sse::not_above(low_step,delta);
        const bool above_high=!below_low && !sse::not_above(delta,high_step);
        const auto step=below_low?low_step:above_high?high_step:delta;
        frame.clamped(axis,delta,below_low,above_high,step);
        return step;
    };
    const auto delta_x=sse::subtract(x,memory.load32(kept_x,at(kept.object,unit::offset)));
    const auto low_value=memory.load32(low,globals::step_low);
    const bool x_below=!sse::not_above(low_value,delta_x);
    const auto high_value=memory.load32(high,globals::step_high);
    const bool x_above=!x_below && !sse::not_above(delta_x,high_value);
    const auto step_x=x_below?low_value:x_above?high_value:delta_x;
    frame.clamped(0u,delta_x,x_below,x_above,step_x);
    const auto new_x=sse::add(memory.load32(kept_x_again,at(kept.object,unit::offset)),step_x);
    frame.moved(0u,new_x);
    memory.store32(kept_x_store,at(kept.object,unit::offset),new_x);
    const auto step_y=clamp(1u,sse::subtract(y,memory.load32(kept_y,at(kept.object,unit::offset+4u))),low_value,high_value);
    const auto new_y=sse::add(memory.load32(kept_y_again,at(kept.object,unit::offset+4u)),step_y);
    frame.moved(1u,new_y);
    memory.store32(kept_y_store,at(kept.object,unit::offset+4u),new_y);
    const auto step_z=clamp(2u,sse::subtract(z,memory.load32(kept_z,at(kept.object,unit::offset+8u))),low_value,high_value);
    const auto new_z=sse::add(step_z,memory.load32(kept_z_again,at(kept.object,unit::offset+8u)));
    frame.moved(2u,new_z);
    memory.store32(kept_z_store,at(kept.object,unit::offset+8u),new_z);
    return Outcome::smoothed;
}
} // namespace aim_offset
} // namespace halo2::native_aim_offset
