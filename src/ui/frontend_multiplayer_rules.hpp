#pragma once
#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"

namespace halo2::detail {
// All stock engineUI0..6 option buffers belong to the retained source engine.
// Native custom Slayer and Defusal keep their separately admitted adapters.
inline SlayerRules frontend_multiplayer_rules(const UiVariant& variant) {
  if(variant.engine<=6 && custom_slayer(variant)==CustomSlayer::none && !bomb_defusal_variant(variant))
    return multiplayer_source_rules(variant,true);
  return slayer_rules(variant,true);
}
}
