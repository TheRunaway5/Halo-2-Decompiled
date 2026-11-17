#pragma once
// Native TU1.5 cluster distance flood (131470, the campaign environment
// unit; docs/native_replacement/B8F40_object_tick.md):
//
//   cluster_distance::flood  131470  every cluster reachable from a start
//                                    cluster through the structure BSP's
//                                    portals, each with its distance from a
//                                    point, nearest-first in discovery order
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_cluster_distance_flood.hpp) maps the role to the original
// instruction. The flood's working set (the visited set, each cluster's entry
// index, the queue position and count, the current and next entries) lives
// in the original's 0x454-byte stack frame, so it is reached through the
// Frame (as addresses: the logic reads and writes them as memory); so are
// the arguments and the 14DD70 portal distance child.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses, including the frame's, happen in the original order
//     and width;
//   - distances are compared with COMISS on the live MXCSR (a NaN counts as
//     "not greater", and sets the sticky invalid flag, as in the original),
//     and are moved as raw bits, so NaN payloads are kept;
//   - cluster and portal indices are 16-bit and sign-extended; the visited
//     set's word of an index is the index shifted arithmetically, as the
//     original's SAR; address arithmetic wraps at 32 bits;
//   - the values the original keeps in callee-saved registers across the
//     14DD70 call are named in PortalWalk; the adapter hands back what the
//     callee returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_cluster_distance {
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

// COMISS a,b; JBE: a is not greater than b, or the two are unordered. Runs
// the original instruction on the live MXCSR (sticky flags, DAZ).
namespace sse {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline bool not_greater(std::uint32_t a,std::uint32_t b) {
    const auto x=_mm_castsi128_ps(_mm_cvtsi32_si128(int(a))),y=_mm_castsi128_ps(_mm_cvtsi32_si128(int(b)));
    unsigned char result;asm volatile("comiss %2,%1\n\tsetbe %0":"=qm"(result):"x"(x),"x"(y):"cc","memory");return result;
}
#else
inline constexpr bool available=false;
inline bool not_greater(std::uint32_t,std::uint32_t){return true;}
#endif
} // namespace sse

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;   // the loaded structure BSP
inline constexpr std::uint32_t distance_limit=0x46a110u;  // float: clusters this far or farther are not reached
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
// A cluster portal joins two clusters. Its geometry (read by 14DD70) follows.
struct ClusterPortal {
    std::int16_t front_cluster;
    std::int16_t back_cluster;
    std::uint8_t geometry[32];
};
static_assert(sizeof(ClusterPortal)==36);
// A cluster: the indices (16-bit) of its portals.
struct Cluster {
    std::uint8_t unused_here_00[0x8c];
    TagBlock portals;
    std::uint8_t unused_here_94[0x1c];
};
static_assert(sizeof(Cluster)==0xb0 && offsetof(Cluster,portals.count)==0x8c && offsetof(Cluster,portals.elements)==0x90);
// One entry of the output: a reached cluster and its distance. The flood
// writes the cluster, the distance and a NONE link; it leaves the rest.
struct ClusterDistance {
    std::int32_t cluster;
    std::uint32_t distance;     // float bits
    std::uint8_t unused_here_08[12];
    std::int32_t link;
};
static_assert(sizeof(ClusterDistance)==0x18 && offsetof(ClusterDistance,link)==0x14);

// The visited set: one bit per cluster index, in 32-bit words.
constexpr std::uint32_t visited_word(std::uint32_t cluster){return std::uint32_t(signed_value(cluster)>>5);}
constexpr std::uint32_t visited_bit(std::uint32_t cluster){return 1u<<(cluster&31u);}

// ------------------------------------------------------------------- flood
// 131470. A breadth-first walk of the cluster graph from the start cluster.
// The output doubles as the queue: entry 0 is the start cluster at distance
// zero, and each cluster first reached through a portal is appended with
// the portal's distance from the point (14DD70), unless that distance is not
// below the limit. A cluster reached again keeps the smaller distance. The
// queue's length (the count of clusters reached) is left in the frame; the
// adapter returns it.
// Frame:
//   local(Local)                       the address of one of the flood's
//                                      locals or arguments in the original's
//                                      stack frame (it moves with the stack
//                                      pointer); they are read and written as
//                                      memory, with roles, in the original's
//                                      order
//   clear_visited()                    the visited set starts empty
//   portal_distance(portal, point, walk)  14DD70: the portal's distance
//                                      from the point (float bits)
namespace cluster_distance {
enum class Local {
    count,          // the queue's length
    position,       // the queue's read position
    portal,         // the position in the expanded cluster's portals
    current_entry,  // the entry being expanded
    next_entry,     // the entry after it
    visited,        // 16 words: one bit per cluster index
    entry_indices,  // 512 words: the queue entry of each reached cluster
    point,          // argument: the point
    output          // argument: the output entries
};
enum class Access {
    limit,output_argument,bsp,position_clear,start_index,start_visited,start_cluster,start_visited_mark,
    start_distance,start_link,count_start,
    position_read,clusters,position_store,entry_cluster,portal_count,current_store,next_store,portal_clear,
    count_for_tail,current_reload,portal_indices,portal_index,portals,front,entry_cluster_again,back,point_argument,
    visited,entry_index,output_for_update,visited_distance,kept_distance,visited_distance_store,
    count_for_append,entry_index_store,count_store,visited_again,append_cluster,append_distance,append_link,visited_mark,
    portal_read,portal_count_again,portal_store,next_reload,output_reload,position_check,count_check
};
// Kept across the 14DD70 call: the BSP, the cluster being expanded, the far
// cluster of the portal, and the link field of the next output entry.
struct PortalWalk {
    std::uint32_t bsp;
    std::uint32_t cluster;
    std::uint32_t other;
    std::uint32_t tail;
};
template<class Memory,class Frame> void flood(Memory &memory,Frame &frame,std::uint32_t start) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto limit_bits=memory.load32(limit,globals::distance_limit);
    auto output=memory.load32(output_argument,local(Local::output));
    PortalWalk walk{memory.load32(bsp,globals::structure_bsp),0,start,0};

