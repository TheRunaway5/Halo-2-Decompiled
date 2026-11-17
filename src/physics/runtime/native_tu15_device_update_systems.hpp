#pragma once
// Native TU1.5 device update systems: the update callbacks the object type
// chain (10DD10) runs for devices
// (docs/native_replacement/1179C0_10B840_device_update.md):
//
//   device_update::run   10B840  a device's position and power channels
//                                (machines, controls and light fixtures):
//                                scripted animation, or each channel
//                                approaching its group's value, with the
//                                device's open/close effects
//   machine_update::run  1179C0  a machine's own update: the power of its
//                                physics cluster, a gear's turning, a door's
//                                unit query, portal occlusion and close
//                                timer, then the notifications of a moved
//                                machine
//
// This is the game logic only, on the layouts of
// native_tu15_object_tick_systems.hpp and the ones below. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (each system's Access enum)
// and the boundary adapter (native_tu15_device_update.hpp) maps the role to
// the original instruction. The argument, the locals the original keeps in
// its stack frame and the child calls come from a Frame object that the
// adapter also supplies. Registers, flags, stack slots and the x87 register
// stack belong to the adapter.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (statements are
//     ordered so that no expression holds two accesses), including the
//     constants the original reloads after a child call;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - single-precision work runs the original SSE instruction with the
//     original operands in the original order on the live MXCSR (the
//     destination operand first: it owns a NaN payload; COMISS and UCOMISS
//     as the original chose them, so the sticky invalid flag is the same);
//     floats are handled as their bit patterns, so payloads pass through;
//   - the extended-precision work (the turning rate, the |value| tests) runs
//     the original x87 operations on the host x87 under its live control
//     word, so rounding and the sticky exception flags are the original's;
//   - the values the original keeps in callee-saved registers across a child
//     call are named in each system's "kept" struct. A conforming callee
//     returns them unchanged; the adapter hands back what the callee actually
//     returned, so the logic continues exactly as the original does.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_device_update {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::ObjectData;
using native_object_update::ClusterRecord;
using native_object_update::GameGlobals;
namespace globals=native_object_update::globals;

// ------------------------------------------------------- single precision
// The original's scalar SSE on the live MXCSR, on bit patterns. Each
// operation is one asm statement, so it stays between the same child calls.
namespace sse {
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// ADDSS/SUBSS/MULSS/DIVSS a,b: a is the destination.
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("addss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("subss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("mulss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("divss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
// CVTSI2SS: a signed integer rounded to single precision.
inline std::uint32_t from_integer(std::uint32_t value) {
    auto x=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(x):"r"(value));return bits(x);
}
// COMISS a,b; JA: a > b, ordered (a NaN is never above).
inline bool above(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
// COMISS a,b; JB: a < b, or unordered.
inline bool below(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
// UCOMISS a,b; LAHF; TEST AH,44; JP: equal, ordered (a NaN is never equal).
inline bool equal(std::uint32_t a,std::uint32_t b) {
    unsigned char zero,parity;
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1":"=qm"(zero),"=qm"(parity):"x"(scalar(a)),"x"(scalar(b)):"cc");
    return zero && !parity;
}
#else
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(_mm_add_ss(scalar(a),scalar(b)));}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){return bits(_mm_sub_ss(scalar(a),scalar(b)));}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(scalar(a),scalar(b)));}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){return bits(_mm_div_ss(scalar(a),scalar(b)));}
inline std::uint32_t from_integer(std::uint32_t value){return bits(_mm_cvtsi32_ss(_mm_setzero_ps(),int(value)));}
inline bool above(std::uint32_t a,std::uint32_t b){return _mm_comigt_ss(scalar(a),scalar(b));}
inline bool below(std::uint32_t a,std::uint32_t b){return !_mm_comige_ss(scalar(a),scalar(b));}
inline bool equal(std::uint32_t a,std::uint32_t b){return _mm_ucomieq_ss(scalar(a),scalar(b)) && a==a;}
#endif
} // namespace sse

// ----------------------------------------------------- extended precision
// The original's x87 work on the host x87 under its live control word.
namespace x87 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// FLD m32: a single widened to extended precision (a signaling NaN is
// quieted and raises invalid; a denormal raises denormal).
inline long double load(std::uint32_t bits){long double x;asm volatile("flds %1":"=t"(x):"m"(bits));return x;}
// FCOMIP a,b; JA: a > b, ordered (any NaN raises invalid and is not above).
inline bool above(long double a,long double b) {
    unsigned char result;
    asm volatile("fldt %2\n\tfldt %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)":"=qm"(result):"m"(a),"m"(b):"cc","st");
    return result;
}
#else
inline long double load(std::uint32_t){throw std::runtime_error("Native device update x87 backend unavailable");}
inline bool above(long double,long double){throw std::runtime_error("Native device update x87 backend unavailable");}
#endif
inline long double magnitude(long double x){return std::fabs(x);}                     // FABS
inline long double difference(long double a,long double b){volatile long double z=a-b;return z;}  // FSUB
inline long double product(long double a,long double b){volatile long double z=a*b;return z;}     // FMUL
inline long double sum(long double a,long double b){volatile long double z=a+b;return z;}         // FADD
} // namespace x87

// ------------------------------------------------------------- game layouts
namespace device_globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // TagInstance[]
inline constexpr std::uint32_t device_groups=0x4ecbf8u;    // DataArray* of DeviceGroup
inline constexpr std::uint32_t game_time=0x51df54u;        // GameTime*
inline constexpr std::uint32_t one=0x46a0d0u;              // float 1.0
inline constexpr std::uint32_t zero=0x46a0e8u;             // float 0.0
inline constexpr std::uint32_t epsilon=0x46a0ecu;          // float 0.0001
inline constexpr std::uint32_t openness_scale=0x46a1b4u;   // float 65535.0
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;    // the current structure BSP's tag data
inline constexpr std::uint32_t portal_openness=0x4f9b58u;  // PortalOpenness*
inline constexpr std::uint32_t portal_open_bits=0x505e7cu; // one bit per cluster portal (the portal mask)
}
// The game clock ([51DF54]).
struct GameTime {
    std::uint32_t unused_here;
    std::uint32_t tick_length;   // +4: float, seconds per tick
    std::int32_t tick;           // +8
};
static_assert(offsetof(GameTime,tick_length)==4 && offsetof(GameTime,tick)==8);
// A tag instance (16 bytes) of the [4F0414] table.
struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;    // +8: the tag's data
    std::uint32_t unused_here_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);
// A device group (12 bytes) of the [4ECBF8] array: the value its devices
// approach.
struct DeviceGroup {
    std::uint16_t identifier;
    std::uint16_t flags;
    std::uint32_t value;         // +4: float
    std::uint32_t initial;       // +8: float
};
static_assert(sizeof(DeviceGroup)==12 && offsetof(DeviceGroup,value)==4);
// The current structure BSP's portal list: the BSP's +0x220 block holds, at
// +0xC, the cluster portal index of each machine portal entry.
struct StructureBsp {
    std::uint8_t unused_here[0x220];
    std::uint32_t machine_portals;   // +0x220: MachinePortals*
};
struct MachinePortals {
    std::uint8_t unused_here[0xc];
    std::uint32_t portal_indices;    // +0xC: int16[]
};
static_assert(offsetof(StructureBsp,machine_portals)==0x220 && offsetof(MachinePortals,portal_indices)==0xc);
// The portal openness words ([4F9B58]): one word per machine portal entry.
struct PortalOpenness {
    std::uint8_t unused_here[0x28];
    std::uint16_t openness[1];       // +0x28
};
static_assert(offsetof(PortalOpenness,openness)==0x28);

