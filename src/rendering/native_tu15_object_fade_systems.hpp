#pragma once
// Native TU1.5 object fade: 4BAE0, called by the PRT lighting setup 1362A0
// (docs/native_replacement/4BAE0_object_fade.md).
//
//   object_fade::fade(object, distance) -> two bytes through the caller
//     The object's draw fade and shadow fade at its distance from the camera,
//     each 0..255 (255: fully drawn):
//       - both are 255 when fades are off ([4C6839]) or the object has its
//         "never fade" flag (bit 19 of its +4 flags);
//       - both stay 0 when the object's definition names no model (hlmt);
//       - otherwise the fade distances come from the model of the object's
//         root (the top of its parent chain, BCFB0) when that object has a
//         model, else from the object's own model:
//           draw fade: 1 until "begin fade distance", then linear down to 0
//             at "disappear distance" (no fade when that is not positive);
//           shadow fade: 1 until reduce_distance[4 - "shadow fade level"]
//             (at most index 4; none for an index of 0 or less), then linear
//             down to 0 over 7 times the object's bounding radius; clamped
//             to [0,1] and cut to 0 at or below 0.095.
//       Each fade is scaled by 255 in x87 arithmetic and converted by the
//       CRT's _ftol2 (32CC0C); the low byte is stored.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores, and every
// floating-point operation is performed by an Arithmetic object (the same
// adapter); every access and operation names its role, and the boundary
// adapter (native_tu15_object_fade.hpp) maps the role to the original
// instruction and its registers. Registers, flags and stack slots belong to
// the adapter.
//
// What stays exact, because translated neighbours and the children observe it:
//   - memory accesses and floating-point operations happen in the original
//     order; SSE single-precision operations and comparisons keep their MXCSR
//     and EFLAGS effects (unordered comparisons included), the x87 scaling
//     runs under the game's control word;
//   - the outputs are first cleared, then written; the original also writes a
//     byte of its own frame (the "no fade" byte) that nothing reads;
//   - the values the original keeps in callee-saved registers across BCFB0
//     and _ftol2 are the fields of Kept.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_fade {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::DataArray;
using native_object_update::ObjectHeader;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t object_headers=native_object_update::globals::object_headers; // DataArray* of ObjectHeader
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // TagInstance[]
inline constexpr std::uint32_t fades_disabled=0x4c6839u;   // byte: draw everything unfaded
inline constexpr std::uint32_t one=0x46a0d0u;              // float 1.0
inline constexpr std::uint32_t shadow_band_scale=0x46a2b8u;// float 7.0: shadow fade band per radius
inline constexpr std::uint32_t no_shadow_lod=0x45695cu;    // float FLT_MAX
inline constexpr std::uint32_t shadow_cutoff=0x46a498u;    // float 0.095
inline constexpr std::uint32_t byte_scale=0x46a148u;       // float 255.0
}
struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;       // +8
    std::uint32_t unused_here_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);
// The fields of an object read here.
struct Object {
    std::uint32_t definition;       // +0x00: the object definition's tag handle
    std::uint32_t flags;            // +0x04: bit 19: never fade
    std::uint8_t unused_here[0x34];
    float bounding_radius;          // +0x3C
};
static_assert(offsetof(Object,flags)==4 && offsetof(Object,bounding_radius)==0x3c);
inline constexpr unsigned never_fade_bit=19;
struct ObjectDefinition {
    std::uint8_t unused_here[0x38];
    std::uint32_t model;            // +0x38: the model (hlmt) tag's handle, NONE for none
};
static_assert(offsetof(ObjectDefinition,model)==0x38);
// The fade fields of a model definition (hlmt), from +0x28.
struct ModelFade {
    float disappear_distance;       // hlmt +0x28
    float begin_fade_distance;      // hlmt +0x2C
    std::uint32_t unused_here_30;
    float reduce_distance[5];       // hlmt +0x34: reduce to L1..L5
    std::uint32_t unused_here_48;
    std::uint16_t shadow_fade_level;// hlmt +0x4C: the level whose distance starts the shadow fade
};
inline constexpr std::uint32_t model_fade_offset=0x28u;
static_assert(offsetof(ModelFade,begin_fade_distance)==4 && offsetof(ModelFade,reduce_distance)==0xc &&
              offsetof(ModelFade,shadow_fade_level)==0x24);
