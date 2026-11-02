#pragma once
#include "halo2/source_physics_entry_observation.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace halo2 {
// Passive diagnostics for the actual 18FA45 ->17E160 looping-effect owner.
// The caller binds its real source bank and supplies completed authoritative
// input observations. This class never writes source memory, executes a game
// owner, supplies a result, or changes input/audio/physics/render admission.
class SourceLoopAudioLocationObservationTu15 {
public:
    struct Input {
        std::uint32_t tick{}, expected_smg_definition=0xffffffffu;
        std::uint32_t player=0xffffffffu, unit=0xffffffffu, weapon=0xffffffffu, actual_definition=0xffffffffu;
        std::int32_t loaded{}, reserve{};
        std::uint32_t shot_tick{};
        bool fire{}, reload{};
        std::uint32_t target_owner=0xffffffffu, target_definition=0xffffffffu;
        bool precondition_observed{};
    };
    SourceLoopAudioLocationObservationTu15() = default;
    SourceLoopAudioLocationObservationTu15(const SourceLoopAudioLocationObservationTu15 &) = delete;
    SourceLoopAudioLocationObservationTu15 &operator=(const SourceLoopAudioLocationObservationTu15 &) = delete;
    ~SourceLoopAudioLocationObservationTu15() { unbind(); }

    void bind(const SourcePhysicsContactMemory &memory) {
        if (was_bound_ || memory_ || binding_) throw std::runtime_error("Loop-audio location observation already bound");
        binding_.emplace(memory, [this](bool entering, std::uint32_t entry, const SourcePhysicsWorldRuntimeAbi &abi) noexcept {
            if (entry != 0x17e160u) return;
            try { observe_entry(entering, abi); } catch (...) { ++callback_read_failures_; }
        });
        memory_ = &memory; was_bound_ = true;
    }
    // Call on the same thread, before the source bank is destroyed. Retained
    // evidence remains available for report() after the world has closed.
    void unbind() noexcept { binding_.reset(); memory_ = nullptr; }

    // This boundary runs outside original FP scopes. Baseline vector storage
    // is prepared/refreshed only here, never in the source entry callback.
    // A neutral actual SMG sample must precede the first delivered fire input.
    void observe_input(const Input &input) noexcept {
        ++input_samples_;
        try {
            if (!memory_) { ++input_errors_; return; }
            if (input.expected_smg_definition == none || input.actual_definition != input.expected_smg_definition) {
                ++non_smg_inputs_;
                if (have_player_) { ++post_pin_non_smg_inputs_; ++identity_changes_; }
                return;
            }
            if (input.player == none || input.unit == none || input.weapon == none || input.loaded < 0 || input.reserve < 0) {
                ++input_errors_; return;
            }
            const auto objects = pool(0x4ecbd0u, 12u);
            if (input.target_owner != none) {
                const auto target_header = objects.lookup(input.target_owner);
                const auto body = target_header ? word(target_header + 8u) : 0u;
                if (!body || input.target_definition == none || word(body) != input.target_definition ||
                    (target_owner_ != none && (target_owner_ != input.target_owner || target_definition_ != input.target_definition))) {
                    ++input_errors_; return;
                }
                target_owner_ = input.target_owner; target_definition_ = input.target_definition;
            }
            if (input.precondition_observed) {
                if (input.target_owner == none || target_owner_ == none) { ++input_errors_; return; }
                if (!precondition_observed_) precondition_tick_ = input.tick;
                precondition_observed_ = true;
            }
            const auto unit_header = objects.lookup(input.unit), weapon_header = objects.lookup(input.weapon);
            if (!unit_header || !weapon_header || !word(unit_header + 8u) || !word(weapon_header + 8u) ||
                word(word(weapon_header + 8u)) != input.actual_definition) { ++input_errors_; return; }
            const auto clock = word(0x51df54u);
            if (!clock) { ++input_errors_; return; }
            const auto source_tick = word(clock + 8u);
            if (!have_player_) {
                have_player_ = true; first_ = input; previous_ = input; last_ = input;
                first_source_tick_ = source_tick;
            } else if (input.player != first_.player || input.unit != first_.unit || input.weapon != first_.weapon ||
                       input.actual_definition != first_.actual_definition || input.expected_smg_definition != first_.expected_smg_definition) {
                ++identity_changes_; return;
            }
            ++smg_inputs_; fire_commands_ += input.fire; reload_commands_ += input.reload;
            current_fire_streak_ = input.fire ? current_fire_streak_ + 1u : 0u;
            if (current_fire_streak_ > longest_fire_streak_) longest_fire_streak_ = current_fire_streak_;
            const bool shot_advanced = std::bit_cast<std::int32_t>(input.shot_tick - previous_.shot_tick) > 0;
            const bool consumed = input.loaded < previous_.loaded;
            if (!fire_seen_ && !shot_seen_) {
                if (!input.fire && !shot_advanced && !consumed) prepare_baseline();
                else if (!baseline_ready_) ++late_baseline_inputs_;
            }
            if (input.fire) { fire_seen_ = true; baseline_frozen_ = baseline_ready_; }
            if (shot_advanced && consumed && (input.fire || previous_.fire || fire_seen_)) {
                ++shot_transitions_; rounds_consumed_ += std::uint32_t(previous_.loaded - input.loaded);
                if (!shot_seen_) {
                    shot_seen_ = true; first_shot_tick_ = input.tick; first_shot_source_tick_ = source_tick;
                    first_shot_weapon_tick_ = input.shot_tick;
                    baseline_frozen_ = baseline_ready_;
                }
            }
            if (reload_commands_ && input.loaded > previous_.loaded && input.reserve < previous_.reserve) {
                ++reload_transfers_; reload_rounds_ += std::uint32_t(input.loaded - previous_.loaded);
            }
            previous_ = input; last_ = input; last_source_tick_ = source_tick;
        } catch (...) { ++input_read_failures_; }
    }

