// Native kernels: unit speech (TU1.5 sound manager). 18F510 itself belongs to
// the object-loop group; it calls these routines through the live world.
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
std::span<const Kernel> unit_speech_kernels(){return {};}
} // namespace halo2::native_tu15_sound
