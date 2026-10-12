#pragma once
#include <cstdint>
#include <functional>
#include <optional>
namespace halo2 {
struct SourceCamouflageObjectView {
  std::uint8_t type;       // Actual body+AA, not the object header's kind.
  std::uint32_t parent;   // Body+14, read only when type is not biped0.
};
struct SourceCamouflageContextServices {
  // Original BADC0 validates the first full datum. Subsequent parent access
  // uses the raw low16 object-header slot, independently of salt/occupancy.
  std::function<std::optional<SourceCamouflageObjectView>(std::uint32_t)> initial;
  std::function<std::optional<SourceCamouflageObjectView>(std::uint32_t)> parent;
  // Read body+2B0 only after the first biped has actually been selected.
  std::function<float(std::uint32_t)> primary;
};
// Whole retail33670/TU33660 counterpart behind scalar extern37. This is a
// first-biped walk, distinct from the ultimate-root lighting/shadow lookup.
float source_object_camouflage(std::uint32_t,const SourceCamouflageContextServices&);
}
