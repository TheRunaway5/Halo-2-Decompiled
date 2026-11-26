#pragma once

#include "gameplay/multiplayer/native_tu15_player_finished_1f0670.hpp"

namespace halo2::native_player_finished_angle_detail {
using namespace native_player_finished_detail;

inline constexpr std::uint32_t early_finish = 0x1f07beu;
inline constexpr std::uint32_t angle_limit = 0x46a544u;

template<class State>
class AngleFrame {
    State &s;
    const Cursor *cursor;
    decltype(State::gp) &gp = s.gp;
    decltype(State::flags) &flags = s.flags;

    void at(std::uint32_t pc) const {
        if (cursor)
            *cursor->position = pc;
    }

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(s.services.instruction))
            throw Resume{next};
    }

    std::uint32_t word(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return memory::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address, unsigned width = 4) {
        s.set(dest, word(pc, address, width), width);
        recheck(next);
    }

    void put(std::uint32_t pc, std::uint32_t next, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        memory::w(s.memory, s.observe, pc, address, width, value);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
        recheck(next);
    }

    void leave(std::uint32_t pc) {
        gp[esp] = flags.plus(gp[esp], 0x54u, 4);
        at(pc);
        (void)s.pop();
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word(pc,address))));
    }

    void scalar_load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address) {
        s.xmm[dest] = scalar(pc,address);
        recheck(next);
    }

    static __m128 subtract(__m128 lhs, __m128 rhs) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("subss %1,%0" : "+x"(lhs) : "x"(rhs));
        return lhs;
#else
        return _mm_sub_ss(lhs,rhs);
#endif
    }

    static __m128 add(__m128 lhs, __m128 rhs) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        asm volatile("addss %1,%0" : "+x"(lhs) : "x"(rhs));
        return lhs;
#else
        return _mm_add_ss(lhs,rhs);
#endif
    }

    std::uint32_t xmm_bits(unsigned lane) const {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(s.xmm[lane])));
    }

    void initialize_angles() {
        load(0x1f0889u, 0x1f088fu, ecx, gp[ebp] + 0x8cu);
        load(0x1f088fu, 0x1f0895u, edx, gp[ebp] + 0x90u);
        load(0x1f0895u, 0x1f0899u, eax, gp[esp] + 0x13u, 1);
        restore(0x1f0899u, 0x1f089au, ebx);
        restore(0x1f089au, 0x1f089bu, edi);
        restore(0x1f089bu, 0x1f089cu, esi);
        put(0x1f089cu, 0x1f08a2u, gp[ebp] + 0x1a4u, gp[ecx]);
        put(0x1f08a2u, 0x1f08a8u, gp[ebp] + 0x1a8u, gp[edx]);
        put(0x1f08a8u, 0x1f08afu, gp[ebp] + 0x1a0u, 1, 1);
        restore(0x1f08afu, 0x1f08b0u, ebp);
        leave(0x1f08b3u);
    }

    void difference() {
        scalar_load(0x1f08b4u, 0x1f08bcu, 0, gp[ebp] + 0x8cu);
        s.xmm[0] = subtract(s.xmm[0], scalar(0x1f08bcu,gp[ebp] + 0x1a4u));
        recheck(0x1f08c4u);
        flags.scalar_compare(s.xmm[0],scalar(0x1f08c4u,0x46a2ecu),false);
        recheck(0x1f08cbu);
        scalar_load(0x1f08cbu, 0x1f08d3u, 1, gp[ebp] + 0x90u);
        s.xmm[1] = subtract(s.xmm[1], scalar(0x1f08d3u,gp[ebp] + 0x1a8u));
        recheck(0x1f08dbu);
        put(0x1f08dbu, 0x1f08e1u, gp[esp] + 0x18u, xmm_bits(1));
        scalar_load(0x1f08e1u, 0x1f08e9u, 1, 0x44923cu);
        put(0x1f08e9u, 0x1f08efu, gp[esp] + 0x14u, xmm_bits(0));
        if (!flags.cf && !flags.zf) {
            s.xmm[0] = subtract(s.xmm[0],s.xmm[1]);
            put(0x1f08f5u, 0x1f08fbu, gp[esp] + 0x14u, xmm_bits(0));
        }
        scalar_load(0x1f08fbu, 0x1f0903u, 2, 0x46a488u);
        flags.scalar_compare(s.xmm[2],s.xmm[0],false);
        if (!flags.cf && !flags.zf) {
            s.xmm[0] = add(s.xmm[0],s.xmm[1]);
            put(0x1f090cu, 0x1f0912u, gp[esp] + 0x14u, xmm_bits(0));
        }
    }

    bool exceeds_limit(std::uint32_t load_delta, std::uint32_t absolute, unsigned offset,
                       std::uint32_t load_limit, std::uint32_t exchange) {
        s.fp.push(memory::ext(std::bit_cast<float>(word(load_delta,gp[esp] + offset))));
        s.fp.status &= ~0x200u;
        recheck(absolute);
        s.fp.st(0) = std::fabs(s.fp.st(0));
        s.fp.status &= ~0x200u;
        s.fp.push(memory::ext(std::bit_cast<float>(word(load_limit,angle_limit))));
        s.fp.status &= ~0x200u;
        recheck(exchange);
        std::swap(s.fp.st(0),s.fp.st(1));
        s.fp.status &= ~0x200u;
        s.compare_and_pop(s.fp.st(1));
        s.fp.status &= ~0x200u;
        s.fp.pop();
        return !flags.cf && !flags.zf;
    }

