#pragma once
#include "halo2/native_tu15_sound_status_context.hpp"
#include "halo2/native_tu15_sound_gain_fanout.hpp"
namespace halo2::tu15_record0_context {
namespace gain=tu15_sound_gain_fanout;
// Canonical eight-route owner profile of the previously reviewed record0.
// This is not admission of every legal source mod/routing/alias program.
void process(const gain::Controls&,const gain::Parameters&,gain::State&,gain::Input,const std::array<gain::Output,8>&,tu15_status_context::Status&,tu15_status_context::Trace&);
}
