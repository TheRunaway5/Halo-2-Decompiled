#pragma once
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_object_admission.hpp"

namespace halo2 {
// Actual133AB0 collector copy branches, supplied in root-to-draw order. This
// enum does not choose a branch from a material name, source layer or owner.
enum class MenuWorldObjectCollectorCopy : std::uint32_t {
  unspecified=0,attached_child=1,material_overlay_1=2,material_overlay_20=3,
  recursive_effect_80=4,recursive_effect_2=5
};
//1332F0 plus the actual133390/133520 additions. Require an admitted owner;
// dynamic encoding-1 is not BSP-owned. No captured default ref0 is used.
bool menu_world_bsp_entry_stencil_marked(const SourceBspAdmission &,
    int source_geometry);
//133790's queue-owned+1AC/+1B0, the explicit133AB0 copy path, and133F70's
// stencil predicate. A supplied empty path means the initial ordinary record.
// This bounded producer knows only4000; it does not fabricate the full entry
// flags, select visibility/attachments/effects, or establish485A75/76.
bool menu_world_object_entry_stencil_marked(const SourceObjectAdmission &,
    std::uint32_t full_queue_owner,std::span<const MenuWorldObjectCollectorCopy> copy_path);
} // namespace halo2
