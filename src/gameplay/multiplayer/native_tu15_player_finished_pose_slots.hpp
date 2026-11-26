#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_cold_frame.hpp"
#include "gameplay/multiplayer/native_tu15_player_finished_flag_slots.hpp"

namespace halo2::native_player_finished_cold_detail {

inline constexpr std::uint32_t distance_test = 0x1f080du;
inline constexpr std::uint32_t angle_success = 0x1f07beu;

template<class State>
class PoseFrame : public ColdFrame<State> {
    using Base = ColdFrame<State>;
    using Base::s;
    using Base::gp;
    using Base::flags;
    using Base::read;
    using Base::load;
    using Base::store;
    using Base::restore;
    using Base::invoke;
    using Base::leave;
    using Base::recheck;
    using Base::scalar;
    using Base::load_scalar;

    static __m128 subtract(__m128 lhs, __m128 rhs) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("subss %1,%0" : "+x"(lhs) : "x"(rhs));
        return lhs;
#else
        return _mm_sub_ss(lhs, rhs);
#endif
    }

    static __m128 add(__m128 lhs, __m128 rhs) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("addss %1,%0" : "+x"(lhs) : "x"(rhs));
        return lhs;
#else
        return _mm_add_ss(lhs, rhs);
#endif
    }

    static __m128 multiply(__m128 lhs, __m128 rhs) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("mulss %1,%0" : "+x"(lhs) : "x"(rhs));
        return lhs;
#else
        return _mm_mul_ss(lhs, rhs);
