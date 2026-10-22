#include "halo2/source_live_shadow_storage_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);
    std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
    return value;
}
std::array<std::uint32_t,3> bindings(const SourceRuntimeMemory& memory) {
    return {word(memory,0x4c2aa8u),word(memory,0x4c2aacu),word(memory,0x52dfd0u)};
}
SourceLiveShadowStorageTu15 inspect(const SourceRuntimeMemory& memory) {
    SourceLiveShadowStorageTu15 result;
    result.bsp=word(memory,0x4ecc18u);
    if(!result.bsp)throw std::invalid_argument("TU1.5 shadow storage has no activated source BSP");
    result.bsp_native=memory.native_address(result.bsp,0x160);
    result.plane_count=word(memory,result.bsp+0x158u);
    result.plane_rows=word(memory,result.bsp+0x15cu);
    if(std::bit_cast<std::int32_t>(result.plane_count)<0)
        throw std::invalid_argument("TU1.5 shadow BSP plane count is invalid");
    if(result.plane_count) {
        const auto bytes=std::uint64_t(result.plane_count)*0x14u;
        if(!result.plane_rows || bytes>std::numeric_limits<std::uint32_t>::max() ||
           std::uint64_t(result.plane_rows)+bytes>0x100000000ull)
            throw std::invalid_argument("TU1.5 shadow BSP plane span wraps its source bank");
        result.plane_rows_native=memory.native_address(result.plane_rows,std::size_t(bytes));
    }
    const auto storage=bindings(memory);
    result.plane_storage=storage[0];result.index_storage=storage[1];result.hash=storage[2];
    if(std::ranges::any_of(storage,[](auto address){return !address;}))
        throw std::invalid_argument("TU1.5 shadow storage is only partially constructed");
    constexpr std::array<std::uint32_t,3> sizes{0x1c000u,0x800u,0x1203cu};
    for(unsigned n=0;n<storage.size();++n) {
        memory.validate_mutable_view(storage[n],sizes[n]);
        for(unsigned other=0;other<n;++other)
            if(std::uint64_t(storage[n])<std::uint64_t(storage[other])+sizes[other] &&
               std::uint64_t(storage[other])<std::uint64_t(storage[n])+sizes[n])
                throw std::invalid_argument("TU1.5 shadow storage allocations overlap");
    }
    result.allocator=word(memory,0x474cb8u);
    if(!result.allocator || word(memory,result.hash+0x20u)!=0x1000u ||
       word(memory,result.hash+0x24u)!=0x800u || word(memory,result.hash+0x28u)!=0x10u ||
       word(memory,result.hash+0x2cu)!=0x1e8a0u || word(memory,result.hash+0x30u)!=0x1e8d0u ||
       word(memory,result.hash+0x34u)!=result.allocator)
        throw std::invalid_argument("TU1.5 shadow plane hash definition or allocator differs");
    constexpr std::array<char,32> name{"transparent planes"};
    if(!std::ranges::equal(memory.view(result.hash,32),std::as_bytes(std::span(name))))
        throw std::invalid_argument("TU1.5 shadow plane hash name differs");
    result.plane_storage_native=memory.native_address(result.plane_storage,0x1c000);
    result.index_storage_native=memory.native_address(result.index_storage,0x800);
    result.hash_native=memory.native_address(result.hash,0x1203c);
    result.buckets_native=memory.native_address(result.hash+0x3cu,0x4000);
    result.nodes_native=memory.native_address(result.hash+0x403cu,0xe000);
    for(const auto address:std::array{result.bsp_native,result.plane_storage_native,
            result.index_storage_native,result.hash_native,result.buckets_native,result.nodes_native})
        if(address.value<=0xffffffffu)
            throw std::invalid_argument("TU1.5 shadow storage lacks native64 backing");
    if(result.plane_count && result.plane_rows_native.value<=0xffffffffu)
        throw std::invalid_argument("TU1.5 shadow BSP planes lack native64 backing");
    return result;
}
}
SourceLiveShadowStorageTu15 source_live_shadow_storage_activate_tu15(PhysicsLiveWorldTu15& world) {
    auto& memory=world.memory();
    const auto storage=bindings(memory);
    const bool cold=std::ranges::all_of(storage,[](auto address){return !address;});
    if(!cold && std::ranges::any_of(storage,[](auto address){return !address;}))
        throw std::invalid_argument("TU1.5 shadow storage is only partially constructed");
    if(!word(memory,0x4ecc18u))
        throw std::invalid_argument("TU1.5 shadow storage activation requires its actual source BSP");
    if(cold && !(world.call_actual(0x1e0f0u)&255u))
        throw std::runtime_error("TU1.5 shadow storage constructor failed");
    const auto retained=inspect(memory);
    //1E0F0's140C80 constructor already executes140CF0. Existing storage
    //takes the actual BSP-deactivation reset before the new BSP insertion.
    if(!cold)world.call_actual(0x22690u);
    world.call_actual(0x1e1a0u);
    const auto activated=inspect(memory);
    if(activated!=retained)
        throw std::runtime_error("TU1.5 shadow activation changed its retained source or native ownership");
    return activated;
}
void source_live_shadow_storage_validate_tu15(PhysicsLiveWorldTu15& world,
    const SourceLiveShadowStorageTu15& retained) {
    if(inspect(world.memory())!=retained)
        throw std::invalid_argument("TU1.5 shadow storage no longer belongs to its retained BSP activation");
}
SourceLiveShadowStorageTu15 source_live_shadow_storage_rebind_tu15(PhysicsLiveWorldTu15& world,
    const SourceLiveShadowStorageTu15& retained) {
    const auto rebound=inspect(world.memory());
    if(rebound.bsp!=retained.bsp || rebound.plane_rows!=retained.plane_rows ||
       rebound.plane_count!=retained.plane_count || rebound.plane_storage!=retained.plane_storage ||
       rebound.index_storage!=retained.index_storage || rebound.hash!=retained.hash ||
       rebound.allocator!=retained.allocator)
        throw std::invalid_argument("TU1.5 shadow storage rebind changed its retained source activation");
    return rebound;
}
}
