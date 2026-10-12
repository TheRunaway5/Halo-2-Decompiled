#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaWidgetTextCloneDescriptor=std::array<std::byte,0x2c>;
using VistaWidgetTextScratch=std::array<std::byte,0x400>;
struct VistaWidgetTextCloneServices {
  // Optional entry observer; supplies no original result or lower service.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::span<const std::byte,0x2c>)> consuming;
  // Retained source32 identity registry; widgets/root require consumed fields,
  //style banks require at least40h bytes. Never cast native pointers to source32.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(std::uint32_t)> definition; //60C701(root+70).
  std::function<std::optional<std::span<const std::byte>>(std::uint32_t)> tag; //639623.
  std::function<void(std::uint32_t,std::uint32_t,std::span<std::byte,0x400>)> localized; //43E3AC.
  std::function<VistaWidgetIdentity(VistaWidgetIdentity)> style; //Widget virtual4C.
  // Actual style virtual4; text includes the first terminator and caller scratch
  //remains live through assignment. No fabricated string ownership/success.
  std::function<void(VistaWidgetIdentity,std::span<const std::byte,0x400>)> assign_string;
  std::function<std::optional<std::span<const std::byte>>()> context; //60BB89.
  std::function<std::span<const std::byte>(VistaWidgetIdentity)> text; //Style virtualC.
  // Actual499D1F font preparation. Text lifetime belongs to actual text owner;
  //font is the exact signed16-to32 value (or1 with descriptor flag8).
  std::function<std::uint8_t(std::span<const std::byte>,std::uint32_t)> prepare_text;
};
// Complete61BB0B with reached611BA1/6101A4/6116D2/62DF53 ownership. The scratch
//is the caller's actual uninitialized buffer input; only source-owned bytes are
//written. Descriptor/style/bounds loads preserve order and may alias widget banks.
//Required lower string/localization/context/font services stay explicit.
void vista_widget_text_descriptor(VistaWidgetIdentity,std::uint32_t identifier,
    std::span<const std::byte,0x2c>,std::span<std::byte,0x400> scratch,
    const VistaWidgetTextCloneServices&);
// Complete61BC81 bridge. Flags sign-extend WORD, animation/font truncate WORD;
//two untouched padding WORDs retain caller scratch. Vector/bounds may alias it.
//It then invokes the complete descriptor consumer above with identifierNONE.
void vista_widget_text_clone_initialize(VistaWidgetIdentity,std::uint32_t flags,
    std::uint32_t animation,std::span<const std::byte,16> vector,
    std::uint32_t font,std::span<const std::byte,8> bounds,
    std::span<std::byte,0x2c> descriptor_scratch,std::span<std::byte,0x400> text_scratch,
    const VistaWidgetTextCloneServices&);
// Separate61BA2A owner; only widget+74 byte changes.
void vista_widget_text_clone_flag(VistaWidgetIdentity,const VistaWidgetTextCloneServices&);
} //namespace halo2
