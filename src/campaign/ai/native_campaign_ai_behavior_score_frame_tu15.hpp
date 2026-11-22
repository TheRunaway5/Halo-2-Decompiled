#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_campaign_ai_behavior_score_detail {
using Word = std::uint32_t;
using physics_step_detail::FpScope;
using physics_step_detail::r;
using physics_step_detail::w;

struct Frame final : physics_world_runtime_detail::WorldState {
    using WorldState::WorldState;

    void at(Word pc) {
        if (services.instruction) services.instruction(pc);
    }
    Word read(Word pc, Word address, unsigned width = 4) {
        at(pc);
        return r(memory, address, width);
    }
    __m128 scalar(Word pc, Word address) {
        return _mm_set_ss(std::bit_cast<float>(read(pc, address)));
    }
    void store(Word pc, Word address, Word value) {
        at(pc);
        w(memory, observe, pc, address, 4, value);
    }
    void push_word(Word pc, Word value) {
        at(pc);
        const auto target = gp[4] - 4u;
        if (observe) {
            const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
                gp[4] = target;
                observe(store);
            };
            w(memory, completed, pc, target, 4, value);
        } else {
            w(memory, observe, pc, target, 4, value);
            gp[4] = target;
        }
    }
    Word pop_word(Word pc) {
        at(pc);
        return pop();
    }
    void finish(Word pc, Word continuation, Word argument_bytes = 0) {
        if (read(pc, gp[4]) != continuation)
            throw std::runtime_error("Native behavior score has a different actual return word");
        gp[4] += 4u + argument_bytes;
    }
    void call(Word pc, Word target, Word continuation, Word argument_bytes = 0) {
        const auto expected_sp = gp[4] + argument_bytes;
        push_word(pc, continuation);
        if (!services.required_owner || active_mmx)
            throw std::runtime_error("Native behavior score requires the actual child owner");
        auto child = save();
        try {
            services.required_owner(memory, {pc, target, false}, child, observe);
        } catch (...) {
            load(child);
            throw;
        }
        load(child);
        if (gp[4] != expected_sp)
            throw std::runtime_error("Native behavior score child returned an unbalanced frame");
    }
};
}
