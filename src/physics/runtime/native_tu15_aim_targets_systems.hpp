#pragma once
// Native TU1.5 aim target chain, entered at 1AE2B0 (combat action unit). The
// weapon targeting search (1AE090) calls it for every object the cluster
// object query (BD460) found near the aim ray, and it calls itself for the
// objects attached to a target:
//
//   aim_targets::evaluate_chain   1AE2B0   an object, its next siblings and,
//                                          for each one that became a target,
//                                          its attached objects
//
// For each object of the chain (the first object, then each next sibling):
//   1. only bipeds, vehicles and machines (types 0, 1, 7) are candidates;
//      an object whose header is flagged 0x10 or whose data is flagged 1 is
//      skipped;
//   2. a biped or vehicle is skipped unless the global limit (46A0D0) is
//      greater than its value at +0x2B0 (unordered values are skipped);
//   3. its bounding sphere must pass the aim test the aim cluster flood
//      (14D200) uses for portals: not behind the ray, not beyond its reach,
//      inside the cone that widens along the ray;
//   4. 1AE500 decides whether it is a target; it gets a pointer to the count
//      argument's slot, whose low byte is cleared for each object, and may
//      write its result there;
//   5. a target is added by 1AE7F0, which returns the new target count; at
//      64 or more the walk stops and returns it;
//   6. the target's attached objects (the chain of its first child) are
//      walked the same way by a recursive call; again at 64 the walk stops.
// The result is the target count.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_aim_targets.hpp) maps the role to the original instruction
// and owns the ABI of the three children (registers, pushes, flags, XMM).
// The arguments and the one local (the current object's data, in the slot
// the original reserves with PUSH ECX) are read and written as memory
// through Frame::local; the original also overwrites the first-object
// argument with the current object's data and reads it back.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width, including the
//     header table read three times per object and the arguments read again
//     for each child call;
//   - the aim test runs the original's scalar SSE on the live MXCSR in the
//     original's order and operand order (dx * dir.x has dx on the left),
//     with COMISS comparisons;
//   - the values the original keeps in callee-saved registers across the
//     children are a Kept, which the adapter refreshes from the registers
//     each child actually returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_aim_targets {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
namespace sse=native_aim_clusters::sse;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of ObjectHeader
inline constexpr std::uint32_t unit_limit=0x46a0d0u;       // float: units at or above it are not candidates
using native_aim_clusters::globals::cone_slope;            // float: how fast the aim cone widens
}
// The DataArray field this walk reads: its datums.
inline constexpr std::uint32_t data_array_data=0x44u;
// An object header (12 bytes) in the object header table.
struct ObjectHeader {
    std::uint16_t identifier;
    std::uint8_t flags;          // +2: 0x10 skips the object here
    std::uint8_t type;           // +3: 0 biped, 1 vehicle, 7 machine, ...
    std::uint32_t unused_here;
    std::uint32_t data;          // +8: the object's data
};
static_assert(sizeof(ObjectHeader)==12 && offsetof(ObjectHeader,flags)==2 && offsetof(ObjectHeader,type)==3 &&
              offsetof(ObjectHeader,data)==8);
// The object data fields this walk reads.
struct ObjectData {
    std::uint8_t unused_here_00[4];
    std::uint8_t flags;          // +4: bit 0 skips the object here
    std::uint8_t unused_here_05[7];
    std::uint32_t next_sibling;  // +0x0C: object handle, NONE at the end
    std::uint32_t first_child;   // +0x10: object handle, NONE for none
    std::uint8_t unused_here_14[0x1c];
    float center[3];             // +0x30: the bounding sphere
    float radius;                // +0x3C
};
static_assert(offsetof(ObjectData,flags)==4 && offsetof(ObjectData,next_sibling)==0xc &&
              offsetof(ObjectData,first_child)==0x10 && offsetof(ObjectData,center)==0x30 &&
              offsetof(ObjectData,radius)==0x3c);
// A biped's or vehicle's value compared with the unit limit; its game
// meaning is not established.
inline constexpr std::uint32_t unit_value_offset=0x2b0u;
namespace object_type {
inline constexpr std::uint32_t candidates=0x83u;            // 1 << biped | 1 << vehicle | 1 << machine
inline constexpr std::uint32_t units=0x03u;                 // biped, vehicle
}
inline constexpr std::uint32_t target_limit=64u;
struct Vector3 {float x,y,z;};

