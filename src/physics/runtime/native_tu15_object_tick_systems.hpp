#pragma once
// Native TU1.5 object update systems: the per-tick object passes of the live
// support unit (docs/native_replacement/B8F40_object_tick.md):
//
//   update_pass::run         B8F40  the tick's update: the player roots, then
//                                   every other awake object
//   move_pass::run           B9090  the post-physics move: every moved object
//                                   follows its new position
//   late_pass::run           B9180  the post-update pass: the late updates,
//                                   then the deletions the tick requested
//   object_update::run       BE4A0  one object's update, then its children's
//   type_updates::run        10DD10 the update callbacks of an object's type
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (each system's Access enum); the boundary adapter
// (native_tu15_object_tick.hpp) maps the role to the original instruction for
// the diagnostic cursor and the store observer. Stack arguments, the locals
// the original keeps in its stack frame, and the child calls come from a
// Frame object that the adapter also supplies. Registers, flags and stack
// slots belong to the adapter, not to this file.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the store
//     observer sees every write, a faulting read must be the original's first
//     fault, and verification replays every read in order);
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a child
//     call are named in each system's "kept" struct (RootWalk, ObjectScan,
//     ...). A conforming callee returns them unchanged; the adapter hands
//     back what the callee actually returned, so the logic continues exactly
//     as the original does even when a callee does not preserve them.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include <cstddef>
#include <cstdint>
#include <optional>

namespace halo2::native_object_update {

// ------------------------------------------------------------------ helpers
// base + offset in the original's 32-bit address arithmetic.
constexpr std::uint32_t at(std::uint32_t base,std::size_t offset){return base+std::uint32_t(offset);}
// The address of element index of an array of size-byte elements.
constexpr std::uint32_t element(std::uint32_t base,std::uint32_t index,std::size_t size){return base+index*std::uint32_t(size);}
constexpr std::int32_t signed_value(std::uint32_t x){return std::int32_t(x);}
// MOVSX of a byte or a word.
constexpr std::uint32_t widen8(std::uint32_t byte){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(byte))));}
constexpr std::uint32_t widen16(std::uint32_t word){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(word))));}

// NONE: the empty handle and the empty index.
inline constexpr std::uint32_t none=0xffffffffu;
// A datum handle: the datum's identifier (salt) in the high word, its index
// in the low word. Lookups use only the index; the identifier is not checked.
constexpr std::uint32_t handle_index(std::uint32_t handle){return handle&0xffffu;}
constexpr std::uint32_t make_handle(std::uint32_t identifier,std::uint32_t index){return (identifier<<16)|index;}

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t object_type_definitions=0x474b90u; // ObjectTypeDefinition*[object type]
inline constexpr std::uint32_t object_globals=0x4eabc4u;          // ObjectGlobals*
inline constexpr std::uint32_t object_headers=0x4ecbd0u;          // DataArray* of ObjectHeader
inline constexpr std::uint32_t game_globals=0x4f3218u;            // GameGlobals*
inline constexpr std::uint32_t player_roots=0x51daf0u;            // PlayerRoots*
inline constexpr std::uint32_t clusters=0x52be5cu;                // DataArray* of ClusterRecord
}

// data_array: the game's generic datum table.
struct DataArray {
    char name[32];
    std::int32_t maximum_count;
    std::int32_t datum_size;          // +0x24
    std::uint8_t alignment_bits,valid;
    std::uint16_t flags;
    std::uint32_t signature;
    std::uint32_t allocation;
    std::int32_t next_index;
    std::int32_t first_unallocated;   // +0x38: every in-use index is below it
    std::int32_t actual_count;
    std::uint32_t next_identifier;
    std::uint32_t data;               // +0x44: the datums
    std::uint32_t in_use_bits;        // +0x48: one bit per index
};
static_assert(offsetof(DataArray,datum_size)==0x24 && offsetof(DataArray,first_unallocated)==0x38 &&
              offsetof(DataArray,data)==0x44 && offsetof(DataArray,in_use_bits)==0x48);

