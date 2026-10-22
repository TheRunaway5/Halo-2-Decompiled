#include "halo2/native_tu15_prt_cache_data.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
struct Bank {std::uint32_t address;std::size_t bytes;const char* owner;};
constexpr std::array banks{
    Bank{0x4c5160u,3u*9u*4u,"native TU1.5 cold PRT three RGB order3 SH channels"},
    Bank{0x4c51d0u,24u*104u,"native TU1.5 cold PRT 24 complete owner records"},
    Bank{0x515ec8u,13u,"native TU1.5 cold PRT count, previous time, phase and validity"}};
}
void mount_native_tu15_prt_cache_data(SourceRuntimeMemory& memory) {
    // These three precise extents lie in the authenticated writable .data
    // section's loader-zeroed tail. 22836..2286C independently prove24 rows
    // at104-byte stride;236C6 retains that same24-owner admission limit.
    // Exclude alignment gaps and all neighboring storage. Never reset an
    // already published bank, or reinterpret foreign matching bytes as ours.
    for(const auto& existing:memory.checkpoint().regions)
        for(const auto& bank:banks)
            if(std::uint64_t(existing.address)<std::uint64_t(bank.address)+bank.bytes &&
                std::uint64_t(bank.address)<std::uint64_t(existing.address)+existing.bytes.size())
                throw std::invalid_argument("Native TU1.5 PRT cold storage overlaps an existing owner");
    for(const auto& bank:banks)
        memory.mount(bank.address,std::vector<std::byte>(bank.bytes),
            SourceRuntimeMemory::Access::ReadWrite,bank.owner);
}
}
