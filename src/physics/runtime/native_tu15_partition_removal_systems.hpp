#pragma once
// Native TU1.5 cluster partition removal, entered at 1D30F0 (physics live
// support unit): an object leaves a cluster partition.
//
//   partition_removal::remove   1D30F0(EDI = partition; object, head*)
//
// The object's references form a list in the partition's reference pool
// (+8), starting at *head: each reference names a cluster (+4, 16-bit) and
// the next reference (+8). For each reference, in list order: it is freed
// (16E8A0 on the reference pool), then the cluster's entry chain (heads at
// +0, entries in the pool at +4; an entry names its owner at +4 and the next
// entry at +8) is searched for the entry this object owns; that entry is
// freed and unlinked from the chain. Finally *head becomes NONE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_partition_removal.hpp) maps the role to the original
// instruction and owns the ABI of the child 16E8A0 (the data array free).
// The original keeps the next reference in the object argument's slot, and
// reads the head pointer argument again at the end; the values it keeps in
// callee-saved registers across the frees are a Kept, refreshed from the
// registers 16E8A0 returned.
#include "physics/runtime/native_tu15_cluster_objects_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_partition_removal {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::widen16;
using native_cluster_objects::ClusterPartition;
using native_cluster_objects::ClusterEntry;
// One of an object's references in the partition's reference pool.
struct ClusterReference {
    std::uint32_t unused_here;
    std::uint16_t cluster;      // +4
    std::uint16_t unused_here_6;
    std::uint32_t next;         // +8: the next reference handle, NONE at the end
};
static_assert(offsetof(ClusterReference,cluster)==4 && offsetof(ClusterReference,next)==8);

namespace partition_removal {
enum class Local {object,head};
enum class Access {
    head_argument,head,object_argument,references_pool,references_data,references_size,reference_cluster,
    reference_next,next_store,heads,link_check,entries_pool,entries_size,entries_data,link_read,entry_owner,
    link_next_check,entry_next,link_handle,unlink,next_reload,head_reload,head_clear
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t object;      // EBP: the object handle
    std::uint32_t partition;   // EDI: the ClusterPartition
    std::uint32_t link;        // ESI: the cluster (low 16 bits) after a reference is freed; the chain link slot
    std::uint32_t after;       // EBX: the entry after the one removed
};
enum class Exit {no_references,removed};

// Frame:
//   local(Local)                  an argument's address now
//   partition()                   EDI at entry
//   release_reference(kept)       16E8A0 for the reference (EAX) in the
//                                 reference pool (EDX)
//   release_entry(kept)           16E8A0 for the entry (EAX) in the entry
//                                 pool (EDX)
template<class Memory,class Frame> Exit remove(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    auto head_slot=memory.load32(head_argument,local(Local::head));
    auto reference=memory.load32(head,head_slot);
    Kept kept{};
    kept.object=memory.load32(object_argument,local(Local::object));
    kept.partition=frame.partition();
    if(reference==none) {
        memory.store32(head_clear,head_slot,none);
        return Exit::no_references;
    }
    for(;;) {
        // The reference: its cluster and the next one; then it is freed.
        const auto references=memory.load32(references_pool,at(kept.partition,offsetof(ClusterPartition,references)));
        const auto data=memory.load32(references_data,at(references,offsetof(DataArray,data)));
        const auto record=(reference&0xffffu)*memory.load32(references_size,at(references,offsetof(DataArray,datum_size)))+data;
        (void)memory.load16(reference_cluster,at(record,offsetof(ClusterReference,cluster)));
        memory.store32(next_store,local(Local::object),memory.load32(reference_next,at(record,offsetof(ClusterReference,next))));
        frame.release_reference(kept);
        // The cluster's entry chain: the entry this object owns.
        const auto heads_table=memory.load32(heads,at(kept.partition,offsetof(ClusterPartition,heads)));
        auto link=element(heads_table,widen16(kept.link&0xffffu),4);
        const bool chained=memory.load32(link_check,link)!=none;
        const auto entries=memory.load32(entries_pool,at(kept.partition,offsetof(ClusterPartition,entries)));
        if(chained) {
            const auto size=memory.load32(entries_size,at(entries,offsetof(DataArray,datum_size)));
            const auto entries_at=memory.load32(entries_data,at(entries,offsetof(DataArray,data)));
            for(;;) {
                const auto entry=(memory.load32(link_read,link)&0xffffu)*size+entries_at;
                if(memory.load32(entry_owner,at(entry,offsetof(ClusterEntry,owner)))==kept.object) {
                    (void)memory.load32(entry_next,at(entry,offsetof(ClusterEntry,next)));
                    (void)memory.load32(link_handle,link);
                    frame.release_entry(kept);
                    memory.store32(unlink,kept.link,kept.after);
                    break;
                }
                link=at(entry,offsetof(ClusterEntry,next));
                if(memory.load32(link_next_check,link)==none)break;
            }
        }
        reference=memory.load32(next_reload,local(Local::object));
        if(reference==none)break;
    }
    head_slot=memory.load32(head_reload,local(Local::head));
    memory.store32(head_clear,head_slot,none);
    return Exit::removed;
}
} // namespace partition_removal
} // namespace halo2::native_partition_removal
