#pragma once
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <functional>
namespace halo2 {
using SourceShadowMainInputWriterTu15=std::function<void(std::uint32_t,std::span<const std::byte>)>;
struct SourceShadowMainDrawHandoffTu15 {
    unsigned units{},mask_words{},associations{},overrides{},sections{},mask_relocations{},instance_relocations{};
};
// Collected ordinary2CBA0 state at its pre-44710 boundary. This publishes real
// source-produced state; it does not claim main culling or GPU submission.
// Pointer relocation binds the caller's actual range/instance data to the
// existing TU1.5 context/BSP allocations, with byte validation before writes.
SourceShadowMainDrawHandoffTu15 source_live_shadow_main_draws_tu15(
    const SourceRuntimeMemory&,const SourceDrawUnitState&,const SourceBspAdmission&,
    std::span<const std::array<std::uint32_t,4>> associations,
    std::uint32_t mask_token,std::uint32_t instance_token,std::span<const Transform>,
    const SourceShadowMainInputWriterTu15&);
}
