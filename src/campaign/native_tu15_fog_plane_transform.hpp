#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include <cmath>

namespace halo2::native_fog_plane_transform_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::wide_event;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x133c2fu;
inline constexpr std::uint32_t last = 0x134040u;
enum class Entry : std::uint32_t { angular = first, explicit_normal = 0x133cabu, advance = 0x134020u };
enum class Next : std::uint32_t { fallback, normal = 0x133d1du, another_plane = 0x133bd4u, output = 0x134045u };

// The local FCOS, FSIN and FSTP helpers are the same project-native hardware
// operations used by the retained owner. Only the typed algorithm and guest
// access boundary move. The live physical exceptions and source stack/TOP
// remain owned by the existing x87 helpers.
template<class State, auto Sine, auto Cosine, auto Spill> struct Transform : Port<State> {
    using Base = Port<State>;
    Entry entry;
    Next &next;
    Transform(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &result)
        : Base(s, cursor, trace, {}), entry(start), next(result) {}
    auto &x() { return this->s.xmm; }
    auto &fp() { return this->s.fp; }
    std::uint32_t local(std::uint32_t offset) const { return this->gp[esp] + offset; }
    std::uint32_t plane(std::uint32_t offset) const { return this->gp[ecx] + offset; }
    __m128 load_scalar(std::uint32_t pc, std::uint32_t address) {
        const auto bits = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(bits));
    }
    long double load_single(std::uint32_t pc, std::uint32_t address) {
        return physics_step_detail::ext(physics_step_detail::f(this->read(pc, address, 4)));
    }
    void push_single(std::uint32_t pc, std::uint32_t address) {
        fp().push(load_single(pc, address));
    }
    void push_double(std::uint32_t pc, std::uint32_t address) {
        this->at(pc);
        std::array<std::byte,8> bytes{};
        this->s.memory.read(address, bytes);
        if (this->trace)
            this->trace->events.push_back(wide_event(0, pc, address, 8, bytes.data()));
        // The double is one guest access, converted to the extended lane
        // under the current x87 control word before the logical push.
        fp().push(static_cast<long double>(std::bit_cast<double>(bytes)));
    }
    void spill(std::uint32_t pc, std::uint32_t slot, bool pop) {
        this->at(pc);
        const auto bits = Spill(fp().st(0), fp().status);
        this->store(pc, local(slot), 4, bits);
        if (pop)
            fp().pop();
    }
    void copy(std::uint32_t pc, unsigned source) {
        this->at(pc);
        fp().push(fp().st(source));
    }
    void discard(std::uint32_t pc) {
        this->at(pc);
        fp().status &= ~0x200u;
        fp().pop();
    }
    void product(std::uint32_t pc) {
        this->at(pc);
        fp().st(0) = physics_step_detail::mul(fp().st(0), fp().st(1));
    }
    void trig(std::uint32_t pc, bool sine) {
        this->at(pc);
        std::uint16_t status{};
        fp().st(0) = sine ? Sine(fp().st(0), status) : Cosine(fp().st(0), status);
        fp().status = std::uint16_t((fp().status & ~0x600u) | (status & 0x600u));
        if (status & 0x400u)
            this->reached(6);
    }
    void negate(std::uint32_t pc) {
        this->at(pc);
        fp().st(0) = -fp().st(0);
        fp().status &= ~0x200u;
    }
    void finish(Next destination, std::uint32_t pc) {
        next = destination;
        this->publish(pc);
    }
    void angular() {
        auto &gp = this->gp;
        this->reached(0);
        gp[eax] = this->read(0x133c2fu, plane(0xcu), 4);
        this->store(0x133c32u, local(0x18u), 4, gp[eax]);
        push_single(0x133c36u, local(0x18u));
        auto pi_offset = load_single(0x133c3au, 0x46a2ecu);
        fp().st(0) = physics_step_detail::add(fp().st(0), pi_offset);
        gp[eax] = this->read(0x133c40u, plane(0x10u), 4);
        push_single(0x133c43u, plane(0x10u));
        this->store(0x133c46u, local(0x1cu), 4, gp[eax]);
        trig(0x133c4au, false);
        push_single(0x133c4cu, plane(0xcu));
        trig(0x133c4fu, false);
        product(0x133c51u);
        spill(0x133c53u, 0x2cu, true);
        x()[1] = load_scalar(0x133c57u, local(0x2cu));
        push_single(0x133c5du, plane(0xcu));
        trig(0x133c60u, true);
        product(0x133c62u);
        spill(0x133c64u, 0x30u, true);
        x()[4] = load_scalar(0x133c68u, local(0x30u));
        discard(0x133c6eu);
        push_single(0x133c70u, plane(0x10u));
        trig(0x133c73u, true);
        spill(0x133c75u, 0x34u, true);
        push_double(0x133c79u, 0x46a0f0u);
        x()[3] = load_scalar(0x133c7fu, local(0x34u));
        trig(0x133c85u, false);
        copy(0x133c87u, 1);
        trig(0x133c89u, false);
        product(0x133c8bu);
        spill(0x133c8du, 0x20u, true);
        this->at(0x133c91u);
        std::swap(fp().st(0), fp().st(1));
        fp().status &= ~0x200u;
        trig(0x133c93u, true);
        product(0x133c95u);
        spill(0x133c97u, 0x24u, true);
        discard(0x133c9bu);
        push_double(0x133c9du, 0x46a0f0u);
        trig(0x133ca3u, true);
        spill(0x133ca5u, 0x28u, true);
        finish(Next::normal, 0x133ca9u);
    }
    void explicit_normal() {
        this->reached(1);
        push_single(0x133cabu, local(0x2cu));
        this->store(0x133cafu, local(0x28u), 4,
            std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[2]))));
        negate(0x133cb5u);
        spill(0x133cb7u, 0x20u, true);
        push_single(0x133cbbu, local(0x30u));
        negate(0x133cbfu);
        spill(0x133cc1u, 0x24u, true);
        push_single(0x133cc5u, local(0x24u));
        const auto y = load_single(0x133cc9u, local(0x24u));
        fp().st(0) = physics_step_detail::mul(fp().st(0), y);
        push_single(0x133ccdu, local(0x20u));
        const auto axis = load_single(0x133cd1u, local(0x20u));
        fp().st(0) = physics_step_detail::mul(fp().st(0), axis);
        this->at(0x133cd5u);
        fp().st(1) = physics_step_detail::add(fp().st(1), fp().st(0));
        fp().pop();
        this->at(0x133cd7u);
        fp().st(0) = physics_solver_detail::root(fp().st(0));
        spill(0x133cd9u, 8, false);
        this->at(0x133cddu);
        fp().st(0) = std::fabs(fp().st(0));
        fp().status &= ~0x200u;
        push_single(0x133cdfu, 0x46a0ecu);
        this->at(0x133ce5u);
        this->s.flags = this->flags;
        this->s.compare_and_pop(fp().st(1));
        this->flags = this->s.flags;
        discard(0x133ce7u);
        if (!this->flags.cf && !this->flags.zf) {
            this->reached(2);
            finish(Next::normal, 0x133ce9u);
            return;
        }
        this->reached(3);
        x()[0] = x()[6];
        x()[0] = _mm_div_ss(x()[0], load_scalar(0x133ceeu, local(8)));
        x()[5] = x()[0];
        x()[5] = native_tu15_leaves::ordered_mul_ss(x()[5], load_scalar(0x133cf7u, local(0x20u)));
        this->store(0x133cfdu, local(0x20u), 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[5]))));
        x()[5] = load_scalar(0x133d03u, local(0x24u));
        x()[5] = native_tu15_leaves::ordered_mul_ss(x()[5], x()[0]);
        x()[0] = native_tu15_leaves::ordered_mul_ss(x()[0], x()[2]);
        this->store(0x133d11u, local(0x24u), 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[5]))));
        this->store(0x133d17u, local(0x28u), 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[0]))));
        finish(Next::normal, 0x133d17u);
    }
    void advance() {
        auto &gp = this->gp;
        gp[eax] = this->read(0x134020u, local(0x14u), 4);
        gp[esi] = this->read(0x134024u, local(0x40u), 4);
        gp[ecx] = this->read(0x134028u, gp[edx] + 0x78u, 4);
        gp[eax] = this->flags.increment(gp[eax], 4, false);
        gp[esi] = this->flags.plus(gp[esi], 0x34u, 4);
        this->compare32(gp[eax], gp[ecx]);
        this->store(0x134031u, local(0x14u), 4, gp[eax]);
        this->store(0x134035u, local(0x40u), 4, gp[esi]);
        if (this->flags.sf != this->flags.of) {
            this->reached(4);
            finish(Next::another_plane, 0x134039u);
        } else {
            gp[esi] = this->pop(0x13403fu);
            this->reached(5);
            finish(Next::output, last);
        }
    }
    void run() {
        try {
            switch (entry) {
            case Entry::angular: angular(); break;
            case Entry::explicit_normal: explicit_normal(); break;
            case Entry::advance: advance(); break;
            }
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog plane transform"};
    return value;
}
inline bool masked_fp() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t control{};
    asm volatile("fnstcw %0" : "=m"(control));
    return (control & 0x3fu) == 0x3fu && (_mm_getcsr() & 0x1f80u) == 0x1f80u;
#else
    return false;
#endif
}

template<auto Sine, auto Cosine, auto Spill, class State, class Reference>
Next bridge(State &s, Entry start, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PLANE_TRANSFORM");
    static const bool verify = flag("HALO2_VERIFY_FOG_PLANE_TRANSFORM");
    if (!masked_fp())
        return Next::fallback;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return Next::fallback;
    }
    Next next{};
    native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        [&](State &v) {
            if (reference(v, start) != next)
                throw std::runtime_error("Campaign fog plane transform continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Transform<State,Sine,Cosine,Spill>{v, cursor, trace, start, next};
        });
    return next;
}

} // namespace halo2::native_fog_plane_transform_detail