// The shadow fade starts at reduce_distance[4 - shadow fade level], an index
// of at most 4; never for an index of 0 or less.
inline constexpr std::int32_t shadow_levels=4;

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    object_headers,object_rows,fades_disabled,object_data,object_flags,no_fade_store,
    object_fade_out,shadow_fade_out,clear_object_fade,clear_shadow_fade,full_object_fade,full_shadow_fade,
    object_definition_tag,tag_instances,object_definition,model_tag,model_definition,
    root_headers,root_rows,root_data,root_definition_tag,root_definition,root_model_tag,root_model_definition,
    shadow_fade_level,one_constant,bounding_radius,shadow_band_scale,shadow_fade_store,object_fade_store,
    shadow_lod,no_shadow_lod,
    faded_shadow_store,zero_shadow_store,full_shadow_store,shadow_cutoff,
    disappear_distance,zero_object_store,begin_fade_compare,disappear_again,begin_fade_excess,begin_fade_span,faded_object_store,
    object_fade_load,object_fade_scale,shadow_fade_load,object_fade_out_again,shadow_fade_scale,object_byte_store,
    shadow_fade_out_again,shadow_byte_store
};
// The floating-point operations (SSE single precision unless noted).
enum class Op : unsigned {
    shadow_band,            // radius * 7
    shadow_end,             // lod + band
    past_shadow_end,        // distance vs shadow end (COMISS)
    past_shadow_lod,        // distance vs lod
    shadow_excess,          // distance - lod
    shadow_ratio,           // excess / band
    shadow_fade,            // 1 - ratio
    shadow_negative,        // 0 vs fade
    shadow_above_one,       // fade vs 1
    shadow_cut,             // cutoff vs fade
    has_disappear,          // disappear distance vs 0
    disappeared,            // distance vs disappear distance
    past_begin_fade,        // distance vs begin fade distance
    object_excess,          // distance - begin
    object_span,            // disappear - begin
    object_ratio,           // excess / span
    object_fade,            // 1 - ratio
};
// The result of COMISS a,b: below (CF), equal (ZF), unordered (PF; then all
// three are set).
struct Ordering {bool below,equal,unordered;};
constexpr bool at_least(Ordering o){return !o.below;}            // JAE
constexpr bool above(Ordering o){return !o.below&&!o.equal;}     // JA

// The values the original keeps in registers across BCFB0 and _ftol2 (EBX,
// EBP, ESI, EDI, and the distance in XMM1 across BCFB0), by use.
struct Kept {
    float distance;             // the camera distance
    std::uint32_t handle;       // the object's handle
    std::uint32_t object;       // the object's data
    std::uint32_t fade;         // the ModelFade in use
    std::uint32_t tags;         // the loaded tag table
};
// How the original leaves (each exit restores a different set of registers).
enum class Exit {unfaded,no_model,faded};

