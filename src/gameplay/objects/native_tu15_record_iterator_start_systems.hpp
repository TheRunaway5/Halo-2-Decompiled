#pragma once
// Native TU1.5 record iterator start, entered at 1D3F90 (sandbox special
// unit): a keyed record iterator (1E5250) set up from a source and stepped
// to its first record.
//
//   record_iterator_start::start   1D3F90(EAX = source, ECX = iterator)
//
// The iterator's two keys (+0xC, +0x10) come from the source's +0x70 and
// +0x74; the source's tag (+0x68, through the tag instances [4F0414]) is
// the definition 1E5250 walks, with EAX the source and ESI the iterator.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_iterator_start.hpp) maps the role to the original
// instruction and owns the ABI of 1E5250.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_iterator_start {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u;}   // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t tag_instance_shift=4u,instance_address=8u;
namespace source {inline constexpr std::uint32_t tag=0x68u,first_key=0x70u,second_key=0x74u;}
namespace iterator {inline constexpr std::uint32_t first_key=0xcu,second_key=0x10u;}

namespace record_iterator_start {
enum class Access {instances,tag,definition,first_read,first_store,second_read,second_store};
// Frame: step(definition) 1E5250 (EBX the definition).
template<class Memory,class Frame> void start(Memory &memory,Frame &frame,std::uint32_t source_address,std::uint32_t iterator_address) {
    using enum Access;
    const auto instance_table=memory.load32(instances,globals::tag_instances);
    const auto tag_handle=memory.load32(tag,at(source_address,source::tag));
    const auto definition_address=memory.load32(definition,((tag_handle&0xffffu)<<tag_instance_shift)+instance_table+instance_address);
    memory.store32(first_store,at(iterator_address,iterator::first_key),memory.load32(first_read,at(source_address,source::first_key)));
    memory.store32(second_store,at(iterator_address,iterator::second_key),memory.load32(second_read,at(source_address,source::second_key)));
    frame.step(definition_address);
}
} // namespace record_iterator_start
} // namespace halo2::native_record_iterator_start
