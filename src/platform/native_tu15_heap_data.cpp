#include "halo2/native_tu15_heap_data.hpp"

namespace halo2 {
namespace {
template<std::size_t N>std::array<std::byte,N*4> encode(const std::array<std::uint32_t,N>& values) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(values[i]>>(j*8));
    return bytes;
}
}
SourceRuntimeMemory native_tu15_process_heap_memory() {
    SourceRuntimeMemory memory;
    // Complete original memmove tail dispatch for zero through three bytes.
    // The constructor copies the caller's aligned 0x30-byte parameter record.
    // These are opaque branch identities, never callable host pointers.
    memory.mount(0x32aadcu,encode(std::array<std::uint32_t,4>{0x32aaecu,0x32aaf4u,0x32ab00u,0x32ab14u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 typed copy-tail branch identities");
    const auto& defaults=native_tu15_heap_defaults;
    memory.mount(0x46e2c0u,encode(std::array{defaults.segment_reserve,defaults.segment_commit,
        defaults.decommit_total_free_threshold,defaults.decommit_free_block_threshold}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 typed heap parameter defaults");
    memory.mount(0x56ee11u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold XAPI heap creation policy bytes");
    memory.mount(0x56ee28u,encode(std::array<std::uint32_t,1>{0}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold process heap binding");
    for(const auto& [cell,ordinal]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
            {0x41b6d0u,199u},{0x41b6d4u,184u},{0x41b6dcu,291u},{0x41b6e0u,217u}}})
        memory.mount(cell,encode(std::array{0x80000000u|ordinal}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 process-heap platform import identity "+std::to_string(ordinal));
    return memory;
}
std::array<std::uint32_t,6> native_tu15_process_heap_arguments(std::uint32_t parameters) {
    return {2,0,0x100000,0x1000,0,parameters};
}
}
