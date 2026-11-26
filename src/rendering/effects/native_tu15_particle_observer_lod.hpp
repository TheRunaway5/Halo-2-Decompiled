#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_particle_observer_lod_detail {
namespace world = native_world_leaves_detail;
namespace fp = physics_step_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x251d00u, last = 0x251e21u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
inline constexpr std::uint32_t observer_stride = 0x358u;
inline constexpr std::uint32_t local_observers = 0x4f54f0u;
inline constexpr std::uint32_t observer_handle = 0x474c24u;
inline constexpr std::uint32_t lod_override = 0x51df50u;

enum Branch : unsigned {synthetic_observer, array_observer, forced_lod, near_lod, far_lod,
    near_interpolation, full_lod, far_interpolation, clamp_one, clamp_zero, keep_previous,
    publish_view, publish_minimum, next_observer, last_observer, branch_count};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle observer LOD verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage().load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline bool masked_x87() {
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    return (control & 0x3fu) == 0x3fu;
}

// The distance uses extended-precision deltas and sums z² + y² before x².
// The host stack is balanced before returning to the retained frame.
inline long double length(long double x, long double y, long double z) {
    long double value;
    asm volatile(
        "fldt %[x]\n\tfldt %[y]\n\tfldt %[z]\n\t"
        "fld %%st(0)\n\tfmul %%st(1),%%st\n\t"
        "fld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp %%st,%%st(1)\n\t"
        "fsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstpt %[value]"
        : [value] "=m"(value)
        : [x] "m"(x), [y] "m"(y), [z] "m"(z)
        : "st", "st(1)", "st(2)", "st(3)", "st(4)", "memory");
    return value;
}

