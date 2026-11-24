#pragma once
// Native TU1.5 aim cluster flood, entered at 14D200 (combat action unit).
// The weapon targeting search (1AE090) calls it once per local player and
// tick, before it looks for targets:
//
//   aim_clusters::flood   14D200   the structure-BSP clusters an aim ray can
//                                  see into, from the cluster the ray starts
//                                  in, through the BSP's cluster portals
//
// The walk is depth-first with an explicit stack (the original's own 512
// entries in its frame). Every cluster taken off the stack is reached: it is
// counted, and written to the caller's output while there is room. Each of
// its portals leads to the cluster on its other side, which is pushed when
// it has not been visited during this flood and the portal's bounding
// sphere passes the aim test:
//   1. not behind the ray: along >= -radius, where along is the portal
//      center's distance along the aim direction;
//   2. not beyond the ray's reach: along <= radius + max distance;
//   3. inside the aim cone, which widens with the distance along the ray:
//        (radius * width * cone_slope + along) * along + radius^2
//          >= |offset|^2 * spread^2
//      with offset = center - point and spread the caller's XMM5.
// An unordered comparison (a NaN) fails the test, so the portal is skipped.
//
// "Visited during this flood" is a stamp: a global counter (4F3CE4) is
// incremented on entry and written to the cluster's slot of a global table
// (4F3CE8) when the cluster is pushed; a slot equal to the counter is
// visited. A global byte (4F3CE1) is set for the duration of the flood.
// The original does not check the cluster index: a portal side of NONE
// (0xFFFF) reads and writes the slot before the table, which is the counter
// itself, so it always reads as visited.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_aim_clusters.hpp) maps the role to the original instruction.
// The walk's locals (the portal position and count, the reached count, the
// portal table, the stack) and the arguments live in the original's
// 0x410-byte frame, so they are read and written as memory through
// Frame::local, in the original's order.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width; cluster and
//     portal indices are 16-bit and sign-extended; address arithmetic wraps
//     at 32 bits;
//   - the aim test runs the original's scalar SSE on the live MXCSR in the
//     original's order (y, then z, then x for the dot product; x, y, z for
//     the squared length; 0 - radius rather than a negation), so rounding,
//     NaN payloads and sticky exception flags match; the comparisons are
//     COMISS (a quiet NaN raises the invalid flag);
//   - the values each portal test leaves in XMM0-XMM4 and XMM6 are reported
//     to the frame (PortalTest), which keeps them as the original's
//     registers.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_aim_clusters {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;

