#pragma once
// Native TU1.5 unit view, entered at 244F80 (combat action unit): where a
// unit views from and toward.
//
//   unit_view::locate   244F80(ECX = unit; position*, forward*)
//
// CD4A0 writes the unit's eye position; the forward is the unit's aim
// (+0x168). A unit in a vehicle (+0x14, BCDF0 kind 2) whose seat (the
// vehicle definition's 0xB0-byte seats +0x1CC, by the unit's int16 +0x1FC)
// carries flag bit 7 views instead from the vehicle's marker F0000DB, when
// BAD60 finds it: the position and the forward of its matrix (+0x60, +0x3C
// of the marker's record).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments and the marker's record
// on the original's frame; the boundary adapter (native_tu15_unit_view.hpp)
// maps the role to the original instruction and owns the ABI of the children
// (CD4A0, BCDF0, BAD60). The values the original keeps in callee-saved
// registers across them are a Kept, refreshed from the registers each child
// returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_view {
using native_object_update::at;
using native_object_update::widen16;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u;
namespace unit {inline constexpr std::uint32_t vehicle=0x14u,aim=0x168u,seat=0x1fcu;}
inline constexpr std::uint32_t vehicle_kind=2u;
namespace definition {inline constexpr std::uint32_t seats=0x1ccu;}
inline constexpr std::uint32_t seat_size=0xb0u;
namespace seat {inline constexpr std::uint32_t marker_view=1u<<7;}
inline constexpr std::uint32_t view_marker=0xf0000dbu;
namespace marker {inline constexpr std::uint32_t forward=0x3cu,position=0x60u;}

namespace unit_view {
enum class Access {objects,object_data,forward_argument,object_address,position_argument,aim_x,aim_x_store,aim_y,aim_y_store,
    aim_z,aim_z_store,vehicle,vehicle_tag,instances,vehicle_definition,seat_index,seats,seat_flags,position_output,
    marker_x,marker_y,position_x,marker_z,position_y,marker_forward_x,position_z,marker_forward_y,marker_forward_z,
    forward_x,forward_y,forward_z};
enum class Outcome {unseated,no_vehicle,own_view,no_marker,marker_view};
// The original's ESI, EDI and EBP across the children: the unit's object,
// its vehicle and the forward's address.
struct Kept {std::uint32_t object,vehicle,forward;};
// Frame:
//   argument(k), marker_record(offset)   an argument's address, the marker record's field, now
//   eye(position, unit, kept)            CD4A0(EAX the position, ECX the unit), refreshes kept
//   find(vehicle, kind, kept) -> object  BCDF0(EAX the vehicle; kind pushed), refreshes kept
//   marker(vehicle, name, kept) -> found BAD60(EBX the record, CL 0; vehicle, name, 1), refreshes kept
template<class Memory,class Frame> Outcome locate(Memory &memory,Frame &frame,std::uint32_t unit_handle) {
    using enum Access;
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    Kept kept{};
    kept.forward=memory.load32(forward_argument,frame.argument(1u));
    kept.object=memory.load32(object_address,data+header_size*(unit_handle&0xffffu)+header_address);
    frame.eye(memory.load32(position_argument,frame.argument(0u)),unit_handle,kept);
    const auto aim=at(kept.object,unit::aim);
    memory.store32(aim_x_store,at(kept.forward,0u),memory.load32(aim_x,at(aim,0u)));
    memory.store32(aim_y_store,at(kept.forward,4u),memory.load32(aim_y,at(aim,4u)));
    memory.store32(aim_z_store,at(kept.forward,8u),memory.load32(aim_z,at(aim,8u)));
    kept.vehicle=memory.load32(vehicle,at(kept.object,unit::vehicle));
    if(kept.vehicle==none)return Outcome::unseated;
    const auto vehicle_object=frame.find(kept.vehicle,vehicle_kind,kept);
    if(!vehicle_object)return Outcome::no_vehicle;
    const auto tag=memory.load32(vehicle_tag,vehicle_object);
    const auto definition_address=memory.load32(vehicle_definition,
        tag_size*(tag&0xffffu)+memory.load32(instances,globals::tag_instances)+tag_data);
    const auto index=widen16(memory.load16(seat_index,at(kept.object,unit::seat)));
    const auto seats_address=memory.load32(seats,at(definition_address,definition::seats));
    if(!(memory.load32(seat_flags,seat_size*index+seats_address)&seat::marker_view))return Outcome::own_view;
    if(!frame.marker(kept.vehicle,view_marker,kept))return Outcome::no_marker;
    // The marker's position and forward, in the original's order.
    const auto position_address=memory.load32(position_output,frame.argument(0u));
    const auto x=memory.load32(marker_x,frame.marker_record(marker::position));
    const auto y=memory.load32(marker_y,frame.marker_record(marker::position+4u));
    memory.store32(position_x,at(position_address,0u),x);
    const auto z=memory.load32(marker_z,frame.marker_record(marker::position+8u));
    memory.store32(position_y,at(position_address,4u),y);
    const auto forward_x_value=memory.load32(marker_forward_x,frame.marker_record(marker::forward));
    memory.store32(position_z,at(position_address,8u),z);
    const auto forward_y_value=memory.load32(marker_forward_y,frame.marker_record(marker::forward+4u));
    const auto forward_z_value=memory.load32(marker_forward_z,frame.marker_record(marker::forward+8u));
    memory.store32(forward_x,at(kept.forward,0u),forward_x_value);
    memory.store32(forward_y,at(kept.forward,4u),forward_y_value);
    memory.store32(forward_z,at(kept.forward,8u),forward_z_value);
    return Outcome::marker_view;
}
} // namespace unit_view
} // namespace halo2::native_unit_view
