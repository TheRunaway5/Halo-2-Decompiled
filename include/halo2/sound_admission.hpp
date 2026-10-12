#pragma once
#include "halo2/sound_create.hpp"

namespace halo2 {
// Retained source owners. This type never creates readiness or local bindings.
struct SoundAdmissionState {
  std::uint32_t system,tag_table,current_matg,current_ugh,class_fades,local_rng;
  std::uint32_t alternate_codec_flag,first_person_globals,current_scene;
};
struct SoundAdmissionServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Whole24BE30 scene/cluster occlusion query. Called only by the original
  // mismatching-cluster branch; absence is unavailable, never clear-world.
  std::function<std::uint32_t(std::uint32_t,std::int16_t,std::int16_t)> occlusion;
};
struct SoundAdmissionConstants {
  SoundCreateConstants shared;
  std::uint32_t occlusion_threshold_bits;
};
// Complete128380 CPU owner, including128320/126760/129450/129570/223EB0,
//12C370/126710/126930 and their gain/class helpers. Caller supplies the actual
// raw request and output addresses; +78/+79/+7A, listeners, local RNG, map and
// scene ownership must already be real. No admission/platform defaults.
bool source_sound_admit(const SoundAdmissionState &,std::uint32_t sound,
    std::uint32_t raw_request,std::uint32_t local_output,std::uint32_t reason_output,
    const SoundAdmissionConstants &,const SoundDriverParameterConstants &,
    const SoundDriverFloatingPointContext &,const SoundAdmissionServices &,
    const SoundStartupObserver & = {});
// Exact original direct-query ABIs for independently measured dependencies.
// These do not constitute an admitted submit or establish startup/listeners.
std::uint32_t source_sound_admission_query(const SoundAdmissionState &,
    std::uint32_t original_entry,std::uint32_t sound,std::uint32_t location,
    std::uint32_t argument_bits,const SoundAdmissionConstants &,
    const SoundDriverParameterConstants &,const SoundDriverFloatingPointContext &,
    const SoundAdmissionServices &,const SoundStartupObserver & = {});
// Explicit1283F4..12846A/128524 slice. Consumes the actual retained local RNG;
// bypassing availability for this measured slice is not successful admission.
bool source_sound_admission_skip_slice(const SoundAdmissionState &,
    std::uint32_t sound,std::uint32_t location,const SoundAdmissionConstants &,
    const SoundDriverFloatingPointContext &,const SoundAdmissionServices &,
    const SoundStartupObserver & = {});
} // namespace halo2
