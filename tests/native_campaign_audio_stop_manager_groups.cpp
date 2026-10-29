// Focused actual-manager registration fixture. Unrelated group inventories
// are omitted; no game owner or algorithm dependency is replaced. The actual
// manager and the complete stop callback execute over the physical corpus.
#include "audio/native_tu15_sound_kernel.hpp"
namespace halo2::native_tu15_sound {
std::span<const Kernel> gain_kernels(){return {};}
std::span<const Kernel> properties_kernels(){return {};}
std::span<const Kernel> listener_kernels(){return {};}
std::span<const Kernel> resource_kernels(){return {};}
std::span<const Kernel> startup_kernels(){return {};}
std::span<const Kernel> object_loop_kernels(){return {};}
std::span<const Kernel> campaign_audio_kernels(){return {};}
std::span<const Kernel> unit_speech_kernels(){return {};}
}
