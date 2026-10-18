#include "halo2/native_tu15_renderer_startup_data.hpp"
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_renderer_startup_data(SourceRuntimeMemory& m) {
    // Check all exact owners before publishing any destination. In particular,
    // a foreign current-context word must not leave either old bank mounted.
    for(const auto& r:m.checkpoint().regions)
        for(const auto [address,bytes]:std::array<std::pair<std::uint32_t,std::uint32_t>,16>{{
                {0x51df44u,4u},{0x556e18u,0x55c0u},{0x4cd3a0u,4u},{0x492348u,0x1cu},
                {0x4ce3f4u,4u},{0x4922a0u,4u},{0x474c04u,1u},{0x44a7ccu,72u},{0x1404f8u,20u},{0x474c94u,4u},
                {0x474c8cu,4u},{0x44ab6cu,12u},{0x44ab4cu,12u},
                {0x46a460u,4u},{0x46a79cu,4u},{0x46a7a0u,4u}}})
            if(std::uint64_t(r.address)<std::uint64_t(address)+bytes&&
               std::uint64_t(r.address)+r.bytes.size()>address)
                throw std::invalid_argument("Renderer constructor data conflicts with its exact owner");
    m.mount(0x51df44,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source renderer raw storage constructor binding");
    m.mount(0x556e18,std::vector<std::byte>(0x55c0),SourceRuntimeMemory::Access::ReadWrite,"source two renderer descriptor constructor destinations");
    // Exact writable loader-zero word. Actual13729/134C42 and existing
    // submitting-call adapters publish the real selected context later.
    m.mount(0x4cd3a0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold current renderer context pointer");
    // Whole1BE90 owns these seven scalar outputs from the actual viewport
    // and optional51DF50 auxiliary input. Its real SSE body computes them.
    m.mount(0x492348,std::array<std::byte,0x1c>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold seven viewport coordinate scalar outputs");
    // Actual3D2BF/3D35F write this DWORD;3D3D4 reads its low16bits.
    // Cold storage does not assert that per-view FP preparation has run.
    m.mount(0x4ce3f4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold first-person model count destination");
    // Actual22589/22590 publishes the matg Rasterizer Data pointer here.
    // Cold availability is separate from executing that new-map producer.
    m.mount(0x4922a0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold rasterizer globals tag pointer destination");
    // Authenticated raw .data byte01 enables the genuine115940 predicate.
    // Preserve its writable one-byte owner; do not replace the predicate.
    m.mount(0x474c04,std::array<std::byte,1>{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored115940 model animation predicate enable");
    // ActualD4F90 selects one default vector or the following five authored
    // vectors and advances by12. Preserve all original FLOAT bits, not hits.
    constexpr std::array<std::uint32_t,18> probe_words{
        0,0,0xc1200000u,0xc1200000u,0,0,0x41200000u,0,0,
        0,0xc1200000u,0,0,0x41200000u,0,0,0,0xc2200000u};
    std::array<std::byte,72> probes{};
    for(unsigned i=0;i<probe_words.size();++i)for(unsigned j=0;j<4;++j)
        probes[i*4+j]=std::byte(probe_words[i]>>(j*8));
    m.mount(0x44a7cc,probes,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored D4F90 one-plus-five probe vectors");
    // Actual1404D0 decrements then bounds the selector to0..4 before
    // reading this inline table. Targets stay original executable identities.
    constexpr std::array<std::uint32_t,5> selector_targets{
        0x1404e9u,0x1404e9u,0x1404e9u,0x1404ddu,0x1404ddu};
    std::array<std::byte,20> selector{};
    for(unsigned i=0;i<selector_targets.size();++i)for(unsigned j=0;j<4;++j)
        selector[i*4+j]=std::byte(selector_targets[i]>>(j*8));
    m.mount(0x1404f8,selector,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1404D0 five-target inline selector table");
    // Authenticated aliases are writable .data; targets are immutable .rdata.
    // D4350 reads each complete FLOAT3. No surrounding color bank is borrowed.
    const auto encode=[](auto words) {
        std::array<std::byte,std::tuple_size_v<decltype(words)>*4> bytes{};
        for(unsigned i=0;i<words.size();++i)for(unsigned j=0;j<4;++j)
            bytes[i*4+j]=std::byte(words[i]>>(j*8));
        return bytes;
    };
    m.mount(0x474c94,encode(std::array{0x44ab6cu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored D4350 first FLOAT3 alias");
    m.mount(0x474c8c,encode(std::array{0x44ab4cu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored D4350 second FLOAT3 alias");
    m.mount(0x44ab6c,encode(std::array{0x3f800000u,0x3ed1eb85u,0x3f333333u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored D4350 first FLOAT3");
    m.mount(0x44ab4c,encode(std::array{0x3f800000u,0x3f800000u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored D4350 second FLOAT3");
    // Direct MOVSS reads in actual D5474/D49D9/D49D1. Preserve the exact
    // authored .rdata scalars; no neighboring bytes or computed outputs.
    m.mount(0x46a460,encode(std::array{0xc47a0000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored D5300 lower sample-coordinate clamp operand");
    m.mount(0x46a79c,encode(std::array{0x3f170a3du}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored D4350 second weighted-color operand");
    m.mount(0x46a7a0,encode(std::array{0x3de147aeu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored D4350 third weighted-color operand");
}
}
