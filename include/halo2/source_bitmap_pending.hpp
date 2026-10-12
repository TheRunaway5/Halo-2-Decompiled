#pragma once
#include "halo2/source_bitmap_async_read.hpp"
#include "halo2/source_bitmap_reservation.hpp"

namespace halo2 {
struct SourceBitmapPendingAddressRange {
    std::uint32_t address{},bytes{};
};
struct SourceBitmapPendingContext {
    SourceBitmapManagerContext manager;
    std::uint32_t physical_base{};
    SourceBitmapPendingAddressRange physical_storage;
    SourceBitmapFileBank files;
    // Address/extent identity used only for alias admission. The normalized
    // typed queue fields are mapped independently to original source globals;
    // sizeof(SourceAsyncQueue) is not their literal field layout.
    std::uint32_t queue_address{};
    std::optional<std::uint32_t> free_mutex,work_mutex;
};
struct SourceBitmapPendingInput {
    std::uint32_t owner_address{},partition{};
    std::uint8_t wait{},force{};
    std::int32_t priority{};
};
enum class SourceBitmapPendingBoundary : std::uint32_t {
    CompletedSourceFailure,
    NeedsWorkMutexAcquire,
    NeedsFreeMutexAcquire
};
struct SourceBitmapPendingResult {
    SourceBitmapPendingBoundary boundary{SourceBitmapPendingBoundary::CompletedSourceFailure};
    std::uint32_t handle{0xffffffffu},pending_delete{0xffffffffu};
    std::optional<SourceBitmapReadRequest> read;
    std::optional<SourceAsyncRequest> request;
    std::optional<std::uint32_t> mutex;
};
// Original 12D5A0 composition over supplied owned metadata banks. Header,
// address and nonaliasing admission precedes any mutation of those banks.
// Failure commits all source failure writes. A real work/free mutex boundary
// commits every preceding source write but executes no wait and creates no
// acquired node, job ID, final image priority, loading increment, ready image,
// or successful caller return. Pixel storage is an explicit address extent;
// this prefix never reads, allocates, initializes or writes its pixels.
// Request callback metadata is independently proved by owned 1A0F75 binding;
// its post-acquisition store is not claimed as observed at the wait boundary.
SourceBitmapPendingResult source_bitmap_begin_pending(SourceBitmapManagerBanks &,
    SourceAsyncQueue &,const SourceBitmapPendingInput &,const SourceBitmapPendingContext &);
} // namespace halo2
