#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"

#include "halo2/particle_emitter.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_emitter_orientation_detail {
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebp;
using world::ecx;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2508a5u, last = 0x250976u;
inline constexpr std::uint32_t emitter_slot = 0xffffffb4u, placement_slot = 0xffffffe8u;
inline constexpr std::uint32_t offset_slot = 0xffffffecu, correction_slot = 0xffffffc4u;
inline constexpr std::uint32_t zero = 0x46a0e8u, angle_threshold = 0x456670u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
static_assert(offsetof(ParticleEmitterState, rotation) == 0x10u && offsetof(ParticleEmitterState, position) == 0x34u);
enum Branch : unsigned {first_angle, second_angle, no_correction, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle emitter orientation verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

struct AngleSites {
    std::uint32_t zero, push, spill, call, returned, threshold, compare, pop;
};

template<class State, class Route>
struct Placement {
    world::Port<State> &port;
    Route &route;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    __m128 &x = port.s.xmm[0];

    template<std::uint32_t target>
    void call(std::uint32_t site, std::uint32_t returned) {
        port.push(site, returned);
        auto child = [&](State &state) { route(state, std::integral_constant<std::uint32_t, target>{}, site); };
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void spill(std::uint32_t site, std::uint32_t address) {
        port.store(site, address, 4, native_tu15_leaves::low_bits(x));
    }

    // 1C180 owns the x87 result and its cdecl return. Keep the original
    // spill and comparison boundaries between that child and either route.
    bool threshold_above_angle(const AngleSites &sites) {
        x = _mm_sub_ss(x, scalar(sites.zero, zero));
        port.push(sites.push, gp[ecx]);
        spill(sites.spill, gp[esp]);
        call<0x1c180u>(sites.call, sites.returned);
        gp[esp] = flags.plus(gp[esp], 4u, 4);
        auto &state = port.s;
        state.fp.push(physics_step_detail::ext(physics_step_detail::f(port.read(sites.threshold, angle_threshold, 4))));
        port.at(sites.compare);
        state.flags = flags;
        state.compare_and_pop(state.fp.st(1));
        flags = state.flags;
        port.at(sites.pop);
        state.fp.pop();
        return !flags.cf && !flags.zf;
    }

    void apply_basis_correction() {
        port.push(0x250911u, 0u);
        gp[edx] = port.read(0x250913u, gp[ebp] + placement_slot, 4);
        gp[eax] = port.read(0x250916u, gp[edx] + 4u, 4);
        port.push(0x250919u, gp[eax]);
        gp[ecx] = port.read(0x25091au, gp[ebp] + placement_slot, 4);
        gp[edx] = port.read(0x25091du, gp[ecx], 4);
        port.push(0x25091fu, gp[edx]);
        gp[eax] = gp[ebp] + correction_slot;
        call<0x1458a0u>(0x250923u, 0x250928u);

        gp[eax] = port.read(0x250928u, gp[ebp] + emitter_slot, 4);
        gp[eax] = flags.plus(gp[eax], offsetof(ParticleEmitterState, rotation), 4);
        gp[edx] = port.read(0x25092eu, gp[ebp] + emitter_slot, 4);
        gp[edx] = flags.plus(gp[edx], offsetof(ParticleEmitterState, rotation), 4);
        gp[ecx] = gp[ebp] + correction_slot;
        call<0x1459b0u>(0x250937u, 0x25093cu);
    }

    void add_offset_axis(unsigned source, unsigned destination, unsigned axis,
                         std::array<std::uint32_t, 5> sites) {
        const auto lane = 4u * axis;
        const auto position = std::uint32_t(offsetof(ParticleEmitterState, position)) + lane;
        gp[source] = port.read(sites[0], gp[ebp] + emitter_slot, 4);
        x = scalar(sites[1], gp[source] + position);
        x = world::ordered_add_ss(x, scalar(sites[2], gp[ebp] + offset_slot + lane));
        gp[destination] = port.read(sites[3], gp[ebp] + emitter_slot, 4);
        spill(sites[4], gp[destination] + position);
    }

    void run() {
        call<0x2503f0u>(first, 0x2508aau);
        gp[edx] = gp[eax];
        gp[esi] = port.read(0x2508acu, gp[ebp] + emitter_slot, 4);
        gp[esi] = flags.plus(gp[esi], offsetof(ParticleEmitterState, rotation), 4);
        gp[eax] = gp[ebp] + offset_slot;
        gp[ecx] = gp[edx];
        gp[edx] = gp[esi];
        call<0x145b70u>(0x2508b9u, 0x2508beu);

        gp[eax] = port.read(0x2508beu, gp[ebp] + placement_slot, 4);
        x = scalar(0x2508c1u, gp[eax] + 4u);
        Branch branch = first_angle;
        if (threshold_above_angle({0x2508c6u, 0x2508ceu, 0x2508cfu, 0x2508d4u,
                                   0x2508d9u, 0x2508dcu, 0x2508e2u, 0x2508e4u})) {
            gp[ecx] = port.read(0x2508e8u, gp[ebp] + placement_slot, 4);
            x = scalar(0x2508ebu, gp[ecx]);
            branch = threshold_above_angle({0x2508efu, 0x2508f7u, 0x2508f8u, 0x2508fdu,
                                            0x250902u, 0x250905u, 0x25090bu, 0x25090du})
                ? no_correction : second_angle;
        }
        if (branch != no_correction) apply_basis_correction();
        add_offset_axis(eax, ecx, 0u, {0x25093cu, 0x25093fu, 0x250944u, 0x250949u, 0x25094cu});
        add_offset_axis(edx, eax, 1u, {0x250951u, 0x250954u, 0x250959u, 0x25095eu, 0x250961u});
        add_offset_axis(ecx, edx, 2u, {0x250966u, 0x250969u, 0x25096eu, 0x250973u, 0x250976u});
        port.publish(last);
        summary().coverage.fetch_or(1u << branch, std::memory_order_relaxed);
    }
};

template<class State, class Reference, class Route>
bool bridge(State &state, Reference reference, Route route) {
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_EMITTER_ORIENTATION");
        return value && *value;
    }();
    if (ref || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, summary().calls, reference, [&](world::Port<State> &port) {
        Placement{port, route}.run();
    });
    return true;
}
} // namespace halo2::native_emitter_orientation_detail
