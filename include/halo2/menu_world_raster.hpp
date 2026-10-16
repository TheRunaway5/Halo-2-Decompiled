#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace halo2 {
// These are actual caller layers, not a pass-selection policy. Other layer
// families have not been admitted by this ordinary-world context.
enum class MenuWorldRasterLayer : std::uint32_t {
  base_1=1,base_3=3,emissive_7=7,shadow_10=10,shadow_apply_11=11,
  sphere_diffuse_12=12,planar_fog_16=16
};
enum class MenuWorldRasterPrimitive { unspecified,ordinary_triangles,other };
enum class MenuWorldRasterPolygon { unspecified,point,line,fill };

struct MenuWorldRasterStencilValues {
  // Original NV/GL encodings, translated to the actual backend later.
  std::uint32_t fail{},zfail{},pass{},function{},reference{},read_mask{},write_mask{};
  bool operator==(const MenuWorldRasterStencilValues &) const=default;
};
struct MenuWorldRasterStencil {
  bool enabled{}; // Effective SDK enable: logical enable AND bound depth surface.
  // Mode0 retains values; they can remain unknown while disabled. Do not
  // manufacture defaults for unused values in a backend pipeline key.
  std::optional<MenuWorldRasterStencilValues> values;
};
struct MenuWorldRasterPacket {
  bool window_exclusive{};
  std::array<std::array<std::int32_t,4>,8> regions;
  std::array<float,2> depth_factor_offset;
  std::array<std::int32_t,2> surface_scale;
  MenuWorldRasterStencil stencil;
  std::uint32_t source_stencil_mode{};
};
struct MenuWorldRasterEntryInput {
  // Only the source4000 predicate consumed by this stencil caller. Its
  // producer does not claim the unrelated render-entry flags are known.
  std::optional<bool> stencil_marked_4000;
  std::optional<bool> special_485a75,special_485a76;
};
// Explicit adapter for a caller that actually owns the complete packed entry.
// Queue-owned callers should use their narrow source predicate instead.
constexpr MenuWorldRasterEntryInput menu_world_raster_entry_from_source_flags(
    std::uint32_t flags,std::optional<bool> special_485a75,
    std::optional<bool> special_485a76) {
  return {(flags&0x4000u)!=0,special_485a75,special_485a76};
}
struct MenuWorldRasterSnapshotInput {
  MenuWorldRasterPrimitive primitive=MenuWorldRasterPrimitive::unspecified;
  MenuWorldRasterPolygon front_polygon=MenuWorldRasterPolygon::unspecified;
  // Actual integer source AA times renderer scaling, applied once. Host
  // aspect/viewport size and SDK viewport scale are not this input's producer.
  std::optional<std::array<std::int32_t,2>> surface_scale;
  std::optional<bool> depth_stencil_surface_bound;
};

// Pure, unknown-aware retail caller transitions. No device, renderer, owner,
// material, visibility, view or pass selection. A fresh context knows none of
// the retained hardware state; there is no "normal title defaults" constructor.
class MenuWorldRasterContext {
public:
  // SDK SetViewport's actual SetScissors(0,FALSE,NULL) reset. Dimensions are
  // source device+954/+958; never inferred from a host target or camera.
  // This bounded path requires dimensions1..4095 (original12-bit fields).
  void viewport_reset(std::array<std::uint32_t,2> source_surface_dimensions);
  // Actual game12560 ZBIAS0 transition, not arbitrary device/reset admission.
  void game_initialize_offset();
  // Actual495E0/496A0 pair. Begin updates fill/factor/bias only; end performs
  // ZBIAS0. Separate depth-clip/multisample effects are outside this packet.
  void shadow_begin();
  void shadow_end();
  void layer_begin(MenuWorldRasterLayer);
  void entry_begin(const MenuWorldRasterEntryInput &);
  void entry_end();
  // A draw packet is available only during a supplied, matched entry scope.
  // Unknown required state and unsupported corridors throw FormatError.
  MenuWorldRasterPacket snapshot(const MenuWorldRasterSnapshotInput &) const;
private:
  struct Offset {
    std::array<std::optional<bool>,3> enabled; // point,line,fill
    std::optional<std::uint32_t> factor,bias;
  } offset_;
  struct Entry {bool restore_mode_1{};};
  std::optional<std::array<std::uint32_t,2>> surface_;
  std::optional<MenuWorldRasterLayer> layer_;
  std::optional<bool> stencil_enabled_;
  std::optional<MenuWorldRasterStencilValues> stencil_values_;
  std::optional<std::uint32_t> stencil_mode_;
  std::optional<Entry> entry_;
  bool shadow_active_{}; // Tracks this API's matching transition, not prior state.
  void stencil_mode(std::uint32_t);
  void no_entry() const;
  void offset_zero();
};
} // namespace halo2
