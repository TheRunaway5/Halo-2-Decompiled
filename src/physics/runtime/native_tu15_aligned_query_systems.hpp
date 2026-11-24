#pragma once
// Native TU1.5 aligned query, entered at 2E9750 (death cycle, AI frame and
// physics live support units): a query through a 16-byte-aligned frame
// record.
//
//   aligned_query::run   2E9750(ECX = source; first, second, third) -> RET C
//
// 2E96A0 sets up the frame's record (ECX its address); 30B890 then runs the
// query on it (ECX the record) with the source's +0xC and +0x10 and the three
// arguments.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_aligned_query.hpp) maps the role to the original instruction
// and owns the ABI of the children (the aligned frame among it); the
// source, which the original keeps in ESI across the first, is refreshed
// from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_aligned_query {
using native_object_update::at;
namespace source {inline constexpr std::uint32_t first=0xcu,second=0x10u;}

namespace aligned_query {
enum class Access {third,second,first,source_second,source_first};
// Frame:
//   argument(k)                    the k-th argument's slot (through EBP)
//   set_up() -> source             2E96A0 on the frame's record
//   run(first, second, third, source_first, source_second)   30B890
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto source_address=frame.set_up();
    const auto third_value=memory.load32(third,frame.argument(3u));
    const auto second_value=memory.load32(second,frame.argument(2u));
    const auto first_value=memory.load32(first,frame.argument(1u));
    const auto source_second_value=memory.load32(source_second,at(source_address,source::second));
    const auto source_first_value=memory.load32(source_first,at(source_address,source::first));
    frame.run(first_value,second_value,third_value,source_first_value,source_second_value);
}
} // namespace aligned_query
} // namespace halo2::native_aligned_query
