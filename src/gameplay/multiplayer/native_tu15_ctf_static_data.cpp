#include "halo2/native_tu15_ctf_static_data.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
template<std::size_t N>std::array<std::byte,N*4> encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(words[i]>>(j*8));
    return bytes;
}
constexpr std::array<std::pair<std::uint32_t,std::size_t>,3> regions{{
    {0x48bfa0u,4},{0x466190u,204},{0x52d710u,4}}};
}
void mount_native_tu15_ctf_static_data(SourceRuntimeMemory& memory) {
    // Preserve complete region ownership, including uncommitted reservations.
    // This cold allocation precedes imported tags and source execution. A
    // repeated declaration must never reset249460's later bank publication.
    for(const auto& old:memory.checkpoint().regions)
        for(const auto [address,bytes]:regions)
            if(std::uint64_t(old.address)<std::uint64_t(address)+bytes&&
               std::uint64_t(address)<std::uint64_t(old.address)+old.bytes.size())
                throw std::invalid_argument("Original CTF class metadata overlaps an existing owner");
    std::size_t mounted=0;
    try {
        // Authenticated TU1.5 .data: writable FILE-BACKED dispatch cell;
        // no CRT constructor is invented. Actual3825D0 publishes its identity
        // for both source types1/9 via existing47D174 and56DFA0 owners.
        memory.mount(0x48bfa0u,encode(std::array{0x466190u}),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 file-backed shared CTF Assault class dispatch cell");
        ++mounted;
        // Complete51-slot .rdata interface. Unowned methods retain their
        // actual original identities and fail at the real dispatcher boundary.
        memory.mount(0x466190u,encode(std::array<std::uint32_t,51>{
        0x00249440u,0x00249460u,0x0031d400u,0x000a6780u,0x0024b390u,
        0x002e4d60u,0x00249600u,0x002e4d60u,0x002e4d60u,0x0008fdb0u,
        0x002e4d60u,0x0031d400u,0x00249590u,0x0024aa90u,0x0024acb0u,
        0x002496b0u,0x0024a810u,0x0024a910u,0x0024a070u,0x0024a130u,
        0x0024a180u,0x0024a2e0u,0x002e4d60u,0x0024a2f0u,0x0024a4b0u,
        0x0024a620u,0x0024a6c0u,0x0025bc70u,0x0025bd10u,0x002e4ea0u,
        0x0024a1e0u,0x0024a290u,0x0008fdb0u,0x0024a6f0u,0x0024b2f0u,
        0x0024b300u,0x0024b2b0u,0x0024b3c0u,0x0025bcb0u,0x000ad740u,
        0x0024c670u,0x0025c250u,0x0025c290u,0x0025c3c0u,0x0024c680u,
        0x0024c6f0u,0x0024ca90u,0x0025bd80u,0x0025bdb0u,0x0025c010u,
        0x0025bd60u}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 complete original CTF Assault class interface");
        ++mounted;
        // .data virtual tail is loader-zeroed. Whole249460, reached through
        // real configure_session, publishes engine globals+FC here later.
        memory.mount(0x52d710u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold CTF Assault bank publication destination");
        ++mounted;
    }catch(...) {
        while(mounted)memory.release(regions[--mounted].first);
        throw;
    }
}
}
