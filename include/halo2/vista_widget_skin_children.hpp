#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaWidgetSkinBounds=std::array<std::byte,8>;
struct VistaWidgetSkinServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  // Genuine642425/6424A9 allocate, construct and append kind8/kind9.
  // Kind9 uses640D6E; these skin records are distinct from the3D backdrop.
  // NULL allocation skips that child; the record address is source32.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::uint32_t)> bitmap,kind9;
  //64252D also initializes the nested grid with the borrowed source point.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::span<const std::byte,4>,
      std::uint32_t,std::uint32_t)> grid;
  //642786 chooses the actual text class/controller before append/clone.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> text;
  // Actual child virtual20, which writes all8 bounds bytes. Its class owns
  // animation translation. No authored-bounds substitute is supplied here.
  std::function<VistaWidgetSkinBounds(VistaWidgetIdentity)> bounds;
};
// Complete642910 skin-child orchestration. Retains raw skin/count/block reads,
// source order, independent nested-row reloads and wrapping WORD translation.
// Banks/position remain live across callbacks. This does not implement child
// allocation/class wrappers or complete renderer residency.
void vista_widget_skin_children(VistaWidgetIdentity,std::span<const std::byte,4> position,
    std::uint32_t skin,const VistaWidgetSkinServices&);
} //namespace halo2
