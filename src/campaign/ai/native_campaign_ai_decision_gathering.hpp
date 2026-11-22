#pragma once
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_campaign_ai_decision_gathering {

template<class State>
class Frame {
    enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
    static constexpr std::uint32_t actor_pool = 0x5020c8u;
    static constexpr std::uint32_t context_serial = 0x47b908u;
    static constexpr std::uint32_t working_mask = 0x56ae84u;
    static constexpr std::uint32_t context = 0x4fafc0u;
    static constexpr std::uint32_t records = 0x4fafc8u;
    static constexpr std::uint32_t counts = 0x4fd4a0u;
    static constexpr std::uint32_t pending = 0x4fd524u;
    static constexpr std::uint32_t behavior_count = 131;
    static constexpr std::uint32_t lane_stride = 12;

    State &state_;
    void (*collect_mask_)(State &);
    void (*select_context_)(State &);

    std::uint32_t &reg(Register name) { return state_.gp[name]; }
    std::uint32_t stack(std::uint32_t offset) { return reg(esp) + offset; }
    void at(std::uint32_t pc) {
        if (!native_census_detail::skip_report(state_.services, pc)) state_.instruction_literal(pc);
    }
    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return physics_world_runtime_detail::r(state_.memory, address, width);
    }
    void write(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        physics_world_runtime_detail::w(state_.memory, state_.observe, pc, address, width, value);
    }
    void store(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        at(pc);
        write(pc, address, width, value);
    }
    void push(std::uint32_t pc, Register name) {
        at(pc);
        state_.push(pc, reg(name));
    }
    void call(std::uint32_t pc, void (*child)(State &)) {
        at(pc);
        state_.push(pc, pc + 5u);
        child(state_);
    }
    std::uint32_t behavior_kind(std::uint32_t pc) {
        return std::bit_cast<std::int16_t>(std::uint16_t(load(pc, reg(eax), 2)));
    }

    void begin() {
        at(0x1b08b0u);
        reg(esp) = state_.flags.minus(reg(esp), 0x4cu, 4);
        reg(eax) = load(0x1b08b3u, actor_pool);
        reg(ecx) = load(0x1b08b8u, reg(eax) + 0x44u);
        reg(eax) = load(0x1b08bbu, context_serial);
        push(0x1b08c0u, ebx);
        push(0x1b08c1u, ebp);
        push(0x1b08c2u, esi);
        reg(esi) = load(0x1b08c3u, stack(0x5cu));
        at(0x1b08c7u);
        reg(ebp) = reg(esi);
        at(0x1b08c9u);
        reg(ebp) = state_.flags.logic(reg(ebp) & 0xffffu, 4);
        at(0x1b08cfu);
        reg(ebp) = state_.flags.product(reg(ebp), 0x888u);
        at(0x1b08d5u);
        reg(ebp) = state_.flags.plus(reg(ebp), reg(ecx), 4);
        push(0x1b08d7u, edi);
        at(0x1b08d8u);
        reg(eax) = state_.flags.increment(reg(eax), 4, false);
        at(0x1b08d9u);
        reg(edi) = working_mask;
        at(0x1b08deu);
        reg(ecx) = reg(esi);
        store(0x1b08e0u, stack(0x14u), 4, reg(ebp));
        store(0x1b08e4u, stack(0x10u), 4, 0);
        store(0x1b08ecu, context_serial, 4, reg(eax));
        call(0x1b08f1u, collect_mask_);
        at(0x1b08f6u);
        reg(eax) = reg(esi);
        call(0x1b08f8u, select_context_);
        at(0x1b08fdu);
        state_.xmm[0] = _mm_setzero_ps();
        at(0x1b0900u);
        reg(ecx) = state_.flags.logic(0, 4);
        store(0x1b0902u, pending, 4, reg(ecx));
        store(0x1b0908u, pending + 4u, 4, reg(ecx));
        store(0x1b090eu, pending + 8u, 4, reg(ecx));
        store(0x1b0914u, pending + 12u, 4, reg(ecx));
        store(0x1b091au, context, 4, reg(eax));
        store(0x1b091fu, pending + 16u, 4, reg(ecx));
        at(0x1b0925u);
        reg(eax) = reg(ebp) + 0x1c8u;
        at(0x1b092bu);
        reg(edi) = 3;
    }

    bool has_candidate() {
        at(0x1b0930u);
        reg(esi) = state_.flags.logic(0xffffffffu, 4);
        const auto datum = load(0x1b0933u, reg(eax) - 4u, 2);
        state_.flags.minus(datum, reg(esi), 2);
        at(0x1b0937u);
        return datum != 0xffffu;
    }

    void start_kind() {
        reg(edx) = behavior_kind(0x1b093du);
        at(0x1b0940u);
        reg(ecx) = reg(edx);
        at(0x1b0942u);
        reg(ecx) = state_.flags.logic(reg(ecx) & 31u, 4);
        at(0x1b0945u);
        reg(ebx) = 1;
        at(0x1b094au);
        reg(ebx) = state_.flags.shift(reg(ebx), reg(ecx), 4);
        at(0x1b094cu);
        reg(edx) = state_.flags.right(reg(edx), 5, 4, true);
        const auto old = load(0x1b094fu, pending + reg(edx) * 4);
        state_.flags.logic(old & reg(ebx), 4);
        at(0x1b0956u);
        if (state_.flags.zf) {
            reg(ecx) = behavior_kind(0x1b0958u);
            at(0x1b095bu);
            reg(edx) = reg(ecx);
            at(0x1b095du);
            reg(ecx) = state_.flags.logic(reg(ecx) & 31u, 4);
            at(0x1b0960u);
            reg(edx) = state_.flags.right(reg(edx), 5, 4, true);
            at(0x1b0963u);
            reg(ebx) = 1;
            at(0x1b0968u);
            reg(ebx) = state_.flags.shift(reg(ebx), reg(ecx), 4);
            reg(ecx) = load(0x1b096au, pending + reg(edx) * 4);
            at(0x1b0971u);
            reg(edx) = pending + reg(edx) * 4;
            at(0x1b0978u);
            reg(ecx) = state_.flags.logic(reg(ecx) | reg(ebx), 4);
            store(0x1b097au, reg(edx), 4, reg(ecx));
            reg(ecx) = behavior_kind(0x1b097cu);
            store(0x1b097fu, counts + reg(ecx), 1, 0);
        }
    }

    void append_candidate() {
        reg(edx) = behavior_kind(0x1b0986u);
        reg(ecx) = load(0x1b0989u, counts + reg(edx), 1);
        at(0x1b0990u);
        reg(ecx) = state_.flags.product(reg(ecx), behavior_count);
        at(0x1b0996u);
        reg(ecx) = state_.flags.plus(reg(ecx), reg(edx), 4);
        state_.set(edx, load(0x1b0998u, reg(eax) - 2u, 2), 2);
        at(0x1b099cu);
        reg(ecx) *= 3;
        at(0x1b099fu);
        reg(ecx) = records + reg(ecx) * 8;
        store(0x1b09a6u, reg(ecx), 2, reg(edx));
        state_.set(edx, load(0x1b09a9u, reg(eax) + 2u, 2), 2);
        store(0x1b09adu, reg(ecx) + 2u, 2, reg(edx));
        state_.set(edx, load(0x1b09b1u, reg(eax) - 4u, 2), 2);
        store(0x1b09b5u, reg(ecx) + 4u, 2, reg(edx));
        store(0x1b09b9u, reg(ecx) + 6u, 1, 1);
        store(0x1b09bdu, reg(ecx) + 8u, 2, reg(esi));
        store(0x1b09c1u, reg(ecx) + 12u, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(state_.xmm[0])));
        store(0x1b09c6u, reg(ecx) + 16u, 4, 0);
        reg(ecx) = behavior_kind(0x1b09cdu);
        const auto address = counts + reg(ecx);
        const auto count = load(0x1b09d0u, address, 1);
        write(0x1b09d0u, address, 1, state_.flags.increment(count, 1, false));
    }

public:
    Frame(State &state, void (*collect_mask)(State &), void (*select_context)(State &))
        : state_(state), collect_mask_(collect_mask), select_context_(select_context) {}

    void gather() {
        begin();
        do {
            if (has_candidate()) {
                start_kind();
                append_candidate();
            }
            at(0x1b09d6u);
            reg(eax) = state_.flags.plus(reg(eax), lane_stride, 4);
            at(0x1b09d9u);
            reg(edi) = state_.flags.increment(reg(edi), 4, true);
            at(0x1b09dau);
        } while (!state_.flags.zf);
    }
};

inline bool reference_requested() {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CAMPAIGN_AI_DECISION_GATHERING");
        return value && *value;
    }();
    return reference;
}

template<class State>
bool bridge(State &state, void (*collect_mask)(State &), void (*select_context)(State &)) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference_requested()) return false;
    Frame frame(state, collect_mask, select_context);
    frame.gather();
    return true;
}

} // namespace halo2::native_campaign_ai_decision_gathering
