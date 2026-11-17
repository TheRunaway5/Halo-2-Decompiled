#pragma once
// Native TU1.5 cluster contents: 134D30, the step of the portal visibility
// pass that turns the clusters a view can see into render work
// (docs/native_replacement/134D30_cluster_contents.md).
//
//   cluster_contents::collect(view)
//     For each cluster on the view's visible cluster list, in list order:
//       1. whether the cluster lies in the current fog (171070); the objects
//          of this cluster carry that byte into their admission;
//       2. unless the view skips objects, every object the cluster references
//          is offered to the object admission (135B60) with the cluster's
//          list position: first the collideable partition, then the
//          non-collideable one. A view with the "machines only" flag offers
//          only machines (object type 7) from the collideable partition;
//       3. the main view, unless it skips lights, offers every light the
//          cluster references to the light admission (135540);
//       4. the main view, while it has no sky cluster yet, takes the owner
//          cluster of the current sky from the structure BSP.
//     Then:
//       5. the main view, unless it skips lights or has no owner (+8 is -1),
//          asks C5980 for up to four lights. Each one that is outside the
//          cluster partition (+0x48 is -1), active (C5170) and not yet queued
//          this frame is appended to the light queue (the queue append, 166B60,
//          runs inline) and marked queued (C5140);
//       6. unless the view skips objects, the objects of the sky cluster are
//          admitted with no cluster position (-1) as sky objects.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_cluster_contents.hpp) maps the role to the original
// instruction. The argument, the locals the original keeps in its stack frame
// and the child calls come from a Frame object that the adapter also
// supplies. Registers, flags and stack slots belong to the adapter.
//
// What stays exact, because translated neighbours and the children observe it:
//   - memory accesses happen in the original order and width (statements are
//     ordered so that no expression holds two accesses), including every
//     global, pool and frame slot the original reads again after a child call;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a child
//     call are the fields of Kept. A conforming callee returns them unchanged;
//     the adapter hands back what the callee actually returned, so the logic
//     continues exactly as the original does.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_cluster_contents {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::ObjectData;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t object_headers=native_object_update::globals::object_headers; // DataArray* of ObjectHeader
inline constexpr std::uint32_t collideable=0x4eabb0u;      // ClusterPartition of collideable objects
inline constexpr std::uint32_t noncollideable=0x4eaba4u;   // ClusterPartition of the other objects
inline constexpr std::uint32_t lights=0x4ecbe0u;           // ClusterPartition of lights
inline constexpr std::uint32_t light_data=0x4ecbdcu;       // DataArray* of Light
inline constexpr std::uint32_t light_frame=0x4ecbd8u;      // the current light frame stamp
inline constexpr std::uint32_t current_fog=0x4c67acu;      // 171070's fog argument (EDI)
inline constexpr std::uint32_t view_lights=0x4c66f4u;      // C5980's argument; the game meaning is not established
inline constexpr std::uint32_t sky_visible=0x4c6709u;      // byte: the sky is drawn
inline constexpr std::uint32_t sky_index=0x4c670cu;        // the current sky, NONE for none
inline constexpr std::uint32_t sky_selection=0x474c24u;    // word, -1 for none; the game meaning is not established
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;    // the current StructureBsp
}
// A cluster partition (12 bytes): which objects or lights each cluster
// references. Every cluster's references form a list of ClusterEntry datums.
struct ClusterPartition {
    std::uint32_t heads;        // uint32[cluster]: the first entry handle, NONE for none
    std::uint32_t entries;      // DataArray* of ClusterEntry
    std::uint32_t references;   // the owner-to-cluster pool (not read here)
};
static_assert(sizeof(ClusterPartition)==12 && offsetof(ClusterPartition,entries)==4);
// One link of a cluster's list.
struct ClusterEntry {
    std::uint32_t unused_here;
    std::uint32_t owner;        // +4: the object or light handle
    std::uint32_t next;         // +8: the next entry handle, NONE at the end
};
static_assert(offsetof(ClusterEntry,owner)==4 && offsetof(ClusterEntry,next)==8);
// An entry of the view's visible cluster list (26 bytes).
struct VisibleCluster {
    std::int16_t cluster;       // the structure cluster index
    std::uint8_t unused_here[24];
};
static_assert(sizeof(VisibleCluster)==26);
// The view's visibility descriptor: the visible cluster list the portal flood
// built (and the frustum data the admissions read).
struct VisibilityDescriptor {
    std::uint8_t unused_here[0xa6c];
    std::int16_t visible_count;     // +0xA6C
    VisibleCluster visible[1];      // +0xA6E
};
static_assert(offsetof(VisibilityDescriptor,visible_count)==0xa6c && offsetof(VisibilityDescriptor,visible)==0xa6e);
// The view being collected (135B60's context).
struct View {
    std::uint32_t descriptor;       // +0x00: VisibilityDescriptor*
    std::uint32_t unused_here_04;
    std::int16_t owner;             // +0x08: -1 when the view has none; the game meaning
                                    // is not established
    std::uint8_t unused_here_0a[6];
    std::uint32_t light_queue;      // +0x10: RenderQueue*
    std::uint32_t object_queue;     // +0x14: RenderQueue* (135B60's)
    std::uint8_t unused_here_18[0x2a48];
    std::uint32_t flags;            // +0x2A60: view_flags
    std::uint8_t unused_here_2a64[0x14];
    std::uint32_t kind;             // +0x2A78: 0 for the main view; otherwise a light or
                                    // shadow view (135B60 picks the second sphere)
};
static_assert(offsetof(View,owner)==8 && offsetof(View,light_queue)==0x10 && offsetof(View,object_queue)==0x14 &&
              offsetof(View,flags)==0x2a60 && offsetof(View,kind)==0x2a78);