// An object's header datum (12 bytes) in the object header array.
struct ObjectHeader {
    std::uint16_t identifier;         // the handle's high word
    std::uint8_t flags;               // header_flags
    std::uint8_t type;                // object type (7: machines)
    std::uint8_t unused_here[4];      // +4: not read by these systems
    std::uint32_t object;             // +8: the object's data (ObjectData)
};
static_assert(sizeof(ObjectHeader)==12 && offsetof(ObjectHeader,flags)==2 && offsetof(ObjectHeader,object)==8);
namespace header_flags {
inline constexpr std::uint32_t active=0x01u;
inline constexpr std::uint32_t awake=0x02u;          // BE4A0 puts an object to sleep (clears it)
inline constexpr std::uint32_t moved=0x04u;          // set by an update that moved the object
inline constexpr std::uint32_t late_update=0x08u;    // updated by the post-update pass (B9180)
inline constexpr std::uint32_t pending_delete=0x10u; // deleted by the post-update pass
inline constexpr std::uint32_t held=0x20u;           // BE4A0 reports the object changed without
                                                     // updating it; B9180 clears it every tick.
                                                     // The game meaning is not established.
}
// An object updates in the update pass when it is active, awake, and not
// left to the post-update pass.
constexpr bool updates_now(std::uint32_t flags) {
    return (flags&header_flags::active) && (flags&header_flags::awake) && !(flags&header_flags::late_update);
}
// A moved object moves in the post-physics pass unless it waits for the
// post-update pass.
constexpr bool moves_now(std::uint32_t flags){return (flags&header_flags::moved) && !(flags&header_flags::late_update);}

// The object update globals ([4EABC4]).
struct ObjectGlobals {
    std::uint8_t unused_here[3];
    std::uint8_t updating;            // +3: 1 while an update pass runs
};
static_assert(offsetof(ObjectGlobals,updating)==3);

// The player-controlled root objects ([51DAF0]): they update first.
struct PlayerRoots {
    std::uint8_t unused_here[0x1d00];
    std::uint32_t handles[32];        // +0x1D00
    std::int32_t count;               // +0x1D80
};
static_assert(offsetof(PlayerRoots,handles)==0x1d00 && offsetof(PlayerRoots,count)==0x1d80);

// The game globals ([4F3218]).
struct GameGlobals {
    std::uint8_t unused_here[0xc];
    std::int8_t mode;                 // +0xC: BE4A0 and BE610 test for 4 and 5. The game
                                      // meaning is not established.
};
static_assert(offsetof(GameGlobals,mode)==0xc);

// An object type definition. Its type chain lists the definitions the type
// derives from, and its own; each definition's callbacks run for every
// object whose type has it in its chain.
struct ObjectTypeDefinition {
    std::uint8_t unused_here_00[0x40];
    std::uint32_t update;             // +0x40: update callback, or 0
    std::uint32_t move;               // +0x44: move callback, or 0
    std::uint8_t unused_here_48[0x3c];
    std::uint32_t type_chain[1];      // +0x84: ObjectTypeDefinition*, null-terminated
};
static_assert(offsetof(ObjectTypeDefinition,update)==0x40 && offsetof(ObjectTypeDefinition,move)==0x44 &&
              offsetof(ObjectTypeDefinition,type_chain)==0x84);

// A cluster record (0xA0 bytes) of the [52BE5C] array: the physics cluster
// of a clustered object.
struct ClusterRecord {
    std::uint32_t unused_here_00;
    std::uint32_t flags;              // +4: cluster_flags
    std::uint8_t unused_here_08[0x6c];
    std::int32_t phantom_count;       // +0x74
    std::uint8_t unused_here_78[0x28];
};
static_assert(sizeof(ClusterRecord)==0xa0 && offsetof(ClusterRecord,flags)==4 && offsetof(ClusterRecord,phantom_count)==0x74);

