#include "halo2/shield_overlay.hpp"

namespace halo2 {
ShieldOverlaySelection select_shield_overlays(const VitalityDefinition &definition,
    float recent, float shield, const ShieldOverlayGeometry &geometry) {
  ShieldOverlaySelection result;
  if (!geometry.visible || !geometry.model_present || geometry.model_count <= 0 ||
      geometry.lod == 0xff || geometry.mode != 0 || definition.damage_info_count <= 0)
    return result;
  const auto damage = geometry.first_person ? definition.shield_damaged_first_person_shader :
                                              definition.shield_damaged_shader;
  if (recent > 0.f && damage != null_datum) {
    result.flags |= 1;
    result.overlays[result.count++] = {ShieldOverlayKind::Damaged, damage, shield};
  }
  const auto overshield = geometry.first_person ? definition.overshield_first_person_shader :
                                                  definition.overshield_shader;
  if (shield > 1.f && overshield != null_datum) {
    result.flags |= 0x20;
    result.overlays[result.count++] = {ShieldOverlayKind::Overshield, overshield, 1.f - shield};
  }
  return result;
}
}
