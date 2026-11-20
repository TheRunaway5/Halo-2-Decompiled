#pragma once
// Native TU1.5 player action search: what a player's "action" input would
// act on, entered at 153BA0 (combat action unit; the player input producer
// 1898A0 calls it for each local player, into its command at +0x28).
//
//   action_context::find   153BA0  the objects near the player's unit that
//                                  the action input could use
//
// The result is the player's action context (12 bytes): an interaction
// (type and data words, then an object) and a melee target. It starts empty
// (no interaction, no object, no melee target). Nothing more happens when the
// player has no unit or the unit has a parent (it rides in a seat).
// Otherwise two object searches run around the unit's bounding sphere
// (sphere query BD080, at most 64 objects each):
//   1. weapons, within the unit's bounding radius plus 0.4 world units;
//   2. every other type, within that radius plus another 3.0.
// Garbage, projectiles, sound scenery and creatures are never considered.
// Each found object, in the order found:
//   - weapons, machines and controls must also be within reach: the
//     distance between the centers no more than the object's bounding
//     radius plus the unit's pickup reach (radius + 0.4);
//   - a biped is tested by 1540D0, a vehicle by 153F20, a weapon by 154270
//     and a control by 1541B0, each with the player and the action context
//     (they fill in the context when the object is a better action);
//   - any other object's attached objects (its children, then their
//     siblings) are appended to the found list while it has room, so they
//     are considered in turn.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_player_action.hpp) maps the role to the original instruction
// for the diagnostic cursor. The calls into translated code (the sphere
// query and the four tests), the original's case table, registers and flags
// belong to that adapter, reached through a Frame object. The search's
// locals (the radii, the type masks, the found list) live in the original's
// stack frame, so they are read and written as memory, with roles, in the
// original's order; Frame::local gives their addresses (the frame moves when
// a child does not restore the stack pointer), Frame::argument the caller's.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the context
//     is cleared by two inlined initialisers; the locals and the caller's
//     arguments are read again for every search and test);
//   - the single-precision arithmetic runs on the live MXCSR in the
//     original's order (x, then z, then y), so rounding, NaN payloads and
//     sticky exception flags match; an unordered distance is out of reach;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a call
//     (the pass across the sphere query; the found count and the position
//     across a test) are a Pass and a Scan. A conforming callee returns them
//     unchanged; the adapter hands back what the callee actually returned, so
//     the search continues exactly as the original does even when a callee
//     does not preserve them. That is also why the passes are a loop over the
//     pass offset the original keeps in its frame.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_player_action {
using native_object_update::DataArray;
using native_object_update::ObjectData;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_object_update::object_of;
using native_object_update::signed_value;
using native_object_update::widen8;
using native_object_update::widen16;

// ------------------------------------------------------- single precision
// The original's scalar SSE on the live MXCSR, on bit patterns; the left
// operand is the destination register.
namespace sse {
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("addss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("subss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("mulss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
// COMISS a,b; JB: a < b, or unordered.
inline bool below(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
#else
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(_mm_add_ss(scalar(a),scalar(b)));}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){return bits(_mm_sub_ss(scalar(a),scalar(b)));}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(scalar(a),scalar(b)));}
inline bool below(std::uint32_t a,std::uint32_t b){return !_mm_comige_ss(scalar(a),scalar(b));}
#endif
} // namespace sse

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;           // DataArray* of Player
inline constexpr std::uint32_t object_headers=native_object_update::globals::object_headers;
inline constexpr std::uint32_t pickup_margin=0x46a240u;     // float 0.4: added to the unit's bounding radius
inline constexpr std::uint32_t search_margin=0x46a0e4u;     // float 3.0: added again for the wider search
}

// A player datum (0x21C bytes): the field this system reads.
struct Player {
    std::uint8_t unused_here_00[0x2c];
    std::uint32_t unit;                // +0x2C: the player's unit (object handle), or NONE
};
static_assert(offsetof(Player,unit)==0x2c);
inline constexpr std::uint32_t player_size=0x21cu;

