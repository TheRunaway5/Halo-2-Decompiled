#pragma once
// Native TU1.5 point blocked, entered at 16BDA0 (physics live support unit):
// whether a point is blocked, by the tests its flags ask for.
//
//   point_blocked::query   16BDA0(flags, point*, skip, skip again) -> AL, RET 0x10
//
// A point in no cluster (14CDB0: ECX [4ECC0C], EDX the point, EAX 0) is
// blocked. Otherwise, with flag 2 or 0xC: the cluster's leaf (word +0 of its
// 8-byte entry at the structure BSP's +0x30; 0xB0-byte leaves at +0xA0)
// blocks it (flag 2) when its kind (int8 +0x70, not -1) names a plane (the
// low 7 bits of a negative kind, 0x18-byte planes at +0x68, word +2 in use)
// the point is behind, or a shape ([4ECC20]'s +0x34C, 16-byte entries, word
// +0xC in use); the leaf's surfaces (a count +0x98, int16 indices at +0x9C)
// block it (flag 4) when 16BBC0 (EAX the surface, EBX the point; the flags
// pushed) says so; nearby objects (flag 8; the iterator BA970/BA9E0 from the
// leaf, its cursor and state in the frame) block it when their type is among
// the flags' bits 4 to 16, their flags (word +2) miss 16E360's mask, the
// point is within their sphere (+4 centre, +0x10 radius; COMISS on the
// squares) and they are neither skip, and 16BC80 (the object, 1, the
// flags, the point and the skips pushed) says so. The answer byte lives in
// the point argument's slot.
//
// The single-precision arithmetic is the original's scalar SSE, in its order
// (the first operand the destination), on the live MXCSR. This is the game
// logic only. The game's memory is the retained TU1.5 bank, reached through
// a Memory object with typed loads and stores; every access names its role
// (Access); the boundary adapter (native_tu15_point_blocked.hpp) maps the
// role to the original instruction and owns the ABI of the children. The
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_point_blocked {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen8;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t locator=0x4ecc0cu,structure_bsp=0x4ecc18u,shapes=0x4ecc20u;}
inline constexpr std::uint32_t cluster_size=8u,leaf_size=0xb0u,plane_size=0x18u,shape_size=0x10u,kind_mask=0x7fu,zero_float=0u;
namespace bsp {inline constexpr std::uint32_t clusters=0x30u,planes=0x68u,leaves=0xa0u;}
namespace leaf {inline constexpr std::uint32_t kind=0x70u,surface_count=0x98u,surfaces=0x9cu;}
namespace plane {inline constexpr std::uint32_t used=2u,x=4u,y=8u,z=0xcu,d=0x10u;}
namespace shapes {inline constexpr std::uint32_t list=0x34cu,used=0xcu;}
namespace object {inline constexpr std::uint32_t type=0u,flags=2u,x=4u,y=8u,z=0xcu,radius=0x10u;}
namespace flag {inline constexpr std::uint32_t leaf=2u,deeper=0xcu,surfaces=4u,objects=8u;inline constexpr std::uint32_t type_shift=4u,
    type_mask=0x1fffu;}
namespace argument {inline constexpr std::uint32_t flags=0u,point=1u,skip=2u,skip_again=3u;}
// The frame's slots, by offset from the return address: the leaf's index (then the iterator's cursor) and the
// iterator's state.
namespace local {inline constexpr std::int32_t cursor=-8,iterator=-4;}

