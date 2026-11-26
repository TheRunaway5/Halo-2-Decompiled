#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_two_bone_solve.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_bitmap_residency_detail {
using namespace native_object_tick_detail::boundary;
using physics_step_detail::f;
using physics_step_detail::r;
using physics_step_detail::si;
using physics_step_detail::w;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline constexpr std::uint32_t resource_pool = 0x4f2d24u, cache_manager = 0x4f2d34u;
inline constexpr std::uint32_t retry_report = 0x4f2d4au;

template<class State, class Route> class Request {
    State &s;
    Route &route;
    decltype(State::gp) &gp;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t get(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(f(get(pc, address)));
    }
    void put(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        w(s.memory, s.observe, pc, address, width, value);
    }
    std::uint32_t stack(unsigned offset) const { return gp[esp] + offset; }
    std::uint32_t row(unsigned offset) const { return gp[ebp] + offset; }
    std::uint32_t low(unsigned reg) const { return gp[reg] & 0xffu; }
    void set_low(unsigned reg, std::uint32_t value) { s.set(reg, value, 1, 0); }

    void test(std::uint32_t pc, std::uint32_t value, unsigned width) {
        at(pc);
        flags.logic(value, width);
    }
    bool zero(std::uint32_t pc) {
        at(pc);
        return flags.zf;
    }
    bool nonpositive(std::uint32_t pc) {
        at(pc);
        return flags.zf || flags.sf != flags.of;
    }
    void push(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, value);
    }
    void pop(std::uint32_t pc, unsigned reg) {
        at(pc);
        gp[reg] = s.pop();
    }
    void call(std::uint32_t pc, std::uint32_t next, std::uint32_t target) {
        push(pc, next);
        route(s, target);
    }
    void release_frame(std::uint32_t add, std::uint32_t ret) {
        at(add);
        gp[esp] = flags.plus(gp[esp], 0x10u, 4);
        at(ret);
        (void)s.pop();
        gp[esp] += 8u;
    }
    void restore(std::uint32_t first) {
        if (!s.template try_pop_registers<esi, ebp, ebx>(first + 2u)) {
            pop(first, esi);
            pop(first + 1u, ebp);
            pop(first + 2u, ebx);
        }
        release_frame(first + 3u, first + 6u);
    }

    void initialize() {
        auto &x = s.xmm;
        at(0x12f910u);
        gp[esp] = flags.minus(gp[esp], 0x10u, 4);
        gp[edx] = get(0x12f913u, stack(0x18u));
        at(0x12f917u);
        set_low(eax, low(edx));
        at(0x12f919u);
        set_low(eax, flags.logic(low(eax) & 1u, 1));
        put(0x12f91bu, stack(4u), low(eax), 1);
        push(0x12f91fu, gp[ebx]);
        at(0x12f920u);
        gp[eax] = gp[edx];
        at(0x12f922u);
        gp[eax] = flags.right(gp[eax], 1u, 4, false);
        at(0x12f924u);
        set_low(eax, flags.logic(low(eax) & 1u, 1));
        push(0x12f926u, gp[ebp]);
        gp[ebp] = get(0x12f927u, stack(0x1cu));
        at(0x12f92bu);
        gp[ebx] = gp[edx];
        at(0x12f92du);
        gp[ebx] = flags.right(gp[ebx], 2u, 4, false);
        put(0x12f930u, stack(0xau), low(eax), 1);
        set_low(eax, get(0x12f934u, 0x474da1u, 1));
        push(0x12f939u, gp[esi]);
        at(0x12f93au);
        set_low(ebx, flags.logic(low(ebx) & 1u, 1));
        at(0x12f93du);
        gp[esi] = flags.logic(0u, 4);
        test(0x12f93fu, low(eax), 1);
        put(0x12f941u, stack(0xfu), low(ebx), 1);
        put(0x12f945u, stack(0x18u), gp[esi]);
        at(0x12f949u);
        x[0] = x[1];
        if (!zero(0x12f94cu)) {
            gp[eax] = get(0x12f94eu, row(0x16u), 1);
            at(0x12f952u);
            x[0] = _mm_cvtsi32_ss(x[0], si(gp[eax]));
            x[0] = ordered_mul_ss(x[0], scalar(0x12f956u, 0x46a18cu));
            at(0x12f95eu);
            x[0] = ordered_add_ss(x[0], x[1]);
            x[0] = ordered_add_ss(x[0], scalar(0x12f962u, 0x4f2d4cu));
        }
        set_low(eax, get(0x12f96au, 0x474d9du, 1));
        test(0x12f96fu, low(eax), 1);
        put(0x12f971u, stack(0x14u), 0, 1);
        put(0x12f976u, stack(0xdu), 0, 1);
        if (!zero(0x12f97bu)) {
            at(0x12f97du);
            x[0] = _mm_setzero_ps();
        }
        set_low(eax, get(0x12f980u, 0x474d9cu, 1));
        test(0x12f985u, low(eax), 1);
        if (!zero(0x12f987u)) x[0] = scalar(0x12f989u, 0x46a0d0u);
    }

    void acquire_row_slot() {
        set_low(eax, get(0x12f9a8u, 0x4f2d38u, 1));
        at(0x12f9adu);
        gp[edi] = flags.logic(0u, 4);
        test(0x12f9afu, low(eax), 1);
        if (zero(0x12f9b1u)) return;
        gp[ecx] = get(0x12f9b3u, row(0x5cu));
        test(0x12f9b6u, gp[ecx], 4);
        at(0x12f9b8u);
        gp[eax] = row(0x5cu);
        if (zero(0x12f9bbu)) {
            gp[esi] = get(0x12f9bdu, row(0x64u));
            test(0x12f9c0u, gp[esi], 4);
            if (nonpositive(0x12f9c2u)) return;
        }
        test(0x12f9c4u, gp[ecx], 4);
        at(0x12f9c6u);
        gp[edi] = gp[eax];
        if (!nonpositive(0x12f9c8u)) return;
        at(0x12f9cau);
        gp[esi] = gp[ebp];
        call(0x12f9ccu, 0x12f9d1u, 0x12f330u);
        gp[edx] = get(0x12f9d1u, stack(0x28u));
    }

    bool prefer_request() {
        gp[eax] = get(0x12f9e5u, 0x4f2d44u);
        test(0x12f9eau, gp[eax], 4);
        if (zero(0x12f9ecu)) return true;
        at(0x12f9eeu);
        flags.minus(gp[eax], 1u, 4);
        if (!zero(0x12f9f1u)) return false;
        flags.minus(get(0x12f9f3u, row(0x17u), 1), 3u, 1);
        return zero(0x12f9f7u);
    }

    void initialize_paged_request() {
        acquire_row_slot();
        gp[ecx] = get(0x12f9d5u, 0x4f2d40u);
        test(0x12f9dbu, gp[ecx], 4);
        at(0x12f9ddu);
        gp[esi] = gp[edi];
        put(0x12f9dfu, stack(0x1cu), gp[esi]);
        if (!nonpositive(0x12f9e3u) && prefer_request()) {
            at(0x12f9f9u);
            flags.minus(gp[ecx], 1u, 4);
            put(0x12f9fcu, stack(0x18u), 1, 1);
            put(0x12fa01u, stack(0x14u), flags.zf, 1);
            at(0x12fa06u);
            return;
        }
        test(0x12fa08u, gp[edi], 4);
        if (zero(0x12fa0au)) return;
        set_low(eax, get(0x12fa0cu, row(0xeu), 1));
        test(0x12fa0fu, low(eax), 1);
        at(0x12fa11u);
        if (!flags.sf) put(0x12fa13u, stack(0x14u), 0, 1);
    }

    bool definitely_reuse() {
        set_low(eax, get(0x12fa18u, 0x4f2d4bu, 1));
        test(0x12fa1du, low(eax), 1);
        if (!zero(0x12fa1fu)) {
            gp[ecx] = get(0x12fa21u, row(0x17u), 1);
            call(0x12fa25u, 0x12fa2au, 0x12f7c0u);
            test(0x12fa2au, low(eax), 1);
            if (!zero(0x12fa2cu)) return reuse_if_resident();
        }
        flags.scalar_compare(s.xmm[1], scalar(0x12fa2eu, row(0x58u)), false);
        at(0x12fa32u);
        return !flags.cf && reuse_if_resident();
    }

    bool reuse_if_resident() {
        test(0x12fa34u, gp[esi], 4);
        return !zero(0x12fa36u);
    }

    void choose_request_mode() {
        set_low(ebx, get(0x12fa4bu, stack(0x18u), 1));
        test(0x12fa4fu, low(ebx), 1);
        at(0x12fa51u);
        gp[ecx] = 2;
        if (!zero(0x12fa56u)) {
            at(0x12fa58u);
            return;
        }
        set_low(ebx, get(0x12fa5au, stack(0x14u), 1));
        test(0x12fa5eu, low(ebx), 1);
        if (!zero(0x12fa60u)) {
            at(0x12fa62u);
            gp[ecx] = 7u;
            at(0x12fa67u);
            return;
        }
        test(0x12fa69u, low(edx) & 8u, 1);
        if (!zero(0x12fa6cu)) {
            at(0x12fa6eu);
            gp[ecx] = 5u;
            at(0x12fa73u);
            return;
        }
        test(0x12fa75u, low(edx) & 0x10u, 1);
        if (!zero(0x12fa78u)) {
            at(0x12fa7au);
            gp[ecx] = 1u;
        }
    }

    void request_resource() {
        test(0x12fa38u, low(ebx), 1);
        if (!zero(0x12fa3au)) {
            at(0x12fa3cu);
            gp[eax] = flags.logic(0u, 4);
            at(0x12fa3eu);
        } else {
            at(0x12fa40u);
            gp[edx] = gp[ebp];
            call(0x12fa42u, 0x12fa47u, 0x12f620u);
            gp[edx] = get(0x12fa47u, stack(0x28u));
        }
        choose_request_mode();
        at(0x12fa7fu);
        gp[edx] = stack(0x11u);
        push(0x12fa83u, gp[edx]);
        at(0x12fa84u);
        gp[edx] = stack(0x2cu);
        push(0x12fa88u, gp[edx]);
        gp[edx] = get(0x12fa89u, stack(0x20u));
        push(0x12fa8du, gp[edx]);
        gp[edx] = get(0x12fa8eu, stack(0x20u));
        push(0x12fa92u, gp[edx]);
        at(0x12fa93u);
        gp[ebx] = gp[ecx];
        set_low(ecx, get(0x12fa95u, stack(0x22u), 1));
        push(0x12fa99u, gp[ebp]);
        call(0x12fa9au, 0x12fa9fu, 0x12f4a0u);
        at(0x12fa9fu);
        gp[ebx] = flags.logic(0u, 4);
        at(0x12faa1u);
        flags.minus(gp[eax], gp[ebx], 4);
    }

    bool touch_page() {
        gp[esi] = get(0x12fae7u, row(0x44u));
        at(0x12faeau);
        gp[ecx] = gp[eax];
        at(0x12faecu);
        gp[ecx] = flags.logic(gp[ecx] & 0xffffu, 4);
        at(0x12faf2u);
        gp[ecx] *= 5u;
        at(0x12faf5u);
        gp[ecx] = gp[esi] + 8u * gp[ecx];
        gp[esi] = get(0x12faf8u, gp[edx] + 0x64u);
        gp[esi] = get(0x12fafbu, gp[esi] + 0x44u);
        at(0x12fafeu);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        at(0x12fb03u);
        gp[eax] *= 3u;
        at(0x12fb06u);
        gp[eax] = gp[esi] + 8u * gp[eax];
        gp[esi] = get(0x12fb09u, gp[edx] + 0x38u);
        put(0x12fb0cu, gp[eax] + 0x14u, gp[esi]);
        flags.minus(get(0x12fb0fu, gp[ecx] + 3u, 1), low(ebx), 1);
        if (zero(0x12fb12u)) return false;
        at(0x12fb14u);
        gp[eax] = gp[ecx] + 0x14u;
        at(0x12fb17u);
        flags.minus(gp[eax], gp[ebx], 4);
        if (zero(0x12fb19u)) return false;
        at(0x12fb1bu);
        gp[esi] = gp[eax];
        put(0x12fb1du, stack(0x1cu), gp[esi]);
        put(0x12fb21u, stack(0x14u), low(ebx), 1);
        at(0x12fb25u);
        return true;
    }

    void scan_cached_pages() {
        gp[edx] = get(0x12fac6u, cache_manager);
        at(0x12faccu);
        gp[edi] = row(0x28u);
        gp[ebp] = get(0x12facfu, resource_pool);
        put(0x12fad5u, stack(0x28u), 3u);
        at(0x12faddu);
        // Alignment LEA leaves ECX and flags intact.
        do {
            gp[eax] = get(0x12fae0u, gp[edi]);
            at(0x12fae2u);
            flags.minus(gp[eax], 0xffffffffu, 4);
            if (!zero(0x12fae5u) && !touch_page()) gp[esi] = get(0x12fb27u, stack(0x1cu));
            gp[eax] = get(0x12fb2bu, stack(0x28u));
            at(0x12fb2fu);
            gp[edi] = flags.plus(gp[edi], 4u, 4);
            at(0x12fb32u);
            gp[eax] = flags.increment(gp[eax], 4, true);
            put(0x12fb33u, stack(0x28u), gp[eax]);
        } while (!zero(0x12fb37u));
    }

    void placeholder(std::uint32_t first, std::uint32_t address) {
        gp[eax] = get(first, address);
        pop(first + 5u, edi);
        at(first + 6u);
        gp[esi] = gp[eax];
        if (!s.template try_pop_registers<esi, ebp>(first + 9u)) {
            pop(first + 8u, esi);
            pop(first + 9u, ebp);
        }
        put(first + 0xau, retry_report, 1, 1);
        pop(first + 0x11u, ebx);
        release_frame(first + 0x12u, first + 0x15u);
    }

    bool use_placeholder() {
        flags.minus(get(0x12fac0u, stack(0x13u), 1), low(ebx), 1);
        if (!zero(0x12fac4u)) return false;
        scan_cached_pages();
        flags.minus(get(0x12fb39u, stack(0x14u), 1), low(ebx), 1);
        if (zero(0x12fb3du)) return false;
        at(0x12fb43u);
        flags.minus(gp[esi], gp[ebx], 4);
        if (!zero(0x12fb45u)) return false;
        gp[ecx] = get(0x12fb4bu, stack(0x24u));
        gp[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(get(0x12fb4fu, gp[ecx] + 0xau, 2)))));
        at(0x12fb53u);
        gp[eax] = flags.minus(gp[eax], gp[ebx], 4);
        if (zero(0x12fb55u)) {
            placeholder(0x12fb8au, 0x492304u);
        } else {
            at(0x12fb57u);
            gp[eax] = flags.increment(gp[eax], 4, true);
            if (zero(0x12fb58u)) placeholder(0x12fb72u, 0x492308u);
            else placeholder(0x12fb5au, 0x49230cu);
        }
        return true;
    }

