#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_sphere_center_bounds_detail {

inline constexpr std::uint32_t complete = 0xffffffffu;

inline bool reference_center() noexcept {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_SPHERE_CENTER_2E8D70");
        return value && *value;
    }();
    return reference;
}

inline bool reference_bounds() noexcept {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_SPHERE_BOUNDS_2E8D90");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class Frame {
    State &state_;
    const typename State::InstructionCursor *cursor_;

    template<class Value>
    bool read(std::uint32_t address, Value &value) const {
        return native_raw_x87_detail::direct_read(
            state_.memory, address, std::as_writable_bytes(std::span{&value, 1}));
    }

    bool vector(std::uint32_t address, __m128 &value) const {
        return (address & 15u) == 0 && read(address, value);
    }

    void publish(std::uint32_t pc) const {
        if (cursor_)
            *cursor_->position = pc;
    }

    bool write_word(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        publish(pc);
        if (state_.memory.try_callback_free_write(address, std::as_bytes(std::span{&value, 1})))
            return true;
        physics_step_detail::w(state_.memory, state_.observe, pc, address, 4u, value);
        return false;
    }

    void write_vector(std::uint32_t pc, std::uint32_t address, __m128 value) {
        publish(pc);
        // Both half-stores belong to this instruction. A provider can fault
        // or switch tracing between the halves.
        physics_step_detail::vw(state_.memory, state_.observe, pc, address, value);
    }

    bool push_frame(std::uint32_t pc) {
        auto &gp = state_.gp;
        gp[4] -= 4u;
        return write_word(pc, gp[4], gp[5]);
    }

    bool can_continue() const {
        return native_mopp_ray_scale_tail_detail::provider_continuation(state_, cursor_);
    }

    bool bounds_fp() const {
        return state_.fp.count < 8u && native_raw_x87_detail::supported(state_);
    }

    std::uint32_t finish(std::uint32_t pop_pc, std::uint32_t return_pc, unsigned argument_bytes) {
        auto &gp = state_.gp;
        gp[4] = gp[5];
        if (!read(gp[4], gp[5]))
            return pop_pc;
        gp[4] += 4u;
        std::uint32_t return_address;
        if (!read(gp[4], return_address))
            return return_pc;
        gp[4] += 4u + argument_bytes;
        publish(return_pc);
        return complete;
    }

public:
    Frame(State &state, const typename State::InstructionCursor *cursor)
        : state_(state), cursor_(cursor) {}

    std::uint32_t center() {
        auto &gp = state_.gp;
        auto &xmm = state_.xmm;
        if (!can_continue())
            return 0;
        if (!push_frame(0x2e8d70u) && !can_continue())
            return 0x2e8d71u;
        gp[5] = gp[4];
        if (!read(gp[5] + 8u, gp[0]))
            return 0x2e8d73u;
        xmm[0] = _mm_setzero_ps();
        gp[4] = state_.flags.logic(gp[4] & 0xfffffff0u, 4u);
        if (gp[0] & 15u)
            return 0x2e8d7cu;
        write_vector(0x2e8d7cu, gp[0], xmm[0]);
        if (!can_continue())
            return 0x2e8d7fu;
        return finish(0x2e8d81u, 0x2e8d82u, 4u);
    }

    template<class Spill>
    std::uint32_t bounds(Spill spill) {
        using namespace physics_step_detail;
        auto &gp = state_.gp;
        auto &xmm = state_.xmm;
        if (!can_continue() || !bounds_fp())
            return 0;
        if (!push_frame(0x2e8d90u) && (!can_continue() || !bounds_fp()))
            return 0x2e8d91u;
        gp[5] = gp[4];
        gp[4] = state_.flags.logic(gp[4] & 0xfffffff0u, 4u);
        gp[4] = state_.flags.minus(gp[4], 0x10u, 4u);
        std::uint32_t word;
        if (!read(gp[5] + 12u, word))
            return 0x2e8d99u;
        state_.fp.push(ext(f(word)));
        if (!read(gp[5] + 8u, gp[0]))
            return 0x2e8d9cu;
        if (!read(gp[1] + 12u, word))
            return 0x2e8d9fu;
        state_.fp.st(0) = add(state_.fp.st(0), ext(f(word)));
        if (!vector(gp[0] + 0x30u, xmm[1]))
            return 0x2e8da2u;
        if (!read(gp[5] + 0x10u, gp[1]))
            return 0x2e8da6u;
        const bool direct_spill = write_word(
            0x2e8da9u, gp[4] + 12u, spill(state_.fp.st(0), state_.fp.status));
        state_.fp.pop();
        if ((!direct_spill && (!can_continue() || !native_raw_x87_detail::supported(state_))) ||
            !read(gp[4] + 12u, word))
            return 0x2e8dadu;
        xmm[0] = _mm_set_ss(f(word));
        xmm[0] = _mm_shuffle_ps(xmm[0], xmm[0], 0);
        xmm[1] = _mm_sub_ps(xmm[1], xmm[0]);
        if (gp[1] & 15u)
            return 0x2e8dbau;
        write_vector(0x2e8dbau, gp[1], xmm[1]);
        if (!can_continue() || !native_raw_x87_detail::supported(state_) ||
            !vector(gp[0] + 0x30u, xmm[1]))
            return 0x2e8dbdu;
        xmm[1] = native_tu15_leaves::ordered_add_ps(xmm[1], xmm[0]);
        if (gp[1] & 15u)
            return 0x2e8dc4u;
        write_vector(0x2e8dc4u, gp[1] + 16u, xmm[1]);
        if (!can_continue())
            return 0x2e8dc8u;
        return finish(0x2e8dcau, 0x2e8dcbu, 12u);
    }
};
#endif

template<class State>
std::uint32_t center(State &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (reference_center() || native_raw_x87_detail::scoped_access_reference())
        return 0;
    return Frame(state, native_mopp_ray_scale_tail_detail::admitted_cursor(state)).center();
#else
    (void)state;
    return 0;
#endif
}

template<class State, class Spill>
std::uint32_t bounds(State &state, Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (reference_bounds() || native_raw_x87_detail::scoped_access_reference())
        return 0;
    return Frame(state, native_mopp_ray_scale_tail_detail::admitted_cursor(state)).bounds(spill);
#else
    (void)state;
    (void)spill;
    return 0;
#endif
}

} // namespace halo2::native_sphere_center_bounds_detail