// A device object's data: the device and machine fields these systems use,
// after the object's (ObjectData).
struct Device {
    std::uint8_t unused_here_00[0x28];
    std::uint32_t location[2];         // +0x28: the object's location (cluster at +0x2C)
    std::uint32_t center[3];           // +0x30: float, the bounding sphere's center
    std::uint32_t radius;              // +0x3C: float, the bounding sphere's radius
    std::uint8_t unused_here_40[0x80];
    std::uint16_t object_flags;        // +0xC0: object_flags (the word over ObjectData's two flag bytes)
    std::uint8_t unused_here_c2[0x12];
    std::uint32_t network_entity;      // +0xD4: the simulation entity, or NONE
    std::uint8_t unused_here_d8[0x52];
    std::int16_t animation_offset;     // +0x12A: the animation block (ObjectData's TimerBlock), from
                                       // the object; -1 for none
    std::uint32_t device_flags;        // +0x12C: device_flags
    std::uint32_t power_group;         // +0x130: DeviceGroup handle, or NONE
    std::uint32_t power;               // +0x134: float
    std::uint32_t power_velocity;      // +0x138: float
    std::uint32_t position_group;      // +0x13C: DeviceGroup handle, or NONE
    std::uint32_t position;            // +0x140: float
    std::uint32_t position_velocity;   // +0x144: float
    std::int16_t delay_ticks;          // +0x148: ticks waited before the position moves
    std::uint8_t unused_here_14a[2];
    std::uint8_t position_script[0x20];   // +0x14C: 10CD70's scripted position state
    std::uint8_t power_script[0x20];      // +0x16C: 10CD70's scripted power state
    std::uint8_t position_channel[0x20];  // +0x18C: AnimationChannel of the position
    std::uint8_t power_channel[0x20];     // +0x1AC: AnimationChannel of the power
    // A machine's part.
    std::uint32_t machine_flags;       // +0x1CC: machine_flags
    std::int32_t open_ticks;           // +0x1D0: a door's ticks fully open; -3 when it opens
    std::uint8_t unused_here_1d4[6];
    std::int16_t first_portal;         // +0x1DA: a door's first machine portal entry
    std::int16_t portal_count;         // +0x1DC
};
static_assert(offsetof(Device,location)==0x28 && offsetof(Device,center)==0x30 && offsetof(Device,radius)==0x3c &&
              offsetof(Device,object_flags)==0xc0 && offsetof(Device,network_entity)==0xd4 &&
              offsetof(Device,animation_offset)==0x12a && offsetof(Device,device_flags)==0x12c &&
              offsetof(Device,power_group)==0x130 && offsetof(Device,power)==0x134 && offsetof(Device,power_velocity)==0x138 &&
              offsetof(Device,position_group)==0x13c && offsetof(Device,position)==0x140 &&
              offsetof(Device,position_velocity)==0x144 && offsetof(Device,delay_ticks)==0x148 &&
              offsetof(Device,position_script)==0x14c && offsetof(Device,power_script)==0x16c &&
              offsetof(Device,position_channel)==0x18c && offsetof(Device,power_channel)==0x1ac &&
              offsetof(Device,machine_flags)==0x1cc && offsetof(Device,open_ticks)==0x1d0 &&
              offsetof(Device,first_portal)==0x1da && offsetof(Device,portal_count)==0x1dc);
// An animation channel (0x20 bytes): an animation of a graph, or none.
struct AnimationChannel {
    std::uint32_t graph;               // NONE: no graph
    std::uint16_t unused_here;
    std::int16_t animation;            // +6: -1: no animation
};
static_assert(offsetof(AnimationChannel,animation)==6);
// The object flags word (Device::object_flags).
namespace object_flags {
inline constexpr std::uint32_t powered_floor=0x0400u;   // the cluster's power is at least 1
inline constexpr std::uint32_t cluster_power=0x1000u;   // the machine drives its cluster's power (1DA750)
}
// A device's flags (Device::device_flags).
namespace device_flags {
inline constexpr std::uint32_t moved=0x004u;             // set when a channel moved (10C5D0); the machine
                                                          // update notifies and clears it
inline constexpr std::uint32_t scripted_position=0x008u; // the position follows a script (10CD70)
inline constexpr std::uint32_t scripted_power=0x010u;    // the power follows a script
inline constexpr std::uint32_t position_script_done=0x020u;
inline constexpr std::uint32_t power_script_done=0x040u;
inline constexpr std::uint32_t initializing=0x080u;      // the first update: the channels report a change
inline constexpr std::uint32_t moving=0x100u;            // the position can move (acceleration and speed)
}
// A machine's flags (Device::machine_flags).
namespace machine_flags {
inline constexpr std::uint32_t door_locked=0x01u;        // the door does not query units
inline constexpr std::uint32_t door_stays_open=0x02u;    // the close timer does not run
inline constexpr std::uint32_t one_sided=0x04u;          // a closed door opens only from its front
inline constexpr std::uint32_t one_sided_for_players=0x08u;
}
// The device definition: the device part of the mach, ctrl and lifi tags,
// then the machine's.
struct DeviceDefinition {
    std::uint8_t unused_here_00[0xbc];
    std::uint32_t flags;               // +0xBC: bit 0, the position loops
    std::uint32_t power_rate;          // +0xC0: floats
    std::uint32_t power_acceleration;  // +0xC4
    std::uint32_t position_rate;       // +0xC8
    std::uint32_t position_acceleration; // +0xCC
    std::uint32_t unpowered_rate;      // +0xD0
    std::uint32_t unpowered_acceleration; // +0xD4
    std::uint8_t unused_here_d8[8];
    std::uint32_t opening_effect;      // +0xE0: the effects' tag indices (8-byte references)
    std::uint32_t unused_here_e4;
    std::uint32_t closing_effect;      // +0xE8
    std::uint32_t unused_here_ec;
    std::uint32_t opened_effect;       // +0xF0
    std::uint32_t unused_here_f4;
    std::uint32_t closed_effect;       // +0xF8
    std::uint8_t unused_here_fc[0x10];
    std::uint32_t delay;               // +0x10C: float, seconds before the position moves
    std::uint32_t unused_here_110;
    std::uint32_t delay_effect;        // +0x114
    // A machine's part.
    std::uint32_t activation_radius;   // +0x118: float, a door's unit query radius
    std::uint16_t machine_type;        // +0x11C: machine_types
    std::uint16_t unused_here_11e;
    std::uint32_t door_open_time;      // +0x120: float, seconds a door stays open
    std::uint32_t closed_openness;     // +0x124: float, a portal's openness when the door is closed
    std::uint32_t open_openness;       // +0x128: float, ... when it is open
};
static_assert(offsetof(DeviceDefinition,flags)==0xbc && offsetof(DeviceDefinition,power_rate)==0xc0 &&
              offsetof(DeviceDefinition,power_acceleration)==0xc4 && offsetof(DeviceDefinition,position_rate)==0xc8 &&
              offsetof(DeviceDefinition,position_acceleration)==0xcc && offsetof(DeviceDefinition,unpowered_rate)==0xd0 &&
              offsetof(DeviceDefinition,unpowered_acceleration)==0xd4 && offsetof(DeviceDefinition,opening_effect)==0xe0 &&
              offsetof(DeviceDefinition,closing_effect)==0xe8 && offsetof(DeviceDefinition,opened_effect)==0xf0 &&
              offsetof(DeviceDefinition,closed_effect)==0xf8 && offsetof(DeviceDefinition,delay)==0x10c &&
              offsetof(DeviceDefinition,delay_effect)==0x114 && offsetof(DeviceDefinition,activation_radius)==0x118 &&
              offsetof(DeviceDefinition,machine_type)==0x11c && offsetof(DeviceDefinition,door_open_time)==0x120 &&
              offsetof(DeviceDefinition,closed_openness)==0x124 && offsetof(DeviceDefinition,open_openness)==0x128);