namespace view_flags {
inline constexpr std::uint32_t skip_lights=0x010u;
inline constexpr std::uint32_t skip_objects=0x040u;
inline constexpr std::uint32_t machines_only=0x400u;  // collideable objects: machines only
}
inline constexpr std::uint32_t machine_type=7u;           // ObjectData::type of a machine
// A render queue (166B60): four parallel streams with a shared count.
struct RenderQueue {
    std::int32_t capacity;
    std::uint16_t count;            // +4
    std::uint16_t unused_here;
    std::uint32_t flags;            // +8: uint16[]
    std::uint32_t handles;          // +0xC: uint32[]
    std::uint32_t values;           // +0x10: uint32[]
    std::uint32_t associations;     // +0x14: uint16[]
};
static_assert(offsetof(RenderQueue,count)==4 && offsetof(RenderQueue,flags)==8 && offsetof(RenderQueue,handles)==0xc &&
              offsetof(RenderQueue,values)==0x10 && offsetof(RenderQueue,associations)==0x14);
// A light datum (0x110 bytes) of the [4ECBDC] array: the fields read here.
struct Light {
    std::uint8_t unused_here_00[0xc];
    std::uint32_t queued_frame;     // +0x0C: the light frame it was last queued in (C5140)
    std::uint8_t unused_here_10[0x38];
    std::int16_t cluster_link;      // +0x48: -1 for a light the direct pass may queue. Lights
                                    // with a link are reached through the light partition;
                                    // that reading is not established.
    std::uint8_t unused_here_4a[0xc6];
};
static_assert(sizeof(Light)==0x110 && offsetof(Light,queued_frame)==0xc && offsetof(Light,cluster_link)==0x48);
// The structure BSP's sky owner table: the cluster that owns each sky.
struct StructureBsp {
    std::uint8_t unused_here[0xac];
    std::int32_t sky_owner_count;   // +0xAC
    std::uint32_t sky_owners;       // +0xB0: int16[sky]
};
static_assert(offsetof(StructureBsp,sky_owner_count)==0xac && offsetof(StructureBsp,sky_owners)==0xb0);