    bool qualified() const noexcept {
        return was_bound_ && have_player_ && baseline_ready_ && baseline_frozen_ && shot_seen_ &&
            shot_transitions_ && rounds_consumed_ && !identity_changes_ && !input_errors_ && !input_read_failures_ &&
            !late_baseline_inputs_ && target_owner_ != none && target_owned_post_fire_returns_ && successful_source_ticks_ >= 2u &&
            entries_ == returns_ && !invalid_calls_ && !callback_read_failures_ && !depth_ && !overflow_depth_;
    }
    // Invoke only after source execution/FP scopes have returned. This is the
    // sole JSON/formatting boundary; samples retain integer source bit patterns.
    nlohmann::json report() const {
        using J = nlohmann::json;
        J calls = J::array();
        for (unsigned n = 0; n < sample_count_; ++n) {
            const auto &sample = samples_[n]; const auto &p = sample.call;
            calls.push_back({{"tick", p.tick}, {"caller_return", 0x18fa4au}, {"effect", p.effect}, {"loop", p.loop},
                {"effect_definition", p.definition}, {"loop_definition", p.loop_definition}, {"effect_object_owner", p.owner},
                {"effect_object_definition", p.owner_definition}, {"new_generation", p.new_generation},
                {"precondition_generation_seen", sample.precondition_generation_seen},
                {"precondition_first_seen_source_tick", sample.precondition_first_seen_source_tick},
                {"eax", sample.eax}, {"output_bits", sample.outputs}});
        }
        return {{"schema", "HALO2_SOURCE_LOOP_AUDIO_LOCATION_TU15_V1"}, {"qualified", qualified()},
            {"scope", "Passive actual TU1.5 loop-effect location ownership; no target damage, audible speaker or framebuffer equivalence is inferred"},
            {"target", {{"owner", target_owner_}, {"definition", target_definition_},
                {"precondition_observed", precondition_observed_}, {"precondition_tick", precondition_tick_},
                {"scope", "Full salted target/definition validated against actual object memory; precondition fact supplied by the caller's real native observations"}}},
            {"player", {{"source_player", first_.player}, {"source_unit", first_.unit}, {"source_weapon", first_.weapon},
                {"weapon_definition", first_.actual_definition}, {"expected_smg_definition", first_.expected_smg_definition},
                {"initial_tick", first_.tick}, {"last_tick", last_.tick}, {"initial_source_tick", first_source_tick_},
                {"last_source_tick", last_source_tick_}, {"initial_loaded", first_.loaded}, {"initial_reserve", first_.reserve},
                {"last_loaded", last_.loaded}, {"last_reserve", last_.reserve}, {"input_samples", input_samples_},
                {"smg_inputs", smg_inputs_}, {"non_smg_inputs", non_smg_inputs_}, {"identity_changes", identity_changes_},
                {"post_pin_non_smg_inputs", post_pin_non_smg_inputs_},
                {"fire_commands", fire_commands_}, {"longest_fire_streak", longest_fire_streak_},
                {"shot_transitions", shot_transitions_}, {"rounds_consumed", rounds_consumed_},
                {"reload_commands", reload_commands_}, {"reload_transfers", reload_transfers_}, {"reload_rounds", reload_rounds_},
                {"input_errors", input_errors_}, {"input_read_failures", input_read_failures_}}},
            {"mode3", {{"observer_bound", was_bound_}, {"entries", entries_}, {"returns", returns_},
                {"other_callers", other_callers_}, {"validated_entries", validated_entries_},
                {"successful_returns", successful_returns_}, {"post_fire_successful_returns", post_fire_returns_},
                {"new_generation_successful_returns", new_generation_returns_},
                {"target_owned_post_fire_successful_returns", target_owned_post_fire_returns_},
                {"preexisting_target_owned_successful_returns", preexisting_target_returns_},
                {"precondition_target_owned_successful_returns", precondition_target_returns_},
                {"precondition_generation_seen", precondition_generation_count_ != 0u},
                {"precondition_generation_count", precondition_generation_count_},
                {"post_fire_successful_ticks", successful_source_ticks_}, {"invalid_calls", invalid_calls_},
                {"read_failures", callback_read_failures_}, {"unfinished_calls", depth_ + overflow_depth_},
                {"baseline_recorded", baseline_ready_}, {"baseline_frozen", baseline_frozen_},
                {"baseline_updates", baseline_updates_}, {"late_baseline_inputs", late_baseline_inputs_},
                {"effect_pool_capacity", baseline_effects_.size()}, {"loop_pool_capacity", baseline_loops_.size()},
                {"first_shot_tick", first_shot_tick_}, {"first_shot_source_tick", first_shot_source_tick_},
                {"first_shot_weapon_tick", first_shot_weapon_tick_},
                {"first_success_source_tick", first_success_source_tick_}, {"last_success_source_tick", last_success_source_tick_},
                {"preexisting_scope", "Matching full effect/loop pair successfully observed on an earlier source clock tick before the first native SMG shot sample; effect birth causality is not inferred"},
                {"sample_limit", samples_.size()}, {"samples", std::move(calls)}}}};
    }

private:
    static constexpr std::uint32_t none = 0xffffffffu;
    std::uint32_t word(std::uint32_t address, unsigned size = 4u) const {
        std::array<std::byte, 4> bytes{}; memory_->read(address, std::span(bytes).first(size));
        std::uint32_t value{};
        for (unsigned n = 0; n < size; ++n) value |= std::uint32_t(std::to_integer<unsigned>(bytes[n])) << (n * 8u);
        return value;
    }
    struct Pool {
        const SourceLoopAudioLocationObservationTu15 *observer{};
        std::uint32_t capacity{}, stride{}, high{}, rows{}, bits{}; bool valid{};
        std::uint32_t identity(unsigned index) const {
            if (!valid || index >= high || !(observer->word(bits + index / 32u * 4u) & (1u << (index % 32u)))) return none;
            const auto salt = observer->word(rows + index * stride, 2u);
            return salt ? (salt << 16u) | index : none;
        }
        std::uint32_t lookup(std::uint32_t datum) const {
            const auto index = datum & 65535u;
            return datum != none && identity(index) == datum ? rows + index * stride : 0u;
        }
    };
    Pool pool(std::uint32_t global, unsigned stride) const {
        Pool result; result.observer = this; const auto header = word(global);
        if (!header || std::uint64_t(header) + 0x4cu > 0x100000000ull) return result;
        result.capacity = word(header + 0x20u); result.stride = word(header + 0x24u); result.high = word(header + 0x38u);
        result.rows = word(header + 0x44u); result.bits = word(header + 0x48u);
        result.valid = result.capacity > 0u && result.capacity <= 65536u && result.stride == stride &&
            result.high <= result.capacity && result.rows && result.bits &&
            std::uint64_t(result.rows) + std::uint64_t(result.capacity) * result.stride <= 0x100000000ull &&
            std::uint64_t(result.bits) + ((std::uint64_t(result.capacity) + 31u) / 32u) * 4u <= 0x100000000ull;
        return result;
    }
    void prepare_baseline() {
        const auto effects = pool(0x4f720cu, 400u), loops = pool(0x4f9b5cu, 24u);
        if (!effects.valid || !loops.valid) { ++input_errors_; return; }
        baseline_effects_.assign(effects.capacity, none); baseline_loops_.assign(loops.capacity, none);
        for (unsigned n = 0; n < effects.high; ++n) baseline_effects_[n] = effects.identity(n);
        for (unsigned n = 0; n < loops.high; ++n) baseline_loops_[n] = loops.identity(n);
        baseline_ready_ = true; ++baseline_updates_;
    }
    struct Pending {
        std::uint32_t sp{}, effect=none, loop=none, definition=none, loop_definition=none, owner=none, owner_definition=none, tick{};
        std::uint32_t translation{}, forward{}, control{};
        bool exact_caller{}, valid{}, post_fire{}, new_generation{}, target_owned{};
    };
    struct Sample {
        Pending call; std::array<std::uint32_t, 7> outputs{}; std::uint32_t eax{};
        bool precondition_generation_seen{}; std::uint32_t precondition_first_seen_source_tick=none;
    };
    std::uint32_t precondition_first_seen(std::uint32_t effect, std::uint32_t loop) const noexcept {
        for (unsigned n = 0; n < precondition_generation_count_; ++n)
            if (precondition_generations_[n][0] == effect && precondition_generations_[n][1] == loop)
                return precondition_generations_[n][2];
        return none;
    }
    void observe_entry(bool entering, const SourcePhysicsWorldRuntimeAbi &a) {
        if (entering) {
            ++entries_;
            if (overflow_depth_ || depth_ == pending_.size()) { ++overflow_depth_; ++invalid_calls_; return; }
            auto &p = pending_[depth_++]; p = {}; p.sp = a.gp[4]; p.effect = a.gp[0];
            if (std::uint64_t(p.sp) + 12u > 0xffffffffull) { ++invalid_calls_; return; }
            p.exact_caller = word(p.sp) == 0x18fa4au;
            if (!p.exact_caller) { ++other_callers_; return; }
            p.translation = a.gp[3]; p.forward = word(p.sp + 4u); p.control = word(p.sp + 8u);
            if (std::uint64_t(p.forward) + 12u > 0x100000000ull) { ++invalid_calls_; return; }
            const auto clock = word(0x51df54u); if (!clock) { ++invalid_calls_; return; }
            p.tick = word(clock + 8u);
            const auto effects = pool(0x4f720cu, 400u), loops = pool(0x4f9b5cu, 24u);
            const auto effect = effects.lookup(p.effect), row = a.gp[6];
            if (!effect || !loops.valid || row < loops.rows || (row - loops.rows) % 24u ||
                (row - loops.rows) / 24u >= loops.high) { ++invalid_calls_; return; }
            p.loop = loops.identity((row - loops.rows) / 24u);
            if (loops.lookup(p.loop) != row || word(row + 3u, 1u) != 3u || (word(row + 4u, 2u) & 2u) ||
                word(row + 0x10u) != p.effect || std::uint64_t(a.gp[7]) + 0x18u > 0x100000000ull ||
                p.translation != a.gp[7] + 0xcu || p.control != a.gp[7] + 4u) { ++invalid_calls_; return; }
            p.definition = word(effect + 4u); p.owner = word(effect + 0x48u); p.loop_definition = word(row + 0xcu);
            if (p.definition == none || p.loop_definition == none) { ++invalid_calls_; return; }
            if (target_owner_ != none && p.owner == target_owner_) {
                const auto header = pool(0x4ecbd0u, 12u).lookup(p.owner);
                const auto body = header ? word(header + 8u) : 0u;
                if (!body || word(body) != target_definition_) { ++invalid_calls_; return; }
                p.owner_definition = target_definition_; p.target_owned = true;
            }
            if (baseline_ready_ && (effects.capacity != baseline_effects_.size() || loops.capacity != baseline_loops_.size())) {
                ++invalid_calls_; return;
            }
            p.post_fire = shot_seen_ && p.tick >= first_shot_source_tick_;
            p.new_generation = baseline_ready_ && baseline_frozen_ && baseline_effects_[p.effect & 65535u] != p.effect &&
                baseline_loops_[p.loop & 65535u] != p.loop;
            p.valid = true; ++validated_entries_;
        } else {
            ++returns_;
            if (overflow_depth_) { --overflow_depth_; return; }
            if (!depth_) { ++invalid_calls_; return; }
            const auto p = pending_[--depth_]; if (!p.exact_caller || !p.valid) return;
            if (std::uint64_t(p.sp) + 12u > 0xffffffffull || a.gp[4] != p.sp + 12u) { ++invalid_calls_; return; }
            if (!(a.gp[0] & 255u)) return;
            Sample sample; sample.call = p; sample.eax = a.gp[0];
            for (unsigned n = 0; n < 3u; ++n) {
                sample.outputs[n] = word(p.translation + n * 4u);
                sample.outputs[n + 3u] = word(p.forward + n * 4u);
            }
            sample.outputs[6] = word(p.control);
            for (const auto value : sample.outputs)
                if ((value & 0x7f800000u) == 0x7f800000u) { ++invalid_calls_; return; }
            if (!pool(0x4f720cu, 400u).lookup(p.effect) || !pool(0x4f9b5cu, 24u).lookup(p.loop)) { ++invalid_calls_; return; }
            if (p.target_owned) {
                const auto header = pool(0x4ecbd0u, 12u).lookup(p.owner);
                const auto body = header ? word(header + 8u) : 0u;
                if (!body || word(body) != p.owner_definition) { ++invalid_calls_; return; }
            }
            ++successful_returns_; if (p.post_fire) ++post_fire_returns_;
            if (!shot_seen_ && p.target_owned && precondition_observed_) {
                ++precondition_target_returns_;
                if (precondition_first_seen(p.effect, p.loop) == none) {
                    if (precondition_generation_count_ == precondition_generations_.size()) { ++invalid_calls_; return; }
                    precondition_generations_[precondition_generation_count_++] = {p.effect, p.loop, p.tick};
                }
            }
            sample.precondition_first_seen_source_tick = precondition_first_seen(p.effect, p.loop);
            sample.precondition_generation_seen = sample.precondition_first_seen_source_tick != none && shot_seen_ &&
                sample.precondition_first_seen_source_tick < first_shot_source_tick_;
            if (p.post_fire && p.target_owned && (p.new_generation || sample.precondition_generation_seen)) {
                ++target_owned_post_fire_returns_;
                if (p.new_generation) ++new_generation_returns_; else ++preexisting_target_returns_;
                if (!successful_source_ticks_ || p.tick != last_success_source_tick_) {
                    if (successful_source_ticks_ && p.tick < last_success_source_tick_) ++invalid_calls_;
                    if (!successful_source_ticks_) first_success_source_tick_ = p.tick;
                    last_success_source_tick_ = p.tick; ++successful_source_ticks_;
                }
                if (sample_count_ < samples_.size()) samples_[sample_count_++] = sample;
            }
        }
    }
    const SourcePhysicsContactMemory *memory_{};
    std::optional<SourcePhysicsEntryObserverBinding> binding_;
    std::vector<std::uint32_t> baseline_effects_, baseline_loops_;
    std::array<Pending, 8> pending_{}; unsigned depth_{}, overflow_depth_{};
    std::array<Sample, 64> samples_{}; unsigned sample_count_{};
    std::array<std::array<std::uint32_t, 3>, 64> precondition_generations_{}; unsigned precondition_generation_count_{};
    bool was_bound_{}, have_player_{}, fire_seen_{}, shot_seen_{}, baseline_ready_{}, baseline_frozen_{};
    bool precondition_observed_{};
    std::uint32_t target_owner_=none, target_definition_=none, precondition_tick_{};
    Input first_{}, previous_{}, last_{};
    std::uint32_t first_source_tick_{}, last_source_tick_{}, first_shot_tick_{}, first_shot_source_tick_{}, first_shot_weapon_tick_{};
    std::uint32_t first_success_source_tick_{}, last_success_source_tick_{};
    std::uint64_t input_samples_{}, smg_inputs_{}, non_smg_inputs_{}, post_pin_non_smg_inputs_{}, identity_changes_{}, input_errors_{}, input_read_failures_{};
    std::uint64_t fire_commands_{}, current_fire_streak_{}, longest_fire_streak_{}, shot_transitions_{}, rounds_consumed_{};
    std::uint64_t reload_commands_{}, reload_transfers_{}, reload_rounds_{}, baseline_updates_{}, late_baseline_inputs_{};
    std::uint64_t entries_{}, returns_{}, other_callers_{}, validated_entries_{}, successful_returns_{}, post_fire_returns_{};
    std::uint64_t new_generation_returns_{}, successful_source_ticks_{}, invalid_calls_{}, callback_read_failures_{};
    std::uint64_t target_owned_post_fire_returns_{}, preexisting_target_returns_{};
    std::uint64_t precondition_target_returns_{};
};
} // namespace halo2
