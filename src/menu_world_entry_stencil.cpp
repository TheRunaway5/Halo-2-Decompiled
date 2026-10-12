#include "halo2/menu_world_entry_stencil.hpp"

namespace halo2 {
namespace {
const SourceBspAdmissionRecord &unique_owner(
    std::span<const SourceBspAdmissionRecord> records,std::uint32_t owner) {
  const SourceBspAdmissionRecord *found=nullptr;
  for(const auto &record:records)if(record.id==owner) {
    if(found)throw FormatError("ordinary stencil queue has an ambiguous source owner");
    found=&record;
  }
  if(!found)throw FormatError("ordinary stencil draw has no admitted source queue owner");
  return *found;
}
bool queue_marked(const SourceBspAdmissionRecord &record) {
  // Original1332F0: (queueFlags &0200)<<5. Its other terms and the ordinary
  // BSP/object additions cannot create4000. Keep the two flag domains apart.
  return (record.flags&0x200u)!=0;
}
}
bool menu_world_bsp_entry_stencil_marked(const SourceBspAdmission &admission,
    int geometry) {
  if(geometry==-1)throw FormatError("ordinary stencil BSP input has no source geometry owner");
  if(geometry>=0) {
    if(geometry>=512)throw FormatError("ordinary stencil BSP cluster exceeds its source index bank");
    return queue_marked(unique_owner(admission.clusters,std::uint32_t(geometry)));
  }
  const auto id=-std::int64_t(geometry)-2;
  if(id>=32768)
    throw FormatError("ordinary stencil BSP instance exceeds its source owner field");
  return queue_marked(unique_owner(admission.instances,std::uint32_t(id)));
}
bool menu_world_object_entry_stencil_marked(const SourceObjectAdmission &admission,
    std::uint32_t owner,std::span<const MenuWorldObjectCollectorCopy> path) {
  if(owner==0xffffffffu)throw FormatError("ordinary stencil object owner is NONE");
  for(const auto &record:admission.objects)
    if(record.id!=owner && (record.id&0xffffu)==(owner&0xffffu))
      throw FormatError("ordinary stencil object queue has an ambiguous owner generation");
  bool marked=queue_marked(unique_owner(admission.objects,owner));
  for(const auto copy:path)switch(copy) {
    case MenuWorldObjectCollectorCopy::attached_child:
    case MenuWorldObjectCollectorCopy::material_overlay_1:
    case MenuWorldObjectCollectorCopy::material_overlay_20:
      // Child preserves+1AC and only changes+1B0 bit40/bit1. The two material
      // overlay copies OR8000 into+1AC, preserving the stencil predicate.
      break;
    case MenuWorldObjectCollectorCopy::recursive_effect_80:
    case MenuWorldObjectCollectorCopy::recursive_effect_2:
      // Both original recursive copies REPLACE+1AC from+1B0. For this ordinary
      // queue-origin corridor+1B0 is209/248 (child changes40/1 only); no4000.
      marked=false;break;
    default:throw FormatError("ordinary stencil collector copy branch is not established");
  }
  return marked;
}
} // namespace halo2
