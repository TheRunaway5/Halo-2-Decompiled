#pragma once
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_campaign_ai_behavior_search {

template<class State>
class Frame {
    enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
    static constexpr std::uint32_t initial_behavior = 0x47c1a4u;
    static constexpr std::uint32_t next_record = 0xcu;
    static constexpr std::uint32_t record_kind = 0x10u;
    static constexpr std::int16_t no_behavior = -1;

    struct RankSites {
        std::uint32_t load_score, compare_best, reject, compare_threshold, load_behavior, keep_score, store_group, stop;
    };
    static constexpr RankSites priority_rank{0x1b0264u, 0x1b0268u, 0x1b026bu, 0x1b026du,
        0x1b0271u, 0x1b0275u, 0x1b0277u, 0x1b027bu};
    static constexpr RankSites direct_rank{0x1b02f2u, 0x1b02f6u, 0x1b02f9u, 0x1b02fbu,
        0x1b02ffu, 0x1b0303u, 0x1b0305u, 0x1b0309u};
    static constexpr RankSites remaining_rank{0x1b032eu, 0x1b0332u, 0x1b0335u, 0x1b0337u,
        0x1b033bu, 0x1b033fu, 0x1b0341u, 0x1b0345u};
    struct LinkedCallSites {
        std::uint32_t load_clock, load_context, push_clock, push_context, score_address, push_score,
            behavior_address, push_behavior, load_actor, push_candidate, call, resume;
    };
    static constexpr LinkedCallSites priority_call{0x1b0246u, 0x1b024au, 0x1b024eu, 0x1b024fu,
        0x1b0250u, 0x1b0254u, 0x1b0255u, 0x1b0259u, 0x1b025au, 0x1b025eu, 0x1b025fu, 0x1b0264u};
    static constexpr LinkedCallSites remaining_call{0x1b0310u, 0x1b0314u, 0x1b0318u, 0x1b0319u,
        0x1b031au, 0x1b031eu, 0x1b031fu, 0x1b0323u, 0x1b0324u, 0x1b0328u, 0x1b0329u, 0x1b032eu};

    State &state_;
    void (*evaluate_)(State &);

    void at(std::uint32_t pc) {
        if (!native_census_detail::skip_report(state_.services, pc)) state_.instruction_literal(pc);
    }
    std::uint32_t &reg(Register name) { return state_.gp[name]; }
    std::uint32_t stack(std::uint32_t offset) { return reg(esp) + offset; }
    static std::int16_t signed_half(std::uint32_t value) {
        return std::bit_cast<std::int16_t>(std::uint16_t(value));
    }
    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_world_runtime_detail::r(state_.memory, address, width);
    }
    void load_register(Register name, std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        state_.set(name, load(pc, address, width), width, 0);
    }
    void store(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        at(pc);
        physics_world_runtime_detail::w(state_.memory, state_.observe, pc, address, width, value);
    }
    void push(std::uint32_t pc, Register name) {
        at(pc);
        state_.push(pc, reg(name));
    }
    void pop(std::uint32_t pc, Register name) {
        at(pc);
        reg(name) = state_.pop();
    }
    void test(std::uint32_t pc, std::uint32_t value, unsigned width) {
        at(pc);
        state_.flags.logic(value, width);
    }
    bool first_group(std::uint32_t read_site, std::uint32_t branch_site) {
        const auto entry = load(read_site, stack(0x2cu), 2);
        state_.flags.minus(reg(ebp), entry, 2);
        at(branch_site);
        return signed_half(reg(ebp)) == signed_half(entry);
    }
    bool next_linked_record(std::uint32_t read_site, std::uint32_t test_site, std::uint32_t branch_site) {
        load_register(edi, read_site, reg(edi) + next_record);
        test(test_site, reg(edi), 4);
        at(branch_site);
        return reg(edi) != 0;
    }
    bool rank(const RankSites &sites) {
        load_register(eax, sites.load_score, stack(0x14u));
        const auto score = signed_half(reg(eax));
        at(sites.compare_best);
        state_.flags.minus(reg(eax), reg(esi), 2);
        at(sites.reject);
        if (score <= signed_half(reg(esi))) return false;

        at(sites.compare_threshold);
        state_.flags.minus(reg(eax), 1, 2);
        load_register(ebx, sites.load_behavior, stack(0x18u));
        at(sites.keep_score);
        reg(esi) = reg(eax);
        store(sites.store_group, stack(0x1cu), 4, reg(ebp));
        at(sites.stop);
        return score > 1;
    }
    bool evaluate_linked(const LinkedCallSites &sites, const RankSites &ranking) {
        load_register(eax, sites.load_clock, stack(0x38u));
        load_register(ecx, sites.load_context, stack(0x34u));
        push(sites.push_clock, eax);
        push(sites.push_context, ecx);
        at(sites.score_address);
        reg(edx) = stack(0x1cu);
        push(sites.push_score, edx);
        at(sites.behavior_address);
        reg(eax) = stack(0x24u);
        push(sites.push_behavior, eax);
        load_register(eax, sites.load_actor, stack(0x34u));
        push(sites.push_candidate, edi);
        at(sites.call);
        state_.push(sites.call, sites.resume);
        evaluate_(state_);
        return rank(ranking);
    }

public:
    Frame(State &state, void (*evaluate)(State &)) : state_(state), evaluate_(evaluate) {}

    bool begin() {
        at(0x1b01e0u);
        reg(esp) = state_.flags.minus(reg(esp), 0x10u, 4);
        push(0x1b01e3u, ebx);
        load_register(ebx, 0x1b01e4u, initial_behavior);
        push(0x1b01eau, ebp);
        load_register(ebp, 0x1b01ebu, stack(0x24u));
        push(0x1b01efu, esi);
        at(0x1b01f0u);
        reg(esi) = state_.flags.logic(0u, 4);
        const auto end = load(0x1b01f2u, stack(0x2cu), 2);
        state_.flags.minus(reg(ebp), end, 2);
        store(0x1b01f7u, stack(0x18u), 4, 0xffffffffu);
        at(0x1b01ffu);
        if (signed_half(reg(ebp)) >= signed_half(end)) return false;
        push(0x1b0205u, edi);
        return true;
    }

    bool begin_priority_chain() {
        load_register(eax, 0x1b0206u, stack(0x40u), 1);
        at(0x1b020au);
        reg(edi) = state_.flags.logic(0u, 4);
        test(0x1b020cu, reg(eax), 1);
        at(0x1b020eu);
        if (!state_.reg(eax, 1)) return false;
        load_register(ecx, 0x1b0214u, stack(0x28u));
        at(0x1b0218u);
        reg(eax) = std::uint32_t(signed_half(reg(ebp)));
        load_register(edx, 0x1b021bu, reg(ecx) + reg(eax) * 4u);
        load_register(edi, 0x1b021eu, reg(edx) + next_record);
        test(0x1b0221u, reg(edi), 4);
        at(0x1b0223u);
        if (!reg(edi)) return false;
        store(0x1b0225u, stack(0x13u), 1, 0u);
        at(0x1b022au);
        return true;
    }
    bool priority_record() {
        const auto kind = load(0x1b0230u, reg(edi) + record_kind, 2);
        state_.flags.minus(kind, 2, 2);
        at(0x1b0235u);
        return signed_half(kind) < 2;
    }
    bool consider_priority() {
        load_register(eax, 0x1b0237u, stack(0x44u), 1);
        test(0x1b023bu, reg(eax), 1);
        at(0x1b023du);
        return !state_.reg(eax, 1) || !first_group(0x1b023fu, 0x1b0244u);
    }
    bool evaluate_priority() { return evaluate_linked(priority_call, priority_rank); }
    bool next_priority() {
        const auto kind = load(0x1b0281u, reg(edi) + record_kind, 2);
        state_.flags.minus(kind, 1, 2);
        at(0x1b0286u);
        if (kind == 1) store(0x1b0288u, stack(0x13u), 1, 1u);
        return next_linked_record(0x1b028du, 0x1b0290u, 0x1b0292u);
    }
    bool priority_replaces_regular() {
        load_register(eax, 0x1b0294u, stack(0x13u), 1);
        test(0x1b0298u, reg(eax), 1);
        at(0x1b029au);
        return state_.reg(eax, 1) != 0;
    }

    bool current_behavior_matches() {
        load_register(edx, 0x1b02a0u, stack(0x28u));
        at(0x1b02a4u);
        reg(ecx) = std::uint32_t(signed_half(reg(ebp)));
        load_register(eax, 0x1b02a7u, reg(edx) + reg(ecx) * 4u);
        load_register(ecx, 0x1b02aau, stack(0x3cu), 2);
        const auto behavior = load(0x1b02afu, reg(eax), 2);
        state_.flags.minus(reg(ecx), behavior, 2);
        at(0x1b02b2u);
        return state_.reg(ecx, 2) == behavior;
    }
    void keep_current_group() {
        load_register(ecx, 0x1b035cu, stack(0x34u));
        store(0x1b0360u, reg(ecx) + 4u, 2, state_.reg(ebp, 2));
    }
    bool consider_regular() {
        load_register(edx, 0x1b02b8u, stack(0x44u), 1);
        test(0x1b02bcu, reg(edx), 1);
        at(0x1b02beu);
        if (state_.reg(edx, 1) && first_group(0x1b02c0u, 0x1b02c5u)) return false;
        at(0x1b02c7u);
        state_.flags.minus(reg(ecx), 0xffffffffu, 2);
        at(0x1b02cbu);
        if (signed_half(reg(ecx)) == no_behavior) return true;
        load_register(ecx, 0x1b02cdu, reg(eax) + 2u, 1);
        test(0x1b02d0u, reg(ecx), 1);
        at(0x1b02d2u);
        return state_.reg(ecx, 1) != 0;
    }
    bool evaluate_regular() {
        load_register(ecx, 0x1b02d4u, stack(0x38u));
        load_register(edx, 0x1b02d8u, stack(0x34u));
        push(0x1b02dcu, ecx);
        push(0x1b02ddu, edx);
        at(0x1b02deu);
        reg(ecx) = stack(0x1cu);
        push(0x1b02e2u, ecx);
        at(0x1b02e3u);
        reg(edx) = stack(0x24u);
        push(0x1b02e7u, edx);
        push(0x1b02e8u, eax);
        load_register(eax, 0x1b02e9u, stack(0x38u));
        at(0x1b02edu);
        state_.push(0x1b02edu, 0x1b02f2u);
        evaluate_(state_);
        return rank(direct_rank);
    }
    bool begin_remaining_chain() {
        test(0x1b030bu, reg(edi), 4);
        at(0x1b030du);
        if (!reg(edi)) return false;
        at(0x1b030fu);
        return true;
    }
    bool evaluate_remaining() { return evaluate_linked(remaining_call, remaining_rank); }
    bool next_remaining() { return next_linked_record(0x1b0347u, 0x1b034au, 0x1b034cu); }

    bool next_group() {
        at(0x1b034eu);
        reg(ebp) = state_.flags.increment(reg(ebp), 4, false);
        const auto end = load(0x1b034fu, stack(0x30u), 2);
        state_.flags.minus(reg(ebp), end, 2);
        at(0x1b0354u);
        if (signed_half(reg(ebp)) < signed_half(end)) return true;
        at(0x1b035au);
        return false;
    }
    void finish(bool have_group) {
        if (have_group) pop(0x1b0364u, edi);
        load_register(edx, 0x1b0365u, stack(0x44u));
        load_register(eax, 0x1b0369u, stack(0x48u));
        load_register(ecx, 0x1b036du, stack(0x18u), 2);
        store(0x1b0372u, reg(edx), 2, state_.reg(esi, 2));
        pop(0x1b0375u, esi);
        store(0x1b0376u, reg(eax), 2, state_.reg(ecx, 2));
        pop(0x1b0379u, ebp);
        at(0x1b037au);
        state_.set(eax, state_.reg(ebx, 2), 2, 0);
        pop(0x1b037du, ebx);
        at(0x1b037eu);
        reg(esp) = state_.flags.plus(reg(esp), 0x10u, 4);
        at(0x1b0381u);
        state_.pop();
        reg(esp) += 44u;
    }
};

template<class Frame>
void search(Frame &frame) {
    do {
        if (frame.begin_priority_chain()) {
            do {
                if (!frame.priority_record()) break;
                if (frame.consider_priority() && frame.evaluate_priority()) return;
            } while (frame.next_priority());
            if (frame.priority_replaces_regular()) continue;
        }

        if (frame.current_behavior_matches()) {
            frame.keep_current_group();
            return;
        }
        if (frame.consider_regular() && frame.evaluate_regular()) return;
        if (frame.begin_remaining_chain()) {
            do {
                if (frame.evaluate_remaining()) return;
            } while (frame.next_remaining());
        }
    } while (frame.next_group());
}

inline bool reference_requested() {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CAMPAIGN_AI_BEHAVIOR_SEARCH");
        return value && *value;
    }();
    return reference;
}

template<class State>
bool bridge(State &state, void (*evaluate)(State &)) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference_requested()) return false;
    Frame frame(state, evaluate);
    const auto have_group = frame.begin();
    if (have_group) search(frame);
    frame.finish(have_group);
    return true;
}

} // namespace halo2::native_campaign_ai_behavior_search
