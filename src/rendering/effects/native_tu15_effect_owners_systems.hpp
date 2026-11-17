#pragma once
// Native TU1.5 effect systems (group E of
// docs/native_replacement/REMAINING_TRANSLATED_20261115.md):
//
//   lens_query_reset::run     20680   start of a lens query frame: when the
//                                     query stamp moved on, clear one flag of
//                                     every lens query record
//   light_fades::run          C35D0   the per-view distance fades of every
//                                     active light (fade_light, C3300, for one
//                                     light; distance_fade, C2A80, for one fade)
//   particle_inputs::run      176A40  the particle property inputs a mask asks
//                                     for (input_value, 252100, for one input)
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (each system's Access enum); the boundary adapter
// (native_tu15_effect_owners.hpp) maps the role to the original instruction
// for the diagnostic cursor and the store observer. Stack arguments, the
// locals the original keeps in its stack frame, the genuine child calls and
// the entries and returns of the inline callees come from a Frame object that
// the adapter also supplies. Registers, flags and stack slots belong to the
// adapter, not to this file.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the store
//     observer sees every write, a faulting read must be the original's first
//     fault, and verification replays every read in order). Where the
//     original reads a field again, so does this code;
//   - single-precision arithmetic runs the original SSE instructions with the
//     original operand order, so NaN payloads, rounding, DAZ/FTZ and the
//     MXCSR sticky flags are identical. Each operation also happens between
//     the same two memory accesses as in the original, so the sticky flags
//     match at a faulting access too;
//   - the light fade distance is the original x87 sequence, in extended
//     precision under the live control word, rounded once by its spill;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a call
//     are named "kept" structs, handed back by the adapter after the call, so
//     the logic continues exactly as the original does even when a callee does
//     not preserve them;
//   - the scratch values the original leaves in its SSE registers are
//     returned in each result (or passed with each child call), for the
//     adapter only.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2::native_effects {

// The data_array layout, its search and the datum handles are the object
// update systems' (native_tu15_object_tick_systems.hpp).
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::make_handle;
using native_object_update::DataArray;
using native_object_update::Search;
using native_object_update::next_in_use;
using native_object_update::datum_of;

// ------------------------------------------------------------- arithmetic
// Single-precision operations exactly as the original SSE instructions, on
// the live MXCSR. The first operand is the destination, so it owns a NaN
// payload when both operands are NaNs, and nothing is reordered or fused.
// Each is a compiler barrier, so it stays between the same memory accesses.
namespace sse {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline float multiply(float a,float b) {
    __m128 x=_mm_set_ss(a);asm volatile("mulss %1,%0":"+x"(x):"x"(_mm_set_ss(b)):"memory");return _mm_cvtss_f32(x);
}
inline float divide(float a,float b) {
    __m128 x=_mm_set_ss(a);asm volatile("divss %1,%0":"+x"(x):"x"(_mm_set_ss(b)):"memory");return _mm_cvtss_f32(x);
}
inline float add(float a,float b) {
    __m128 x=_mm_set_ss(a);asm volatile("addss %1,%0":"+x"(x):"x"(_mm_set_ss(b)):"memory");return _mm_cvtss_f32(x);
}
inline float subtract(float a,float b) {
    __m128 x=_mm_set_ss(a);asm volatile("subss %1,%0":"+x"(x):"x"(_mm_set_ss(b)):"memory");return _mm_cvtss_f32(x);
}
// COMISS a,b then JA: a > b and both ordered. COMISS also signals invalid
// for a quiet NaN, which the sticky flags record.
inline bool greater(float a,float b) {
    unsigned char result;
    asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc","memory");
    return result;
}
// CVTSI2SS: rounds on the live MXCSR.
inline float from_integer(std::int32_t value) {
    __m128 x=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(x):"r"(value):"memory");return _mm_cvtss_f32(x);
}
// CVTTSS2SI: truncates; NaN and out-of-range values give 0x80000000.
inline std::int32_t truncate(float value) {
    std::int32_t result;asm volatile("cvttss2si %1,%0":"=r"(result):"x"(_mm_set_ss(value)):"memory");return result;
}
#else
inline constexpr bool available=false;
[[noreturn]] inline void unavailable(){throw std::runtime_error("Native effect systems need the x86 SSE backend");}
inline float multiply(float,float){unavailable();}
inline float divide(float,float){unavailable();}
inline float add(float,float){unavailable();}
inline float subtract(float,float){unavailable();}
inline bool greater(float,float){unavailable();}
inline float from_integer(std::int32_t){unavailable();}
inline std::int32_t truncate(float){unavailable();}
#endif
} // namespace sse

