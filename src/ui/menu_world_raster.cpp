#include "halo2/menu_world_raster.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <limits>

namespace halo2 {
namespace {
bool base(MenuWorldRasterLayer layer) {
  return layer==MenuWorldRasterLayer::base_1 || layer==MenuWorldRasterLayer::base_3;
}
bool supported(MenuWorldRasterLayer layer) {
  return base(layer) || layer==MenuWorldRasterLayer::emissive_7 ||
      layer==MenuWorldRasterLayer::shadow_10 || layer==MenuWorldRasterLayer::shadow_apply_11 ||
      layer==MenuWorldRasterLayer::sphere_diffuse_12 ||
      layer==MenuWorldRasterLayer::planar_fog_16;
}
}
void MenuWorldRasterContext::no_entry() const {
  if(entry_)throw FormatError("ordinary raster transition interrupts an active source entry");
}
void MenuWorldRasterContext::viewport_reset(std::array<std::uint32_t,2> dimensions) {
  no_entry();
  for(auto value:dimensions)if(!value || value>4095)
    throw FormatError("ordinary raster source surface is outside the admitted12-bit corridor");
  surface_=dimensions;
}
void MenuWorldRasterContext::offset_zero() {
  offset_.enabled={false,false,false};
  // Actual3F59B0(0) stores -0 in both registers. The fragment producer below
  // supplies positive zero when the matching polygon-enable bit is clear.
  offset_.factor=0x80000000u;offset_.bias=0x80000000u;
}
void MenuWorldRasterContext::game_initialize_offset() {
  no_entry();
  if(shadow_active_)throw FormatError("ordinary raster initialization interrupts a shadow pair");
  offset_zero();
}
void MenuWorldRasterContext::shadow_begin() {
  no_entry();
  if(shadow_active_)throw FormatError("ordinary raster shadow setup is already active");
  shadow_active_=true;
  //495E0 does not establish the point/line enable bits. Preserve unknowns.
  offset_.enabled[2]=true;offset_.factor=0x40000000u;offset_.bias=0x44098000u;
}
void MenuWorldRasterContext::shadow_end() {
  no_entry();
  if(!shadow_active_)throw FormatError("ordinary raster shadow teardown has no matching setup");
  offset_zero();shadow_active_=false;
}
void MenuWorldRasterContext::stencil_mode(std::uint32_t mode) {
  if(mode>2)throw FormatError("ordinary raster stencil mode is not admitted");
  stencil_mode_=mode;stencil_enabled_=mode!=0;
  if(mode)stencil_values_=MenuWorldRasterStencilValues{
      0x1E00,0x1E00,0x1E01,0x207,mode==1?0u:255u,255,255};
}
void MenuWorldRasterContext::layer_begin(MenuWorldRasterLayer layer) {
  no_entry();
  if(!supported(layer))throw FormatError("ordinary raster source layer is not admitted");
  if(layer==MenuWorldRasterLayer::shadow_10 && !shadow_active_)
    throw FormatError("ordinary raster shadow layer omits its source setup");
  if(shadow_active_ && layer!=MenuWorldRasterLayer::shadow_10)
    throw FormatError("ordinary raster layer transition omits shadow teardown");
  layer_=layer;stencil_mode(base(layer)?1u:0u);
}
void MenuWorldRasterContext::entry_begin(const MenuWorldRasterEntryInput &input) {
  no_entry();
  if(!layer_)throw FormatError("ordinary raster entry has no established source layer");
  if(*layer_==MenuWorldRasterLayer::shadow_10 && !shadow_active_)
    throw FormatError("ordinary raster shadow entry has no active source setup");
  if(shadow_active_ && *layer_!=MenuWorldRasterLayer::shadow_10)
    throw FormatError("ordinary raster shadow entry precedes its matching layer transition");
  if(!input.stencil_marked_4000 || !input.special_485a75 || !input.special_485a76)
    throw FormatError("ordinary raster entry is missing its source stencil predicate or special context");
  const bool special=*input.special_485a75 || *input.special_485a76;
  const bool marked=*input.stencil_marked_4000;
  const bool restore=base(*layer_) && (special || marked);
  if(base(*layer_)) {
    if(special)stencil_mode(0);
    else if(marked)stencil_mode(2);
  }
  entry_=Entry{restore};
}
void MenuWorldRasterContext::entry_end() {
  if(!entry_)throw FormatError("ordinary raster entry teardown has no matching setup");
  if(entry_->restore_mode_1)stencil_mode(1);
  entry_.reset();
}
MenuWorldRasterPacket MenuWorldRasterContext::snapshot(const MenuWorldRasterSnapshotInput &input) const {
  if(!entry_ || !surface_ || !stencil_enabled_ || !stencil_mode_)
    throw FormatError("ordinary raster snapshot is missing its source lifecycle state");
  if(input.primitive!=MenuWorldRasterPrimitive::ordinary_triangles)
    throw FormatError("ordinary raster snapshot requires source triangles");
  unsigned polygon;
  switch(input.front_polygon) {
    case MenuWorldRasterPolygon::point:polygon=0;break;
    case MenuWorldRasterPolygon::line:polygon=1;break;
    case MenuWorldRasterPolygon::fill:polygon=2;break;
    default:throw FormatError("ordinary raster snapshot is missing its source polygon mode");
  }
  if(!input.surface_scale || !input.depth_stencil_surface_bound)
    throw FormatError("ordinary raster snapshot is missing actual scale or depth attachment presence");
  MenuWorldRasterPacket packet{}; // Every required field is assigned below.
  packet.window_exclusive=false;packet.surface_scale=*input.surface_scale;
  std::array<std::int32_t,4> region{};
  for(unsigned axis=0;axis<2;++axis) {
    const auto scale=packet.surface_scale[axis];
    if(scale<=0)throw FormatError("ordinary raster surface scale must be positive");
    const auto maximum=(std::uint64_t((*surface_)[axis])+1u)*std::uint32_t(scale);
    if(maximum>std::uint32_t(std::numeric_limits<std::int32_t>::max()))
      throw FormatError("ordinary raster scaled window exceeds its signed shader field");
    region[axis+2]=std::int32_t(maximum);
  }
  packet.regions.fill(region);
  if(!offset_.enabled[polygon])
    throw FormatError("ordinary raster matching polygon offset-enable bit is unknown");
  packet.depth_factor_offset={0.0f,0.0f};
  if(*offset_.enabled[polygon]) {
    if(!offset_.factor || !offset_.bias)
      throw FormatError("ordinary raster enabled polygon offset registers are unknown");
    packet.depth_factor_offset={std::bit_cast<float>(*offset_.factor),std::bit_cast<float>(*offset_.bias)};
  }
  if(*stencil_enabled_ && !stencil_values_)
    throw FormatError("ordinary raster enabled stencil values are unknown");
  packet.stencil={*stencil_enabled_ && *input.depth_stencil_surface_bound,stencil_values_};
  packet.source_stencil_mode=*stencil_mode_;
  return packet;
}
} // namespace halo2
