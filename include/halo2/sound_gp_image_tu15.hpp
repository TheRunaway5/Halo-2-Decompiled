#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace halo2 {
struct SoundGpEffectImageTu15 {
  // Exact unrelocated compiler descriptor. Its Y/scratch fields retain their
  // original encoded units; these are not native or hardware pointers.
  std::array<std::uint32_t,8> descriptor;
  std::uint32_t program_word,state_word;
};
struct SoundGpImageTu15 {
  std::vector<std::uint32_t> control_program,effect_program,initial_data;
  std::array<std::uint32_t,6> header;
  std::vector<SoundGpEffectImageTu15> effects;
  std::vector<std::byte> decoded_resource;
  std::uint32_t program_base_word,data_base_word;
  std::uint32_t user_scratch_base,user_scratch_bytes,minimum_scratch_bytes;
};
// Authenticate the owned TU1.5 executable, retain its GP control program and
// decode its actual DSPImage seed table/effects with 38DF59/38DFD2/38DFFF's
// byte calculations. No captured program/state is a runtime input.
// This is immutable image preparation, not the SDK's allocation/relocation,
// upload request, DSP execution, device readiness or platform-call result.
SoundGpImageTu15 load_sound_gp_image_tu15(const std::filesystem::path &);
} // namespace halo2
