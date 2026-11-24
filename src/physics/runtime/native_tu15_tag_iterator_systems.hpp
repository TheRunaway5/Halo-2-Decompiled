#pragma once
// Native TU1.5 tag iterator, entered at 125790 (physics live support unit):
// the next tag of a group.
//
//   tag_iterator::next   125790(EDI = iterator) -> EAX (tag handle or NONE)
//
// The iterator holds the current tag (+8), the next tag instance index
// (+0xC) and the group (+0x10, NONE for every group). Below the tag count
// ([556D90]+0x18), the instances (16 bytes at 4F0414: group, handle) are
// stepped through: an instance with a group and a handle is the next tag
// when the iterator wants every group, or when the iterator's group is one
// of the three groups 125810 returns for the instance (its group and the
// groups it derives from). The tag's handle becomes the current one and is
// returned; past the last instance the result is NONE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_tag_iterator.hpp) maps the role to the original instruction
// and owns the ABI of the child 125810. The values the original keeps in
// callee-saved registers across it (NONE in EBX among them) are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_tag_iterator {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t tags=0x556d90u;            // +0x18: the tag count
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // 16-byte instances
}
inline constexpr std::uint32_t tag_count=0x18u,instance_shift=4u;
struct TagIterator {std::uint32_t unused_here[2],current,index,group;};
static_assert(offsetof(TagIterator,current)==8 && offsetof(TagIterator,index)==0xc && offsetof(TagIterator,group)==0x10);
struct TagInstance {std::uint32_t group,handle,address,size;};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,handle)==4);

namespace tag_iterator {
enum class Access {
    tags_first,index_first,count_first,index,instances,index_store,group,handle,wanted_group,wanted_again,group_0,
    group_1,group_2,tags,index_again,count,found_handle,current_store,found_again
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t iterator;   // EDI
    std::uint32_t instance;   // ESI: the tag instance
    std::uint32_t none;       // EBX: NONE, as the comparisons read it
};
// How the walk ended: no instance left at the start, a tag found, or the
// instances ran out.
enum class Exit {exhausted,found,ended};
struct Result {Exit exit;std::uint32_t handle;};
// Frame:
//   group_groups(kept)   125810 for the instance; returns EAX (three
//                        groups), refreshes kept
template<class Memory,class Frame> Result next(Memory &memory,Frame &frame,std::uint32_t iterator) {
    using enum Access;
    Kept kept{iterator,0u,none};
    const auto table=memory.load32(tags_first,globals::tags);
    const auto start=memory.load32(index_first,at(iterator,offsetof(TagIterator,index)));
    if(!(std::int32_t(start)<std::int32_t(memory.load32(count_first,at(table,tag_count)))))return {Exit::exhausted,kept.none};
    for(;;) {
        const auto position=memory.load32(index,at(kept.iterator,offsetof(TagIterator,index)));
        kept.instance=(position<<instance_shift)+memory.load32(instances,globals::tag_instances);
        memory.store32(index_store,at(kept.iterator,offsetof(TagIterator,index)),position+1u);
        if(kept.instance!=0 && memory.load32(group,at(kept.instance,offsetof(TagInstance,group)))!=kept.none &&
           memory.load32(handle,at(kept.instance,offsetof(TagInstance,handle)))!=kept.none) {
            bool found=memory.load32(wanted_group,at(kept.iterator,offsetof(TagIterator,group)))==kept.none;
            if(!found) {
                const auto groups=frame.group_groups(kept);
                const auto wanted=memory.load32(wanted_again,at(kept.iterator,offsetof(TagIterator,group)));
                found=wanted==memory.load32(group_0,groups) || wanted==memory.load32(group_1,at(groups,4u)) ||
                      wanted==memory.load32(group_2,at(groups,8u));
            }
            if(found) {
                memory.store32(current_store,at(kept.iterator,offsetof(TagIterator,current)),
                               memory.load32(found_handle,at(kept.instance,offsetof(TagInstance,handle))));
                return {Exit::found,memory.load32(found_again,at(kept.instance,offsetof(TagInstance,handle)))};
            }
        }
        const auto table_now=memory.load32(tags,globals::tags);
        const auto position_now=memory.load32(index_again,at(kept.iterator,offsetof(TagIterator,index)));
        if(!(std::int32_t(position_now)<std::int32_t(memory.load32(count,at(table_now,tag_count)))))return {Exit::ended,kept.none};
    }
}
} // namespace tag_iterator
} // namespace halo2::native_tag_iterator