// The object fields the search reads beyond ObjectData's shared ones.
struct ObjectBounds {
    std::uint8_t unused_here_00[0x28];
    std::uint32_t location[2];         // +0x28: the object's location (cluster), for the sphere query
    float center[3];                   // +0x30: bounding sphere center (x, y, z)
    float radius;                      // +0x3C: bounding sphere radius
};
static_assert(offsetof(ObjectBounds,location)==0x28 && offsetof(ObjectBounds,center)==0x30 &&
              offsetof(ObjectBounds,radius)==0x3c);

// The player's action context (s_player_action_context).
struct ActionContext {
    std::uint32_t interaction;         // +0: type and data words
    std::uint32_t object;              // +4: the interaction's object, or NONE
    std::uint32_t melee_target;        // +8: a unit, or NONE
};
static_assert(sizeof(ActionContext)==12);

// Object types (ObjectData::type) and the masks the search uses.
namespace object_type {
inline constexpr std::int32_t biped=0,vehicle=1,weapon=2,equipment=3,garbage=4,projectile=5,scenery=6,machine=7,
    control=8,light_fixture=9,sound_scenery=10,crate=11,creature=12;
constexpr std::uint32_t bit(std::int32_t type){return 1u<<(std::uint32_t(type)&31u);}  // SHL by CL
}
inline constexpr std::uint32_t ignored_types=object_type::bit(object_type::garbage)|object_type::bit(object_type::projectile)|
    object_type::bit(object_type::sound_scenery)|object_type::bit(object_type::creature);
static_assert(ignored_types==0x1430u);
inline constexpr std::uint32_t reach_types=object_type::bit(object_type::weapon)|object_type::bit(object_type::machine)|
    object_type::bit(object_type::control);
static_assert(reach_types==0x184u);
// The two searches' type masks, as the original stores them.
inline constexpr std::uint32_t weapon_types=object_type::bit(object_type::weapon);
inline constexpr std::uint32_t other_types=~object_type::bit(object_type::weapon);
inline constexpr std::uint32_t found_limit=64u;

// The search's locals: the original's 0x120-byte stack frame.
struct Locals {
    std::uint32_t center;              // +0x00: the unit's bounding center (address)
    std::uint32_t pass;                // +0x04: the current pass's byte offset in radii/type_masks
    std::uint32_t pickup_reach;        // +0x08: float, radius + 0.4
    std::uint32_t location;            // +0x0C: the unit's location (address)
    std::uint32_t radii[2];            // +0x10: float, each pass's search radius
    std::uint32_t type_masks[2];       // +0x18: each pass's object types
    std::uint32_t found[found_limit];  // +0x20: the found object handles
};
static_assert(sizeof(Locals)==0x120 && offsetof(Locals,pickup_reach)==8 && offsetof(Locals,radii)==0x10 &&
              offsetof(Locals,type_masks)==0x18 && offsetof(Locals,found)==0x20);
inline constexpr std::uint32_t pass_count_bytes=std::uint32_t(sizeof(Locals::radii));

