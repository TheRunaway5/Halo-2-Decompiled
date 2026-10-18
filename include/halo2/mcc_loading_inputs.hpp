#pragma once
#include "halo2/vista_loading_assets.hpp"

namespace halo2 {
// These are the five authored MCC bitmap inputs also used by the existing
// normal-mode loading draw catalogue. They do not constitute the Vista bank:
// the separate logo mask and ambience require their own verified producers.
struct MccLoadingBitmapInput {
  unsigned texture_slot{};
  TagHandle handle;
  unsigned frame{};
  VistaLoadingImage image;
};
using MccLoadingBitmapInputs = std::array<MccLoadingBitmapInput,5>;
// Uses the existing Cache bitmap decoder and pinned BC storage conversion.
// All pixels and mips are owned after return; no executable/loading.bin input,
// texture resize, colour correction or loading-readiness signal is involved.
MccLoadingBitmapInputs load_mcc_loading_bitmap_inputs(const Cache& mainmenu);
}
