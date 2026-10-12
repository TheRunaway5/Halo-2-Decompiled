#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2 {
// Actual64-byte stage bank fields copied by18A90 from16F60's parameter bank.
// scalar is the literal third float in the authored postprocess bitmap record.
struct SourceBitmapStageResidency {
    std::uint32_t datum=0xffffffffu,image_count{};
    std::uint8_t flags{};
    float frame{},scalar{},minimum_request=3.f;
};
struct SourceBitmapResidencyView {
    float coverage_adjustment{}; // Actual16B90 context+2C; explicit x87 producer.
    bool view_modifier{};        // Actual4BA019.
    std::uint8_t enabled_stages{}; // Context+1420, four actual texture stages.
};
struct SourceBitmapResidencyRequest {
    unsigned stage{};
    std::uint32_t datum=0xffffffffu;
    std::int16_t image_index{}; //143C0 consumes CX, even if wrapping chose negative.
    float fractional_request{}; // XMM2, clamped0..4, distinct from the bucket.
};
// Complete1B0A0's finite authored frame/scalar and valid stage-state branch.
// The returned ordered requests are the actual143C0 service calls. minimum
// changes after a request even if the supplied resident service later fails.
// This does not replace12310/12CCF0 cache deadlines, partitions or async loads.
std::vector<SourceBitmapResidencyRequest> source_bitmap_residency_requests(
    std::array<SourceBitmapStageResidency,4> &,const SourceBitmapResidencyView &);
struct SourceBitmapResidentPartition {
    std::uint32_t handle=0xffffffffu,texture_token{};
    bool ready{};
    std::int32_t last_use{};
};
struct SourceBitmapResidentState {
    std::uint8_t flags{},bias{}; // Bitmap+F/+16.
    float cutoff{};             // Bitmap+58.
    std::uint32_t cached_texture{}; // Bitmap+50.
    std::int32_t cached_until{};    // Bitmap+70.
    std::array<SourceBitmapResidentPartition,3> partitions; // +28 handles.
    std::optional<std::uint8_t> software_category; // Actual bitmap+17.
    std::int32_t software_control{},software_pending_size{}; // +5C/+64.
    std::uint32_t software_texture{}; // Actual inline header/resource at+5C.
};
struct SourceBitmapResidentContext {
    std::uint32_t request_flags=3; //12310 passes3; CPU D15E0 passes0.
    bool bias_enabled{},software_images{}; //468841/4E647B.
    float global_bias{}; //4E647C.
    std::int32_t clock{},use_clock{}; //4E6488/cachebank+38.
    bool software_cache_enabled{}; // Actual4E6468, consumed by12C960.
};
struct SourceBitmapResidentPeek {
    std::uint32_t texture_token{};
    bool requires_request{}; //Actual12310 would now call12CE00(bitmap,3).
};
// Complete12310/D15E0→12CCF0 for the actual hardware-image cache branch. Native
// allocation/IO is separate and is never replaced with a default texture here.
// Existing12C960 inline software headers are admitted with explicit category,
// control and resource inputs. Missing-header12C820 construction is separate.
SourceBitmapResidentPeek source_bitmap_resident_peek(SourceBitmapResidentState &,
    float fractional_request,const SourceBitmapResidentContext &);
} // namespace halo2
