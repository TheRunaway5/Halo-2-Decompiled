#pragma once
#include "halo2/controller_settings.hpp"
#include "halo2/pc_settings.hpp"
namespace halo2 {
// Original22DCBC/22DF04 dynamic control tokens, resolved against the live
// native adapter. The keyboard names are its actual configured bindings.
std::string training_control_text(const std::string &,
                                  const ControllerSettings &,
                                  const PcKeyboardSettings &, bool gamepad);
} // namespace halo2
