#pragma once
// Native TU1.5 resource acquire, entered at 1262F0 (AI frame and physics
// live support units): an animation resource's data in the resource cache,
// loading it when it has no block yet.
//
//   resource_acquire::address   1262F0(EAX = resource; argument) -> EAX, RET 4
//
// A request the record search (126190) still finds for the resource is
// released (16E8A0, EDX the request pool at [4F041C]). A resource without a
// block (+0xC NONE) gets one (125FB0: the resource, 1) and a new record of
// the record pool at [4F0418] (16E770, EDX the pool; 16-byte records: +4 a
// byte cleared, +8 NONE, +0xC the resource), and 1260C0 loads it (the
// argument, 0). A resource with a block touches the record of that index
// (1262B0, EDI the record). Either way the block (24-byte blocks of the
// cache at [4F0424], +0x64) takes the cache's stamp (+0x38) at +0x14, and
// the answer is the cache's data ([4F0420]) plus the block's offset (+8)
// shifted by the cache's shift (+0x34).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the argument it reads; the boundary
// adapter (native_tu15_resource_acquire.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original keeps
// in callee-saved registers across them are a Kept, refreshed from the
// registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_resource_acquire {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {
inline constexpr std::uint32_t records=0x4f0418u;     // DataArray* of 16-byte records
inline constexpr std::uint32_t requests=0x4f041cu;    // DataArray* of requests
inline constexpr std::uint32_t cache_data=0x4f0420u;  // the cache's data
inline constexpr std::uint32_t cache=0x4f0424u;       // the cache
}
inline constexpr std::uint32_t datum_data=0x44u,record_size=16u,block_size=24u;
namespace cache {inline constexpr std::uint32_t shift=0x34u,stamp=0x38u,blocks=0x64u;}
namespace record {inline constexpr std::uint32_t loaded=4u,block=8u,resource=0xcu;}
namespace block {inline constexpr std::uint32_t offset=8u,stamp=0x14u;}
namespace resource {inline constexpr std::uint32_t block=0xcu;}

namespace resource_acquire {
enum class Access {requests,block,records,record_data,argument,record_block,record_loaded,record_resource,block_store,
    records_again,records_data,block_again,cache,blocks,stamp,blocks_data,stamp_store,block_reload,blocks_again,blocks_data_again,
    shift,offset,base};
enum class Outcome {loaded,touched};
// The original's ESI and EDI across the children: the resource and the
// block (or the touched record).
struct Kept {std::uint32_t resource,block;};
// A new record: its handle and the pool EDX holds after 16E770.
struct Created {std::uint32_t handle,pool;};
// Frame:
//   argument()                         the argument's slot now
//   find(kept) -> handle               126190(EBX the resource)
//   release(requests, kept)            16E8A0(EAX the request, EDX the pool)
//   choose(kept) -> block              125FB0(the resource, 1 pushed)
//   create(records, kept) -> Created   16E770(EDX the pool)
//   pushing(value)                     a value pushed for 1260C0
//   load_resource(kept)                1260C0
//   touch(record, kept)                1262B0(EDI the record)
//   answer(shifted, base) -> address   the saved registers back, then ADD EAX,ECX
// Every child refreshes kept.
template<class Memory,class Frame> std::uint32_t address(Memory &memory,Frame &frame,std::uint32_t resource_address,Outcome &outcome) {
    using enum Access;
    Kept kept{resource_address,0u};
    if(frame.find(kept)!=none)frame.release(memory.load32(requests,globals::requests),kept);
    kept.block=memory.load32(block,at(kept.resource,resource::block));
    if(kept.block==none) {
        kept.block=frame.choose(kept);
        const auto created=frame.create(memory.load32(records,globals::records),kept);
        const auto data=memory.load32(record_data,at(created.pool,datum_data));
        const auto argument_value=memory.load32(argument,frame.argument());
        const auto record_address=record_size*(created.handle&0xffffu)+data;
        frame.pushing(0u);
        memory.store32(record_block,at(record_address,record::block),none);
        memory.store8(record_loaded,at(record_address,record::loaded),0u);
        memory.store32(record_resource,at(record_address,record::resource),kept.resource);
        frame.pushing(argument_value);
        memory.store32(block_store,at(kept.resource,resource::block),kept.block);
        frame.load_resource(kept);
        outcome=Outcome::loaded;
    } else {
        const auto data=memory.load32(records_data,at(memory.load32(records_again,globals::records),datum_data));
        frame.touch(record_size*(kept.block&0xffffu)+data,kept);
        outcome=Outcome::touched;
    }
    // The block takes the cache's stamp; the answer is its data.
    const auto block_handle=memory.load32(block_again,at(kept.resource,resource::block));
    const auto cache_address=memory.load32(Access::cache,globals::cache);
    const auto blocks_address=memory.load32(blocks,at(cache_address,cache::blocks));
    const auto stamp_value=memory.load32(stamp,at(cache_address,cache::stamp));
    memory.store32(stamp_store,memory.load32(blocks_data,at(blocks_address,datum_data))+block_size*(block_handle&0xffffu)+block::stamp,stamp_value);
    const auto handle=memory.load32(block_reload,at(kept.resource,resource::block));
    const auto blocks_base=memory.load32(blocks_data_again,at(memory.load32(blocks_again,at(cache_address,cache::blocks)),datum_data));
    const auto shift_value=memory.load32(shift,at(cache_address,cache::shift));
    const auto offset_value=memory.load32(offset,blocks_base+block_size*(handle&0xffffu)+block::offset);
    const auto shifted=offset_value<<(shift_value&31u);
    return frame.answer(shifted,memory.load32(base,globals::cache_data));
}
} // namespace resource_acquire
} // namespace halo2::native_resource_acquire
