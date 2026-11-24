#pragma once
// Native TU1.5 platform carry, entered at 1F8710 (physics live support
// unit): a character carried by the surface it stands on.
//
//   platform_carry::carry   1F8710(ground, body, character) -> RET C
//
// The ground record's handles +0x1C, +0x5C and +0x20 become NONE. The
// displacement starts as the character's previous position (+0x124), the
// surface offset as the zero vector ([474D04]) and the rotation as the
// identity ([474D30] + 4, nine floats). The character's surface (its word
// +0x18 among the 0xA0-byte entries of [52BE5C]) has an object (+0x70)
// that is moving when its byte +0x44 is set. 1FA110 looks for an attached
// object (EAX the character, ECX the ground; the attached position, offset
// and the rotation pushed; AL); without one, 1FA1A0 (EAX the character; the
// ground, the surface offset and the rotation pushed; AL) says whether the
// character is carried. With an attached object, its position becomes the
// surface offset, the body is flagged (+0 bit 3) and takes the rotation
// (+0x30).
//  * On a moving surface the body's motion (+0x64) becomes the surface
//    object's position (+0x30) and the displacement loses it.
//  * Otherwise, when carried or attached, the motion becomes the surface
//    offset, the displacement loses it, the body takes the rotation and,
//    attached with the ground's ticks (+0x62) above 1, the displacement
//    gains the attached offset.
// 1F8B30 then gets the ground and the displacement (ESI the character, EDI
// the body; RET 8). Afterwards a moving surface adds the motion to the
// body's position (+0xC); a carried or attached character adds the surface
// offset and turns the body's forward (+0x18) and up (+0x24) through the
// rotation. The ground's ticks count up to 0xFE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_platform_carry.hpp) maps the role to the
// original instruction and owns the ABI of the children. The surface entry
// and the body the original keeps in EBX and EBP across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_platform_carry {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t identity=0x474d30u,zero_vector=0x474d04u,surfaces=0x52be5cu;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu,surface_size=0xa0u,rotation_words=9u,
    identity_rotation=4u,tick_limit=0xfeu;
