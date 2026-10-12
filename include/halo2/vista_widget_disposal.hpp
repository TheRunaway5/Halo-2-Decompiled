#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
struct VistaWidgetDisposalServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<void(VistaWidgetIdentity,std::uint32_t)> shutdown; //Actual virtual8 target.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> destructor; //Actual virtual0 target,arg0.
  std::function<void(VistaWidgetIdentity)> free; //60D2EA.
};
// Complete611891, including live post-write next/previous reads under aliases.
void vista_widget_disposal_detach(VistaWidgetIdentity parent,VistaWidgetIdentity child,
    const VistaWidgetDisposalServices&);
// Complete610257. first/last are raw DWORD arguments compared as signed32;
// slots are source32 screen+A04+index*4. No guessed kind/index restriction.
void vista_widget_dispose_kinds(VistaWidgetIdentity,std::uint32_t first,
    std::uint32_t last,const VistaWidgetDisposalServices&);
} //namespace halo2