// An object's data: the fields the update systems use.
struct ObjectData {
    std::uint32_t definition;              // +0x00: the definition's tag index (low word)
    std::uint32_t attachment_flags;        // +0x04: attachment_flags (C0F60)
    std::uint32_t unused_here_08;
    std::uint32_t next_sibling;            // +0x0C
    std::uint32_t first_child;             // +0x10
    std::uint32_t parent;                  // +0x14
    std::uint8_t unused_here_18[0x92];
    std::int8_t type;                      // +0xAA: index of the ObjectTypeDefinition
    std::uint8_t unused_here_ab[8];
    std::uint8_t countdown;                // +0xB3: ticks the object stays awake
    std::uint32_t cluster;                 // +0xB4: ClusterRecord handle, or NONE
    std::uint8_t unused_here_b8[8];
    std::uint8_t update_flags;             // +0xC0: update_flags
    std::uint8_t cluster_flags;            // +0xC1: object_cluster_flags
    std::uint8_t unused_here_c2[0x16];
    std::uint8_t mode_flags;               // +0xD8: bit 0 asks for B37A0 in modes 4 and 5
    std::uint8_t unused_here_d9[0x3b];
    std::int16_t node_matrices_size;       // +0x114: bytes of node matrices (0x34 each)
    std::uint8_t unused_here_116[6];
    std::int16_t node_attachments_size;    // +0x11C: bytes of node attachments (8 each)
    std::int16_t node_attachments_offset;  // +0x11E: from the object
    std::uint8_t unused_here_120[4];
    std::int16_t node_lights_size;         // +0x124: bytes of node light rows (24 per row)
    std::int16_t node_lights_offset;       // +0x126: from the object
    std::uint8_t unused_here_128[2];
    std::int16_t timers_offset;            // +0x12A: TimerBlock, from the object; -1 for none
};
static_assert(offsetof(ObjectData,next_sibling)==0xc && offsetof(ObjectData,first_child)==0x10 &&
              offsetof(ObjectData,parent)==0x14 && offsetof(ObjectData,type)==0xaa &&
              offsetof(ObjectData,countdown)==0xb3 && offsetof(ObjectData,cluster)==0xb4 &&
              offsetof(ObjectData,update_flags)==0xc0 && offsetof(ObjectData,cluster_flags)==0xc1 &&
              offsetof(ObjectData,mode_flags)==0xd8 && offsetof(ObjectData,node_matrices_size)==0x114 &&
              offsetof(ObjectData,node_attachments_size)==0x11c && offsetof(ObjectData,node_attachments_offset)==0x11e &&
              offsetof(ObjectData,node_lights_size)==0x124 && offsetof(ObjectData,node_lights_offset)==0x126 &&
              offsetof(ObjectData,timers_offset)==0x12a);
namespace update_flags {
inline constexpr std::uint32_t previous_parent=0x02u; // BE850 copies `parent` here each move
inline constexpr std::uint32_t parent=0x04u;          // BE4A0 runs BDA50 for it; BE850 clears it.
                                                      // The game meaning is not established.
inline constexpr std::uint32_t disabled=0x08u;        // skipped by the update and move scans
inline constexpr std::uint32_t clustered=0x40u;       // the object has a physics cluster
}
namespace object_cluster_flags {
inline constexpr std::uint32_t in_cluster=0x01u;      // BE610 sets or clears it each move
inline constexpr std::uint32_t release=0x08u;         // BE610 releases the cluster (1E1980)
}
// The object's timer block (TimerBlock): two countdown timers and the
// handle that enables them.
struct TimerBlock {
    std::uint8_t unused_here[0x60];
    std::uint8_t second_timer[4];      // +0x60
    std::uint8_t first_timer[4];       // +0x64
    std::uint32_t owner;               // +0x68: NONE when the timers do not run
};
static_assert(offsetof(TimerBlock,second_timer)==0x60 && offsetof(TimerBlock,first_timer)==0x64 &&
              offsetof(TimerBlock,owner)==0x68);

