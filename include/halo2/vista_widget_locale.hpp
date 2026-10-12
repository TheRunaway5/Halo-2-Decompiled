#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
// All identities name live source32 memory. Callers register the real scratch
// banks; no host pointer is narrowed and no locale/style result is synthesized.
struct VistaWidgetLocaleServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<std::uint32_t()> interface; //60BB1D.
  std::function<std::uint32_t(std::uint32_t)> globals; //63967A.
  std::function<std::uint32_t(std::uint32_t)> definition; //60C701.
  std::function<std::uint32_t(std::uint32_t)> screen_tag; //639623.
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> localized; //43E3AC(list,SID,out).
  std::function<std::uint32_t(VistaWidgetIdentity,std::uint32_t)> style; //Actual widget virtual4C target.
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> assign; //Actual style virtual4 target,this,out.
};
std::uint32_t vista_widget_locale_context(const VistaWidgetLocaleServices&); //60BB89.
VistaWidgetIdentity vista_widget_locale_root(VistaWidgetIdentity,const VistaWidgetLocaleServices&); //611BA1.
void vista_widget_screen_locale(VistaWidgetIdentity,std::uint32_t string_id,
    std::uint32_t output,const VistaWidgetLocaleServices&); //6101A4.
// Complete60BEBB. frame is the body ESP after its saved ESI: +4..+D3
// is the 26-pair authored enum/SID table, +D8 the live enum argument and
// +DC the live output argument. Prefix/padding bytes stay caller-owned.
void vista_widget_footer_locale_lookup(std::uint32_t frame,const VistaWidgetLocaleServices&);
// Complete61BF85. string_id is the original entry-captured argument; scratch
// names its 400-byte bank. Only the first WORD is cleared before resolution.
void vista_widget_header_locale(VistaWidgetIdentity,std::uint32_t string_id,
    std::uint32_t scratch,const VistaWidgetLocaleServices&);
// Complete61C16C. The enum argument is read after the context callback. The
// explicit lookup frame above retains lower call arguments and table writes.
void vista_widget_footer_locale(VistaWidgetIdentity,std::uint32_t enum_argument,
    std::uint32_t scratch,std::uint32_t lookup_frame,const VistaWidgetLocaleServices&);
} //namespace halo2
