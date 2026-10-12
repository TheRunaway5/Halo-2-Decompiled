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
};
// Whole retail223C70 (TU1.1 225D80) pointer/resource fixups. The actual ready
// allocation and resource header/descriptor banks are caller-owned. This does
// not allocate, submit IO, manufacture completion, set linked+1E or touch cache
// age;12DE70 owns those separate operations. Native buffers retain32-bit source
// tokens and packed relative addresses, never host vector pointers.
void source_geometry_resource_publish(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory&);
}