// --------------------------------------------------------- shared lookups
// The header of a handle's object, through the global header table (two
// reads: the table, then its datums).
template<class Memory,class A> std::uint32_t header_of(Memory &memory,std::uint32_t handle,A table,A headers) {
    const auto array=memory.load32(table,globals::object_headers);
    return element(memory.load32(headers,at(array,offsetof(DataArray,data))),handle_index(handle),sizeof(ObjectHeader));
}
// The data of a handle's object (the header table, its datums, the header's
// object pointer).
template<class Memory,class A> std::uint32_t object_of(Memory &memory,std::uint32_t handle,A table,A headers,A object) {
    return memory.load32(object,at(header_of(memory,handle,table,headers),offsetof(ObjectHeader,object)));
}
// The datum at index of a data array (its size, then its datums).
template<class Memory,class A> std::uint32_t datum_of(Memory &memory,std::uint32_t array,std::uint32_t index,A size,A data) {
    const auto datum_size=memory.load32(size,at(array,offsetof(DataArray,datum_size)));
    return datum_size*index+memory.load32(data,at(array,offsetof(DataArray,data)));
}

// The original's inlined data_array iteration: the next in-use index after
// `previous`, below the array's first unallocated index. The limit and the
// bit vector are read again for every search.
enum class SearchEnd {
    negative_start,   // previous + 1 is negative
    past_limit,       // previous + 1 is at or past the limit
    bits_exhausted,   // no in-use bit before the limit
    invalid_index     // the bit found is NONE's; a non-negative start cannot reach it
};
struct Search {
    std::optional<SearchEnd> end;
    std::uint32_t index;  // the found index, or where the search stopped
};
template<class Memory,class A> Search next_in_use(Memory &memory,std::uint32_t array,std::uint32_t previous,A limit_access,
                                                  A bits_access,A word_access) {
    auto index=previous+1u;
    if(signed_value(index)<0)return {SearchEnd::negative_start,index};
    const auto limit=memory.load32(limit_access,at(array,offsetof(DataArray,first_unallocated)));
    if(signed_value(index)>=signed_value(limit))return {SearchEnd::past_limit,index};
    const auto bits=memory.load32(bits_access,at(array,offsetof(DataArray,in_use_bits)));
    // The word of a bit is the index shifted arithmetically, as the original does.
    while(!(memory.load32(word_access,element(bits,std::uint32_t(signed_value(index)>>5),4))&(1u<<(index&31u)))) {
        ++index;
        if(signed_value(index)>=signed_value(limit))return {SearchEnd::bits_exhausted,index};
    }
    if(index==none)return {SearchEnd::invalid_index,index};
    return {std::nullopt,index};
}

// A scan over the object header array. The original keeps the array and the
// position in callee-saved registers across the per-object calls.
struct ObjectScan {
    std::uint32_t array;
    std::uint32_t index;
};

// ------------------------------------------------------------ update pass
// B8F40. The tick's update: each player root, then every other in-use
// object, in data array order. An object updates (BE4A0) when it is active,
// awake and not left to the post-update pass; a root that moved also
// updates its attachments (10E630). The object globals' `updating` byte is
// set for the pass. Frame:
//   keep_root_count(n), root_count()  the root count, kept in the original's
//                                     stack frame and read again each root
//   update_root(walk), attach_root(walk)
//   update_object(handle, scan)
//   search_ended(search)              the scan's end, for the adapter
namespace update_pass {
enum class Access {
    roots,state,root_count,updating_set,
    root_handle,root_table,root_headers,root_flags,root_flags_after,
    scan_table,scan_limit,scan_bits,scan_word,datum_size,datum_data,datum_identifier,datum_flags,
    object_table,object_headers,object,object_flags,
    state_again,updating_clear
};
// The roots walk, kept across the update and attach calls.
struct RootWalk {
    std::uint32_t list;      // PlayerRoots::handles
    std::uint32_t position;
    std::uint32_t handle;
    std::uint32_t header;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto player_roots=memory.load32(roots,globals::player_roots);
    const auto object_globals=memory.load32(state,globals::object_globals);
    RootWalk walk{at(player_roots,offsetof(PlayerRoots,handles)),0,0,0};
    const auto count=memory.load32(root_count,at(player_roots,offsetof(PlayerRoots,count)));
    frame.keep_root_count(count);
    memory.store8(updating_set,at(object_globals,offsetof(ObjectGlobals,updating)),1);
    if(signed_value(count)>0) do {
        walk.handle=memory.load32(root_handle,element(walk.list,walk.position,4));
        walk.header=header_of(memory,walk.handle,root_table,root_headers);
        if(updates_now(memory.load8(root_flags,at(walk.header,offsetof(ObjectHeader,flags))))) {
            frame.update_root(walk);
            // A root that moved in its update brings its attachments along.
            if(moves_now(memory.load8(root_flags_after,at(walk.header,offsetof(ObjectHeader,flags)))))frame.attach_root(walk);
        }
        ++walk.position;
    } while(signed_value(walk.position)<signed_value(frame.root_count()));

