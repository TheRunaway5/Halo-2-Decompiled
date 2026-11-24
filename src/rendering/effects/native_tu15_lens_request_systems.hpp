#pragma once
// Native TU1.5 lens request, entered at 2DB80 (lens flare unit): a lens
// flare's request for the current view, projected and queued.
//
//   lens_request::submit   2DB80(ECX = lens tag, EDI = forward; kind, category, owner, ordinal,
//                                position*, color*, brightness, opacity, scale) -> AL, RET 24
//
// A view ([4C66F4], NONE for none), a lens and a positive brightness are
// needed. The lens's definition (the tag instances at [4F0414], 16 bytes,
// +8) refuses a request without opacity when its flags (+0x2A) carry bit
// 0, and a request of the kind its kind flags (+0x28: bit 2 for kind 0,
// bit 3 otherwise) exclude. A definition whose range ends differ (+0x18,
// +0x1C) shows only nearer than the end along the view's forward axis
// ([4C65CC]) from the camera ([4C65C0]). The request (the frame's 0x14
// bytes: the kind byte, the view, category, owner, ordinal) and the lens
// projected by 2E210 (the frame's 0x18 bytes) go to 20990; a definition
// with reflections (int16 +0x16) queues them again as category 4, and
// 20B20 links the second to the first. The answer, a byte in the position
// argument's own slot, is 1 once queued.
//
// The single-precision arithmetic is the original's scalar SSE on the live
// MXCSR. This is the game logic only. The game's memory is the retained
// TU1.5 bank, reached through a Memory object with typed loads and stores;
// every access names its role (Access), and the boundary adapter
// (native_tu15_lens_request.hpp) maps the role to the original instruction
// and owns the ABI of the children; the definition, the projected lens's
// address and then the first queued lens, which the original keeps in EBX
// and ESI across them, are a Kept, refreshed from the registers each
// returned.
#include "physics/runtime/native_tu15_perpendicular_systems.hpp"
#include <cstdint>

namespace halo2::native_lens_request {
using native_object_update::at;
using native_object_update::none;
using native_perpendicular::unequal;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t view=0x4c66f4u;       // the submitting view, NONE for none
inline constexpr std::uint32_t tags=0x4f0414u;       // 16-byte tag instances (+8 the definition)
inline constexpr std::uint32_t camera=0x4c65c0u;     // float[3]: the view's camera
inline constexpr std::uint32_t forward=0x4c65ccu;    // float[3]: the view's forward axis
}
namespace argument {inline constexpr std::uint32_t kind=1u,category=2u,owner=3u,ordinal=4u,position=5u,color=6u,brightness=7u,opacity=8u,scale=9u;}
namespace definition {inline constexpr std::uint32_t reflections=0x16u,range_begin=0x18u,range_end=0x1cu,kind_flags=0x28u,flags=0x2au;}
namespace request {inline constexpr std::uint32_t kind=0u,view=4u,category=8u,owner=0xcu,ordinal=0x10u,size=0x14u;}
namespace flag {inline constexpr std::uint32_t needs_opacity=1u,excludes_kind_zero=4u,excludes_other_kinds=8u;}
inline constexpr std::uint32_t tag_size=16u,tag_definition=8u,reflected_category=4u,zero_bits=0u;

