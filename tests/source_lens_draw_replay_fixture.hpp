#pragma once
#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_execution_lens.hpp"
#include "halo2/source_lens_program_tu15.hpp"

namespace halo2::test {
// Declared CPU device ABI from the real frame adapter, with an actual owned
// cache metadata row. This fixture does not create or claim a GPU resource.
struct SourceLensDrawReplayFixture final:SourcePhysicsContactMemory {
    SourceRuntimeMemory storage;
    SourceLensDeviceDomains domains;
    std::uint32_t metadata{},header=0xf7500000u,alias=0xf7600000u;
    void mount(std::uint32_t p,std::span<const std::byte> data) {
        storage.mount(p,data,SourceRuntimeMemory::Access::ReadWrite,"declared original lens device input");domains.emplace(p,data.size());
    }
    void put(std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes;
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));storage.write(p,bytes);}
    SourceLensDrawReplayFixture(PhysicsLiveWorldTu15 &world,const Cache &cache) {
        auto &bank=world.memory();
        mount(0x40eeb0u,bank.view(0x40eeb0u,0x411800u-0x40eeb0u));
        mount(0x491788u,std::as_bytes(std::span(source_lens_pixel_words)));
        mount(0x4c4b00u,bank.view(0x4c4b00u,0x200));
        mount(0x4ce1e0u,std::vector<std::byte>(0x48));mount(0x52df98u,std::vector<std::byte>(32));
        mount(0xf7200000u,std::vector<std::byte>(8192));mount(0xf7210000u,std::vector<std::byte>(0x200000));
        mount(header,std::vector<std::byte>(32));
        put(0x411568u,0xf7200000u);put(0x411570u,0);put(0xf7200000u,0xf7210000u);put(0xf7200004u,0xf7410000u);
        for(unsigned stage=0;stage<4;++stage){const std::array<std::uint32_t,6> sampler{1,1,1,2,2,2};
            storage.write(0x40eeb0u+stage*128,std::as_bytes(std::span(sampler)));put(0xf720000cu+stage*4,0x80000000u);}
        for(const auto &lens:cache.select_tags({.group=TagGroup::from_string("lens"),.require_address=true})){
            const auto bitmap=Bytes(cache.tag_payload(lens.handle())).u32(0x24);if(bitmap==0xffffffffu)continue;
            const auto *tag=cache.query_tag({.handle=TagHandle{bitmap},.group=tag_groups::bitmap,.require_address=true});if(!tag)continue;
            const Bytes root(cache.tag_payload(tag->handle()));if(!root.u32(0x44))continue;
            const auto row=root.u32(0x48);const auto data=cache.read_tag_bytes(row,0x74);if(Bytes(data).u16(0xa)!=0)continue;
            metadata=row;mount(metadata,data);put(metadata+0x40,alias);break;
        }
        if(!metadata)throw FormatError("Fixture cache lacks an actual 2D lens bitmap row");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override{storage.read(p,out);}
    void write(std::uint32_t p,std::span<const std::byte> in)override{storage.write(p,in);}
};
}