    ObjectScan scan{memory.load32(scan_table,globals::object_headers),none};
    for(;;) {
        const auto search=next_in_use(memory,scan.array,scan.index,scan_limit,scan_bits,scan_word);
        if(search.end){frame.search_ended(search);break;}
        scan.index=search.index;
        const auto header=datum_of(memory,scan.array,scan.index,datum_size,datum_data);
        const auto identifier=memory.load16(datum_identifier,at(header,offsetof(ObjectHeader,identifier)));
        const auto flags=memory.load8(datum_flags,at(header,offsetof(ObjectHeader,flags)));
        const auto handle=make_handle(identifier,scan.index);
        if(!updates_now(flags))continue;
        const auto data=object_of(memory,handle,object_table,object_headers,object);
        if(memory.load8(object_flags,at(data,offsetof(ObjectData,update_flags)))&update_flags::disabled)continue;
        frame.update_object(handle,scan);
    }
    const auto object_globals_again=memory.load32(state_again,globals::object_globals);
    memory.store8(updating_clear,at(object_globals_again,offsetof(ObjectGlobals,updating)),0);
}
} // namespace update_pass

// -------------------------------------------------------------- move pass
// B9090. After the physics step: each player root that moved (BE850, then
// its transform 10E710), then every other moved object that is not disabled
// (BE850), then 102910. Objects waiting for the post-update pass do not
// move here. Frame:
//   move_root(walk), transform_root(walk), move_object(handle, scan)
//   search_ended(search), finish()    102910, after the scan
namespace move_pass {
enum class Access {
    roots,root_count,state,updating_set,
    root_handle,root_table,root_headers,root_flags,
    scan_table,scan_limit,scan_bits,scan_word,datum_size,datum_data,datum_identifier,datum_flags,object,object_flags,
    state_again,updating_clear
};
// The roots walk, kept across the move and transform calls. The original
// also keeps the root count there, so it is not read again.
struct RootWalk {
    std::uint32_t list;
    std::uint32_t count;
    std::uint32_t position;
    std::uint32_t handle;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto player_roots=memory.load32(roots,globals::player_roots);
    RootWalk walk{at(player_roots,offsetof(PlayerRoots,handles)),
                  memory.load32(root_count,at(player_roots,offsetof(PlayerRoots,count))),0,0};
    const auto object_globals=memory.load32(state,globals::object_globals);
    memory.store8(updating_set,at(object_globals,offsetof(ObjectGlobals,updating)),1);
    if(signed_value(walk.count)>0) do {
        walk.handle=memory.load32(root_handle,element(walk.list,walk.position,4));
        const auto header=header_of(memory,walk.handle,root_table,root_headers);
        if(moves_now(memory.load8(root_flags,at(header,offsetof(ObjectHeader,flags))))) {
            frame.move_root(walk);
            frame.transform_root(walk);
        }
        ++walk.position;
    } while(signed_value(walk.position)<signed_value(walk.count));

