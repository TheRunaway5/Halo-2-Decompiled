#pragma once
// Native TU1.5 object subtree systems: the per-object children of the object
// update passes (docs/native_replacement/BE850_object_subtree.md):
//
//   object_move::run         BE850  one object's post-physics move, then its
//                                   children's
//   cluster_move::run        BE610  the move of an object's physics cluster and
//                                   its phantoms
//   attachment_refresh::run  C0F60  an object's node lights, placement and
//                                   children, and optionally its siblings
//   type_moves::run          10DD80 the move callbacks of an object's type
//   node_notify::run         C0200  node notification when the model has both
//                                   node tags
//   timers::run              1D5790 an object's two countdown timers
//   countdown::run           1E15D0 one countdown timer
//   node_lights::run         C0910  the lit colours of an object's node rows
//
// This is the game logic only, on the layouts of
// native_tu15_object_tick_systems.hpp and the ones below. The game's memory is
// the retained TU1.5 bank, reached through a Memory object with typed loads
// and stores; every access names its role (each system's Access enum) and the
// boundary adapter (native_tu15_object_subtree.hpp) maps the role to the
// original instruction. Stack arguments, the locals the original keeps in its
// stack frame and the child calls come from a Frame object that the adapter
// also supplies. Registers, flags and stack slots belong to the adapter.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (statements are
//     ordered so that no expression holds two accesses);
//   - address arithmetic is 32-bit and wraps as the original's does,
//     including the original's divisions by multiplication;
//   - single-precision work (C0910) runs the original SSE instructions in the
//     original operand order on the live MXCSR: MULSS keeps its destination
//     operand first (it owns a NaN payload), and COMISS decides the clamps
//     (it sets the sticky invalid flag for any NaN). Colours are handled as
//     their bit patterns, so payloads pass through unchanged;
//   - the values the original keeps in callee-saved registers across a child
//     call are named in each system's "kept" struct. A conforming callee
//     returns them unchanged; the adapter hands back what the callee actually
//     returned, so the logic continues exactly as the original does.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_object_subtree {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen8;
using native_object_update::widen16;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::header_of;
using native_object_update::object_of;
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::ObjectData;
using native_object_update::ObjectTypeDefinition;
using native_object_update::ClusterRecord;
using native_object_update::TimerBlock;
namespace globals=native_object_update::globals;
namespace header_flags=native_object_update::header_flags;
namespace update_flags=native_object_update::update_flags;
namespace object_cluster_flags=native_object_update::object_cluster_flags;

// ------------------------------------------------------- single precision
// The original's scalar SSE on the live MXCSR, on bit patterns.
namespace sse {
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// MULSS: a * b, a the destination.
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b) {
    auto x=scalar(a);asm volatile("mulss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);
}
// COMISS a,b; JA: a > b, ordered.
inline bool above(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
#else
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(scalar(a),scalar(b)));}
inline bool above(std::uint32_t a,std::uint32_t b){return _mm_comigt_ss(scalar(a),scalar(b));}
#endif
} // namespace sse

// ------------------------------------------------------------- game layouts
namespace subtree_globals {
inline constexpr std::uint32_t tag_instances=0x4f0414u;       // TagInstance[]
inline constexpr std::uint32_t node_light_limit=0x46a0d0u;    // float: the brightest a lit channel gets
inline constexpr std::uint32_t phantom_default=0x474d04u;     // B9770's ECX and EDI when no phantom moved;
                                                              // the game meaning is not established
}
// The object header flag BE850 refreshes the attachments for.
inline constexpr std::uint32_t header_attachments=0x40u;
// An object's attachment flags (ObjectData::attachment_flags).
namespace attachment_flags {
inline constexpr std::uint32_t skip=0x01u;          // C0F60 leaves a ready object alone
inline constexpr std::uint32_t node_lights=0x10u;   // the object has node light attachments
inline constexpr std::uint32_t placement=0x40u;     // the object has a placement (C3750)
}
// A cluster record's flags (ClusterRecord::flags). The game meaning of these
// bits is not established; the names say what the move does with them.
namespace cluster_flags {
inline constexpr std::uint32_t one_phantom=0x00000002u;  // no extent (BB200) and one phantom move
inline constexpr std::uint32_t fixed=0x00000100u;        // the phantoms do not move
inline constexpr std::uint32_t node_bits=0x00008000u;    // BE850 builds the node bit vector (1DBBD0)
inline constexpr std::uint32_t pending=0x00010000u;      // cleared when the phantom update changes
}
// A node light attachment (8 bytes) in the object's attachment list.
struct NodeLightAttachment {
    std::uint8_t attached;      // nonzero: nothing to do
    std::uint8_t unused_here[3];
    std::uint32_t light;        // the light's handle, or NONE
};
static_assert(sizeof(NodeLightAttachment)==8 && offsetof(NodeLightAttachment,light)==4);
// A tag instance (16 bytes) of the [4F0414] table.
struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;   // +8
    std::uint32_t unused_here_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);