namespace point_blocked {
enum class Access {locator,point_argument,answer_clear,flags_argument,bsp,clusters,cluster_leaf,leaf_store,leaves,kind,planes,
    plane_used,plane_z,point_z,plane_y,point_y,plane_x,point_x,plane_d,shapes_global,shape_list,shape_used,answer_set,answer_load,
    leaves_again,surface_count,surfaces,surface,flags_again,count_again,surface_answer,leaf_load,answer_check,cursor_clear,
    cursor_load,object_type,object_flags,object_x,object_y,object_z,object_radius,skip,skip_again,flags_push,answer_final};
enum class Outcome {outside,unasked,answered,blocked};
// The original's EBX (the leaf, then the point), EBP (the point), ESI and EDI across the children.
struct Kept {std::uint32_t held,point,source,mask;};
// Frame: the hooks name the original instructions they stand for; children refresh kept.
template<class Memory,class Frame> std::uint32_t query(Memory &memory,Frame &frame,Kept kept,Outcome &outcome) {
    using enum Access;
    frame.framing();
    const auto locator_value=memory.load32(Access::locator,globals::locator);
    frame.saving(0u);
    kept.point=memory.load32(point_argument,frame.argument(argument::point));
    frame.saving(1u);
    memory.store8(answer_clear,frame.argument(argument::point),0u);
    const auto cluster=frame.locate(locator_value,kept);
    if(cluster==none){outcome=Outcome::outside;frame.restoring(0u);return 1u;}
    const auto flags_value=memory.load32(flags_argument,frame.argument(argument::flags));
    frame.saving(2u);
    if(!(flags_value&flag::leaf)) {
        frame.tested8(flags_value&flag::deeper);
        if(!(flags_value&flag::deeper)){outcome=Outcome::unasked;frame.restoring(1u);return kept.held&0xffu;}
    }
    // The leaf: a plane the point is behind, or a shape in use.
    frame.leaf_tested(flags_value&flag::leaf);
    const auto bsp_address=memory.load32(Access::bsp,globals::structure_bsp);
    const auto cluster_table=memory.load32(Access::clusters,at(bsp_address,bsp::clusters));
    const auto leaf_index=widen16(memory.load16(cluster_leaf,cluster_table+cluster_size*cluster));
    kept.held=leaf_index;
    frame.leaf_found(leaf_index);
    memory.store32(leaf_store,frame.local(local::cursor),leaf_index);
    if(flags_value&flag::leaf) {
        const auto leaves_address=memory.load32(Access::leaves,at(bsp_address,bsp::leaves));
        const auto kind_value=widen8(memory.load8(kind,frame.leaf(leaf_index,leaves_address)+leaf::kind));
        frame.kind_tested(kind_value);
        if((kind_value&0xffu)!=0xffu) {
            bool behind=false;
            if(kind_value&0x80u) {
                const auto planes_address=memory.load32(Access::planes,at(bsp_address,bsp::planes));
                const auto entry=frame.plane(kind_value,planes_address+plane_size*(kind_value&kind_mask));
                const auto used=memory.load16(plane_used,at(entry,plane::used));
                frame.used_tested(used);
                if(used!=0xffffu) {
                    const auto cz=memory.load32(plane_z,at(entry,plane::z));
                    auto distance=sse::multiply(cz,memory.load32(point_z,kept.point+8u));
                    const auto cy=memory.load32(plane_y,at(entry,plane::y));
                    auto term=sse::multiply(cy,memory.load32(point_y,kept.point+4u));
                    distance=sse::add(distance,term);
                    const auto cx=memory.load32(plane_x,at(entry,plane::x));
                    term=sse::multiply(cx,memory.load32(point_x,kept.point));
                    distance=sse::add(distance,term);
                    distance=sse::subtract(distance,memory.load32(plane_d,at(entry,plane::d)));
                    frame.plane_distance(distance,term);
                    behind=!sse::not_above(zero_float,distance);
                }
            } else {
                const auto list=memory.load32(shape_list,at(memory.load32(shapes_global,globals::shapes),shapes::list));
                const auto used=memory.load16(shape_used,frame.shape(kind_value,shape_size*(kind_value&kind_mask))+list+shapes::used);
                frame.used_tested(used);
                behind=used!=0xffffu;
            }
            if(behind)memory.store8(answer_set,frame.argument(argument::point),1u);
        }
    }
    // The leaf's surfaces.
    auto flags_now=flags_value;
    frame.tested8(flags_now&flag::surfaces);
    if(flags_now&flag::surfaces) {
        const auto answer=memory.load8(answer_load,frame.argument(argument::point));
        frame.tested8(answer);
        if(!answer) {
            const auto leaves_address=memory.load32(leaves_again,at(bsp_address,bsp::leaves));
            const auto scaled=frame.surface_leaf(leaf_index);
            const auto count=memory.load32(surface_count,scaled+leaves_address+leaf::surface_count);
            // The leaf in ESI and the surface's index in EDI, as 16BBC0 returns them.
            kept.source=frame.surfaces_counted(scaled,leaves_address,count);
            kept.mask=0u;
            if(std::int32_t(count)>0) {
                for(;;) {
                    const auto surface_index=widen16(memory.load16(surface,memory.load32(surfaces,kept.source+leaf::surfaces)+2u*kept.mask));
                    const auto found=frame.surface_blocks(surface_index,flags_now,kept)&0xffu;
                    flags_now=memory.load32(flags_again,frame.argument(argument::flags));
                    if(found){memory.store8(surface_answer,frame.argument(argument::point),1u);break;}
                    const auto count_now=memory.load32(count_again,kept.source+leaf::surface_count);
                    ++kept.mask;
                    frame.surface_counted(kept.mask,count_now);
                    if(!(std::int32_t(kept.mask)<std::int32_t(count_now)))break;
                }
                kept.held=memory.load32(leaf_load,frame.local(local::cursor));
                frame.leaf_found(kept.held);
            }
        }
    }
    // Nearby objects.
    frame.tested8(flags_now&flag::objects);
    const auto answered=memory.load8(answer_check,frame.argument(argument::point));
    if(!(flags_now&flag::objects)){outcome=Outcome::answered;frame.restoring(1u);return answered;}
    frame.tested8(answered);
    if(answered){outcome=Outcome::answered;frame.restoring(1u);return answered;}
    memory.store32(cursor_clear,frame.local(local::cursor),0u);
    // 16E360's mask in EDI, and the flags' types (from the ECX it returns) in ESI, as the iterator leaves them.
    const auto [ignored,flags_after]=frame.ignored_flags(kept);
    kept.source=(flags_after>>flag::type_shift)&flag::type_mask;
    kept.mask=ignored;
    frame.types(kept);
    auto found=frame.first_object(kept.held,kept);   // the leaf's index, in EBX as 16E360 returns it
    if(found!=none)kept.source&=0xffffu;
    if(found!=none)frame.types_widened(kept);
    for(;found!=none;found=frame.next_object(kept)) {
        const auto cursor=memory.load32(cursor_load,frame.local(local::cursor));
        const auto type=memory.load8(object_type,at(cursor,object::type));
        if(!frame.type_tested(kept.source,type))continue;
        const auto object_flags_value=memory.load16(object_flags,at(cursor,object::flags));
        if(!frame.flags_tested(object_flags_value,kept.mask))continue;
        const auto x=memory.load32(object_x,at(cursor,object::x));
        const auto dx=sse::subtract(x,memory.load32(point_x,kept.point));
        const auto y=memory.load32(object_y,at(cursor,object::y));
        const auto dy=sse::subtract(y,memory.load32(point_y,kept.point+4u));
        const auto z=memory.load32(object_z,at(cursor,object::z));
        const auto dz=sse::subtract(z,memory.load32(point_z,kept.point+8u));
        const auto radius=memory.load32(object_radius,at(cursor,object::radius));
        const auto distance=sse::add(sse::add(sse::multiply(dx,dx),sse::multiply(dy,dy)),sse::multiply(dz,dz));
        const auto reach=sse::multiply(radius,radius);
        frame.sphere(reach,dy,dz,radius,distance);
        if(sse::below(reach,distance))continue;
        const auto skip_value=memory.load32(Access::skip,frame.argument(argument::skip));
        frame.compared(found,skip_value);
        if(found==skip_value)continue;
        const auto skip_again_value=memory.load32(Access::skip_again,frame.argument(argument::skip_again));
        frame.compared(found,skip_again_value);
        if(found==skip_again_value)continue;
        frame.pushing_skips(skip_again_value,skip_value);
        const auto flags_pushed=memory.load32(flags_push,frame.argument(argument::flags));
        if(frame.object_blocks(found,flags_pushed,kept)&0xffu){outcome=Outcome::blocked;frame.restoring(3u);return 1u;}
    }
    outcome=Outcome::answered;
    const auto answer=memory.load8(answer_final,frame.argument(argument::point));
    frame.restoring(2u);
    return answer;
}
} // namespace point_blocked
} // namespace halo2::native_point_blocked