namespace character {inline constexpr std::uint32_t surface=0x18u,previous=0x124u;}
namespace ground {inline constexpr std::uint32_t first=0x1cu,second=0x5cu,third=0x20u,ticks=0x62u;}
namespace surface {inline constexpr std::uint32_t object=0x70u;}
namespace surface_object {inline constexpr std::uint32_t position=0x30u,moving=0x44u;}
namespace body {inline constexpr std::uint32_t flags=0u,position=0xcu,forward=0x18u,up=0x24u,rotation=0x30u,motion=0x64u,
    attached_flag=8u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame below the saved registers (offsets from the ESP after all four
// are pushed): whether an object is attached, whether the surface moves,
// the displacement, the surface offset, the attached position and offset
// (later a copy of the vector turned), the rotation's three rows, and the
// three arguments (the body's slot later holds whether it is carried).
namespace local {inline constexpr std::uint32_t found=0x12u,moving=0x13u,displacement=0x14u,offset=0x20u,attached=0x2cu,
    attached_offset=0x38u,turned=0x38u,rotation=0x44u,ground=0x6cu,body=0x70u,carried=0x70u,character=0x74u,size=0x58u;}
// The rotation's rows as the original reads them.
namespace row {inline constexpr std::uint32_t xx=0x44u,xy=0x48u,xz=0x4cu,yx=0x50u,yy=0x54u,yz=0x58u,zx=0x5cu,zy=0x60u,zz=0x64u;}

namespace platform_carry {
enum class Access {character_load,previous_x,surface_index,previous_x_store,previous_y,previous_z,previous_z_store,identity_load,
    body_load,previous_y_store,surfaces_load,surface_data,ground_load,ground_first_clear,ground_second_clear,ground_third_clear,
    zero_load,identity_copy,rotation_copy_store,zero_x,offset_x_store,zero_y,zero_z,offset_y_store,surface_object_load,
    offset_z_store,moving_flag,search_character,moving_store,found_store,transform_ground,transform_character,found_reload,
    carried_set,carried_clear,attached_x,attached_y,attached_offset_x_store,attached_z,attached_offset_z_store,body_flags,
    body_flags_store,attached_offset_y_store,body_rotation_load,body_rotation_store,moving_reload,moving_surface,surface_z,
    surface_y,surface_x,motion_z_store,displacement_x_load,motion_x_store,motion_y_store,motion_x_reload,displacement_x_store,
    displacement_y_load,displacement_y_store,displacement_z_load,motion_z_reload,displacement_z_store,carried_load,offset_x_load,
    carry_x_load,carry_y_load,carry_z_load,offset_x_sub,offset_y_sub,offset_z_sub,motion_x_set,offset_y_load,motion_y_set,
    offset_z_load,motion_z_set,carry_x_store,carry_y_store,carry_z_store,carry_rotation_load,carry_rotation_store,ground_again,
    ground_ticks_test,attached_offset_x,attached_offset_y,displacement_y_store_attached,attached_offset_z,
    displacement_x_store_attached,update_ground,update_character,moving_final,position_x,motion_x_add,position_x_store,
    position_y,motion_y_add,position_y_store,position_z,motion_z_add,position_z_store,carried_final,found_final,carry_position_x,
    offset_x_add,carry_position_x_store,carry_position_y,offset_y_add,carry_position_y_store,carry_position_z,offset_z_add,
    carry_position_z_store,forward_x,forward_x_copy,forward_y,forward_z,forward_z_copy,forward_y_copy,
    f_vy,f_vz,f_vx,f_yx,f_zx,f_xx,f_out_x,f_vx2,f_vy2,f_yy,f_xy,f_vz2,f_zy,f_out_y,f_vx3,f_vy3,f_xz,f_yz,f_vz3,f_zz,f_out_z,
    up_x,up_x_copy,up_y,up_z,up_z_copy,up_y_copy,u_vx,u_vy,u_vz,u_out_x,u_vy2,u_yy,u_vz2,u_vx2,u_zy,u_out_y,u_vy3,u_vz3,u_yz,u_vx3,
    u_out_z,ground_final,ticks_load,ticks_store};
// The original's EBX (the surface entry) and EBP (the body).
struct Kept {std::uint32_t surface_entry,body_address;};

// The rotation's columns kept in registers between the two turns.
struct Columns {std::uint32_t xx,xy,xz,yx,zx,zz;};
// Turns the vector at `v` (copied to the turned local first) through the
// rotation into `out`, as the original orders each product and sum. The
// forward turn keeps its rows in registers; the up turn reuses them.
template<class Memory,class Frame> Columns turn_forward(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum Access;
    const auto t=frame.local(local::turned);
    const auto copy_x=memory.load32(forward_x,out+x);
    memory.store32(forward_x_copy,t+x,copy_x);
    const auto copy_y=memory.load32(forward_y,out+y);
    const auto copy_z=memory.load32(forward_z,out+z);
    memory.store32(forward_z_copy,t+z,copy_z);
    memory.store32(forward_y_copy,t+y,copy_y);
    frame.turning();
    const auto vy=memory.load32(f_vy,t+y),vz=memory.load32(f_vz,t+z),vx=memory.load32(f_vx,t+x);
    Columns c{};
    c.yx=memory.load32(f_yx,frame.local(row::yx));c.zx=memory.load32(f_zx,frame.local(row::zx));
    const auto yz_sum=sse::add(sse::multiply(vy,c.yx),sse::multiply(vz,c.zx));
    c.xx=memory.load32(f_xx,frame.local(row::xx));
    const auto out_x=sse::add(yz_sum,sse::multiply(vx,c.xx));
    memory.store32(f_out_x,out+x,out_x);
    const auto vx2=memory.load32(f_vx2,t+x);
    const auto vy2_load=memory.load32(f_vy2,t+y);
    const auto vy2=sse::multiply(vy2_load,memory.load32(f_yy,frame.local(row::yy)));
    c.xy=memory.load32(f_xy,frame.local(row::xy));
    const auto xy_sum=sse::add(sse::multiply(vx2,c.xy),vy2);
    const auto vz2_load=memory.load32(f_vz2,t+z);
    const auto vz2=sse::multiply(vz2_load,memory.load32(f_zy,frame.local(row::zy)));
    const auto out_y=sse::add(xy_sum,vz2);
    memory.store32(f_out_y,out+y,out_y);
    const auto vx3=memory.load32(f_vx3,t+x);
    const auto vy3_load=memory.load32(f_vy3,t+y);
    c.xz=memory.load32(f_xz,frame.local(row::xz));
    const auto vy3=sse::multiply(vy3_load,memory.load32(f_yz,frame.local(row::yz)));
    const auto vz3=memory.load32(f_vz3,t+z);
    const auto xz_sum=sse::add(sse::multiply(vx3,c.xz),vy3);
    c.zz=memory.load32(f_zz,frame.local(row::zz));
    const auto zz_product=sse::multiply(vz3,c.zz);
    const auto out_z=sse::add(xz_sum,zz_product);
    frame.turned_forward(c,out_z,zz_product);
    memory.store32(f_out_z,out+z,out_z);
    return c;
}
template<class Memory,class Frame> void turn_up(Memory &memory,Frame &frame,std::uint32_t out,const Columns &c) {
    using enum Access;
    const auto t=frame.local(local::turned);
    const auto copy_x=memory.load32(up_x,out+x);
    memory.store32(up_x_copy,t+x,copy_x);
    const auto copy_y=memory.load32(up_y,out+y);
    const auto copy_z=memory.load32(up_z,out+z);
    memory.store32(up_z_copy,t+z,copy_z);
    memory.store32(up_y_copy,t+y,copy_y);
    frame.turning();
    const auto x_product=sse::multiply(c.xx,memory.load32(u_vx,t+x));
    const auto y_product=sse::multiply(memory.load32(u_vy,t+y),c.yx);
    const auto z_product=sse::multiply(memory.load32(u_vz,t+z),c.zx);
    const auto out_x=sse::add(sse::add(y_product,z_product),x_product);
    memory.store32(u_out_x,out+x,out_x);
    const auto vy2=memory.load32(u_vy2,t+y);
    const auto y2=sse::multiply(vy2,memory.load32(u_yy,frame.local(row::yy)));
    const auto vz2=memory.load32(u_vz2,t+z);
    const auto x2=sse::multiply(c.xy,memory.load32(u_vx2,t+x));
    const auto z2=sse::multiply(vz2,memory.load32(u_zy,frame.local(row::zy)));
    const auto out_y=sse::add(sse::add(y2,z2),x2);
    memory.store32(u_out_y,out+y,out_y);
    const auto vy3=memory.load32(u_vy3,t+y),vz3=memory.load32(u_vz3,t+z);
    const auto y3=sse::multiply(vy3,memory.load32(u_yz,frame.local(row::yz)));
    const auto x3=sse::multiply(c.xz,memory.load32(u_vx3,t+x));
    const auto z3=sse::multiply(vz3,c.zz);
    const auto out_z=sse::add(sse::add(y3,z3),x3);
    frame.turned_up(out_x,z2,x3,z3,out_z);
    memory.store32(u_out_z,out+z,out_z);
}

// Frame: see native_tu15_platform_carry.hpp.
template<class Memory,class Frame> void carry(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The displacement, the ground's handles, the surface and the identity rotation.
    frame.framing();
    const auto character_address=memory.load32(character_load,frame.local(local::character));
    const auto previous=frame.previous_address(character_address);
    const auto previous_x_value=memory.load32(previous_x,previous+x);
    const auto surface_number=memory.load32(surface_index,at(character_address,character::surface));
    frame.saving(1u);
    kept.surface_entry=frame.surface_offset(surface_number);
    memory.store32(previous_x_store,frame.local(local::displacement+x),previous_x_value);
    const auto previous_y_value=memory.load32(previous_y,previous+y);
    memory.store32(previous_z_store,frame.local(local::displacement+z),memory.load32(previous_z,previous+z));
    frame.saving(2u);
    frame.saving(3u);
    const auto identity=memory.load32(identity_load,globals::identity);
    kept.body_address=memory.load32(body_load,frame.local(local::body));
    memory.store32(previous_y_store,frame.local(local::displacement+y),previous_y_value);
    const auto surfaces=memory.load32(surfaces_load,globals::surfaces);
    const auto surface_data_address=memory.load32(surface_data,at(surfaces,datum_data));
    const auto ground_address=memory.load32(ground_load,frame.local(local::ground));
    memory.store32(ground_first_clear,at(ground_address,ground::first),none);
    memory.store32(ground_second_clear,at(ground_address,ground::second),none);
    memory.store32(ground_third_clear,at(ground_address,ground::third),none);
    const auto zero=memory.load32(zero_load,globals::zero_vector);
    frame.saving(4u);
    kept.surface_entry=frame.surface_entry(kept.surface_entry,surface_data_address);
    for(std::uint32_t k=0;k<rotation_words;++k) {
        const auto at_k=frame.copy_offset(k);
        memory.store32(rotation_copy_store,frame.local(local::rotation)+at_k,memory.load32(identity_copy,identity+identity_rotation+at_k));
    }
    frame.copied(identity+identity_rotation,frame.local(local::rotation));
    memory.store32(offset_x_store,frame.local(local::offset+x),memory.load32(zero_x,zero+x));
    const auto zero_y_value=memory.load32(zero_y,zero+y);
    const auto zero_z_value=memory.load32(zero_z,zero+z);
    memory.store32(offset_y_store,frame.local(local::offset+y),zero_y_value);
    const auto object=memory.load32(surface_object_load,at(kept.surface_entry,surface::object));
    memory.store32(offset_z_store,frame.local(local::offset+z),zero_z_value);
    const bool moving=frame.moving(memory.load8(moving_flag,at(object,surface_object::moving)));
    // The attached object, else whether the surface carries the character.
    frame.search_arguments();
    memory.load32(search_character,frame.local(local::character));
    memory.store8(moving_store,frame.local(local::moving),moving?1u:0u);
    const auto found=frame.attached(kept);
    memory.store8(found_store,frame.local(local::found),found);
    // Afterwards the original tests AL: the attached answer, or the found
    // byte reloaded from the ESP 1FA1A0 left.
    auto attached_now=found;
    bool carried=false;
    if(!found) {
        memory.load32(transform_ground,frame.local(local::ground));
        frame.transform_arguments();
        memory.load32(transform_character,frame.local(local::character));
        carried=frame.carried(kept);
        attached_now=memory.load8(found_reload,frame.local(local::found));
        if(carried)memory.store8(carried_set,frame.local(local::carried),1u);
    }
    if(!carried) {
        frame.answered_found();
        memory.store8(carried_clear,frame.local(local::carried),0u);
    }
    if(!carried && attached_now) {
        const auto attached_x_value=memory.load32(attached_x,frame.local(local::attached+x));
        const auto attached_y_value=memory.load32(attached_y,frame.local(local::attached+y));
        memory.store32(attached_offset_x_store,frame.local(local::offset+x),attached_x_value);
        memory.store32(attached_offset_z_store,frame.local(local::offset+z),memory.load32(attached_z,frame.local(local::attached+z)));
        memory.store32(body_flags_store,at(kept.body_address,body::flags),
            frame.flagged(memory.load32(body_flags,at(kept.body_address,body::flags))));
        frame.copying_rotation(kept.body_address);
        memory.store32(attached_offset_y_store,frame.local(local::offset+y),attached_y_value);
        for(std::uint32_t k=0;k<rotation_words;++k) {
            const auto at_k=frame.copy_offset(k);
            memory.store32(body_rotation_store,at(kept.body_address,body::rotation)+at_k,
                memory.load32(body_rotation_load,frame.local(local::rotation)+at_k));
        }
        frame.copied(frame.local(local::rotation),at(kept.body_address,body::rotation));
    }
    // The body's motion and the displacement.
    if(frame.moving_again(memory.load8(moving_reload,frame.local(local::moving)))) {
        const auto moving_object=memory.load32(moving_surface,at(kept.surface_entry,surface::object));
        const auto motion_z=memory.load32(surface_z,at(moving_object,surface_object::position+z));
        const auto motion_y=memory.load32(surface_y,at(moving_object,surface_object::position+y));
        const auto motion_x=memory.load32(surface_x,at(moving_object,surface_object::position+x));
        frame.surface_position(moving_object);
        memory.store32(motion_z_store,at(kept.body_address,body::motion+z),motion_z);
        const auto dx=memory.load32(displacement_x_load,frame.local(local::displacement+x));
        memory.store32(motion_x_store,at(kept.body_address,body::motion+x),motion_x);
        memory.store32(motion_y_store,at(kept.body_address,body::motion+y),motion_y);
        memory.store32(displacement_x_store,frame.local(local::displacement+x),
            sse::subtract(dx,memory.load32(motion_x_reload,at(kept.body_address,body::motion+x))));
        const auto dy=sse::subtract(memory.load32(displacement_y_load,frame.local(local::displacement+y)),motion_y);
        memory.store32(displacement_y_store,frame.local(local::displacement+y),dy);
        const auto dz_load=memory.load32(displacement_z_load,frame.local(local::displacement+z));
        const auto dz=sse::subtract(dz_load,memory.load32(motion_z_reload,at(kept.body_address,body::motion+z)));
        frame.moved(dz,motion_y);
        memory.store32(displacement_z_store,frame.local(local::displacement+z),dz);
    } else if(frame.carried_again(memory.load8(carried_load,frame.local(local::carried)),attached_now)) {
        frame.found_tested(attached_now);
        const auto offset_x=memory.load32(offset_x_load,frame.local(local::offset+x));
        const auto cx=memory.load32(carry_x_load,frame.local(local::displacement+x));
        const auto cy=memory.load32(carry_y_load,frame.local(local::displacement+y));
        const auto cz=memory.load32(carry_z_load,frame.local(local::displacement+z));
        const auto dx=sse::subtract(cx,memory.load32(offset_x_sub,frame.local(local::offset+x)));
        const auto dy=sse::subtract(cy,memory.load32(offset_y_sub,frame.local(local::offset+y)));
        const auto dz=sse::subtract(cz,memory.load32(offset_z_sub,frame.local(local::offset+z)));
        frame.motion_address(kept.body_address);
        memory.store32(motion_x_set,at(kept.body_address,body::motion+x),offset_x);
        memory.store32(motion_y_set,at(kept.body_address,body::motion+y),memory.load32(offset_y_load,frame.local(local::offset+y)));
        memory.store32(motion_z_set,at(kept.body_address,body::motion+z),memory.load32(offset_z_load,frame.local(local::offset+z)));
        frame.copying_rotation(kept.body_address);
        memory.store32(carry_x_store,frame.local(local::displacement+x),dx);
        memory.store32(carry_y_store,frame.local(local::displacement+y),dy);
        memory.store32(carry_z_store,frame.local(local::displacement+z),dz);
        for(std::uint32_t k=0;k<rotation_words;++k) {
            const auto at_k=frame.copy_offset(k);
            memory.store32(carry_rotation_store,at(kept.body_address,body::rotation)+at_k,
                memory.load32(carry_rotation_load,frame.local(local::rotation)+at_k));
        }
        frame.copied(frame.local(local::rotation),at(kept.body_address,body::rotation));
        frame.displaced(dx,dy,dz);
        if(attached_now) {
            const auto ground_now=memory.load32(ground_again,frame.local(local::ground));
            if(frame.ticked(memory.load8(ground_ticks_test,at(ground_now,ground::ticks)))) {
                const auto ax=sse::add(memory.load32(attached_offset_x,frame.local(local::attached_offset+x)),dx);
                const auto ay=sse::add(memory.load32(attached_offset_y,frame.local(local::attached_offset+y)),dy);
                memory.store32(displacement_y_store_attached,frame.local(local::displacement+y),ay);
                const auto az_load=memory.load32(attached_offset_z,frame.local(local::attached_offset+z));
                memory.store32(displacement_x_store_attached,frame.local(local::displacement+x),ax);
                const auto az=sse::add(az_load,dz);
                frame.attached_displaced(ax,az);
                memory.store32(displacement_z_store,frame.local(local::displacement+z),az);
            }
        }
    }
    // 1F8B30, then the body follows.
    const auto ground_now=memory.load32(update_ground,frame.local(local::ground));
    const auto character_now=memory.load32(update_character,frame.local(local::character));
    frame.update(kept,ground_now,character_now);
    if(frame.moving_again(memory.load8(moving_final,frame.local(local::moving)))) {
        static constexpr Access loads[]{position_x,position_y,position_z},adds[]{motion_x_add,motion_y_add,motion_z_add},
            stores[]{position_x_store,position_y_store,position_z_store};
        std::uint32_t sum=0u;
        for(std::uint32_t k=0;k<3u;++k) {
            const auto position=memory.load32(loads[k],at(kept.body_address,body::position+4u*k));
            sum=sse::add(position,memory.load32(adds[k],at(kept.body_address,body::motion+4u*k)));
            memory.store32(stores[k],at(kept.body_address,body::position+4u*k),sum);
        }
        frame.positioned(sum);
    } else {
        bool follows=frame.carried_final(memory.load8(carried_final,frame.local(local::carried)));
        if(!follows)follows=frame.found_final(memory.load8(found_final,frame.local(local::found)));
        if(follows) {
            const auto px_load=memory.load32(carry_position_x,at(kept.body_address,body::position+x));
            const auto px=sse::add(px_load,memory.load32(offset_x_add,frame.local(local::offset+x)));
            memory.store32(carry_position_x_store,at(kept.body_address,body::position+x),px);
            const auto py_load=memory.load32(carry_position_y,at(kept.body_address,body::position+y));
            const auto py=sse::add(py_load,memory.load32(offset_y_add,frame.local(local::offset+y)));
            frame.forward_address(kept.body_address);
            memory.store32(carry_position_y_store,at(kept.body_address,body::position+y),py);
            const auto pz_load=memory.load32(carry_position_z,at(kept.body_address,body::position+z));
            const auto pz=sse::add(pz_load,memory.load32(offset_z_add,frame.local(local::offset+z)));
            frame.same_vector();
            memory.store32(carry_position_z_store,at(kept.body_address,body::position+z),pz);
            const auto columns=turn_forward(memory,frame,at(kept.body_address,body::forward));
            frame.up_address(kept.body_address);
            turn_up(memory,frame,at(kept.body_address,body::up),columns);
        }
    }
    // The ground's ticks.
    const auto ground_last=memory.load32(ground_final,frame.local(local::ground));
    const auto ticks=frame.counted(memory.load8(ticks_load,at(ground_last,ground::ticks)));
    memory.store8(ticks_store,at(ground_last,ground::ticks),ticks);
    frame.finished();
}
} // namespace platform_carry
} // namespace halo2::native_platform_carry
