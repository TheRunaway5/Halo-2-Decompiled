#pragma once
#include <cstddef>

namespace halo2 {
// Host elapsed times around the existing submission path. These are not GPU
// timestamps: fence, acquire, submit and present may include driver waits.
struct RendererFrameTiming {
    bool measured{};
    double fence_ms{}, global_constants_ms{}, view_constants_ms{}, geometry_ms{},
        acquire_ms{}, commands_ms{}, submit_ms{}, present_ms{}, screenshot_ms{};
    std::size_t materials{}, views{}, dynamic_vertices{};
    unsigned width{},height{};
    double geometry_pack_ms{},geometry_upload_ms{},geometry_diagnostics_ms{};
    double geometry_plan_ms{},geometry_bind_ms{};
};
}
