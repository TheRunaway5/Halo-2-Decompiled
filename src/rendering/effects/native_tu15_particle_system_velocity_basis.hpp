#pragma once

#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include "rendering/effects/native_tu15_particle_system_update.hpp"
#include <cmath>

namespace halo2::native_particle_system_velocity_basis_detail {
namespace system = native_particle_system_update_detail;
namespace world = native_world_leaves_detail;
namespace fp = physics_step_detail;
using world::ebx;
using world::esp;
using native_tu15_leaves::ordered_mul_ss;

inline constexpr std::uint32_t first = 0x177b7cu, last = 0x177ccbu;
inline constexpr std::uint32_t final_lane = 0x177ccfu, final_store = 0x177cd3u;
inline constexpr std::uint32_t negative_one = 0x46a0dcu, one = 0x46a0d0u, epsilon = 0x46a0ecu;
inline constexpr auto velocity = offsetof(native_effects::location_update::ParticleDatum, velocity);
inline constexpr world::Function velocity_sites[]{{first, last, 0u, 0u}};
inline constexpr world::Function lane_sites[]{{final_lane, final_store, 0u, 0u}};
enum Branch : unsigned {normalized, below_epsilon, perpendicular_x, perpendicular_y, perpendicular_z, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> velocities{}, lane_stores{}, coverage{};
    ~VerifySummary() {
        if (velocities || lane_stores)
            std::fprintf(stderr, "Particle system velocity basis verify: %llu velocities, %llu last lanes, coverage %llx\n",
                         static_cast<unsigned long long>(velocities.load()), static_cast<unsigned long long>(lane_stores.load()),
                         static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_SYSTEM_VELOCITY_BASIS");
        return value && *value;
    }();
    return enabled;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
struct VelocityBasis {
    world::Port<State> &port;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;
    decltype(port.s.fp) &stack = port.s.fp;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t{1} << branch; }

    std::uint32_t local(std::uint32_t offset) { return port.gp[esp] + offset; }

    long double extended(std::uint32_t site, std::uint32_t address) {
        return fp::ext(fp::f(port.read(site, address, 4)));
    }

    __m128 scalar(std::uint32_t site, std::uint32_t offset) {
        return world::scalar(fp::f(port.read(site, local(offset), 4)));
    }

    __m128 coefficient(std::uint32_t site, std::uint32_t address) {
        return world::scalar(fp::f(port.read(site, address, 4)));
    }

    void spill_extended(std::uint32_t site, std::uint32_t offset) {
        port.at(site);
        const auto spill = native_live_support::x87::store_single(stack.st(0));
        stack.status = (stack.status & ~0x200u) | (spill.rounded_up ? 0x200u : 0u);
        port.store(site, local(offset), 4, std::uint32_t(spill.bits));
    }

    void spill(std::uint32_t site, std::uint32_t offset, unsigned reg) {
        port.store(site, local(offset), 4, native_tu15_leaves::low_bits(x[reg]));
    }

    void compare_and_discard(std::uint32_t compare, std::uint32_t discard) {
        port.at(compare);
        port.s.flags = flags;
        port.s.compare_and_pop(stack.st(1));
        flags = port.s.flags;
        port.at(discard);
        stack.pop();
    }

    void reverse_axis(unsigned axis, std::uint32_t load, std::uint32_t multiply, std::uint32_t store) {
        stack.push(extended(load, port.gp[ebx] + velocity + 4u * axis));
        stack.st(0) = fp::mul(stack.st(0), extended(multiply, negative_one));
        spill_extended(store, 0x40u + 4u * axis);
        stack.pop();
    }

    void square_axis(std::uint32_t load, std::uint32_t multiply, std::uint32_t offset) {
        stack.push(extended(load, local(offset)));
        stack.st(0) = fp::mul(stack.st(0), extended(multiply, local(offset)));
    }

    void sum_top(std::uint32_t site) {
        port.at(site);
        stack.st(1) = fp::add(stack.st(1), stack.st(0));
        stack.pop();
    }

    bool scale_needed() {
        reverse_axis(0u, first, 0x177b7fu, 0x177b85u);
        reverse_axis(1u, 0x177b89u, 0x177b8cu, 0x177b92u);
        x[2] = scalar(0x177b96u, 0x44u);
        reverse_axis(2u, 0x177b9cu, 0x177b9fu, 0x177ba5u);

        square_axis(0x177ba9u, 0x177badu, 0x44u);
        square_axis(0x177bb1u, 0x177bb5u, 0x40u);
        sum_top(0x177bb9u);
        square_axis(0x177bbbu, 0x177bbfu, 0x48u);
        sum_top(0x177bc3u);
        port.at(0x177bc5u);
        stack.st(0) = physics_solver_detail::root(stack.st(0));
        spill_extended(0x177bc7u, 0x18u);
        port.at(0x177bcbu);
        stack.st(0) = std::fabs(stack.st(0));
        stack.push(extended(0x177bcdu, epsilon));
        compare_and_discard(0x177bd3u, 0x177bd5u);
        return flags.cf || flags.zf;
    }

    void load_or_normalize_velocity() {
        if (!scale_needed()) {
            reached(below_epsilon);
            x[5] = scalar(0x177c11u, 0x48u);
            x[4] = scalar(0x177c17u, 0x40u);
            return;
        }
        reached(normalized);
        x[0] = coefficient(0x177bd9u, one);
        x[0] = _mm_div_ss(x[0], scalar(0x177be1u, 0x18u));
        x[4] = x[0];
        x[4] = ordered_mul_ss(x[4], scalar(0x177beau, 0x40u));
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[0] = ordered_mul_ss(x[0], scalar(0x177bf4u, 0x48u));
        x[5] = x[0];
        spill(0x177bfdu, 0x40u, 4u);
        spill(0x177c03u, 0x44u, 2u);
        spill(0x177c09u, 0x48u, 5u);
    }

    void absolute_axis(std::uint32_t load, std::uint32_t store, std::uint32_t source, std::uint32_t output) {
        stack.push(extended(load, local(source)));
        stack.st(0) = std::fabs(stack.st(0));
        spill_extended(store, output);
        stack.pop();
    }

    void choose_perpendicular() {
        absolute_axis(0x177c1du, 0x177c23u, 0x40u, 0x18u);
        absolute_axis(0x177c27u, 0x177c2du, 0x44u, 0x1cu);
        absolute_axis(0x177c31u, 0x177c37u, 0x48u, 0x28u);
        stack.push(extended(0x177c3bu, local(0x18u)));
        x[0] = scalar(0x177c3fu, 0x28u);
        stack.push(extended(0x177c45u, local(0x1cu)));
        compare_and_discard(0x177c49u, 0x177c4bu);
        if (!flags.cf) {
            flags.scalar_compare(x[0], scalar(0x177c4fu, 0x18u), false);
            if (!flags.cf) {
                reached(perpendicular_x);
                x[1] = _mm_setzero_ps();
                x[3] = x[1];
                x[0] = x[5];
                x[1] = _mm_sub_ss(x[1], x[2]);
                return;
            }
        }
        flags.scalar_compare(x[0], scalar(0x177c65u, 0x1cu), false);
        if (!flags.cf) {
            reached(perpendicular_y);
            x[0] = _mm_setzero_ps();
            x[3] = x[0];
            x[3] = _mm_sub_ss(x[3], x[5]);
            x[1] = x[4];
            return;
        }
        reached(perpendicular_z);
        x[1] = _mm_setzero_ps();
        x[0] = x[1];
        x[3] = x[2];
        x[0] = _mm_sub_ss(x[0], x[4]);
    }

    void run() {
        load_or_normalize_velocity();
        choose_perpendicular();
        x[6] = ordered_mul_ss(x[1], x[2]);
        x[7] = ordered_mul_ss(x[0], x[5]);
        x[6] = _mm_sub_ss(x[6], x[7]);
        spill(0x177c9au, 0x4cu, 6u);
        spill(0x177ca0u, 0x60u, 1u);
        x[6] = ordered_mul_ss(x[3], x[5]);
        x[1] = ordered_mul_ss(x[1], x[4]);
        spill(0x177cb1u, 0x5cu, 0u);
        x[6] = _mm_sub_ss(x[6], x[1]);
        spill(0x177cbbu, 0x58u, 3u);
        spill(0x177cc1u, 0x50u, 6u);
        x[0] = ordered_mul_ss(x[0], x[4]);
        x[2] = ordered_mul_ss(x[2], x[3]);
        port.publish(last);
    }
};
#endif

template<class State, class Ref>
bool bridge_velocity(State &state, Ref reference) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    world::Publication publication;
    if (reference_mode() || !system::admit(state, velocity_sites, publication)) return false;
    world::execute(state, publication, summary().velocities, reference, [&](world::Port<State> &port) {
        VelocityBasis basis{port};
        basis.run();
        summary().coverage.fetch_or(basis.hit, std::memory_order_relaxed);
    });
    return true;
#else
    (void)state;
    (void)reference;
    return false;
#endif
}

template<class State, class Ref>
bool bridge_lane(State &state, Ref reference) {
    world::Publication publication;
    if (reference_mode() || !system::admit(state, lane_sites, publication)) return false;
    world::execute(state, publication, summary().lane_stores, reference, [](world::Port<State> &port) {
        auto &x = port.s.xmm;
        x[0] = _mm_sub_ss(x[0], x[2]);
        port.store(final_store, port.gp[esp] + 0x54u, 4, native_tu15_leaves::low_bits(x[0]));
        port.publish(final_store);
    });
    return true;
}
} // namespace halo2::native_particle_system_velocity_basis_detail