namespace object_fade {
// Frame (the adapter):
//   object_handle(), distance()        the arguments in EBX and XMM1
//   argument(role)                     an output pointer from its slot
//   set_local(role, v) / store_local_byte(role, v)
//   root_object(k) -> handle           BCFB0 (the handle in ECX)
//   push_local(role)                   FLD of a frame float
//   to_integer(k) -> EAX               _ftol2 (32CC0C) of ST(0)
//   finish(exit)
// Arithmetic (the adapter): multiply/add/subtract/divide(op, a, b) -> float,
//   compare(op, a, b) -> Ordering, scale(role, address) (x87 FMUL m32).
// Memory: load8/load16/load32/load_float, store8.

// The definition of a loaded tag.
template<class Memory> std::uint32_t definition_of(Memory &memory,Access access,std::uint32_t tags,std::uint32_t tag) {
    return memory.load32(access,at(element(tags,handle_index(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
}

// The shadow fade (1 when it has not started).
template<class Memory,class Frame> void shadow_fade(Memory &memory,Frame &frame,float distance,float one,float band,float lod) {
    using enum Access;
    const auto end=frame.add(Op::shadow_end,lod,band);
    const float zero=0.0f;
    if(at_least(frame.compare(Op::past_shadow_end,distance,end)))return frame.set_local(zero_shadow_store,zero);
    if(!above(frame.compare(Op::past_shadow_lod,distance,lod)))return;
    const auto excess=frame.subtract(Op::shadow_excess,distance,lod);
    const auto ratio=frame.divide(Op::shadow_ratio,excess,band);
    const auto fade=frame.subtract(Op::shadow_fade,one,ratio);
    // The fade is stored before it is clamped.
    const auto negative=frame.compare(Op::shadow_negative,zero,fade);
    frame.set_local(faded_shadow_store,fade);
    if(above(negative))return frame.set_local(zero_shadow_store,zero);
    if(above(frame.compare(Op::shadow_above_one,fade,one)))return frame.set_local(full_shadow_store,one);
    const auto cutoff=memory.load_float(shadow_cutoff,globals::shadow_cutoff);
    // JB: kept above the cutoff (and when unordered).
    if(frame.compare(Op::shadow_cut,cutoff,fade).below)return;
    frame.set_local(zero_shadow_store,zero);
}

// The draw fade (1 when it has not started).
template<class Memory,class Frame> void draw_fade(Memory &memory,Frame &frame,const Kept &k,float distance,float one) {
    using enum Access;
    const float zero=0.0f;
    const auto disappear=memory.load_float(disappear_distance,at(k.fade,offsetof(ModelFade,disappear_distance)));
    if(!above(frame.compare(Op::has_disappear,disappear,zero)))return;
    if(!frame.compare(Op::disappeared,distance,disappear).below)return frame.set_local(zero_object_store,zero);
    const auto begin=memory.load_float(begin_fade_compare,at(k.fade,offsetof(ModelFade,begin_fade_distance)));
    if(!above(frame.compare(Op::past_begin_fade,distance,begin)))return;
    // The distances are read again for the arithmetic.
    const auto disappear_value=memory.load_float(disappear_again,at(k.fade,offsetof(ModelFade,disappear_distance)));
    const auto excess=frame.subtract(Op::object_excess,distance,memory.load_float(begin_fade_excess,at(k.fade,offsetof(ModelFade,begin_fade_distance))));
    const auto span=frame.subtract(Op::object_span,disappear_value,memory.load_float(begin_fade_span,at(k.fade,offsetof(ModelFade,begin_fade_distance))));
    const auto ratio=frame.divide(Op::object_ratio,excess,span);
    frame.set_local(faded_object_store,frame.subtract(Op::object_fade,one,ratio));
}

// 4BAE0(object in EBX, distance in XMM1; draw fade out, shadow fade out).
template<class Memory,class Frame> void fade(Memory &memory,Frame &frame) {
    using enum Access;
    Kept k{};
    k.handle=frame.object_handle();
    k.distance=frame.distance();
    const auto headers=memory.load32(object_headers,globals::object_headers);
    const auto rows=memory.load32(object_rows,at(headers,offsetof(DataArray,data)));
    const auto disabled=memory.load8(Access::fades_disabled,globals::fades_disabled);
    k.object=memory.load32(object_data,at(element(rows,handle_index(k.handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto never_fade=(memory.load32(object_flags,at(k.object,offsetof(Object,flags)))>>never_fade_bit)&1u;
    const auto unfaded=(never_fade|disabled)&0xffu;
    const auto object_out=frame.argument(object_fade_out);
    frame.store_local_byte(no_fade_store,unfaded);
    const auto shadow_out=frame.argument(shadow_fade_out);
    memory.store8(clear_object_fade,object_out,0);
    memory.store8(clear_shadow_fade,shadow_out,0);
    if(unfaded) {
        memory.store8(full_object_fade,object_out,0xff);
        memory.store8(full_shadow_fade,shadow_out,0xff);
        return frame.finish(Exit::unfaded);
    }
    // The object's own model.
    const auto definition_tag=memory.load32(object_definition_tag,at(k.object,offsetof(Object,definition)));
    k.tags=memory.load32(Access::tag_instances,globals::tag_instances);
    const auto definition=definition_of(memory,object_definition,k.tags,definition_tag);
    const auto model=memory.load32(model_tag,at(definition,offsetof(ObjectDefinition,model)));
    if(model==none)return frame.finish(Exit::no_model);
    k.fade=definition_of(memory,model_definition,k.tags,model)+model_fade_offset;
    // The root object's model, when the object has a parent and the root has one.
    const auto root=frame.root_object(k);
    if(root!=k.handle) {
        const auto root_headers_array=memory.load32(root_headers,globals::object_headers);
        const auto root_rows_array=memory.load32(root_rows,at(root_headers_array,offsetof(DataArray,data)));
        const auto root_object=memory.load32(root_data,at(element(root_rows_array,handle_index(root),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
        const auto root_definition_handle=memory.load32(root_definition_tag,at(root_object,offsetof(Object,definition)));
        const auto root_definition_data=definition_of(memory,root_definition,k.tags,root_definition_handle);
        const auto root_model=memory.load32(root_model_tag,at(root_definition_data,offsetof(ObjectDefinition,model)));
        if(root_model!=none)k.fade=definition_of(memory,root_model_definition,k.tags,root_model)+model_fade_offset;
    }
    const auto level=shadow_levels-std::int32_t(memory.load16(shadow_fade_level,at(k.fade,offsetof(ModelFade,shadow_fade_level))));
    const auto one=memory.load_float(one_constant,globals::one);
    const auto radius=memory.load_float(bounding_radius,at(k.object,offsetof(Object,bounding_radius)));
    const auto band=frame.multiply(Op::shadow_band,radius,memory.load_float(shadow_band_scale,globals::shadow_band_scale));
    frame.set_local(shadow_fade_store,one);
    frame.set_local(object_fade_store,one);
    const auto lod=level>0?memory.load_float(shadow_lod,at(k.fade,offsetof(ModelFade,reduce_distance)+std::uint32_t(level<shadow_levels?level:shadow_levels)*4u)):
                           memory.load_float(no_shadow_lod,globals::no_shadow_lod);
    shadow_fade(memory,frame,k.distance,one,band,lod);
    draw_fade(memory,frame,k,k.distance,one);
    // Each fade times 255 (x87), converted by _ftol2; the draw fade's byte is
    // stored after the shadow fade is loaded and its output read.
    frame.push_local(object_fade_load);
    frame.scale(object_fade_scale,globals::byte_scale);
    const auto object_byte=frame.to_integer(k);
    frame.push_local(shadow_fade_load);
    const auto object_target=frame.argument(object_fade_out_again);
    frame.scale(shadow_fade_scale,globals::byte_scale);
    memory.store8(object_byte_store,object_target,object_byte);
    const auto shadow_byte=frame.to_integer(k);
    const auto shadow_target=frame.argument(shadow_fade_out_again);
    memory.store8(shadow_byte_store,shadow_target,shadow_byte);
    frame.finish(Exit::faded);
}
} // namespace object_fade
} // namespace halo2::native_object_fade