    ObjectScan scan{memory.load32(scan_table,globals::object_headers),none};
    for(;;) {
        const auto search=next_in_use(memory,scan.array,scan.index,scan_limit,scan_bits,scan_word);
        if(search.end){frame.search_ended(search);break;}
        scan.index=search.index;
        const auto header=datum_of(memory,scan.array,scan.index,datum_size,datum_data);
        const auto handle=make_handle(memory.load16(datum_identifier,at(header,offsetof(ObjectHeader,identifier))),scan.index);
        if(!moves_now(memory.load8(datum_flags,at(header,offsetof(ObjectHeader,flags)))))continue;
        const auto data=memory.load32(object,at(header,offsetof(ObjectHeader,object)));
        if(memory.load8(object_flags,at(data,offsetof(ObjectData,update_flags)))&update_flags::disabled)continue;
        frame.move_object(handle,scan);
    }
    frame.finish();
    const auto object_globals_again=memory.load32(state_again,globals::object_globals);
    memory.store8(updating_clear,at(object_globals_again,offsetof(ObjectGlobals,updating)),0);
}
} // namespace move_pass

// -------------------------------------------------------- post-update pass
// B9180. Every in-use object loses its `held` flag. Each one marked for a
// late update that is active, awake and not pending deletion has the mark
// cleared and updates (BE4A0), and moves (BE850) if the update moved it.
// Then 102910, then every object pending deletion is detached (BA3E0) and
// released (BA490), and the pass ends in C13B0 (a tail call: it returns to
// this pass's caller). Frame:
//   update_object(scan), move_object(scan)
//   finish_updates()                  102910
//   detach_object(scan), release_object(scan)
//   search_ended(search), finish()    C13B0
namespace late_pass {
enum class Access {
    state,scan_table,updating_set,
    scan_limit,scan_bits,scan_word,datum_size,datum_data,datum_flags,datum_identifier,held_clear,late_clear,flags_after,
    delete_table,delete_limit,delete_bits,delete_word,delete_size,delete_data,delete_identifier,delete_flags,
    state_again,updating_clear
};
// Kept across the update and move calls: the array, the position, the
// object's header and its handle.
struct LateScan {
    std::uint32_t array;
    std::uint32_t index;
    std::uint32_t header;
    std::uint32_t handle;
};
// Kept across the detach and release calls.
struct DeleteScan {
    std::uint32_t array;
    std::uint32_t index;
    std::uint32_t handle;
};
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto object_globals=memory.load32(state,globals::object_globals);
    LateScan late{memory.load32(scan_table,globals::object_headers),none,0,0};
    memory.store8(updating_set,at(object_globals,offsetof(ObjectGlobals,updating)),1);
    for(;;) {
        const auto search=next_in_use(memory,late.array,late.index,scan_limit,scan_bits,scan_word);
        if(search.end){frame.search_ended(search);break;}
        late.index=search.index;
        late.header=datum_of(memory,late.array,late.index,datum_size,datum_data);
        // This scan reads the flags before the identifier.
        auto flags=memory.load8(datum_flags,at(late.header,offsetof(ObjectHeader,flags)));
        late.handle=make_handle(memory.load16(datum_identifier,at(late.header,offsetof(ObjectHeader,identifier))),late.index);
        flags&=~header_flags::held;
        memory.store8(held_clear,at(late.header,offsetof(ObjectHeader,flags)),flags);
        if(!(flags&header_flags::late_update))continue;
        if(!(flags&header_flags::active) || !(flags&header_flags::awake) || (flags&header_flags::pending_delete))continue;
        memory.store8(late_clear,at(late.header,offsetof(ObjectHeader,flags)),flags&~header_flags::late_update);
        frame.update_object(late);
        if(memory.load8(flags_after,at(late.header,offsetof(ObjectHeader,flags)))&header_flags::moved)frame.move_object(late);
    }
    frame.finish_updates();

    DeleteScan doomed{memory.load32(delete_table,globals::object_headers),none,0};
    for(;;) {
        const auto search=next_in_use(memory,doomed.array,doomed.index,delete_limit,delete_bits,delete_word);
        if(search.end){frame.search_ended(search);break;}
        doomed.index=search.index;
        const auto header=datum_of(memory,doomed.array,doomed.index,delete_size,delete_data);
        doomed.handle=make_handle(memory.load16(delete_identifier,at(header,offsetof(ObjectHeader,identifier))),doomed.index);
        if(!(memory.load8(delete_flags,at(header,offsetof(ObjectHeader,flags)))&header_flags::pending_delete))continue;
        frame.detach_object(doomed);
        frame.release_object(doomed);
    }
    const auto object_globals_again=memory.load32(state_again,globals::object_globals);
    memory.store8(updating_clear,at(object_globals_again,offsetof(ObjectGlobals,updating)),0);
    frame.finish();
}
} // namespace late_pass