namespace machine_types {
inline constexpr std::uint32_t door=0u;
inline constexpr std::uint32_t gear=2u;
}
// The unit fields the door query reads.
struct Unit {
    std::uint8_t unused_here_00[0x30];
    std::uint32_t center[3];           // +0x30: float
    std::uint8_t unused_here_3c[0xce];
    std::uint8_t flags;                // +0x10A: bit 2, the door ignores the unit
    std::uint8_t unused_here_10b[0x31];
    std::uint32_t player;              // +0x13C: the controlling player, or NONE
};
static_assert(offsetof(Unit,center)==0x30 && offsetof(Unit,flags)==0x10a && offsetof(Unit,player)==0x13c);
struct UnitDefinition {
    std::uint8_t unused_here_00[0xbc];
    std::uint32_t flags;               // +0xBC: bit 14, the unit does not open doors
    std::uint8_t unused_here_c0[0x12c];
    std::uint32_t vehicle_flags;       // +0x1EC: bit 20, a vehicle opens doors
};
static_assert(offsetof(UnitDefinition,flags)==0xbc && offsetof(UnitDefinition,vehicle_flags)==0x1ec);

// The original tests 1 << type (the type's low five bits): bit 1 is a
// vehicle, bit 7 a machine.
constexpr bool is_vehicle(std::uint32_t type){return (type&31u)==1u;}
constexpr bool is_machine(std::uint32_t type){return (type&31u)==7u;}
// The definition data of a tag index (the [4F0414] row's +8).
constexpr std::uint32_t tag_row(std::uint32_t instances,std::uint32_t tag) {
    return at(element(instances,tag&0xffffu,sizeof(TagInstance)),offsetof(TagInstance,definition));
}
// A device group's value (the group's datum through the [4ECBF8] array).
constexpr std::uint32_t group_value(std::uint32_t groups,std::uint32_t group) {
    return at(element(groups,handle_index(group),sizeof(DeviceGroup)),offsetof(DeviceGroup,value));
}