// An object definition: the fields these systems use.
struct ObjectDefinition {
    std::uint8_t unused_here_00[0x1c];
    std::uint8_t node_light_flags;   // +0x1C: bit 0, the object has node lights
    std::uint8_t unused_here_1d[0x1b];
    std::uint32_t model;             // +0x38: the model tag's handle, or NONE
    std::uint8_t unused_here_3c[0x74];
    std::uint32_t light_block;       // +0xB0: LightBlockRow[], one per node light row
};
static_assert(offsetof(ObjectDefinition,node_light_flags)==0x1c && offsetof(ObjectDefinition,model)==0x38 &&
              offsetof(ObjectDefinition,light_block)==0xb0);
// A model definition: its two node tag references (+0x00 and +0x10, their
// indices at +4 and +0x14).
struct ModelDefinition {
    std::uint32_t first_node_group;
    std::uint32_t first_node_tag;    // +4
    std::uint8_t unused_here_08[8];
    std::uint32_t second_node_group;
    std::uint32_t second_node_tag;   // +0x14
};
static_assert(offsetof(ModelDefinition,first_node_tag)==4 && offsetof(ModelDefinition,second_node_tag)==0x14);
// A light block row (16 bytes): the light entries of one node light row.
struct LightBlockRow {
    std::uint32_t unused_here[2];
    std::int32_t count;              // +8
    std::uint32_t entries;           // +0xC: LightEntry[]
};
static_assert(sizeof(LightBlockRow)==16 && offsetof(LightBlockRow,count)==8 && offsetof(LightBlockRow,entries)==0xc);
// A light entry (0x28 bytes). An entry with a contribution function adds a
// weighted colour (134730, from its accumulate fields); one with a scale
// function scales the lit colour.
struct LightEntry {
    std::uint32_t unused_here_00;
    std::uint32_t accumulate_value;  // +4: 134730's EAX
    std::uint8_t accumulate_first[12];   // +8: 134730's ECX points here
    std::uint8_t accumulate_second[12];  // +0x14: 134730's ESI points here
    std::uint32_t scale;             // +0x20: object function, or 0
    std::uint32_t contribution;      // +0x24: object function, or 0
};
static_assert(sizeof(LightEntry)==0x28 && offsetof(LightEntry,accumulate_value)==4 && offsetof(LightEntry,accumulate_first)==8 &&
              offsetof(LightEntry,accumulate_second)==0x14 && offsetof(LightEntry,scale)==0x20 &&
              offsetof(LightEntry,contribution)==0x24);
// A node light colour row (12 bytes). The object keeps the base rows, then
// the lit rows.
struct ColourRow {
    std::uint32_t red,green,blue;    // float bit patterns
};
static_assert(sizeof(ColourRow)==12);
// A countdown timer (4 bytes).
struct CountdownTimer {
    std::uint8_t counter;
    std::uint8_t limit;              // 0: the timer is idle
    std::uint8_t unused_here;
    std::uint8_t flags;              // countdown_flags
};
static_assert(sizeof(CountdownTimer)==4 && offsetof(CountdownTimer,flags)==3);
namespace countdown_flags {
inline constexpr std::uint32_t counting_up=0x01u;
inline constexpr std::uint32_t held=0x02u;       // the timer reached its end and stays there
}

// Object type 7 (machines) moves its cluster's node bit vector instead of
// rebuilding node matrices. The original tests bit 7 of 1 << type, which
// takes the type's low five bits.
constexpr bool is_machine(std::uint32_t type){return (type&31u)==7u;}
// The bytes of BE850's node bit vector: one bit per node matrix (0x34 bytes)
// in whole dwords. The original divides the sign-extended size by 52 as an
// unsigned multiply by 0x4EC4EC4F (the high dword, shifted right 4), then
// rounds up to 32 bits.
constexpr std::uint32_t node_bit_bytes(std::uint32_t size) {
    const auto nodes=std::uint32_t((std::uint64_t(0x4ec4ec4fu)*size)>>32)>>4;
    return std::uint32_t(signed_value(nodes+0x1fu)>>5)<<2;
}
// The node light rows of a size (24 bytes per row: a base and a lit
// colour). The original divides the sign-extended size as an unsigned
// multiply by 0xAAAAAAAB (the high dword, shifted right 3), then halves it
// rounding toward zero.
constexpr std::uint32_t node_light_rows(std::uint32_t size) {
    const auto twelfths=std::uint32_t((std::uint64_t(0xaaaaaaabu)*size)>>32)>>3;
    const auto sign=signed_value(twelfths)<0?0xffffffffu:0u;
    return std::uint32_t(signed_value(twelfths-sign)>>1);
}

