#pragma once
// Native TU1.5 seated vector, entered at CDCF0 (physics live support unit): a
// seated player's vector turned into its vehicle's upright seat frame.
//
//   seated_vector::turn   CDCF0(EAX = object; vector*) -> AL, RET 4
//
// An object (through the object headers at [4ECBD0]) with a player (+0x13C
// not NONE) in a seat (word +0x1FC not NONE) of its parent (+0x14) whose seat
// (0xB0 bytes at the parent's definition +0x1CC, through the tag instances at
// 4F0414) does not have bit 4 of its flags (+0) set has its vector turned:
// the parent's up axis (BBFF0, with its forward, into the frame) crossed
// with the axis [474D18] points at, normalized by 30BD0 (its length in XMM0),
// or crossed with the axis [474D1C] points at when that length is exactly
// [46A0E8] (UCOMISS/LAHF); 144210 builds the frame's matrix from the up and
// side axes; the vector becomes the matrix's columns times its x, y and z,
// summed z, y, x. The answer is whether the vector was turned.
//
// The single-precision arithmetic is the original's scalar SSE, in its order
// (the first operand the destination), on the live MXCSR. This is the game
// logic only. The game's memory is the retained TU1.5 bank, reached through
// a Memory object with typed loads and stores; every access names its role
// (Access), including the original's frame (the axes, the side axis and the
// matrix); the boundary adapter (native_tu15_seated_vector.hpp) maps the
// role to the original instruction and owns the ABI of the children. The
// vector pointer the original keeps in EBP across them is a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_seated_vector {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,cross_axis=0x474d18u,
    fallback_axis=0x474d1cu,no_length=0x46a0e8u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,seat_size=0xb0u;
namespace object {inline constexpr std::uint32_t parent=0x14u,player=0x13cu,seat=0x1fcu;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu;}
namespace seat {inline constexpr std::uint32_t flags=0u;inline constexpr std::uint32_t unturned_bit=4u;}
// The matrix 144210 builds: its scale, then three columns of three floats and a translation.
namespace matrix {inline constexpr std::uint32_t side=4u,across=0x10u,up=0x1cu;}
// The frame's slots, by offset from the return address: the up and forward axes, the matrix (its first column the
// side axis).
namespace local {inline constexpr std::int32_t up=-0x4c,forward=-0x40,matrix=-0x34,side=-0x30;}

