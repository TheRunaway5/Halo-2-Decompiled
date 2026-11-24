#pragma once
// Native TU1.5 body list ray, entered at 315190 (live support unit): a ray
// cast against each body of a list, in the body's frame.
//
//   body_list_ray::cast   315190(ECX = list; input, collector)
//
// The ray in each body's frame goes into a ray input on the stack: the
// input's filter info (+0x24) and filter, the filter interface (+0x10) of
// the list's world (+8) filter (+0xD0) when the input enables it (byte
// +0x20) and the world has one, else none. Each body (the list's bodies
// +0x80, count +0x84) with a shape (+0) has its motion (+8) hold the
// rotation's columns (+0x20, +0x30, +0x40) and the translation (+0x50); the
// ray's ends (+0, +0x10) less the translation, times the rotation's
// transpose (each row scaled by its component, the x term plus the y term,
// plus the z term), are the local ray, and the shape's cast (vtable +0x24,
// through the dispatch: the local input, the body and the collector) takes it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores (vectors as
// the original's 16-byte moves); every access names its role (Access); the
// boundary adapter (native_tu15_body_list_ray.hpp) maps the role to the
// original instruction and owns the ABI of the virtual call. The values the
// original keeps in callee-saved registers across it are a Kept, refreshed
// from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include <cstdint>

