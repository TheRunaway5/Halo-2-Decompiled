#include "halo2/native_tu15_main_view_data.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
struct Bank{std::uint32_t address;std::size_t bytes;const char* owner;};
constexpr std::array banks{
    Bank{0x4c65c0u,0x74u,"native TU1.5 cold complete submitting camera destination"},
    Bank{0x4c6634u,0xc0u,"native TU1.5 cold complete main view projection destination"},
    Bank{0x4c66f4u,21u,"native TU1.5 cold main view five DWORD inputs and flag"},
    Bank{0x4c670cu,4u,"native TU1.5 cold main view optional object binding"},
    Bank{0x4c6830u,1u,"native TU1.5 cold main view caller reset flag"},
    Bank{0x4c6839u,1u,"native TU1.5 cold main view geometry modifier flag"},
    Bank{0x4c683cu,4u,"native TU1.5 cold main view geometry LOD cap"},
    Bank{0x4c6840u,6u,"native TU1.5 cold main view six geometry feature flags"},
    Bank{0x4c6848u,8u,"native TU1.5 cold main view coverage and secondary scales"},
    Bank{0x473644u,4u,"native TU1.5 authored mutable main view effect scale"}};
}
void mount_native_tu15_main_view_data(SourceRuntimeMemory& memory) {
    // 2BA24..2BAAB publish the exact scalar inputs and116-byte camera;
    // whole2FD70 constructs the192-byte projection. Whole2C470 writes the
    // separate cap, six feature bytes and two scales. Do not claim the gaps
    // between these fields or reset any previously published/foreign owner.
    for(const auto& existing:memory.checkpoint().regions)
        for(const auto& bank:banks)
            if(std::uint64_t(existing.address)<std::uint64_t(bank.address)+bank.bytes &&
                std::uint64_t(bank.address)<std::uint64_t(existing.address)+existing.bytes.size())
                throw std::invalid_argument("Native TU1.5 main-view cold storage overlaps an existing owner");
    for(const auto& bank:banks){
        std::vector<std::byte> bytes(bank.bytes);
        // This sole initialized field is literal writable .data in TU1.5,
        // independently of2C470's later per-player output. The other353
        // bytes are the exact loader-zeroed tail, not preset view results.
        if(bank.address==0x473644u){constexpr std::uint32_t bits=0x3f59999au;
            for(unsigned i=0;i<4;++i)bytes[i]=std::byte(bits>>(8*i));}
        memory.mount(bank.address,bytes,SourceRuntimeMemory::Access::ReadWrite,bank.owner);
    }
}
}
