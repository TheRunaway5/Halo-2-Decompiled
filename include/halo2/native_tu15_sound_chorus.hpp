#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
namespace halo2::tu15_sound_chorus {
using Word24=halo2::tu15_sound::Word24;
struct Parameters {Word24 gain,depth;bool operator==(const Parameters&)const=default;};
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {
    Word24 entry_status;
    std::array<AddressingMode,5> addressing;
    AddressingMode state_relative{AddressingMode::linear}; // M5: R5+displacement state access.
    // Native relative geometry. Only the recovered unchanged span400 graph
    // is admitted; warm live geometry drift against cached end is unsupported.
    std::uint32_t live_begin_offset{},live_span{400};
};
struct Bindings {std::span<const Word24,32> audio,modulation;std::span<Word24,32> output;};
struct ActiveState {Bindings bindings;std::uint32_t cursor{},cached_end{400};};
struct State {Word24 flags;std::optional<ActiveState> active;};
struct SampleTrace {
    std::int64_t delay_before_correction,delay_after_correction;
    std::uint32_t history_index;Word24 delayed_word;
    std::int64_t output_accumulator;std::uint32_t next_cursor;
    bool operator==(const SampleTrace&)const=default;
};
// History is actual typed native sample storage, not a serialized DSP bank.
// The four authored views share one endpoint each: offsets0/400/800/1200 in
// a1601-word owner. Bindings borrowed on cold initialization must outlive warm
// calls. Warm declared binding changes are ignored exactly as in the source.
void process(const Controls&,const Parameters&,State&,Bindings declared,
             std::span<Word24,401> history,std::span<SampleTrace,32> trace);
Parameters authored_parameters();
} // namespace halo2::tu15_sound_chorus
