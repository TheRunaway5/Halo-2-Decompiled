#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// The source unit constructor stores -1 in both byte selectors. These fields
// are independent of the native high-level player State enum.
struct UnitZoomState {
  std::int8_t current = -1;
  std::int8_t requested = -1;
  std::uint32_t motion{}; // Source unit+26C; no motion producer is represented here.
};
struct ZoomWeaponDefinition {
  std::uint32_t flags{};       // Authored weap+12C, binocular prohibition bit16.
  std::int16_t levels{};       // Authored signed weap+1FE.
  float minimum{}, maximum{}; // Authored weap+200/+204.
};
struct ZoomWeaponView {
  ZoomWeaponDefinition definition;
  // All authored magazines in original order, runtime mode from weapon+224+n*10.
  std::span<const std::int16_t> magazine_modes;
};
struct BinocularZoomDefinition {
  std::int32_t levels{};       // Original first matg player-info record+B8.
  float minimum{}, maximum{}; // That record+BC/+C0.
};
struct UnitZoomContext {
  std::uint32_t parent;       // Actual unit+14, NONE is FFFFFFFF.
  std::uint32_t action_word; // Actual component at unit+signed346+4.
  std::array<const ZoomWeaponView *,2> equipped;
};
// C9920 -> E8610/104A30. Context is required; no native State implies admission.
bool source_zoom_admitted(const UnitZoomContext &);
// C9CC0 -> 1051C0. This is a selector producer, not an admission bypass.
std::int16_t source_zoom_next(std::int16_t current,const ZoomWeaponView *primary,
                             const BinocularZoomDefinition &);
// C8140 copies the low incoming selector byte without inventing a clamp.
void source_zoom_request(UnitZoomState &,std::int16_t incoming);
// C6C50's bounded zoom writes. update_enabled is exactly !(unit-def+BC bit10)
// && !(unit-object+10A bit2), supplied from the actual source fields.
bool source_zoom_update(UnitZoomState &,const UnitZoomContext &,bool update_enabled);
// C9A40's three zoom-field stores. Other view/sound/reset effects remain outside.
void source_zoom_reset(UnitZoomState &);
// C6C50 composes this bit into each equipped hand's 1045E0 input argument.
std::uint16_t source_zoom_weapon_input(std::uint16_t other_flags,const UnitZoomState &);
// C9BE0 binocular arithmetic and 105240's one-level weapon branch. A valid
// multi-level weapon needs the original CRT power consumer, which is unported;
// nullopt explicitly preserves that boundary. This does not calculate camera FOV.
std::optional<float> source_zoom_magnification(std::int16_t current,
    const ZoomWeaponView *primary,const BinocularZoomDefinition &);
// Original13B09A HUD prefix: CURRENT is a signed source byte, independent of
// the camera's retained view selector. No admission or presentation is inferred.
struct ZoomHudStatus {
  std::int16_t current;
  std::uint8_t binocular;
};
ZoomHudStatus source_zoom_hud_status(std::int8_t current,const ZoomWeaponView *primary);
// Original13B7C0 zoom/binocular unit-mask subset only (40/80/100/400). The
// caller replaces these bits and independently owns every other HUD condition.
std::uint16_t source_zoom_hud_mask(const ZoomHudStatus &);
} // namespace halo2
