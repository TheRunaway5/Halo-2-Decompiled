#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F action){try{action();}catch(const std::exception&){return;}throw std::runtime_error("Unowned native cache input was admitted");}
void word(std::span<std::byte> bytes,std::size_t at,std::uint32_t value){
    Bytes(bytes).require(at,4);for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));
}
Buffer integer(std::uint32_t value){Buffer result(4);word(result,0,value);return result;}
struct Fixture {
    static constexpr std::uint32_t base=0x90000000u,size=0x5000,metadata=base+0x3000;
    SourceRuntimeMemory memory;
    SourceRuntimeHostPhysicalMemory physical{memory};
    SourceRuntimeCacheTagImage tags;
    SourceRuntimeCacheBspImage bsp;
    explicit Fixture(bool native_pages=false) {
        memory.mount(0x4f2950u,integer(0x82d00000u),SourceRuntimeMemory::Access::ReadWrite,"test existing game arena binding");
        for(auto address:{0x556d90u,0x4f0414u,0x474c20u,0x4ecc20u,0x4ecc1cu})
            memory.mount(address,integer(0),SourceRuntimeMemory::Access::ReadWrite,"test vanilla cache publication field");
        if(native_pages)check(physical.allocate({size,base&0x7fffffffu,
            (base&0x7fffffffu)+size-1,4096,4})==base,"Native cache page allocation moved");
        else memory.mount(base,Buffer(size),SourceRuntimeMemory::Access::ReadWrite,"test native gameplay cache allocation");
        tags.index=base;tags.table=base+32;tags.count=3;
        tags.scenario=0xe0010000u;tags.globals=0xe0020001u;tags.reservation_bytes=size;
        tags.regions={{base,Buffer(80)},{metadata,Buffer(0x500)}};
        auto& index=tags.regions[0].bytes;
        word(index,8,tags.table);word(index,12,tags.scenario);word(index,16,tags.globals);
        word(index,24,tags.count);word(index,28,0x74616773u);
        for(const auto& [n,group,datum,address,bytes]:std::array{
            std::array{0u,0x73636e72u,tags.scenario,metadata,0x284u},
            std::array{1u,0x6d617467u,tags.globals,metadata+0x300u,0x100u},
            std::array{2u,0x73627370u,0xe0030002u,0u,0u}}) {
            const auto at=32+n*16;
            word(index,at,group);word(index,at+4,datum);word(index,at+8,address);word(index,at+12,bytes);
        }
        bsp.mapping=metadata+0x240u;bsp.index=0;bsp.file_offset=0x100000;
        bsp.bsp=0xe0030002u;bsp.lightmap=0xffffffffu;bsp.region={base+0x1000u,Buffer(0x400)};
        word(bsp.region.bytes,0,0x400);word(bsp.region.bytes,4,bsp.region.address+16);
        word(bsp.region.bytes,8,0xffffffffu);word(bsp.region.bytes,12,0x73627370u);
        word(bsp.mapping_bytes,0,bsp.file_offset);word(bsp.mapping_bytes,4,0x400);
        word(bsp.mapping_bytes,8,bsp.region.address);word(bsp.mapping_bytes,0x14,bsp.bsp);
        word(bsp.mapping_bytes,0x1c,bsp.lightmap);
        auto& root=tags.regions[1].bytes;word(root,0x210,1);word(root,0x214,bsp.mapping);
        std::copy(bsp.mapping_bytes.begin(),bsp.mapping_bytes.end(),root.begin()+0x240);
    }
    SourceRuntimeNativeCacheRange range(){return {memory.native_address(base,size),size};}
};
}
int main()try {
    Fixture owner,foreign;
    check(owner.range().allocation.value>UINT32_MAX,"Native cache allocation was narrowed");
    rejects([&]{source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags);});
    rejects([&]{source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags,foreign.range());});
    auto short_range=owner.range();short_range.bytes-=4096;
    rejects([&]{source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags,short_range);});
    auto escaped=owner.tags;escaped.regions[1].address=Fixture::base+Fixture::size;
    rejects([&]{source_bind_runtime_cache_tags_tu15(owner.memory,escaped,owner.range());});
    check(Bytes(owner.memory.view(0x556d90u,4)).u32(0)==0,"Failed cache admission published an index");
    source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags,owner.range());
    check(Bytes(owner.memory.view(0x556d90u,4)).u32(0)==Fixture::base,"Native cache lost the vanilla index binding");
    rejects([&]{source_deliver_runtime_cache_bsp_tu15(owner.memory,owner.bsp);});
    rejects([&]{source_deliver_runtime_cache_bsp_tu15(owner.memory,owner.bsp,foreign.range());});
    auto bad_bsp=owner.bsp;bad_bsp.bsp^=0x10000u;
    rejects([&]{source_deliver_runtime_cache_bsp_tu15(owner.memory,bad_bsp,owner.range());});
    source_deliver_runtime_cache_bsp_tu15(owner.memory,owner.bsp,owner.range());
    check(std::ranges::equal(owner.memory.view(owner.bsp.region.address,owner.bsp.region.bytes.size()),
        owner.bsp.region.bytes),"Native BSP delivery changed resource bytes");
    const auto before=owner.range();SourceRuntimeMemory restored;restored.restore(owner.memory.checkpoint());
    owner.memory.swap_backing(restored);
    rejects([&]{source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags,before);});
    source_bind_runtime_cache_tags_tu15(owner.memory,owner.tags,owner.range());
    source_deliver_runtime_cache_bsp_tu15(owner.memory,owner.bsp,owner.range());
    Fixture protected_tags(true);
    source_bind_runtime_cache_tags_tu15(protected_tags.memory,protected_tags.tags,protected_tags.range());
    // Original11EF70 protects static metadata before1255C0 publishes BSP
    // resources into the separate writable portion of this same allocation.
    protected_tags.physical.protect(Fixture::metadata,4096,2);
    source_deliver_runtime_cache_bsp_tu15(protected_tags.memory,protected_tags.bsp,protected_tags.range());
    check(std::ranges::equal(protected_tags.memory.view(protected_tags.bsp.region.address,
        protected_tags.bsp.region.bytes.size()),protected_tags.bsp.region.bytes),"Read-only static tags prevented writable BSP delivery");
    protected_tags.physical.protect(protected_tags.bsp.region.address,4096,2);
    rejects([&]{source_deliver_runtime_cache_bsp_tu15(protected_tags.memory,protected_tags.bsp,protected_tags.range());});
    std::cout<<"PASS native cache ownership, bounds, vanilla publication, BSP identity, page protections and checkpoint rebinding\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
