#pragma once
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_campaign_ai_behavior_candidate {

template<class State, class Dispatch>
class Frame {
    enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
    static constexpr std::uint32_t descriptor_table = 0x47b478u;
    static constexpr std::uint32_t behavior_epoch = 0x47b908u;
    static constexpr std::uint32_t required_flags = 0x4fafc0u;
    static constexpr std::uint32_t enabled_behaviors = 0x56ae84u;
    static constexpr std::uint32_t actor_pool = 0x5020c8u;
    static constexpr std::uint32_t game_time = 0x51df54u;
    static constexpr std::uint32_t actor_cooldowns = 0x1e8u;
    static constexpr std::int16_t behavior_count = 0x83;
    static constexpr std::int16_t cooldown_count = 14;
    static constexpr std::int16_t always_ready = -1;
    static constexpr std::int16_t if_current_tick = -2;

    State &state_;
    Dispatch &dispatch_;

    void at(std::uint32_t pc) {
        if (!native_census_detail::skip_report(state_.services, pc)) state_.instruction_literal(pc);
    }
    std::uint32_t &reg(Register name) { return state_.gp[name]; }
    std::uint32_t stack(std::uint32_t offset) { return reg(esp) + offset; }
    std::int16_t signed_low(Register name) {
        return std::bit_cast<std::int16_t>(std::uint16_t(reg(name)));
    }
    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_world_runtime_detail::r(state_.memory, address, width);
    }
    void load_register(Register name, std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        state_.set(name, load(pc, address, width), width, 0);
    }
    void store(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        at(pc);
        physics_world_runtime_detail::w(state_.memory, state_.observe, pc, address, 2, value);
    }
    void push(std::uint32_t pc, Register name) {
        at(pc);
        state_.push(pc, reg(name));
    }
    void pop(std::uint32_t pc, Register name) {
        at(pc);
        reg(name) = state_.pop();
    }
    void invoke(std::uint32_t pc, std::uint32_t resume, std::uint32_t target) {
        state_.push(pc, resume);
        dispatch_(state_, target, pc, false);
    }
    void test(std::uint32_t pc, std::uint32_t value, unsigned width) {
        at(pc);
        state_.flags.logic(value, width);
    }
    void compare(std::uint32_t pc, std::uint32_t lhs, std::uint32_t rhs, unsigned width) {
        at(pc);
        state_.flags.minus(lhs, rhs, width);
    }

public:
    Frame(State &state, Dispatch &dispatch) : state_(state), dispatch_(dispatch) {}

    bool initialize_selection() {
        load_register(ecx, 0x1b0020u, stack(0x0cu));
        push(0x1b0024u, ebx);
        push(0x1b0025u, ebp);
        load_register(ebp, 0x1b0026u, stack(0x0cu));
        load_register(edx, 0x1b002au, reg(ebp), 2);
        const auto behavior = signed_low(edx);
        test(0x1b002eu, behavior, 2);
        at(0x1b0031u);
        reg(ebx) = reg(eax);
        load_register(eax, 0x1b0033u, stack(0x10u));
        store(0x1b0037u, reg(eax), 0xffffu);
        store(0x1b003cu, reg(ecx), 0u);
        at(0x1b0041u);
        if (behavior < 0) return false;
        compare(0x1b0047u, behavior, behavior_count, 2);
        at(0x1b004cu);
        return behavior < behavior_count;
    }

    bool descriptor_enabled() {
        push(0x1b0052u, esi);
        load_register(esi, 0x1b0053u, behavior_epoch);
        at(0x1b0059u);
        reg(eax) = std::uint32_t(signed_low(edx));
        push(0x1b005cu, edi);
        load_register(edi, 0x1b005du, descriptor_table + reg(eax) * 4u);
        load_register(ecx, 0x1b0064u, reg(edi) + 8u);
        compare(0x1b0067u, reg(ecx), reg(esi), 4);
        at(0x1b0069u);
        if (reg(ecx) == reg(esi)) return false;

        load_register(ecx, 0x1b006fu, required_flags);
        load_register(esi, 0x1b0075u, reg(edi) + 4u);
        at(0x1b0078u);
        reg(esi) = state_.flags.logic(reg(esi) & reg(ecx), 4);
        compare(0x1b007au, reg(esi), reg(ecx), 4);
        at(0x1b007cu);
        if (reg(esi) != reg(ecx)) return false;

        at(0x1b0082u);
        reg(ecx) = reg(eax);
        at(0x1b0084u);
        reg(ecx) = state_.flags.logic(reg(ecx) & 31u, 4);
        at(0x1b0087u);
        reg(esi) = 1;
        at(0x1b008cu);
        reg(esi) = state_.flags.shift(reg(esi), reg(ecx), 4);
        at(0x1b008eu);
        reg(eax) = state_.flags.right(reg(eax), 5, 4, true);
        const auto enabled = load(0x1b0091u, enabled_behaviors + reg(eax) * 4u) & reg(esi);
        state_.flags.logic(enabled, 4);
        at(0x1b0098u);
        return enabled != 0;
    }

    bool cooldown_ready() {
        load_register(eax, 0x1b009eu, actor_pool);
        load_register(ecx, 0x1b00a3u, reg(eax) + 0x44u);
        load_register(eax, 0x1b00a6u, reg(ebp) + 4u, 2);
        const auto cooldown = signed_low(eax);
        at(0x1b00aau);
        reg(esi) = reg(ebx);
        at(0x1b00acu);
        reg(esi) = state_.flags.logic(reg(esi) & 0xffffu, 4);
        at(0x1b00b2u);
        reg(esi) = state_.flags.product(reg(esi), 0x888u);
        at(0x1b00b8u);
        reg(esi) = state_.flags.plus(reg(esi), reg(ecx), 4);
        compare(0x1b00bau, cooldown, always_ready, 2);
        load_register(ecx, 0x1b00beu, game_time);
        at(0x1b00c4u);
        if (cooldown == always_ready) return true;
        compare(0x1b00c6u, cooldown, if_current_tick, 2);
        at(0x1b00cau);
        if (cooldown == if_current_tick) {
            load_register(ebp, 0x1b00ccu, stack(0x20u));
            load_register(ebp, 0x1b00d0u, reg(ebp) + 8u);
            const auto current_tick = load(0x1b00d3u, reg(ecx) + 8u);
            state_.flags.minus(reg(ebp), current_tick, 4);
            const bool ready = reg(ebp) == current_tick;
            load_register(ebp, 0x1b00d6u, stack(0x14u));
            at(0x1b00dau);
            if (ready) return true;
        }
        test(0x1b00dcu, cooldown, 2);
        at(0x1b00dfu);
        if (cooldown < 0) return false;
        compare(0x1b00e5u, cooldown, cooldown_count, 2);
        at(0x1b00e9u);
        if (cooldown >= cooldown_count) return false;

        at(0x1b00efu);
        reg(eax) = std::uint32_t(cooldown);
        load_register(eax, 0x1b00f2u, reg(esi) + actor_cooldowns + reg(eax) * 4u);
        compare(0x1b00f9u, reg(eax), 0xffffffffu, 4);
        at(0x1b00fcu);
        if (reg(eax) == 0xffffffffu) return true;

        load_register(ecx, 0x1b00feu, reg(ecx) + 8u);
        at(0x1b0101u);
        reg(ecx) = state_.flags.minus(reg(ecx), reg(eax), 4);
        load_register(eax, 0x1b0103u, game_time);
        at(0x1b0108u);
        auto &elapsed = state_.xmm[0];
        elapsed = _mm_cvtsi32_ss(elapsed, std::bit_cast<std::int32_t>(reg(ecx)));
        const auto seconds_per_tick = std::bit_cast<float>(load(0x1b010cu, reg(eax) + 4u));
        elapsed = _mm_mul_ss(elapsed, _mm_set_ss(seconds_per_tick));
        const auto minimum_time = std::bit_cast<float>(load(0x1b0111u, reg(ebp) + 8u));
        state_.flags.scalar_compare(elapsed, _mm_set_ss(minimum_time), false);
        at(0x1b0115u);
        return !state_.flags.cf && !state_.flags.zf;
    }

    bool selects_child() {
        const auto kind = load(0x1b011bu, reg(edi) + 2u, 2);
        state_.flags.minus(kind, 0u, 2);
        at(0x1b0120u);
        return kind == 0;
    }

    void select_child() {
        load_register(ecx, 0x1b0122u, stack(0x24u));
        push(0x1b0126u, ecx);
        push(0x1b0127u, ebx);
        const auto child = load(0x1b0128u, reg(edi) + 0x10u);
        invoke(0x1b0128u, 0x1b012bu, child);
        const auto selected = signed_low(eax);
        test(0x1b012bu, selected, 2);
        at(0x1b012eu);
        if (selected < 0) return;
        compare(0x1b0130u, selected, behavior_count, 2);
        at(0x1b0134u);
        if (selected >= behavior_count) return;
        load_register(ebx, 0x1b0136u, behavior_epoch);
        at(0x1b013cu);
        reg(edx) = std::uint32_t(selected);
        load_register(ecx, 0x1b013fu, descriptor_table + reg(edx) * 4u);
        load_register(edi, 0x1b0146u, reg(ecx) + 8u);
        compare(0x1b0149u, reg(edi), reg(ebx), 4);
        at(0x1b014bu);
        if (reg(edi) == reg(ebx)) return;
        load_register(edi, 0x1b014du, reg(ecx) + 4u);
        load_register(ecx, 0x1b0150u, required_flags);
        at(0x1b0156u);
        reg(edi) = state_.flags.logic(reg(edi) & reg(ecx), 4);
        compare(0x1b0158u, reg(edi), reg(ecx), 4);
        at(0x1b015au);
        if (reg(edi) != reg(ecx)) return;
        at(0x1b015cu);
        reg(ecx) = reg(edx);
        at(0x1b015eu);
        reg(ecx) = state_.flags.logic(reg(ecx) & 31u, 4);
        at(0x1b0161u);
        reg(edi) = 1;
        at(0x1b0166u);
        reg(edi) = state_.flags.shift(reg(edi), reg(ecx), 4);
        at(0x1b0168u);
        reg(edx) = state_.flags.right(reg(edx), 5, 4, true);
        const auto enabled = load(0x1b016bu, enabled_behaviors + reg(edx) * 4u) & reg(edi);
        state_.flags.logic(enabled, 4);
        at(0x1b0172u);
        if (!enabled) return;
        load_register(edx, 0x1b0174u, stack(0x18u));
        store(0x1b0178u, reg(edx), std::uint16_t(selected));
        load_register(eax, 0x1b017bu, stack(0x1cu));
        store(0x1b017fu, reg(eax), 3);
        at(0x1b0184u);
    }

    void score_behavior() {
        load_register(ecx, 0x1b0186u, stack(0x18u));
        store(0x1b018au, reg(ecx), state_.reg(edx, 2));
        load_register(eax, 0x1b018du, reg(edi) + 0x10u);
        test(0x1b0190u, reg(eax), 4);
        at(0x1b0192u);
        if (reg(eax)) {
            push(0x1b0194u, ebx);
            at(0x1b0195u);
            invoke(0x1b0195u, 0x1b0197u, reg(eax));
            load_register(edx, 0x1b0197u, stack(0x1cu));
            store(0x1b019bu, reg(edx), state_.reg(eax, 2));
            at(0x1b019eu);
        } else {
            load_register(eax, 0x1b01a0u, stack(0x1cu));
            store(0x1b01a4u, reg(eax), 0u);
        }
    }

    void record_attempt() {
        load_register(eax, 0x1b01a9u, reg(ebp) + 4u, 2);
        const auto cooldown = signed_low(eax);
        test(0x1b01adu, cooldown, 2);
        at(0x1b01b0u);
        if (cooldown < 0) return;
        compare(0x1b01b2u, cooldown, cooldown_count, 2);
        at(0x1b01b6u);
        if (cooldown >= cooldown_count) return;
        load_register(edx, 0x1b01b8u, game_time);
        at(0x1b01beu);
        reg(ecx) = std::uint32_t(cooldown);
        load_register(eax, 0x1b01c1u, reg(edx) + 8u);
        at(0x1b01c4u);
        physics_world_runtime_detail::w(state_.memory, state_.observe, 0x1b01c4u,
            reg(esi) + actor_cooldowns + reg(ecx) * 4u, 4, reg(eax));
    }

    void release_descriptor() {
        pop(0x1b01cbu, edi);
        pop(0x1b01ccu, esi);
    }

    void finish() {
        pop(0x1b01cdu, ebp);
        pop(0x1b01ceu, ebx);
        at(0x1b01cfu);
        state_.pop();
        reg(esp) += 20u;
    }
};

inline bool reference_requested() {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CAMPAIGN_AI_BEHAVIOR_CANDIDATE");
        return value && *value;
    }();
    return reference;
}

template<class State, class Dispatch>
bool bridge(State &state, Dispatch dispatch) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference_requested()) return false;
    Frame frame(state, dispatch);
    if (frame.initialize_selection()) {
        if (frame.descriptor_enabled() && frame.cooldown_ready()) {
            if (frame.selects_child()) frame.select_child();
            else frame.score_behavior();
            frame.record_attempt();
        }
        frame.release_descriptor();
    }
    frame.finish();
    return true;
}

} // namespace halo2::native_campaign_ai_behavior_candidate
