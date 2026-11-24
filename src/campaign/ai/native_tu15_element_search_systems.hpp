#pragma once
// Native TU1.5 element search, entered at 1E5360 (AI frame and physics live
// support units): which element of a definition, or of the tags it
// references, holds a value.
//
//   element_search::find   1E5360(EDX = definition; output, value)
//
// The definition's own block, then each of its 0x20-byte references (+0x4C
// count, +0x50 array; the reference's tag handle at +4, its block through
// the tag instances at 4F0414), holds 0x6C-byte elements (+0x2C count, +0x30
// array) whose first word is compared with the value. The answer is packed
// as two words: the reference's index (0xFFFF for the definition's own
// block) and the element's index, both 0xFFFF when nothing holds it. A
// found answer is first reported to 1E5C80. The search ends without an
// answer past the last reference, or at a reference without a block; an
// element index of 0xFFFF counts as no element. Without a definition the
// answer is the empty one.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the packed
// answer, the reference's index and offset) and its arguments, which it
// rereads; the boundary adapter (native_tu15_element_search.hpp) maps the
// role to the original instruction and owns the ABI of the child (1E5C80).
// The answer the original keeps in ESI across it is a Kept, refreshed from
// the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_element_search {
using native_object_update::at;
using native_object_update::signed_value;
inline constexpr std::uint32_t none=0xffffffffu,none_word=0xffffu;
namespace definition {inline constexpr std::uint32_t references=0x4cu,reference_array=0x50u;}
namespace reference {inline constexpr std::uint32_t size=0x20u,tag=4u;}
namespace block {inline constexpr std::uint32_t count=0x2cu,elements=0x30u;}
inline constexpr std::uint32_t element_size=0x6cu;
// The compiled form of the element's null check: the offset -0x6C names no element.
inline constexpr std::uint32_t null_offset=0u-element_size;
namespace tags {inline constexpr std::uint32_t instances=0x4f0414u,instance_size=16u,instance_data=8u;}

namespace element_search {
enum class Access {reference_init,element_init,index_init,empty_packed,empty_output,empty_answer,offset_init,
    element_count,elements,value,element_value,element_reload,reference_word,reference_store,element_store,
    index_load,offset_load,reference_count,index_store,offset_store,reference_array,reference_tag,instances,tag_data,
    found_packed,found_output,found_answer,missing_packed,missing_output,missing_answer};
enum class Outcome {no_definition,found_own,found_referenced,exhausted,unresolved};
// The original's ESI across 1E5C80: the packed answer.
struct Kept {std::uint32_t packed;};
// Frame:
//   answer(), index(), offset(), argument(k)   the frame's packed answer, the reference's index and offset, the arguments now
//   element(index, offset)    the element's registers
//   next_block(block)         the reference's block, 0 for none
//   report(packed, kept)      1E5C80(the packed answer pushed), refreshes kept
template<class Memory,class Frame> Outcome answer_missing(Memory &memory,Frame &frame,Outcome outcome) {
    using enum Access;
    const auto packed=memory.load32(missing_packed,frame.answer());
    memory.store32(missing_answer,memory.load32(missing_output,frame.argument(0u)),packed);
    return outcome;
}
template<class Memory,class Frame> Outcome find(Memory &memory,Frame &frame,std::uint32_t definition_address) {
    using enum Access;
    memory.store16(reference_init,frame.answer(),none_word);
    memory.store16(element_init,frame.answer()+2u,none_word);
    memory.store32(index_init,frame.index(),none);
    if(!definition_address) {
        const auto packed=memory.load32(empty_packed,frame.answer());
        memory.store32(empty_answer,memory.load32(empty_output,frame.argument(0u)),packed);
        return Outcome::no_definition;
    }
    memory.store32(offset_init,frame.offset(),0u-reference::size);
    auto block_address=definition_address;
    auto found=none_word;
    bool own=true;
    for(;;) {
        if(found!=none_word) {
            Kept kept{memory.load32(found_packed,frame.answer())};
            frame.report(kept.packed,kept);
            memory.store32(found_answer,memory.load32(found_output,frame.argument(0u)),kept.packed);
            return own?Outcome::found_own:Outcome::found_referenced;
        }
        const auto count=memory.load32(element_count,at(block_address,block::count));
        if(signed_value(count)>0) {
            std::uint32_t index=0u,offset=0u;
            bool hit=false;
            for(;;) {
                frame.element(index,offset);
                const auto element=offset==null_offset?0u:memory.load32(elements,at(block_address,block::elements))+offset;
                const auto sought=memory.load32(value,frame.argument(1u));
                if(memory.load32(element_value,element)==sought){hit=true;break;}
                ++index;offset+=element_size;
                if(!(signed_value(index)<signed_value(count)))break;
            }
            if(hit) {
                const auto reference_index=memory.load16(reference_word,frame.index())&0xffffu;
                found=index&0xffffu;
                memory.store16(reference_store,frame.answer(),reference_index);
                memory.store16(element_store,frame.answer()+2u,found);
                if(found!=none_word)continue;        // the block is still there: the loop's top answers
            } else {
                found=memory.load16(element_reload,frame.answer()+2u)&0xffffu;
            }
        }
        const auto next_index=memory.load32(index_load,frame.index())+1u;
        const auto next_offset=memory.load32(offset_load,frame.offset())+reference::size;
        const auto references=memory.load32(reference_count,at(definition_address,definition::references));
        memory.store32(index_store,frame.index(),next_index);
        memory.store32(offset_store,frame.offset(),next_offset);
        if(!(signed_value(next_index)<signed_value(references)))return answer_missing(memory,frame,Outcome::exhausted);
        const auto handle=memory.load32(reference_tag,
            memory.load32(Access::reference_array,at(definition_address,definition::reference_array))+next_offset+reference::tag);
        block_address=0u;
        if(handle!=none)
            block_address=memory.load32(tag_data,memory.load32(Access::instances,tags::instances)+tags::instance_size*(handle&0xffffu)+tags::instance_data);
        frame.next_block(block_address);
        own=false;
        if(!block_address && found==none_word)return answer_missing(memory,frame,Outcome::unresolved);
    }
}
} // namespace element_search
} // namespace halo2::native_element_search