// ---------------------------------------------------------- machine update
// 1179C0(handle). Returns whether the machine changed (AL).
//
//   1. A machine that drives its physics cluster's power passes the power
//      (at least 1 with `powered_floor`) to the cluster (1DA750), unless the
//      power is within epsilon of 0 and the floor is off.
//   2. With an animation block and a position animation, the animation
//      follows the position (1CE8D0 sets its frame, 1CEBD0 advances it).
//   3. A gear turns: its rate is the unpowered rate weighted by 1 - power
//      plus the powered rate weighted by the power (extended precision); its
//      position advances by rate * tick length and wraps at 1. A turning gear
//      marks its network entity (B7E50), reports its moved position (10C5D0)
//      and publishes the position to its position group.
//   4. An unlocked door queries the units around it on every fourth tick
//      outside game mode 4 (BD080). A unit that may open it opens it (10C290
//      sets the position group to 1, and the open ticks restart at -3),
//      unless the door is closed and one-sided and the unit is behind it (the
//      marker 0x7000686, else the object's center and forward vector, BBFF0).
//   5. A door's portals follow whether it is open: each machine portal
//      entry's open bit, and its openness word from the position.
//   6. A fully open door counts its ticks and asks its group to close (10C290
//      to 0) after its open time.
//   7. A moved machine (its own `moved` flag, or a moved parent) notifies
//      (BF050, 1181F0, BDA50) and returns 1; otherwise 118350 runs and the
//      changed byte is returned.
//
// Frame:
//   handle(), phase_handle(), handle_again()   the argument, read from its slot
//   begin(m)                       keeps the definition; clears changed and animated
//   mark_changed(), changed(), mark_animated(), animated(), clear_open(), mark_open(), open()
//   definition()                   the definition, read again from its slot
//   power_cluster(m, record, power)                 1DA750
//   follow_position(m, position), reset_channel(m)  1CE8D0
//   advance_channel(m, value)                       1CEBD0
//   keep_rate(rate), rate()        the turning rate, spilled as a single (FST)
//   mark_entity(m, entity), report_position(m), set_group(m, group, value)
//   query_units(m, radius) -> count, begin_units(count), unit(index),
//   unit_count(), next_unit(index), unit_index(), unit_object()
//   find_marker(m) -> found, query_forward(m, x, y, z)      BAD60, BBFF0
//   marker_position(axis), marker_forward(axis)
//   keep_portal_cursor(c), portal_cursor(), round_openness(value), openness()
//   notify_moved(m), update_attachments(m), moved(m), unmoved(m)
namespace machine_update {
enum class Access {
    table,headers,tags,object,definition_index,definition,object_flags,
    power_magnitude,power_epsilon,cluster,cluster_table,cluster_one,cluster_data,cluster_power,
    table_again,headers_again,object_again,animation_offset,channel_graph,channel_animation,animation_offset_again,
    machine_type,follow_position,rate_one,rate_power,velocity_clear,unpowered_rate,powered_rate,rate_power_again,
    rate_epsilon,time,tick_length,turn_position,turn_one,turn_store,wrap_store,wrap_one,
    entity_table,entity_headers,entity_object,network_entity,channel_position,turned_one,
    group_position,group_table,group_data,published_position,group_store,door_one,
    door_flags,door_type,door_time,door_tick,game,mode,query_epsilon,activation_radius,object_radius,
    activation_radius_again,
    unit_table,unit_headers,unit_object,unit_tags,unit_definition_index,unit_definition,unit_type,vehicle_flags,
    unit_state,unit_flags,one_sided_flags,unit_player,door_position,door_zero,center_x,center_y,center_z,
    unit_x,unit_z,unit_y,side_zero,open_group,open_ticks_open,queried_one,
    occlusion_type,occlusion_group,occlusion_flags,occlusion_group_table,occlusion_group_data,occlusion_group_value,
    occlusion_position,first_portal,portal_count,openness_table,open_bits,openness_scale,
    structure,portal_list,portal_indices,closed_openness,open_openness,portal_index,openness_position,open_word,
    open_word_store,openness_store,first_portal_again,portal_count_again,
    timer_position,timer_flags,timer_game,timer_mode,open_ticks,open_ticks_store,timer_time,timer_tick_length,
    door_open_time,close_group,open_ticks_clear,
    finish_flags,moved_clear,parent,parent_table,parent_headers,parent_flags,parent_object,parent_parent
};
// Kept across the child calls: the machine's header row (index * 12), its
// data, its definition, its animation block (0 for none), the door query's
// unit and position, the handle at the end, whether it moved, and the
// parent being walked.
struct Machine {
    std::uint32_t row;
    std::uint32_t object;
    std::uint32_t definition;
    std::uint32_t animation;
    std::uint32_t unit;
    std::uint32_t index;
    std::uint32_t handle;
    std::uint8_t moved;
    std::uint32_t parent;
};
enum class Exit {moved,unmoved};
struct Result {Exit exit;std::uint8_t value;};

// The door's unit query (step 4).
template<class Memory,class Frame> void query_units(Memory &memory,Frame &frame,Machine &m) {
    using enum Access;
    const auto epsilon=memory.load32(query_epsilon,device_globals::epsilon);
    const bool no_radius=sse::above(epsilon,memory.load32(activation_radius,at(m.definition,offsetof(DeviceDefinition,activation_radius))));
    frame.clear_open();
    // A door without its own radius queries with its bounding radius.
    const auto radius=no_radius?memory.load32(object_radius,at(m.object,offsetof(Device,radius))):
        memory.load32(activation_radius_again,at(m.definition,offsetof(DeviceDefinition,activation_radius)));
    const auto count=widen16(frame.query_units(m,radius));
    frame.begin_units(count);
    if(signed_value(count)<=0)return;
    m.index=0;
    for(;;) {
        const auto handle=frame.unit(m.index);
        const auto array=memory.load32(unit_table,globals::object_headers);
        const auto rows=memory.load32(unit_headers,at(array,offsetof(DataArray,data)));
        const auto header=element(rows,handle_index(handle),sizeof(ObjectHeader));
        m.unit=memory.load32(unit_object,at(header,offsetof(ObjectHeader,object)));
        const auto instances=memory.load32(unit_tags,device_globals::tag_instances);
        const auto tag=memory.load32(unit_definition_index,at(m.unit,offsetof(ObjectData,definition)));
        const auto definition=memory.load32(unit_definition,tag_row(instances,tag));
        const auto type=memory.load8(unit_type,at(header,offsetof(ObjectHeader,type)));
        // A unit opens the door unless it is a vehicle that does not open
        // doors, or its own flag or its definition says it does not.
        bool opens=true,ignored=false;
        if(is_vehicle(type) && !((memory.load32(vehicle_flags,at(definition,offsetof(UnitDefinition,vehicle_flags)))>>20)&1u))
            opens=false;
        else ignored=(memory.load8(unit_state,at(m.unit,offsetof(Unit,flags)))>>2)&1u;
        if(!ignored && !((memory.load32(unit_flags,at(definition,offsetof(UnitDefinition,flags)))>>14)&1u) && opens) {
            // A closed one-sided door (for every unit, or for players) opens
            // only for a unit that is not in front of its marker.
            const auto flags=memory.load32(one_sided_flags,at(m.object,offsetof(Device,machine_flags)));
            bool one_sided=flags&machine_flags::one_sided;
            if(!one_sided && (flags&machine_flags::one_sided_for_players))
                one_sided=memory.load32(unit_player,at(m.unit,offsetof(Unit,player)))!=none;
            bool behind=true;
            if(one_sided) {
                const auto position=memory.load32(door_position,at(m.object,offsetof(Device,position)));
                if(sse::equal(position,memory.load32(door_zero,device_globals::zero))) {
                    // The door's marker, else its center and forward vector.
                    if(signed_value(widen16(frame.find_marker(m)))<=0) {
                        const auto x=memory.load32(center_x,at(m.object,offsetof(Device,center)));
                        const auto y=memory.load32(center_y,at(m.object,offsetof(Device,center)+4));
                        const auto z=memory.load32(center_z,at(m.object,offsetof(Device,center)+8));
                        frame.query_forward(m,x,y,z);
                        m.index=frame.unit_index();
                        m.unit=frame.unit_object();
                    }
                    // forward . (unit center - marker): x, then z, then y.
                    const auto unit_x_value=memory.load32(unit_x,at(m.unit,offsetof(Unit,center)));
                    const auto dx=sse::subtract(unit_x_value,frame.marker_position(0));
                    const auto forward_x=frame.marker_forward(0);
                    const auto unit_z_value=memory.load32(unit_z,at(m.unit,offsetof(Unit,center)+8));
                    const auto dz=sse::subtract(unit_z_value,frame.marker_position(2));
                    const auto unit_y_value=memory.load32(unit_y,at(m.unit,offsetof(Unit,center)+4));
                    const auto dy=sse::subtract(unit_y_value,frame.marker_position(1));
                    auto side=sse::multiply(forward_x,dx);
                    side=sse::add(side,sse::multiply(frame.marker_forward(2),dz));
                    side=sse::add(side,sse::multiply(frame.marker_forward(1),dy));
                    behind=!sse::above(side,memory.load32(side_zero,device_globals::zero));
                }
            }
            if(behind)frame.mark_open();
        }
        const auto units=frame.unit_count();
        frame.next_unit(++m.index);
        if(signed_value(m.index)>=signed_value(units))break;
    }
    if(!frame.open())return;
    const auto group=memory.load32(open_group,at(m.object,offsetof(Device,position_group)));
    if(group!=none)frame.set_group(m,group,0x3f800000u);   // 1.0
    memory.store32(open_ticks_open,at(m.object,offsetof(Device,open_ticks)),0xfffffffdu);
}

// The door's portals (step 5): each machine portal entry's open bit, and its
// openness word: the position between the closed and open openness, clamped
// to [0, 1] (a NaN stays) and scaled to 65535.
template<class Memory,class Frame> void occlude_portals(Memory &memory,Frame &frame,Machine &m,bool open,std::uint32_t one) {
    using enum Access;
    auto index=widen16(memory.load16(first_portal,at(m.object,offsetof(Device,first_portal))));
    auto end=index+widen16(memory.load16(portal_count,at(m.object,offsetof(Device,portal_count))));
    if(signed_value(index)>=signed_value(end))return;
    const auto openness=memory.load32(openness_table,device_globals::portal_openness);
    const auto bits=memory.load32(open_bits,device_globals::portal_open_bits);
    const auto scale=memory.load32(openness_scale,device_globals::openness_scale);
    frame.keep_portal_cursor(at(element(openness,index,2),offsetof(PortalOpenness,openness)));
    do {
        const auto bsp=memory.load32(structure,device_globals::structure_bsp);
        const auto portals=memory.load32(portal_list,at(bsp,offsetof(StructureBsp,machine_portals)));
        const auto indices=memory.load32(portal_indices,at(portals,offsetof(MachinePortals,portal_indices)));
        const auto closed=memory.load32(closed_openness,at(m.definition,offsetof(DeviceDefinition,closed_openness)));
        const auto opened=memory.load32(open_openness,at(m.definition,offsetof(DeviceDefinition,open_openness)));
        const auto portal=widen16(memory.load16(portal_index,element(indices,index&0xffffu,2)));
        const auto span=sse::subtract(opened,closed);
        const auto weighted=sse::multiply(span,memory.load32(openness_position,at(m.object,offsetof(Device,position))));
        // The word of a portal's bit is the index shifted arithmetically.
        const auto word=element(bits,std::uint32_t(signed_value(portal)>>5),4);
        const auto bit=1u<<(portal&31u);
        const auto current=memory.load32(open_word,word);
        auto value=sse::add(weighted,closed);
        memory.store32(open_word_store,word,open?current|bit:current&~bit);
        if(sse::above(0u,value))value=0u;
        else if(sse::above(value,one))value=one;
        value=sse::multiply(value,scale);
        // The x87 rounds the scaled openness to an integer (FLD; FISTP
        // through two frame slots, in the live rounding mode).
        frame.round_openness(value);
        const auto cursor=frame.portal_cursor();
        const auto rounded=frame.openness();
        m.definition=frame.definition();
        memory.store16(openness_store,cursor,rounded);
        const auto first=widen16(memory.load16(first_portal_again,at(m.object,offsetof(Device,first_portal))));
        frame.keep_portal_cursor(cursor+2u);
        end=widen16(memory.load16(portal_count_again,at(m.object,offsetof(Device,portal_count))))+first;
        ++index;
    } while(signed_value(index)<signed_value(end));
}

// The notifications of a moved machine (step 7).
template<class Memory,class Frame> Result finish(Memory &memory,Frame &frame,Machine &m) {
    using enum Access;
    const auto flags=memory.load32(finish_flags,at(m.object,offsetof(Device,device_flags)));
    m.handle=frame.handle_again();
    m.moved=(flags>>2)&1u;
    if(m.moved) {
        frame.notify_moved(m);
        const auto value=memory.load32(moved_clear,at(m.object,offsetof(Device,device_flags)));
        memory.store32(moved_clear,at(m.object,offsetof(Device,device_flags)),value&~device_flags::moved);
    }
    // A machine also moved when a parent moved.
    m.parent=memory.load32(parent,at(m.object,offsetof(ObjectData,parent)));
    if(!m.moved) {
        while(m.parent!=none) {
            const auto array=memory.load32(parent_table,globals::object_headers);
            const auto rows=memory.load32(parent_headers,at(array,offsetof(DataArray,data)));
            const auto header=element(rows,handle_index(m.parent),sizeof(ObjectHeader));
            const auto header_flags=memory.load8(parent_flags,at(header,offsetof(ObjectHeader,flags)));
            const auto data=memory.load32(parent_object,at(header,offsetof(ObjectHeader,object)));
            if(header_flags&native_object_update::header_flags::moved)m.moved=1;
            m.parent=memory.load32(parent_parent,at(data,offsetof(ObjectData,parent)));
            if(m.moved)break;
        }
    }
    if(!m.moved) {
        frame.unmoved(m);
        return {Exit::unmoved,frame.changed()};
    }
    frame.update_attachments(m);
    m.moved=1;
    frame.moved(m);
    return {Exit::moved,m.moved};
}

template<class Memory,class Frame> Result run(Memory &memory,Frame &frame) {
    using enum Access;
    Machine m{};
    m.row=handle_index(frame.handle())*std::uint32_t(sizeof(ObjectHeader));
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    const auto instances=memory.load32(tags,device_globals::tag_instances);
    m.object=memory.load32(object,at(rows+m.row,offsetof(ObjectHeader,object)));
    const auto tag=memory.load32(definition_index,at(m.object,offsetof(ObjectData,definition)));
    m.definition=memory.load32(definition,tag_row(instances,tag));
    const auto flags=memory.load16(object_flags,at(m.object,offsetof(Device,object_flags)));
    frame.begin(m);

    // 1. The cluster's power.
    if(flags&object_flags::cluster_power) {
        const auto power=x87::magnitude(x87::load(memory.load32(power_magnitude,at(m.object,offsetof(Device,power)))));
        const auto epsilon=x87::load(memory.load32(power_epsilon,device_globals::epsilon));
        if(!x87::above(epsilon,power) || (flags&object_flags::powered_floor)) {
            const auto cluster_handle=memory.load32(cluster,at(m.object,offsetof(ObjectData,cluster)));
            if(cluster_handle!=none) {
                const auto clusters=memory.load32(cluster_table,globals::clusters);
                const auto one=memory.load32(cluster_one,device_globals::one);
                const auto offset=handle_index(cluster_handle)*std::uint32_t(sizeof(ClusterRecord));
                const auto record=offset+memory.load32(cluster_data,at(clusters,offsetof(DataArray,data)));
                const auto floor=flags&object_flags::powered_floor?one:0u;
                const auto value=memory.load32(cluster_power,at(m.object,offsetof(Device,power)));
                frame.power_cluster(m,record,sse::above(value,floor)?value:floor);
                frame.mark_changed();
            }
        }
    }

    // 2. The animation follows the position when the machine has an
    // animation block and a position animation.
    {
        const auto array_again=memory.load32(table_again,globals::object_headers);
        const auto rows_again=memory.load32(headers_again,at(array_again,offsetof(DataArray,data)));
        const auto data=memory.load32(object_again,at(rows_again+m.row,offsetof(ObjectHeader,object)));
        if(memory.load16(animation_offset,at(data,offsetof(Device,animation_offset)))!=0xffffu &&
           memory.load32(channel_graph,at(m.object,offsetof(Device,position_channel)))!=none &&
           memory.load16(channel_animation,at(m.object,offsetof(Device,position_channel)+offsetof(AnimationChannel,animation)))!=0xffffu) {
            m.animation=widen16(memory.load16(animation_offset_again,at(m.object,offsetof(Device,animation_offset))))+m.object;
            frame.mark_animated();
        }
    }

    // 3. A gear turns. `one` is the 1.0 the door steps compare with; the
    // original reloads it after each child call.
    std::uint32_t one;
    if(memory.load16(machine_type,at(m.definition,offsetof(DeviceDefinition,machine_type)))==machine_types::gear) {
        if(frame.animated()) {
            frame.follow_position(m,memory.load32(follow_position,at(m.object,offsetof(Device,position))));
            m.definition=frame.definition();
        }
        // The rate, at extended precision: (1 - power) * unpowered rate +
        // powered rate * power. The velocity is cleared in between.
        const auto rate_one_value=x87::load(memory.load32(rate_one,device_globals::one));
        const auto inverse=x87::difference(rate_one_value,x87::load(memory.load32(rate_power,at(m.object,offsetof(Device,power)))));
        memory.store32(velocity_clear,at(m.object,offsetof(Device,position_velocity)),0);
        const auto unpowered=x87::product(inverse,
            x87::load(memory.load32(unpowered_rate,at(m.definition,offsetof(DeviceDefinition,unpowered_rate)))));
        const auto powered_value=x87::load(memory.load32(powered_rate,at(m.definition,offsetof(DeviceDefinition,position_rate))));
        const auto powered=x87::product(powered_value,x87::load(memory.load32(rate_power_again,at(m.object,offsetof(Device,power)))));
        const auto rate=x87::sum(unpowered,powered);
        frame.keep_rate(rate);
        const auto epsilon=x87::load(memory.load32(rate_epsilon,device_globals::epsilon));
        if(x87::above(x87::magnitude(rate),epsilon)) {
            const auto clock=memory.load32(time,device_globals::game_time);
            const auto tick=memory.load32(tick_length,at(clock,offsetof(GameTime,tick_length)));
            const auto step=sse::multiply(tick,frame.rate());
            auto position=sse::add(step,memory.load32(turn_position,at(m.object,offsetof(Device,position))));
            one=memory.load32(turn_one,device_globals::one);
            // A full turn wraps (a NaN position does not).
            const bool wraps=!sse::below(position,one);
            memory.store32(turn_store,at(m.object,offsetof(Device,position)),position);
            if(wraps) {
                const bool animated=frame.animated();
                position=sse::subtract(position,one);
                memory.store32(wrap_store,at(m.object,offsetof(Device,position)),position);
                if(animated) {
                    frame.advance_channel(m,0x3f800000u);   // to 1.0, then from 0
                    frame.reset_channel(m);
                    one=memory.load32(wrap_one,device_globals::one);
                    m.definition=frame.definition();
                }
            }
            // The machine's network entity is marked for its position update.
            const auto entity_array=memory.load32(entity_table,globals::object_headers);
            const auto entity_rows=memory.load32(entity_headers,at(entity_array,offsetof(DataArray,data)));
            const auto data=memory.load32(entity_object,at(entity_rows+m.row,offsetof(ObjectHeader,object)));
            const auto entity=memory.load32(network_entity,at(data,offsetof(Device,network_entity)));
            if(entity!=none)frame.mark_entity(m,entity);
            frame.report_position(m);
            if(frame.animated()) {
                frame.advance_channel(m,memory.load32(channel_position,at(m.object,offsetof(Device,position))));
                one=memory.load32(turned_one,device_globals::one);
                m.definition=frame.definition();
            }
            const auto group=memory.load32(group_position,at(m.object,offsetof(Device,position_group)));
            if(group!=none) {
                const auto groups=memory.load32(group_table,device_globals::device_groups);
                const auto values=memory.load32(group_data,at(groups,offsetof(DataArray,data)));
                const auto position_value=memory.load32(published_position,at(m.object,offsetof(Device,position)));
                memory.store32(group_store,group_value(values,group),position_value);
            }
            frame.mark_changed();
        } else one=memory.load32(door_one,device_globals::one);
    } else one=memory.load32(door_one,device_globals::one);

    // 4. An unlocked door's unit query, on every fourth tick outside game
    // mode 4.
    if(!(memory.load8(door_flags,at(m.object,offsetof(Device,machine_flags)))&machine_flags::door_locked)) {
        if(memory.load16(door_type,at(m.definition,offsetof(DeviceDefinition,machine_type)))!=machine_types::door)
            return finish(memory,frame,m);
        const auto clock=memory.load32(door_time,device_globals::game_time);
        const auto tick=memory.load32(door_tick,at(clock,offsetof(GameTime,tick)));
        const auto phase=tick+frame.phase_handle();
        frame.mark_changed();
        if(!(phase&3u)) {
            const auto game_globals=memory.load32(game,globals::game_globals);
            if(memory.load8(mode,at(game_globals,offsetof(GameGlobals,mode)))!=4u) {
                query_units(memory,frame,m);
                one=memory.load32(queried_one,device_globals::one);
                m.definition=frame.definition();
            }
        }
    }

    // 5. A door's portals follow whether it is open: its group's value
    // while it moves, or its position.
    if(memory.load16(occlusion_type,at(m.definition,offsetof(DeviceDefinition,machine_type)))!=machine_types::door)
        return finish(memory,frame,m);
    const auto group=memory.load32(occlusion_group,at(m.object,offsetof(Device,position_group)));
    bool open=false;
    if(group!=none) {
        if(memory.load32(occlusion_flags,at(m.object,offsetof(Device,device_flags)))&device_flags::moving) {
            const auto groups=memory.load32(occlusion_group_table,device_globals::device_groups);
            const auto values=memory.load32(occlusion_group_data,at(groups,offsetof(DataArray,data)));
            open=!sse::equal(memory.load32(occlusion_group_value,group_value(values,group)),0u);
        }
        if(!open)open=!sse::equal(memory.load32(occlusion_position,at(m.object,offsetof(Device,position))),0u);
    }
    occlude_portals(memory,frame,m,open,one);

    // 6. A fully open door counts its ticks and asks its group to close after
    // its open time (outside game mode 4, unless it stays open).
    if(sse::equal(memory.load32(timer_position,at(m.object,offsetof(Device,position))),one) &&
       !(memory.load8(timer_flags,at(m.object,offsetof(Device,machine_flags)))&machine_flags::door_stays_open)) {
        const auto game_globals=memory.load32(timer_game,globals::game_globals);
        if(memory.load8(timer_mode,at(game_globals,offsetof(GameGlobals,mode)))!=4u) {
            const auto ticks=memory.load32(open_ticks,at(m.object,offsetof(Device,open_ticks)))+1u;
            memory.store32(open_ticks_store,at(m.object,offsetof(Device,open_ticks)),ticks);
            const auto ticks_value=sse::from_integer(ticks);
            const auto clock=memory.load32(timer_time,device_globals::game_time);
            const auto elapsed=sse::multiply(ticks_value,memory.load32(timer_tick_length,at(clock,offsetof(GameTime,tick_length))));
            if(sse::above(elapsed,memory.load32(door_open_time,at(m.definition,offsetof(DeviceDefinition,door_open_time))))) {
                const auto close=memory.load32(close_group,at(m.object,offsetof(Device,position_group)));
                if(close!=none)frame.set_group(m,close,0);
            }
            frame.mark_changed();
            return finish(memory,frame,m);
        }
    }
    memory.store32(open_ticks_clear,at(m.object,offsetof(Device,open_ticks)),0);
    return finish(memory,frame,m);
}
} // namespace machine_update

