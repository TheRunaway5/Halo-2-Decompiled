#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
namespace halo2::tu15_sound_ring_effect {
static_assert(sizeof(std::uintptr_t)==8,"TU1.5 native reverb requires64-bit native pointers");
using Word = std::uint32_t;
using Frame = std::array<Word,16>;
using Workspace = std::array<Frame,34>;
struct Ring { std::vector<Word> words; std::uint32_t write_byte_cursor{}; };
struct Tap { std::uint32_t read_byte_cursor{}; };
struct OnePole { Word previous{},feedback{},feedforward{}; };
enum class TapOwner : std::uint32_t { targets, adjusted };
struct Fade {
    Word pending_mask{},sample_count{},step{},amplitude{};
    std::uint32_t selected_read{},selected_write{},write_stride{},tap_cursor{};
    std::array<Word,8> adjusted_taps{};
    bool bindings_initialized{};
    TapOwner tap_owner{TapOwner::targets};
};
struct State {
    Word flags{};
    std::array<Ring,14> rings;
    std::array<Tap,31> taps;
    std::array<Word,19> delays{};
    std::array<Word,8> reflection_gains{};
    Word cross_gain{},mono_gain{};
    std::array<Word,6> target_gains{},current_gains{};
    Word output_mode{};
    std::array<OnePole,10> filters{};
    std::array<Word,7> diffusion_gains{};
    std::array<Word,4> wet_gains{};
    std::array<Word,12> cluster_taps{};
    std::array<Word,10> dormant_biquad_coefficients{};
    Fade fade;
    Word smoothing_count{};
};
struct ArithmeticTrace { std::array<Word,1024> values{}; std::uint32_t count{}; };
struct Controls { Word entry_status{}; std::array<Word,8> modifiers{}; };
// Typed block arithmetic and ordered circular histories for original records 8/9.
// The caller supplies raw admitted 24-bit words; this API does not establish a
// CPU coefficient converter, voice/PCM scale, DSP clock or hardware DMA receipt.
void process(State&,Workspace&,std::span<Word,32> input,std::array<std::span<Word,32>,4> outputs,const Controls&,ArithmeticTrace* = nullptr);
}