// --------------------------------------------------------- one object
// BE4A0. Updates one object, then its children (recursively), and returns
// whether anything changed. An object that is not awake is not updated
// (false); a held one is not updated (true). Otherwise:
//   - the header's `moved` flag is cleared; the update callbacks set it again;
//   - BDA50 runs when the object's `parent` flag is set, and B37A0 when the
//     game mode is 4 or 5 and the object's mode flag is set;
//   - the object's timers run (1D5790) when it has a timer block whose owner
//     is not NONE;
//   - its type updates (10DD10), its damage (D8330) and its node lights
//     (C0910) run;
//   - each child updates, through the sibling chain;
//   - a clustered object updates its cluster record (1D6FB0);
//   - a nonzero countdown is decremented and keeps the object changed;
//   - otherwise an object that did not change and did not move goes to sleep
//     (its `awake` flag is cleared).
// Each update result is OR-ed into `changed`. Frame:
//   handle()                         the argument
//   keep_header(h), header()         the original rewrites its argument slot
//                                    with the header, and reads it back
//   parent_update(u), mode_update(u) BDA50, B37A0
//   timers(u), type_updates(u), damage(u), node_lights(u),
//   update_child(u), cluster_update(u, record)
namespace object_update {
enum class Access {
    table,headers,header_flags,object,moved_clear,parent_flags,game,mode,mode_flags,
    timer_table,timer_headers,timer_object,timers_offset,block_offset,timer_owner,
    first_child,child_table,child_headers,child_object,next_sibling,
    cluster,cluster_flags,cluster_table,cluster_data,
    countdown,countdown_store,sleep_flags,sleep_store
};
enum class Exit {asleep,held,counting_down,updated};
struct Result {
    Exit exit;
    std::uint8_t changed;
};
// Kept across the child calls: the handle, the header row's offset in the
// header array, the object, the child being updated, and `changed`.
struct Update {
    std::uint32_t handle;
    std::uint32_t row;
    std::uint32_t object;
    std::uint32_t child;
    std::uint8_t changed;
};
template<class Memory,class Frame> Result run(Memory &memory,Frame &frame) {
    using enum Access;
    Update u{frame.handle(),0,0,0,0};
    u.row=handle_index(u.handle)*std::uint32_t(sizeof(ObjectHeader));
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    const auto flags=memory.load8(header_flags,at(rows+u.row,offsetof(ObjectHeader,flags)));
    const auto header=rows+u.row;
    frame.keep_header(header);
    if(!(flags&header_flags::awake))return {Exit::asleep,0};
    if(flags&header_flags::held)return {Exit::held,1};
    u.object=memory.load32(object,at(header,offsetof(ObjectHeader,object)));
    memory.store8(moved_clear,at(header,offsetof(ObjectHeader,flags)),flags&~header_flags::moved);
    if(memory.load8(parent_flags,at(u.object,offsetof(ObjectData,update_flags)))&update_flags::parent)frame.parent_update(u);
    const auto game_mode=signed_value(widen8(memory.load8(mode,at(memory.load32(game,globals::game_globals),offsetof(GameGlobals,mode)))));
    if(game_mode>=4 && game_mode<=5 && (memory.load8(mode_flags,at(u.object,offsetof(ObjectData,mode_flags)))&1u))frame.mode_update(u);

    // Whether the object has a timer block is read from the object found
    // again through its header (at the kept row offset); the block's offset
    // is then read from the object found first.
    const auto rows_again=memory.load32(timer_headers,
        at(memory.load32(timer_table,globals::object_headers),offsetof(DataArray,data)));
    const auto again=memory.load32(timer_object,at(rows_again+u.row,offsetof(ObjectHeader,object)));
    if(memory.load16(timers_offset,at(again,offsetof(ObjectData,timers_offset)))!=0xffffu) {
        const auto block=widen16(memory.load16(block_offset,at(u.object,offsetof(ObjectData,timers_offset))))+u.object;
        if(memory.load32(timer_owner,at(block,offsetof(TimerBlock,owner)))!=none)u.changed=frame.timers(u,block);
    }
    u.changed|=frame.type_updates(u);
    u.changed|=frame.damage(u);
    u.changed|=frame.node_lights(u);

    for(u.child=memory.load32(first_child,at(u.object,offsetof(ObjectData,first_child)));u.child!=none;) {
        u.changed|=frame.update_child(u);
        const auto child=object_of(memory,u.child,child_table,child_headers,child_object);
        u.child=memory.load32(next_sibling,at(child,offsetof(ObjectData,next_sibling)));
    }

    const auto reference=memory.load32(cluster,at(u.object,offsetof(ObjectData,cluster)));
    if(reference!=none && (memory.load8(cluster_flags,at(u.object,offsetof(ObjectData,update_flags)))&update_flags::clustered)) {
        const auto records=memory.load32(cluster_table,globals::clusters);
        const auto offset=handle_index(reference)*std::uint32_t(sizeof(ClusterRecord));
        frame.cluster_update(u,offset+memory.load32(cluster_data,at(records,offsetof(DataArray,data))));
    }

    if(const auto ticks=memory.load8(countdown,at(u.object,offsetof(ObjectData,countdown)))) {
        memory.store8(countdown_store,at(u.object,offsetof(ObjectData,countdown)),ticks-1u);
        return {Exit::counting_down,1};
    }
    if(!u.changed) {
        // The header comes back from the argument slot the original rewrote.
        const auto kept=frame.header();
        const auto now=memory.load8(sleep_flags,at(kept,offsetof(ObjectHeader,flags)));
        if(!(now&header_flags::moved))memory.store8(sleep_store,at(kept,offsetof(ObjectHeader,flags)),now&~header_flags::awake);
    }
    return {Exit::updated,u.changed};
}
} // namespace object_update

