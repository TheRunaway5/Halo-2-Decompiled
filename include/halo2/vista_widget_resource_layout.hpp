#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace halo2 {
struct VistaWidgetResourceLayoutServices {
  // Source32 resident bank registry. No host-pointer narrowing or zero-filled
  // fallback. Registry reads are ordinary memory; callbacks can mutate banks.
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  // Actual61D228, void bitmap residency request. A reached request is mandatory;
  // return values are neither invented nor interpreted as successful loading.
  std::function<void(std::uint32_t datum)> request_bitmap;
  // Actual48AA97 this=manager, argument=resolved mcsr cursor resource.42EDAF
  // invokes it whenever87A5C4 is nonzero, including resourceNULL. The actual
  // PC14/23/17/206 +6C datum is mcsr cursors\\default\\default, not an audio tag.
  std::function<void(std::uint32_t manager,std::uint32_t resource)> apply_cursor_resource;
};

// Complete60F790 and small source leaves60C701/60C688/60BB1D/639623/63965D,
// 781C57/42EDAF. Writes+9FA before lookup; reloads signed count+38 and pointer+3C
// after each bitmap service; resolves+6C before consulting the actual cursor
// manager; clears+9FE only at the normal tail. Faults/callback exceptions leave
// the source write prefix intact. Cached-loaded globals remain caller-owned.
void vista_widget_resource_layout(std::uint32_t widget,
    const VistaWidgetResourceLayoutServices&);
} // namespace halo2
