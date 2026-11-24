#pragma once
// Native TU1.5 cluster object query, entered at BD460 (combat action unit).
// The weapon targeting search (1AE090) calls it with the clusters the aim
// cluster flood (14D200) reached:
//
//   cluster_objects::query   BD460   every object the given clusters
//                                    reference, each once, up to a capacity
//
// A structure cluster references objects through two cluster partitions,
// one for collideable objects and one for the others. Each partition has a
// head per cluster and a pool of entries; a cluster's entries form a list
// (owner, next). The query walks, for each cluster in the list order, the
// collideable list (flag 1), then the non-collideable list (flag 2); zero
// flags mean both, and the argument is rewritten to all flags. Every owner
// is appended to the output unless it is already marked with this query's
// stamp: a global counter (4EABCC) is incremented on entry and written to
// the owner's index in a global mark table (4EABD0). When an unmarked owner
// arrives and the output is full, it is still marked and the query stops.
// A global byte (4EABC8) is set while the query runs. The count found is the
// result.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_cluster_objects.hpp) maps the role to the original
// instruction. The arguments and the locals (the list position, the
// collideable flag, the current cluster) live in the original's frame and
// are read and written as memory through Frame::local, in the original's
// order.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width; the original
//     keeps the collideable pool in a register between loads (it reloads it
//     after each append), and re-reads the non-collideable pool at every
//     step; the pool's datum size and data are read again for every entry;
//   - an entry is the pool's data plus (handle & 0xFFFF) times the datum
//     size, with no validity check, in 32-bit arithmetic;
//   - counts, positions and capacities compare as signed 16-bit values;
//     cluster indices are sign-extended.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_cluster_objects {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::widen16;

// ------------------------------------------------------------- game layouts
// A cluster partition (12 bytes): which objects each cluster references.
struct ClusterPartition {
    std::uint32_t heads;        // uint32[cluster]: the first entry handle, NONE for none
    std::uint32_t entries;      // DataArray* of ClusterEntry
    std::uint32_t references;   // the owner-to-cluster pool (not read here)
};
static_assert(sizeof(ClusterPartition)==12 && offsetof(ClusterPartition,entries)==4);
// One link of a cluster's list.
struct ClusterEntry {
    std::uint32_t unused_here;
    std::uint32_t owner;        // +4: the object handle
    std::uint32_t next;         // +8: the next entry handle, NONE at the end
};
static_assert(offsetof(ClusterEntry,owner)==4 && offsetof(ClusterEntry,next)==8);
namespace globals {
inline constexpr std::uint32_t collideable=0x4eabb0u;      // ClusterPartition of collideable objects
inline constexpr std::uint32_t noncollideable=0x4eaba4u;   // ClusterPartition of the other objects
inline constexpr std::uint32_t query_active=0x4eabc8u;     // byte: 1 while a query runs
inline constexpr std::uint32_t query_stamp=0x4eabccu;      // the current query's stamp
inline constexpr std::uint32_t query_marks=0x4eabd0u;      // one stamp per object index
}
namespace partition_flags {
inline constexpr std::uint32_t collideable=1u;
inline constexpr std::uint32_t noncollideable=2u;
inline constexpr std::uint32_t all=0xffffffffu;            // what zero flags become
}

