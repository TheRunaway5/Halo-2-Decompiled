#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaWidgetGridBounds=std::array<std::byte,8>;
struct VistaWidgetGridInitializeServices {
  // Retained source32 storage, valid through both lower callbacks. Raw reads
  // preserve the original order and widths (BYTE, WORD or DWORD), including
  // the639640 tag-table/global loads. Providers must supply real cache bytes.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  // Actual642910 receives the source WORD point, unchanged resolved skin
  // identity and retained grid. It must construct the real skin children.
  std::function<void(VistaWidgetIdentity,std::span<const std::byte,4>,std::uint32_t)> skin_children;
  // Actual6425BF consumes this grid's direct child bounds after construction.
  std::function<VistaWidgetGridBounds(VistaWidgetIdentity)> child_bounds;
};
// Complete620493. Source signedBYTE dimensions/order, signedDWORD row division
// and WORD positioning retain their original admission/arithmetic. Position
// storage stays borrowed until the point is computed. NONE skin datum returns
// untouched, before skin resolution, position reads or lower service calls.
// Lower services are required only if reached. Provider failure propagates
// after its retained bank effects; no substitute children/bounds are returned.
void vista_widget_grid_initialize(VistaWidgetIdentity,std::span<const std::byte,4> position,
    const VistaWidgetGridInitializeServices&);
} //namespace halo2
