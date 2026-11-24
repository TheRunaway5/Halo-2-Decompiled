#pragma once
// Native TU1.5 memory copy, entered at 2E2E30 (scene device constructor
// unit): the runtime's memcpy.
//
//   memory_copy::copy   2E2E30(destination, source, bytes) (the caller pops)
//
// The bytes are copied as REP MOVSD then REP MOVSB copy them: the whole
// dwords first, then the remaining bytes, each step in the direction the
// string direction flag gives (backwards from the given addresses when it
// is set).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments on the stack; the
// boundary adapter (native_tu15_memory_copy.hpp) maps the role to the
// original instruction and owns the string instructions' registers.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_memory_copy {
namespace argument {inline constexpr std::uint32_t destination=0u,source=1u,bytes=2u;}

namespace memory_copy {
enum class Access {bytes_count,source,destination,word_read,word_write,byte_read,byte_write};
enum class Outcome {copied};
// Frame:
//   argument(k)       an argument's slot now
//   step(width)       the direction flag's step for that width
//   words_started(), bytes_started()   the string instructions' counts
template<class Memory,class Frame> Outcome copy(Memory &memory,Frame &frame) {
    using enum Access;
    const auto bytes=memory.load32(bytes_count,frame.argument(argument::bytes));
    auto from=memory.load32(source,frame.argument(argument::source));
    auto to=memory.load32(destination,frame.argument(argument::destination));
    frame.words_started();
    const auto word_step=frame.step(4u);
    for(std::uint32_t k=0;k<bytes>>2;++k,from+=word_step,to+=word_step)memory.store32(word_write,to,memory.load32(word_read,from));
    frame.bytes_started();
    const auto byte_step=frame.step(1u);
    for(std::uint32_t k=0;k<(bytes&3u);++k,from+=byte_step,to+=byte_step)memory.store8(byte_write,to,memory.load8(byte_read,from));
    return Outcome::copied;
}
} // namespace memory_copy
} // namespace halo2::native_memory_copy