// The object admission's arguments (135B60, stdcall, the view in EAX).
struct ObjectAdmission {
    std::uint32_t object;           // the object handle
    std::uint32_t position;         // the visible cluster position (low word), -1 for sky objects
    std::uint32_t inside_fog;       // low byte: the cluster is in the current fog
    std::uint32_t sky;              // low byte: a sky object
};
// The queue append's arguments (166B60, stdcall, the queue in ECX).
struct QueueAppend {
    std::uint32_t handle;
    std::uint32_t flags;            // low word
    std::uint32_t value;
    std::uint32_t association;      // low word
};

// ------------------------------------------------------------ access roles
// One role per original memory access. The walks of a cluster's list repeat
// the same five accesses at ten places; each place is a Walk and its accesses
// are walk_access(walk, field).
enum class Access : unsigned {
    entry_view,entry_descriptor,visible_count,sky_none,first_position,first_descriptor,first_offset,
    loop_descriptor,loop_offset,visible_cluster,current_fog,collideable_pool,
    fog_view,inside_fog,object_skip,collideable_heads,collideable_head,inside_fog_word,
    filter_view,filter_flags,object_headers,object_header_data,object_data,object_type,admit_position,collideable_pool_again,
    other_offset,other_descriptor,other_cluster,other_heads,other_head,other_view,other_position,
    light_view,light_kind,light_skip,light_offset,light_descriptor,light_cluster,light_heads,light_head,
    light_walk_view,light_position,
    sky_known,sky_view,sky_kind,sky_visible,sky_index,sky_selection,structure_bsp,sky_owner_count,sky_owners,sky_owner,sky_store,
    next_offset,next_view,next_descriptor,next_position,offset_store,next_count,position_store,descriptor_store,after_view,
    direct_kind,direct_skip,direct_owner,direct_source,listed_count,light_array,
    listed_light,light_records,light_link,light_frame,light_queued,queue_view,light_queue,
    queue_count,queue_capacity,queue_handles,append_handle,handle_store,queue_count_flags,queue_flags,append_flags,flags_store,
    queue_count_values,queue_values,append_value,value_store,queue_count_associations,queue_associations,append_association,
    association_store,queue_grow,
    listed_count_again,sky_objects_view,
    sky_cluster,sky_skip,sky_collideable_heads,sky_collideable_head,sky_other_heads,sky_other_head,
    walk_base
};
// The ten list walks, by where they run.
enum class Walk : unsigned {
    collideable_first,collideable_next,     // step 2, collideable partition
    other_first,other_next,                 // step 2, non-collideable partition
    light_first,light_next,                 // step 3
    sky_collideable_first,sky_collideable_next,
    sky_other_first,sky_other_next,         // step 6
    count
};
enum class LinkField : unsigned {pool,size,data,next,owner};
constexpr Access walk_access(Walk walk,LinkField field) {
    return Access(unsigned(Access::walk_base)+unsigned(walk)*5u+unsigned(field));
}
inline constexpr unsigned access_count=unsigned(Access::walk_base)+unsigned(Walk::count)*5u;

// The values the original keeps in callee-saved registers across the child
// calls (EBX, ESI, EDI, EBP), by use.
struct Kept {
    std::uint32_t cluster;      // the cluster index, across the fog query
    std::uint32_t pool;         // the collideable entries pool, across the fog query
    std::uint32_t next;         // the next entry of the list being walked
    std::uint32_t inside_fog;   // the fog word passed to each object admission
    std::uint32_t view;         // the view, across admissions and the light passes
    std::uint32_t position;     // the cluster position, across the light admissions
    std::uint32_t entry;        // the light walk's current owner (135540's EBX)
    std::uint32_t listed;       // the listed light's position and handle, and the
    std::uint32_t light;        // light datum, across the direct pass's calls
    std::uint32_t lights;       // the light array, across the direct pass's calls
    std::uint32_t sky_offset;   // the sky cluster's head offset (cluster * 4)
};
// The result of one walk step: the entry's owner (NONE at the end) and the
// next entry handle.
struct Link {std::uint32_t owner,next;};
inline constexpr Link end_of_list{none,none};

