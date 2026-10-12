#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
struct VistaWidgetAssetBirthServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  // Exact source-address reads, including cache globals/table/tag/record data.
  // Reads may expose callback mutations; missing bytes are never zero-filled.
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<std::uint32_t()> cached_ui_clock;
  // Original61D228 is a void residency request, not a resident-handle query.
  std::function<void(std::uint32_t)> bitmap_residency;
  // Caller-owned SSE context. Only masked exceptions are admitted; DAZ affects
  // the source UCOMISS zero tests. No arithmetic changes scale payload bits.
  std::uint32_t mxcsr{};
};
// Complete61C3FB bitmap constructor; consumes98h bytes and the raw38h authored
// definition. Reuses the complete611D81 birth. Keeps the original second clock,
// raw definition identity, cached first-frame bounds and requested-frame rules.
VistaWidgetIdentity vista_widget_bitmap_construct(VistaWidgetIdentity,
    std::uint32_t definition,const VistaWidgetAssetBirthServices&);
// Complete62044F/642405 grid birth; no skin children or initialization inferred.
VistaWidgetIdentity vista_widget_grid_construct(VistaWidgetIdentity,
    std::uint32_t row,std::uint32_t definition,const VistaWidgetAssetBirthServices&);
} //namespace halo2
