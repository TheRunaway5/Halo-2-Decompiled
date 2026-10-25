#pragma once
#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"

namespace halo2::detail {
// Frontend engine4/5 are the original CTF/Assault family. Their complete
// validated option buffers belong to the retained source engine. Custom
// Defusal keeps its separately admitted native rules adapter.
inline SlayerRules frontend_multiplayer_rules(const UiVariant& variant) {
  if((variant.engine==4 || variant.engine==5) && !bomb_defusal_variant(variant))
    return multiplayer_source_rules(variant,true);
  return slayer_rules(variant,true);
}
}