// ------------------------------------------------------------ object move
// BE850. One object's post-physics move: its cluster moves (BE610); an
// object that is not a machine rebuilds its node matrices (BF0C0); a
// clustered machine whose cluster asks for it clears a node bit vector in
// its frame and has 1DBBD0 mark it; the type's move callbacks run (10DD80);
// the `parent` flag moves to `previous_parent`; the bounds update (BFF20);
// an object with attachments refreshes them (C0F60) unless its bounds
// changed; each child moves; the nodes are notified (C0200). Frame:
//   handle(), handle_again()      the argument, read again after the clear
//   move_cluster(m), rebuild_node_matrices(m), clear_node_bits(bytes),
//   mark_node_bits(m, record), move_types(m), update_bounds(m) -> changed,
//   refresh_attachments(m), move_child(m), notify_nodes(m)
namespace object_move {
enum class Access {
    table,headers,object,cluster,header_type,cluster_again,cluster_flags,cluster_table,cluster_data,record_flags,
    node_matrices,parent_flags,previous_set,previous_clear,parent_clear,attachments_flag,
    first_child,child_table,child_headers,child_object,next_sibling
};
// Kept across the child calls: the handle, the object, its header and the
// child being moved.
struct Move {
    std::uint32_t handle;
    std::uint32_t object;
    std::uint32_t header;
    std::uint32_t child;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    Move m{frame.handle(),0,0,0};
    m.header=element(rows,handle_index(m.handle),sizeof(ObjectHeader));
    m.object=memory.load32(object,at(m.header,offsetof(ObjectHeader,object)));
    if(memory.load32(cluster,at(m.object,offsetof(ObjectData,cluster)))!=none)frame.move_cluster(m);
    if(!is_machine(memory.load8(header_type,at(m.header,offsetof(ObjectHeader,type)))))frame.rebuild_node_matrices(m);
    else {
        const auto reference=memory.load32(cluster_again,at(m.object,offsetof(ObjectData,cluster)));
        if(reference!=none && (memory.load8(cluster_flags,at(m.object,offsetof(ObjectData,update_flags)))&update_flags::clustered)) {
            const auto records=memory.load32(cluster_table,globals::clusters);
            const auto data=memory.load32(cluster_data,at(records,offsetof(DataArray,data)));
            const auto record=element(data,handle_index(reference),sizeof(ClusterRecord));
            if(memory.load32(record_flags,at(record,offsetof(ClusterRecord,flags)))&cluster_flags::node_bits) {
                const auto size=widen16(memory.load16(node_matrices,at(m.object,offsetof(ObjectData,node_matrices_size))));
                frame.clear_node_bits(node_bit_bytes(size));
                frame.mark_node_bits(m,record);
            }
            m.handle=frame.handle_again();
        }
    }
    frame.move_types(m);

    // The parent flag becomes the previous-parent flag, then is cleared.
    const auto flags=at(m.object,offsetof(ObjectData,update_flags));
    if(memory.load8(parent_flags,flags)&update_flags::parent) {
        const auto value=memory.load8(previous_set,flags);
        memory.store8(previous_set,flags,value|update_flags::previous_parent);
    } else {
        const auto value=memory.load8(previous_clear,flags);
        memory.store8(previous_clear,flags,value&~update_flags::previous_parent);
    }
    const auto value=memory.load8(parent_clear,flags);
    memory.store8(parent_clear,flags,value&~update_flags::parent);

    const auto bounds_changed=frame.update_bounds(m);
    if((memory.load8(attachments_flag,at(m.header,offsetof(ObjectHeader,flags)))&header_attachments) && !bounds_changed)
        frame.refresh_attachments(m);
    for(m.child=memory.load32(first_child,at(m.object,offsetof(ObjectData,first_child)));m.child!=none;) {
        frame.move_child(m);
        const auto child=object_of(memory,m.child,child_table,child_headers,child_object);
        m.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,next_sibling)));
    }
    frame.notify_nodes(m);
}
} // namespace object_move