// The x87 part of the light fade (C2A80), on the host x87 under the live
// control word (so precision and rounding control apply, as in the original).
namespace x87 {
// A value rounded to single precision by FSTP m32, with that store's C1
// (set when the rounding went up), which the adapter publishes.
struct Single {
    float value;
    bool rounded_up;
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
// sqrt((z*z + x*x) + y*y) - radius - class_distance for the offset
// (position - camera) per axis, in that order. Every single-precision operand
// is loaded with FLD m32 and then used from a register, as the retained
// translated route does: a hardware FSUB m32 raises no #D for a denormal
// beside a NaN, so the sticky flags would differ. The ST(0)-destination forms
// avoid the AT&T FSUBR ambiguity.
inline Single fade_distance(float position_x,float camera_x,float position_y,float camera_y,float position_z,
                            float camera_z,float radius,float class_distance) {
    float result;std::uint16_t status;
    asm volatile(
        "flds %[px]\n\tflds %[cx]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "flds %[py]\n\tflds %[cy]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "flds %[pz]\n\tflds %[cz]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "fld %%st(0)\n\tfmul %%st(1),%%st\n\t"
        "fld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\t"
        "flds %[radius]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "flds %[offset]\n\tfsubr %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "fstps %[result]\n\tfnstsw %[status]"
        :[result]"=m"(result),[status]"=m"(status)
        :[px]"m"(position_x),[cx]"m"(camera_x),[py]"m"(position_y),[cy]"m"(camera_y),[pz]"m"(position_z),
         [cz]"m"(camera_z),[radius]"m"(radius),[offset]"m"(class_distance)
        :"st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","memory");
    return {result,(status&0x200u)!=0};
}
#else
inline constexpr bool available=false;
inline Single fade_distance(float,float,float,float,float,float,float,float) {
    throw std::runtime_error("Native effect systems need the x86 x87 backend");
}
#endif
} // namespace x87

// ---------------------------------------------------------- lens queries
// 20680, at the start of each lens query frame. The lens query stamp
// ([4922C0], the low word of a 64-bit count that 13EF0 advances after every
// submitted query batch) is compared with the stamp of the last reset. A new
// stamp starts a new query frame: bit 1 of every query record's flags is
// cleared, the stamp is remembered, and two bytes record that this frame
// reset and whether it issues hardware queries. Otherwise only the "reset"
// byte is cleared.
namespace lens_query_reset {
namespace globals {
inline constexpr std::uint32_t query_stamp=0x4922c0u;     // int32: the stamp's low word
inline constexpr std::uint32_t view_mode=0x4920b8u;       // int32: modes 4..7 issue no hardware queries
inline constexpr std::uint32_t reset_stamp=0x532080u;     // int32: the stamp of the last reset, 0 before the first
inline constexpr std::uint32_t frame_reset=0x52dfd8u;     // byte: 1 when this frame reset the records
inline constexpr std::uint32_t queries_issued=0x52dfd9u;  // byte: the reset frame issues hardware queries
inline constexpr std::uint32_t records=0x52dfdcu;         // LensQueryRecord[record_count]
inline constexpr std::uint32_t current_record=0x532094u;  // int32: set to NONE every frame (meaning not established)
}
inline constexpr std::uint32_t record_count=511;

// One lens query (52DFDC + 32 * index). 20E30 retires a record by zeroing it
// and setting its flags to FF80; 13EF0's rollback restores its area.
struct LensQueryRecord {
    std::uint32_t unknown_00;
    std::uint16_t flags;          // +4: query_flags
    std::uint16_t area;           // +6: the query's screen area
    std::uint8_t unknown_08[24];
};
static_assert(sizeof(LensQueryRecord)==32 && offsetof(LensQueryRecord,flags)==4 && offsetof(LensQueryRecord,area)==6);
namespace query_flags {
inline constexpr std::uint32_t frame=0x2u;  // cleared for every record when a query frame starts (meaning not established)
}
// The view modes without hardware lens queries (source_lens_query_frame_tu15.cpp
// issues them only outside 4..7).
constexpr bool queries_off(std::int32_t mode){return mode>=4 && mode<=7;}

enum class Access {
    stamp,mode,last_stamp,
    record_word,record_word_store,
    last_stamp_store,reset_set,queries_store,current_on_reset,
    reset_clear,current_otherwise
};
// What the reset read and decided, for the adapter.
struct Result {
    bool reset;
    std::int32_t stamp;
    std::optional<std::int32_t> mode,last;  // read only for a non-negative stamp
};
template<class Memory> Result run(Memory &memory) {
    using enum Access;
    Result result{false,signed_value(memory.load32(stamp,globals::query_stamp)),{},{}};
    // A negative stamp never starts a query frame.
    if(result.stamp>=0) {
        result.mode=signed_value(memory.load32(mode,globals::view_mode));
        result.last=signed_value(memory.load32(last_stamp,globals::reset_stamp));
        // A later stamp, or the first one, starts a query frame. Without
        // hardware queries the same stamp starts one again too.
        result.reset=(queries_off(*result.mode) && result.stamp==*result.last) || result.stamp>*result.last ||
                     *result.last==0;
    }
    if(!result.reset) {
        memory.store8(reset_clear,globals::frame_reset,0u);
        memory.store32(current_otherwise,globals::current_record,none);
        return result;
    }
    // The flags and the area are read and written together, as one word.
    for(std::uint32_t index=0;index<record_count;++index) {
        const auto word=at(element(globals::records,index,sizeof(LensQueryRecord)),offsetof(LensQueryRecord,flags));
        memory.store32(record_word_store,word,memory.load32(record_word,word)&~query_flags::frame);
    }
    memory.store32(last_stamp_store,globals::reset_stamp,std::uint32_t(result.stamp));
    memory.store8(reset_set,globals::frame_reset,1u);
    memory.store8(queries_store,globals::queries_issued,queries_off(*result.mode)?0u:1u);
    memory.store32(current_on_reset,globals::current_record,none);
    return result;
}
} // namespace lens_query_reset

// ------------------------------------------------------------ light fades
// C35D0, once per view: every active light gets five distance fades from the
// current camera, which the light draw uses (effect_light.hpp names them;
// the native presentation, source_effect_light_view_fades, computes the same
// values):
//
//   fade       distance class        range               stored at
//   overall    definition +B8        10                  +100
//   diffuse    definition +BA        5                   +F8
//   specular   definition +BA        the radius          +FC
//   secondary  definition +BC        1                   +104
//   volume     0                     half the radius     +108
//
// then diffuse = min(diffuse, overall) and secondary = min(secondary,
// diffuse). Each fade is 1 near the light, falling linearly to 0 over its
// range once the camera is further than the class's distance from the
// light's surface (distance_fade).
namespace light_fades {
namespace globals {
inline constexpr std::uint32_t lights=0x4ecbdcu;              // DataArray* of LightDatum
inline constexpr std::uint32_t tag_instances=0x4f0414u;       // TagInstance*: indexed by a tag handle's low word
inline constexpr std::uint32_t camera_position=0x4c65c0u;     // float[3]: the current view's camera
inline constexpr std::uint32_t minimum_range=0x46a178u;       // float 0.1: the smallest fade range
inline constexpr std::uint32_t overall_range=0x46a110u;       // float 10
inline constexpr std::uint32_t diffuse_range=0x46a100u;       // float 5
inline constexpr std::uint32_t one=0x46a0d0u;                 // float 1: the secondary range, and the fade's 1 - x
inline constexpr std::uint32_t volume_range_scale=0x46a0ccu;  // float 0.5
// Each fade's class distances: five floats, indexed by the distance class.
inline constexpr std::uint32_t overall_distances=0x44a6d8u;
inline constexpr std::uint32_t diffuse_distances=0x44a6ecu;
inline constexpr std::uint32_t specular_distances=0x44a700u;
inline constexpr std::uint32_t secondary_distances=0x44a714u;
inline constexpr std::uint32_t volume_distances=0x44a728u;
}

// A tag instance: the loaded definition's address at +8.
struct TagInstance {
    std::uint8_t unknown_00[8];
    std::uint32_t address;
    std::uint32_t unknown_0c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,address)==8);

// A light definition: the three authored distance classes (0..4).
struct LightDefinition {
    std::uint8_t unknown_00[0xb8];
    std::int16_t overall_class;    // +B8
    std::int16_t diffuse_class;    // +BA: also the specular fade's
    std::int16_t secondary_class;  // +BC
};
static_assert(offsetof(LightDefinition,overall_class)==0xb8 && offsetof(LightDefinition,diffuse_class)==0xba &&
              offsetof(LightDefinition,secondary_class)==0xbc);

// A light datum (0x110 bytes) of the light data array. The original
// addresses it with this fixed size, not the array's datum size.
struct LightDatum {
    std::uint16_t identifier;       // the handle's high word
    std::uint8_t flags;             // +2: light_flags
    std::uint8_t unknown_03;
    std::uint32_t definition;       // +4: the definition's tag handle
    std::uint8_t unknown_08[0x20];
    float position[3];              // +28
    float radius;                   // +34
    std::uint8_t unknown_38[0xc0];
    float diffuse_fade;             // +F8
    float specular_fade;            // +FC
    float overall_fade;             // +100
    float secondary_fade;           // +104
    float volume_fade;              // +108
    std::uint8_t unknown_10c[4];
};
static_assert(sizeof(LightDatum)==0x110 && offsetof(LightDatum,flags)==2 && offsetof(LightDatum,definition)==4 &&
              offsetof(LightDatum,position)==0x28 && offsetof(LightDatum,radius)==0x34 &&
              offsetof(LightDatum,diffuse_fade)==0xf8 && offsetof(LightDatum,specular_fade)==0xfc &&
              offsetof(LightDatum,overall_fade)==0x100 && offsetof(LightDatum,secondary_fade)==0x104 &&
              offsetof(LightDatum,volume_fade)==0x108);
namespace light_flags {
inline constexpr std::uint32_t active=0x2u;  // the light is live; only active lights fade
}

// ---- distance_fade (C2A80)
enum class Fade : unsigned {overall,diffuse,specular,secondary,volume};
// One fade's inputs. The original passes the position in EAX, the class in
// ECX, the range in XMM2, and the radius and the class distance table as
// stack arguments (FadeCall is what the caller passes; the callee reads the
// two stack arguments back through the frame).
struct FadeCall {
    Fade fade;
    std::uint32_t position;        // the light's position
    std::int32_t distance_class;
    float range;
    float radius;
    std::uint32_t distances;       // the class distance table
};
// Where the distance fell: before the range starts (fade 1), past its end
// (fade 0) or within it.
enum class FadeEnd {inside,beyond,within};
// A fade and the scratch values C2A80 leaves in its SSE registers (for the
// adapter).
struct FadeResult {
    float fade;            // XMM0: 1 - clamped
    float clamped;         // XMM1: the ratio as clamped to [0, 1] (0, 1 or the ratio)
    float range;           // XMM2: the call's range, or the divisor when within
    float divisor;         // XMM3: 0 when inside, otherwise the divisor
    float minimum;         // XMM4
    float ratio;           // XMM5
    FadeEnd end;
    bool range_divides;    // the range (not the minimum) was the divisor
};
enum class FadeAccess {position_x,minimum,camera_x,position_y,camera_y,position_z,camera_z,class_distance,one};

// C2A80, one fade:
//   distance = |position - camera| - radius - distances[class]
//   fade     = 1 - clamp(distance / max(range, minimum), 0, 1)
// Both decisions are COMISS: an unordered comparison is "not greater", so a
// NaN range divides by the minimum and a NaN ratio gives 1 - ratio. The
// original repeats the range comparison and the division before each later
// decision; the repeats have the same operands, so they give the same values
// and raise no further MXCSR flags, and are not repeated here.
// Frame: enter_fade(call) (the CALL), distance_table() and radius() (the
// stack arguments), spill_distance() and reload_distance() (the original
// spills the distance over the radius argument and reloads it for the SSE
// part), return_from_fade(result) (RET 8).
template<class Memory,class Frame> FadeResult distance_fade(Memory &memory,Frame &frame,const FadeCall &call) {
    using enum FadeAccess;
    frame.enter_fade(call);
    const auto x=memory.load_float(position_x,at(call.position,0));
    const auto minimum_range=memory.load_float(minimum,globals::minimum_range);
    // Decided before the camera is read, as in the original.
    const bool range_divides=sse::greater(call.range,minimum_range);
    const auto camera_x_value=memory.load_float(camera_x,globals::camera_position);
    const auto y=memory.load_float(position_y,at(call.position,4));
    const auto camera_y_value=memory.load_float(camera_y,globals::camera_position+4u);
    const auto z=memory.load_float(position_z,at(call.position,8));
    const auto distances=frame.distance_table();
    const auto camera_z_value=memory.load_float(camera_z,globals::camera_position+8u);
    const auto radius=frame.radius();
    const auto offset=memory.load_float(class_distance,element(distances,std::uint32_t(call.distance_class),4));
    frame.spill_distance(x87::fade_distance(x,camera_x_value,y,camera_y_value,z,camera_z_value,radius,offset));
    const auto distance=frame.reload_distance();
    const auto divisor=range_divides?call.range:minimum_range;
    const auto ratio=sse::divide(distance,divisor);
    const float zero=0.0f;
    // Decided before 1.0 is read, as in the original.
    const bool inside=sse::greater(zero,ratio);
    const auto one_value=memory.load_float(one,globals::one);
    FadeResult result;
    if(inside)result={sse::subtract(one_value,zero),zero,call.range,zero,minimum_range,ratio,FadeEnd::inside,range_divides};
    else if(sse::greater(ratio,one_value))
        result={sse::subtract(one_value,one_value),one_value,call.range,divisor,minimum_range,ratio,FadeEnd::beyond,range_divides};
    else result={sse::subtract(one_value,ratio),ratio,divisor,divisor,minimum_range,ratio,FadeEnd::within,range_divides};
    frame.return_from_fade(result);
    return result;
}

// ---- fade_light (C3300)
enum class LightAccess {
    lights,datums,tag_table,overall_range,definition_tag,definition,
    overall_radius,overall_class,
    diffuse_radius,diffuse_range,overall_store,diffuse_class,
    specular_radius,specular_range,diffuse_store,specular_class,
    secondary_radius,secondary_range,specular_store,secondary_class,
    volume_radius,volume_range,volume_range_scale,secondary_store,
    volume_store,diffuse_limit,diffuse_reload,diffuse_copy,secondary_limit
};
// The scratch values C3300 leaves in its SSE registers (for the adapter).
struct LightResidue {
    float overall;          // XMM6
    float diffuse;          // XMM7: the diffuse fade before its limit
    float secondary;        // XMM0: the secondary fade as reloaded from the frame
    FadeResult volume;      // XMM1..XMM5: the last fade's scratch values
    bool diffuse_limited;   // diffuse > overall
    bool secondary_limited; // secondary > the stored diffuse
};

// C3300: the five fades of one light (handle), into its datum.
// Frame: pass_fade_arguments(call) (the stack arguments, before each fade),
// keep_secondary(value) and reload_secondary() (C3300 keeps the secondary
// fade in its stack frame across the volume fade), and distance_fade's.
template<class Memory,class Frame> LightResidue fade_light(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum LightAccess;
    const auto light_array=memory.load32(lights,globals::lights);
    const auto data=memory.load32(datums,at(light_array,offsetof(DataArray,data)));
    const auto tags=memory.load32(tag_table,globals::tag_instances);
    const auto overall_range_value=memory.load_float(overall_range,globals::overall_range);
    const auto light=element(data,handle_index(handle),sizeof(LightDatum));
    const auto tag=memory.load32(definition_tag,at(light,offsetof(LightDatum,definition)));
    const auto light_definition=memory.load32(definition,at(element(tags,handle_index(tag),sizeof(TagInstance)),
                                                             offsetof(TagInstance,address)));
    const auto position=at(light,offsetof(LightDatum,position));
    // The radius is read again for every use.
    const auto radius=[&](LightAccess access){return memory.load_float(access,at(light,offsetof(LightDatum,radius)));};
    const auto distance_class=[&](LightAccess access,std::size_t field) {
        return signed_value(widen16(memory.load16(access,at(light_definition,field))));
    };
    const auto fade_field=[&](std::size_t field){return at(light,field);};

    // Each fade: its radius argument, its range, the previous fade's store,
    // then its distance class, in the original's order.
    FadeCall call{Fade::overall,position,0,overall_range_value,radius(overall_radius),globals::overall_distances};
    call.distance_class=distance_class(overall_class,offsetof(LightDefinition,overall_class));
    frame.pass_fade_arguments(call);
    const auto overall=distance_fade(memory,frame,call).fade;

    call={Fade::diffuse,position,0,0.0f,radius(diffuse_radius),globals::diffuse_distances};
    call.range=memory.load_float(diffuse_range,globals::diffuse_range);
    memory.store_float(overall_store,fade_field(offsetof(LightDatum,overall_fade)),overall);
    call.distance_class=distance_class(diffuse_class,offsetof(LightDefinition,diffuse_class));
    frame.pass_fade_arguments(call);
    const auto diffuse=distance_fade(memory,frame,call).fade;

    call={Fade::specular,position,0,0.0f,radius(specular_radius),globals::specular_distances};
    call.range=radius(specular_range);
    memory.store_float(diffuse_store,fade_field(offsetof(LightDatum,diffuse_fade)),diffuse);
    call.distance_class=distance_class(specular_class,offsetof(LightDefinition,diffuse_class));
    frame.pass_fade_arguments(call);
    const auto specular=distance_fade(memory,frame,call).fade;

    call={Fade::secondary,position,0,0.0f,radius(secondary_radius),globals::secondary_distances};
    call.range=memory.load_float(secondary_range,globals::one);
    memory.store_float(specular_store,fade_field(offsetof(LightDatum,specular_fade)),specular);
    call.distance_class=distance_class(secondary_class,offsetof(LightDefinition,secondary_class));
    frame.pass_fade_arguments(call);
    const auto secondary=distance_fade(memory,frame,call).fade;

    // The volume fade uses class 0 and half the radius as its range. Its
    // stack arguments go first; then the secondary fade is kept in the
    // frame and stored (the original's order).
    call={Fade::volume,position,0,0.0f,radius(volume_radius),globals::volume_distances};
    const auto volume_radius_value=radius(volume_range);
    call.range=sse::multiply(volume_radius_value,memory.load_float(volume_range_scale,globals::volume_range_scale));
    frame.pass_fade_arguments(call);
    frame.keep_secondary(secondary);
    memory.store_float(secondary_store,fade_field(offsetof(LightDatum,secondary_fade)),secondary);
    const auto volume=distance_fade(memory,frame,call);

    LightResidue residue{overall,diffuse,0.0f,volume,false,false};
    // diffuse = min(diffuse, overall): decided before the volume store.
    residue.diffuse_limited=sse::greater(diffuse,overall);
    memory.store_float(volume_store,fade_field(offsetof(LightDatum,volume_fade)),volume.fade);
    if(residue.diffuse_limited)memory.store_float(diffuse_limit,fade_field(offsetof(LightDatum,diffuse_fade)),overall);
    // secondary = min(secondary, diffuse), against the diffuse fade as stored;
    // the limit is copied as an integer.
    residue.secondary=frame.reload_secondary();
    const auto stored_diffuse=memory.load_float(diffuse_reload,fade_field(offsetof(LightDatum,diffuse_fade)));
    residue.secondary_limited=sse::greater(residue.secondary,stored_diffuse);
    if(residue.secondary_limited) {
        const auto bits=memory.load32(diffuse_copy,fade_field(offsetof(LightDatum,diffuse_fade)));
        memory.store32(secondary_limit,fade_field(offsetof(LightDatum,secondary_fade)),bits);
    }
    return residue;
}

// ---- run (C35D0)
enum class Access {
    lights,
    first_size,first_data,first_identifier,
    datums,flags,
    scan_limit,scan_bits,scan_word,
    next_size,next_data,next_identifier
};
// The walk, kept across the per-light fades: the light array and the
// current light's handle and index. C3300 saves the handle and index in its
// stack frame and restores them; the adapter hands back what it restored.
struct LightWalk {
    std::uint32_t lights;
    std::uint32_t handle;
    std::uint32_t index;
};
// Frame: first_in_use(walk) (16EA60, the genuine child: the first in-use
// index of the array, or NONE; the array comes back in walk), light_faded(
// walk, residue) (C3300's return), searched(search) (the next-light search's
// end, for the adapter).
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    LightWalk walk{memory.load32(lights,globals::lights),none,none};
    const auto first=frame.first_in_use(walk);
    if(first!=none) {
        const auto datum=datum_of(memory,walk.lights,first,first_size,first_data);
        walk.handle=make_handle(memory.load16(first_identifier,datum),first);
    }
    while(walk.handle!=none) {
        walk.index=handle_index(walk.handle);
        const auto data=memory.load32(datums,at(walk.lights,offsetof(DataArray,data)));
        const auto light=element(data,walk.index,sizeof(LightDatum));
        if(memory.load8(flags,at(light,offsetof(LightDatum,flags)))&light_flags::active)
            frame.light_faded(walk,fade_light(memory,frame,walk.handle));
        // The next in-use light: the search starts after this light's index.
        const auto search=next_in_use(memory,walk.lights,walk.handle==none?none:walk.index,scan_limit,scan_bits,scan_word);
        frame.searched(search);
        if(search.end) {
            walk.handle=none;
            break;
        }
        const auto datum=datum_of(memory,walk.lights,search.index,next_size,next_data);
        walk.handle=make_handle(memory.load16(next_identifier,datum),search.index);
    }
}
} // namespace light_fades

