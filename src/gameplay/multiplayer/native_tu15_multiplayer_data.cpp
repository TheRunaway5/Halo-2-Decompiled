#include "halo2/native_tu15_multiplayer_data.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
template<std::size_t N> auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)
        bytes[i*4+j]=std::byte(words[i]>>(j*8));
    return bytes;
}
#include "gameplay/multiplayer/native_tu15_multiplayer_sound_class_data.inc"
#include "core/native_tu15_name_mapping_data.inc"
constexpr std::array<std::pair<std::uint32_t,std::size_t>,22> base_regions{{
    {0x48a138u,4},{0x45fce0u,204},{0x51df9cu,4},{0x52d89cu,4},
    {0x52d8a0u,4},{0x52d8a4u,4},{0x515dc8u,2},{0x515dd0u,16},
    {0x46a0d8u,4},{0x466164u,10},{0x466170u,16},{0x466180u,8},{0x466188u,1},
    {0x248e10u,36},{0x4772a8u,1024},{0x44a834u,52},{0x2446b0u,36},
    {0x4486a1u,1},{0x45f918u,15},{0x45f904u,18},{0x45f8f0u,18},{0x44e9f0u,40}}};
constexpr auto regions=[] {
    std::array<std::pair<std::uint32_t,std::size_t>,base_regions.size()+multiplayer_sound_class_regions.size()+native_name_mapping_regions.size()> result{};
    std::size_t index=0;
    for(const auto row:base_regions)result[index++]=row;
    for(const auto& row:multiplayer_sound_class_regions)result[index++]={row.address,row.bytes.size()};
    for(const auto& row:native_name_mapping_regions)result[index++]={row.address,row.bytes.size()};
    return result;
}();
}

