#pragma once
// Native TU1.5 record view, entered at 304C50 (physics live support unit): a
// record's four-word view handed on.
//
//   record_view::forward   304C50(target, record, third, fourth)
//
// The view (the frame's four words) is the record's source's +0xC (the
// source the record's +0), the record's +4 and +8, and the record itself;
// 2F4990 (cdecl) takes the target, the view, and the third and fourth
// arguments.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_view.hpp) maps the role to the original instruction
// and owns the ABI of the child.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_view {
using native_object_update::at;
namespace record {inline constexpr std::uint32_t source=0u,second=4u,third=8u;}
inline constexpr std::uint32_t source_word=0xcu;

namespace record_view {
enum class Access {record,third_word,source,word2_store,record_store,source_word,second_word,third_argument,word0_store,
    fourth_argument,target,word1_store};
// Frame: argument(k), local(k) the slots now; forward(target, view, third, fourth) 2F4990.
template<class Memory,class Frame> void forward(Memory &memory,Frame &frame) {
    using enum Access;
    const auto record_address=memory.load32(Access::record,frame.argument(2u));
    const auto third_value=memory.load32(third_word,at(record_address,record::third));
    const auto source_address=memory.load32(source,at(record_address,record::source));
    memory.store32(word2_store,frame.local(2u),third_value);
    memory.store32(record_store,frame.local(3u),record_address);
    const auto source_value=memory.load32(Access::source_word,at(source_address,native_record_view::source_word));
    const auto second_value=memory.load32(second_word,at(record_address,record::second));
    const auto third_argument_value=memory.load32(third_argument,frame.argument(3u));
    memory.store32(word0_store,frame.local(0u),source_value);
    const auto fourth_value=memory.load32(fourth_argument,frame.argument(4u));
    const auto target_value=memory.load32(target,frame.argument(1u));
    memory.store32(word1_store,frame.local(1u),second_value);
    frame.forward(target_value,frame.local(0u),third_argument_value,fourth_value);
}
} // namespace record_view
} // namespace halo2::native_record_view
