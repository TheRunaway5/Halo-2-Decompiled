#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Whole original CPU owners1FB90,20E30,20E90 and an explicitly bounded
//12822 suffix entry for differential validation, including the complete CRC
// owners166A80/166AE0. The real game arena remains a required input. They do not create a
// graphics device or admit renderer32's earlier queue/texture constructors.
void mount_native_tu15_renderer_tail_storage_data(SourceRuntimeMemory&);
bool source_renderer_tail_storage_tu15_owns_entry(std::uint32_t);
void source_run_renderer_tail_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});

struct NativeTu15RendererTailStorageViews {
    NativeRuntimeAddress state_1fb90{}; // Original1FB90's0x3FC arena bytes.
    NativeRuntimeAddress state_20e90{};    // Original20E90's0x24 arena bytes.
    NativeRuntimeAddress state_12560{};  // Original12560's0x34 arena bytes.
    NativeRuntimeAddress query_bank{};    // Original12560's0x40C0 static bank.
};
// Borrowed wide views into actual owned pages. Rebind after bank replacement;
// the constructor's unspecified arena bytes are deliberately left untouched.
NativeTu15RendererTailStorageViews native_tu15_renderer_tail_storage_views(
    const SourceRuntimeMemory&);
// Native composition of12560's CPU-only12822..1287E storage prefix. The
// caller invokes complete original20E30/126850/20E90 over the same arena.
//126850's aligned allocation is identical for the literal0x34 request.
// No prior GPU-call result or renderer-initialized flag is supplied/written.
using SourceRendererCpuOriginalCall=std::function<std::uint32_t(
    std::uint32_t entry,std::optional<std::uint32_t> eax)>;
void source_renderer_cpu_interface_initialize_tu15(SourceRuntimeMemory &,
    const SourceRendererCpuOriginalCall &);
}