void mount_native_tu15_multiplayer_data(SourceRuntimeMemory& memory) {
    // Reject partial owners and uncommitted reservations before publishing
    // any region. A repeated mount must never reset real engine state.
    for(const auto& old:memory.checkpoint().regions)
        for(const auto [address,bytes]:regions)
            if(std::uint64_t(old.address)<std::uint64_t(address)+bytes&&
               std::uint64_t(address)<std::uint64_t(old.address)+old.bytes.size())
                throw std::invalid_argument("Original multiplayer data overlaps an existing owner");
    std::size_t mounted=0;
    try {
        // Authenticated TU1.5 21b207e7... .data/.rdata. The real3825D0
        // constructor publishes this object's identity into the registry.
        memory.mount(0x48a138u,encode(std::array{0x45fce0u}),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 authored Juggernaut class dispatch cell");
        ++mounted;
        memory.mount(0x45fce0u,encode(std::array<std::uint32_t,51>{
        0x0031ede0u,0x001981e0u,0x0031d400u,0x000a6780u,0x0025bb50u,0x00198200u,
        0x00198250u,0x00198200u,0x002e4d60u,0x0008fdb0u,0x002e4d60u,0x0031d400u,
        0x0031d400u,0x002e4d60u,0x00198790u,0x00198430u,0x0025bca0u,0x0008fdb0u,
        0x00198540u,0x001984d0u,0x0025bc60u,0x002e4d60u,0x002e4d60u,0x0008fdb0u,
        0x0008fdb0u,0x0025bc40u,0x001cb030u,0x00198680u,0x0025bd10u,0x002e4ea0u,
        0x001982b0u,0x0025bc90u,0x0008fdb0u,0x0025bca0u,0x002c5f70u,0x001986d0u,
        0x002e4d60u,0x00198760u,0x0025bcb0u,0x0031dd70u,0x0031d400u,0x0025c250u,
        0x0025c290u,0x0025c3c0u,0x00198880u,0x001988c0u,0x00198910u,0x0025bd80u,
        0x0025bdb0u,0x0025c010u,0x0025bd60u}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 complete original Juggernaut class interface");
        ++mounted;
        // Loader-zeroed cells. Whole1981E0/2C6150/2C6F50/2C90D0
        // publish game-engine globals+FC; no initialized bank is supplied.
        for(const auto address:{0x51df9cu,0x52d89cu,0x52d8a0u,0x52d8a4u}) {
            memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
                "native TU1.5 cold multiplayer mode bank publication destination");
            ++mounted;
        }
        // Whole2C6F50 counts and deduplicates marker groups11..18. The
        // two-byte count and eight two-byte indices are separate owners.
        memory.mount(0x515dc8u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold King hill group count");
        ++mounted;
        memory.mount(0x515dd0u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold King eight distinct hill group indices");
        ++mounted;
        // Whole2C7C50 reads the authored -6.0f operand at2C7C86 while
        // constructing the selected hill's actual region geometry.
        memory.mount(0x46a0d8u,encode(std::array{0xc0c00000u}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 original King region geometry operand");
        ++mounted;
        // Whole2486F0 publishes four shared ranking-criteria lists. The
        // comparison owners248E40/248E80 stop at each original FF sentinel.
        // Preserve the unread gaps and neighboring engine vtable separately.
        const std::array<std::uint8_t,10> criteria0{0,2,3,10,12,14,18,4,6,255};
        const std::array<std::uint8_t,16> criteria1{0,2,3,10,12,14,15,16,17,18,4,6,7,8,9,255};
        const std::array<std::uint8_t,8> criteria2{0,2,3,10,13,18,5,255};
        const std::array<std::uint8_t,1> criteria3{255};
        const auto rank=[&](std::uint32_t address,auto& values) {
            memory.mount(address,std::as_bytes(std::span(values)),SourceRuntimeMemory::Access::ReadOnly,
                "native TU1.5 original sentinel-terminated ranking criteria");
            ++mounted;
        };
        rank(0x466164u,criteria0);rank(0x466170u,criteria1);
        rank(0x466180u,criteria2);rank(0x466188u,criteria3);
        memory.mount(0x248e10u,encode(std::array<std::uint32_t,9>{
            0x248d06u,0x248d28u,0x248d48u,0x248d66u,0x248d84u,
            0x248da2u,0x248dc0u,0x248ddeu,0x248dfcu}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 complete original team-ranking dispatch table");
        ++mounted;
        // Actual17F80B reaches authored channel8 during MCC round reset.
        // Whole17F790 masks each adjacent sample index to0..1023.
        constexpr std::array<std::uint8_t,1024> response_eight{
            220,20,6,250,75,185,52,68,91,169,65,218,128,197,245,94,
            49,152,188,77,168,18,196,49,53,234,250,137,33,57,69,55,
            48,29,218,80,55,128,170,151,90,73,34,12,230,162,206,55,
            222,11,159,72,167,102,211,15,117,68,207,110,85,5,92,215,
            13,169,32,221,24,95,74,4,130,33,184,71,106,3,26,12,
            105,240,215,104,40,4,192,7,227,188,193,6,172,164,38,150,
            141,85,56,67,6,197,37,32,112,89,140,67,55,50,34,87,
            16,158,57,26,131,181,48,26,129,116,12,16,209,253,193,153,
            146,177,232,55,30,87,241,214,5,144,15,72,249,50,88,104,
            212,97,250,218,24,117,104,156,143,6,209,224,245,168,109,218,
            192,65,90,129,129,234,10,103,103,214,143,53,226,181,76,101,
            125,117,184,60,73,118,87,56,150,122,20,94,201,245,144,41,
            127,126,27,253,82,138,106,101,253,177,70,123,169,35,183,69,
            148,14,112,231,125,28,138,233,96,82,189,5,135,228,177,37,
            250,212,28,157,187,117,181,183,118,27,77,31,133,152,114,211,
            104,81,134,149,30,1,55,3,252,129,153,229,233,44,126,71,
            34,163,177,101,232,30,50,152,194,128,162,187,53,229,125,178,
            10,90,194,21,155,240,52,41,190,109,88,161,48,54,199,215,
            171,65,150,109,9,44,199,153,24,195,40,126,250,140,250,84,
            79,54,79,73,102,236,254,84,60,243,142,114,30,33,130,244,
            13,245,230,227,84,123,6,155,237,56,164,40,154,199,48,2,
            211,233,187,40,246,41,181,211,82,98,179,35,250,190,199,146,
            131,252,36,7,0,24,30,214,2,169,193,16,95,128,138,215,
            244,104,251,189,197,16,28,70,30,125,36,20,146,148,210,116,
            14,134,51,45,74,72,229,213,86,84,14,30,22,89,151,197,
            211,158,101,39,82,61,154,161,3,124,33,56,52,219,5,54,
            114,183,95,191,115,128,214,120,47,233,252,209,15,163,10,143,
            86,105,181,152,32,131,61,47,169,10,206,22,179,131,228,70,
            55,169,83,57,191,108,14,241,21,143,227,58,34,105,184,205,
            40,156,11,87,180,228,179,141,250,70,52,203,103,45,25,229,
            168,103,34,43,118,234,79,199,213,222,251,0,167,101,157,236,
            181,253,232,189,154,156,80,171,39,195,63,7,45,47,111,43,
            215,241,64,58,230,15,255,215,130,229,100,90,124,7,216,43,
            50,71,53,60,96,23,13,207,160,139,190,12,98,147,216,1,
            151,62,135,32,95,31,39,78,110,36,32,24,3,115,172,159,
            147,39,62,84,153,244,132,146,31,23,104,178,235,22,104,38,
            128,51,53,167,53,151,118,176,53,144,21,154,33,130,128,49,
            147,65,176,156,218,10,246,225,156,87,210,103,48,43,89,43,
            237,175,233,179,193,37,60,213,178,148,13,215,63,191,221,155,
            175,43,160,191,192,97,71,254,89,174,123,40,28,247,7,118,
            1,129,170,54,95,172,113,231,7,13,182,89,76,106,115,110,
            42,110,133,124,231,54,253,125,215,244,194,137,30,86,242,67,
            156,110,225,58,112,67,173,101,153,76,164,60,184,118,22,16,
            9,139,58,165,243,250,71,72,222,117,236,178,41,211,192,158,
            106,46,92,218,89,54,46,36,16,22,76,51,121,141,186,182,
            23,242,253,34,138,82,241,155,121,239,33,96,182,177,58,107,
            212,110,75,73,129,2,215,72,90,165,8,131,9,9,123,110,
            225,11,117,236,87,23,113,9,247,150,108,224,193,231,76,95,
            12,206,67,202,86,90,45,79,168,166,23,6,101,251,153,25,
            187,47,165,19,9,86,222,117,235,18,191,24,199,31,6,5,
            5,226,63,182,72,39,89,8,111,60,154,41,16,39,118,105,
            185,173,251,184,81,79,245,26,43,126,237,127,206,180,158,185,
            118,52,155,123,204,130,42,148,104,249,154,237,13,4,152,231,
            182,201,70,19,230,120,21,131,212,105,176,32,93,187,109,176,
            221,63,25,151,90,187,17,108,144,234,254,234,231,189,170,240,
            80,183,184,110,131,121,65,148,67,214,162,155,126,248,239,239,
            122,115,221,161,110,84,36,90,40,139,151,73,172,53,124,180,
            232,162,233,41,230,48,34,128,29,62,70,36,194,231,197,81,
            80,53,114,65,137,4,30,126,180,205,22,198,236,253,0,54,
            166,170,212,181,211,172,3,209,71,140,253,131,61,180,180,130,
            40,222,196,51,49,183,90,76,0,22,17,181,190,95,76,79,
            114,223,218,153,15,53,212,100,237,30,19,18,133,65,165,6,
            141,186,37,72,236,138,220,137,21,63,3,249,187,86,156,171,
            251,74,188,112,101,65,41,107,127,201,176,194,182,37,163,50};
        memory.mount(0x4772a8u,std::as_bytes(std::span(response_eight)),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 original round-reset response curve eight 1024 byte samples");
        ++mounted;
        // WholeD7B80 rebuilds the thirteen scenario placement classes at
        // round reset. D7BB7 reads one original capacity word per class.
        memory.mount(0x44a834u,encode(std::array<std::uint32_t,13>{
            128,256,128,256,0,0,2000,400,100,500,256,1024,128}),
            SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 original thirteen scenario placement capacities");
        ++mounted;
        // Whole244460 selects the authored spawn-zone mode flag at2445D3.
        // Preserve all nine source-type cases, including the two empty slots.
        memory.mount(0x2446b0u,encode(std::array<std::uint32_t,9>{
            0x2445dfu,0x2445dau,0x2445e4u,0x2445e8u,0u,0u,
            0x2445edu,0x2445f2u,0x2445dfu}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 complete original spawn-zone mode-flag dispatch table");
        ++mounted;
        // Complete18BAC0 passes these exact substring selectors to228390.
        // Empty selects every nonempty class; the ambient selectors override
        // only their matching targets. Preserve original terminating bytes.
        const auto mount_selector=[&](std::uint32_t address,const auto& name) {
            memory.mount(address,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,
                "native TU1.5 original game-over sound-class substring selector");
            ++mounted;
        };
        mount_selector(0x4486a1u,"");
        mount_selector(0x45f918u,"ambient_nature");
        mount_selector(0x45f904u,"ambient_machinery");
        mount_selector(0x45f8f0u,"ambient_computers");
        //15C4E0 indexes these five two-policy arrays while retiring world
        // items and player corpses. E+C0C supplies the actual policy index.
        memory.mount(0x44e9f0u,encode(std::array<std::uint32_t,10>{
            0x41f00000u,0x41f00000u,0x41f00000u,0x40a00000u,0x42b40000u,
            0x41700000u,0x41700000u,0x40a00000u,0x42b40000u,0x41200000u}),
            SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 original item and corpse retirement policies");
        ++mounted;
        for(const auto& row:multiplayer_sound_class_regions) {
            memory.mount(row.address,row.bytes,row.read_only?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,
                "native TU1.5 original54 sound-class names and complete name-pointer table");
            ++mounted;
        }
        // Complete233CF1 reads canonical pointer/glyph records and the exact
        // UTF16 terminators. Preserve original .data glyph-table mutability;
        // the token table and pointed strings are .rdata, not cold zeroes.
        for(const auto& row:native_name_mapping_regions) {
            memory.mount(row.address,row.bytes,row.read_only?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,
                "native TU1.5 original name token and private glyph operands");
            ++mounted;
        }
    }catch(...) {
        while(mounted)memory.release(regions[--mounted].first);
        throw;
    }
}
}