// ----------------------------------------------------------- cluster move
// BE610. An object without a cluster is in its parent's cluster state (in a
// cluster when it has no parent). A clustered object's phantom updates
// (1D9EC0); a change clears the record's `pending` flag. When it changed, or
// the object was in its cluster, and the cluster has phantoms, they move
// (move_phantoms). The object is then in its cluster when the phantom did
// not change, and outside game mode 4 a cluster marked for release is
// released (1E1980). Returns whether the object ends in its cluster. Frame:
//   handle(), keep_object(o)         the argument; the object in a frame slot
//   update_phantom(c) -> changed, keep_phantom_result(r), phantom_result(),
//   final_phantom_result()           1D9EC0; its result kept in a frame byte
//   start_moving(c) -> moving, compute_bounds(c), compute_extent(c),
//   query_collisions(c, without_extent), move_phantom(c),
//   move_second_phantom(c), finish_phantoms(c, moved, default_value),
//   release_cluster(c, reference)
namespace cluster_move {
enum class Access {
    table,headers,object,object_flags,parent,parent_object,parent_flags,
    cluster,cluster_table,cluster_data,pending_clear,in_cluster_test,phantom_count,
    fixed_flags,extent_flags,second_flags,phantom_default,
    game,mode,release_word,release_reference,release_clear,in_cluster_set,in_cluster_clear
};
// Kept across the child calls: the handle, the object, its cluster record,
// and whether the object ends in its cluster.
struct ClusterMove {
    std::uint32_t handle;
    std::uint32_t object;
    std::uint32_t record;
    std::uint8_t in_cluster;
};
// The phantom move of a cluster that has phantoms: unless the cluster is
// fixed and when it starts moving (1D9F50), its bounds (1DC610) and, unless
// it moves one phantom, its extent (BB200) feed the collision query (B9460);
// if the phantom update changed, the phantom moves (1D8C80, and 1D8D80
// unless one phantom). B9770 always runs last.
template<class Memory,class Frame> void move_phantoms(Memory &memory,Frame &frame,ClusterMove &c) {
    using enum Access;
    bool moved=false;
    if(!(memory.load32(fixed_flags,at(c.record,offsetof(ClusterRecord,flags)))&cluster_flags::fixed) && frame.start_moving(c)) {
        frame.compute_bounds(c);
        const bool without_extent=memory.load32(extent_flags,at(c.record,offsetof(ClusterRecord,flags)))&cluster_flags::one_phantom;
        if(!without_extent)frame.compute_extent(c);
        frame.query_collisions(c,without_extent);
        if(frame.phantom_result()) {
            frame.move_phantom(c);
            if(!(memory.load32(second_flags,at(c.record,offsetof(ClusterRecord,flags)))&cluster_flags::one_phantom))
                frame.move_second_phantom(c);
            moved=true;
        }
    }
    const auto fallback=moved?0u:memory.load32(phantom_default,subtree_globals::phantom_default);
    frame.finish_phantoms(c,moved,fallback);
}
template<class Memory,class Frame> bool run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    ClusterMove c{frame.handle(),0,0,0};
    c.object=memory.load32(object,at(element(rows,handle_index(c.handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto flags=memory.load8(object_flags,at(c.object,offsetof(ObjectData,update_flags)));
    frame.keep_object(c.object);
    if(!(flags&update_flags::clustered)) {
        const auto parent_handle=memory.load32(parent,at(c.object,offsetof(ObjectData,parent)));
        if(parent_handle==none)c.in_cluster=1;
        else {
            const auto parent_data=memory.load32(parent_object,
                at(element(rows,handle_index(parent_handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
            c.in_cluster=memory.load8(parent_flags,at(parent_data,offsetof(ObjectData,cluster_flags)))&object_cluster_flags::in_cluster;
        }
    } else {
        const auto reference=memory.load32(cluster,at(c.object,offsetof(ObjectData,cluster)));
        const auto records=memory.load32(cluster_table,globals::clusters);
        const auto offset=handle_index(reference)*std::uint32_t(sizeof(ClusterRecord));
        c.record=offset+memory.load32(cluster_data,at(records,offsetof(DataArray,data)));
        const auto changed=frame.update_phantom(c);
        frame.keep_phantom_result(changed);
        bool active=true;
        if(changed) {
            const auto record_flags=memory.load32(pending_clear,at(c.record,offsetof(ClusterRecord,flags)));
            memory.store32(pending_clear,at(c.record,offsetof(ClusterRecord,flags)),record_flags&~cluster_flags::pending);
        } else active=memory.load8(in_cluster_test,at(c.object,offsetof(ObjectData,cluster_flags)))&object_cluster_flags::in_cluster;
        if(active && signed_value(memory.load32(phantom_count,at(c.record,offsetof(ClusterRecord,phantom_count))))>0)
            move_phantoms(memory,frame,c);
        c.in_cluster=frame.final_phantom_result()==0;
        const auto game_mode=memory.load8(mode,at(memory.load32(game,globals::game_globals),offsetof(native_object_update::GameGlobals,mode)));
        // The release bit is bit 11 of the word at the update flags (bit 3 of
        // the cluster flags).
        if(game_mode!=4u && ((memory.load16(release_word,at(c.object,offsetof(ObjectData,update_flags)))>>11)&1u)) {
            frame.release_cluster(c,memory.load32(release_reference,at(c.object,offsetof(ObjectData,cluster))));
            const auto cluster_bits=memory.load8(release_clear,at(c.object,offsetof(ObjectData,cluster_flags)));
            memory.store8(release_clear,at(c.object,offsetof(ObjectData,cluster_flags)),cluster_bits&~object_cluster_flags::release);
        }
    }
    const auto state=at(c.object,offsetof(ObjectData,cluster_flags));
    if(c.in_cluster) {
        const auto bits=memory.load8(in_cluster_set,state);
        memory.store8(in_cluster_set,state,bits|object_cluster_flags::in_cluster);
    } else {
        const auto bits=memory.load8(in_cluster_clear,state);
        memory.store8(in_cluster_clear,state,bits&~object_cluster_flags::in_cluster);
    }
    return c.in_cluster;
}
} // namespace cluster_move

// ----------------------------------------------------- attachment refresh
// C0F60(handle, detach, reconnect, siblings, ready). A ready object is
// skipped when it is pending deletion or its `skip` flag is set; otherwise
// its birth (BBD10) runs and a blocked birth ends the refresh. Each node
// light attachment that is not attached and has a light is detached (C5250)
// and reconnected (C4D30) as the arguments ask, unless the birth blocked;
// the placement runs (C3750); the first child refreshes with the same
// arguments, its siblings, and ready when the birth did not block; and, as
// asked, the refresh continues with the next sibling (the original rewrites
// its handle argument and loops). Frame:
//   handle(), handle_again(), ready(), detach_requested(),
//   reconnect_requested(), siblings_requested(), continue_with(sibling)
//   birth(r) -> blocked, detach_light(r), reconnect_light(r, light),
//   place(r), refresh_child(r, child)
namespace attachment_refresh {
enum class Access {
    table,headers,object,header_flags,skip_flags,node_flags,nodes_size,nodes_offset,
    node_attached,node_light,node_light_again,placement_flags,first_child,next_sibling
};
// Kept across the child calls: the handle, the object, the node attachment
// being visited and the ones left, and whether the birth blocked.
struct Refresh {
    std::uint32_t handle;
    std::uint32_t object;
    std::uint32_t entry;
    std::uint32_t remaining;
    std::uint8_t blocked;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    for(;;) {
        Refresh r{frame.handle(),0,0,0,0};
        const auto array=memory.load32(table,globals::object_headers);
        const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
        const bool ready=frame.ready();
        const auto header=element(rows,handle_index(r.handle),sizeof(ObjectHeader));
        r.object=memory.load32(object,at(header,offsetof(ObjectHeader,object)));
        if(ready) {
            if(memory.load8(header_flags,at(header,offsetof(ObjectHeader,flags)))&native_object_update::header_flags::pending_delete)return;
            if(memory.load8(skip_flags,at(r.object,offsetof(ObjectData,attachment_flags)))&attachment_flags::skip)return;
            r.blocked=0;
        } else {
            r.blocked=frame.birth(r);
            if(r.blocked)return;
        }
        if(memory.load32(node_flags,at(r.object,offsetof(ObjectData,attachment_flags)))&attachment_flags::node_lights) {
            // The count is the sign-extended size shifted right as unsigned.
            const auto count=widen16(memory.load16(nodes_size,at(r.object,offsetof(ObjectData,node_attachments_size))))>>3;
            const auto list=widen16(memory.load16(nodes_offset,at(r.object,offsetof(ObjectData,node_attachments_offset))))+r.object;
            if(signed_value(count)>0) {
                r.entry=list;r.remaining=count;
                do {
                    if(!memory.load8(node_attached,at(r.entry,offsetof(NodeLightAttachment,attached))) &&
                        memory.load32(node_light,at(r.entry,offsetof(NodeLightAttachment,light)))!=none && !r.blocked) {
                        if(frame.detach_requested())frame.detach_light(r);
                        if(frame.reconnect_requested())
                            frame.reconnect_light(r,memory.load32(node_light_again,at(r.entry,offsetof(NodeLightAttachment,light))));
                    }
                    r.entry+=std::uint32_t(sizeof(NodeLightAttachment));
                } while(--r.remaining);
                r.handle=frame.handle_again();
            }
        }
        if(memory.load32(placement_flags,at(r.object,offsetof(ObjectData,attachment_flags)))&attachment_flags::placement)frame.place(r);
        const auto child=memory.load32(first_child,at(r.object,offsetof(ObjectData,first_child)));
        if(child!=none)frame.refresh_child(r,child);
        if(!frame.siblings_requested())return;
        const auto sibling=memory.load32(next_sibling,at(r.object,offsetof(ObjectData,next_sibling)));
        if(sibling==none)return;
        frame.continue_with(sibling);
    }
}
} // namespace attachment_refresh

// --------------------------------------------------------------- type moves
// 10DD80. Runs the move callback of each definition in the type chain of the
// object's type (the object's handle in EDI). Frame:
//   handle()                    the register argument
//   type_move(callback, walk)   the callback, with the handle argument
namespace type_moves {
enum class Access {table,headers,object,type,definition,chain_first,chain_entry,move,chain_next};
// Kept across the callbacks: the handle and the address of the chain slot.
struct ChainWalk {
    std::uint32_t handle;
    std::uint32_t slot;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    ChainWalk walk{frame.handle(),0};
    const auto data=memory.load32(object,at(element(rows,handle_index(walk.handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto object_type=widen8(memory.load8(type,at(data,offsetof(ObjectData,type))));
    const auto type_definition=memory.load32(definition,element(globals::object_type_definitions,object_type,4));
    walk.slot=at(type_definition,offsetof(ObjectTypeDefinition,type_chain));
    if(!memory.load32(chain_first,walk.slot))return;
    std::uint32_t next;
    do {
        const auto entry=memory.load32(chain_entry,walk.slot);
        if(const auto callback=memory.load32(move,at(entry,offsetof(ObjectTypeDefinition,move))))frame.type_move(callback,walk);
        next=memory.load32(chain_next,walk.slot+4u);
        walk.slot+=4u;
    } while(next);
}
} // namespace type_moves

// -------------------------------------------------------------- node notify
// C0200. When the object's model has both node tags, BAC30 builds the node
// matrices into a local and 10E1F0 gets that local and BAC30's result. The
// object's handle is in EAX. Frame:
//   handle(), node_matrices(handle), notify_nodes()
namespace node_notify {
enum class Access {table,headers,tags,object,definition_index,definition,model,model_definition,first_node_tag,second_node_tag};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    const auto instances=memory.load32(tags,subtree_globals::tag_instances);
    const auto handle=frame.handle();
    const auto data=memory.load32(object,at(element(rows,handle_index(handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto definition_tag=memory.load32(definition_index,at(data,offsetof(ObjectData,definition)))&0xffffu;
    const auto object_definition=memory.load32(definition,at(element(instances,definition_tag,sizeof(TagInstance)),offsetof(TagInstance,definition)));
    const auto model_tag=memory.load32(model,at(object_definition,offsetof(ObjectDefinition,model)));
    if(model_tag==none)return;
    const auto model_data=memory.load32(model_definition,at(element(instances,handle_index(model_tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
    if(memory.load32(first_node_tag,at(model_data,offsetof(ModelDefinition,first_node_tag)))==none)return;
    if(memory.load32(second_node_tag,at(model_data,offsetof(ModelDefinition,second_node_tag)))==none)return;
    frame.node_matrices(handle);
    frame.notify_nodes();
}
} // namespace node_notify

// ---------------------------------------------------------------- countdown
// 1E15D0. One countdown timer (its address in ECX). An idle timer (limit 0)
// or a held one does nothing (false). Counting up, the counter is stored
// incremented; at the limit it is set to the limit and held. Counting down,
// the counter is stored decremented; at zero or below it is cleared and
// held. Either way the timer ran (true).
namespace countdown {
enum class Access {limit,flags,count_up,count_up_store,limit_store,held_up,count_down,count_down_store,zero_store,held_down};
enum class Exit {idle,held,running,reached_limit};
template<class Memory> Exit run(Memory &memory,std::uint32_t timer) {
    using enum Access;
    const auto counter=at(timer,offsetof(CountdownTimer,counter)),flags_address=at(timer,offsetof(CountdownTimer,flags));
    const auto end=memory.load8(limit,at(timer,offsetof(CountdownTimer,limit)));
    if(!end)return Exit::idle;
    const auto state=memory.load8(flags,flags_address);
    if(state&countdown_flags::held)return Exit::held;
    if(state&countdown_flags::counting_up) {
        const auto value=(memory.load8(count_up,counter)+1u)&0xffu;
        memory.store8(count_up_store,counter,value);
        if(signed_value(widen8(value))<signed_value(widen8(end)))return Exit::running;
        memory.store8(limit_store,counter,end);
        memory.store8(held_up,flags_address,state|countdown_flags::held);
        return Exit::reached_limit;
    }
    const auto value=(memory.load8(count_down,counter)-1u)&0xffu;
    memory.store8(count_down_store,counter,value);
    if(signed_value(widen8(value))>0)return Exit::running;
    memory.store8(zero_store,counter,0);
    memory.store8(held_down,flags_address,state|countdown_flags::held);
    return Exit::running;
}
} // namespace countdown

// ------------------------------------------------------------------- timers
// 1D5790. Runs the first countdown timer of a timer block (its address in
// ESI); when that timer is held, it is reset (limit, counter, flags); then
// runs the second timer. Returns the OR of both results. 1E15D0 leaves the
// timer's address in ECX, and the reset uses it. Frame:
//   block(), run_first_timer(t) -> ran, run_second_timer(t) -> ran
namespace timers {
enum class Access {first_held,reset_limit,reset_counter,reset_flags};
// Kept across the timer calls: the block, the timer's address and the first
// timer's result.
struct Timers {
    std::uint32_t block;
    std::uint32_t timer;
    std::uint8_t first;
};
template<class Memory,class Frame> std::uint8_t run(Memory &memory,Frame &frame) {
    using enum Access;
    Timers t{frame.block(),0,0};
    t.timer=at(t.block,offsetof(TimerBlock,first_timer));
    t.first=frame.run_first_timer(t);
    if(memory.load8(first_held,at(t.block,offsetof(TimerBlock,first_timer)+offsetof(CountdownTimer,flags)))&countdown_flags::held) {
        memory.store8(reset_limit,at(t.timer,offsetof(CountdownTimer,limit)),0);
        memory.store8(reset_counter,at(t.timer,offsetof(CountdownTimer,counter)),0);
        memory.store8(reset_flags,at(t.timer,offsetof(CountdownTimer,flags)),0);
    }
    t.timer=at(t.block,offsetof(TimerBlock,second_timer));
    const auto second=frame.run_second_timer(t);
    return std::uint8_t(t.first|second);
}
} // namespace timers

// -------------------------------------------------------------- node lights
// C0910. When the object's definition has node lights, each node light row
// is lit: its base colour is copied to its lit row; each light entry of the
// row with a contribution function adds its weighted colour (BCB70 weighs
// the function for the object; 134730 accumulates with the weight); each
// entry with a scale function scales the lit colour by the function's
// weight (0 when BCB70 does not weigh it); each channel is clamped to
// [0, limit] (a NaN channel stays). Returns the result byte the original
// keeps in its frame (1 when there are rows, else 0). Frame:
//   handle(), keep_definition(d), definition(),
//   begin_rows(first_row, rows)    keeps the source row, the light row
//                                  offset (0), the result (1) and the rows
//   current_light_offset(), current_source_row()   the row's slots, at its start
//   light_offset(), source_row(), remaining(),     the same slots, at its end
//   next_row(light_offset, source_row, remaining)
//   reset_contribution_index(), reset_contribution_offset(),
//   contribution_entry_offset(), contribution_index(), contribution_offset(),
//   next_contribution(i, o)        the walk's index and entry offset slots
//   weigh_contribution(r, function) -> weighed, clear_contribution_weight(),
//   contribution_weight(), accumulate(r, value, weight)
//   weigh_scale(r, function) -> weighed, clear_scale_weight(), scale_weight()
//   result()
namespace node_lights {
enum class Access {
    table,headers,object,definition_index,tags,definition,lit_flag,lights_size,lights_offset,
    light_block,copy_red,store_red,copy_green,store_green,copy_blue,store_blue,
    contribution_count,contribution_entries,contribution,accumulate_value,contribution_count_again,
    scale_count,scale_entries,scale,scale_red,scaled_red,scale_green,scaled_green,scale_blue,scaled_blue,scale_count_again,
    red_test,limit,red_value,red_keep,green_test,red_store,green_value,green_keep,blue_test,green_store,blue_value,blue_keep,blue_store
};
enum class Exit {unlit,no_rows,lit};
struct Result {
    Exit exit;
    std::uint32_t rows;
    std::uint8_t value;
};
// Kept across the child calls: the light block row, the lit colour row, the
// entry being weighed, and the scale walk's index and entry offset.
struct LitRow {
    std::uint32_t light_row;
    std::uint32_t lit_row;
    std::uint32_t entry;
    std::uint32_t scale_index;
    std::uint32_t scale_offset;
};
// COMISS 0,channel; JA: a negative channel becomes 0 (a NaN does not);
// otherwise a channel above the limit becomes the limit, else the channel is
// read again. The bit patterns are the original's: +0 and the limit's.
template<class Memory> std::uint32_t clamp_rest(Memory &memory,std::uint32_t address,std::uint32_t limit_bits,Access value,Access keep) {
    if(sse::above(memory.load32(value,address),limit_bits))return limit_bits;
    return memory.load32(keep,address);
}
template<class Memory,class Frame> Result run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto handle=frame.handle();
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows_table=memory.load32(headers,at(array,offsetof(DataArray,data)));
    const auto data=memory.load32(object,at(element(rows_table,handle_index(handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto definition_tag=memory.load32(definition_index,at(data,offsetof(ObjectData,definition)))&0xffffu;
    const auto instances=memory.load32(tags,subtree_globals::tag_instances);
    auto object_definition=memory.load32(definition,at(element(instances,definition_tag,sizeof(TagInstance)),offsetof(TagInstance,definition)));
    const auto lit=memory.load8(lit_flag,at(object_definition,offsetof(ObjectDefinition,node_light_flags)));
    frame.keep_definition(object_definition);
    if(!(lit&1u))return {Exit::unlit,0,0};
    const auto size=widen16(memory.load16(lights_size,at(data,offsetof(ObjectData,node_lights_size))));
    LitRow r{};
    const auto first_row=widen16(memory.load16(lights_offset,at(data,offsetof(ObjectData,node_lights_offset))))+data;
    const auto rows=node_light_rows(size);
    if(signed_value(rows)<=0)return {Exit::no_rows,rows,0};
    frame.begin_rows(first_row,rows);
    r.lit_row=first_row+rows*std::uint32_t(sizeof(ColourRow));
    for(;;) {
        const auto block=memory.load32(light_block,at(object_definition,offsetof(ObjectDefinition,light_block)));
        r.light_row=block+frame.current_light_offset();
        const auto source=frame.current_source_row();
        // The base colour is copied channel by channel.
        const auto red=memory.load32(copy_red,at(source,offsetof(ColourRow,red)));
        memory.store32(store_red,at(r.lit_row,offsetof(ColourRow,red)),red);
        const auto green=memory.load32(copy_green,at(source,offsetof(ColourRow,green)));
        memory.store32(store_green,at(r.lit_row,offsetof(ColourRow,green)),green);
        const auto blue=memory.load32(copy_blue,at(source,offsetof(ColourRow,blue)));
        memory.store32(store_blue,at(r.lit_row,offsetof(ColourRow,blue)),blue);

        // Contributions: the index and the entry offset live in frame slots.
        const auto contributions=memory.load32(contribution_count,at(r.light_row,offsetof(LightBlockRow,count)));
        frame.reset_contribution_index();
        if(signed_value(contributions)>0) {
            frame.reset_contribution_offset();
            std::uint32_t index,count;
            do {
                const auto entries=memory.load32(contribution_entries,at(r.light_row,offsetof(LightBlockRow,entries)));
                r.entry=entries+frame.contribution_entry_offset();
                if(const auto function=memory.load32(contribution,at(r.entry,offsetof(LightEntry,contribution)))) {
                    if(!frame.weigh_contribution(r,function))frame.clear_contribution_weight();
                    const auto value=memory.load32(accumulate_value,at(r.entry,offsetof(LightEntry,accumulate_value)));
                    const auto weight=frame.contribution_weight();
                    frame.accumulate(r,value,weight);
                }
                index=frame.contribution_index()+1u;
                const auto offset=frame.contribution_offset()+std::uint32_t(sizeof(LightEntry));
                count=memory.load32(contribution_count_again,at(r.light_row,offsetof(LightBlockRow,count)));
                frame.next_contribution(index,offset);
            } while(signed_value(index)<signed_value(count));
        }

        // Scales.
        const auto scales=memory.load32(scale_count,at(r.light_row,offsetof(LightBlockRow,count)));
        r.scale_index=0;
        if(signed_value(scales)>0) {
            r.scale_offset=0;
            std::uint32_t count;
            do {
                const auto entries=memory.load32(scale_entries,at(r.light_row,offsetof(LightBlockRow,entries)));
                if(const auto function=memory.load32(scale,at(entries+r.scale_offset,offsetof(LightEntry,scale)))) {
                    std::uint32_t weight=0;  // +0.0 when the function does not weigh
                    if(frame.weigh_scale(r,function))weight=frame.scale_weight();
                    else frame.clear_scale_weight();
                    // The weight multiplies red; green and blue multiply the weight.
                    const auto red_bits=memory.load32(scale_red,at(r.lit_row,offsetof(ColourRow,red)));
                    memory.store32(scaled_red,at(r.lit_row,offsetof(ColourRow,red)),sse::multiply(weight,red_bits));
                    const auto green_bits=memory.load32(scale_green,at(r.lit_row,offsetof(ColourRow,green)));
                    memory.store32(scaled_green,at(r.lit_row,offsetof(ColourRow,green)),sse::multiply(green_bits,weight));
                    const auto blue_bits=memory.load32(scale_blue,at(r.lit_row,offsetof(ColourRow,blue)));
                    memory.store32(scaled_blue,at(r.lit_row,offsetof(ColourRow,blue)),sse::multiply(blue_bits,weight));
                }
                count=memory.load32(scale_count_again,at(r.light_row,offsetof(LightBlockRow,count)));
                ++r.scale_index;
                r.scale_offset+=std::uint32_t(sizeof(LightEntry));
            } while(signed_value(r.scale_index)<signed_value(count));
        }

        // Clamp each channel to [0, limit]. Each channel's sign test reads it
        // before the previous channel is stored.
        const auto red_address=at(r.lit_row,offsetof(ColourRow,red));
        const auto green_address=at(r.lit_row,offsetof(ColourRow,green));
        const auto blue_address=at(r.lit_row,offsetof(ColourRow,blue));
        const bool red_negative=sse::above(0u,memory.load32(red_test,red_address));
        const auto limit_bits=memory.load32(limit,subtree_globals::node_light_limit);
        const auto red_result=red_negative?0u:clamp_rest(memory,red_address,limit_bits,red_value,red_keep);
        const bool green_negative=sse::above(0u,memory.load32(green_test,green_address));
        memory.store32(red_store,red_address,red_result);
        const auto green_result=green_negative?0u:clamp_rest(memory,green_address,limit_bits,green_value,green_keep);
        const bool blue_negative=sse::above(0u,memory.load32(blue_test,blue_address));
        memory.store32(green_store,green_address,green_result);
        const auto blue_result=blue_negative?0u:clamp_rest(memory,blue_address,limit_bits,blue_value,blue_keep);

        const auto light_offset=frame.light_offset();
        const auto source_row=frame.source_row();
        const auto remaining=frame.remaining();
        memory.store32(blue_store,blue_address,blue_result);
        r.lit_row+=std::uint32_t(sizeof(ColourRow));
        frame.next_row(light_offset+std::uint32_t(sizeof(LightBlockRow)),source_row+std::uint32_t(sizeof(ColourRow)),remaining-1u);
        if(remaining==1u)break;
        object_definition=frame.definition();
    }
    return {Exit::lit,rows,std::uint8_t(frame.result())};
}
} // namespace node_lights
} // namespace halo2::native_object_subtree
