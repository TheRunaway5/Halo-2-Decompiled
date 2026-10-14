#pragma once
#include "halo2/audio.hpp"

namespace halo2 {
// MCC 769370 consumes signed little-endian packet lengths and at most 240
// decoded frames per packet. The output uses the ordinary native PCM owner.
PcmSound decode_mcc_opus(const EncodedSound&);
}