// ------------------------------------------------------- single precision
// The original's scalar SSE on the live MXCSR, on bit patterns; the left
// operand is the destination register.
namespace sse {
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("addss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("subss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("mulss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("divss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
// COMISS a,b; JB: a < b, or unordered.
inline bool below(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
// COMISS a,b; JBE: a <= b, or unordered (the aim target chain's unit gate).
inline bool not_above(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tsetbe %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
#else
inline constexpr bool available=false;
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(_mm_add_ss(scalar(a),scalar(b)));}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){return bits(_mm_sub_ss(scalar(a),scalar(b)));}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(scalar(a),scalar(b)));}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){return bits(_mm_div_ss(scalar(a),scalar(b)));}
inline bool below(std::uint32_t a,std::uint32_t b){return !_mm_comige_ss(scalar(a),scalar(b));}
inline bool not_above(std::uint32_t a,std::uint32_t b){return !_mm_comigt_ss(scalar(a),scalar(b));}
#endif
} // namespace sse

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;     // the loaded structure BSP
inline constexpr std::uint32_t flood_active=0x4f3ce1u;      // byte: 1 while the flood runs
inline constexpr std::uint32_t visit_stamp=0x4f3ce4u;       // the current flood's stamp
inline constexpr std::uint32_t cluster_stamps=0x4f3ce8u;    // one stamp per cluster index
inline constexpr std::uint32_t cone_slope=0x46a11cu;        // float: how fast the cone widens
}
// A tag block: the element count and the address of the first element.
struct TagBlock {
    std::int32_t count;
    std::uint32_t elements;
};
// The structure BSP blocks the flood uses.
struct StructureBsp {
    std::uint8_t unused_here_00[0x5c];
    TagBlock cluster_portals;   // ClusterPortal[]
    std::uint8_t unused_here_64[0x38];
    TagBlock clusters;          // Cluster[]
};
static_assert(offsetof(StructureBsp,cluster_portals.elements)==0x60 && offsetof(StructureBsp,clusters.elements)==0xa0);
// A cluster portal joins two clusters; its bounding sphere is the part the
// aim test reads.
struct ClusterPortal {
    std::int16_t front_cluster;
    std::int16_t back_cluster;
    std::uint8_t unused_here_04[4];
    float center[3];
    float radius;
    std::uint8_t unused_here_18[12];
};
static_assert(sizeof(ClusterPortal)==36 && offsetof(ClusterPortal,center)==8 && offsetof(ClusterPortal,radius)==0x14);
// A cluster: the indices (16-bit) of its portals.
struct Cluster {
    std::uint8_t unused_here_00[0x8c];
    TagBlock portals;
    std::uint8_t unused_here_94[0x1c];
};
static_assert(sizeof(Cluster)==0xb0 && offsetof(Cluster,portals.count)==0x8c && offsetof(Cluster,portals.elements)==0x90);
// A point or direction argument: three floats.
struct Vector3 {float x,y,z;};
static_assert(sizeof(Vector3)==12);

