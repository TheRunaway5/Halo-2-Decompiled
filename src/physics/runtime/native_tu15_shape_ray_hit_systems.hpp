#pragma once
// Native TU1.5 shape ray hit, entered at 2E9140 (live support unit): a
// shape's ray cast reported to a ray hit collector (the shape's
// cast-with-collector).
//
//   shape_ray_hit::cast   2E9140(ECX = shape; input, body, collector)
//
// The output's fraction (+0x14) starts at the collector's early-out fraction
// (+4). The shape's cast (vtable +0x20, through the dispatch: the hit flag,
// the input and the output) answers whether it hit; on a hit the output
// takes the body's shape key (+4) and its normal (+0) turns into the body's
// frame (the motion +8, whose rotation columns are at +0x20, +0x30 and
// +0x40: each column scaled by its component of the normal, the x term plus
// the y term, plus the z term), and the collector's first virtual (through
// the dispatch: the body and the output) takes it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores (the normal
// and the columns as the original's 16-byte moves); every access names its
// role (Access); the boundary adapter (native_tu15_shape_ray_hit.hpp) maps
// the role to the original instruction and owns the ABI of the virtual
// calls. The values the original keeps in callee-saved registers across
// them are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include <cstdint>

namespace halo2::native_shape_ray_hit {
using native_object_update::at;
namespace sse=native_packed_sse;
namespace shape {inline constexpr std::uint32_t vtable=0u,cast=0x20u;}
namespace collector {inline constexpr std::uint32_t vtable=0u,early_out=4u,add_hit=0u;}
namespace body {inline constexpr std::uint32_t key=4u,motion=8u;}
namespace motion {inline constexpr std::uint32_t rotation=0x20u,column=0x10u;}
namespace output {inline constexpr std::uint32_t normal=0u,key=0x10u,fraction=0x14u;}
// The aligned frame below the saved ESI: the hit flag, then the output.
namespace local {inline constexpr std::uint32_t hit=0xfu,output=0x10u,size=0x2cu;}
// The arguments: the input, the body, the collector.
namespace argument {inline constexpr unsigned input=0u,body=1u,collector=2u;}

namespace shape_ray_hit {
enum class Access {shape_vtable,collector_argument,early_out,fraction_store,input_argument,cast_slot,hit,normal,body_argument,key,motion,
    key_store,column,collector_vtable,normal_store,add_hit_slot};
enum class Outcome {missed,reported};
// The original's ESI (the collector) across the virtual calls.
struct Kept {std::uint32_t collector;};
// The normal in the body's frame and the original's partial terms.
struct Rotated {__m128 x_component,z_term,y_term,normal;};
// Frame: framing(); saving(); argument(n) EBP-relative; local(offset) on the aligned frame; output_pushed(); input_pushed(input);
// cast(slot, kept) [the shape's cast]; byte_compared(hit); rotation(motion); rotated(rotated); hit_pushed(body, collector);
// reported(slot, kept) [the collector]; restoring().
template<class Memory,class Frame> Outcome cast(Memory &memory,Frame &frame,std::uint32_t shape_address,Kept kept) {
    using enum Access;
    frame.framing();
    const auto vtable=memory.load32(shape_vtable,at(shape_address,shape::vtable));
    frame.saving();
    kept.collector=memory.load32(collector_argument,frame.argument(argument::collector));
    const auto early_out_value=memory.load32(early_out,at(kept.collector,collector::early_out));
    memory.store32(fraction_store,frame.local(local::output+output::fraction),early_out_value);
    frame.output_pushed();
    const auto input=memory.load32(input_argument,frame.argument(argument::input));
    frame.input_pushed(input);
    const auto cast_target=memory.load32(cast_slot,at(vtable,shape::cast));
    const auto hit_flag=frame.cast(cast_target,kept);
    const auto hit_value=memory.load8(hit,hit_flag);
    frame.byte_compared(hit_value);
    if(!hit_value){frame.restoring();return Outcome::missed;}
    // The hit: the body's key, and the normal turned into the body's frame.
    const auto normal_value=memory.load128(normal,frame.local(local::output+output::normal));
    const auto body_address=memory.load32(body_argument,frame.argument(argument::body));
    const auto key_value=memory.load32(key,at(body_address,body::key));
    const auto motion_address=memory.load32(motion,at(body_address,body::motion));
    const auto rotation=frame.rotation(motion_address);
    memory.store32(key_store,frame.local(local::output+output::key),key_value);
    const auto z_column=memory.load128(column,rotation+2u*motion::column);
    const auto y_column=memory.load128(column,rotation+motion::column);
    const auto x_column=memory.load128(column,rotation);
    const auto collector_table=memory.load32(collector_vtable,at(kept.collector,collector::vtable));
    Rotated turned{};
    turned.x_component=sse::broadcast<0>(normal_value);
    turned.z_term=sse::multiply(z_column,sse::broadcast<2>(normal_value));
    turned.y_term=sse::multiply(y_column,sse::broadcast<1>(normal_value));
    turned.normal=sse::add(sse::add(sse::multiply(x_column,turned.x_component),turned.y_term),turned.z_term);
    frame.rotated(turned);
    frame.hit_pushed(body_address);
    memory.store128(normal_store,frame.local(local::output+output::normal),turned.normal);
    const auto add_hit_target=memory.load32(add_hit_slot,at(collector_table,collector::add_hit));
    frame.reported(add_hit_target,kept);
    frame.restoring();
    return Outcome::reported;
}
} // namespace shape_ray_hit
} // namespace halo2::native_shape_ray_hit