    // The start cluster is entry 0, at distance zero, and visited.
    frame.clear_visited();
    memory.store32(position_clear,local(Local::position),0);
    memory.store16(start_index,element(local(Local::entry_indices),start,2),0);
    const auto start_word=element(local(Local::visited),visited_word(start),4);
    const auto start_bits=memory.load32(start_visited,start_word);
    memory.store32(start_cluster,at(output,offsetof(ClusterDistance,cluster)),start);
    memory.store32(start_visited_mark,start_word,start_bits|visited_bit(start));
    memory.store32(start_distance,at(output,offsetof(ClusterDistance,distance)),0);  // +0.0
    memory.store32(start_link,at(output,offsetof(ClusterDistance,link)),none);
    memory.store32(count_start,local(Local::count),1);

    auto entry=output;
    for(;;) {
        const auto position=memory.load32(position_read,local(Local::position));
        const auto cluster_table=memory.load32(clusters,at(walk.bsp,offsetof(StructureBsp,clusters.elements)));
        memory.store32(position_store,local(Local::position),position+1u);
        walk.cluster=element(cluster_table,memory.load32(entry_cluster,at(entry,offsetof(ClusterDistance,cluster))),sizeof(Cluster));
        const auto portals_here=memory.load32(portal_count,at(walk.cluster,offsetof(Cluster,portals.count)));
        memory.store32(current_store,local(Local::current_entry),entry);
        memory.store32(next_store,local(Local::next_entry),entry+std::uint32_t(sizeof(ClusterDistance)));
        memory.store32(portal_clear,local(Local::portal),0);
        if(signed_value(portals_here)>0) {
            // Appends go after the last entry.
            walk.tail=at(element(output,memory.load32(count_for_tail,local(Local::count)),sizeof(ClusterDistance)),
                         offsetof(ClusterDistance,link));
            auto current=entry;
            for(std::uint32_t portal=0;;) {
                const auto indices=memory.load32(portal_indices,at(walk.cluster,offsetof(Cluster,portals.elements)));
                const auto number=widen16(memory.load16(portal_index,element(indices,portal,2)));
                const auto record=element(memory.load32(portals,at(walk.bsp,offsetof(StructureBsp,cluster_portals.elements))),
                                          number,sizeof(ClusterPortal));
                // The far side: the front cluster, unless that is the one
                // being expanded.
                walk.other=widen16(memory.load16(front,at(record,offsetof(ClusterPortal,front_cluster))));
                if(walk.other==memory.load32(entry_cluster_again,at(current,offsetof(ClusterDistance,cluster))))
                    walk.other=widen16(memory.load16(back,at(record,offsetof(ClusterPortal,back_cluster))));
                if(walk.other!=none) {
                    const auto point=memory.load32(point_argument,local(Local::point));
                    const auto distance=frame.portal_distance(record,point,walk);
                    const auto word=element(local(Local::visited),visited_word(walk.other),4);
                    if(memory.load32(visited,word)&visited_bit(walk.other)) {
                        // Reached before: keep the smaller distance. The new
                        // one wins when it is not greater, or unordered.
                        const auto index=widen16(memory.load16(entry_index,element(local(Local::entry_indices),walk.other,2)));
                        const auto reached=at(element(memory.load32(output_for_update,local(Local::output)),index,
                                                      sizeof(ClusterDistance)),offsetof(ClusterDistance,distance));
                        auto kept=distance;
                        if(!sse::not_greater(distance,memory.load32(visited_distance,reached)))
                            kept=memory.load32(kept_distance,reached);
                        memory.store32(visited_distance_store,reached,kept);
                    } else if(!sse::not_greater(limit_bits,distance)) {
                        // New and nearer than the limit: append it.
                        const auto count=memory.load32(count_for_append,local(Local::count));
                        memory.store16(entry_index_store,element(local(Local::entry_indices),walk.other,2),count);
                        memory.store32(count_store,local(Local::count),count+1u);
                        const auto bits=memory.load32(visited_again,word);
                        memory.store32(append_cluster,walk.tail-0x14u,walk.other);
                        memory.store32(append_distance,walk.tail-0x10u,distance);
                        memory.store32(append_link,walk.tail,none);
                        walk.tail+=std::uint32_t(sizeof(ClusterDistance));
                        memory.store32(visited_mark,word,bits|visited_bit(walk.other));
                    }
                }
                portal=memory.load32(portal_read,local(Local::portal))+1u;
                const auto bound=memory.load32(portal_count_again,at(walk.cluster,offsetof(Cluster,portals.count)));
                memory.store32(portal_store,local(Local::portal),portal);
                if(!(signed_value(portal)<signed_value(bound)))break;
                current=memory.load32(current_reload,local(Local::current_entry));
            }
            entry=memory.load32(next_reload,local(Local::next_entry));
            output=memory.load32(output_reload,local(Local::output));
        } else entry+=std::uint32_t(sizeof(ClusterDistance));
        const auto read=memory.load32(position_check,local(Local::position));
        const auto length=memory.load32(count_check,local(Local::count));
        if(!(signed_value(read)<signed_value(length)))break;
    }
}
} // namespace cluster_distance
} // namespace halo2::native_cluster_distance
