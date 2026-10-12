#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
enum class SourceBitmapStorageKind : std::uint32_t {
    Image,
    PredictedImage,
    PhysicalManager
};
struct SourceBitmapStorageInput {
    // Explicit addresses supplied after an allocator has actually succeeded.
    // These do not allocate memory or substitute a kernel return value.
    std::uint32_t address{};
    std::uint32_t allocator_interface{};
    std::uint32_t pool_allocator_interface{}; // Actual46875C, for the manager.
};
std::size_t source_bitmap_storage_bytes(SourceBitmapStorageKind);
// Exact12C0D0 constructor bodies16B5F0/13D170 after allocation admission.
// Unwritten image payload bytes stay unchanged, including row salt words.
void source_bitmap_initialize_storage(SourceBitmapStorageKind,
    const SourceBitmapStorageInput &,std::span<std::byte>);

struct SourceBitmapPhysicalReservation {
    std::uint32_t upper_after{},physical_base{},protection_bytes{};
    std::int32_t page_count{};
    bool calls_protection_wrapper{}; // The wrapper itself skips zero bytes.
};
// Exact12C1E0 arithmetic before XPhysicalProtect. Actual current-stage lower
// and upper bounds are required. This does not protect, activate/reset pools,
// resize the physical manager, or establish native texture-budget history.
SourceBitmapPhysicalReservation source_bitmap_reserve_stage(
    std::uint32_t actual_lower,std::uint32_t actual_upper);
} // namespace halo2
