#include "halo2/source_runtime_sound_codec_event_port.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {namespace {
void literal(SourceRuntimeMemory& m,std::uint32_t p,std::span<const std::byte> expected){
    std::size_t covered{};m.visit_committed([&](auto first,auto bytes){const auto begin=std::max<std::uint64_t>(first,p),
        end=std::min<std::uint64_t>(std::uint64_t(first)+bytes.size(),std::uint64_t(p)+expected.size());if(begin<end)covered+=end-begin;});
    if(covered){if(covered!=expected.size())throw std::invalid_argument("Partially owned codec event immutable operand");
        for(unsigned i=0;i<expected.size();++i)if(!m.is_read_only(p+i,1)||m.view(p+i,1)[0]!=expected[i])
            throw std::invalid_argument("Different codec event immutable bytes/access");return;}
    m.mount(p,expected,SourceRuntimeMemory::Access::ReadOnly,"authenticated codec completion immutable operand");
}
}
void mount_native_tu15_sound_codec_event_data(SourceRuntimeMemory& m){
    literal(m,0x390ba0,std::array{std::byte{0x40},std::byte{},std::byte{},std::byte{},std::byte{0x10},std::byte{},std::byte{},std::byte{}});
    for(const auto& [p,v]:std::array{std::pair{0x41b7a8u,0x8000002cu},std::pair{0x41b7b4u,0x8000006du},
        std::pair{0x41b7b0u,0x80000062u},std::pair{0x41b670u,0x8000002fu},std::pair{0x41b7dcu,0x80000064u},
        std::pair{0x41b7f0u,0x80000089u},std::pair{0x41b7f4u,0x80000099u},std::pair{0x41b7c4u,0x80000077u}}){
        std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));literal(m,p,b);}
    // SDK owns the earlier mutable391208/4 cleanup cell. Keep its exact
    // backing; the context is a separate declared loader-zero source cell.
    m.validate_mutable_view(0x391208,4);
    m.mount(0x39120c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"codec interrupt service context loader-zero cell");
}
}
