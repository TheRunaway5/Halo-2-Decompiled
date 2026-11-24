#pragma once
// Native TU1.5 object shadow fade, entered at 3D7E0 (shadow frame unit):
// how far an object's shadow is faded in, and whether it is shown.
//
//   shadow_fade::evaluate   3D7E0(object, float *fade, bool *selector_twelve)
//
//   1. An object flagged at +6 bit 0 is not measured (fade 0).
//   2. Its definition's distance mode (+0x18) picks how: mode 2 measures
//      from the object's bounding sphere center; mode 0 (only for the
//      selector values 0 and 1 at +0xAA) from the position 30C40 gives.
//      The distance to the camera (x87: FSUB of the camera position, the
//      squared sum, FSQRT, times a scale, FSTP to single) goes to 4BAE0,
//      which writes a level byte; the fade is that level times a scale.
//   3. A measured fade above zero shows the object. With a selector of 0,
//      an object flagged at +0x10A bit 2 fades in with its age (one minus
//      (now - created ticks) * seconds per tick minus a delay, clamped to
//      zero and one; zero hides it), and the fade is multiplied by one minus
//      the object's camouflage (+0x2B0) when that lies within the age floor
//      and one.
//   4. An object that is not shown takes its parent's fade and shown state
//      (BCFB0 gives the parent; 3D7E0 recurses).
//   5. Bipeds and vehicles fade by one minus their camouflage clamped to
//      zero and one; a fade not above zero hides them.
//   6. *fade is the fade; *selector_twelve is whether the selector is 12.
// The result (AL) is whether the object is shown.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_shadow_fade.hpp) maps the role to the original instruction
// and owns the ABI of the children (30C40, 4BAE0, BCFB0 and 3D7E0 itself).
// The arguments and the frame (the fade, the selector, the position, the
// level byte; the object argument's slot also carries the scaled distance
// and 4BAE0's level) are read and written as memory through Frame::local,
// in the original's order.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses in the original order and width, including the
//     object data and camouflage read again for each comparison;
//   - COMISS/UCOMISS comparisons and scalar SSE on the live MXCSR, and the
//     x87 distance on the live control word, in the original's operand
//     order;
//   - the register file each child and the caller see (the adapter, from
//     the stage outcomes reported through the Frame).
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace halo2::native_shadow_fade {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;

// ------------------------------------------------------------- arithmetic
namespace scalar {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
// CVTSI2SS of a 32-bit integer.
inline std::uint32_t from_integer(std::uint32_t x) {
    __m128 r=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(r):"r"(std::int32_t(x)));return sse::bits(r);
}
// UCOMISS a,b; LAHF; TEST AH,44; JP: ordered and equal.
inline bool ordered_equal(std::uint32_t a,std::uint32_t b) {
    unsigned char zero,parity;
    asm volatile("ucomiss %3,%2\n\tsetz %0\n\tsetp %1":"=qm"(zero),"=qm"(parity):"x"(sse::scalar(a)),"x"(sse::scalar(b)):"cc");
    return zero && !parity;
}
// The x87 distance: FLD/FSUB m32 for x, y and z; FLD ST0; FMUL ST1; FLD
// ST2; FMUL ST3; FADDP; FLD ST3; FMUL ST4; FADDP; FSQRT; FSTP ST3; FSTP
// ST0 twice; FMUL m32; FSTP m32, with the status after the store. As in the
// retained route, each m32 operand is loaded (FLD m32, which quiets a
// signalling NaN) before it is used, so two NaNs combine by significand.
struct Rounded {std::uint32_t value;std::uint16_t store_status;};
inline Rounded scaled_distance(const std::uint32_t (&position)[3],const std::uint32_t (&camera)[3],std::uint32_t scale) {
    Rounded r{};
    asm volatile(
        "flds %5\n\tflds %2\n\tfsub %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "flds %6\n\tflds %3\n\tfsub %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "flds %7\n\tflds %4\n\tfsub %%st(1),%%st\n\tfstp %%st(1)\n\t"
        "fld %%st(0)\n\tfmul %%st(1),%%st\n\t"
        "fld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\t"
        "flds %8\n\tfmul %%st(1),%%st\n\tfstp %%st(1)\n\tfstps %0\n\tfnstsw %1"
        :"=m"(r.value),"=m"(r.store_status)
        :"m"(position[0]),"m"(position[1]),"m"(position[2]),"m"(camera[0]),"m"(camera[1]),"m"(camera[2]),"m"(scale)
        :"st","st(1)","st(2)","st(3)","st(4)","st(5)");
    return r;
}
#else
inline constexpr bool available=false;
inline std::uint32_t from_integer(std::uint32_t){throw std::runtime_error("Required TU1.5 CVTSI2SS backend unavailable");}
inline bool ordered_equal(std::uint32_t,std::uint32_t){throw std::runtime_error("Required TU1.5 UCOMISS backend unavailable");}
struct Rounded {std::uint32_t value;std::uint16_t store_status;};
inline Rounded scaled_distance(const std::uint32_t (&)[3],const std::uint32_t (&)[3],std::uint32_t) {
    throw std::runtime_error("Required TU1.5 x87 distance backend unavailable");
}
#endif
} // namespace scalar

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances; +8 a tag's data
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of object headers
inline constexpr std::uint32_t camera_position=0x4c65c0u;  // float[3]
inline constexpr std::uint32_t distance_scale=0x4c65ecu;   // float
inline constexpr std::uint32_t level_scale=0x46a0c4u;      // float: the fade of one level step
inline constexpr std::uint32_t zero=0x46a0e8u;             // float 0.0
inline constexpr std::uint32_t one=0x46a0d0u;              // float 1.0
inline constexpr std::uint32_t age_delay=0x46a11cu;        // float subtracted from the age
inline constexpr std::uint32_t game_time=0x51df54u;        // GameTime*
}
struct GameTime {std::uint32_t unused_here;float seconds_per_tick;std::int32_t tick;};
static_assert(offsetof(GameTime,seconds_per_tick)==4 && offsetof(GameTime,tick)==8);
inline constexpr std::uint32_t datum_data=0x44u;
inline constexpr std::uint32_t header_type=3u,header_data=8u,header_size=12u;
inline constexpr std::uint32_t tag_size=16u,tag_data=8u;
// The object data fields read here.
namespace object {
inline constexpr std::uint32_t definition=0x0u;     // tag handle
inline constexpr std::uint32_t flags=0x6u;          // bit 0: not measured
inline constexpr std::uint32_t parent=0x14u;        // object handle
inline constexpr std::uint32_t center=0x30u;        // float[3]
inline constexpr std::uint32_t selector=0xaau;      // int8; the game meaning is not established
inline constexpr std::uint32_t created=0xbcu;       // tick
inline constexpr std::uint32_t fade_flags=0x10au;   // bit 2: fades in with its age
inline constexpr std::uint32_t camouflage=0x2b0u;   // float
}
inline constexpr std::uint32_t definition_distance_mode=0x18u;   // int16
namespace object_type {inline constexpr std::uint32_t units=0x03u;}   // biped, vehicle
inline constexpr std::uint32_t selector_twelve=12u;

