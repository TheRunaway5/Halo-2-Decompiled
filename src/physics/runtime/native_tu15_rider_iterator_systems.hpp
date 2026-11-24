#pragma once
// Native TU1.5 rider iterator, entered at D2BD0 (physics live support
// unit): the next unit riding an object.
//
//   rider_iterator::next   D2BD0(EDX iterator) -> AL
//
// The iterator holds its root object (+0), the current object (+4), the
// next child to look at (+8) and, for the unit it finds, its object data
// (+0xC, the object's +0x14), its handle (+0x10) and its word (+0x14, the
// object's +0x1FC). Objects ([4ECBD0], 12-byte entries, the address at +8)
// link their first child (+0x10) and next sibling (+0xC); a unit is of
// type 0 or 1 (+0xAA). From the root, the pending children are looked at
// first, then the current object descends to the root's first child that is
// a unit with the riding bit 26 (+4) set; from any other current object the
// pending children are looked at, and the current object moves on through
// its siblings the same way, until the current
// object is the root or none. A unit among the pending children is the
// answer; one found from the root lets the walk go on before answering.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_rider_iterator.hpp) maps the role to the original
// instruction and leaves through the original's epilogue for each exit.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_rider_iterator {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu;
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace iterator {inline constexpr std::uint32_t root=0u,current=4u,pending=8u,object=0xcu,handle=0x10u,word=0x14u;}
namespace object {inline constexpr std::uint32_t flags=4u,sibling=0xcu,child=0x10u,data=0x14u,type=0xaau,word=0x1fcu;
    inline constexpr unsigned riding_bit=26u;inline constexpr std::uint32_t unit_types=3u;}
// The frame (offsets from the ESP after ECX, EBX, EBP, ESI and EDI are
// pushed): the answer byte in ECX's slot.
namespace local {inline constexpr std::uint32_t answer=0x13u;}