// ------------------------------------------------------------ list walking
// One step of a cluster's list in a partition's entries pool: the entry's
// next link, then its owner. The original reads next before owner.
template<class Memory> Link step(Memory &memory,Walk walk,std::uint32_t pool,std::uint32_t entry) {
    const auto size=memory.load32(walk_access(walk,LinkField::size),at(pool,offsetof(DataArray,datum_size)));
    const auto data=memory.load32(walk_access(walk,LinkField::data),at(pool,offsetof(DataArray,data)));
    const auto datum=element(data,handle_index(entry),size);
    const auto next=memory.load32(walk_access(walk,LinkField::next),at(datum,offsetof(ClusterEntry,next)));
    const auto owner=memory.load32(walk_access(walk,LinkField::owner),at(datum,offsetof(ClusterEntry,owner)));
    return {owner,next};
}
// The first step of a walk whose pool is read from its partition first.
template<class Memory> Link first_step(Memory &memory,Walk walk,std::uint32_t partition,std::uint32_t entry) {
    if(entry==none)return end_of_list;
    const auto pool=memory.load32(walk_access(walk,LinkField::pool),at(partition,offsetof(ClusterPartition,entries)));
    return step(memory,walk,pool,entry);
}
// The address of a cluster's list head in a partition.
constexpr std::uint32_t head_of(std::uint32_t heads,std::uint32_t cluster){return element(heads,cluster,4);}

