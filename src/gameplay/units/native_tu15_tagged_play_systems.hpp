#pragma once
// Native TU1.5 tagged play, entered at 1D6430 (physics live support, AI
// frame and AI actor units): an object's tagged entry found and played.
//
//   tagged_play::play   1D6430(EDI = object; first, second, third) -> RET C
//
// An object with a tag (+0x68, NONE for none) has 1D45A0 find the entry for
// the first argument (EAX), the object and the key 7000101 into the frame's
// three words; 1E5FB0 then plays the first word (EAX) with the tag's
// definition (EBX; the tag instances at [4F0414], 16 bytes, +8), the other
// two words and the second and third arguments.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_tagged_play.hpp) maps the role to the original instruction
// and owns the ABI of the children; the object, which the original keeps in
// EDI across them, is refreshed from the register each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_tagged_play {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t tags=0x4f0414u;}   // 16-byte tag instances (+8 the definition)
inline constexpr std::uint32_t object_tag=0x68u,tag_size=16u,tag_definition=8u,entry_key=0x7000101u;

namespace tagged_play {
enum class Access {tag,first,third,second,word2,word1,tag_again,tags,definition,word0};
// Frame:
//   argument(k), local(k)                         the argument's and the frame word's slots now
//   found(object, first) -> object                1D45A0 into the frame's words
//   played(word0, definition, word1, word2, second, third)   1E5FB0
template<class Memory,class Frame> bool play(Memory &memory,Frame &frame,std::uint32_t object_address) {
    using enum Access;
    if(memory.load32(tag,at(object_address,object_tag))==none)return false;
    object_address=frame.found(object_address,memory.load32(first,frame.argument(1u)));
    const auto third_value=memory.load32(third,frame.argument(3u));
    const auto second_value=memory.load32(second,frame.argument(2u));
    const auto word2_value=memory.load32(word2,frame.local(2u));
    const auto word1_value=memory.load32(word1,frame.local(1u));
    const auto tag_handle=memory.load32(tag_again,at(object_address,object_tag));
    const auto definition_address=memory.load32(definition,memory.load32(tags,globals::tags)+tag_size*(tag_handle&0xffffu)+tag_definition);
    frame.played(memory.load32(word0,frame.local(0u)),definition_address,word1_value,word2_value,second_value,third_value);
    return true;
}
} // namespace tagged_play
} // namespace halo2::native_tagged_play