namespace aim_clusters {
// The original keeps these in its stack frame (Frame::local).
enum class Local {
    portal,          // the position in the current cluster's portals
    portal_count,    // the current cluster's portal count
    reached,         // clusters reached so far
    portals,         // the BSP's portal table
    stack,           // 512 cluster indices (16-bit)
    point,           // argument: where the aim ray starts
    direction,       // argument: the aim direction
    max_distance,    // argument: float, how far the ray reaches
    width,           // argument: float, the cone's width factor
    capacity,        // argument: 16-bit, the output's room
    output,          // argument: the output (16-bit cluster indices)
    count_output     // argument: where the clamped count goes, or null
};
enum class Access {
    stamp_read,stamp_store,active_set,start_stamp,start_mark,stack_start,
    bsp,clusters,stack_pop,capacity_check,output_argument,output_store,portal_count,reached_store,portal_clear,
    portal_count_store,portals,portal_indices,portals_store,
    portal_index,portals_reload,front,back,stamp_check,other_stamp,
    point_argument,center_x,point_x,center_y,point_y,center_z,point_z,direction_argument,direction_y,direction_z,
    radius,direction_x,max_distance,width,cone_slope,
    stamp_mark,other_mark,stack_push,
    portal_read,portal_store,portal_bound,reached_reload,
    count_output_argument,active_clear,count_store,capacity_result
};
// How far one portal's aim test went, and the values it computed (the
// adapter keeps them as the original's XMM registers).
enum class Test {
    behind,        // failed 1
    beyond,        // failed 2
    outside_cone,  // failed 3
    inside         // passed: the far cluster is pushed
};
struct PortalTest {
    Test result;
    std::uint32_t dx,dy,dz;           // center - point
    std::uint32_t radius;
    std::uint32_t along;              // dot(direction, offset)
    std::uint32_t negative_radius;    // 0 - radius
    std::uint32_t reach;              // radius + max distance
    std::uint32_t cone_bound;         // (radius*width*slope + along)*along + radius^2
    std::uint32_t radius_squared;
    std::uint32_t length_squared;     // dx^2 + dy^2 + dz^2
    std::uint32_t dz_squared;
    std::uint32_t spread_bound;       // length_squared * spread * spread
};
// What the flood returns: the reached count (32-bit) and the caller's
// capacity (read as 32 bits); the caller gets the count clamped to it.
struct Result {
    std::uint32_t reached;
    std::uint32_t capacity;
};

// The aim test of one portal's bounding sphere (see the file comment), in
// the original's order of loads and operations.
template<class Memory> PortalTest aim_test(Memory &memory,std::uint32_t portal,std::uint32_t spread,
                                           const auto &local) {
    using enum Access;
    PortalTest t{};
    // Each load is its own statement: the original reads the center, then
    // the point, axis by axis.
    const auto point=memory.load32(point_argument,local(Local::point));
    const auto offset=[&](Access center,Access from,std::size_t center_field,std::size_t point_field) {
        const auto c=memory.load32(center,at(portal,std::uint32_t(center_field)));
        const auto p=memory.load32(from,at(point,std::uint32_t(point_field)));
        return sse::subtract(c,p);
    };
    t.dx=offset(center_x,point_x,offsetof(ClusterPortal,center[0]),offsetof(Vector3,x));
    t.dy=offset(center_y,point_y,offsetof(ClusterPortal,center[1]),offsetof(Vector3,y));
    t.dz=offset(center_z,point_z,offsetof(ClusterPortal,center[2]),offsetof(Vector3,z));
    const auto direction=memory.load32(direction_argument,local(Local::direction));
    const auto direction_y_bits=memory.load32(direction_y,at(direction,offsetof(Vector3,y)));
    const auto direction_z_bits=memory.load32(direction_z,at(direction,offsetof(Vector3,z)));
    t.radius=memory.load32(radius,at(portal,offsetof(ClusterPortal,radius)));
    // along = (dir.y*dy + dir.z*dz) + dir.x*dx
    const auto z_part=sse::multiply(direction_z_bits,t.dz);
    t.along=sse::add(sse::multiply(direction_y_bits,t.dy),z_part);
    t.along=sse::add(t.along,sse::multiply(memory.load32(direction_x,at(direction,offsetof(Vector3,x))),t.dx));
    // 1. Not behind the ray.
    t.negative_radius=sse::subtract(0u,t.radius);
    if(sse::below(t.along,t.negative_radius)){t.result=Test::behind;return t;}
    // 2. Not beyond its reach.
    t.reach=sse::add(t.radius,memory.load32(max_distance,local(Local::max_distance)));
    if(sse::below(t.reach,t.along)){t.result=Test::beyond;return t;}
    // 3. Inside the cone.
    t.cone_bound=sse::multiply(t.radius,memory.load32(width,local(Local::width)));
    t.cone_bound=sse::multiply(t.cone_bound,memory.load32(cone_slope,globals::cone_slope));
    t.cone_bound=sse::multiply(sse::add(t.cone_bound,t.along),t.along);
    t.radius_squared=sse::multiply(t.radius,t.radius);
    t.cone_bound=sse::add(t.cone_bound,t.radius_squared);
    t.length_squared=sse::add(sse::multiply(t.dx,t.dx),sse::multiply(t.dy,t.dy));
    t.dz_squared=sse::multiply(t.dz,t.dz);
    t.length_squared=sse::add(t.length_squared,t.dz_squared);
    t.spread_bound=sse::multiply(sse::multiply(t.length_squared,spread),spread);
    t.result=sse::below(t.cone_bound,t.spread_bound)?Test::outside_cone:Test::inside;
    return t;
}

// Frame:
//   local(Local)        the address of a local or argument in the original's
//                       frame
//   tested(PortalTest)  one portal's aim test ran (the adapter keeps its
//                       values as the original's XMM registers)
// The start cluster is the caller's 16-bit DX; spread is its XMM5 (bits).
template<class Memory,class Frame> Result flood(Memory &memory,Frame &frame,std::uint32_t start,std::uint32_t spread) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    // A new stamp for this flood; the start cluster is visited and stacked.
    const auto stamp=memory.load32(stamp_read,globals::visit_stamp)+1u;
    memory.store32(stamp_store,globals::visit_stamp,stamp);
    memory.store8(active_set,globals::flood_active,1);
    const auto start_slot=element(globals::cluster_stamps,widen16(start),4);
    if(memory.load32(start_stamp,start_slot)!=stamp)memory.store32(start_mark,start_slot,stamp);
    memory.store16(stack_start,local(Local::stack),start);
    std::uint32_t depth=1;      // the stack's height; tested as signed 16-bit
    std::uint32_t reached=0;