// ----------------------------------------------------------- device update
// 10B840(handle). Returns whether the device changed (AL).
//
//   1. The `moving` flag is cleared; this update sets it again.
//   2. The position. A scripted position follows its script (10CD70) until
//      the script is done; its velocity is the change over the tick length.
//      Otherwise a position with a group approaches the group's value when
//      the value, the velocity or the first update asks for it: the
//      animation follows (1CE8D0); the acceleration and speed are the
//      powered ones weighted by the power plus the unpowered ones weighted by
//      1 - power, and `moving` says both are positive. A closed device waits
//      its delay first (counting ticks; the delay effect plays on the first).
//      The velocity is clamped to the speed (keeping its direction) and the
//      channel steps (121890). The opened or closed effect plays when it
//      arrives, the opening or closing effect when its velocity turns (10CA30).
//      A moved position (or the first update) is reported (10C5D0) and the
//      animation advanced (1CEBD0).
//   3. The power: a scripted power follows its script; otherwise, unless the
//      position is scripted, a power with a group approaches the group's value
//      the same way (no delay, no effects, the animation checked by B8760).
//   4. The first update of a device with a position: a machine notifies
//      (BF050, 1181F0); the flag clears unless the object still counts its
//      awake ticks. Returns 1. Otherwise the changed byte.
//
// Frame:
//   handle(), handle_argument()        the argument, read from its slot
//   begin(d, definition)               keeps the row and definition; changed 0, no block
//   keep_block(block), block(), definition(), group(), row()
//   mark_changed(), changed()
//   script_position(d, before) -> done, scripted_position(), script_start()   10CD70
//   script_power(d) -> done
//   keep_group(group), follow_position(d, value), follow_power(d, value)   1CE8D0
//   channel_valid(d) -> valid                                              B8760
//   keep_motion(acceleration, speed), keep_direction(up), direction(), acceleration()
//   keep_old_velocity(v), old_velocity(), keep_old_position(p), old_position(), speed()
//   step_position(d, target, tick, acceleration, loops) -> arrived,
//   step_power(d, target, tick, acceleration, before, speed), old_power()   121890
//   play_effect(d, effect)                                                  10CA30
//   report_script_position(d), report_position(d), report_script_power(d), report_power(d)   10C5D0
//   advance_position(d, value, block), advance_power(d, value, block)      1CEBD0
//   notify_machine(d)                                                       BF050, 1181F0
namespace device_update {
enum class Access {
    table,headers,tags,object,definition_index,animation_offset,definition,animation_offset_again,flags_load,flags_store,
    // the scripted position
    script_old_position,clock,script_store,tick_length,script_flags,script_velocity,script_flags_store,
    // the position
    position_group,group_table,group_data,group_value,position_compare,velocity_compare,velocity_zero,delay_clear,
    channel_graph,channel_animation,follow_position,
    one,inverse_power,position_acceleration,unpowered_acceleration,acceleration_power,position_rate,speed_power,
    unpowered_rate,direction_velocity,moving_set,moving_clear,delay,moving_store,clock_again,delay_tick_length,delay_time,
    closed_position,closed_target,delay_store,delay_effect,
    old_velocity,old_position,clamp_store,step_tick_length,loop_flags,step_target,
    opened_effect,closed_effect,new_velocity,new_velocity_again,opening_effect,closing_effect,new_position,
    initializing_flags,moved_graph,moved_animation,moved_position,
    // the power
    power_flags,power_script_flags,power_script_store,power_group,power_group_table,power_group_data,power_group_value,
    power_compare,power_velocity_compare,power_velocity_zero,follow_power,power_acceleration,power_clock,power_tick_length,
    old_power,power_rate,power_target,new_power,moved_power,
    // the end
    final_flags,final_group,type_table,type_headers,object_type,countdown_table,countdown_headers,countdown_object,
    countdown,initializing_clear
};
// Kept across the child calls: the device's header row, its data, the
// handle, the position group's datum, the address of the channel value (the
// position or the power), the address of the position velocity, and the
// power group's datum.
struct Device {
    std::uint32_t row;
    std::uint32_t object;
    std::uint32_t handle;
    std::uint32_t group;
    std::uint32_t value;
    std::uint32_t velocity;
    std::uint32_t power_group;
};
enum class Exit {initialized,changed};
struct Result {Exit exit;std::uint8_t value;};
// How a channel's approach ended.
enum class Approach {
    none,      // no group (or, for the position, at rest: the delay is cleared)
    idle,      // the power is at its group's value and at rest
    updated    // the channel waited or stepped
};

// The position approaching its group's value (step 2, unscripted).
template<class Memory,class Frame> Approach approach_position(Memory &memory,Frame &frame,Device &d,std::uint32_t flags,
                                                              std::uint32_t device_definition) {
    using enum Access;
    using Layout=native_device_update::Device;
    const auto group=memory.load32(position_group,at(d.object,offsetof(Layout,position_group)));
    if(group==none)return Approach::none;
    const auto groups=memory.load32(group_table,device_globals::device_groups);
    const auto values=memory.load32(group_data,at(groups,offsetof(DataArray,data)));
    d.group=element(values,handle_index(group),sizeof(DeviceGroup));
    const auto target=memory.load32(group_value,at(d.group,offsetof(DeviceGroup,value)));
    d.value=at(d.object,offsetof(Layout,position));
    const bool at_target=sse::equal(target,memory.load32(position_compare,d.value));
    frame.keep_group(d.group);
    if(at_target) {
        const auto velocity=memory.load32(velocity_compare,at(d.object,offsetof(Layout,position_velocity)));
        if(sse::equal(velocity,memory.load32(velocity_zero,device_globals::zero)) && !(flags&device_flags::initializing)) {
            memory.store16(delay_clear,at(d.object,offsetof(Layout,delay_ticks)),0);
            return Approach::none;
        }
    }
    if(frame.block()) {
        const auto channel=at(d.object,offsetof(Layout,position_channel));
        if(memory.load32(channel_graph,channel)!=none &&
           memory.load16(channel_animation,at(channel,offsetof(AnimationChannel,animation)))!=0xffffu) {
            frame.follow_position(d,memory.load32(follow_position,d.value));
            d.group=frame.group();
            device_definition=frame.definition();
        }
    }
    // The acceleration and speed between the unpowered and powered ones.
    const auto one_value=memory.load32(one,device_globals::one);
    const auto inverse=sse::subtract(one_value,memory.load32(inverse_power,at(d.object,offsetof(Layout,power))));
    auto acceleration=memory.load32(position_acceleration,at(device_definition,offsetof(DeviceDefinition,position_acceleration)));
    const auto unpowered=memory.load32(unpowered_acceleration,at(device_definition,offsetof(DeviceDefinition,unpowered_acceleration)));
    acceleration=sse::multiply(acceleration,memory.load32(acceleration_power,at(d.object,offsetof(Layout,power))));
    auto speed=memory.load32(position_rate,at(device_definition,offsetof(DeviceDefinition,position_rate)));
    speed=sse::multiply(speed,memory.load32(speed_power,at(d.object,offsetof(Layout,power))));
    acceleration=sse::add(acceleration,sse::multiply(unpowered,inverse));
    speed=sse::add(speed,sse::multiply(memory.load32(unpowered_rate,at(device_definition,offsetof(DeviceDefinition,unpowered_rate))),inverse));
    d.velocity=at(d.object,offsetof(Layout,position_velocity));
    const bool up=sse::above(memory.load32(direction_velocity,d.velocity),0u);
    frame.keep_motion(acceleration,speed);
    frame.keep_direction(up);
    const bool moving=sse::above(acceleration,0u) && sse::above(speed,0u);
    const auto device_flags_value=memory.load32(moving?moving_set:moving_clear,at(d.object,offsetof(Layout,device_flags)));
    const auto delay_ticks=memory.load16(delay,at(d.object,offsetof(Layout,delay_ticks)));
    memory.store32(moving_store,at(d.object,offsetof(Layout,device_flags)),
        moving?device_flags_value|device_flags::moving:device_flags_value&~device_flags::moving);
    device_definition=frame.definition();
    // A closed device waits its delay before it opens.
    const auto waited_ticks=sse::from_integer(widen16(delay_ticks));
    const auto game_clock=memory.load32(clock_again,device_globals::game_time);
    const auto waited=sse::multiply(waited_ticks,memory.load32(delay_tick_length,at(game_clock,offsetof(GameTime,tick_length))));
    if(sse::below(waited,memory.load32(delay_time,at(device_definition,offsetof(DeviceDefinition,delay))))) {
        const auto position=memory.load32(closed_position,d.value);
        if(sse::equal(position,0u) && !sse::above(position,memory.load32(closed_target,at(d.group,offsetof(DeviceGroup,value))))) {
            const auto ticks=(delay_ticks+1u)&0xffffu;
            memory.store16(delay_store,at(d.object,offsetof(Layout,delay_ticks)),ticks);
            if(ticks==1u) {
                device_definition=frame.definition();
                frame.play_effect(d,memory.load32(delay_effect,at(device_definition,offsetof(DeviceDefinition,delay_effect))));
            }
            return Approach::updated;
        }
    }
    // The velocity is clamped to the speed (extended precision), keeping its
    // direction, and the channel steps.
    const auto velocity=memory.load32(old_velocity,d.velocity);
    const auto position=memory.load32(old_position,d.value);
    frame.keep_old_velocity(velocity);
    const auto magnitude=x87::magnitude(x87::load(frame.old_velocity()));
    frame.keep_old_position(position);
    if(x87::above(magnitude,x87::load(frame.speed()))) {
        const auto clamped=frame.direction()?speed:sse::subtract(0u,speed);
        memory.store32(clamp_store,d.velocity,clamped);
    }
    const auto tick=memory.load32(step_tick_length,at(game_clock,offsetof(GameTime,tick_length)));
    const auto step_acceleration=frame.acceleration();
    device_definition=frame.definition();
    const bool loops=memory.load8(loop_flags,at(device_definition,offsetof(DeviceDefinition,flags)))&1u;
    const auto target_value=memory.load32(step_target,at(d.group,offsetof(DeviceGroup,value)));
    bool plays=false;std::uint32_t effect{};
    if(frame.step_position(d,target_value,tick,step_acceleration,loops)) {
        // Arrived: opened when it was moving up, else closed.
        const bool was_up=frame.direction();
        device_definition=frame.definition();
        effect=was_up?memory.load32(opened_effect,at(device_definition,offsetof(DeviceDefinition,opened_effect))):
                      memory.load32(closed_effect,at(device_definition,offsetof(DeviceDefinition,closed_effect)));
        plays=true;
    } else {
        // The velocity turned (its product with the old one is not
        // positive): opening when it is now above the old one, else closing.
        const auto now=memory.load32(new_velocity,d.velocity);
        if(!sse::equal(now,0u)) {
            const auto before=frame.old_velocity();
            if(!sse::below(0u,sse::multiply(now,before))) {
                const bool opening=sse::above(memory.load32(new_velocity_again,d.velocity),before);
                device_definition=frame.definition();
                effect=opening?memory.load32(opening_effect,at(device_definition,offsetof(DeviceDefinition,opening_effect))):
                               memory.load32(closing_effect,at(device_definition,offsetof(DeviceDefinition,closing_effect)));
                plays=true;
            }
        }
    }
    if(plays)frame.play_effect(d,effect);
    // A moved position, or the first update, is reported.
    const auto before_position=frame.old_position();
    if(sse::equal(before_position,memory.load32(new_position,d.value)) &&
       !(memory.load8(initializing_flags,at(d.object,offsetof(Layout,device_flags)))&device_flags::initializing))
        return Approach::updated;
    frame.report_position(d);
    if(const auto block=frame.block()) {
        const auto channel=at(d.object,offsetof(Layout,position_channel));
        if(memory.load32(moved_graph,channel)!=none &&
           memory.load16(moved_animation,at(channel,offsetof(AnimationChannel,animation)))!=0xffffu)
            frame.advance_position(d,memory.load32(moved_position,d.value),block);
    }
    return Approach::updated;
}

// The power approaching its group's value (step 3, unscripted).
template<class Memory,class Frame> Approach approach_power(Memory &memory,Frame &frame,Device &d) {
    using enum Access;
    using Layout=native_device_update::Device;
    const auto group=memory.load32(power_group,at(d.object,offsetof(Layout,power_group)));
    if(group==none)return Approach::none;
    const auto groups=memory.load32(power_group_table,device_globals::device_groups);
    const auto values=memory.load32(power_group_data,at(groups,offsetof(DataArray,data)));
    d.power_group=element(values,handle_index(group),sizeof(DeviceGroup));
    const auto target=memory.load32(power_group_value,at(d.power_group,offsetof(DeviceGroup,value)));
    d.value=at(d.object,offsetof(Layout,power));
    if(sse::equal(target,memory.load32(power_compare,d.value))) {
        const auto velocity=memory.load32(power_velocity_compare,at(d.object,offsetof(Layout,power_velocity)));
        if(sse::equal(velocity,memory.load32(power_velocity_zero,device_globals::zero)))return Approach::idle;
    }
    if(frame.block() && frame.channel_valid(d))frame.follow_power(d,memory.load32(follow_power,d.value));
    auto device_definition=frame.definition();
    const auto acceleration=memory.load32(power_acceleration,at(device_definition,offsetof(DeviceDefinition,power_acceleration)));
    const auto game_clock=memory.load32(power_clock,device_globals::game_time);
    const auto tick=memory.load32(power_tick_length,at(game_clock,offsetof(GameTime,tick_length)));
    device_definition=frame.definition();
    const auto before=memory.load32(old_power,d.value);
    const auto speed=memory.load32(power_rate,at(device_definition,offsetof(DeviceDefinition,power_rate)));
    const auto target_value=memory.load32(power_target,at(d.power_group,offsetof(DeviceGroup,value)));
    frame.step_power(d,target_value,tick,acceleration,before,speed);
    const auto stepped_from=frame.old_power();
    if(!sse::equal(stepped_from,memory.load32(new_power,d.value))) {
        frame.report_power(d);
        if(const auto block=frame.block(); block && frame.channel_valid(d))
            frame.advance_power(d,memory.load32(moved_power,d.value),block);
    }
    return Approach::updated;
}

template<class Memory,class Frame> Result run(Memory &memory,Frame &frame) {
    using enum Access;
    using Layout=native_device_update::Device;
    Device d{};
    d.handle=frame.handle();
    d.row=handle_index(d.handle)*std::uint32_t(sizeof(ObjectHeader));
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    const auto instances=memory.load32(tags,device_globals::tag_instances);
    d.object=memory.load32(object,at(rows+d.row,offsetof(ObjectHeader,object)));
    const auto tag=memory.load32(definition_index,at(d.object,offsetof(ObjectData,definition)));
    const bool has_block=memory.load16(animation_offset,at(d.object,offsetof(Layout,animation_offset)))!=0xffffu;
    const auto definition_data=memory.load32(definition,tag_row(instances,tag));
    frame.begin(d,definition_data);
    if(has_block)
        frame.keep_block(widen16(memory.load16(animation_offset_again,at(d.object,offsetof(Layout,animation_offset))))+d.object);

    // 1. `moving` is recomputed by this update.
    const auto flags=memory.load32(flags_load,at(d.object,offsetof(Layout,device_flags)))&~device_flags::moving;
    memory.store32(flags_store,at(d.object,offsetof(Layout,device_flags)),flags);

    // 2. The position.
    if(flags&device_flags::scripted_position) {
        if(!(flags&device_flags::position_script_done)) {
            const auto before=memory.load32(script_old_position,at(d.object,offsetof(Layout,position)));
            const bool done=frame.script_position(d,before);
            const auto after=frame.scripted_position();
            const auto clock_value=memory.load32(clock,device_globals::game_time);
            memory.store32(script_store,at(d.object,offsetof(Layout,position)),after);
            const auto change=sse::subtract(after,frame.script_start());
            const auto velocity=sse::divide(change,memory.load32(tick_length,at(clock_value,offsetof(GameTime,tick_length))));
            const auto script_flags_value=memory.load32(script_flags,at(d.object,offsetof(Layout,device_flags)));
            memory.store32(script_velocity,at(d.object,offsetof(Layout,position_velocity)),velocity);
            memory.store32(script_flags_store,at(d.object,offsetof(Layout,device_flags)),
                done?script_flags_value|device_flags::position_script_done:script_flags_value&~device_flags::position_script_done);
            frame.report_script_position(d);
            frame.mark_changed();
        }
    } else if(approach_position(memory,frame,d,flags,definition_data)==Approach::updated) {
        d.row=frame.row();
        frame.mark_changed();
    }

    // 3. The power.
    const auto power_flags_value=memory.load32(power_flags,at(d.object,offsetof(Layout,device_flags)));
    if(power_flags_value&device_flags::scripted_power) {
        if(!(power_flags_value&device_flags::power_script_done)) {
            const bool done=frame.script_power(d);
            const auto script_flags_value=memory.load32(power_script_flags,at(d.object,offsetof(Layout,device_flags)));
            memory.store32(power_script_store,at(d.object,offsetof(Layout,device_flags)),
                done?script_flags_value|device_flags::power_script_done:script_flags_value&~device_flags::power_script_done);
            frame.report_script_power(d);
            frame.mark_changed();
        }
    } else if(!(power_flags_value&device_flags::scripted_position)) {
        switch(approach_power(memory,frame,d)) {
        case Approach::updated:frame.mark_changed();d.row=frame.row();break;
        case Approach::idle:d.row=frame.row();break;
        case Approach::none:break;
        }
    }

    // 4. The first update of a device with a position.
    const auto final_flags_value=memory.load32(final_flags,at(d.object,offsetof(Layout,device_flags)));
    if(!(final_flags_value&device_flags::scripted_position) &&
       memory.load32(final_group,at(d.object,offsetof(Layout,position_group)))==none)
        return {Exit::changed,frame.changed()};
    if(!(final_flags_value&device_flags::initializing))return {Exit::changed,frame.changed()};
    const auto type_array=memory.load32(type_table,globals::object_headers);
    const auto type_rows=memory.load32(type_headers,at(type_array,offsetof(DataArray,data)));
    if(is_machine(memory.load8(object_type,at(type_rows+d.row,offsetof(ObjectHeader,type)))))frame.notify_machine(d);
    // The flag stays while the object counts down its awake ticks.
    bool counting=false;
    if(frame.handle_argument()!=none) {
        const auto countdown_array=memory.load32(countdown_table,globals::object_headers);
        const auto countdown_rows=memory.load32(countdown_headers,at(countdown_array,offsetof(DataArray,data)));
        const auto data=memory.load32(countdown_object,at(countdown_rows+d.row,offsetof(ObjectHeader,object)));
        counting=memory.load8(countdown,at(data,offsetof(ObjectData,countdown)))!=0;
    }
    if(!counting) {
        const auto value=memory.load32(initializing_clear,at(d.object,offsetof(Layout,device_flags)));
        memory.store32(initializing_clear,at(d.object,offsetof(Layout,device_flags)),value&~device_flags::initializing);
    }
    return {Exit::initialized,1};
}
} // namespace device_update
} // namespace halo2::native_device_update
