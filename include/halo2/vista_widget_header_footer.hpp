#pragma once
#include "halo2/vista_widget_clone_string.hpp"

namespace halo2 {
// Address-shaped body-local banks. Prefix register-save slots are retained
// caller inputs; compiler prologue/epilogue/security machinery is not owned.
// Padding stays explicit. Register the source32 scratch identity and its actual
// span for all lower services; never narrow a host pointer.
using VistaWidgetHeaderScratch=std::array<std::byte,0x430>;
using VistaWidgetFooterScratch=std::array<std::byte,0x428>;
using VistaWidgetFooterLayoutScratch=std::array<std::byte,0x4ac>;
struct VistaWidgetHeaderFooterServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<std::uint32_t()> context; //60BB89.
  std::function<std::uint32_t(std::uint32_t)> definition; //60C701.
  std::function<std::uint32_t(std::uint32_t)> tag; //639623.
  std::function<void(VistaWidgetIdentity,VistaWidgetIdentity)> append; //61208E.
  // Complete61BC81 provider, using source addresses of vector16/bounds8 banks.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t,
      std::uint32_t,std::uint32_t,std::uint32_t)> initialize_text;
  std::function<std::uint32_t(VistaWidgetIdentity)> style; //Virtual4C.
  std::function<std::uint32_t(std::uint32_t)> text; //Style virtualC.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> assign_text; //61BAA9.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> header_locale; //61BF85.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> footer_locale; //61C16C.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::uint32_t)> text_child; //611973.
  std::function<std::uint8_t(VistaWidgetIdentity,std::uint32_t,std::uint32_t,
      std::uint32_t)> animation_descriptor; //612250, output34h source bank.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> detach_kinds; //610257.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> bounds; //Virtual20, output8.
  //611EA1(rect8,prefix count,result8,flag0), with genuine lower text/font owner.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t,
      std::uint32_t,std::uint32_t)> measure;
  std::function<VistaWidgetIdentity(std::uint32_t,std::uint32_t)> allocate; //60D2D8.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity,std::uint32_t)> construct_text; //60F65D.
  VistaUtf16FailureServices failure; //Only reached CRT invalid-input paths.
  std::uint32_t mxcsr{}; //Explicit masked x86 SSE control for610B4E arithmetic.
};
// Complete60FD41/60FF73 orchestration. Lower text/font/localization services are
// lazy and mandatory when reached; no guessed font success or width is supplied.
void vista_widget_header_initialize(VistaWidgetIdentity,std::uint32_t scratch_identity,
    std::span<std::byte,0x430>,const VistaWidgetHeaderFooterServices&);
void vista_widget_footer_initialize(VistaWidgetIdentity,std::uint32_t append_argument,
    std::uint32_t scratch_identity,std::span<std::byte,0x428>,
    const VistaWidgetHeaderFooterServices&);
// Complete610B4E: authored/embedded footer choice, animation position, segment
// parsing, generated-child creation, measurement and reverse layout. Missing
// source banks (including NULL allocation dereferences) remain failures after
// their exact partial writes. Caller registry/banks stay live through callbacks.
void vista_widget_footer_layout(VistaWidgetIdentity,std::uint32_t scratch_identity,
    std::span<std::byte,0x4ac>,const VistaWidgetHeaderFooterServices&);
} //namespace halo2