// ------------------------------------------------------- type updates
// 10DD10. Runs the update callback of each definition in the type chain of
// the object's type and returns the OR of their results. The original
// counts the chain with a 32-bit counter but indexes it with the counter's
// low 16 bits, sign-extended. Frame:
//   handle()                        the argument
//   type_update(callback, walk)     the callback, with the handle argument
namespace type_updates {
enum class Access {table,headers,object,type,definition,chain_first,chain_entry,update,chain_next};
// Kept across the callbacks: the handle, the type's definition, the chain
// position and the OR of the results.
struct ChainWalk {
    std::uint32_t handle;
    std::uint32_t definition;
    std::uint32_t position;
    std::uint8_t changed;
};
inline std::uint32_t chain_entry_address(const ChainWalk &walk) {
    return element(at(walk.definition,offsetof(ObjectTypeDefinition,type_chain)),widen16(walk.position),4);
}
template<class Memory,class Frame> std::uint8_t run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,at(array,offsetof(DataArray,data)));
    ChainWalk walk{frame.handle(),0,0,0};
    const auto data=memory.load32(object,at(element(rows,handle_index(walk.handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto object_type=widen8(memory.load8(type,at(data,offsetof(ObjectData,type))));
    walk.definition=memory.load32(definition,element(globals::object_type_definitions,object_type,4));
    // The first entry is read twice: once to see that the chain is not empty.
    if(memory.load32(chain_first,at(walk.definition,offsetof(ObjectTypeDefinition,type_chain)))) {
        std::uint32_t next;
        do {
            const auto entry=memory.load32(chain_entry,chain_entry_address(walk));
            if(const auto callback=memory.load32(update,at(entry,offsetof(ObjectTypeDefinition,update))))
                walk.changed|=frame.type_update(callback,walk);
            ++walk.position;
            next=memory.load32(chain_next,chain_entry_address(walk));
        } while(next);
    }
    return walk.changed;
}
} // namespace type_updates
} // namespace halo2::native_object_update