// --------------------------------------------------------- cluster contents
namespace cluster_contents {
// Frame:
//   argument(role)                     the view, read from its slot
//   local(role) / set_local(role, v)   the frame slots: the list position,
//                                      the entry offset, the descriptor, the
//                                      sky cluster word, the fog byte or the
//                                      listed light count, the listed lights
//   listed(role, position)             one slot of the listed light buffer
//   fog_membership(k)                  171070 (cluster, fog, pool kept)
//   admit_object(walk, args, k)        135B60
//   admit_light(k)                     135540 (the light in Kept::entry)
//   list_lights(k, source) -> count    C5980 into the frame buffer
//   light_active(k) -> AL              C5170
//   mark_queued(k)                     C5140
//   append_argument(role)              the queue append's stack arguments
//   begin_append(args) / end_append()  166B60's call and return
// The queue append (166B60, inline): an entry joins the four streams unless
// the queue is full (one slot always stays free). Returns whether it joined.
template<class Memory,class Frame> bool append(Memory &memory,Frame &frame,std::uint32_t queue,const QueueAppend &arguments) {
    using enum Access;
    frame.begin_append(arguments);
    auto count=memory.load16(queue_count,at(queue,offsetof(RenderQueue,count)));
    const auto last=std::uint32_t(memory.load32(queue_capacity,at(queue,offsetof(RenderQueue,capacity))))-1u;
    if(signed_value(count)>=signed_value(last))return frame.end_append(false);
    // Each stream re-reads the count, then its array, then the argument.
    const auto handles=memory.load32(queue_handles,at(queue,offsetof(RenderQueue,handles)));
    const auto handle=frame.append_argument(append_handle);
    memory.store32(handle_store,element(handles,count,4),handle);
    count=memory.load16(queue_count_flags,at(queue,offsetof(RenderQueue,count)));
    const auto flags=memory.load32(queue_flags,at(queue,offsetof(RenderQueue,flags)));
    const auto flag_word=frame.append_argument(append_flags);
    memory.store16(flags_store,element(flags,count,2),flag_word);
    count=memory.load16(queue_count_values,at(queue,offsetof(RenderQueue,count)));
    const auto values=memory.load32(queue_values,at(queue,offsetof(RenderQueue,values)));
    const auto value=frame.append_argument(append_value);
    memory.store32(value_store,element(values,count,4),value);
    count=memory.load16(queue_count_associations,at(queue,offsetof(RenderQueue,count)));
    const auto associations=memory.load32(queue_associations,at(queue,offsetof(RenderQueue,associations)));
    const auto association=frame.append_argument(append_association);
    memory.store16(association_store,element(associations,count,2),association);
    memory.increment16(queue_grow,at(queue,offsetof(RenderQueue,count)));
    return frame.end_append(true);
}

// Step 2: the cluster's objects. The collideable walk keeps its pool in a
// register, read before the fog query and again after each admission; the
// non-collideable walk reads its pool before each step.
template<class Memory,class Frame> void admit_cluster_objects(Memory &memory,Frame &frame,Kept &k) {
    using enum Access;
    const auto heads=memory.load32(collideable_heads,at(globals::collideable,offsetof(ClusterPartition,heads)));
    const auto first=memory.load32(collideable_head,head_of(heads,k.cluster));
    auto link=first==none?end_of_list:step(memory,Walk::collideable_first,k.pool,first);
    k.next=link.next;
    // The fog byte is passed as the whole frame word (its upper bytes are
    // whatever the slot held).
    k.inside_fog=frame.local(inside_fog_word);
    while(link.owner!=none) {
        const auto view=frame.argument(filter_view);
        bool admit=true;
        if(memory.load32(filter_flags,at(view,offsetof(View,flags)))&view_flags::machines_only) {
            const auto array=memory.load32(object_headers,globals::object_headers);
            const auto rows=memory.load32(object_header_data,at(array,offsetof(DataArray,data)));
            const auto object=memory.load32(object_data,at(element(rows,handle_index(link.owner),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
            admit=memory.load8(object_type,at(object,offsetof(ObjectData,type)))==machine_type;
        }
        if(admit) {
            const auto position=frame.local(admit_position);
            k.view=view;
            frame.admit_object(Walk::collideable_next,ObjectAdmission{link.owner,position,k.inside_fog,0},k);
            k.pool=memory.load32(collideable_pool_again,at(globals::collideable,offsetof(ClusterPartition,entries)));
        }
        link=k.next==none?end_of_list:step(memory,Walk::collideable_next,k.pool,k.next);
        k.next=link.next;
    }
    // The non-collideable partition. The cluster index is read again from
    // the visible list.
    const auto offset=frame.local(other_offset);
    const auto descriptor=frame.local(other_descriptor);
    const auto cluster=widen16(memory.load16(other_cluster,at(descriptor,offsetof(VisibilityDescriptor,visible)+offset)));
    const auto other=memory.load32(other_heads,at(globals::noncollideable,offsetof(ClusterPartition,heads)));
    const auto head=memory.load32(other_head,head_of(other,cluster));
    link=first_step(memory,Walk::other_first,globals::noncollideable,head);
    k.next=link.next;
    if(link.owner==none)return;
    k.view=frame.argument(other_view);
    do {
        const auto position=frame.local(other_position);
        frame.admit_object(Walk::other_next,ObjectAdmission{link.owner,position,k.inside_fog,0},k);
        // The pool is read again before the end of the list is tested.
        const auto pool=memory.load32(walk_access(Walk::other_next,LinkField::pool),
                                      at(globals::noncollideable,offsetof(ClusterPartition,entries)));
        link=k.next==none?end_of_list:step(memory,Walk::other_next,pool,k.next);
        k.next=link.next;
    } while(link.owner!=none);
}

// Step 3: the cluster's lights (the main view only).
template<class Memory,class Frame> void admit_cluster_lights(Memory &memory,Frame &frame,Kept &k) {
    using enum Access;
    const auto view=frame.argument(light_view);
    if(memory.load32(light_kind,at(view,offsetof(View,kind)))!=0)return;
    if(memory.load8(light_skip,at(view,offsetof(View,flags)))&view_flags::skip_lights)return;
    const auto offset=frame.local(light_offset);
    const auto descriptor=frame.local(light_descriptor);
    const auto cluster=widen16(memory.load16(light_cluster,at(descriptor,offsetof(VisibilityDescriptor,visible)+offset)));
    const auto heads=memory.load32(light_heads,at(globals::lights,offsetof(ClusterPartition,heads)));
    const auto head=memory.load32(light_head,head_of(heads,cluster));
    auto link=first_step(memory,Walk::light_first,globals::lights,head);
    k.next=link.next;k.entry=link.owner;
    if(k.entry==none)return;
    k.view=frame.argument(light_walk_view);
    k.position=frame.local(light_position);
    do {
        frame.admit_light(k);
        link=first_step(memory,Walk::light_next,globals::lights,k.next);
        k.next=link.next;k.entry=link.owner;
    } while(k.entry!=none);
}

// Step 4: the sky's owner cluster, once per view (the main view only).
template<class Memory,class Frame> void find_sky_cluster(Memory &memory,Frame &frame) {
    using enum Access;
    if(frame.local(sky_known)!=0xffffu)return;
    const auto view=frame.argument(sky_view);
    if(memory.load32(sky_kind,at(view,offsetof(View,kind)))!=0)return;
    if(!memory.load8(Access::sky_visible,globals::sky_visible))return;
    const auto sky=memory.load32(Access::sky_index,globals::sky_index);
    if(sky==none)return;
    if(memory.load16(Access::sky_selection,globals::sky_selection)==0xffffu)return;
    const auto bsp=memory.load32(structure_bsp,globals::structure_bsp);
    if(signed_value(memory.load32(sky_owner_count,at(bsp,offsetof(StructureBsp,sky_owner_count))))<=0)return;
    const auto owners=memory.load32(Access::sky_owners,at(bsp,offsetof(StructureBsp,sky_owners)));
    const auto owner=memory.load16(sky_owner,element(owners,sky,2));
    frame.set_local(sky_store,owner);
}

// Step 5: the lights C5980 lists for the main view. Returns whether any
// was listed (the original then reads the view again).
template<class Memory,class Frame> bool queue_listed_lights(Memory &memory,Frame &frame,Kept &k,std::uint32_t view) {
    using enum Access;
    if(memory.load32(direct_kind,at(view,offsetof(View,kind)))!=0)return false;
    if(memory.load8(direct_skip,at(view,offsetof(View,flags)))&view_flags::skip_lights)return false;
    if(memory.load16(direct_owner,at(view,offsetof(View,owner)))==0xffffu)return false;
    // C5980's buffer and size are pushed before this read.
    const auto source=memory.load32(direct_source,globals::view_lights);
    const auto count=frame.list_lights(k,source);
    frame.set_local(listed_count,count);
    if(signed_value(count)<=0)return false;
    k.lights=memory.load32(light_array,globals::light_data);
    for(k.position=0;;) {
        k.listed=frame.listed(listed_light,k.position);
        const auto records=memory.load32(light_records,at(k.lights,offsetof(DataArray,data)));
        k.light=element(records,handle_index(k.listed),sizeof(Light));
        const bool outside=memory.load16(light_link,at(k.light,offsetof(Light,cluster_link)))==0xffffu;
        if(outside && (frame.light_active(k)&0xffu)) {
            const auto frame_stamp=memory.load32(Access::light_frame,globals::light_frame);
            if(memory.load32(light_queued,at(k.light,offsetof(Light,queued_frame)))!=frame_stamp) {
                const auto queue_owner=frame.argument(queue_view);
                const auto queue=memory.load32(light_queue,at(queue_owner,offsetof(View,light_queue)));
                if(append(memory,frame,queue,QueueAppend{k.listed,0,0,none}))frame.mark_queued(k);
            }
        }
        const auto listed_total=frame.local(listed_count_again);
        if(signed_value(++k.position)>=signed_value(listed_total))return true;
    }
}

// Step 6: the sky cluster's objects, admitted as sky objects.
template<class Memory,class Frame> void admit_sky_objects(Memory &memory,Frame &frame,Kept &k) {
    using enum Access;
    const auto sky=frame.local(sky_cluster);
    if((sky&0xffffu)==0xffffu)return;
    if(memory.load8(sky_skip,at(k.view,offsetof(View,flags)))&view_flags::skip_objects)return;
    const auto heads=memory.load32(sky_collideable_heads,at(globals::collideable,offsetof(ClusterPartition,heads)));
    k.sky_offset=widen16(sky)*4u;
    const auto first=memory.load32(sky_collideable_head,heads+k.sky_offset);
    auto link=first_step(memory,Walk::sky_collideable_first,globals::collideable,first);
    k.next=link.next;
    while(link.owner!=none) {
        frame.admit_object(Walk::sky_collideable_next,ObjectAdmission{link.owner,none,0,1},k);
        link=first_step(memory,Walk::sky_collideable_next,globals::collideable,k.next);
        k.next=link.next;
    }
    const auto other=memory.load32(sky_other_heads,at(globals::noncollideable,offsetof(ClusterPartition,heads)));
    const auto head=memory.load32(sky_other_head,k.sky_offset+other);
    link=first_step(memory,Walk::sky_other_first,globals::noncollideable,head);
    k.next=link.next;
    while(link.owner!=none) {
        frame.admit_object(Walk::sky_other_next,ObjectAdmission{link.owner,none,0,1},k);
        link=first_step(memory,Walk::sky_other_next,globals::noncollideable,k.next);
        k.next=link.next;
    }
}

// 134D30(view): the whole step.
template<class Memory,class Frame> void collect(Memory &memory,Frame &frame) {
    using enum Access;
    Kept k{};
    k.view=frame.argument(entry_view);
    auto descriptor=memory.load32(entry_descriptor,at(k.view,offsetof(View,descriptor)));
    const bool any=signed_value(widen16(memory.load16(visible_count,at(descriptor,offsetof(VisibilityDescriptor,visible_count)))))>0;
    frame.set_local(sky_none,none);
    frame.set_local(first_position,0);
    frame.set_local(first_descriptor,descriptor);
    if(any) {
        frame.set_local(first_offset,0);
        for(bool first=true;;first=false) {
            if(!first)descriptor=frame.local(loop_descriptor);
            const auto offset=frame.local(loop_offset);
            k.cluster=widen16(memory.load16(visible_cluster,at(descriptor,offsetof(VisibilityDescriptor,visible)+offset)));
            const auto fog=memory.load32(current_fog,globals::current_fog);
            k.pool=memory.load32(collideable_pool,at(globals::collideable,offsetof(ClusterPartition,entries)));
            const auto inside=frame.fog_membership(k,fog);
            const auto view=frame.argument(fog_view);
            frame.set_local(inside_fog,inside);
            if(!(memory.load8(object_skip,at(view,offsetof(View,flags)))&view_flags::skip_objects))
                admit_cluster_objects(memory,frame,k);
            admit_cluster_lights(memory,frame,k);
            find_sky_cluster(memory,frame);
            // The next list entry: offset += 26, position += 1, and the
            // descriptor (and its count) are read again from the view.
            const auto next_entry=frame.local(next_offset)+std::uint32_t(sizeof(VisibleCluster));
            const auto view_again=frame.argument(next_view);
            descriptor=memory.load32(next_descriptor,at(view_again,offsetof(View,descriptor)));
            const auto position=frame.local(next_position)+1u;
            frame.set_local(offset_store,next_entry);
            const auto count=widen16(memory.load16(next_count,at(descriptor,offsetof(VisibilityDescriptor,visible_count))));
            frame.set_local(position_store,position);
            frame.set_local(descriptor_store,descriptor);
            if(signed_value(position)>=signed_value(count))break;
        }
        k.view=frame.argument(after_view);
    }
    if(queue_listed_lights(memory,frame,k,k.view))k.view=frame.argument(sky_objects_view);
    admit_sky_objects(memory,frame,k);
    frame.finish();
}
} // namespace cluster_contents
} // namespace halo2::native_cluster_contents
