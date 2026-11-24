#pragma once
// Native TU1.5 unit camera position, entered at CD4A0 (physics live
// support, AI frame and sandbox special units): where a unit's camera is.
//
//   unit_camera_position::position   CD4A0(ECX = unit handle, EAX = out*)
//
// A seated unit (a parent, +0x14) takes BBF20's position (ECX the output)
// and, in a seat (the word +0x1FC) with a camera marker (+0x60 of the seat,
// 0xB0-byte entries at its parent's definition +0x1CC; a vehicle, type 1,
// needs one named), that marker's position on the parent through BAD60.
// A unit that is not a biped (type +0xAA) or has +0x10A bit 2 takes the
// marker of the seat it controls (the object +0x24C, its seat +0x1FC, the
// unit's own seats) or the head (0x4000095) on itself. A biped's camera is
// its position (BBE00) raised by its eye height: the definition's standing
// (+0x218) and crouching (+0x21C) heights blended by its crouch (+0x2C4,
// none when E3310 answers), times its scale (+0xA0). E18D0 then moves a copy
// of it along the unit's aim (+0x168) and 11FA50's up; when that moves it
// further than the epsilon ([46A0E8], through 30BD0), a ray (16C620, flags
// 0x4808C2D) from the eye along the move scaled by [4502C8] ends where it
// hits, pulled back by [46A194], or at its end; a second ray from there
// along the move that hits a plane the moved camera is more than the margin
// ([46A12C]) beyond places the camera that margin before the hit, and
// otherwise the moved camera is the camera.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_unit_camera_position.hpp) maps the role to
// the original instruction, owns the ABI of the children and leaves through
// the original's epilogue for each exit. The values the original keeps in
// callee-saved registers across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_camera_position {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,one=0x46a0d0u,epsilon=0x46a0e8u,
    reach=0x4502c8u,back_off=0x46a194u,margin=0x46a12cu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_data=8u,handle_mask=0xffffu,none=0xffffffffu,
    seat_size=0xb0u,head_marker=0x4000095u,ray_flags=0x4808c2du,vehicle=1u,no_material=0xffffu;
namespace unit {inline constexpr std::uint32_t tag=0u,parent=0x14u,type=0xaau,scale=0xa0u,object_flags=0x10au,aim=0x168u,
    seat=0x1fcu,controlled=0x24cu,crouch=0x2c4u;inline constexpr unsigned controlled_bit=2u;}