namespace seated_vector {
enum class Access {headers,datums,object_address,vector_pointer,player,seat,parent,parent_object,parent_tag,instances,definition,
    seats,seat_flags,axis_pointer,up_read,axis_read,side_store,no_length,vector_read,column_read,vector_store};
enum class Outcome {no_player,unseated,unturned,turned};
// The original's EBP (the vector pointer) across the children.
struct Kept {std::uint32_t vector;};
// Frame:
//   framing(object), saving(n), argument(), local(offset)   SUB ESP,4C (after the first loads); PUSH EBP, ESI, EBX, EDI
//   player_tested(player), seat_tested(seat), parent_indexed(parent), tag_indexed(tag), seat_scaled(seat),
//   flags_tested(flags)                          the original's tests and index arithmetic
//   axes(parent, kept)                           BBFF0(EAX = parent, ESI = forward, EDI = up), refreshes kept
//   crossed(pass, registers), normalized(pass, kept) -> XMM0   30BD0(EAX = the side axis), refreshes kept
//   length_tested(length, bound)                 UCOMISS; LAHF; TEST AH,44; POP EDI
//   matrix(kept)                                 144210(EAX = up, EDX = side, ECX = the matrix), refreshes kept
//   turned(registers)                            the transform's XMM0 to XMM4
template<class Memory,class Frame> std::uint32_t turn(Memory &memory,Frame &frame,std::uint32_t object_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.framing(object_handle);
    const auto object_value=memory.load32(object_address,datums_address+header_size*(object_handle&0xffffu)+header_address);
    frame.saving(0u);
    kept.vector=memory.load32(vector_pointer,frame.argument());
    frame.saving(1u);
    const auto player_value=memory.load32(player,at(object_value,object::player));
    frame.player_tested(player_value);
    if(player_value==none){outcome=Outcome::no_player;return 0u;}
    const auto seat_value=memory.load16(Access::seat,at(object_value,object::seat));
    frame.seat_tested(seat_value);
    if(seat_value==0xffffu){outcome=Outcome::unseated;return 0u;}
    frame.saving(2u);
    const auto parent_handle=memory.load32(parent,at(object_value,object::parent));
    frame.parent_indexed(parent_handle);
    const auto tag=memory.load32(parent_tag,memory.load32(parent_object,datums_address+header_size*(parent_handle&0xffffu)+header_address));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    frame.tag_indexed(tag);
    const auto seats_address=memory.load32(seats,at(memory.load32(definition,tag_size*(tag&0xffffu)+instances_address+tag_data),definition::seats));
    frame.seat_scaled(seat_value);
    const auto flags=memory.load32(seat_flags,seat_size*widen16(seat_value)+seats_address+seat::flags);
    frame.flags_tested(flags);
    if((flags>>seat::unturned_bit)&1u){outcome=Outcome::unturned;return 0u;}
    frame.saving(3u);
    frame.axes(parent_handle,kept);
    // The side axis: the up axis across a global axis, in the matrix's first column, normalized.
    const auto side=[&](unsigned pass) {
        const auto axis=memory.load32(axis_pointer,pass?globals::fallback_axis:globals::cross_axis);
        const auto up=frame.local(local::up);
        const auto up_z=memory.load32(up_read,up+8u);
        const auto axis_z=memory.load32(axis_read,axis+8u);
        const auto up_y=memory.load32(up_read,up+4u);
        const auto axis_y=memory.load32(axis_read,axis+4u);
        const auto x=sse::subtract(sse::multiply(axis_z,up_y),sse::multiply(axis_y,up_z));
        const auto axis_z_again=memory.load32(axis_read,axis+8u);
        memory.store32(side_store,frame.local(local::side),x);
        const auto axis_x=memory.load32(axis_read,axis);
        const auto up_x=memory.load32(up_read,frame.local(local::up));
        const auto x_term=sse::multiply(axis_z_again,up_x);
        const auto y=sse::subtract(sse::multiply(axis_x,up_z),x_term);
        memory.store32(side_store,frame.local(local::side)+4u,y);
        const auto axis_y_again=memory.load32(axis_read,axis+4u);
        const auto axis_x_again=memory.load32(axis_read,axis);
        const auto y_term=sse::multiply(axis_x_again,up_y);
        const auto z=sse::subtract(sse::multiply(axis_y_again,up_x),y_term);
        memory.store32(side_store,frame.local(local::side)+8u,z);
        frame.crossed(pass,up_y,y_term,z,x_term);
        return frame.normalized(pass,kept);
    };
    const auto length=side(0u);
    const auto bound=memory.load32(no_length,globals::no_length);
    frame.length_tested(length,bound);
    if(scalar::ordered_equal(length,bound))side(1u);
    frame.matrix(kept);
    // The vector turned: the columns times its x, y and z, summed z, y, x.
    const auto vector_y=memory.load32(vector_read,kept.vector+4u);
    const auto vector_z=memory.load32(vector_read,kept.vector+8u);
    const auto vector_x=memory.load32(vector_read,kept.vector);
    const auto column=[&](std::uint32_t offset,std::uint32_t word){return memory.load32(column_read,frame.local(local::matrix)+offset+4u*word);};
    const auto up_x=column(matrix::up,0u);
    const auto across_x=column(matrix::across,0u);
    auto sum=sse::add(sse::multiply(up_x,vector_z),sse::multiply(across_x,vector_y));
    const auto turned_x=sse::add(sum,sse::multiply(vector_x,column(matrix::side,0u)));
    const auto across_y=column(matrix::across,1u);
    memory.store32(vector_store,kept.vector,turned_x);
    sum=sse::add(sse::multiply(column(matrix::up,1u),vector_z),sse::multiply(across_y,vector_y));
    const auto side_y_term=sse::multiply(column(matrix::side,1u),vector_x);
    const auto turned_y=sse::add(sum,side_y_term);
    memory.store32(vector_store,kept.vector+4u,turned_y);
    const auto up_z_term=sse::multiply(column(matrix::up,2u),vector_z);
    const auto across_z_term=sse::multiply(column(matrix::across,2u),vector_y);
    const auto side_z_term=sse::multiply(column(matrix::side,2u),vector_x);
    const auto turned_z=sse::add(sse::add(up_z_term,across_z_term),side_z_term);
    memory.store32(vector_store,kept.vector+8u,turned_z);
    frame.turned(vector_x,side_z_term,across_z_term,turned_z,side_y_term);
    outcome=Outcome::turned;
    return 1u;
}
} // namespace seated_vector
} // namespace halo2::native_seated_vector