namespace rider_iterator {
enum class Access {
    root_load,current_test,header_load,found_clear,pending_test,pending_load,data_load,object_load,sibling_load,pending_store,
    type_load,pending_again,root_reload,data_root,root_object,first_child,child_object,child_type,child_flags,child_sibling,
    current_store,data_current,current_object,current_child,pending_set,current_load,root_test,pending_check,next_pending,next_data,
    next_object,next_sibling,next_pending_store,next_type,next_pending_again,found_test,current_reload,sibling_data,sibling_current,
    current_sibling,sibling_object,sibling_type,sibling_flags,sibling_next,current_next_store,next_data_current,next_current_object,
    next_current_child,next_pending_set,current_check,answer_load,handle_store,word_load,word_store,found_object_load,found_set,
    found_object_store,handle_store_next,word_load_next,word_store_next,found_object_next,found_object_store_next};
// The original's exits: the shared epilogue, and a unit found among the
// pending children past the root (its own epilogue among the stores).
enum class Exit {common,found};
// The roles of one look through the pending children.
struct PendingRoles {Access pending,data,object,sibling,store,type;};
// The roles of one walk along a chain of children for the current object.
struct ChainRoles {Access object,type,flags,sibling,store,data,current_object,child,pending_store;bool siblings;};

// The pending children, one at a time, until a unit (true) or none.
template<class Memory,class Frame> bool pending_unit(Memory &memory,Frame &frame,std::uint32_t it,std::uint32_t header,
                                                    const PendingRoles &r,Access again,bool first,std::uint32_t &handle,std::uint32_t &found) {
    for(;;) {
        handle=memory.load32(r.pending,at(it,iterator::pending));
        const auto index=frame.pending_index(handle);
        const auto objects=memory.load32(r.data,at(header,datum_data));
        found=memory.load32(r.object,objects+entry::size*index+entry::address);
        memory.store32(r.store,at(it,iterator::pending),memory.load32(r.sibling,at(found,object::sibling)));
        if(frame.unit(memory.load8(r.type,at(found,object::type))))return true;
        if(!frame.pending_left(memory.load32(again,at(it,iterator::pending)),first))return false;
    }
}
// Along a chain of children from the given one: the first riding unit; the
// current object becomes it (or none), and the pending children its
// children.
template<class Memory,class Frame> void walk(Memory &memory,Frame &frame,std::uint32_t it,std::uint32_t header,std::uint32_t objects,
                                            std::uint32_t child,const ChainRoles &r) {
    if(frame.named(child)) {
        for(;;) {
            const auto o=memory.load32(r.object,objects+entry::size*frame.child_index(child)+entry::address);
            if(frame.unit(memory.load8(r.type,at(o,object::type))) &&
               frame.riding(memory.load32(r.flags,at(o,object::flags))))break;
            child=memory.load32(r.sibling,at(o,object::sibling));
            if(!frame.named(child))break;
        }
    }
    const bool any=frame.chain_end(child,r.siblings);
    memory.store32(r.store,at(it,iterator::current),child);
    if(any) {
        const auto data=memory.load32(r.data,at(header,datum_data));
        const auto o=memory.load32(r.current_object,data+entry::size*frame.current_index(child)+entry::address);
        memory.store32(r.pending_store,at(it,iterator::pending),memory.load32(r.child,at(o,object::child)));
    }
}

// Frame: see native_tu15_rider_iterator.hpp.
template<class Memory,class Frame> Exit next(Memory &memory,Frame &frame,std::uint32_t it) {
    using enum Access;
    frame.reserving();
    const auto root=memory.load32(root_load,at(it,iterator::root));
    const bool starting=frame.same(root,memory.load32(current_test,at(it,iterator::current)));
    frame.saving(1u);
    const auto header=memory.load32(header_load,globals::objects);
    frame.saving(2u);
    frame.saving(3u);
    frame.saving(4u);
    memory.store8(found_clear,frame.local(local::answer),0u);
    std::uint32_t handle{},found{};
    if(starting) {
        // From the root: its pending children, then down to its children.
        bool unit_found=false;
        if(frame.named(memory.load32(pending_test,at(it,iterator::pending))))
            unit_found=pending_unit(memory,frame,it,header,{pending_load,data_load,object_load,sibling_load,pending_store,type_load},
                pending_again,true,handle,found);
        if(unit_found) {
            memory.store32(handle_store,at(it,iterator::handle),handle);
            const auto word=memory.load16(word_load,at(found,object::word));
            memory.store16(word_store,at(it,iterator::word),word);
            const auto data=memory.load32(found_object_load,at(found,object::data));
            memory.store8(found_set,frame.local(local::answer),1u);
            memory.store32(found_object_store,at(it,iterator::object),data);
        } else {
            const auto root_handle=memory.load32(root_reload,at(it,iterator::root));
            const auto objects=memory.load32(data_root,at(header,datum_data));
            const auto root_object=memory.load32(Access::root_object,objects+entry::size*frame.root_index(root_handle)+entry::address);
            walk(memory,frame,it,header,objects,memory.load32(first_child,at(root_object,object::child)),
                {child_object,child_type,child_flags,child_sibling,current_store,data_current,current_object,current_child,pending_set,false});
        }
    }

    // Past the root: the pending children, then on through the siblings.
    const auto current=memory.load32(current_load,at(it,iterator::current));
    if(!frame.same(memory.load32(root_test,at(it,iterator::root)),current) && frame.named(frame.ending_none(current))) {
        for(;;) {
            if(frame.named(memory.load32(pending_check,at(it,iterator::pending)))) {
                if(pending_unit(memory,frame,it,header,{next_pending,next_data,next_object,next_sibling,next_pending_store,next_type},
                    next_pending_again,false,handle,found)) {
                    memory.store32(handle_store_next,at(it,iterator::handle),handle);
                    const auto word=memory.load16(word_load_next,at(found,object::word));
                    frame.leaving_found(1u);
                    memory.store16(word_store_next,at(it,iterator::word),word);
                    const auto data=memory.load32(found_object_next,at(found,object::data));
                    frame.leaving_found(2u);
                    memory.store32(found_object_store_next,at(it,iterator::object),data);
                    return Exit::found;
                }
            }
            if(frame.answered(memory.load8(found_test,frame.local(local::answer))))return Exit::common;
            const auto current_handle=memory.load32(current_reload,at(it,iterator::current));
            const auto objects=memory.load32(sibling_data,at(header,datum_data));
            const auto current_object_address=memory.load32(sibling_current,objects+entry::size*frame.root_index(current_handle)+entry::address);
            walk(memory,frame,it,header,objects,memory.load32(current_sibling,at(current_object_address,object::sibling)),
                {sibling_object,sibling_type,sibling_flags,sibling_next,current_next_store,next_data_current,next_current_object,
                 next_current_child,next_pending_set,true});
            if(!frame.named(memory.load32(current_check,at(it,iterator::current))))break;
        }
    }
    memory.load8(answer_load,frame.local(local::answer));
    return Exit::common;
}
} // namespace rider_iterator
} // namespace halo2::native_rider_iterator
