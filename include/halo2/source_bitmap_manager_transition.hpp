#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
struct SourceBitmapManagerRegion {
    std::uint32_t address{};
    std::span<std::byte> bytes;
};
// Explicit owned storage. All retained source pointers, salts and untouched
// payload bytes remain in their original raw banks; these regions allocate
// nothing and do not manufacture an original kernel outcome.
struct SourceBitmapManagerBanks {
    SourceBitmapManagerRegion manager,image_pool,physical_pool;
    SourceBitmapManagerRegion image_rows,physical_rows,image_bits,physical_bits;
    std::span<SourceBitmapManagerRegion> owners;
};
struct SourceBitmapManagerFence {
    std::uint32_t queued{},completed{};
};
struct SourceBitmapManagerContext {
    std::optional<std::uint8_t> cache_busy_override; // Actual4E6479, if consumed.
    std::optional<SourceBitmapManagerFence> fence; // Actual device fence RAM.
};
enum class SourceBitmapManagerBoundary : std::uint32_t {
    Complete,
    NeedsWorkMutexAcquire // First real NtWaitForSingleObjectEx80020F5A entry.
};
// Complete12D160/12D1A0 queries. Lookup uses the low16 handle bits, as source.
// Busy may clear a completed texture resource fence at image+1C.
std::uint8_t source_bitmap_image_state(const SourceBitmapManagerBanks &,std::uint32_t handle);
bool source_bitmap_image_busy(SourceBitmapManagerBanks &,std::uint32_t handle,
    const SourceBitmapManagerContext &);
// Complete12D200 and13D830 for admitted occupied matching slots. Deletion
// requires actual exclusive bank ownership. A busy callback reports its first
// service boundary without releasing metadata or claiming a completed wait.
// Ready+3 is an actual input; none of these functions sets it true.
SourceBitmapManagerBoundary source_bitmap_image_delete(SourceBitmapManagerBanks &,
    std::uint32_t handle,const SourceBitmapManagerContext &);
SourceBitmapManagerBoundary source_bitmap_manager_delete(SourceBitmapManagerBanks &,
    std::uint32_t handle,const SourceBitmapManagerContext &);
} // namespace halo2
