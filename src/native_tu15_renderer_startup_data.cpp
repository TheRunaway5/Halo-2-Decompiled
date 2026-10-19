#include "halo2/native_tu15_renderer_startup_data.hpp"
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_renderer_startup_data(SourceRuntimeMemory& m) {
    // Check all exact owners before publishing any destination. In particular,
    // a foreign current-context word must not leave either old bank mounted.
    for(const auto& r:m.checkpoint().regions)
        for(const auto [address,bytes]:std::array<std::pair<std::uint32_t,std::uint32_t>,110>{{
                {0x51df44u,4u},{0x556e18u,0x55c0u},{0x4cd3a0u,4u},{0x492348u,0x1cu},
                {0x4ce3f4u,4u},{0x4922a0u,4u},{0x474c04u,1u},{0x44a7ccu,72u},{0x1404f8u,20u},{0x474c94u,4u},
                {0x474c8cu,4u},{0x44ab6cu,12u},{0x44ab4cu,12u},
                {0x46a460u,4u},{0x46a79cu,4u},{0x46a7a0u,4u},{0x138be8u,4u},{0x45f2a6u,2u},{0x1d588u,4u},{0x1d5a7u,1u},
                {0x40db3fu,1u},{0x45c4c8u,4u},{0x45f2b0u,2u},{0x46a24cu,4u},{0x46a258u,4u},{0x46a248u,4u},{0x46a7a4u,4u},{0x46a1ccu,4u},
                {0x46a1c8u,4u},{0x46a1c4u,4u},{0x46a798u,4u},{0x4c6958u,850u*32u},{0x4cd3a4u,850u},
                {0x4cd6f6u,850u},{0x4cda48u,192u*4u},{0x4cdd4cu,32u*36u},{0x4cd398u,4u},{0x4cd39cu,4u},
                {0x4cdd48u,4u},{0x4ce1ccu,4u},{0x41c4e8u,4u},{0x2e6dfcu,92u},{0x2e6e58u,108u},
                {0x30987cu,80u},{0x3098ccu,108u},{0x309538u,88u},{0x309590u,108u},{0x515f18u,4u},{0x5332c8u,4u*0x484u},{0x474d0cu,4u},{0x474d14u,4u},{0x44ac5cu,12u},{0x46a45cu,4u},{0x46a4c0u,4u},{0x48c530u,0x88u},{0x48c5b8u,0xe4u},{0x4920dcu,1u},{0x4c6838u,1u},{0x4c6834u,4u},{0x4d1828u,4u},{0x492250u,4u},{0x4920e4u,4u},{0x4ecbd4u,1u},{0x46a218u,4u},{0x46a638u,4u},{0x449ea4u,4u},{0x46a26cu,4u},{0x46a498u,4u},{0x46a8dcu,4u},{0x2e78d8u,92u},{0x2e7934u,108u},{0x15e74u,24u},{0x15e8cu,11u},{0x46a6e4u,4u},{0x46a6e0u,4u},{0x44a8a0u,4u},{0x46aa30u,4u},{0x4d044u,20u},{0x4d058u,17u},{0x4d06cu,20u},{0x4d080u,20u},{0x1cb40u,16u},{0x4739c4u,1u},{0x492398u,20u},{0x46a104u,4u},{0x492295u,1u},{0x44a498u,4u},{0x5564c4u,8u},{0xf0d28u,31u},{0x16957bu,164u},{0xcc30cu,64u},{0x46a4bcu,4u},{0x46a4c8u,8u},{0xe9ba8u,12u},{0xe9bb6u,60u},{0xe9bf4u,72u},{0xe9c3eu,60u},{0x478ea8u,1024u},{0x48c7a4u,2u},{0x48c7a6u,1u},{0x48c7a7u,1u},{0x19b9f8u,164u},{0x41c4ccu,4u},{0x100d04u,28u},{0x100d20u,28u},{0x46a7d4u,4u},{0x18aefcu,32u},{0x2c2b30u,16u},{0x56e0e8u,4u},{0x46a784u,4u}}})
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
    // Exact selector cell read by genuine138C20 for authored format11.
    // Preserve its original decoder target identity, not executable code.
    m.mount(0x138be8,encode(std::array{0x13887cu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored138C20 format11 direct bitmap decoder selector");

    // Exact signed16 bit-depth cell read by genuine138510 at138544 for
    // format11 direct sampling; no neighboring table bytes are mounted.
    m.mount(0x45f2a6,std::array<std::byte,2>{std::byte{0x20},std::byte{0}},
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored138510 format11 bitmap bit depth");

    // Exact actual F7E500C9 complete1D2D0/1D490 constructor operands.
    // These embedded branch cells keep their original identities; no code
    // body, surrounding table or constructed bitmap row is mounted here.
    m.mount(0x1d588,encode(std::array{0x1d512u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1D490 bitmap format15 constructor target");
    m.mount(0x1d5a7,std::array<std::byte,1>{std::byte{0x0a}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1D490 bitmap format15 constructor selector");
    // D3D section flags7 require RW ownership; the actual cold flag is08.
    // The existing complete407C00 cache-view owner consumes this byte after
    // genuine405770 returns; this is not an invented hardware-ready value.
    m.mount(0x40db3f,std::array<std::byte,1>{std::byte{0x08}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored407C00 format15 cache bitmap flag");
    m.mount(0x45c4c8,encode(std::array{0x0fu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1301E0 bitmap format16 hardware format");
    m.mount(0x45f2b0,std::array<std::byte,2>{std::byte{0x08},std::byte{0}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored138510 bitmap format16 block bit depth");

    // Whole authenticatedD5700 has no children and five direct RO operands.
    // 46A0D0/46A25C already have exact owners; add only the three missing
    // scalar cells, preserving every original FLOAT bit and all table gaps.
    m.mount(0x46a24c,encode(std::array{0x3a001002u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5700 scalar operand46A24C");
    m.mount(0x46a258,encode(std::array{0x35000000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5700 scalar operand46A258");
    m.mount(0x46a248,encode(std::array{0x3a802008u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5700 scalar operand46A248");

    // Whole actualD5D30 has15 direct RO scalars; the other10 and its
    // authored up-vector alias/target are already retained. Add only these
    // five absent exact cells. Animation/LUT descendants stay independent.
    m.mount(0x46a7a4,encode(std::array{0xbf400000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5D30 scalar operand46A7A4");
    m.mount(0x46a1cc,encode(std::array{0x3de978d5u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5D30 scalar operand46A1CC");
    m.mount(0x46a1c8,encode(std::array{0x3f1645a2u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5D30 scalar operand46A1C8");
    m.mount(0x46a1c4,encode(std::array{0x3e991687u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5D30 scalar operand46A1C4");
    m.mount(0x46a798,encode(std::array{0x3fb33333u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredD5D30 scalar operand46A798");

    // Existing SourceDrawUnitState and literal2CBD0 producers own these five
    // exact typed arrays. All bytes are authentic TU1.5 loader-zeroed .data
    // virtual tail, not constructed draw/shadow output or a ready flag.
    // Leave the four runtime counts and existing4CD3A0 context owner alone.
    m.mount(0x4c6958,std::vector<std::byte>(850u*32u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw units850x32");
    m.mount(0x4cd3a4,std::vector<std::byte>(850u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw coverage850");
    m.mount(0x4cd6f6,std::vector<std::byte>(850u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw override slots850");
    m.mount(0x4cda48,std::vector<std::byte>(192u*4u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw overrides192x4");
    m.mount(0x4cdd4c,std::vector<std::byte>(32u*36u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source transparent sections32x36");

    // Complete the same authenticated nine-bank loader-zero BSS shape.
    // These count destinations are not initialized draw results: genuine
    // 2CB10/2CBD0 and the retained main handoff publish their actual values.
    // Existing4CD3A0 current-context ownership remains a separate word.
    m.mount(0x4cd398,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw count");
    m.mount(0x4cd39c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source main draw prefix");
    m.mount(0x4cdd48,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source draw override count");
    m.mount(0x4ce1cc,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed source transparent section highwater");

    // Whole authenticated2E79A0 consumes only three literal RO operands.
    // Its one/absolute-value SIMD mask already have exact owners; add only
    // this missing FMUL16777216 scalar from original executable .rdata.
    m.mount(0x41c4e8,encode(std::array{0x4b800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2E79A0 collision quantization scalar");

    // Exact bounded byte-selector and target tables for the reached original
    // collision interpreter and its two native recursive children. Source
    // guards bound input bytes0..6B; both table reads remain genuine calls.
    // Targets are original code identities only; no executable body/gap is
    // mounted and no selector behavior or callback return is substituted.
    m.mount(0x2e6dfc,encode(std::array{0x2e6df0u,0x2e6b49u,0x2e6b07u,0x2e6b14u,0x2e6b2au,0x2e6b9du,0x2e6be2u,0x2e6c30u,0x2e6301u,0x2e62ceu,0x2e6365u,0x2e63c5u,0x2e6975u,0x2e6919u,0x2e6d82u,0x2e6d34u,0x2e6d3au,0x2e6d49u,0x2e6d61u,0x2e6c90u,0x2e6ca0u,0x2e6cb9u,0x2e6dbau}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2E6290 collision branch identities");
    m.mount(0x2e6e58,std::array<std::byte,108>{
        std::byte{0x00},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04},std::byte{0x16},std::byte{0x05},std::byte{0x06},std::byte{0x07},
        std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},
        std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x0a},std::byte{0x0a},std::byte{0x0a},std::byte{0x0b},
        std::byte{0x0b},std::byte{0x0b},std::byte{0x0c},std::byte{0x0c},std::byte{0x0c},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},
        std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},
        std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},
        std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0f},std::byte{0x10},std::byte{0x11},std::byte{0x12},
        std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},
        std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x14},std::byte{0x14},std::byte{0x14},std::byte{0x14},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15}
    },SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2E6290 collision byte selectors");
    m.mount(0x30987c,encode(std::array{0x3097e7u,0x30962eu,0x309654u,0x309661u,0x309677u,0x309696u,0x3096bfu,0x3096f1u,0x3096e9u,0x309636u,0x30964cu,0x30983fu,0x3097f0u,0x3097f6u,0x309805u,0x30981du,0x309732u,0x309742u,0x30975bu,0x3097b1u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored309600 collision branch identities");
    m.mount(0x3098cc,std::array<std::byte,108>{
        std::byte{0x00},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04},std::byte{0x13},std::byte{0x05},std::byte{0x06},std::byte{0x07},
        std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},
        std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x09},
        std::byte{0x09},std::byte{0x09},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x0a},std::byte{0x0a},std::byte{0x0a},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},
        std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},
        std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},
        std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0c},std::byte{0x0d},std::byte{0x0e},std::byte{0x0f},
        std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},
        std::byte{0x10},std::byte{0x10},std::byte{0x10},std::byte{0x10},std::byte{0x11},std::byte{0x11},std::byte{0x11},std::byte{0x11},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12}
    },SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored309600 collision byte selectors");
    m.mount(0x309538,encode(std::array{0x3094a1u,0x309348u,0x309306u,0x309313u,0x309329u,0x309350u,0x309379u,0x3093abu,0x3092ceu,0x3092dbu,0x3092e8u,0x3093a3u,0x3092feu,0x3094f9u,0x3094aau,0x3094b0u,0x3094bfu,0x3094d7u,0x3093ecu,0x3093fcu,0x309415u,0x30946bu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3092A0 collision branch identities");
    m.mount(0x309590,std::array<std::byte,108>{
        std::byte{0x00},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04},std::byte{0x15},std::byte{0x05},std::byte{0x06},std::byte{0x07},
        std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},
        std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x0a},
        std::byte{0x0a},std::byte{0x0a},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0c},std::byte{0x0c},std::byte{0x0c},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},
        std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},
        std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},
        std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x0e},std::byte{0x0f},std::byte{0x10},std::byte{0x11},
        std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15},
        std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x14},std::byte{0x14},std::byte{0x14},std::byte{0x14}
    },SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3092A0 collision byte selectors");

    // Authentic loader-zero .data virtual-tail signed count consumed by
    // whole41630. Existing3F2A0 owns its real reset; this mount provides only
    // the original cold destination, never a pointer/ready/resource result.
    m.mount(0x515f18,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed41630 renderer record count");

    // Complete original four-view shadow-selection history storage.
    // Each view owns32 original36-byte entries and its4-byte live count;
    // preserve loader-zero bytes, leaving genuine resets/selection to run.
    m.mount(0x5332c8,std::vector<std::byte>(4u*0x484u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed four-view shadow selection history4x1156");

    // Whole original32340 selects these two authored FLOAT3 bases.
    // The first target44AC34 is already inside the original identity owner;
    // retain it and add only missing aliases, second vector and RO scalars.
    m.mount(0x474d0c,encode(std::array{0x44ac34u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored32340 first shadow basis binding");
    m.mount(0x474d14,encode(std::array{0x44ac5cu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored32340 fallback shadow basis binding");
    m.mount(0x44ac5c,encode(std::array{0xbf800000u,0u,0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored32340 fallback shadow FLOAT3 basis");
    m.mount(0x46a45c,encode(std::array{0x3951b717u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored32340 minimum shadow extent operand");
    m.mount(0x46a4c0,encode(std::array{0x3db851ecu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored32340 minimum shadow opacity operand");

    // Whole original32340 passes this complete authored render-light record
    // to4A1D0 and its ligh definition toC3960/C3AB0. Retain every original
    // initialized .data byte, including genuine colors/scales/NONE fields.
    // Later original/caller stores produce current center/basis/extents.
    m.mount(0x48c530,encode(std::array{0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0xbf800000u,0x0u,0x3f800000u,0x0u,0x3f800000u,0x0u,0x3f800000u,0x3f800000u,0x3f666666u,0x3f666666u,0x3f4ccccdu,0x0u,0x0u,0x0u,0x40000000u,0x0u,0x0u,0x3f800000u,0x3f800000u,0x3f800000u,0x0u,0x0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored32340 complete render-light record136");
    m.mount(0x48c5b8,encode(std::array{0x1000au,0x2u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x40000000u,0x3f800000u,0x3bb60b61u,0x3fc00000u,0x40000000u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0xffffffffu,0xffffffffu,0x1u,0x3f800000u,0x0u,0x10000u,0xffffffffu,0xffffffffu,0x0u,0xffffffffu,0xffffffffu,0x0u,0x0u,0x3f800000u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u,0x0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored32340 complete ligh definition228");
    // Exact loader-zero publication destinations of genuine32340 and4A1D0.
    // Keep neighboring view state and all source producer logic separate.
    m.mount(0x4920dc,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed32340 secondary-query mode destination");
    m.mount(0x4c6838,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed32340 shadow phase-active destination");
    m.mount(0x4c6834,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed32340 shadow feature-mask destination");
    m.mount(0x4d1828,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed32340 active shadow definition destination");
    m.mount(0x492250,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed32340 shadow opacity destination");
    m.mount(0x4920e4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed4A1D0 active light definition destination");

    // Genuine134C00 writes this one-byte phase flag at134CD2 and clears it
    // at134CE8. Preserve only the authentic loader-zero BSS destination;
    // adjacent pool bindings and live generation fields retain their owners.
    m.mount(0x4ecbd4,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed134C00 scoped update flag");

    // Whole136030's three missing authored scalar operands. All other
    // direct operands and135E00 child operands already retain exact owners.
    // Preserve original RO bits and leave current query outputs to execute.
    m.mount(0x46a218,encode(std::array{0x3e051eb8u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored136030 shadow query scalar46A218");
    m.mount(0x46a638,encode(std::array{0x41c80000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored136030 shadow query scalar46A638");
    m.mount(0x449ea4,encode(std::array{0x3e124925u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored136030 shadow query scalar449EA4");
    // Additional exact numeric operands in the retained2CBA0 call closure;
    // these are original static bits, not output coverage/fade values.
    m.mount(0x46a26c,encode(std::array{0x43800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2CBD0 shadow query scalar46A26C");
    m.mount(0x46a498,encode(std::array{0x3dc28f5cu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4BAE0 shadow query scalar46A498");
    m.mount(0x46a8dc,encode(std::array{0x3fd9999au}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4BCB0 shadow query scalar46A8DC");

    // Original2E6ED0 bounds the source selector to0..6B before both reads.
    // Keep exact23-target/108-selector typed data, without adjacent code.
    m.mount(0x2e78d8,encode(std::array{0x2e78ceu,0x2e7628u,0x2e75e6u,0x2e75f3u,0x2e7609u,0x2e767au,0x2e76bfu,0x2e770du,0x2e6f44u,0x2e6f0eu,0x2e6fa9u,0x2e700au,0x2e74c7u,0x2e746cu,0x2e7895u,0x2e7847u,0x2e784du,0x2e785cu,0x2e7874u,0x2e776du,0x2e777du,0x2e7796u,0x2e7808u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2E6ED0 collision branch targets23");
    m.mount(0x2e7934,std::array<std::byte,108>{std::byte{0x00},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04},std::byte{0x16},std::byte{0x05},std::byte{0x06},std::byte{0x07},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x08},std::byte{0x08},std::byte{0x08},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x09},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x0a},std::byte{0x0a},std::byte{0x0a},std::byte{0x0b},std::byte{0x0b},std::byte{0x0b},std::byte{0x0c},std::byte{0x0c},std::byte{0x0c},std::byte{0x0d},std::byte{0x0d},std::byte{0x0d},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0e},std::byte{0x0f},std::byte{0x10},std::byte{0x11},std::byte{0x12},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x16},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x13},std::byte{0x14},std::byte{0x14},std::byte{0x14},std::byte{0x14},std::byte{0x15},std::byte{0x15},std::byte{0x15},std::byte{0x15}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2E6ED0 collision byte selectors108");

    // Whole15D50 admits only layer3..13 before its two finite reads.
    // Preserve exact original branch identities, with no adjacent code/data.
    m.mount(0x15e74,encode(std::array{0x15e4fu,0x15deeu,0x15e04u,0x15e21u,0x15e39u,0x15e69u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored15D50 material-layer branch targets6");
    m.mount(0x15e8c,std::array<std::byte,11>{std::byte{0x00},std::byte{0x05},std::byte{0x05},std::byte{0x05},std::byte{0x05},std::byte{0x05},std::byte{0x05},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored15D50 material-layer byte selectors11");

    // Complete actual3DFD0 numeric RO closure for repeat model preparation.
    // Preserve all original scalar bits; caller state/flags stay separately
    // owned and current source results are never supplied by this data bank.
    m.mount(0x46a6e4,encode(std::array{0x43c80000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3DFD0 repeat-model scalar46A6E4");
    m.mount(0x46a6e0,encode(std::array{0x3b23d70bu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3DFD0 repeat-model scalar46A6E0");
    m.mount(0x44a8a0,encode(std::array{0x3e19999au}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3DFD0 repeat-model scalar44A8A0");
    m.mount(0x46aa30,encode(std::array{0x3d09382du}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored3DFD0 repeat-model scalar46AA30");
    // Whole4CBE0 locally bounds its outer selector and both nested target
    // tables. Preserve all five entries even where targets are repeated.
    m.mount(0x4d044,encode(std::array{0x4cc60u,0x4cdb8u,0x4cc8fu,0x4cd3bu,0x4cdffu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4CBE0 outer branch targets5");
    m.mount(0x4d058,std::array<std::byte,17>{std::byte{0x00},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x04},std::byte{0x01},std::byte{0x04},std::byte{0x02},std::byte{0x03}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4CBE0 outer byte selectors17");
    m.mount(0x4d06c,encode(std::array{0x4cdb4u,0x4cc71u,0x4cd31u,0x4cc85u,0x4cc7bu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4CBE0 type branch targets5");
    m.mount(0x4d080,encode(std::array{0x4cfd9u,0x4cfd9u,0x4cfd9u,0x4cfe2u,0x4cfe2u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored4CBE0 property branch targets5");
    // Only the four authenticated1CB00 mappings are admitted here. Its caller
    // has no local property-domain bound: indices >=4 remain unsupported,
    // with no surrounding code backing or changes to original dispatch.
    m.mount(0x1cb40,encode(std::array{0x1cb38u,0x1cb07u,0x1cb0fu,0x1cb17u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1CB00 conditional branch targets4");

    // Authored initial value of registered script boolean object_light_interpolate.
    // Mount only this original byte; genuine script writes remain authoritative.
    m.mount(0x4739c4,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored object_light_interpolate script configuration");

    // Shared five-DWORD mask field of the existing source renderer state
    // (492368+30). Only authentic loader-zero cold storage is admitted here;
    // genuine1BDD0/1BD30/1BE30 updates and later original resets stay live.
    // This does not claim completion of the separate renderer constructor.
    m.mount(0x492398,std::array<std::byte,20>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 shared renderer accumulated mask5 cold field");

    // Exact Original135970 read-only scalar at1359CD (MOVSS), proven with
    // the complete already-entered136EA0 retained-owned literal closure.
    m.mount(0x46a104,std::array{std::byte{0},std::byte{0},std::byte{0x20},std::byte{0x40}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored135970 render-view scalar");

    // Exact transient Original25D30 suppression input, not a ready flag.
    // Loaderzero only: genuine41CB0 set/clear and all later source writes
    // remain authoritative.492080 alreadybelongs to the existing submitted
    // view owner491E20/298 and is deliberately not mounted or reset here.
    m.mount(0x492295,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold transient fog suppression byte");

    // Authored FLOAT9 consumed by genuine16A2B0 FLD m32, shared with the
    // existing typed network-session parameter declaration. Exact RO4B only.
    m.mount(0x44a498,std::array{std::byte{0},std::byte{0},std::byte{0x10},std::byte{0x41}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored16A2B0 first-person finish scalar");

    // Exact adjacent loader-zero destinations of the genuine angle CRTs.
    // World executes380D50 then380D30 from the forward original _cinit order;
    // no constructed pi/two-pi output is installed by this storage owner.
    m.mount(0x5564c4,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold angle-wrap CRT destinations");

    // Original F0BE0 bounds action-1 to0..14 before reading the adjacent
    // fifteen byte selectors and four DWORD targets. Keep all authored
    // branch identities as read-only data; no code or selected result is
    // supplied, and the genuine dispatcher and child calls still execute.
    constexpr std::array<std::uint32_t,4> action_targets{
        0xf0c8bu,0xf0c41u,0xf0c66u,0xf0d12u};
    std::array<std::byte,31> action_dispatch{};
    const auto target_bytes=encode(action_targets);
    for(unsigned i=0;i<target_bytes.size();++i)
        action_dispatch[i]=target_bytes[i];
    constexpr std::array<std::uint8_t,15> action_selectors{
        0,0,0,0,3,3,3,3,3,3,3,3,3,1,2};
    for(unsigned i=0;i<action_selectors.size();++i)
        action_dispatch[16+i]=std::byte(action_selectors[i]);
    m.mount(0xf0d28,action_dispatch,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored F0BE0 four-target fifteen-selector dispatch data");

    // Whole Original169493 sign-extends CX and bounds the unsigned result
    // to0..40 before reading this exact unaligned inline DWORD table.
    // Every branch remains in its genuine MOV/RET leaf; preserve authored
    // target identities rather than replacing the query with result values.
    m.mount(0x16957b,encode(std::array{
        0x1694a9u,0x1694afu,0x1694b5u,0x1694bbu,0x1694c1u,0x1694c7u,0x1694cdu,0x1694d3u,
        0x1694d9u,0x1694f1u,0x1694f7u,0x1694fdu,0x169503u,0x1694dfu,0x1694e5u,0x1694ebu,
        0x16957au,0x169509u,0x16950fu,0x16950fu,0x169515u,0x169515u,0x16951bu,0x16951bu,
        0x16957au,0x16957au,0x169521u,0x169527u,0x16952du,0x169533u,0x169539u,0x16953fu,
        0x169545u,0x16954bu,0x169551u,0x169557u,0x16955du,0x169563u,0x169569u,0x16956fu,
        0x169575u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored169493 forty-one-target inline selector data");

    // Whole OriginalCC1E0 bounds action-1 to0..7 on both hand-equality
    // routes before these adjacent eight-DWORD reads. Keep both complete
    // authored tables, including shared fallbacks; genuine guards, dispatch
    // and later E8E10 calls remain responsible for the selected behavior.
    m.mount(0xcc30c,encode(std::array{
        0xcc27fu,0xcc286u,0xcc271u,0xcc278u,0xcc303u,0xcc303u,0xcc28du,0xcc294u,
        0xcc2beu,0xcc2c5u,0xcc2b0u,0xcc2b7u,0xcc303u,0xcc303u,0xcc2ccu,0xcc2d3u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredCC1E0 two eight-target hand selector tables");

    // Whole Original2C390 reads one FLOAT and one DOUBLE in addition to
    // its three already owned constants. Preserve their authored bits and
    // exact separate extents; genuine x87/SSE constructor stores produce
    // the current destination under the caller's floating-point controls.
    m.mount(0x46a4bc,encode(std::array{0x3df5c28fu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2C390 constructor FLOAT operand");
    m.mount(0x46a4c8,encode(std::array{0x80000000u,0xbff41b2fu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2C390 constructor DOUBLE operand");
    // Original E8D80 iterates the sixty-action catalog. E8E10 preserves
    // the request type and E8E50 selects E9990 for eighteen authored types.
    // Keep only its exact target/selector spans, without alignment or zero
    // sentinel overlaps. Other callback types and out-of-catalog inputs do
    // not gain an invented guard, fallback or executable image backing.
    m.mount(0xe9ba8,encode(std::array{
        0xe99b2u,0xe99bau,0xe99beu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredE9990 first three branch targets");
    m.mount(0xe9bb6,std::array<std::byte,60>{
        std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x03},std::byte{0x03},
        std::byte{0x03},std::byte{0x03},std::byte{0x00},std::byte{0x00},std::byte{0x02},std::byte{0x02},std::byte{0x02},std::byte{0x02},std::byte{0x03},std::byte{0x03},
        std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},
        std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},
        std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},
        std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x03},std::byte{0x01},std::byte{0x01},std::byte{0x01},std::byte{0x02},std::byte{0x02},std::byte{0x02}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredE9990 first sixty catalog selectors");
    m.mount(0xe9bf4,encode(std::array{
        0xe99d1u,0xe99ebu,0xe9a05u,0xe9a1fu,0xe9a39u,0xe9a53u,0xe9a6du,0xe9a87u,
        0xe9aa1u,0xe9abbu,0xe9ad5u,0xe9aefu,0xe9b09u,0xe9b23u,0xe9b3du,0xe9b57u,
        0xe9b71u,0xe9b8bu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredE9990 second eighteen branch targets");
    m.mount(0xe9c3e,std::array<std::byte,60>{
        std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x01},std::byte{0x02},std::byte{0x03},std::byte{0x04},std::byte{0x05},std::byte{0x12},std::byte{0x12},
        std::byte{0x12},std::byte{0x12},std::byte{0x06},std::byte{0x07},std::byte{0x08},std::byte{0x09},std::byte{0x0a},std::byte{0x0b},std::byte{0x12},std::byte{0x12},
        std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},
        std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},
        std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},
        std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x12},std::byte{0x0c},std::byte{0x0d},std::byte{0x0e},std::byte{0x0f},std::byte{0x10},std::byte{0x11}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredE9990 second sixty catalog selectors");

    // Reached17F946 evaluates transition type3: base4782A8 +3*1024.
    // Reuse the retained native material function bank's transition[3].
    // These lossless run counts expand its authored samples exactly;
    // no curve is regenerated mathematically and no adjacent row is mounted.
    constexpr std::array<std::uint8_t,256> response_three_runs{
        65,26,20,18,15,13,13,12,11,10,10,9,9,9,9,8,
        8,7,8,7,7,7,7,6,7,6,6,6,6,6,6,6,
        6,5,6,5,5,5,6,5,5,5,5,4,5,5,5,4,
        5,4,5,4,5,4,5,4,4,4,5,4,4,4,4,4,
        4,4,4,4,4,3,4,4,4,4,3,4,4,3,4,3,
        4,4,3,4,3,4,3,3,4,3,4,3,3,4,3,3,
        3,4,3,3,3,4,3,3,3,3,3,3,3,3,3,3,
        3,4,2,3,3,3,3,3,3,3,3,3,3,3,2,3,
        3,3,3,3,2,3,3,3,2,3,3,3,2,3,3,2,
        3,3,2,3,2,3,3,2,3,2,3,3,2,3,2,3,
        2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,2,
        3,2,3,2,2,3,2,2,3,2,3,2,2,3,2,2,
        2,3,2,2,3,2,2,2,3,2,2,2,3,2,2,2,
        3,2,2,2,2,3,2,2,2,2,3,2,2,2,2,2,
        2,3,2,2,2,2,2,2,2,2,3,2,2,2,2,2,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1};
    std::array<std::byte,1024> response_three{};std::size_t response_three_at{};
    for(unsigned value=0;value<response_three_runs.size();++value)
        for(unsigned n=0;n<response_three_runs[value];++n) {
            if(response_three_at==response_three.size())
                throw std::logic_error("Authored transition row three exceeds its exact extent");
            response_three[response_three_at++]=std::byte(value);
        }
    if(response_three_at!=response_three.size())
        throw std::logic_error("Authored transition row three lacks its exact extent");
    m.mount(0x478ea8u,response_three,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored17F946 existing material transition row three");
    // Process-cold authored state, not a per-shot or map-transition reset.
    // Actual15F77A reads four bytes;15F788 increments only the low16.
    // The independent policy atA6 is read/cleared by167785/167792.
    // Retain the incidental high byte separately and never make it writable.
    m.mount(0x48c7a4u,std::array{std::byte{42},std::byte{}},
        SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 process-cold15F788 event counter16");
    m.mount(0x48c7a6u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 process-cold167792 independent policy8");
    m.mount(0x48c7a7u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 exact incidental high byte of15F77A load");
    // Complete19B8F0 masks to six bits, then unsigned-bounds the
    // selector to0..40 before reading exactly one authored branch target.
    // Keep out-of-range returns and the existing native bodies unchanged.
    m.mount(0x19b9f8u,encode(std::array<std::uint32_t,41>{
        0x19b905u,0x19b908u,0x19b90eu,0x19b914u,0x19b91au,0x19b920u,0x19b926u,0x19b92cu,
        0x19b932u,0x19b938u,0x19b93eu,0x19b944u,0x19b94au,0x19b950u,0x19b956u,0x19b962u,
        0x19b968u,0x19b96eu,0x19b974u,0x19b95cu,0x19b97au,0x19b980u,0x19b986u,0x19b98cu,
        0x19b992u,0x19b998u,0x19b99eu,0x19b9a4u,0x19b9aau,0x19b9b0u,0x19b9b6u,0x19b9bcu,
        0x19b9c2u,0x19b9c8u,0x19b9ceu,0x19b9d4u,0x19b9dau,0x19b9e0u,0x19b9e6u,0x19b9ecu,
        0x19b9f2u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored19B8F0 bounded forty-one branch targets");
    // Actual FFF44 subtracts this authored half-pi word after the existing
    // native11F7E0 vector-angle producer. Reuse the exact by-value operand
    // already used by the development projectile impact calculation.
    m.mount(0x41c4ccu,encode(std::array<std::uint32_t,1>{0x3fc90fdbu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authoredFFF44 projectile impact half-pi operand");
    // Both complete FFA60 switch prefixes sign-extend the selected short,
    // then unsigned-compare six before reading one of these seven targets.
    // Publish only the authored target words; target code remains native.
    m.mount(0x100d04u,encode(std::array<std::uint32_t,7>{
        0x100118u,0x100129u,0x100130u,0x100137u,0x10013eu,0x100145u,0x100145u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored100105 bounded projectile response targets");
    m.mount(0x100d20u,encode(std::array<std::uint32_t,7>{
        0x100c6cu,0x100c36u,0x100cf6u,0x100c9eu,0x100cf6u,0x100cf6u,0x100c36u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored100C2F bounded projectile finalization targets");
    // Mode one in10070C..100756 uses the exact authored multiplier already
    // retained by the development projectile effect-scale calculation.
    // Other modes and the native comparison/clamp instructions are unchanged.
    m.mount(0x46a7d4u,encode(std::array<std::uint32_t,1>{0x3f22f983u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored100722 projectile effect-scale multiplier");
    // Complete18AE80 subtracts thirteen and unsigned-bounds seven before
    // selecting one of these eight authored fallback targets. Keep target
    // code native and exclude the preceding padding byte and following code.
    m.mount(0x18aefcu,encode(std::array<std::uint32_t,8>{
        0x18aee5u,0x18aeeau,0x18aeeau,0x18aed0u,
        0x18aed0u,0x18aeeau,0x18aed7u,0x18aedeu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored18AEC9 bounded category fallback targets");
    // The complete2C2AD0 owner zero-extends its type word and unsigned-bounds
    // three before dispatch. Preserve its real post-table continuation and
    // native child calls; mount only the four authored target words.
    m.mount(0x2c2b30u,encode(std::array<std::uint32_t,4>{
        0x2c2adfu,0x2c2af4u,0x2c2b09u,0x2c2b1fu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2C2AD8 bounded particle motion targets");
    // Persistent cold memory effect of the complete17-byte CRT3829C0:
    // source46A1B8 is exactly3E2AAAAB, also used by the development collider.
    // Materialize that field once; later writes and restored values stay mutable.
    m.mount(0x56e0e8u,encode(std::array<std::uint32_t,1>{0x3e2aaaabu}),
        SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 CRT3829C0 mutable particle collision fraction");
    // The unchanged collision response already uses this exact .005f word.
    m.mount(0x46a784u,encode(std::array<std::uint32_t,1>{0x3ba3d70au}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored2C318D particle collision position offset");
}
}