namespace cluster_objects {
enum class Local {
    position,            // the position in the cluster list
    collideable_wanted,  // flags & collideable
    cluster,             // the current cluster index
    flags,               // argument
    cluster_count,       // argument: 16-bit
    clusters,            // argument: the cluster list (16-bit indices)
    capacity,            // argument: 16-bit, the output's room
    output               // argument: the output (object handles)
};
enum class Access {
    flags_argument,flags_all,stamp_read,stamp_store,count_check,active_set,position_clear,
    flags_read,collideable_pool,collideable_flag_store,collideable_flag_read,cluster_list,cluster_index,cluster_store,
    // the collideable list: head, first entry, marks and appends, following entries
    collideable_heads,collideable_head,first_size,first_data,first_next,first_owner,
    stamp_now,mark,capacity,mark_store,output_argument,output_store,pool_after_append,
    next_size,next_data,next_next,next_owner,
    // the non-collideable list
    noncollideable_wanted,cluster_reload,noncollideable_heads,noncollideable_head,
    other_first_pool,other_first_size,other_first_data,other_first_next,other_first_owner,
    other_stamp_now,other_mark,other_capacity,other_mark_store,other_output_argument,other_output_store,
    other_pool_after_append,
    other_next_pool,other_next_size,other_next_data,other_next_next,other_next_owner,
    // the cluster loop and the exit
    position_read,count_bound,position_store,active_clear
};
// The access roles of one partition's walk.
struct EntryRoles {Access size,data,next,owner;};
struct WalkRoles {
    Access heads,head;
    EntryRoles first,following;
    Access stamp_now,mark,capacity,mark_store,output_argument,output_store,pool_after_append;
};
inline constexpr WalkRoles collideable_roles{
    Access::collideable_heads,Access::collideable_head,
    {Access::first_size,Access::first_data,Access::first_next,Access::first_owner},
    {Access::next_size,Access::next_data,Access::next_next,Access::next_owner},
    Access::stamp_now,Access::mark,Access::capacity,Access::mark_store,Access::output_argument,Access::output_store,
    Access::pool_after_append};
inline constexpr WalkRoles noncollideable_roles{
    Access::noncollideable_heads,Access::noncollideable_head,
    {Access::other_first_size,Access::other_first_data,Access::other_first_next,Access::other_first_owner},
    {Access::other_next_size,Access::other_next_data,Access::other_next_next,Access::other_next_owner},
    Access::other_stamp_now,Access::other_mark,Access::other_capacity,Access::other_mark_store,
    Access::other_output_argument,Access::other_output_store,Access::other_pool_after_append};

// One step of a list: the entry a handle names, its next handle and owner.
struct Link {std::uint32_t next,owner;};
template<class Memory> Link entry(Memory &memory,std::uint32_t pool,std::uint32_t handle,const EntryRoles &roles) {
    const auto size=memory.load32(roles.size,at(pool,offsetof(DataArray,datum_size)));
    const auto data=memory.load32(roles.data,at(pool,offsetof(DataArray,data)));
    const auto record=(handle&0xffffu)*size+data;
    const auto next=memory.load32(roles.next,at(record,offsetof(ClusterEntry,next)));
    return {next,memory.load32(roles.owner,at(record,offsetof(ClusterEntry,owner)))};
}

// The query state the walks share.
struct Query {
    std::uint32_t found;             // objects appended (16-bit significant)
    std::uint32_t collideable_pool;  // held between loads, as the original's EAX
};

// Walks one cluster's list in a partition. Returns false when an unmarked
// owner arrived with the output full (the query stops).
// pool(): the partition's entry pool for the next step (the collideable
// walk uses the held pool, the non-collideable walk reads it again).
template<class Memory,class Frame,class Pool> bool walk(Memory &memory,Frame &frame,Query &q,std::uint32_t partition,
                                                        std::uint32_t cluster,const WalkRoles &roles,Pool pool) {
    const auto local=[&](Local l){return frame.local(l);};
    const auto heads=memory.load32(roles.heads,at(partition,offsetof(ClusterPartition,heads)));
    const auto first=memory.load32(roles.head,element(heads,cluster,4));
    Link link{none,none};
    if(first!=none)link=entry(memory,pool(true),first,roles.first);
    while(link.owner!=none) {
        const auto now=memory.load32(roles.stamp_now,globals::query_stamp);
        const auto slot=element(globals::query_marks,link.owner&0xffffu,4);
        if(memory.load32(roles.mark,slot)!=now) {
            const bool full=!(std::int16_t(q.found)<std::int16_t(memory.load16(roles.capacity,local(Local::capacity))));
            memory.store32(roles.mark_store,slot,now);
            if(full)return false;
            const auto output=memory.load32(roles.output_argument,local(Local::output));
            memory.store32(roles.output_store,element(output,widen16(q.found),4),link.owner);
            q.collideable_pool=memory.load32(roles.pool_after_append,
                                             at(globals::collideable,offsetof(ClusterPartition,entries)));
            ++q.found;
        }
        if(link.next==none)break;
        link=entry(memory,pool(false),link.next,roles.following);
    }
    return true;
}

// Frame:
//   local(Local)  the address of a local or argument in the original's frame
// Returns the count found (the original's AX).
template<class Memory,class Frame> std::uint32_t query(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Query q{0,0};
    if(memory.load32(flags_argument,local(Local::flags))==0)
        memory.store32(flags_all,local(Local::flags),partition_flags::all);
    // A new stamp for this query (INC of the counter in place).
    memory.store32(stamp_store,globals::query_stamp,memory.load32(stamp_read,globals::query_stamp)+1u);
    const auto count=memory.load16(count_check,local(Local::cluster_count));
    memory.store8(active_set,globals::query_active,1);
    memory.store32(position_clear,local(Local::position),0);
    if(std::int16_t(count)>0) {
        const auto flags=memory.load32(flags_read,local(Local::flags));
        q.collideable_pool=memory.load32(collideable_pool,at(globals::collideable,offsetof(ClusterPartition,entries)));
        auto collideable_wanted=flags&partition_flags::collideable;
        memory.store32(collideable_flag_store,local(Local::collideable_wanted),collideable_wanted);
        for(std::uint32_t position=0;;) {
            const auto list=memory.load32(cluster_list,local(Local::clusters));
            const auto cluster=widen16(memory.load16(cluster_index,element(list,widen16(position),2)));
            memory.store32(cluster_store,local(Local::cluster),cluster);
            if(collideable_wanted &&
               !walk(memory,frame,q,globals::collideable,cluster,collideable_roles,[&](bool){return q.collideable_pool;}))
                break;
            if(memory.load8(noncollideable_wanted,local(Local::flags))&partition_flags::noncollideable) {
                const auto here=widen16(memory.load16(cluster_reload,local(Local::cluster)));
                const auto pool=[&](bool first) {
                    return memory.load32(first?other_first_pool:other_next_pool,
                                         at(globals::noncollideable,offsetof(ClusterPartition,entries)));
                };
                if(!walk(memory,frame,q,globals::noncollideable,here,noncollideable_roles,pool))break;
            }
            position=memory.load32(position_read,local(Local::position))+1u;
            const auto bound=memory.load16(count_bound,local(Local::cluster_count));
            memory.store32(position_store,local(Local::position),position);
            if(!(std::int16_t(position)<std::int16_t(bound)))break;
            collideable_wanted=memory.load32(collideable_flag_read,local(Local::collideable_wanted));
        }
    }
    memory.store8(active_clear,globals::query_active,0);
    return q.found;
}
} // namespace cluster_objects
} // namespace halo2::native_cluster_objects