#endif
    }

    std::uint32_t scalar_bits(unsigned lane) const {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(s.xmm[lane])));
    }

    void subtract_memory(std::uint32_t pc, std::uint32_t next, unsigned lane, std::uint32_t address) {
        s.xmm[lane] = subtract(s.xmm[lane], scalar(pc, address));
        recheck(next);
    }

    void compare_memory(std::uint32_t pc, std::uint32_t next, unsigned lane, std::uint32_t address) {
        flags.scalar_compare(s.xmm[lane], scalar(pc, address), false);
        recheck(next);
    }

    bool exceeds_angle_limit(std::uint32_t pc, std::uint32_t stack_offset) {
        const auto bits = read(pc, gp[esp] + stack_offset);
        s.fp.push(memory::ext(std::bit_cast<float>(bits)));
        s.fp.status &= ~0x200u;
        recheck(pc + 4u);
        s.fp.st(0) = std::fabs(s.fp.st(0));
        s.fp.status &= ~0x200u;
        s.fp.push(memory::ext(std::bit_cast<float>(read(pc + 6u, 0x46a544u))));
        s.fp.status &= ~0x200u;
        recheck(pc + 12u);
        std::swap(s.fp.st(0), s.fp.st(1));
        s.fp.status &= ~0x200u;
        s.compare_and_pop(s.fp.st(1));
        s.fp.status &= ~0x200u;
        s.fp.pop();
        return !flags.cf && !flags.zf;
    }

    bool accepts_slot(std::uint32_t pc, std::uint32_t slot) {
        flags.minus(read(pc, gp[ebp] + 0x80u), slot, 4);
        recheck(pc + 7u);
        return flags.zf;
    }

    void initialize_angles() {
        load(0x1f0709u, 0x1f070fu, eax, gp[ebp] + 0x90u);
        load(0x1f070fu, 0x1f0715u, edx, gp[ebp] + 0x8cu);
        restore(0x1f0715u, 0x1f0716u, ebx);
        restore(0x1f0716u, 0x1f0717u, edi);
        store(0x1f0717u, 0x1f071du, gp[ebp] + 0x1a8u, gp[eax]);
        load(0x1f071du, 0x1f0721u, eax, gp[esp] + 0xbu, 1);
        restore(0x1f0721u, 0x1f0722u, esi);
        store(0x1f0722u, 0x1f0728u, gp[ebp] + 0x1a4u, gp[edx]);
        store(0x1f0728u, 0x1f072fu, gp[ebp] + 0x1a0u, 1, 1);
        restore(0x1f072fu, 0x1f0730u, ebp);
        leave(0x1f0733u);
    }

    void wrapped_differences() {
        load_scalar(0x1f0734u, 0x1f073cu, 0, gp[ebp] + 0x8cu);
        subtract_memory(0x1f073cu, 0x1f0744u, 0, gp[ebp] + 0x1a4u);
        compare_memory(0x1f0744u, 0x1f074bu, 0, 0x46a2ecu);
        load_scalar(0x1f074bu, 0x1f0753u, 1, gp[ebp] + 0x90u);
        subtract_memory(0x1f0753u, 0x1f075bu, 1, gp[ebp] + 0x1a8u);
        store(0x1f075bu, 0x1f0761u, gp[esp] + 0x18u, scalar_bits(1));
        load_scalar(0x1f0761u, 0x1f0769u, 1, 0x44923cu);
        store(0x1f0769u, 0x1f076fu, gp[esp] + 0x14u, scalar_bits(0));
        if (!flags.cf && !flags.zf) {
            s.xmm[0] = subtract(s.xmm[0], s.xmm[1]);
            store(0x1f0775u, 0x1f077bu, gp[esp] + 0x14u, scalar_bits(0));
        }
        load_scalar(0x1f077bu, 0x1f0783u, 2, 0x46a488u);
        flags.scalar_compare(s.xmm[2], s.xmm[0], false);
        if (!flags.cf && !flags.zf) {
            s.xmm[0] = add(s.xmm[0], s.xmm[1]);
            store(0x1f078cu, 0x1f0792u, gp[esp] + 0x14u, scalar_bits(0));
        }
    }

    void initialize_heading() {
        load(0x1f09e6u, 0x1f09ecu, edx, gp[ebp] + 0x8cu);
        load(0x1f09ecu, 0x1f09f0u, eax, gp[esp] + 0x13u, 1);
        restore(0x1f09f0u, 0x1f09f1u, ebx);
        restore(0x1f09f1u, 0x1f09f2u, edi);
        restore(0x1f09f2u, 0x1f09f3u, esi);
        store(0x1f09f3u, 0x1f09fau, gp[ebp] + 0x1acu, 0, 1);
        store(0x1f09fau, 0x1f0a01u, gp[ebp] + 0x1adu, 0, 1);
        store(0x1f0a01u, 0x1f0a07u, gp[ebp] + 0x1a4u, gp[edx]);
        store(0x1f0a07u, 0x1f0a0eu, gp[ebp] + 0x1a0u, 1, 1);
        restore(0x1f0a0eu, 0x1f0a0fu, ebp);
        leave(0x1f0a12u);
    }

    std::uint32_t turn_in_both_directions() {
        load_scalar(0x1f0a13u, 0x1f0a1bu, 0, gp[ebp] + 0x8cu);
        subtract_memory(0x1f0a1bu, 0x1f0a23u, 0, gp[ebp] + 0x1a4u);
        compare_memory(0x1f0a23u, 0x1f0a2au, 0, 0x46a2ecu);
        load_scalar(0x1f0a2au, 0x1f0a32u, 1, 0x44923cu);
        if (!flags.cf && !flags.zf)
            s.xmm[0] = subtract(s.xmm[0], s.xmm[1]);
        load_scalar(0x1f0a38u, 0x1f0a40u, 2, 0x46a488u);
        flags.scalar_compare(s.xmm[2], s.xmm[0], false);
        if (!flags.cf && !flags.zf)
            s.xmm[0] = add(s.xmm[0], s.xmm[1]);
        load_scalar(0x1f0a49u, 0x1f0a51u, 1, 0x46a8ecu);
        flags.scalar_compare(s.xmm[1], s.xmm[0], false);
        gp[ecx] = 1;
        if (!flags.cf && !flags.zf)
            store(0x1f0a5bu, 0x1f0a61u, gp[ebp] + 0x1acu, s.reg(ecx, 1), 1);
        compare_memory(0x1f0a61u, 0x1f0a68u, 0, 0x46a8e8u);
        if (!flags.cf && !flags.zf)
            store(0x1f0a6au, 0x1f0a70u, gp[ebp] + 0x1adu, s.reg(ecx, 1), 1);
        load(0x1f0a70u, 0x1f0a76u, eax, gp[ebp] + 0x1adu, 1);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf)
            return restore_all;
        load(0x1f0a7eu, 0x1f0a84u, eax, gp[ebp] + 0x1acu, 1);
        flags.logic(s.reg(eax, 1), 1);
        return team_result;
    }