namespace lens_request {
enum class Access {view,position_argument,result_clear,brightness,opacity,tags,definition,definition_flags,kind,kind_flags,
    range_begin,range_end,position_x,position_y,position_z,camera_x,camera_y,camera_z,forward_y,forward_z,forward_x,range_end_again,
    request_clear,kind_again,request_view,category,request_kind,owner,request_category,ordinal,request_owner,scale,brightness_again,
    request_ordinal,opacity_again,color,reflections,reflected_category,result_set,result};
enum class Outcome {unbound,dark,needs_opacity,excluded,out_of_range,refused,queued,reflected};
enum class Queue {first,reflection};
struct Kept {std::uint32_t definition,projected,first;};   // EBX, ESI (the projected lens, then the first queued)
// Frame:
//   argument(k), request(offset)           the frame's slots now
//   offset(dx, dy, dz)                     the position less the camera
//   measured(y_term, x_term, along)        the distance along the forward axis
//   project(kept, position, color, brightness, opacity, scale)   2E210, refreshes kept
//   queue(kept, Queue) -> handle           20990, refreshes kept
//   link(kept, first, second)              20B20, refreshes kept
template<class Memory,class Frame> Outcome decide(Memory &memory,Frame &frame,std::uint32_t lens) {
    using enum Access;
    const auto view_value=memory.load32(view,globals::view);
    const auto position=memory.load32(position_argument,frame.argument(argument::position));
    memory.store8(result_clear,frame.argument(argument::position),0u);
    if(view_value==none || lens==none)return Outcome::unbound;
    if(sse::not_above(memory.load32(Access::brightness,frame.argument(argument::brightness)),zero_bits))return Outcome::dark;
    const auto opacity_value=memory.load32(opacity,frame.argument(argument::opacity));
    const auto tag_array=memory.load32(tags,globals::tags);
    const bool opaque=!sse::not_above(opacity_value,zero_bits);
    Kept kept{memory.load32(Access::definition,tag_array+tag_size*(lens&0xffffu)+tag_definition),0u,0u};
    if(!opaque && (memory.load8(definition_flags,at(kept.definition,definition::flags))&flag::needs_opacity))
        return Outcome::needs_opacity;
    const bool other_kind=memory.load8(kind,frame.argument(argument::kind))&0xffu;
    if(memory.load8(kind_flags,at(kept.definition,definition::kind_flags))&(other_kind?flag::excludes_other_kinds:flag::excludes_kind_zero))
        return Outcome::excluded;
    // A ranged lens shows only nearer than its range's end along the view.
    const auto begin=memory.load32(range_begin,at(kept.definition,definition::range_begin));
    const bool ranged=unequal(begin,memory.load32(range_end,at(kept.definition,definition::range_end)));
    const auto x=memory.load32(position_x,at(position,0u));
    const auto y=memory.load32(position_y,at(position,4u));
    const auto z=memory.load32(position_z,at(position,8u));
    const auto dx=sse::subtract(x,memory.load32(camera_x,globals::camera));
    const auto dy=sse::subtract(y,memory.load32(camera_y,globals::camera+4u));
    const auto dz=sse::subtract(z,memory.load32(camera_z,globals::camera+8u));
    frame.offset(dx,dy,dz);
    if(ranged) {
        const auto forward_y_value=memory.load32(forward_y,globals::forward+4u);
        const auto z_term=sse::multiply(dz,memory.load32(forward_z,globals::forward+8u));
        const auto y_term=sse::multiply(forward_y_value,dy);
        const auto x_term=sse::multiply(memory.load32(forward_x,globals::forward),dx);
        const auto end=memory.load32(range_end_again,at(kept.definition,definition::range_end));
        const auto along=sse::add(sse::add(z_term,y_term),x_term);
        frame.measured(end,y_term,x_term,along);
        if(sse::not_above(end,along))return Outcome::out_of_range;
    }
    // The request, built while the arguments are read, and the projected lens.
    for(std::uint32_t field=0;field<request::size;field+=4u)memory.store32(request_clear,frame.request(field),0u);
    const auto kind_value=memory.load8(kind_again,frame.argument(argument::kind));
    memory.store32(request_view,frame.request(request::view),view_value);
    const auto category_value=memory.load32(category,frame.argument(argument::category));
    memory.store8(request_kind,frame.request(request::kind),kind_value);
    const auto owner_value=memory.load32(owner,frame.argument(argument::owner));
    memory.store32(request_category,frame.request(request::category),category_value);
    const auto ordinal_value=memory.load32(ordinal,frame.argument(argument::ordinal));
    memory.store32(request_owner,frame.request(request::owner),owner_value);
    const auto scale_value=memory.load32(scale,frame.argument(argument::scale));
    const auto brightness_value=memory.load32(brightness_again,frame.argument(argument::brightness));
    memory.store32(request_ordinal,frame.request(request::ordinal),ordinal_value);
    const auto opacity_again_value=memory.load32(opacity_again,frame.argument(argument::opacity));
    const auto color_value=memory.load32(color,frame.argument(argument::color));
    frame.project(kept,position,color_value,brightness_value,opacity_again_value,scale_value);
    const auto first=frame.queue(kept,Queue::first);
    if(first==none)return Outcome::refused;
    auto second=none;
    const bool reflects=memory.load16(reflections,at(kept.definition,definition::reflections));
    if(reflects) {
        memory.store32(Access::reflected_category,frame.request(request::category),native_lens_request::reflected_category);
        second=frame.queue(kept,Queue::reflection);
    }
    frame.link(kept,kept.first,second);
    memory.store8(result_set,frame.argument(argument::position),1u);
    return reflects?Outcome::reflected:Outcome::queued;
}
// The answer is the byte the position's slot then holds.
template<class Memory,class Frame> std::uint32_t submit(Memory &memory,Frame &frame,std::uint32_t lens,Outcome &outcome) {
    outcome=decide(memory,frame,lens);
    return memory.load8(Access::result,frame.argument(argument::position));
}
} // namespace lens_request
} // namespace halo2::native_lens_request
