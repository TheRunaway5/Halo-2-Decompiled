#pragma once
#include "halo2/source_bitmap_residency.hpp"

namespace halo2 {
// These are the authored +1C/+34 fields, independent of current +28 handles.
struct SourceBitmapCpuResource {
    std::uint32_t location=0xffffffffu;
    std::int32_t size{};
};
struct SourceBitmapCpuState {
    SourceBitmapResidentState resident;
    std::array<SourceBitmapCpuResource,3> resources;
    std::uint8_t flags_low{}; // Record+E, including its sign bit.
    std::int16_t type{};      // Record+A; only used for the original default.
};
struct SourceBitmapCpuContext {
    SourceBitmapResidentContext cache{0}; // D15E0 always passes request flags0.
    bool force_zero{},force_one{}; // 46883D then46883C, in this order.
    std::int32_t hardware_mode{},hardware_category{}; // 4E6470/4E6474.
    bool fallback_marked{}; // Actual shared flag4E647A, retained on entry.
    std::array<std::uint32_t,3> default_textures{}; // 485AE4/8/EC.
};
enum class SourceBitmapCpuBoundary : std::uint32_t {
    Complete,
    InlineHeaderConstruction // Actual12C820 entry; no header result supplied.
};
struct SourceBitmapCpuSelection {
    std::uint32_t texture_token{};
    SourceBitmapCpuBoundary boundary=SourceBitmapCpuBoundary::Complete;
    bool request_entered{}; // D15E0 actually reached12CE00.
    unsigned requested_partition=3; // Actual12C990 input;3 means no call.
};
// Complete D15E0 ->12CCF0 ->12CE00 ready-image selection for explicit current
// banks, with separately rounded source bias arithmetic. A missing inline
// header stops at12C820 rather than inventing storage or readiness. The CPU
// flags0 contract cannot enter12D5A0/120CE0 allocation, priority or wait paths.
// Callers must not use this helper as a native residency-history producer.
SourceBitmapCpuSelection source_bitmap_cpu_select(SourceBitmapCpuState &,
    float raw_request,SourceBitmapCpuContext &);
} // namespace halo2