public:
    using Base::Base;

    std::uint32_t test_wrapped_angles() {
        if (s.active_mmx || s.fp.count > 6)
            return 0;
        if (!accepts_slot(0x1f06f2u, 10))
            return restore_all;
        load(0x1f06ffu, 0x1f0705u, eax, gp[ebp] + 0x1a0u, 1);
        flags.logic(s.reg(eax, 1), 1);
        if (flags.zf) {
            initialize_angles();
            return done;
        }
        wrapped_differences();
        return exceeds_angle_limit(0x1f0792u, 0x14u) || exceeds_angle_limit(0x1f07a6u, 0x18u)
            ? angle_success : restore_all;
    }

    std::uint32_t test_heading() {
        if (!accepts_slot(0x1f09cfu, 2))
            return restore_all;
        load(0x1f09dcu, 0x1f09e2u, eax, gp[ebp] + 0x1a0u, 1);
        flags.logic(s.reg(eax, 1), 1);
        if (!flags.zf)
            return turn_in_both_directions();
        initialize_heading();
        return done;
    }

    template<class Initialize, class Compare>
    std::uint32_t test_effect_position(Initialize initialize, Compare compare) {
        if (!accepts_slot(0x1f07cfu, 11))
            return restore_all;
        load(0x1f07dcu, 0x1f07e2u, ecx, gp[ebp] + 0x1a0u, 1);
        flags.logic(s.reg(ecx, 1), 1);
        if (!flags.zf) {
            gp[esi] = gp[esp] + 0x1cu;
            invoke(0x1f0808u, distance_test, compare);
            return distance_test;
        }
        gp[esi] = gp[ebp] + 0x1a4u;
        invoke(0x1f07ecu, 0x1f07f1u, initialize);
        load(0x1f07f1u, 0x1f07f5u, eax, gp[esp] + 0x13u, 1);
        restore(0x1f07f5u, 0x1f07f6u, ebx);
        restore(0x1f07f6u, 0x1f07f7u, edi);
        restore(0x1f07f7u, 0x1f07f8u, esi);
        store(0x1f07f8u, 0x1f07ffu, gp[ebp] + 0x1a0u, 1, 1);
        restore(0x1f07ffu, 0x1f0800u, ebp);
        leave(0x1f0803u);
        return done;
    }

    std::uint32_t test_unit_position() {
        if (!accepts_slot(0x1f0953u, 13))
            return restore_all;
        load(0x1f0960u, 0x1f0966u, eax, gp[ebp] + 0x1a0u, 1);
        gp[ebx] = flags.plus(gp[ebx], 0x64u, 4);
        flags.logic(s.reg(eax, 1), 1);
        if (!flags.zf) {
            load(0x1f0996u, 0x1f0998u, edx, gp[ebx]);
            load(0x1f0998u, 0x1f099bu, eax, gp[ebx] + 4u);
            load(0x1f099bu, 0x1f099eu, ecx, gp[ebx] + 8u);
            store(0x1f099eu, 0x1f09a2u, gp[esp] + 0x1cu, gp[edx]);
            store(0x1f09a2u, 0x1f09a6u, gp[esp] + 0x20u, gp[eax]);
            store(0x1f09a6u, 0x1f09aau, gp[esp] + 0x24u, gp[ecx]);
            return distance_test;
        }
        load(0x1f096du, 0x1f096fu, ecx, gp[ebx]);
        gp[eax] = gp[ebp] + 0x1a4u;
        store(0x1f0975u, 0x1f0977u, gp[eax], gp[ecx]);
        load(0x1f0977u, 0x1f097au, edx, gp[ebx] + 4u);
        store(0x1f097au, 0x1f097du, gp[eax] + 4u, gp[edx]);
        load(0x1f097du, 0x1f0980u, ecx, gp[ebx] + 8u);
        restore(0x1f0980u, 0x1f0981u, ebx);
        restore(0x1f0981u, 0x1f0982u, edi);
        restore(0x1f0982u, 0x1f0983u, esi);
        store(0x1f0983u, 0x1f0986u, gp[eax] + 8u, gp[ecx]);
        load(0x1f0986u, 0x1f098au, eax, gp[esp] + 7u, 1);
        store(0x1f098au, 0x1f0991u, gp[ebp] + 0x1a0u, 1, 1);
        restore(0x1f0991u, 0x1f0992u, ebp);
        leave(0x1f0995u);
        return done;
    }

    std::uint32_t test_squared_distance() {
        load_scalar(0x1f080du, 0x1f0815u, 2, gp[ebp] + 0x1acu);
        subtract_memory(0x1f0815u, 0x1f081bu, 2, gp[esp] + 0x24u);
        load_scalar(0x1f081bu, 0x1f0823u, 1, gp[ebp] + 0x1a8u);
        subtract_memory(0x1f0823u, 0x1f0829u, 1, gp[esp] + 0x20u);
        load_scalar(0x1f0829u, 0x1f0831u, 0, gp[ebp] + 0x1a4u);
        subtract_memory(0x1f0831u, 0x1f0837u, 0, gp[esp] + 0x1cu);
        s.xmm[3] = multiply(s.xmm[2], s.xmm[2]);
        s.xmm[2] = multiply(s.xmm[1], s.xmm[1]);
        s.xmm[1] = s.xmm[0];
        s.xmm[3] = add(s.xmm[3], s.xmm[2]);
        s.xmm[1] = multiply(s.xmm[1], s.xmm[0]);
        s.xmm[3] = add(s.xmm[3], s.xmm[1]);
        compare_memory(0x1f0854u, shared_position_gate, 3, 0x46a130u);
        return shared_position_gate;
    }
};
} // namespace halo2::native_player_finished_cold_detail
