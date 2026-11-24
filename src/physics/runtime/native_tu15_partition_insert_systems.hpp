#pragma once
// Native TU1.5 cluster partition insert, entered at 1D2F10 (physics live
// support unit): an object enters the clusters of a cluster partition.
//
//   partition_insert::insert   1D2F10(EAX = cluster set; partition, object,
//                              head*, center*, radius, location*,
//                              payload size, payload*, overflowed*)
//
// The clusters are gathered into a 256-entry list on the stack: those of a
// cluster set (14D080), when one is given; else, when the location names a
// cluster (+4, NONE for none), those a sphere around the center touches
// (14D0E0, counted at 4F3CE4 and flagged at 4F3CE1 while it runs) when the
// radius is above 46A0E8 (COMISS), or only the location's cluster. The
// overflowed byte says whether the gathering found more than it returned
// (a signed 16-bit compare). Then, for each cluster in the list: a reference
// is allocated in the partition's reference pool (+8; 16E6A0) and pushed on
// the object's reference list (*head), naming the cluster; an entry is
// allocated in the entry pool (+4) and pushed on the cluster's chain (heads
// at +0), naming the object, with the payload copied behind it (+0xC;
// REP MOVSD, then REP MOVSB for the rest). A failed allocation skips its
// step.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_partition_insert.hpp) maps the role to the original
// instruction and owns the ABI of the children (14D080, 14D0E0, 16E6A0). The
// values the original keeps in callee-saved registers across the
// allocations are a Kept, refreshed from the registers 16E6A0 returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_cluster_objects_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_partition_removal_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_partition_insert {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::widen16;
using native_cluster_objects::ClusterPartition;
using native_cluster_objects::ClusterEntry;
using native_partition_removal::ClusterReference;
namespace sse=native_aim_clusters::sse;

namespace globals {
inline constexpr std::uint32_t sphere_queries=0x4f3ce4u;   // counts the sphere queries
inline constexpr std::uint32_t querying=0x4f3ce1u;         // byte: a sphere query runs
inline constexpr std::uint32_t sphere_radius=0x46a0e8u;    // float: the smallest radius queried
}
inline constexpr std::uint32_t list_capacity=0x100u;        // cluster indices (int16) on the stack
inline constexpr std::uint32_t location_cluster=4u;         // int16 in the location
inline constexpr std::uint32_t entry_payload=0xcu;          // the payload behind a ClusterEntry
inline constexpr std::uint32_t no_cluster=0xffffu;

