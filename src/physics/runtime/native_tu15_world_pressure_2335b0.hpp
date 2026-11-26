#pragma once

#include "halo2/detail/physics_scalar_memory.hpp"
#include "physics/runtime/native_tu15_pool_pressure_metric.hpp"
#include <cstdlib>

namespace halo2::native_world_pressure_2335b0 {

namespace metric = native_pool_pressure_metric;
inline constexpr std::uint32_t entry = 0x2335b0u;
inline constexpr unsigned eax=0, ecx=1, edx=2, esp=4, esi=6;

template<class State> bool selected(const State &state) {
    return state.selected_observation_pcs && state.image == SourcePhysicsContactImage::TitleUpdate15;
}

template<class State> bool quiet(const State &state) {
    const auto *cursor = state.immutable_cursor.cursor;
    return cursor && !(cursor->first_trace && *cursor->first_trace) &&
        !(cursor->second_trace && *cursor->second_trace);
}

template<class State> void publish(State &state, std::uint32_t pc) {
    if(quiet(state)) *state.immutable_cursor.cursor->position = pc;
    else if(state.services.instruction) state.services.instruction(pc);
}

template<class State> void push_esi(State &state) {
    const auto address = state.gp[esp] - 4u;
    const auto saved = state.gp[esi];
    // Commit ESP after the write succeeds and before delivering a post-store observer.
    if(state.observe) {
        const SourcePhysicsContactRecordObserver observe = [&](const SourcePhysicsContactRecordStore &event) {
            state.gp[esp] = address;
            state.observe(event);
        };
        physics_step_detail::w(state.memory,observe,entry,address,4,saved);
    } else {
        physics_step_detail::w(state.memory,{},entry,address,4,saved);
        state.gp[esp] = address;
    }
}

inline const std::byte *slot_row(const SourcePhysicsContactMemory &memory, std::uint32_t last) {
    const auto *page = memory.direct_page(last,metric::slot_count * metric::slot_size);
    return page ? page->bytes + (last & 4095u) : nullptr;
}

inline std::uint32_t word(const std::byte *bytes) {
    std::uint32_t value;
    std::memcpy(&value,bytes,sizeof(value));
    return value;
}

template<class State> bool reduce_direct(State &state) {
    if constexpr(std::endian::native != std::endian::little) return false;
    auto &g = state.gp;
    if(!quiet(state) || g[edx] != metric::slot_count) return false;
    const auto last = g[eax] - (metric::slot_count - 1u) * metric::slot_size;
    const auto *totals = slot_row(state.memory,last);
    const auto *occupied = slot_row(state.memory,last + metric::occupied_offset);
    const auto *strides = slot_row(state.memory,last + metric::stride_offset);
    if(!totals || !occupied || !strides) return false;

    // The three complete bank ranges admit no fault, callback, or trace.
    // Unsigned sums and products preserve the source's modulo-32-bit result.
    auto sum = g[ecx];
    auto offset = metric::slot_count * metric::slot_size;
    std::uint32_t product;
    do {
        offset -= metric::slot_size;
        product = metric::product(word(totals + offset),word(occupied + offset),word(strides + offset));
        sum += product;
    } while(offset);
    g[esi] = product;
    g[ecx] = sum;
    g[eax] = state.flags.minus(last,metric::slot_size,4);
    g[edx] = state.flags.increment(1u,4,true);
    return true;
}

template<class State> void reduce_ordered(State &state) {
    using physics_step_detail::r;
    auto &g = state.gp;
    auto &flags = state.flags;
    do {
        publish(state,0x2335c0u);
        g[esi] = r(state.memory,g[eax],4);
        publish(state,0x2335c2u);
        const auto occupied = r(state.memory,g[eax] + metric::occupied_offset,4);
        g[esi] = flags.minus(g[esi],occupied,4);
        publish(state,0x2335c5u);
        const auto stride = r(state.memory,g[eax] + metric::stride_offset,4);
        g[esi] = flags.product(g[esi],stride);
        publish(state,0x2335ccu);
        g[ecx] = flags.plus(g[ecx],g[esi],4);
        publish(state,0x2335ceu);
        g[eax] = flags.minus(g[eax],metric::slot_size,4);
        publish(state,0x2335d1u);
        g[edx] = flags.increment(g[edx],4,true);
        publish(state,0x2335d2u);
    } while(!flags.zf);
}

template<class State> bool run(State &state) {
    static const bool replay = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WORLD_PRESSURE_2335B0");
        return value && *value;
    }();
    if(replay || !selected(state)) return false;
    auto &g = state.gp;
    auto &flags = state.flags;
    publish(state,entry);
    push_esi(state);
    publish(state,0x2335b1u);
    g[ecx] = flags.logic(0u,4);
    publish(state,0x2335b3u);
    g[eax] = flags.plus(g[eax],metric::totals_offset,4);
    publish(state,0x2335b8u);
    g[edx] = metric::slot_count;
    publish(state,0x2335bdu);
    if(!reduce_direct(state)) reduce_ordered(state);
    publish(state,0x2335d4u);
    state.xmm[0] = metric::convert(state.xmm[0],g[ecx]);
    publish(state,0x2335d8u);
    const auto scale = physics_step_detail::r(state.memory,metric::scale_address,4);
    state.xmm[0] = metric::scale(state.xmm[0],scale);
    publish(state,0x2335e0u);
    g[esi] = state.pop();
    publish(state,0x2335e1u);
    state.pop();
    return true;
}

} // namespace halo2::native_world_pressure_2335b0