    do {
        const auto structure=memory.load32(bsp,globals::structure_bsp);
        const auto cluster_table=memory.load32(clusters,at(structure,offsetof(StructureBsp,clusters.elements)));
        --depth;
        const auto here=memory.load16(stack_pop,element(local(Local::stack),widen16(depth),2));
        const auto cluster=element(cluster_table,widen16(here),sizeof(Cluster));
        // Reached: written to the output while there is room (the count and
        // the capacity compare as signed 16-bit values).
        if(std::int16_t(reached)<std::int16_t(memory.load16(capacity_check,local(Local::capacity)))) {
            const auto output=memory.load32(output_argument,local(Local::output));
            memory.store16(output_store,element(output,widen16(reached),2),here);
        }
        const auto portal_total=memory.load32(portal_count,at(cluster,offsetof(Cluster,portals.count)));
        ++reached;
        memory.store32(reached_store,local(Local::reached),reached);
        memory.store32(portal_clear,local(Local::portal),0);
        memory.store32(portal_count_store,local(Local::portal_count),portal_total);
        if(signed_value(portal_total)<=0)continue;

        const auto portal_table=memory.load32(portals,at(structure,offsetof(StructureBsp,cluster_portals.elements)));
        const auto indices=memory.load32(portal_indices,at(cluster,offsetof(Cluster,portals.elements)));
        memory.store32(portals_store,local(Local::portals),portal_table);
        std::uint32_t position=0;   // tested and used as signed 16-bit
        do {
            const auto number=widen16(memory.load16(portal_index,element(indices,position,2)));
            const auto portal=element(memory.load32(portals_reload,local(Local::portals)),number,sizeof(ClusterPortal));
            // The far side: the front cluster, unless that is this one.
            auto other=memory.load16(front,at(portal,offsetof(ClusterPortal,front_cluster)));
            if(std::uint16_t(other)==std::uint16_t(here))
                other=memory.load16(back,at(portal,offsetof(ClusterPortal,back_cluster)));
            const auto now=memory.load32(stamp_check,globals::visit_stamp);
            const auto other_slot=element(globals::cluster_stamps,widen16(other),4);
            if(memory.load32(other_stamp,other_slot)!=now) {
                const auto test=aim_test(memory,portal,spread,local);
                frame.tested(test);
                if(test.result==Test::inside) {
                    memory.store32(other_mark,other_slot,memory.load32(stamp_mark,globals::visit_stamp));
                    memory.store16(stack_push,element(local(Local::stack),widen16(depth),2),other);
                    ++depth;
                }
            }
            // The position is stored as 32 bits and compared as 16.
            position=memory.load32(portal_read,local(Local::portal))+1u;
            memory.store32(portal_store,local(Local::portal),position);
            position=widen16(position);
        } while(signed_value(position)<signed_value(memory.load32(portal_bound,local(Local::portal_count))));
        reached=memory.load32(reached_reload,local(Local::reached));
    } while(std::int16_t(depth)>0);

    // Report the count (16-bit) where the caller asked, and the result is
    // the count clamped to the capacity.
    const auto count_output=memory.load32(count_output_argument,local(Local::count_output));
    memory.store8(active_clear,globals::flood_active,0);
    if(count_output)memory.store16(count_store,count_output,reached);
    return Result{reached,memory.load32(capacity_result,local(Local::capacity))};
}
} // namespace aim_clusters
} // namespace halo2::native_aim_clusters
