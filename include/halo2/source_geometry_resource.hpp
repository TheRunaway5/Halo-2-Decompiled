#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SourceGeometryResourceMemory {
    // Both adapters resolve the same current owned namespace. Publication
    // writes must be immediately visible to descriptor/buffer reads.
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
    std::function<std::span<std::byte>(std::uint32_t,std::size_t)> write;
    // Optional TU1.5 store owner, including its original instruction PC.
    // It performs the write itself and may publish before/written bytes.
    // Retail/TU1.1 callers keep the ordinary span writer above.
    std::function<void(std::uint32_t instruction,std::uint32_t address,
        std::span<const std::byte>)> store_tu15;
};
// Whole retail223C70 (TU1.1 225D80 / TU1.5 22A680) pointer/resource fixups. The actual ready
// allocation and resource header/descriptor banks are caller-owned. This does
// not allocate, submit IO, manufacture completion, set linked+1E or touch cache
// age;12DE70 owns those separate operations. Native buffers retain32-bit source
// tokens and packed relative addresses, never host vector pointers.
void source_geometry_resource_publish(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory&);
// Optional live retained-frame driver. Descriptor fixup DATA stays in the
// same owner; callers provide only delayed allocation and iteration storage
// when those cells can alias the published resource/descriptor banks.
struct SourceGeometryResourcePublishDriver {
    std::function<std::uint32_t(std::uint32_t root)> initial_allocation;
    std::function<bool(std::int32_t count)> begin;
    std::function<std::uint32_t(std::uint32_t descriptor_base)> descriptor;
    std::function<std::uint32_t()> current_allocation;
    std::function<bool()> advance;
};
void source_geometry_resource_publish_driven(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory&,const SourceGeometryResourcePublishDriver&);
}
