#pragma once
#include "halo2/sound_driver_parameters.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Actual source game-time inputs, not presentation/device time. Tick arithmetic
// keeps its original32-bit wrapping and signed comparisons. Delta remains the
// exact authored/current FLOAT word from the game-time owner.
struct SourceScriptSoundClockTimeTu15 {
    std::uint32_t tick_bits{};
    std::int16_t tick_rate{};
    std::uint32_t delta_time_bits{};
};

// Closed arithmetic slices of18D0E0,18CFB0,18D030 and18CCD0 respectively.
// Callers establish the actual sound/row/reference owners. A missing row is
// represented explicitly; these functions choose no sound or permutation and
// never use PCM duration. Duration bits are the actual qualified213690 result.
std::int32_t source_script_sound_explicit_remaining_tu15(
    bool matched,std::uint32_t start_tick_bits,std::int32_t ticks_at30,
    const SourceScriptSoundClockTimeTu15&,const SoundDriverFloatingPointContext&);
std::int32_t source_script_sound_stored_remaining_tu15(
    bool matched,std::uint32_t start_tick_bits,std::int32_t maximum_ms,
    const SourceScriptSoundClockTimeTu15& query_time,
    const SourceScriptSoundClockTimeTu15& return_time,
    const SoundDriverFloatingPointContext&);
std::int32_t source_script_sound_language_remaining_tu15(
    bool matched,std::uint32_t start_tick_bits,std::uint32_t selected_seconds_bits,
    const SourceScriptSoundClockTimeTu15&,const SoundDriverFloatingPointContext&);
bool source_script_sound_clock_expired_tu15(
    std::uint32_t start_tick_bits,std::uint32_t selected_seconds_bits,
    const SourceScriptSoundClockTimeTu15&,const SoundDriverFloatingPointContext&);

struct SourceScriptSoundClockSelectionTu15 {
    std::uint8_t pitch_range=0xff,permutation=0xff;
    bool operator==(const SourceScriptSoundClockSelectionTu15&) const=default;
};
// Native metadata for the original16-row policy. There is deliberately no
// serialized/source voice field: the playback adapter separately owns its
// actual64-bit voices. The predicate and ignored trailing byte retain their
// original initialization/stores without assigning them new meanings.
struct SourceScriptSoundClockRowTu15 {
    std::uint32_t sound=0xffffffffu,start_tick_bits=0xffffffffu;
    SourceScriptSoundClockSelectionTu15 selection;
    std::uint8_t predicate=0xff,trailing=0xff;
    bool operator==(const SourceScriptSoundClockRowTu15&) const=default;
};
using SourceScriptSoundClockRowsTu15=std::array<SourceScriptSoundClockRowTu15,16>;

struct SourceScriptSoundClockPolicyTu15 {
    // Called at the actual source read phases, including after selection.
    std::function<SourceScriptSoundClockTimeTu15()> time;
    // Qualified213690 result for this row's full sound and raw selector.
    // This does not permit selecting a convenient PCM/permutation duration.
    std::function<std::uint32_t(unsigned,std::uint32_t,
        const SourceScriptSoundClockSelectionTu15&)> selected_seconds;
    // Actual213410 selection phase. Writes occur in the selected row, so a
    // failing provider cannot roll back its already performed selector stores.
    std::function<void(std::uint32_t,SourceScriptSoundClockSelectionTu15&)> select;
    // Ordered128E70 intent. A row ordinal is not a source or native voice ID.
    // Immediate host destruction is not inferred from this source request.
    std::function<void(unsigned)> request_stop;
};

// Exact18CCD0 metadata policy: first eligible candidate, last exact match.
// The outer18CAC0 caller rejects NONE before entering this kernel; the kernel
// itself preserves18CCD0's literal NONE comparisons for source-level tests.
// Providers must remain on the owning executor thread and may not reenter or
// replace the rows during a call. Earlier callback/metadata effects persist
// if a later provider fails, as they do before a native admission failure.
std::optional<unsigned> source_script_sound_clock_admit_tu15(
    SourceScriptSoundClockRowsTu15&,std::uint32_t sound,
    const SourceScriptSoundClockPolicyTu15&,const SoundDriverFloatingPointContext&);

//18D0E0/18D030 and18D170 use the FIRST matching full datum; NONE is excluded.
std::optional<unsigned> source_script_sound_clock_find_first_tu15(
    const SourceScriptSoundClockRowsTu15&,std::uint32_t sound);
//18D170 clears sound/start only after its ordered stop request. Selector,
// predicate and trailing byte are retained. The native voice link is owned
// and cleared separately by the playback adapter, never by narrowing an ID.
std::optional<unsigned> source_script_sound_clock_stop_first_tu15(
    SourceScriptSoundClockRowsTu15&,std::uint32_t sound,
    const std::function<void(unsigned)>& request_stop);

struct SourceScriptSoundClockSelectionInputTu15 {
    std::uint32_t sound=0xffffffffu;
    std::uint8_t pitch_range_count{};
    std::int16_t permutation_count{};
    // Already qualified21FE40 result: native class bit5 and non-NONE range+6
    // yield this bit ordinal; an absent value selects2133D0's uniform path.
    std::optional<std::uint16_t> prefix_bit;
    std::span<const std::uint16_t> skip_fractions;
    std::int32_t game_kind{};
    std::uint8_t skip_control{};
};
using SourceScriptSoundClockRandomStoreTu15=
    std::function<void(std::uint32_t instruction,std::uint32_t before,std::uint32_t after)>;
//213410/21FB20/2133D0 selection over caller-qualified metadata and actual
// auxiliary random/prefix owners. The32-bit policy currently owns the valid
//1..31 permutation domain; larger/custom domains explicitly fail instead of
// silently widening the original mask or changing its selection policy.
// Prior RNG writes persist if an observer/provider fails. The caller must
// validate all owning sound/class/range namespaces before entering this pure
// kernel; these operands alone are not an asset-admission receipt.
void source_script_sound_clock_select_tu15(
    const SourceScriptSoundClockSelectionInputTu15&,
    std::span<std::uint32_t> prefix_flags,std::uint32_t& auxiliary_random,
    SourceScriptSoundClockSelectionTu15&,const SoundDriverFloatingPointContext&,
    const SourceScriptSoundClockRandomStoreTu15& = {});
} // namespace halo2

namespace halo2 {
// Numeric tail2B3D07..2B3D30 of the two-argument HS sound_impulse_time
// handler. The18D0E0 result is GAME ticks; the handler reads the live delta
// again AFTER that query, converts to FLOAT, multiplies by that exact delta
// and30, then spills through the original x87 FISTP. This does not query a
// clock owner, choose a sound, or perform HS completion.
std::int32_t source_script_sound_clock_hs_explicit_tail_tu15(
    std::int32_t game_ticks,std::uint32_t fresh_delta_time_bits,
    const SoundDriverFloatingPointContext&);
} // namespace halo2