public:
    Request(State &state, Route &children) : s(state), route(children), gp(s.gp), flags(s.flags) {}

    void run() {
        initialize();
        flags.logic(get(0x12f991u, row(0xfu), 1) & 2u, 1);
        if (zero(0x12f995u)) {
            gp[eax] = get(0x12f997u, row(0x50u));
            restore(0x12f99au);
            return;
        }
        test(0x12f9a3u, low(ebx), 1);
        push(0x12f9a5u, gp[edi]);
        if (zero(0x12f9a6u)) initialize_paged_request();
        if (!definitely_reuse()) {
            request_resource();
            if (zero(0x12faa3u)) {
                if (use_placeholder()) return;
            } else {
                at(0x12faa5u);
                gp[esi] = gp[eax];
            }
            flags.minus(get(0x12faa7u, stack(0x11u), 1), low(ebx), 1);
            if (!zero(0x12faabu)) put(0x12faadu, retry_report, 1, 1);
        }
        pop(0x12fab4u, edi);
        at(0x12fab5u);
        gp[eax] = gp[esi];
        restore(0x12fab7u);
    }
};

template<class State, class Route>
bool bridge(State &state, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_BITMAP_RESIDENCY");
    if (reference || state.image != SourcePhysicsContactImage::TitleUpdate15) return false;
    Request<State, Route>{state, route}.run();
    return true;
#else
    (void)state;
    (void)route;
    return false;
#endif
}
} // namespace halo2::native_bitmap_residency_detail