namespace aim_targets {
// The arguments (the first is nearest the return address) and the local.
enum class Local {
    current,           // the current object's data (the original's PUSH ECX slot)
    search,            // passed to 1AE7F0 and to the recursion; the game meaning is not established
    object,            // the first object's handle; then the current object's data
    point,             // Vector3*: where the aim ray starts
    direction,         // Vector3*: the aim ray's direction
    max_distance,      // float: the ray's length
    width,             // float: the cone's width factor
    spread,            // float: the cone's spread
    test_first,        // passed to 1AE500; the game meaning is not established
    test_second,       // passed to 1AE500; the game meaning is not established
    count,             // the target count in; its slot is also 1AE500's result byte
    add_context        // passed to 1AE7F0 and to the recursion; the game meaning is not established
};
enum class Access {
    first_handle,headers,count_argument,
    header_data,object_data,result_clear,header_data_again,object_data_again,current_store,object_store,
    type,header_flags,current_reload,object_flags,unit_limit,unit_value,
    header_data_sphere,object_data_sphere,radius,center_x,point_argument,center_y,center_z,direction_argument,
    point_y,point_z,direction_y,direction_z,point_x,direction_x,max_distance,width,cone_slope,spread,
    test_second,test_first,
    add_context,add_point,add_result,add_search,
    object_for_children,first_child,
    recurse_second,recurse_first,recurse_spread,recurse_width,recurse_distance,recurse_point,
    headers_reload,object_for_next,next_sibling
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t handle;     // EBX: the current object's handle; the search after a target is added
    std::uint32_t direction;  // ESI: the direction argument, once a sphere was tested
    std::uint32_t count;      // EDI: the target count
    std::uint32_t context;    // EBP: the object header table; the add context after a target
};

// The aim test of one object's bounding sphere, with the values the
// original leaves in its XMM registers.
enum class Stage {behind,beyond,outside_cone,inside};
struct SphereTest {
    Stage result;
    std::uint32_t radius,dx,dy,dz,along,negative_radius,reach,cone_bound,spread,spread_bound;
};
template<class Memory,class Local_> SphereTest sphere_test(Memory &memory,std::uint32_t object,Kept &kept,Local_ local) {
    using enum Access;
    SphereTest t{};
    t.radius=memory.load32(radius,at(object,offsetof(ObjectData,radius)));
    const auto center_x_bits=memory.load32(center_x,at(object,offsetof(ObjectData,center[0])));
    const auto point=memory.load32(point_argument,local(Local::point));
    const auto center_y_bits=memory.load32(center_y,at(object,offsetof(ObjectData,center[1])));
    const auto center_z_bits=memory.load32(center_z,at(object,offsetof(ObjectData,center[2])));
    kept.direction=memory.load32(direction_argument,local(Local::direction));
    t.dy=sse::subtract(center_y_bits,memory.load32(point_y,at(point,offsetof(Vector3,y))));
    t.dz=sse::subtract(center_z_bits,memory.load32(point_z,at(point,offsetof(Vector3,z))));
    const auto direction_y_bits=memory.load32(direction_y,at(kept.direction,offsetof(Vector3,y)));
    const auto direction_z_bits=memory.load32(direction_z,at(kept.direction,offsetof(Vector3,z)));
    t.dx=sse::subtract(center_x_bits,memory.load32(point_x,at(point,offsetof(Vector3,x))));
    // along = (dir.y*dy + dir.z*dz) + dx*dir.x: how far along the ray the center is.
    t.along=sse::add(sse::multiply(direction_y_bits,t.dy),sse::multiply(direction_z_bits,t.dz));
    t.along=sse::add(t.along,sse::multiply(t.dx,memory.load32(direction_x,at(kept.direction,offsetof(Vector3,x)))));
    t.negative_radius=sse::subtract(0u,t.radius);
    if(sse::below(t.along,t.negative_radius)){t.result=Stage::behind;return t;}
    t.reach=sse::add(t.radius,memory.load32(max_distance,local(Local::max_distance)));
    if(sse::below(t.reach,t.along)){t.result=Stage::beyond;return t;}
    // Inside the cone: (r*width*slope + along)*along + r*r against
    // |center - point|^2 * spread * spread.
    auto cone=sse::multiply(t.radius,memory.load32(width,local(Local::width)));
    cone=sse::multiply(cone,memory.load32(cone_slope,globals::cone_slope));
    cone=sse::multiply(sse::add(cone,t.along),t.along);
    t.cone_bound=sse::add(cone,sse::multiply(t.radius,t.radius));
    auto length_squared=sse::add(sse::multiply(t.dx,t.dx),sse::multiply(t.dy,t.dy));
    length_squared=sse::add(length_squared,sse::multiply(t.dz,t.dz));
    t.spread=memory.load32(spread,local(Local::spread));
    t.spread_bound=sse::multiply(sse::multiply(length_squared,t.spread),t.spread);
    t.result=sse::below(t.cone_bound,t.spread_bound)?Stage::outside_cone:Stage::inside;
    return t;
}

// Frame:
//   local(Local)          an argument's or the local's address, as the
//                         original addresses it now
//   tested(SphereTest)    the aim test ran
//   is_target(kept)       1AE500 with the loaded arguments; returns AL != 0
//   add_target(kept)      1AE7F0 with the loaded arguments; returns the count
//   attached(kept)        the recursive walk of the target's first child's
//                         chain with the loaded arguments; returns the count
// The three child hooks refresh kept from the registers the child returned.
// Returns the target count (the original's EDI, returned in EAX).
template<class Memory,class Frame> std::uint32_t evaluate_chain(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{};
    kept.handle=memory.load32(first_handle,local(Local::object));
    kept.context=memory.load32(headers,globals::object_headers);
    kept.count=memory.load32(count_argument,local(Local::count));
    for(;;) {
        const auto index=(kept.handle&0xffffu)*std::uint32_t(sizeof(ObjectHeader));
        const auto object=memory.load32(object_data,
            at(memory.load32(header_data,at(kept.context,data_array_data))+index,offsetof(ObjectHeader,data)));
        memory.store8(result_clear,local(Local::count),0);
        const auto header=memory.load32(header_data_again,at(kept.context,data_array_data))+index;
        const auto same_object=memory.load32(object_data_again,at(header,offsetof(ObjectHeader,data)));
        memory.store32(current_store,local(Local::current),same_object);
        memory.store32(object_store,local(Local::object),object);
        const auto type_bit=1u<<(memory.load8(type,at(header,offsetof(ObjectHeader,type)))&31u);
        bool candidate=(type_bit&object_type::candidates) &&
                       !(memory.load8(header_flags,at(header,offsetof(ObjectHeader,flags)))&0x10u);
        if(candidate) {
            const auto current=memory.load32(current_reload,local(Local::current));
            candidate=!(memory.load8(object_flags,at(current,offsetof(ObjectData,flags)))&1u);
            if(candidate && (type_bit&object_type::units)) {
                const auto limit=memory.load32(unit_limit,globals::unit_limit);
                candidate=!sse::not_above(limit,memory.load32(unit_value,at(current,unit_value_offset)));
            }
        }
        if(candidate) {
            const auto sphere=memory.load32(object_data_sphere,
                at(memory.load32(header_data_sphere,at(kept.context,data_array_data))+index,offsetof(ObjectHeader,data)));
            const auto test=sphere_test(memory,sphere,kept,local);
            frame.tested(test);
            if(test.result==Stage::inside) {
                (void)memory.load32(test_second,local(Local::test_second));
                (void)memory.load32(test_first,local(Local::test_first));
                if(frame.is_target(kept)) {
                    kept.context=memory.load32(add_context,local(Local::add_context));
                    (void)memory.load32(add_point,local(Local::point));
                    (void)memory.load32(add_result,local(Local::count));
                    kept.handle=memory.load32(add_search,local(Local::search));
                    kept.count=frame.add_target(kept);
                    if(signed_value(kept.count)>=signed_value(target_limit))return kept.count;
                    const auto target=memory.load32(object_for_children,local(Local::object));
                    if(memory.load32(first_child,at(target,offsetof(ObjectData,first_child)))!=none) {
                        (void)memory.load32(recurse_second,local(Local::test_second));
                        (void)memory.load32(recurse_first,local(Local::test_first));
                        (void)memory.load32(recurse_spread,local(Local::spread));
                        (void)memory.load32(recurse_width,local(Local::width));
                        (void)memory.load32(recurse_distance,local(Local::max_distance));
                        (void)memory.load32(recurse_point,local(Local::point));
                        kept.count=frame.attached(kept);
                        if(signed_value(kept.count)>=signed_value(target_limit))return kept.count;
                    }
                }
                kept.context=memory.load32(headers_reload,globals::object_headers);
            }
        }
        const auto current=memory.load32(object_for_next,local(Local::object));
        kept.handle=memory.load32(next_sibling,at(current,offsetof(ObjectData,next_sibling)));
        if(kept.handle==none)return kept.count;
    }
}
} // namespace aim_targets
} // namespace halo2::native_aim_targets