template<class State>
struct Scan {
    world::Port<State> &port;
    std::array<std::uint32_t,8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t(1) << branch; }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(fp::f(port.read(site, address, 4)));
    }

    bool above(__m128 left, __m128 right) {
        flags.scalar_compare(left, right, false);
        return !flags.cf && !flags.zf;
    }

    void distance() {
        auto &stack = port.s.fp;
        auto &xmm = port.s.xmm;
        const auto load = [&](std::uint32_t site, std::uint32_t address) {
            return native_live_support::x87::load_single(port.read(site, address, 4));
        };
        gp[eax] = flags.logic(0u, 4);
        flags.minus(gp[edx], 0xfffffca8u, 4);
        if (!flags.zf) {
            flags.minus(port.read(0x251d0au, observer_handle, 2), 0xffffffffu, 2);
            if (!flags.zf) gp[eax] = gp[ebx];
        }
        reached(gp[eax] == 0u ? synthetic_observer : array_observer);

        // Keep each committed delta on the retained x87 stack while the
        // following operand can fault. Pure arithmetic has no bank access.
        stack.push(load(0x251d16u, gp[ecx]));
        xmm[0] = xmm[3];
        stack.st(0) = fp::sub(stack.st(0), load(0x251d1bu, gp[eax]));
        stack.push(load(0x251d1du, gp[ecx] + 4u));
        stack.st(0) = fp::sub(stack.st(0), load(0x251d20u, gp[eax] + 4u));
        stack.push(load(0x251d23u, gp[ecx] + 8u));
        stack.st(0) = fp::sub(stack.st(0), load(0x251d26u, gp[eax] + 8u));
        const auto norm = length(stack.st(2), stack.st(1), stack.st(0));
        stack.pop();
        stack.pop();
        stack.st(0) = norm;
        stack.st(0) = fp::mul(stack.st(0), load(0x251d41u, gp[eax] + 0x50u));

        gp[eax] = port.read(0x251d44u, lod_override, 4);
        flags.logic(gp[eax], 4);
        const auto spilled = native_live_support::x87::store_single(stack.st(0));
        stack.status = (stack.status & ~0x200u) | (spilled.rounded_up ? 0x200u : 0u);
        port.store(0x251d4bu, gp[esp] + 0x14u, 4, std::uint32_t(spilled.bits));
        stack.pop();
    }

    void distance_fade() {
        auto &x = port.s.xmm;
        if (!flags.zf) {
            flags.minus(port.read(0x251d51u, gp[eax] + 5u, 1), 0u, 1);
            if (!flags.zf) {
                reached(forced_lod);
                x[0] = x[2];
                return;
            }
        }
        const auto def = gp[ebp];
        x[1] = scalar(0x251d57u, gp[esp] + 0x14u);
        if (!above(x[1], scalar(0x251d5du, def + 0x18u))) {
            reached(near_lod);
            return;
        }
        x[4] = scalar(0x251d63u, def + 0x24u);
        if (!above(x[4], x[1])) {
            reached(far_lod);
            return;
        }

        x[0] = scalar(0x251d6du, def + 0x1cu);
        x[0] = world::ordered_add_ss(x[0], scalar(0x251d72u, def + 0x18u));
        if (above(x[0], x[1])) {
            reached(near_interpolation);
            x[1] = _mm_sub_ss(x[1], scalar(0x251d7cu, def + 0x18u));
            x[1] = native_tu15_leaves::ordered_mul_ss(x[1], scalar(0x251d81u, def + 0x20u));
            x[0] = x[1];
            return;
        }

        x[0] = scalar(0x251d8bu, def + 0x24u);
        x[0] = _mm_sub_ss(x[0], scalar(0x251d90u, def + 0x28u));
        if (!above(x[1], x[0])) {
            reached(full_lod);
            x[0] = x[2];
            return;
        }
        reached(far_interpolation);
        x[0] = scalar(0x251d9au, def + 0x24u);
        x[0] = _mm_sub_ss(x[0], x[1]);
        x[0] = native_tu15_leaves::ordered_mul_ss(x[0], scalar(0x251da3u, def + 0x2cu));
    }

    void combine_with_previous_view() {
        auto &x = port.s.xmm;
        gp[eax] = port.read(0x251dadu, gp[esp] + 0x30u, 4);
        gp[eax] = port.read(0x251db1u, gp[eax] + 12u, 2);
        gp[eax] = flags.right(gp[eax], 10u, 4, false);
        gp[eax] = (gp[eax] & ~0xffu) | flags.logic(gp[eax] & 1u, 1);
        const bool min_one = !flags.zf;
        reached(min_one ? clamp_one : clamp_zero);
        const auto minimum = min_one ? x[2] : x[3];
        x[1] = minimum;
        if (above(x[0], x[1])) x[1] = x[0];
        else flags.logic(gp[eax] & 0xffu, 1);

        x[4] = scalar(0x251dd6u, gp[edi] + 0x1cu);
        if (above(x[4], x[1])) {
            reached(keep_previous);
            x[0] = x[4];
            return;
        }
        flags.logic(gp[eax] & 0xffu, 1);
        x[1] = minimum;
        if (above(x[0], x[1])) {
            reached(publish_view);
            return;
        }
        reached(publish_minimum);
        flags.logic(gp[eax] & 0xffu, 1);
        x[0] = minimum;
    }

    void run() {
        for (;;) {
            distance();
            distance_fade();
            combine_with_previous_view();
            gp[eax] = port.read(0x251dfeu, local_observers, 4);
            port.store(0x251e03u, gp[edi] + 0x1cu, 4, fp::u(_mm_cvtss_f32(port.s.xmm[0])));
            gp[eax] = native_live_support::widen16(port.read(0x251e08u, gp[eax] + 8u, 2));
            gp[esi] = flags.increment(gp[esi], 4, false);
            gp[edx] = flags.plus(gp[edx], observer_stride, 4);
            gp[ebx] = flags.plus(gp[ebx], observer_stride, 4);
            flags.minus(gp[esi], gp[eax], 4);
            if (flags.sf == flags.of) break;
            reached(next_observer);
        }
        reached(last_observer);
        port.publish(last);
    }
};
#endif

template<class State, class Reference>
bool bridge(State &state, Reference reference) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_OBSERVER_LOD");
        return value && *value;
    }();
    if (reference_mode || state.active_mmx || state.fp.count > 3u || !masked_x87()) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, summary().calls, reference, [](world::Port<State> &port) {
        Scan scan{port};
        scan.run();
        coverage().fetch_or(scan.hit, std::memory_order_relaxed);
    });
    return true;
#else
    (void)state;
    (void)reference;
    return false;
#endif
}
} // namespace halo2::native_particle_observer_lod_detail