namespace shadow_fade {
enum class Local {
    object,            // the object handle; then the scaled distance, 4BAE0's level, a cleared byte
    fade_output,       // float*
    selector_output,   // bool*
    fade,              // the fade so far
    selector,          // the sign-extended selector
    cleared,           // a float cleared before the unit fade
    position,          // float[3]
    level_byte         // 4BAE0's first output
};
enum class Access {
    tag_instances,object_argument,headers,header_rows,object_data,definition_handle,definition,selector,
    fade_clear,object_flags,selector_store,distance_mode,
    // the distance, mode 2 (center) and mode 0 (30C40)
    center_object,center_x,center_y,center_z,position_x_store,position_x,camera_x,position_y_store,position_y,
    position_z_store,camera_y,position_z,camera_z,distance_scale,distance_store,distance_load,level,
    position_x_child,camera_x_child,position_y_child,camera_y_child,position_z_child,camera_z_child,
    distance_scale_child,distance_store_child,distance_load_child,level_child,
    // the fade
    level_scale,fade_threshold,fade_store,selector_check,fade_flags,one,game_time,now,created,seconds_per_tick,
    age_delay,age_store,age_floor_store,age_one_store,one_unmeasured,
    rows_again,object_again,camouflage_floor,camouflage_ceiling,camouflage_value,camouflage_equal,
    camouflage_fraction,camouflage_store,
    // the parent
    parent,parent_selector_output,parent_fade,headers_again,one_after,
    // the unit fade and the outputs
    rows_final,type,object_clear,cleared_store,unit_object,unit_camouflage,selector_final,fade_output,
    fade_result_store,selector_output,selector_result_store
};
// The roles of one distance measurement.
struct DistanceRoles {Access x,camera_x,y,camera_y,z,camera_z,scale,store,load,level;};
inline constexpr DistanceRoles center_roles{Access::position_x,Access::camera_x,Access::position_y,Access::camera_y,
    Access::position_z,Access::camera_z,Access::distance_scale,Access::distance_store,Access::distance_load,Access::level};
inline constexpr DistanceRoles position_roles{Access::position_x_child,Access::camera_x_child,Access::position_y_child,
    Access::camera_y_child,Access::position_z_child,Access::camera_z_child,Access::distance_scale_child,
    Access::distance_store_child,Access::distance_load_child,Access::level_child};
// How a camouflage value (at +0x2B0) clamps: to the floor, to one, or kept.
enum class Clamp {floor,ceiling,value};
// The values the original keeps in callee-saved registers across children.
struct Kept {
    std::uint32_t handle;         // EBX: the object handle
    std::uint32_t headers;        // EBP: the object header table
    std::uint32_t data;           // ESI: the object's data
    std::uint32_t header_offset;  // EDI: the object's header offset (index * 12)
};
inline std::uint32_t widen8(std::uint32_t x){return std::uint32_t(std::int32_t(std::int8_t(x)));}
inline std::uint32_t widen16(std::uint32_t x){return std::uint32_t(std::int32_t(std::int16_t(x)));}

// Frame:
//   local(Local)                  an argument's or local's address now
//   position(kept)                30C40 writes the object's position into
//                                 Local::position
//   rounded(Rounded)              the x87 distance and its store ran
//   leveled(kept, center)         4BAE0 writes the level into Local::object
//   scaled(fade)                  the level times the level scale
//   shown(bool)                   the shown state (DL) was written
//   aged(timed, fade)             the age stage ran (zero floor; the fade
//                                 when timed)
//   camouflage_clamped(Clamp, v)  the camouflage clamp ran
//   camouflage_faded(fade)        the camouflage fraction applied
//   parent_shown(kept)            BCFB0 then 3D7E0 for the parent; returns AL
//   unit_faded(Clamp, v, fade, shown)  the unit fade ran
// The child hooks refresh kept from the registers the child returned.
template<class Memory,class Frame> bool evaluate(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{};
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    kept.handle=memory.load32(object_argument,local(Local::object));
    kept.headers=memory.load32(headers,globals::object_headers);
    kept.header_offset=(kept.handle&0xffffu)*header_size;
    const auto data_field=memory.load32(header_rows,at(kept.headers,datum_data))+kept.header_offset+header_data;
    kept.data=memory.load32(object_data,data_field);
    const auto definition_tag=memory.load32(definition_handle,at(kept.data,object::definition));
    const auto definition_data=memory.load32(definition,(definition_tag&0xffffu)*tag_size+instances+tag_data);
    const auto selector_value=widen8(memory.load8(selector,at(kept.data,object::selector)));
    std::uint32_t fade=0,one_value=0;
    memory.store32(fade_clear,local(Local::fade),fade);
    const bool unmeasured=memory.load8(object_flags,at(kept.data,object::flags))&1u;
    memory.store32(selector_store,local(Local::selector),selector_value);
    bool shown=false,measured=false,to_parent=true;
    // 2. The distance and the level.
    if(!unmeasured) {
        const auto mode=std::int32_t(widen16(memory.load16(distance_mode,at(definition_data,definition_distance_mode))));
        if(mode==2 || (mode==0 && (selector_value==0 || selector_value==1))) {
            const bool from_center=mode==2;
            const auto &roles=from_center?center_roles:position_roles;
            const auto position_at=[&](unsigned axis){return local(Local::position)+4u*axis;};
            std::uint32_t position[3],camera[3];
            if(from_center) {
                const auto center=at(memory.load32(center_object,data_field),object::center);
                position[0]=memory.load32(center_x,center);
                position[1]=memory.load32(center_y,center+4u);
                position[2]=memory.load32(center_z,center+8u);
                memory.store32(position_x_store,position_at(0),position[0]);
                position[0]=memory.load32(position_x,position_at(0));
                camera[0]=memory.load32(camera_x,globals::camera_position);
                memory.store32(position_y_store,position_at(1),position[1]);
                position[1]=memory.load32(position_y,position_at(1));
                memory.store32(position_z_store,position_at(2),position[2]);
                camera[1]=memory.load32(camera_y,globals::camera_position+4u);
                position[2]=memory.load32(position_z,position_at(2));
                camera[2]=memory.load32(camera_z,globals::camera_position+8u);
            } else {
                frame.position(kept);
                position[0]=memory.load32(position_x_child,position_at(0));
                camera[0]=memory.load32(camera_x_child,globals::camera_position);
                position[1]=memory.load32(position_y_child,position_at(1));
                camera[1]=memory.load32(camera_y_child,globals::camera_position+4u);
                position[2]=memory.load32(position_z_child,position_at(2));
                camera[2]=memory.load32(camera_z_child,globals::camera_position+8u);
            }
            const auto distance=scalar::scaled_distance(position,camera,memory.load32(roles.scale,globals::distance_scale));
            frame.rounded(distance);
            memory.store32(roles.store,local(Local::object),distance.value);
            (void)memory.load32(roles.load,local(Local::object));
            frame.leveled(kept,from_center);
            const auto level_value=memory.load8(roles.level,local(Local::object));
            fade=sse::multiply(scalar::from_integer(level_value),memory.load32(level_scale,globals::level_scale));
            frame.scaled(fade);
            measured=!sse::not_above(fade,memory.load32(fade_threshold,globals::zero));
            memory.store32(fade_store,local(Local::fade),fade);
        }
    }
    // 3. A measured fade shows the object; its age and camouflage fade it.
    if(measured) {
        const auto selector_now=memory.load32(selector_check,local(Local::selector));
        shown=true;frame.shown(shown);
        if(selector_now!=0) {
            one_value=memory.load32(one_after,globals::one);
            to_parent=false;
        } else {
            const auto fade_flag_bits=memory.load8(fade_flags,at(kept.data,object::fade_flags));
            one_value=memory.load32(one,globals::one);
            const bool timed=(fade_flag_bits>>2)&1u;
            if(timed) {
                const auto time=memory.load32(game_time,globals::game_time);
                const auto now_tick=memory.load32(now,at(time,offsetof(GameTime,tick)));
                const auto ticks=now_tick-memory.load32(created,at(kept.data,object::created));
                auto age=sse::multiply(scalar::from_integer(ticks),
                                       memory.load32(seconds_per_tick,at(time,offsetof(GameTime,seconds_per_tick))));
                age=sse::subtract(age,memory.load32(age_delay,globals::age_delay));
                fade=sse::subtract(one_value,age);
                const bool negative=!sse::not_above(0u,fade);
                memory.store32(age_store,local(Local::fade),fade);
                if(negative) {
                    fade=0;memory.store32(age_floor_store,local(Local::fade),fade);shown=false;
                } else if(!sse::not_above(fade,one_value)) {
                    fade=one_value;memory.store32(age_one_store,local(Local::fade),fade);shown=true;
                } else shown=!sse::not_above(fade,0u);
                frame.shown(shown);
            }
            frame.aged(timed,fade);
            // The camouflage, clamped to the age floor (zero) and one.
            const auto again=memory.load32(object_again,
                memory.load32(rows_again,at(kept.headers,datum_data))+kept.header_offset+header_data);
            const auto camouflage_at=at(again,object::camouflage);
            Clamp clamp;std::uint32_t clamped;
            if(!sse::not_above(0u,memory.load32(camouflage_floor,camouflage_at))){clamp=Clamp::floor;clamped=0;}
            else if(!sse::not_above(memory.load32(camouflage_ceiling,camouflage_at),one_value)){clamp=Clamp::ceiling;clamped=one_value;}
            else {clamp=Clamp::value;clamped=memory.load32(camouflage_value,camouflage_at);}
            frame.camouflage_clamped(clamp,clamped);
            if(scalar::ordered_equal(clamped,memory.load32(camouflage_equal,camouflage_at))) {
                fade=sse::multiply(sse::subtract(one_value,memory.load32(camouflage_fraction,camouflage_at)),fade);
                frame.camouflage_faded(fade);
                memory.store32(camouflage_store,local(Local::fade),fade);
            }
            to_parent=!shown;
        }
    } else {
        one_value=memory.load32(one_unmeasured,globals::one);
        shown=false;frame.shown(shown);
    }
    // 4. Not shown: the parent's fade and shown state.
    if(to_parent && memory.load32(parent,at(kept.data,object::parent))!=none) {
        (void)memory.load32(parent_selector_output,local(Local::selector_output));
        shown=(frame.parent_shown(kept)&0xffu)!=0;
        fade=memory.load32(parent_fade,local(Local::fade));
        kept.headers=memory.load32(headers_again,globals::object_headers);
        one_value=memory.load32(one_after,globals::one);
    }
    // 5. Bipeds and vehicles fade by their camouflage.
    const auto header=memory.load32(rows_final,at(kept.headers,datum_data))+kept.header_offset;
    const auto type_bit=1u<<(memory.load8(type,header+header_type)&31u);
    if(type_bit&object_type::units) {
        memory.store8(object_clear,local(Local::object),0);
        memory.store32(cleared_store,local(Local::cleared),0);
        const auto camouflage_value_now=memory.load32(unit_camouflage,
            at(memory.load32(unit_object,header+header_data),object::camouflage));
        Clamp clamp;std::uint32_t clamped;
        if(!sse::not_above(0u,camouflage_value_now)){clamp=Clamp::floor;clamped=0;}
        else if(!sse::not_above(camouflage_value_now,one_value)){clamp=Clamp::ceiling;clamped=one_value;}
        else {clamp=Clamp::value;clamped=camouflage_value_now;}
        fade=sse::multiply(sse::subtract(one_value,clamped),fade);
        if(!sse::below(0u,fade))shown=false;
        frame.unit_faded(clamp,clamped,fade,shown);
    }
    // 6. The outputs.
    const auto selector_now=memory.load32(selector_final,local(Local::selector));
    memory.store32(fade_result_store,memory.load32(fade_output,local(Local::fade_output)),fade);
    memory.store8(selector_result_store,memory.load32(selector_output,local(Local::selector_output)),
                  selector_now==selector_twelve);
    return shown;
}
} // namespace shadow_fade
} // namespace halo2::native_shadow_fade