public:
    AngleFrame(State &state, const Cursor *cursor) : s(state), cursor(cursor) {}

    template<bool Early>
    void success() {
        if constexpr (Early) {
            restore(0x1f07beu, 0x1f07bfu, ebx);
            restore(0x1f07bfu, 0x1f07c0u, edi);
            put(0x1f07c0u, 0x1f07c5u, gp[esp] + 0xbu, 1, 1);
            load(0x1f07c5u, 0x1f07c9u, eax, gp[esp] + 0xbu, 1);
            restore(0x1f07c9u, 0x1f07cau, esi);
            restore(0x1f07cau, 0x1f07cbu, ebp);
            leave(0x1f07ceu);
        } else {
            restore(0x1f0942u, 0x1f0943u, ebx);
            restore(0x1f0943u, 0x1f0944u, edi);
            put(0x1f0944u, 0x1f0949u, gp[esp] + 0xbu, 1, 1);
            load(0x1f0949u, 0x1f094du, eax, gp[esp] + 0xbu, 1);
            restore(0x1f094du, 0x1f094eu, esi);
            restore(0x1f094eu, 0x1f094fu, ebp);
            leave(0x1f0952u);
        }
    }

    std::uint32_t test() {
        flags.minus(word(0x1f0872u,gp[ebp] + 0x80u),12u,4);
        recheck(0x1f0879u);
        if (!flags.zf)
            return restore_all;
        load(0x1f087fu, 0x1f0885u, eax, gp[ebp] + 0x1a0u, 1);
        flags.logic(s.reg(eax,1),1);
        if (flags.zf) {
            initialize_angles();
            return done;
        }
        difference();
        if (exceeds_limit(0x1f0912u,0x1f0916u,0x14u,0x1f0918u,0x1f091eu))
            return early_finish;
        if (!exceeds_limit(0x1f092au,0x1f092eu,0x18u,0x1f0930u,0x1f0936u))
            return restore_all;
        success<false>();
        return done;
    }
};

template<bool Early, class State>
std::uint32_t bridge(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PLAYER_FINISHED_ANGLE_SLOT");
        return value && *value;
    }();
    const Cursor *cursor;
    if (reference || !admit(s,cursor))
        return 0;
    if constexpr (!Early)
        if (s.active_mmx || s.fp.count > 6)
            return 0;
    try {
        AngleFrame<State> frame(s,cursor);
        if constexpr (Early) {
            frame.template success<true>();
            return done;
        } else {
            return frame.test();
        }
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_player_finished_angle_detail