// ---------------------------------------------------------- action context
// 153BA0. Frame:
//   argument(Argument)           the address of one of the caller's arguments
//   local(Local)                 the address of one of the search's locals
//   find_objects(pass, types, location, center, radius, found, limit)
//                                BD080: the sphere query; its AX result
//   reach_measured(reach)        the distance test's values (the adapter's registers)
//   action_case(type)            the case of the original's type table the object selects
//   test(test, scan, player, context)  the object's test (1540D0, 153F20, 154270 or 1541B0)
namespace action_context {
enum class Argument {player,context};
enum class Local {center,pass,pickup_reach,location,radii,type_masks,found};
enum class Access {
    player_argument,player_table,player_rows,context_argument,
    context_clear_0,context_clear_4,context_clear_8,interaction_clear,object_clear,object_none,melee_none,
    player_unit,object_table,object_headers,unit_object,unit_parent,unit_radius,pickup_margin,
    pickup_reach_store,pass_radius_store,search_margin,weapon_types_store,other_types_store,search_radius_store,
    location_store,center_store,pass_clear,
    pass_radius,center_reload,location_reload,pass_types,
    found_handle,found_table,found_headers,found_object,found_type,
    center_again,position_x,center_x,position_z,center_z,position_y,center_y,object_radius,reach_reload,
    test_context,test_player,
    first_attached,attached_append,attached_object,attached_sibling,
    pass_reload,pass_store
};
enum class Exit {no_unit,riding,searched};
// The object tests the original's type table selects; `attached` is its
// default (the object's attached objects join the found list).
enum class Case {unit,vehicle,weapon,control,attached};
// Kept across the sphere query.
struct Pass {
    std::uint32_t offset;     // byte offset of the pass's radius and type mask (0, then 4)
};
// Kept across a test.
struct Scan {
    std::uint32_t count;      // the found objects
    std::uint32_t index;      // the object being considered
};
// The distance test's values (bit patterns), as the original leaves them.
struct Reach {
    std::uint32_t dx,dz,dy;   // the object's center minus the unit's
    std::uint32_t reach;      // the object's radius plus the unit's pickup reach
    std::uint32_t distance_squared;
    std::uint32_t reach_squared;
};

// Is the object within reach of the unit? Its center may be no further from
// the unit's than its radius plus the pickup reach. Order: x, z, y.
template<class Memory,class Frame> bool within_reach(Memory &memory,Frame &frame,std::uint32_t object) {
    using enum Access;
    const auto center=memory.load32(center_again,frame.local(Local::center));
    Reach r{};
    const auto x=memory.load32(position_x,at(object,offsetof(ObjectBounds,center[0])));
    r.dx=sse::subtract(x,memory.load32(center_x,at(center,0)));
    const auto z=memory.load32(position_z,at(object,offsetof(ObjectBounds,center[2])));
    r.dz=sse::subtract(z,memory.load32(center_z,at(center,8)));
    const auto y=memory.load32(position_y,at(object,offsetof(ObjectBounds,center[1])));
    r.dy=sse::subtract(y,memory.load32(center_y,at(center,4)));
    const auto radius=memory.load32(object_radius,at(object,offsetof(ObjectBounds,radius)));
    r.reach=sse::add(radius,memory.load32(reach_reload,frame.local(Local::pickup_reach)));
    const auto xx=sse::multiply(r.dx,r.dx);
    const auto xz=sse::add(xx,sse::multiply(r.dz,r.dz));
    r.distance_squared=sse::add(xz,sse::multiply(r.dy,r.dy));
    r.reach_squared=sse::multiply(r.reach,r.reach);
    const bool out=sse::below(r.reach_squared,r.distance_squared);
    frame.reach_measured(r);
    return !out;
}

// One found object (scan.index): test it, or append its attached objects.
template<class Memory,class Frame> void consider(Memory &memory,Frame &frame,Scan &scan) {
    using enum Access;
    const auto handle=memory.load32(found_handle,element(frame.local(Local::found),scan.index,4));
    const auto table=memory.load32(found_table,globals::object_headers);
    const auto headers=memory.load32(found_headers,at(table,offsetof(DataArray,data)));
    const auto object=memory.load32(found_object,
        at(element(headers,handle_index(handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto type=widen8(memory.load8(found_type,at(object,offsetof(ObjectData,type))));
    const auto bit=object_type::bit(std::int32_t(type));
    if(bit&ignored_types)return;
    if((bit&reach_types) && !within_reach(memory,frame,object))return;
    const auto selected=frame.action_case(type);
    if(selected!=Case::attached) {
        const auto context=memory.load32(test_context,frame.argument(Argument::context));
        const auto player=memory.load32(test_player,frame.argument(Argument::player));
        frame.test(selected,scan,player,context);
        return;
    }
    // Its children, then each child's next sibling, while the list has room.
    auto attached=memory.load32(first_attached,at(object,offsetof(ObjectData,first_child)));
    while(attached!=none && scan.count<found_limit) {
        memory.store32(attached_append,element(frame.local(Local::found),scan.count,4),attached);
        const auto data=memory.load32(attached_object,
            at(element(headers,handle_index(attached),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
        attached=memory.load32(attached_sibling,at(data,offsetof(ObjectData,next_sibling)));
        ++scan.count;
    }
}

template<class Memory,class Frame> Exit find(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto player=memory.load32(player_argument,frame.argument(Argument::player));
    const auto players=memory.load32(player_table,globals::players);
    const auto rows=memory.load32(player_rows,at(players,offsetof(DataArray,data)));
    const auto context=memory.load32(context_argument,frame.argument(Argument::context));
    const auto record=element(rows,handle_index(player),player_size);

    // The context starts empty (two inlined initialisers: zero, then no
    // object and no melee target).
    memory.store32(context_clear_0,at(context,0),0);
    memory.store32(context_clear_4,at(context,4),0);
    memory.store32(context_clear_8,at(context,8),0);
    memory.store32(interaction_clear,at(context,offsetof(ActionContext,interaction)),0);
    memory.store32(object_clear,at(context,offsetof(ActionContext,object)),0);
    memory.store32(object_none,at(context,offsetof(ActionContext,object)),none);
    memory.store32(melee_none,at(context,offsetof(ActionContext,melee_target)),none);

    const auto unit=memory.load32(player_unit,at(record,offsetof(Player,unit)));
    if(unit==none)return Exit::no_unit;
    const auto object=object_of(memory,unit,object_table,object_headers,unit_object);
    if(memory.load32(unit_parent,at(object,offsetof(ObjectData,parent)))!=none)return Exit::riding;

    // The pickup reach and the two searches' radii and types.
    const auto radius=memory.load32(unit_radius,at(object,offsetof(ObjectBounds,radius)));
    const auto pickup_reach=sse::add(radius,memory.load32(pickup_margin,globals::pickup_margin));
    memory.store32(pickup_reach_store,local(Local::pickup_reach),pickup_reach);
    memory.store32(pass_radius_store,local(Local::radii),pickup_reach);
    const auto search_radius=sse::add(pickup_reach,memory.load32(search_margin,globals::search_margin));
    memory.store32(weapon_types_store,local(Local::type_masks),weapon_types);
    memory.store32(other_types_store,local(Local::type_masks)+4u,other_types);
    memory.store32(search_radius_store,local(Local::radii)+4u,search_radius);
    memory.store32(location_store,local(Local::location),at(object,offsetof(ObjectBounds,location)));
    memory.store32(center_store,local(Local::center),at(object,offsetof(ObjectBounds,center)));
    memory.store32(pass_clear,local(Local::pass),0);

    // The passes. The pass offset is read back from the frame after the
    // tests, so the loop runs on it (two passes when nothing disturbs it).
    Pass pass{0};
    do {
        const auto search=memory.load32(pass_radius,local(Local::radii)+pass.offset);
        const auto center=memory.load32(center_reload,local(Local::center));
        const auto location=memory.load32(location_reload,local(Local::location));
        const auto types=memory.load32(pass_types,local(Local::type_masks)+pass.offset)&~ignored_types;
        Scan scan{widen16(frame.find_objects(pass,types,location,center,search,local(Local::found),found_limit)),0};
        if(signed_value(scan.count)>0) {
            do consider(memory,frame,scan);
            while(signed_value(++scan.index)<signed_value(scan.count));
            pass.offset=memory.load32(pass_reload,local(Local::pass));
        }
        pass.offset+=4u;
        memory.store32(pass_store,local(Local::pass),pass.offset);
    } while(pass.offset<pass_count_bytes);
    return Exit::searched;
}
} // namespace action_context
} // namespace halo2::native_player_action
