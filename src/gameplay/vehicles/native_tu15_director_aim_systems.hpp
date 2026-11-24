#pragma once
// Native TU1.5 director aim, entered at 245060 (vehicle director unit): an
// aim record set up for a unit and a forward direction.
//
//   director_aim::set_up   245060(ECX = unit, EAX = forward*; record) -> RET 4
//
// The record's state (+0) and two scalars (+0x24, +0x88) clear, its +0x28
// takes [55D6E4], its offset (+0x10) the zero vector ([474D04]) and its
// forward (+0x2C) the given one; 11FA50 (through the dispatch: ECX the
// forward, EDX the up +0x38) completes the frame. With a unit, CD4A0 writes
// its position (+4) and BC200 its +0x44; a unit whose parent (+0x14) holds a
// weapon (BCDF0, 2) with a marker for its seat (the weapon's definition
// +0x1CC, 0xB0-byte entries by the unit's word +0x1FC, bit 7) takes the
// marker's frame (BAD60: position, forward, up); otherwise the parent's
// position (BBE00) and its node matrix (144BF0) turn the forward into the
// matrix's frame (each row against it: z, then y, then x), 11FA50 completes
// it again, and the forward and up return through the matrix's transpose.
// The state then becomes 1; 175380 (through the dispatch, EAX the record)
// finishes it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_director_aim.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them, and the matrix's
// up y and z it keeps in XMM7 and XMM6 across 11FA50, are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_director_aim {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t zero_vector=0x474d04u,reach=0x55d6e4u,object_headers=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,handle_mask=0xffffu,none=0xffffffffu;
inline constexpr std::uint32_t marker_name=0xf0000dbu,weapon_slot=2u;
namespace record {inline constexpr std::uint32_t state=0u,position=4u,offset=0x10u,first=0x24u,reach=0x28u,forward=0x2cu,up=0x38u,
    lights=0x44u,last=0x88u;}