// --------------------------------------------------------- particle inputs
// 176A40: a particle's property inputs. For every bit set in the mask,
// lowest first, outputs[input] = input_value(input). The inputs follow the
// particle tags' 17-entry property input list (the names below; each entry's
// field agrees with its name).
namespace particle_inputs {
namespace globals {
inline constexpr std::uint32_t game_time=0x51df54u;     // GameTime*
inline constexpr std::uint32_t effects=0x4f720cu;       // DataArray* of EffectDatum
inline constexpr std::uint32_t camera=0x491e38u;        // Camera: the current view (source_world_particle_draws_tu15.cpp)
inline constexpr std::uint32_t word_scale=0x46a160u;    // float: a random word to [0, 1]
inline constexpr std::uint32_t byte_scale=0x46a0c4u;    // float: a random byte to [0, 1]
inline constexpr std::uint32_t one=0x46a0d0u;           // float 1
}
enum class Input : std::uint32_t {
    particle_age,particle_emit_time,particle_random_1,particle_random_2,
    emitter_age,emitter_random_1,emitter_random_2,
    system_lod,game_time,effect_a_scale,effect_b_scale,
    particle_rotation,explosion_animation,explosion_rotation,
    particle_random_3,particle_random_4,location_random
};
inline constexpr std::uint32_t input_count=17;
// The original's code for an input, chosen through its jump table (the
// adapter reads it). Neighbouring inputs of one kind share code, which
// addresses the field from the input number.
enum class Reader {
    particle_age,particle_emit_time,particle_random_1_2,emitter_age,emitter_random,system_lod,game_time,
    effect_a_scale,effect_b_scale,particle_rotation,explosion_animation,explosion_rotation,particle_random_3_4,
    location_random
};

struct Particle {
    std::uint8_t unknown_00[8];
    float age;                    // +8
    std::uint8_t unknown_0c[4];
    float emit_time;              // +10
    std::uint16_t random[4];      // +14: random 1..4
    float position[3];            // +1C
    std::uint8_t unknown_28[0x10];
    float rotation;               // +38: the rotation input is its fractional part; the two
                                  // explosion inputs scale their projections by it
};
static_assert(offsetof(Particle,age)==8 && offsetof(Particle,emit_time)==0x10 && offsetof(Particle,random)==0x14 &&
              offsetof(Particle,position)==0x1c && offsetof(Particle,rotation)==0x38);
// Random 1 and 2 are addressed as +10 + 2 * input (inputs 2, 3), random 3
// and 4 as 2 * input - 4 (inputs 14, 15).
inline constexpr std::uint32_t particle_random_1_2_base=0x10u,particle_random_3_4_base=0xfffffffcu;

struct Emitter {
    std::uint8_t unknown_00[4];
    float age;                    // +4
    std::uint8_t unknown_08[0x10];
    std::uint32_t effect;         // +18: the owning effect's handle, NONE for none
    std::uint8_t unknown_1c[0xc];
    float random[2];              // +28: random 1, 2, addressed as +14 + 4 * input (inputs 5, 6)
};
static_assert(offsetof(Emitter,age)==4 && offsetof(Emitter,effect)==0x18 && offsetof(Emitter,random)==0x28);
inline constexpr std::uint32_t emitter_random_base=0x14u;

// The record 176A40's callers pass third: the system LOD and the location
// random byte (what else it holds is not established here).
struct Location {
    std::uint8_t unknown_00[3];
    std::uint8_t random;          // +3
    std::uint8_t unknown_04[0x18];
    float lod;                    // +1C
};
static_assert(offsetof(Location,random)==3 && offsetof(Location,lod)==0x1c);

// An effect datum (0x190 bytes) of the effect data array.
struct EffectDatum {
    std::uint8_t unknown_00[0x6c];
    float a_scale;                // +6C
    float b_scale;                // +70
    std::uint8_t unknown_74[0x11c];
};
static_assert(sizeof(EffectDatum)==0x190 && offsetof(EffectDatum,a_scale)==0x6c && offsetof(EffectDatum,b_scale)==0x70);

struct GameTime {
    std::uint8_t unknown_00[4];
    float seconds_per_tick;       // +4
    std::int32_t elapsed_ticks;   // +8
};
static_assert(offsetof(GameTime,seconds_per_tick)==4 && offsetof(GameTime,elapsed_ticks)==8);

struct Camera {
    float position[3];
    float forward[3];             // +C
    float up[3];                  // +18
};
static_assert(offsetof(Camera,forward)==0xc && offsetof(Camera,up)==0x18);

// A direction vector (three floats) in memory: the original builds it in its
// frame for the normalize child (4EFA0).
struct Vector {
    float x,y,z;
};

enum class Access {
    particle_age,particle_emit_time,random_word,emitter_age,emitter_random,system_lod,
    game_time,elapsed_ticks,seconds_per_tick,word_scale,
    effect,effect_table,effect_data,effect_scale,
    rotation,rotation_spill,rotation_reload,rotation_again,
    position_x,camera_x,offset_x,position_y,camera_y,offset_y,position_z,camera_z,offset_z,
    up_y,forward_z,forward_y,up_z,left_x,up_x,forward_x,left_y,left_z,
    normalized_z,normalized_y,normalized_x,left_z_again,left_y_again,left_x_again,
    projection_rotation,projection_spill,projection_reload,
    location_random_byte,byte_scale,one,
    output
};

// The records the inputs read.
struct Records {
    std::uint32_t particle,location,emitter;
};
// Kept across 252100 (whose explosion inputs call the genuine 4EFA0): the
// pending mask, the outputs, the input and the particle.
struct Kept {
    std::uint32_t remaining;
    std::uint32_t outputs;
    std::uint32_t input;
    std::uint32_t particle;
};
// The SSE scratch values at each 4EFA0 call (for the adapter).
struct AnimationScratch {
    float offset_z;
};
struct RotationScratch {
    float left_y,left_z,up_y_forward_x,forward_z_up_x,forward_x,up_x;
};

// How input_value ended, for the adapter (the original has one return per
// reader and outcome).
enum class Exit {past_table,no_record,value,no_effect,wrapped,unwrapped};
struct InputResult {
    float value;                          // XMM0
    std::optional<Reader> reader;         // past the table: none
    Exit exit;
    std::optional<std::int32_t> truncated;  // the fraction's integer part (EAX)
    std::optional<float> converted;         // the rotation's integer part as a float (XMM2 lane 0)
};

// The integer part of x, truncated. The original spills x to its frame and
// truncates the reloaded copy.
template<class Memory,class Frame> std::int32_t truncate_through_frame(Memory &memory,Frame &frame,float value,
                                                                      Access spill,Access reload) {
    memory.store_float(spill,frame.local_scratch(),value);
    return sse::truncate(memory.load_float(reload,frame.local_scratch()));
}
// A fraction x - trunc(x), wrapped into [0, 1): COMISS 0, x, so a negative
// fraction (not a NaN) moves up by 1.
template<class Memory> InputResult wrap(Memory &memory,InputResult result,Reader reader) {
    if(sse::greater(0.0f,result.value)) {
        result.value=sse::add(result.value,memory.load_float(Access::one,globals::one));
        result.exit=Exit::wrapped;
    } else result.exit=Exit::unwrapped;
    result.reader=reader;
    return result;
}
// 252100: one input's value. Inputs past the table, and inputs whose record
// is missing (null), give 0.
// Frame: reader(input) (the original's jump table), local_vector(),
// local_left(), local_scratch() (its frame's locals), normalize(vector,
// kept, scratch) (4EFA0), and enter_value()/return_from_value(result) (the
// CALL and the RET).
template<class Memory,class Frame> InputResult input_value(Memory &memory,Frame &frame,const Records &records,Kept &kept) {
    using enum Access;
    const auto input=kept.input;
    frame.enter_value();
    InputResult result{0.0f,std::nullopt,Exit::past_table,std::nullopt,std::nullopt};
    const auto finish=[&](InputResult r){frame.return_from_value(r);return r;};
    const auto value=[&](Reader reader,float v){result.value=v;result.reader=reader;result.exit=Exit::value;return finish(result);};
    const auto missing=[&](Reader reader){result.reader=reader;result.exit=Exit::no_record;return finish(result);};
    // A random word, scaled to [0, 1].
    const auto scaled_word=[&](std::uint32_t base) {
        const auto word=memory.load16(random_word,element(records.particle+base,input,2));
        return sse::multiply(sse::from_integer(std::int32_t(word)),memory.load_float(word_scale,globals::word_scale));
    };
    if(input>=input_count)return finish(result);
    const auto reader=frame.reader(input);
    switch(reader) {
    case Reader::particle_age:
        if(!records.particle)return missing(reader);
        return value(reader,memory.load_float(Access::particle_age,at(records.particle,offsetof(Particle,age))));
    case Reader::particle_emit_time:
        if(!records.particle)return missing(reader);
        return value(reader,memory.load_float(Access::particle_emit_time,at(records.particle,offsetof(Particle,emit_time))));
    case Reader::particle_random_1_2:
        if(!records.particle)return missing(reader);
        return value(reader,scaled_word(particle_random_1_2_base));
    case Reader::particle_random_3_4:
        if(!records.particle)return missing(reader);
        return value(reader,scaled_word(particle_random_3_4_base));
    case Reader::emitter_age:
        if(!records.emitter)return missing(reader);
        return value(reader,memory.load_float(Access::emitter_age,at(records.emitter,offsetof(Emitter,age))));
    case Reader::emitter_random:
        if(!records.emitter)return missing(reader);
        return value(reader,memory.load_float(Access::emitter_random,element(records.emitter+emitter_random_base,input,4)));
    case Reader::system_lod:
        if(!records.location)return missing(reader);
        return value(reader,memory.load_float(Access::system_lod,at(records.location,offsetof(Location,lod))));
    case Reader::game_time: {
        // The elapsed ticks times the seconds per tick.
        const auto time=memory.load32(Access::game_time,globals::game_time);
        const auto ticks=sse::from_integer(signed_value(memory.load32(elapsed_ticks,at(time,offsetof(GameTime,elapsed_ticks)))));
        return value(reader,sse::multiply(ticks,memory.load_float(seconds_per_tick,at(time,offsetof(GameTime,seconds_per_tick)))));
    }
    case Reader::effect_a_scale:
    case Reader::effect_b_scale: {
        // The emitter record is not tested for null here.
        const auto handle=memory.load32(effect,at(records.emitter,offsetof(Emitter,effect)));
        if(handle==none) {
            result.reader=reader;result.exit=Exit::no_effect;
            return finish(result);
        }
        const auto effects=memory.load32(effect_table,globals::effects);
        const auto data=memory.load32(effect_data,at(effects,offsetof(DataArray,data)));
        const auto datum=element(data,handle_index(handle),sizeof(EffectDatum));
        const auto field=reader==Reader::effect_a_scale?offsetof(EffectDatum,a_scale):offsetof(EffectDatum,b_scale);
        return value(reader,memory.load_float(effect_scale,at(datum,field)));
    }
    case Reader::particle_rotation: {
        if(!records.particle)return missing(reader);
        // The fractional part of the rotation (read again for the subtraction).
        const auto rotation_value=memory.load_float(rotation,at(records.particle,offsetof(Particle,rotation)));
        result.truncated=truncate_through_frame(memory,frame,rotation_value,rotation_spill,rotation_reload);
        const auto again=memory.load_float(rotation_again,at(records.particle,offsetof(Particle,rotation)));
        result.converted=sse::from_integer(*result.truncated);
        result.value=sse::subtract(again,*result.converted);
        return finish(wrap(memory,result,reader));
    }
    case Reader::explosion_animation: {
        // The projection on the camera's forward axis:
        // dot(normalize(particle - camera), forward) * rotation, wrapped into
        // [0, 1). The offset goes to the frame's vector, which the genuine
        // 4EFA0 normalizes in place; each component is stored before the
        // next is read.
        if(!records.particle || !records.location)return missing(reader);
        const auto vector=frame.local_vector();
        const auto px=memory.load_float(position_x,at(records.particle,offsetof(Particle,position)));
        memory.store_float(offset_x,at(vector,offsetof(Vector,x)),sse::subtract(px,memory.load_float(camera_x,globals::camera)));
        const auto py=memory.load_float(position_y,at(records.particle,offsetof(Particle,position)+4u));
        memory.store_float(offset_y,at(vector,offsetof(Vector,y)),sse::subtract(py,memory.load_float(camera_y,globals::camera+4u)));
        const auto pz=memory.load_float(position_z,at(records.particle,offsetof(Particle,position)+8u));
        const auto offset_z_value=sse::subtract(pz,memory.load_float(camera_z,globals::camera+8u));
        memory.store_float(offset_z,at(vector,offsetof(Vector,z)),offset_z_value);
        frame.normalize(vector,kept,AnimationScratch{offset_z_value});
        // The vector as normalized, against the axis: (z*fz + y*fy) + fx*x.
        const auto normalized=frame.local_vector();
        const auto z=memory.load_float(normalized_z,at(normalized,offsetof(Vector,z)));
        const auto z_term=sse::multiply(z,memory.load_float(forward_z,globals::camera+offsetof(Camera,forward)+8u));
        const auto y=memory.load_float(normalized_y,at(normalized,offsetof(Vector,y)));
        const auto y_term=sse::multiply(y,memory.load_float(forward_y,globals::camera+offsetof(Camera,forward)+4u));
        auto sum=sse::add(z_term,y_term);
        const auto fx=memory.load_float(forward_x,globals::camera+offsetof(Camera,forward));
        sum=sse::add(sum,sse::multiply(fx,memory.load_float(normalized_x,at(normalized,offsetof(Vector,x)))));
        sum=sse::multiply(sum,memory.load_float(projection_rotation,at(kept.particle,offsetof(Particle,rotation))));
        result.truncated=truncate_through_frame(memory,frame,sum,projection_spill,projection_reload);
        result.value=sse::subtract(sum,sse::from_integer(*result.truncated));
        return finish(wrap(memory,result,reader));
    }
    case Reader::explosion_rotation: {
        // The same projection on the camera's left axis (up x forward), which
        // the original builds in its frame next to the vector.
        if(!records.particle || !records.location)return missing(reader);
        const auto vector=frame.local_vector();
        const auto px=memory.load_float(position_x,at(records.particle,offsetof(Particle,position)));
        const auto offset_x_value=sse::subtract(px,memory.load_float(camera_x,globals::camera));
        // The original loads three axis components before storing x.
        const auto up_y_value=memory.load_float(up_y,globals::camera+offsetof(Camera,up)+4u);
        const auto forward_z_value=memory.load_float(forward_z,globals::camera+offsetof(Camera,forward)+8u);
        const auto forward_y_value=memory.load_float(forward_y,globals::camera+offsetof(Camera,forward)+4u);
        memory.store_float(offset_x,at(vector,offsetof(Vector,x)),offset_x_value);
        const auto py=memory.load_float(position_y,at(records.particle,offsetof(Particle,position)+4u));
        memory.store_float(offset_y,at(vector,offsetof(Vector,y)),sse::subtract(py,memory.load_float(camera_y,globals::camera+4u)));
        const auto pz=memory.load_float(position_z,at(records.particle,offsetof(Particle,position)+8u));
        memory.store_float(offset_z,at(vector,offsetof(Vector,z)),sse::subtract(pz,memory.load_float(camera_z,globals::camera+8u)));
        // left = up x forward; the original's operand order in each product.
        const auto up_z_value=memory.load_float(up_z,globals::camera+offsetof(Camera,up)+8u);
        const auto up_z_forward_y=sse::multiply(up_z_value,forward_y_value);
        const auto left_x_value=sse::subtract(sse::multiply(up_y_value,forward_z_value),up_z_forward_y);
        const auto up_x_value=memory.load_float(up_x,globals::camera+offsetof(Camera,up));
        const auto left=frame.local_left();
        memory.store_float(Access::left_x,at(left,offsetof(Vector,x)),left_x_value);
        const auto forward_x_value=memory.load_float(forward_x,globals::camera+offsetof(Camera,forward));
        const auto up_z_forward_x=sse::multiply(up_z_value,forward_x_value);
        const auto forward_z_up_x=sse::multiply(forward_z_value,up_x_value);
        const auto forward_y_up_x=sse::multiply(forward_y_value,up_x_value);
        const auto up_y_forward_x=sse::multiply(up_y_value,forward_x_value);
        const auto left_y_value=sse::subtract(up_z_forward_x,forward_z_up_x);
        const auto left_z_value=sse::subtract(forward_y_up_x,up_y_forward_x);
        memory.store_float(Access::left_y,at(left,offsetof(Vector,y)),left_y_value);
        memory.store_float(Access::left_z,at(left,offsetof(Vector,z)),left_z_value);
        frame.normalize(vector,kept,RotationScratch{left_y_value,left_z_value,up_y_forward_x,forward_z_up_x,forward_x_value,up_x_value});
        // The vector as normalized, against left: (lz*z + ly*y) + lx*x.
        const auto normalized=frame.local_vector();
        const auto left_again=frame.local_left();
        const auto lz=memory.load_float(left_z_again,at(left_again,offsetof(Vector,z)));
        const auto z_term=sse::multiply(lz,memory.load_float(normalized_z,at(normalized,offsetof(Vector,z))));
        const auto ly=memory.load_float(left_y_again,at(left_again,offsetof(Vector,y)));
        const auto y_term=sse::multiply(ly,memory.load_float(normalized_y,at(normalized,offsetof(Vector,y))));
        auto sum=sse::add(z_term,y_term);
        const auto lx=memory.load_float(left_x_again,at(left_again,offsetof(Vector,x)));
        sum=sse::add(sum,sse::multiply(lx,memory.load_float(normalized_x,at(normalized,offsetof(Vector,x)))));
        sum=sse::multiply(sum,memory.load_float(projection_rotation,at(kept.particle,offsetof(Particle,rotation))));
        result.truncated=truncate_through_frame(memory,frame,sum,projection_spill,projection_reload);
        result.value=sse::subtract(sum,sse::from_integer(*result.truncated));
        return finish(wrap(memory,result,reader));
    }
    case Reader::location_random: {
        if(!records.location)return missing(reader);
        const auto byte=memory.load8(location_random_byte,at(records.location,offsetof(Location,random)));
        return value(reader,sse::multiply(sse::from_integer(std::int32_t(byte)),memory.load_float(byte_scale,globals::byte_scale)));
    }
    }
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}

// 176A40. Arguments: the mask, the emitter, the location, the particle and
// the outputs (float[input_count]). The original reads the three records
// again for every input, keeps the pending mask in the mask argument's slot
// (its bit scan reads it there) and reuses the outputs argument's slot for
// the current input.
// Frame: mask(), outputs(), particle(), location(), emitter() (the
// arguments), clear_input(), keep_input(i), input() (the input slot),
// pending() and keep_pending(kept) (the mask slot), unscanned_input() (see
// below), and input_value's.
// Returns whether any input was evaluated.
template<class Memory,class Frame> bool run(Memory &memory,Frame &frame) {
    Kept kept{};
    kept.remaining=frame.mask();
    kept.outputs=frame.outputs();
    frame.clear_input();
    if(!kept.remaining)return false;
    do {
        // BSF: the lowest pending input. BSF leaves its destination unchanged
        // when the mask slot is empty, so the input is then what the scan
        // register still holds; only a 4EFA0 that does not preserve the
        // pending mask register can get there.
        const auto pending=frame.pending();
        frame.keep_input(pending?std::uint32_t(std::countr_zero(pending)):frame.unscanned_input());
        kept.input=frame.input();
        Records records{};
        records.particle=frame.particle();
        records.location=frame.location();
        records.emitter=frame.emitter();
        kept.particle=records.particle;
        kept.remaining&=~(1u<<(kept.input&31u));
        frame.keep_pending(kept);
        const auto result=input_value(memory,frame,records,kept);
        memory.store_float(Access::output,element(kept.outputs,kept.input,4),result.value);
    } while(kept.remaining);
    return true;
}
} // namespace particle_inputs
} // namespace halo2::native_effects
