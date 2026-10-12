#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Actual mutable SDK global storage from the selected source image/state.
// These addresses do not initialize any counters, device or hardware.
struct SoundCapabilitiesBindings {
  std::uint32_t free_2d,free_3d,free_sge,pool_bytes,contiguous_bytes;
  std::uint32_t fault,critical_section,statistics_pointer,private_pool_pointer;
  std::uint32_t device_context_pointer;
};
// Exact immutable identities read from the owned executable, not host handles.
struct SoundCapabilitiesConstructorConstants {
  std::uint32_t reference_vtable,apu_vtable,apu_interface_vtable,sge_vtable;
  std::uint32_t base_vtable,reference_add_method;
};
struct SoundCapabilitiesServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Actual source thread IRQL byte consumed by3804DE; no implicit host default.
  std::function<std::uint8_t()> current_irql;
  // Genuine synchronization must own the actual requested record/identity.
  std::function<void(std::uint32_t)> enter_critical_section,leave_critical_section;
  // Real raw allocation/capacity services, not an initialized source object.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> raw_pool_allocate; // bytes,tag
  std::function<std::uint32_t(std::uint32_t)> raw_pool_capacity;
  // Complete386BE7 private-pool owner is separate and mandatory if selected.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> source_private_allocate;
};
struct SoundCapabilitiesStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const SoundCapabilitiesStore &,const SoundCapabilitiesStore &)=default;
};
using SoundCapabilitiesObserver=std::function<void(const SoundCapabilitiesStore &)>;
// Complete381BEE→380810 CPU reader, including3804DE guard. Returning0 only
// proves the original four counter copies completed, never device admission.
std::int32_t source_sound_capabilities(std::uint32_t device,std::uint32_t output,
    const SoundCapabilitiesBindings &,const SoundCapabilitiesServices &,
    const SoundCapabilitiesObserver & = {});
// Complete384C7C CPU constructor including38689A/38AD61/38B257/3817AF and
// supported actual38034C reference method. A different virtual method stops.
// The caller owns real raw storage and actual parameter object; this does not
// bypass a saved-setting/hardware prerequisite in natural DirectSoundCreate.
std::uint32_t source_sound_apu_construct(std::uint32_t apu,std::uint32_t parameters,
    const SoundCapabilitiesBindings &,const SoundCapabilitiesConstructorConstants &,
    const SoundCapabilitiesServices &,const SoundCapabilitiesObserver & = {});
// Complete383F92→383ECC allocation/accounting CPU order. Private-pool branch
// delegates to its required original owner. Host capacity is not Xbox rounding.
std::uint32_t source_sound_sdk_allocate(std::uint32_t tag,std::uint32_t bytes,
    std::uint32_t zero,const SoundCapabilitiesBindings &,const SoundCapabilitiesServices &,
    const SoundCapabilitiesObserver & = {});
// Complete3868DC descriptor initializer, including38683D and source allocation.
// count is the actual caller word (low16 used). The original3858D8 caller uses
//0x7FF only AFTER38B1EB hardware initialization; this leaf does not close it.
std::int32_t source_sound_sge_initialize(std::uint32_t sge,std::uint32_t count,
    const SoundCapabilitiesBindings &,const SoundCapabilitiesServices &,
    const SoundCapabilitiesObserver & = {});
} // namespace halo2