namespace unit {inline constexpr std::uint32_t parent=0x14u,seat=0x1fcu;}
namespace weapon {inline constexpr std::uint32_t tag=0u,node=0x70u,node_extra=0x7cu;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu,seat_size=0xb0u;inline constexpr unsigned marked_bit=7u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame below the saved registers: the transposed z, the weapon, the
// unit, the node matrix (forward +0x1C, left +0x28, up +0x34), the parent's
// position, the marker's frame (forward +0x94, up +0xAC, position +0xB8).
namespace local {inline constexpr std::uint32_t z=0x10u,weapon=0x10u,unit=0x14u,matrix=0x18u,forward=0x1cu,left=0x28u,up=0x34u,
    position=0x4cu,marker=0x58u,marker_forward=0x94u,marker_up=0xacu,marker_position=0xb8u,record=0xccu,size=0xb8u;}

// The matrix's rows as read, and the original's last partial products.
struct Rows {std::uint32_t forward_x,forward_y,forward_z,left_x,left_y,left_z,up_x,up_y,up_z;};

namespace director_aim {
enum class Access {zero_load,record_argument,last_clear,state_clear,zero_x,offset_x,zero_y,offset_y,zero_z,offset_z,first_clear,forward_x_in,
    reach_load,forward_x_store,forward_y_in,forward_y_store,forward_z_in,forward_z_store,reach_store,headers_load,datums_load,unit_load,
    unit_store,unit_again,parent_load,weapon_store,weapon_tag,instances_load,definition_load,unit_reload,seat_load,seats_load,seat_flags,
    marker_load,marker_store,weapon_load,
    v_y,v_z,v_x,m_forward_z,m_forward_y,m_up_z,m_up_y,m_forward_x,m_left_y,t_x_store,m_left_z,m_left_x,t_y_store,m_up_x,t_z_store,
    w_y,w_z,w_x,n_up_x,n_left_x,n_forward_x,w_x_store,w_z_store,n_left_y,n_forward_y,n_left_z,w_y_store,n_forward_z,w_z_load,w_z_out,
    u_z,u_x,u_y,u_z_store,o_left_x,o_forward_x,u_z_load,u_x_store,o_left_y,o_left_z,u_y_store,u_z_out,state_set};
// The original's EBX (the unit, then its parent), EBP (the record), ESI (the position), EDI (the forward), EDX (the up as
// 11FA50 returns it) and XMM7/XMM6 (the matrix's up y and z).
struct Kept {std::uint32_t unit_handle,record_address,position,forward,up,up_y,up_z;};
// Frame: see native_tu15_director_aim.hpp.
template<class Memory,class Frame> void set_up(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The record's fields, its forward and the frame completing it.
    const auto zero=memory.load32(zero_load,globals::zero_vector);
    frame.framing();
    kept.record_address=memory.load32(record_argument,frame.local(local::record));
    memory.store32(last_clear,at(kept.record_address,record::last),frame.cleared());
    memory.store32(state_clear,at(kept.record_address,record::state),0u);
    frame.unit_kept(kept);
    frame.saving(1u);
    const auto offset=frame.offset_address(kept.record_address);
    memory.store32(offset_x,offset+x,memory.load32(zero_x,zero+x));
    memory.store32(offset_y,offset+y,memory.load32(zero_y,zero+y));
    memory.store32(offset_z,offset+z,memory.load32(zero_z,zero+z));
    memory.store32(first_clear,at(kept.record_address,record::first),frame.cleared());
    const auto forward_in=frame.forward_input();
    const auto fx=memory.load32(forward_x_in,forward_in+x);
    const auto reach=memory.load32(reach_load,globals::reach);
    frame.saving(2u);
    kept.forward=frame.forward_address(kept.record_address);
    memory.store32(forward_x_store,kept.forward+x,fx);
    memory.store32(forward_y_store,kept.forward+y,memory.load32(forward_y_in,forward_in+y));
    const auto fz=memory.load32(forward_z_in,forward_in+z);
    frame.up_address(kept.record_address);
    memory.store32(forward_z_store,kept.forward+z,fz);
    memory.store32(reach_store,at(kept.record_address,record::reach),reach);
    frame.completed(kept,0u);
    // The unit.
    if(frame.unit_compared(kept.unit_handle)) {
        const auto headers=memory.load32(headers_load,globals::object_headers);
        const auto datums=memory.load32(datums_load,at(headers,datum_data));
        const auto unit_address=memory.load32(unit_load,datums+header_size*frame.indexed(kept.unit_handle)+header_address);
        memory.store32(unit_store,frame.local(local::unit),unit_address);
        frame.positioned(kept);
        frame.lit(kept);
        const auto unit_now=memory.load32(unit_again,frame.local(local::unit));
        kept.unit_handle=memory.load32(parent_load,at(unit_now,unit::parent));
        if(frame.parent_compared(kept.unit_handle)) {
            const auto weapon_address=frame.armed(kept);
            memory.store32(weapon_store,frame.local(local::weapon),weapon_address);
            if(weapon_address) {
                const auto tag=memory.load32(weapon_tag,at(weapon_address,weapon::tag));
                const auto instances=memory.load32(instances_load,globals::tag_instances);
                const auto definition_address=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
                const auto unit_again_value=memory.load32(unit_reload,frame.local(local::unit));
                const auto seat=memory.load16(seat_load,at(unit_again_value,unit::seat));
                const auto seats=memory.load32(seats_load,at(definition_address,definition::seats));
                const auto flags=memory.load32(seat_flags,frame.seat_offset(seat)+seats);
                if(frame.marked(flags)) {
                    if(frame.marker_found(kept)) {
                        // The marker's frame: position, forward and up.
                        static constexpr std::uint32_t sources[]{local::marker_position,local::marker_position+4u,local::marker_position+8u,
                            local::marker_forward,local::marker_forward+4u,local::marker_forward+8u,local::marker_up,local::marker_up+4u,
                            local::marker_up+8u};
                        std::uint32_t values[9];
                        const auto copy=[&](unsigned k){values[k]=memory.load32(marker_load,frame.local(sources[k]));};
                        const auto put=[&](std::uint32_t address,unsigned k){memory.store32(marker_store,address,values[k]);};
                        copy(0);copy(1);copy(2);put(kept.position+x,0);copy(3);put(kept.position+y,1);copy(4);put(kept.position+z,2);
                        copy(5);put(kept.forward+x,3);copy(6);put(kept.forward+y,4);copy(7);put(kept.forward+z,5);copy(8);
                        put(at(kept.record_address,record::up+x),6);put(at(kept.record_address,record::up+y),7);
                        put(at(kept.record_address,record::up+z),8);
                    }
                } else {
                    // The node matrix: the forward into its frame, completed, then both back.
                    frame.parent_positioned(kept);
                    frame.node_matrix(memory.load32(weapon_load,frame.local(local::weapon)),kept);
                    Rows m{};
                    const auto vy=memory.load32(v_y,kept.forward+y),vz=memory.load32(v_z,kept.forward+z),vx=memory.load32(v_x,kept.forward+x);
                    m.forward_z=memory.load32(m_forward_z,frame.local(local::forward+z));
                    m.forward_y=memory.load32(m_forward_y,frame.local(local::forward+y));
                    m.up_z=memory.load32(m_up_z,frame.local(local::up+z));
                    m.up_y=memory.load32(m_up_y,frame.local(local::up+y));
                    const auto tx=sse::add(sse::add(sse::multiply(m.forward_z,vz),sse::multiply(m.forward_y,vy)),
                        sse::multiply(memory.load32(m_forward_x,frame.local(local::forward+x)),vx));
                    m.left_y=memory.load32(m_left_y,frame.local(local::left+y));
                    memory.store32(t_x_store,kept.forward+x,tx);
                    m.left_z=memory.load32(m_left_z,frame.local(local::left+z));
                    const auto ty_partial=sse::add(sse::multiply(m.left_z,vz),sse::multiply(m.left_y,vy));
                    m.left_x=memory.load32(m_left_x,frame.local(local::left+x));
                    const auto left_x_term=sse::multiply(m.left_x,vx);
                    memory.store32(t_y_store,kept.forward+y,sse::add(ty_partial,left_x_term));
                    const auto up_y_term=sse::multiply(m.up_y,vy);
                    const auto tz_partial=sse::add(sse::multiply(m.up_z,vz),up_y_term);
                    m.up_x=memory.load32(m_up_x,frame.local(local::up+x));
                    const auto up_x_term=sse::multiply(m.up_x,vx);
                    const auto tz=sse::add(tz_partial,up_x_term);
                    frame.rotated(vx,up_x_term,up_y_term,tz,left_x_term,m);
                    memory.store32(t_z_store,kept.forward+z,tz);
                    frame.completed(kept,1u);
                    // The forward back through the transpose.
                    const auto wy=memory.load32(w_y,kept.forward+y),wz=memory.load32(w_z,kept.forward+z),wx=memory.load32(w_x,kept.forward+x);
                    const auto up_x=memory.load32(n_up_x,frame.local(local::up+x));
                    const auto left_x=memory.load32(n_left_x,frame.local(local::left+x));
                    const auto rx_partial=sse::add(sse::multiply(up_x,wz),sse::multiply(left_x,wy));
                    const auto forward_x=memory.load32(n_forward_x,frame.local(local::forward+x));
                    memory.store32(w_x_store,kept.forward+x,sse::add(rx_partial,sse::multiply(forward_x,wx)));
                    memory.store32(w_z_store,frame.local(local::z),wz);
                    const auto up_y_kept=sse::multiply(kept.up_y,wz);
                    const auto left_y=memory.load32(n_left_y,frame.local(local::left+y));
                    const auto ry_partial=sse::add(up_y_kept,sse::multiply(left_y,wy));
                    const auto forward_y=memory.load32(n_forward_y,frame.local(local::forward+y));
                    const auto left_z=memory.load32(n_left_z,frame.local(local::left+z));
                    memory.store32(w_y_store,kept.forward+y,sse::add(ry_partial,sse::multiply(forward_y,wx)));
                    const auto left_z_term=sse::multiply(left_z,wy);
                    const auto forward_z=memory.load32(n_forward_z,frame.local(local::forward+z));
                    const auto rz=sse::add(sse::add(sse::multiply(kept.up_z,memory.load32(w_z_load,frame.local(local::z))),left_z_term),
                        sse::multiply(forward_z,wx));
                    memory.store32(w_z_out,kept.forward+z,rz);
                    // The up back through the transpose.
                    const auto uz=memory.load32(u_z,kept.up+z),ux=memory.load32(u_x,kept.up+x),uy=memory.load32(u_y,kept.up+y);
                    memory.store32(u_z_store,frame.local(local::z),uz);
                    const auto up_x_uz=sse::multiply(up_x,uz);
                    const auto ux_partial=sse::add(up_x_uz,sse::multiply(memory.load32(o_left_x,frame.local(local::left+x)),uy));
                    const auto out_x=sse::add(ux_partial,sse::multiply(memory.load32(o_forward_x,frame.local(local::forward+x)),ux));
                    const auto uz_again=memory.load32(u_z_load,frame.local(local::z));
                    memory.store32(u_x_store,kept.up+x,out_x);
                    const auto left_y_uy=sse::multiply(memory.load32(o_left_y,frame.local(local::left+y)),uy);
                    const auto out_y_partial=sse::add(sse::multiply(kept.up_y,uz_again),left_y_uy);
                    const auto left_z_uy=sse::multiply(memory.load32(o_left_z,frame.local(local::left+z)),uy);
                    const auto out_z_partial=sse::add(sse::multiply(kept.up_z,uz_again),left_z_uy);
                    const auto forward_y_ux=sse::multiply(forward_y,ux),forward_z_ux=sse::multiply(forward_z,ux);
                    const auto out_y=sse::add(out_y_partial,forward_y_ux),out_z=sse::add(out_z_partial,forward_z_ux);
                    frame.transposed(left_z_uy,forward_z_ux,forward_y_ux,ux,uy,uz_again,out_z,out_y);
                    memory.store32(u_y_store,kept.up+y,out_y);
                    memory.store32(u_z_out,kept.up+z,out_z);
                }
            }
        }
        memory.store32(state_set,at(kept.record_address,record::state),1u);
    }
    frame.finished(kept);
}
} // namespace director_aim
} // namespace halo2::native_director_aim