namespace halo2::native_body_list_ray {
using native_object_update::at;
namespace sse=native_packed_sse;
namespace list {inline constexpr std::uint32_t world=8u,bodies=0x80u,count=0x84u;}
namespace world {inline constexpr std::uint32_t filter=0xd0u;}
namespace filter {inline constexpr std::uint32_t interface=0x10u;}
namespace input {inline constexpr std::uint32_t from=0u,to=0x10u,filter_enabled=0x20u,filter_info=0x24u;}
namespace body {inline constexpr std::uint32_t shape=0u,motion=8u;}
namespace motion {inline constexpr std::uint32_t first=0x20u,second=0x30u,third=0x40u,translation=0x50u;}
namespace shape {inline constexpr std::uint32_t vtable=0u,cast=0x24u;}
// The aligned frame below the saved registers: the bodies left, then the local ray input.
namespace local {inline constexpr std::uint32_t remaining=0xcu,from=0x10u,to=0x20u,filter_info=0x30u,filter=0x34u,size=0x34u;}
namespace argument {inline constexpr unsigned input=0u,collector=1u;}

// The body's motion: its rotation's columns and its translation.
struct Motion {__m128 first,second,third,translation;};
// The rotation's transpose, as the original's shuffles build it: lane k of
// row r is column k's component r, the fourth lane repeating the third.
struct Transpose {__m128 low,high,x_row,y_row,z_row;};
inline Transpose transposed(const Motion &m) {
    Transpose t{};
    t.low=_mm_shuffle_ps(m.third,m.third,0x44);
    t.high=_mm_shuffle_ps(m.third,m.third,0xee);
    const auto first_low=_mm_shuffle_ps(m.first,m.second,0x44),first_high=_mm_shuffle_ps(m.first,m.second,0xee);
    t.x_row=_mm_shuffle_ps(first_low,t.low,0x88);
    t.y_row=_mm_shuffle_ps(first_low,t.low,0xdd);
    t.z_row=_mm_shuffle_ps(first_high,t.high,0x88);
    return t;
}
// A point in the body's frame, with the original's partial terms.
struct Local {__m128 offset,y_term,z_term,point;};
inline Local into_body(const Motion &m,const Transpose &t,__m128 point) {
    Local l{};
    l.offset=sse::subtract(point,m.translation);
    l.y_term=sse::multiply(t.y_row,sse::broadcast<1>(l.offset));
    const auto x_term=sse::multiply(t.x_row,sse::broadcast<0>(l.offset));
    l.z_term=sse::multiply(t.z_row,sse::broadcast<2>(l.offset));
    l.point=sse::add(sse::add(x_term,l.y_term),l.z_term);
    return l;
}

namespace body_list_ray {
enum class Access {input_argument,filter_info,filter_info_store,filter_enabled,filter_clear,world_load,world_filter,filter_store,
    filter_none,count,bodies,collector_argument,remaining_first,entry,shape_load,motion_load,column,point,local_store,entry_again,vtable,
    cast_slot,remaining_load,remaining_store};
enum class Outcome {empty,cast};
// The original's EBX (the collector), ESI (the body cursor) and EDI (the input) across the virtual call.
struct Kept {std::uint32_t collector,cursor,input;};
// Frame: framing(); saving(); argument(n) EBP-relative; local(offset) on the aligned frame; filter_tested(enabled);
// compared(value); filter_interface(filter); decremented(v); incremented(v); tested(shape); transformed(motion, transpose, local);
// pushed(collector, body); cast(slot, kept) [the shape's cast]; advanced(cursor); restoring().
template<class Memory,class Frame> Outcome cast(Memory &memory,Frame &frame,std::uint32_t list_address,Kept kept) {
    using enum Access;
    frame.framing();
    frame.saving();
    kept.input=memory.load32(input_argument,frame.argument(argument::input));
    const auto info=memory.load32(filter_info,at(kept.input,input::filter_info));
    memory.store32(filter_info_store,frame.local(local::filter_info),info);
    const auto enabled=memory.load8(filter_enabled,at(kept.input,input::filter_enabled));
    frame.filter_tested(enabled);
    memory.store32(filter_clear,frame.local(local::filter),0u);
    bool filtered=false;
    if(enabled) {
        const auto world_address=memory.load32(world_load,at(list_address,list::world));
        const auto filter_object=memory.load32(world_filter,at(world_address,world::filter));
        frame.compared(filter_object);
        if(filter_object) {
            memory.store32(filter_store,frame.local(local::filter),frame.filter_interface(filter_object));
            filtered=true;
        }
    }
    if(!filtered)memory.store32(filter_none,frame.local(local::filter),0u);
    const auto count_value=memory.load32(count,at(list_address,list::count));
    const auto last=frame.decremented(count_value);
    kept.cursor=memory.load32(bodies,at(list_address,list::bodies));
    if(std::int32_t(last)<0){frame.restoring();return Outcome::empty;}
    kept.collector=memory.load32(collector_argument,frame.argument(argument::collector));
    memory.store32(remaining_first,frame.local(local::remaining),frame.incremented(last));
    for(;;) {
        const auto body_address=memory.load32(entry,kept.cursor);
        const auto shape_address=memory.load32(shape_load,at(body_address,body::shape));
        frame.tested(shape_address);
        if(shape_address) {
            // The ray's ends into the body's frame (the motion read once for each end).
            const auto motion_address=memory.load32(motion_load,at(body_address,body::motion));
            const auto read_motion=[&] {
                Motion m{};
                m.third=memory.load128(column,motion_address+motion::third);
                m.translation=memory.load128(column,motion_address+motion::translation);
                m.first=memory.load128(column,motion_address+motion::first);
                m.second=memory.load128(column,motion_address+motion::second);
                return m;
            };
            const auto from_motion=read_motion();
            const auto from=memory.load128(point,at(kept.input,input::from));
            const auto to=memory.load128(point,at(kept.input,input::to));
            const auto local_from=into_body(from_motion,transposed(from_motion),from);
            memory.store128(local_store,frame.local(local::from),local_from.point);
            const auto to_motion=read_motion();
            const auto to_transpose=transposed(to_motion);
            const auto local_to=into_body(to_motion,to_transpose,to);
            memory.store128(local_store,frame.local(local::to),local_to.point);
            frame.transformed(to_transpose,local_to);
            // The shape's cast of the local ray.
            const auto body_now=memory.load32(entry_again,kept.cursor);
            const auto table=memory.load32(vtable,at(shape_address,shape::vtable));
            frame.pushed(kept.collector,body_now);
            const auto target=memory.load32(cast_slot,at(table,shape::cast));
            frame.cast(target,kept);
        }
        const auto remaining=memory.load32(remaining_load,frame.local(local::remaining));
        kept.cursor=frame.advanced(kept.cursor);
        const auto left=frame.decremented(remaining);
        memory.store32(remaining_store,frame.local(local::remaining),left);
        if(!left)break;
    }
    frame.restoring();
    return Outcome::cast;
}
} // namespace body_list_ray
} // namespace halo2::native_body_list_ray
