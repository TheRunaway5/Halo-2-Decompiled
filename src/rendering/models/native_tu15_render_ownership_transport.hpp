#pragma once
#include "rendering/models/native_tu15_render_ownership_detail.hpp"

namespace halo2::render_ownership_detail {
// Narrow compatibility view: the semantic kernels never receive this frame.
// A generated caller lends its GP registers and condition codes directly.
struct Registers {
    std::array<std::uint32_t, 8>& gp;
    std::uint32_t& eflags;
    void* blend_context;
    void (*publish_blend)(void*, std::uint32_t);
};
void run_transport(SourcePhysicsContactMemory&, std::uint32_t, Registers,
    NativeTu15PhysicsFrameFp&, const SourcePhysicsContactRecordObserver&, bool live_sse = false);
}
