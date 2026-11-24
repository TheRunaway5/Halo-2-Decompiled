#pragma once
// Native TU1.5 geometry cache entry check, entered at 130D20 (geometry cache
// frame, model render, map world support, shadow frame and physics live
// support units): whether a cached geometry entry can be used.
//
//   cache_entry::usable   130D20(entry handle) -> AL
//
// The entry (12 bytes of the array at 4F2D5C) has a record (24 bytes of the
// cache's record array: [4F2D6C]+0x64) stamped with the frame it was last
// used (+0x14). An entry used this frame (the cache's frame, +0x38), or one
// not resident (its byte +2 is 0), can be used. One last used five or more
// frames ago cannot. Otherwise, when its owner (+8) is ready (byte +0x1E),
// 22A790 is asked with the record's block (+8 shifted by the cache's block
// shift +0x34, past the blocks at 4F2D64); else it cannot.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_cache_entry.hpp) maps the role to the original instruction
// and owns the ABI of the child 22A790.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_cache_entry {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t entries=0x4f2d5cu;   // DataArray* of 12-byte entries
inline constexpr std::uint32_t cache=0x4f2d6cu;     // +0x34 block shift, +0x38 frame, +0x64 records
inline constexpr std::uint32_t blocks=0x4f2d64u;
}
inline constexpr std::uint32_t datum_data=0x44u,entry_size=12u,record_size=24u,stale_frames=5u;
namespace entry {inline constexpr std::uint32_t resident=2u,owner=8u;}
namespace record {inline constexpr std::uint32_t block=8u,stamp=0x14u;}
namespace cache {inline constexpr std::uint32_t block_shift=0x34u,frame=0x38u,records=0x64u;}
inline constexpr std::uint32_t owner_ready=0x1eu;

namespace cache_entry {
enum class Local {handle};
enum class Access {handle_argument,entries,entry_data,cache,records,record_data,stamp,cache_frame,resident,owner,ready,
    block,shift,blocks,owner_again};
enum class Exit {current,not_resident,stale,not_ready,asked};
// Frame:
//   local(Local)        the argument's address now
//   ask(block)          22A790(block; ESI the owner); returns AL
template<class Memory,class Frame> Exit usable(Memory &memory,Frame &frame) {
    using enum Access;
    const auto handle=memory.load32(handle_argument,frame.local(Local::handle));
    const auto entries_array=memory.load32(entries,globals::entries);
    const auto entry_address=memory.load32(entry_data,at(entries_array,datum_data))+(handle&0xffffu)*entry_size;
    const auto cache_address=memory.load32(cache,globals::cache);
    const auto records_array=memory.load32(records,at(cache_address,cache::records));
    const auto record_address=memory.load32(record_data,at(records_array,datum_data))+(handle&0xffffu)*record_size;
    const auto stamp_value=memory.load32(stamp,at(record_address,record::stamp));
    const auto age=memory.load32(cache_frame,at(cache_address,cache::frame))-stamp_value;
    if(age==0)return Exit::current;
    if(!memory.load8(resident,at(entry_address,entry::resident)))return Exit::not_resident;
    if(!(std::int32_t(age)<std::int32_t(stale_frames)))return Exit::stale;
    const auto owner_address=memory.load32(owner,at(entry_address,entry::owner));
    if(!memory.load8(ready,at(owner_address,owner_ready)))return Exit::not_ready;
    const auto block_index=memory.load32(block,at(record_address,record::block));
    const auto shift_value=memory.load32(shift,at(cache_address,cache::block_shift));
    const auto blocks_address=memory.load32(blocks,globals::blocks);
    (void)memory.load32(owner_again,at(entry_address,entry::owner));
    frame.ask((block_index<<(shift_value&31u))+blocks_address);
    return Exit::asked;
}
} // namespace cache_entry
} // namespace halo2::native_cache_entry
