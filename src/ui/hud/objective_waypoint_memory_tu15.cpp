#include "halo2/objective_waypoint_memory_tu15.hpp"
#include "halo2/native_tu15_multiplayer_data.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>

namespace halo2 {
namespace {
template<std::size_t N> auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(unsigned i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(words[i]>>(j*8));
    return bytes;
}
template<std::size_t N>void share_immutable_words(SourceRuntimeMemory& memory,std::uint32_t address,
    const std::array<std::uint32_t,N>& words,const char* owner) {
    const auto bytes=encode(words);std::array<bool,N*4> shared{};
    for(unsigned i=0;i<bytes.size();++i) {
        try {
            const auto previous=memory.view(address+i,1);shared[i]=true;
            if(!memory.is_read_only(address+i,1) || previous[0]!=bytes[i])
                throw FormatError("Objective arithmetic operand differs from its original owner");
        }catch(const std::out_of_range&){}
    }
    for(unsigned i=0;i<bytes.size();) {
        if(shared[i]){++i;continue;}
        const auto first=i++;while(i<bytes.size() && !shared[i])++i;
        memory.mount(address+first,std::span(bytes).subspan(first,i-first),SourceRuntimeMemory::Access::ReadOnly,owner);
    }
}
void share_immutable_word(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t bits,const char* owner) {
    share_immutable_words(memory,address,std::array{bits},owner);
}
void bind(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(value>>(i*8));
    try {memory.write(address,bytes);}
    catch(const std::out_of_range&) {
        memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadWrite,
            "native objective actual cache resource binding");
    }
}
}
SourceRuntimeMemory native_objective_waypoint_memory_tu15(const Cache& cache) {
    if(!cache.is_mcc() || !cache.original_data() || cache.original_data()->image!=OriginalData::Image::TitleUpdate15)
        throw FormatError("Native objective presentation requires its authenticated MCC/TU1.5 resources");
    SourceRuntimeMemory memory;
    // Only the compiled multiplayer consumer's authenticated arithmetic data.
    // World startup, sound SDK construction and platform imports have their
    // own owners and do not belong to this metadata-only presentation bank.
    mount_native_tu15_multiplayer_branch_data(memory);
#include "gameplay/multiplayer/native_tu15_objective_presentation_data.inc"
    // Same individually authenticated shared scalar operands as the native
    // world bank; these have no mutable game state or constructor side effect.
    // Authentication: waypoint-arithmetic-authority.json, TU1.5 SHA256 above.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,9>{{
        {0x4500b8u,0x42c80000u},{0x46a0ccu,0x3f000000u},{0x46a0d0u,0x3f800000u},
        {0x46a0d4u,0xbf000000u},{0x46a0dcu,0xbf800000u},{0x46a0ecu,0x38d1b717u},
        {0x46a11cu,0x40000000u},{0x46a178u,0x3dcccccdu},{0x46a250u,0x4b7fffffu}}})
        share_immutable_word(memory,address,bits,"native TU1.5 shared waypoint arithmetic operand");
    share_immutable_word(memory,0x46a984u,0x42000000u,
        "native TU1.5 waypoint binary32 scalar operand");
    // Original CRT32B1A6/32B0D3 policies, also declared by the native world.
    share_immutable_word(memory,0x46e4f4u,0x173fu,"native TU1.5 CRT x87 wrapper policy");
    share_immutable_word(memory,0x46e4f0u,0x1b3fu,"native TU1.5 CRT x87 integer wrapper policy");
    if(cache.tags().size()>65536)throw FormatError("Native objective cache exceeds original tag indices");
    const auto size=std::max<std::size_t>(16,cache.tags().size()*16);
    const auto directory=memory.find_unowned_range(0xf6000000u,0xff000000u,size,0x10000u);
    if(!directory)throw FormatError("Native objective cache has no private source-token directory range");
    memory.mount(*directory,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,
        "native objective actual typed cache directory");
    bind(memory,0x4f0414u,*directory);
    for(const auto& tag:cache.tags()) {
        if(!tag.address || (tag.group!="matg" && tag.group!="hudg" && tag.group!="bitm"))continue;
        const auto metadata=cache.load_source_tag_metadata(tag.datum);
        memory.mount(metadata.source_address(),metadata.bytes_at(metadata.source_address(),metadata.size()),
            SourceRuntimeMemory::Access::ReadOnly,"native objective canonical "+tag.group+" metadata");
        const auto row=*directory+(tag.datum&65535u)*16;
        bind(memory,row,tag.type().value);bind(memory,row+4,tag.datum);
        bind(memory,row+8,metadata.source_address());bind(memory,row+12,metadata.size());
    }
    const auto globals=cache.load_source_tag_metadata(cache.globals_tag().value);
    const Bytes root(globals.bytes_at(globals.source_address(),globals.size()));
    if(root.u32(0x110)!=1)throw FormatError("Native objective HUD interface lacks its actual unique row");
    const Bytes interface(globals.bytes_at(root.u32(0x114),0x98));
    const auto hud=cache.load_source_tag_metadata(interface.u32(0x34));
    if(hud.group()!="hudg")throw FormatError("Native objective HUD metadata group differs");
    bind(memory,0x51df94u,hud.source_address());
    return memory;
}
}