namespace partition_insert {
// The arguments, and the locals of the 0x204-byte frame: the list's
// remaining count and the list.
enum class Local {partition,object,head,center,radius,location,payload_size,payload,overflowed,remaining,list};
enum class Access {
    partition_argument,set_total,location_argument,location_word,radius_argument,radius_floor,query_count,
    radius_again,query_count_store,center_argument,querying_set,querying_clear,single_cluster,overflowed_argument,
    overflowed_store,remaining_init,references_pool,cluster_read,references_data,references_size,reference_cluster,
    head_argument,list_head,reference_next,list_store,heads,entries_pool,entries_size,entries_data,object_argument,
    entry_owner,slot_head,entry_next,slot_store,payload_argument,size_argument,payload_word,payload_word_store,
    payload_byte,payload_byte_store,remaining_read,remaining_store
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t partition;   // EBP: the ClusterPartition
    std::uint32_t cursor;      // EBX: the next cluster in the list
    std::uint32_t cluster;     // ESI: the cluster (low 16 bits); then the chain's head slot
};
// What the gathering returned (EAX) and found (ECX).
struct Gathered {std::uint32_t count,total;};
enum class Exit {none_gathered,inserted};

// Frame:
//   local(Local)               an argument's or local's address now
//   gather_set(kept)           14D080(EBX the set, EDI the remaining
//                              local; 256, the list); returns EAX,
//                              refreshes kept
//   query(kept)                14D0E0(cluster, center, radius, 256, the
//                              list) after the pushes; returns EAX,
//                              refreshes kept
//   clamped(Gathered)          the count clamped to the list (EAX, ECX)
//   allocate_reference(kept)   16E6A0 on the reference pool (EDX); returns
//                              EAX, refreshes kept
//   allocate_entry(kept)       16E6A0 on the entry pool (EDX), ESI the
//                              chain's head slot; returns EAX, refreshes kept
//   pool()                     the pool (EDX) 16E6A0 returned
//   copied_words()             between REP MOVSD and REP MOVSB
template<class Memory,class Frame> Exit insert(Memory &memory,Frame &frame,std::uint32_t cluster_set) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{};
    kept.partition=memory.load32(partition_argument,local(Local::partition));
    Gathered gathered{};
    if(cluster_set!=0) {
        gathered.count=frame.gather_set(kept);
        gathered.total=memory.load32(set_total,local(Local::remaining));
    } else {
        const auto location=memory.load32(location_argument,local(Local::location));
        const auto cluster=memory.load16(location_word,at(location,location_cluster));
        gathered.count=0;
        if(cluster!=no_cluster) {
            const auto radius=memory.load32(radius_argument,local(Local::radius));
            if(!sse::not_above(radius,memory.load32(radius_floor,globals::sphere_radius))) {
                const auto queries=memory.load32(query_count,globals::sphere_queries);
                (void)memory.load32(radius_again,local(Local::radius));
                memory.store32(query_count_store,globals::sphere_queries,queries+1u);
                (void)memory.load32(center_argument,local(Local::center));
                memory.store8(querying_set,globals::querying,1u);
                gathered.count=frame.query(kept);
                memory.store8(querying_clear,globals::querying,0u);
            } else {
                memory.store16(single_cluster,local(Local::list),cluster);
                gathered.count=1u;
            }
        }
        gathered.total=gathered.count;
        if(std::int16_t(gathered.count)>std::int16_t(list_capacity))gathered.count=list_capacity;
        frame.clamped(gathered);
    }
    const auto overflowed=memory.load32(overflowed_argument,local(Local::overflowed));
    memory.store8(overflowed_store,overflowed,std::int16_t(gathered.total)>std::int16_t(gathered.count)?1u:0u);
    if(std::int16_t(gathered.count)<=0)return Exit::none_gathered;
    kept.cursor=local(Local::list);
    memory.store32(remaining_init,local(Local::remaining),gathered.count&0xffffu);
    for(;;) {
        // The reference: on the object's list, naming the cluster.
        (void)memory.load32(references_pool,at(kept.partition,offsetof(ClusterPartition,references)));
        (void)memory.load16(cluster_read,kept.cursor);
        const auto reference=frame.allocate_reference(kept);
        const auto cluster=widen16(kept.cluster);
        if(reference!=none) {
            const auto pool=frame.pool();
            const auto data=memory.load32(references_data,at(pool,offsetof(DataArray,data)));
            const auto record=(reference&0xffffu)*memory.load32(references_size,at(pool,offsetof(DataArray,datum_size)))+data;
            memory.store32(reference_cluster,at(record,offsetof(ClusterReference,cluster)),cluster);
            const auto head_slot=memory.load32(head_argument,local(Local::head));
            memory.store32(reference_next,at(record,offsetof(ClusterReference,next)),memory.load32(list_head,head_slot));
            memory.store32(list_store,head_slot,reference);
        }
        // The entry: on the cluster's chain, naming the object, with the payload.
        (void)memory.load32(heads,at(kept.partition,offsetof(ClusterPartition,heads)));
        (void)memory.load32(entries_pool,at(kept.partition,offsetof(ClusterPartition,entries)));
        const auto entry_handle=frame.allocate_entry(kept);
        if(entry_handle!=none) {
            const auto pool=frame.pool();
            const auto offset=(entry_handle&0xffffu)*memory.load32(entries_size,at(pool,offsetof(DataArray,datum_size)));
            const auto entry=offset+memory.load32(entries_data,at(pool,offsetof(DataArray,data)));
            memory.store32(entry_owner,at(entry,offsetof(ClusterEntry,owner)),memory.load32(object_argument,local(Local::object)));
            const auto slot=kept.cluster;
            memory.store32(entry_next,at(entry,offsetof(ClusterEntry,next)),memory.load32(slot_head,slot));
            memory.store32(slot_store,slot,entry_handle);
            const auto source=memory.load32(payload_argument,local(Local::payload));
            if(source!=0) {
                const auto size=memory.load32(size_argument,local(Local::payload_size));
                const auto target=at(entry,entry_payload);
                std::uint32_t copied=0;
                for(;copied<(size&~3u);copied+=4u)
                    memory.store32(payload_word_store,target+copied,memory.load32(payload_word,source+copied));
                frame.copied_words();
                for(;copied<size;++copied)
                    memory.store8(payload_byte_store,target+copied,memory.load8(payload_byte,source+copied));
            }
        }
        const auto remaining=memory.load32(remaining_read,local(Local::remaining))-1u;
        kept.cursor+=2u;
        memory.store32(remaining_store,local(Local::remaining),remaining);
        if(remaining==0)break;
    }
    return Exit::inserted;
}
} // namespace partition_insert
} // namespace halo2::native_partition_insert
