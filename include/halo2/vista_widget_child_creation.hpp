#pragma once
#include "halo2/vista_widget_asset_birth.hpp"

namespace halo2 {
struct VistaWidgetChildCreationServices {
  // Genuine retained allocation service60D2D8(size,0). A null result skips
  // construction and all later work. Source32 storage remains live throughout.
  std::function<VistaWidgetIdentity(std::uint32_t,std::uint32_t)> allocate;
  VistaWidgetAssetBirthServices birth;
  // Complete620493, including nested skin creation and merged bounds.
  std::function<void(VistaWidgetIdentity,std::span<const std::byte,4>)> grid_initialize;
  // Complete61BB0B and61BAA9 respectively. Text/font/style ownership belongs
  // to these genuine lower services; no substitute label or width is supplied.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> text_descriptor;
  std::function<void(VistaWidgetIdentity,std::uint32_t)> assign_text;
};
// Complete640D6E kind9 birth. Its raw36-byte record is independent of the
// retained3D title backdrop. Bitmap residency is a void request, not success.
VistaWidgetIdentity vista_widget_kind9_construct(VistaWidgetIdentity,
    std::uint32_t definition,const VistaWidgetAssetBirthServices&);
// Complete64268E/6426FB through original61B9B1. Controller is an index here,
// converted by the source's masked32-bit shift (NONE maps to a zero mask).
VistaWidgetIdentity vista_widget_skin_text_construct(VistaWidgetIdentity,
    std::uint32_t controller_index,bool short_string,
    const VistaWidgetAssetBirthServices&);
// Complete642425/6424A9/64252D/642786. Allocation, construction, ownership
// byte, append/reparent and initialization retain source order. Text chooses
// its class from the record's lowBYTE bit4, and samples the parent's controller
// WORD after allocation. It initializes the descriptor before assigning the
// original empty UTF16 literal79BD7C. Missing consumed lower services reject.
VistaWidgetIdentity vista_widget_create_bitmap(VistaWidgetIdentity parent,
    std::uint32_t definition,const VistaWidgetChildCreationServices&);
VistaWidgetIdentity vista_widget_create_kind9(VistaWidgetIdentity parent,
    std::uint32_t definition,const VistaWidgetChildCreationServices&);
VistaWidgetIdentity vista_widget_create_grid(VistaWidgetIdentity parent,
    std::span<const std::byte,4> position,std::uint32_t row,
    std::uint32_t definition,const VistaWidgetChildCreationServices&);
VistaWidgetIdentity vista_widget_create_text(VistaWidgetIdentity parent,
    std::uint32_t ordinal,std::uint32_t definition,
    const VistaWidgetChildCreationServices&);
} //namespace halo2
