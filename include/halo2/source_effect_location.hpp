#pragma once
#include "halo2/transform.hpp"

namespace halo2 {
// Completed original effect body+104/+108, before particle color selection.
struct SourceEffectColors {
  std::uint32_t a=0xff808080u,b=0xff808080u;
};
// Completed original effect allocation. NONE is an allocation/visibility
// result, not permission for presentation to manufacture a replacement.
struct SourceWeaponEffectPresentation {
  std::uint32_t instance=0xffffffffu;
  SourceEffectColors colors;
  bool colors_observed{};
};
// Original effect location name and completed world transform, read from the
// same retained effect definition and3C-byte location pool record.
struct SourceEffectLocation {
  std::uint32_t name=0xffffffffu;
  Transform frame;
};
} // namespace halo2
