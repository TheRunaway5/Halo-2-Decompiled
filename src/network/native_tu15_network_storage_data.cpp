#include "halo2/source_network_storage_startup_tu15.hpp"
#include <array>

namespace halo2 {
namespace {
template<std::size_t N>auto words(const std::array<std::uint32_t,N>& values) {
    std::array<std::byte,N*4> bytes{};
    for(unsigned i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(values[i]>>(j*8));return bytes;
}
}
void mount_native_tu15_network_storage_data(SourceRuntimeMemory& memory) {
    // Authored shared allocator singleton and its two source method identities.
    // SourcePoolHostStorage owns actual backing; these are never host pointers.
    memory.mount(0x474cb8u,words(std::array{0x486034u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 shared raw allocator binding");
    memory.mount(0x486034u,words(std::array{0x45f048u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 shared raw allocator identity");
    memory.mount(0x45f048u,words(std::array<std::uint32_t,2>{0x11f3f0u,0x11f420u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 shared raw allocate and release method identities");
    // Two control words and four eight-slot callback/context banks. 6DE60 and
    // 70660 construct their contents, counts and initialized bytes themselves.
    memory.mount(0x4dc060u,std::vector<std::byte>(0x88),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold network callback registration storage");
    memory.mount(0x4dc3f0u,std::vector<std::byte>(0x144),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold network callback state storage");
    memory.mount(0x4dc534u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold transport QoS attempt state and pool binding");
    memory.mount(0x56e1ccu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold cached Ethernet link status");
    // The original optimized name copy reads aligned words through the NUL.
    // Preserve its one byte of authored alignment padding, not adjacent data.
    constexpr std::array<char,24> name{"transport qos attempts"};
    memory.mount(0x45c9b4u,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 transport QoS pool name and alignment padding");
}
}
