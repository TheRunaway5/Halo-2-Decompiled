#pragma once
#include "halo2/vista_widget_clone_string.hpp"

namespace halo2 {
struct VistaWidgetTextAssignmentServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<VistaWidgetIdentity(VistaWidgetIdentity)> style; //Actual virtual4C.
  std::function<std::uint32_t(VistaWidgetIdentity,VistaUtf16View)> assign_string; //Style virtual4.
  std::function<std::uint8_t(VistaUtf16View,std::uint32_t)> prepare_text; //499D1F.
};
//Complete61BAA9 tail dispatch. Input and style banks remain borrowed and live;
//the style provider may mutate their contents before assignment begins.
std::uint32_t vista_widget_text_assign(VistaWidgetIdentity,VistaUtf16View,
    const VistaWidgetTextAssignmentServices&);
//Complete61BAB9. Reads the first style's font before preparation; only a true
//original AL result reaches a fresh style lookup and assignment. No readiness
//result is supplied here. The original return value is not a defined contract.
void vista_widget_text_prepare_assign(VistaWidgetIdentity,VistaUtf16View,
    const VistaWidgetTextAssignmentServices&);
//Complete61BA98: actual virtual4C followed by its font DWORD write.
void vista_widget_text_font(VistaWidgetIdentity,std::uint32_t,
    const VistaWidgetTextAssignmentServices&);
//Complete60F425/60F4C9, including the original32/512 UTF16-unit capacities,
//full forward-overlap copy behavior, fresh style virtualC and signed counter.
//Clone's separate1024-unit owner remains643DDB in vista_widget_clone_string.
std::uint32_t vista_widget_short_string_assign(VistaWidgetIdentity,VistaUtf16View,
    const VistaCloneStringServices&);
std::uint32_t vista_widget_long_string_assign(VistaWidgetIdentity,VistaUtf16View,
    const VistaCloneStringServices&);
//Complete60F5E3/60F6CA and60F4A7/60F554 literal source32 LEA arithmetic.
VistaWidgetIdentity vista_widget_skin_style_identity(VistaWidgetIdentity);
std::uint32_t vista_widget_skin_text_identity(VistaWidgetIdentity);
} //namespace halo2
