#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::array<std::byte,4> encode(std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
    return bytes;
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
}
void mount_native_tu15_renderer_tail_storage_data(SourceRuntimeMemory& memory) {
    // These exact extents are zero in the authenticated TU1.5 image. Source
    // constructors still perform every store and choose the arena bindings.
    for(const auto address:std::array<std::uint32_t,3>{0x515ec0,0x515ec4,0x515e2c})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "source12560 tail arena binding");
    memory.mount(0x52dfd8,std::vector<std::byte>(0x40c0),SourceRuntimeMemory::Access::ReadWrite,
        "source12560 query records and three mask banks");
    // Actual3803B0 writes the authored46A0C4 operand here. This separate
    // loader-zero owner begins at the query bank exclusive end; no result preset.
    memory.mount(0x532098u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 3803B0 lens normalization scalar");
    const auto negative_one=encode(0xbf800000);bool present=false;
    try {
        const auto old=memory.view(0x46a0dc,4);present=true;
        if(!memory.is_read_only(0x46a0dc,4)||!std::ranges::equal(old,negative_one))
            throw std::invalid_argument("Source renderer tail shared negative-one operand differs");
    }catch(const std::out_of_range&){}
    if(!present)memory.mount(0x46a0dc,negative_one,SourceRuntimeMemory::Access::ReadOnly,
        "source20E90 authored negative-one scalar");
    //1FB90 shares the already declared immutable1.0 operand. Require that
    // owner rather than mounting a second, potentially conflicting region.
    const auto one=encode(0x3f800000);
    if(!memory.is_read_only(0x46a0d0,4)||!std::ranges::equal(memory.view(0x46a0d0,4),one))
        throw std::invalid_argument("Source renderer tail shared one operand differs");
}
NativeTu15RendererTailStorageViews native_tu15_renderer_tail_storage_views(
    const SourceRuntimeMemory& memory) {
    const auto capture=word(memory,0x515ec0),lens=word(memory,0x515ec4),render=word(memory,0x515e2c);
    if(!capture||!lens||!render)
        throw std::invalid_argument("Source renderer tail arena bindings are not constructed");
    // Resolving complete extents rejects truncated, non-owned or stale tokens.
    NativeTu15RendererTailStorageViews result{memory.native_address(capture,0x3fc),
        memory.native_address(lens,0x24),memory.native_address(render,0x34),
        memory.native_address(0x52dfd8,0x40c0)};
    for(const auto address:std::array{result.state_1fb90,result.state_20e90,
        result.state_12560,result.query_bank})
        if(address.value<=0xffffffffu)
            throw std::invalid_argument("Source renderer tail storage lacks native64 backing");
    return result;
}
}