namespace definition {inline constexpr std::uint32_t standing_height=0x218u,crouching_height=0x21cu,seats=0x1ccu;}
namespace seat {inline constexpr std::uint32_t marker=8u,camera=0x60u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame below the saved registers (offsets from the ESP after all four
// are pushed): the moved camera, the ray (first the aim), its end, the move,
// the unit's definition, the ray result (its distance +4, its hit +8, its
// material +0x24) or the marker (its position +0x60).
namespace local {inline constexpr std::uint32_t moved=0x10u,ray=0x1cu,end=0x28u,move=0x34u,definition=0x40u,result=0x44u,
    distance=0x48u,hit=0x4cu,material=0x68u,marker=0x44u,marker_position=0xa4u,size=0xa4u;}

namespace unit_camera_position {
enum class Access {headers_load,datums_load,unit_load,unit_tag,instances_load,definition_load,definition_store,parent_load,
    object_flags,object_type,biped_tag,biped_definition,crouch_load,one_load,crouching_load,standing_load,scale_load,eye_z,
    eye_z_store,eye_x_copy,eye_y_copy,eye_z_copy,moved_x_store,aim_x,moved_y_store,aim_y,moved_z_store,aim_z,ray_x_store,
    ray_z_store,material_store,ray_y_store,moved_x,eye_x_sub,move_x_store,moved_y,eye_y_sub,move_y_store,moved_z,eye_z_sub,
    move_z_store,epsilon_load,reach_load,move_x,scaled_x_store,move_y,scaled_y_store,move_z,scaled_z_store,distance_load,
    back_off_load,scaled_x,eye_x_add,end_x_store,scaled_y,eye_y_add,end_y_store,scaled_z,eye_z_add,end_z_store,hit_x_copy,
    hit_y_copy,hit_z_copy,end_x_copy,end_y_copy,end_z_copy,end_x,hit_x,end_y,end_z,moved_x_again,hit_y,hit_z,moved_y_again,
    moved_z_again,move_y_again,move_z_again,move_x_again,margin_load,pulled_x,out_x_store,pulled_y,out_y_store,pulled_z,
    out_z_store,moved_x_final,moved_y_final,moved_z_final,out_moved_x,out_moved_y,out_moved_z,controlled_load,controlled_object,
    controlled_seat,seat_definition,seats_load,seat_marker,marker_x,marker_y,marker_z,out_marker_x,out_marker_y,out_marker_z,
    seat_word,parent_reload,headers_again,datums_again,instances_again,parent_object,parent_tag,parent_definition,parent_seats,
    parent_type,camera_test,camera_marker,camera_x,camera_y,camera_z,out_camera_x,out_camera_y,out_camera_z};
enum class Exit {pulled,moved,marker,unchanged};
// Who pushes BAD60's arguments: the controlled seat's marker, the head, or
// the seat's camera on the parent.
enum class Marker {seat,head,camera};
// The original's EBX (the unit's handle), EBP (the datums, later the
// biped's definition), ESI (the output) and EDI (the unit).
struct Kept {std::uint32_t handle,base,output,unit_address;};
// The registers the plane test leaves, for the exits after it.
struct Plane {std::uint32_t hit_along,camera_along,margin,dx,px,py,dz,dy;};

// The moved camera becomes the camera.
template<class Memory,class Frame> Exit place_moved(Memory &memory,Frame &frame,const Kept &kept) {
    using enum Access;
    const auto mx=memory.load32(moved_x_final,frame.local(local::moved+x));
    const auto my=memory.load32(moved_y_final,frame.local(local::moved+y));
    const auto mz=memory.load32(moved_z_final,frame.local(local::moved+z));
    memory.store32(out_moved_x,kept.output+x,mx);
    frame.leaving_moved();
    memory.store32(out_moved_y,kept.output+y,my);
    memory.store32(out_moved_z,kept.output+z,mz);
    return Exit::moved;
}

// Frame: see native_tu15_unit_camera_position.hpp.
template<class Memory,class Frame> Exit position(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The unit and its definition.
    frame.framing();
    frame.saving(1u);frame.saving(2u);frame.saving(3u);
    kept.output=frame.output();
    const auto headers=memory.load32(headers_load,globals::object_headers);
    kept.base=memory.load32(datums_load,at(headers,datum_data));
    kept.handle=frame.handle();
    const auto index=frame.indexed(kept.handle);
    frame.saving(4u);
    kept.unit_address=memory.load32(unit_load,kept.base+header_size*index+header_address);
    const auto tag=memory.load32(unit_tag,at(kept.unit_address,unit::tag));
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    const auto definition_address=memory.load32(definition_load,frame.tag_offset_edx(tag)+instances+tag_data);
    memory.store32(definition_store,frame.local(local::definition),definition_address);
    const auto parent=memory.load32(parent_load,at(kept.unit_address,unit::parent));
    if(frame.seated(parent)) {
        // A seated unit: BBF20, then the seat's camera marker on the parent.
        frame.seated_position(kept);
        const auto seat_index=memory.load16(seat_word,at(kept.unit_address,unit::seat));
        if(!frame.in_seat(seat_index))return Exit::unchanged;
        const auto parent_handle=memory.load32(parent_reload,at(kept.unit_address,unit::parent));
        const auto headers_now=memory.load32(headers_again,globals::object_headers);
        const auto datums_now=memory.load32(datums_again,at(headers_now,datum_data));
        const auto instances_now=memory.load32(instances_again,globals::tag_instances);
        const auto parent_object_address=memory.load32(parent_object,datums_now+4u*frame.parent_index(parent_handle)+header_address);
        const auto parent_tag_value=memory.load32(parent_tag,at(parent_object_address,unit::tag));
        const auto parent_definition_address=memory.load32(parent_definition,frame.tag_offset_eax(parent_tag_value)+instances_now+tag_data);
        const auto seats=memory.load32(parent_seats,at(parent_definition_address,definition::seats));
        frame.seat_widened(seat_index);
        const auto type=memory.load8(parent_type,at(parent_object_address,unit::type));
        const auto seat_entry=frame.seat_entry(seat_index,seats);
        if(frame.vehicle(type) && !frame.named(memory.load32(camera_test,at(seat_entry,seat::camera))))return Exit::unchanged;
        frame.marker_query(kept,memory.load32(camera_marker,at(seat_entry,seat::camera)),parent_handle,Marker::camera);
        const auto cx=memory.load32(camera_x,frame.local(local::marker_position+x));
        const auto cy=memory.load32(camera_y,frame.local(local::marker_position+y));
        const auto cz=memory.load32(camera_z,frame.local(local::marker_position+z));
        memory.store32(out_camera_x,kept.output+x,cx);
        memory.store32(out_camera_y,kept.output+y,cy);
        memory.store32(out_camera_z,kept.output+z,cz);
        return Exit::unchanged;
    }
    if(frame.controlling(memory.load8(object_flags,at(kept.unit_address,unit::object_flags))) ||
       frame.not_biped(memory.load8(object_type,at(kept.unit_address,unit::type)))) {
        // The marker of the seat it controls, or its head.
        frame.parent_tested(parent);
        const auto controlled=memory.load32(controlled_load,at(kept.unit_address,unit::controlled));
        std::uint32_t marker_name=head_marker;
        auto source=Marker::head;
        if(frame.different(controlled,parent)) {
            const auto object=memory.load32(controlled_object,kept.base+4u*frame.controlled_index(controlled)+header_address);
            const auto seat_index=memory.load16(controlled_seat,at(object,unit::seat));
            if(frame.in_seat_word(seat_index)) {
                frame.seat_widened_edx(seat_index);
                const auto own_definition=memory.load32(seat_definition,frame.local(local::definition));
                const auto offset=frame.seat_offset(seat_index);
                const auto seats=memory.load32(seats_load,at(own_definition,definition::seats));
                marker_name=memory.load32(seat_marker,offset+seats+seat::marker);
                source=Marker::seat;
            }
        }
        frame.marker_query(kept,marker_name,kept.handle,source);
        const auto mx=memory.load32(marker_x,frame.local(local::marker_position+x));
        const auto my=memory.load32(marker_y,frame.local(local::marker_position+y));
        const auto mz=memory.load32(marker_z,frame.local(local::marker_position+z));
        memory.store32(out_marker_x,kept.output+x,mx);
        frame.leaving_marker();
        memory.store32(out_marker_y,kept.output+y,my);
        memory.store32(out_marker_z,kept.output+z,mz);
        return Exit::marker;
    }
    // A biped: its position raised by its eye height.
    const auto biped_tag_value=memory.load32(biped_tag,at(kept.unit_address,unit::tag));
    kept.base=memory.load32(biped_definition,frame.tag_offset_eax(biped_tag_value)+instances+tag_data);
    frame.biped_position(kept);
    std::uint32_t crouch=0u;
    if(!frame.uncrouched(kept))crouch=memory.load32(crouch_load,at(kept.unit_address,unit::crouch));
    const auto one=memory.load32(one_load,globals::one);
    const auto crouching=memory.load32(crouching_load,at(kept.base,definition::crouching_height));
    auto height=sse::subtract(one,crouch);
    height=sse::multiply(height,memory.load32(standing_load,at(kept.base,definition::standing_height)));
    const auto crouched=sse::multiply(crouching,crouch);
    height=sse::add(height,crouched);
    height=sse::multiply(height,memory.load32(scale_load,at(kept.unit_address,unit::scale)));
    height=sse::add(height,memory.load32(eye_z,kept.output+z));
    memory.store32(eye_z_store,kept.output+z,height);
    frame.eye_registers(crouch,height,crouched);
    // E18D0 moves a copy of it along the aim and 11FA50's up.
    frame.eye_copying(kept);
    const auto ex=memory.load32(eye_x_copy,kept.output+x);
    const auto ey=memory.load32(eye_y_copy,kept.output+y);
    const auto ez=memory.load32(eye_z_copy,kept.output+z);
    const auto aim=frame.aim_address(kept);
    memory.store32(moved_x_store,frame.local(local::moved+x),ex);
    const auto ax=memory.load32(aim_x,aim+x);
    memory.store32(moved_y_store,frame.local(local::moved+y),ey);
    const auto ay=memory.load32(aim_y,aim+y);
    memory.store32(moved_z_store,frame.local(local::moved+z),ez);
    const auto az=memory.load32(aim_z,aim+z);
    memory.store32(ray_x_store,frame.local(local::ray+x),ax);
    memory.store32(ray_z_store,frame.local(local::ray+z),az);
    frame.aim_arguments();
    memory.store16(material_store,frame.local(local::material),no_material);
    memory.store32(ray_y_store,frame.local(local::ray+y),ay);
    frame.move_camera(kept);
    // The move, and whether it is longer than the epsilon.
    const auto mx_load=memory.load32(moved_x,frame.local(local::moved+x));
    memory.store32(move_x_store,frame.local(local::move+x),sse::subtract(mx_load,memory.load32(eye_x_sub,kept.output+x)));
    const auto my_load=memory.load32(moved_y,frame.local(local::moved+y));
    memory.store32(move_y_store,frame.local(local::move+y),sse::subtract(my_load,memory.load32(eye_y_sub,kept.output+y)));
    const auto mz_load=memory.load32(moved_z,frame.local(local::moved+z));
    const auto move_z_value=sse::subtract(mz_load,memory.load32(eye_z_sub,kept.output+z));
    frame.move_address();
    memory.store32(move_z_store,frame.local(local::move+z),move_z_value);
    const auto length=frame.measured(kept,move_z_value);
    if(!frame.beyond(length,memory.load32(epsilon_load,globals::epsilon)))return Exit::unchanged;
    // The first ray: from the eye along the move scaled by the reach.
    const auto reach=memory.load32(reach_load,globals::reach);
    const auto dx=memory.load32(move_x,frame.local(local::move+x));
    frame.ray_result_pushed();
    const auto sx=sse::multiply(dx,reach);
    memory.store32(scaled_x_store,frame.local(local::ray+x),sx);
    const auto dy=memory.load32(move_y,frame.local(local::move+y));
    frame.ray_pushed();
    const auto sy=sse::multiply(dy,reach);
    memory.store32(scaled_y_store,frame.local(local::ray+y),sy);
    const auto dz=memory.load32(move_z,frame.local(local::move+z));
    frame.origin_pushed(kept);
    const auto sz=sse::multiply(dz,reach);
    memory.store32(scaled_z_store,frame.local(local::ray+z),sz);
    if(frame.cast(kept,false,reach,sz)) {
        const auto distance=memory.load32(distance_load,frame.local(local::distance));
        const auto t=sse::multiply(distance,memory.load32(back_off_load,globals::back_off));
        static constexpr Access scaled_loads[]{scaled_x,scaled_y,scaled_z},adds[]{eye_x_add,eye_y_add,eye_z_add},
            stores[]{end_x_store,end_y_store,end_z_store};
        std::uint32_t end_value=0u;
        for(std::uint32_t k=0;k<3u;++k) {
            const auto scaled=sse::multiply(memory.load32(scaled_loads[k],frame.local(local::ray+4u*k)),t);
            end_value=sse::add(scaled,memory.load32(adds[k],kept.output+4u*k));
            memory.store32(stores[k],frame.local(local::end+4u*k),end_value);
        }
        frame.ended(t,end_value);
    } else {
        const auto hx=memory.load32(hit_x_copy,frame.local(local::hit+x));
        const auto hy=memory.load32(hit_y_copy,frame.local(local::hit+y));
        const auto hz=memory.load32(hit_z_copy,frame.local(local::hit+z));
        memory.store32(end_x_copy,frame.local(local::end+x),hx);
        memory.store32(end_y_copy,frame.local(local::end+y),hy);
        memory.store32(end_z_copy,frame.local(local::end+z),hz);
    }
    // The second ray: from its end along the move.
    frame.second_ray_pushed();
    if(!frame.cast(kept,true,0u,0u))return place_moved(memory,frame,kept);
    const auto qx=memory.load32(end_x,frame.local(local::end+x));
    const auto hx=memory.load32(hit_x,frame.local(local::hit+x));
    const auto qy=memory.load32(end_y,frame.local(local::end+y));
    const auto qz=memory.load32(end_z,frame.local(local::end+z));
    const auto px_load=memory.load32(moved_x_again,frame.local(local::moved+x));
    const auto hy=memory.load32(hit_y,frame.local(local::hit+y));
    const auto hz=memory.load32(hit_z,frame.local(local::hit+z));
    const auto py_load=memory.load32(moved_y_again,frame.local(local::moved+y));
    const auto hx_rel=sse::subtract(hx,qx),px_rel=sse::subtract(px_load,qx);
    const auto pz_load=memory.load32(moved_z_again,frame.local(local::moved+z));
    const auto pz_rel=sse::subtract(pz_load,qz),hz_rel=sse::subtract(hz,qz);
    Plane plane{};
    plane.dy=memory.load32(move_y_again,frame.local(local::move+y));
    const auto hy_rel=sse::subtract(hy,qy),py_rel=sse::subtract(py_load,qy);
    plane.dz=memory.load32(move_z_again,frame.local(local::move+z));
    auto hit_along=sse::add(sse::multiply(hz_rel,plane.dz),sse::multiply(hy_rel,plane.dy));
    plane.dx=memory.load32(move_x_again,frame.local(local::move+x));
    const auto pz_term=sse::multiply(pz_rel,plane.dz);
    hit_along=sse::add(hit_along,sse::multiply(hx_rel,plane.dx));
    plane.margin=memory.load32(margin_load,globals::margin);
    plane.py=sse::multiply(py_rel,plane.dy);
    auto camera_along=sse::add(pz_term,plane.py);
    plane.px=sse::multiply(px_rel,plane.dx);
    camera_along=sse::add(sse::add(camera_along,plane.px),plane.margin);
    plane.hit_along=hit_along;plane.camera_along=camera_along;
    if(!frame.past(camera_along,hit_along,plane))return place_moved(memory,frame,kept);
    // Pulled back by the margin before the hit.
    const auto hx_load=memory.load32(pulled_x,frame.local(local::hit+x));
    const auto dx_margin=sse::multiply(plane.dx,plane.margin);
    memory.store32(out_x_store,kept.output+x,sse::subtract(hx_load,dx_margin));
    const auto hy_load=memory.load32(pulled_y,frame.local(local::hit+y));
    const auto dy_margin=sse::multiply(plane.dy,plane.margin);
    memory.store32(out_y_store,kept.output+y,sse::subtract(hy_load,dy_margin));
    const auto hz_load=memory.load32(pulled_z,frame.local(local::hit+z));
    frame.leaving_pulled();
    const auto dz_margin=sse::multiply(plane.dz,plane.margin);
    const auto out_z=sse::subtract(hz_load,dz_margin);
    frame.pulled(plane,out_z,dx_margin,dy_margin,dz_margin);
    memory.store32(out_z_store,kept.output+z,out_z);
    return Exit::pulled;
}
} // namespace unit_camera_position
} // namespace halo2::native_unit_camera_position
