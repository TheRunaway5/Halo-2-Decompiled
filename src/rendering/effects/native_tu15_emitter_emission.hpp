#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"

#include "halo2/particle_emitter.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_emitter_emission_detail {
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x25097bu, last = 0x250ab5u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
inline constexpr std::uint32_t emitter_slot = 0xffffffb4u, fraction_slot = 0xffffffbcu;
inline constexpr std::uint32_t interval_slot = 0xffffffc0u, scale_slot = 0xfffffffcu;
inline constexpr std::uint32_t one = 0x46a0d0u, zero = 0x46a0e8u, epsilon = 0x46a0ecu;
inline constexpr std::uint32_t remainder = offsetof(ParticleEmitterState, emission_remainder);
static_assert(remainder == 12u && offsetof(ParticleEmitterState, first_particle) == 4u);
enum Branch : unsigned {add_emission, keep_remainder, below_threshold, steady_emission, interpolated_emission,
    give_birth, repeat_birth, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle emitter emission verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Route>
struct Tail {
    world::Port<State> &port;
    Route &route;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    __m128 &x = port.s.xmm[0];
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t(1) << branch; }

    template<std::uint32_t target>
    void call(std::uint32_t site, std::uint32_t returned) {
        port.push(site, returned);
        auto child = [&](State &state) { route(state, std::integral_constant<std::uint32_t, target>{}); };
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

    void accumulate_emission() {
        gp[eax] = port.read(first, gp[ebp] - 8u, 4);
        call<0x176a20u>(0x25097eu, 0x250983u);
        gp[eax] &= 0xffu;
        flags.logic(gp[eax], 4);
        if (!flags.zf) {
            gp[ecx] = port.read(0x25098au, gp[ebp] + emitter_slot, 4);
            flags.minus(port.read(0x25098du, gp[ecx] + offsetof(ParticleEmitterState, first_particle), 4), 0xffffffffu, 4);
            if (!flags.zf) {
                reached(keep_remainder);
                return;
            }
        }

        reached(add_emission);
        gp[ebx] = port.read(0x250993u, gp[ebp] + 0x14u, 4);
        gp[eax] = port.read(0x250996u, gp[ebp] + 0x10u, 4);
        call<0x250410u>(0x250999u, 0x25099eu);
        x = native_tu15_leaves::ordered_mul_ss(x, scalar(0x25099eu, gp[ebp] + 8u));
        gp[edx] = port.read(0x2509a3u, gp[ebp] + emitter_slot, 4);
        x = world::ordered_add_ss(x, scalar(0x2509a6u, gp[edx] + remainder));
        gp[eax] = port.read(0x2509abu, gp[ebp] + emitter_slot, 4);
        spill(0x2509aeu, gp[eax] + remainder);
    }

    bool use_interpolated_emission() {
        gp[eax] = port.read(0x2509ddu, gp[ebp] + 0xcu, 4);
        call<0x250440u>(0x2509e0u, 0x2509e5u);
        gp[edx] = gp[eax] & 0xffu;
        flags.logic(gp[edx], 4);
        if (flags.zf) return false;
        gp[ecx] = port.read(0x2509ecu, gp[ebp] + 0xcu, 4);
        call<0x178070u>(0x2509efu, 0x2509f4u);
        call<0x250430u>(0x2509f4u, 0x2509f9u);
        gp[eax] &= 0xffu;
        flags.logic(gp[eax], 4);
        return !flags.zf;
    }

    void prepare_births() {
        x = scalar(0x2509d0u, zero);
        spill(0x2509d8u, gp[ebp] + fraction_slot);
        if (use_interpolated_emission()) {
            reached(interpolated_emission);
            gp[ecx] = port.read(0x250a00u, gp[ebp] + emitter_slot, 4);
            x = scalar(0x250a03u, one);
            x = _mm_div_ss(x, scalar(0x250a0bu, gp[ecx] + remainder));
            spill(0x250a10u, gp[ebp] + interval_slot);
            x = scalar(0x250a15u, zero);
            spill(0x250a1du, gp[ebp] + fraction_slot);
        } else {
            reached(steady_emission);
            x = scalar(0x250a24u, zero);
            spill(0x250a2cu, gp[ebp] + interval_slot);
            x = scalar(0x250a31u, one);
            spill(0x250a39u, gp[ebp] + fraction_slot);
            x = scalar(0x250a3eu, zero);
            spill(0x250a46u, gp[ebp] + 8u);
        }
    }

    bool birth_available() {
        gp[edx] = port.read(0x250a4bu, gp[ebp] + emitter_slot, 4);
        x = scalar(0x250a4eu, gp[edx] + remainder);
        x = world::ordered_add_ss(x, scalar(0x250a53u, epsilon));
        flags.scalar_compare(x, scalar(0x250a5bu, one), false);
        return !flags.cf;
    }

    void birth_particle() {
        gp[eax] = port.read(0x250a64u, gp[ebp] + emitter_slot, 4);
        x = scalar(0x250a67u, gp[eax] + remainder);
        x = _mm_sub_ss(x, scalar(0x250a6cu, one));
        gp[ecx] = port.read(0x250a74u, gp[ebp] + emitter_slot, 4);
        spill(0x250a77u, gp[ecx] + remainder);

        gp[edx] = port.read(0x250a7cu, gp[ebp] + scale_slot, 4);
        port.push(0x250a7fu, gp[edx]);
        gp[eax] = port.read(0x250a80u, gp[ebp] + 8u, 4);
        port.push(0x250a83u, gp[eax]);
        gp[ecx] = port.read(0x250a84u, gp[ebp] + fraction_slot, 4);
        port.push(0x250a87u, gp[ecx]);
        gp[edx] = port.read(0x250a88u, gp[ebp] + 0x1cu, 4);
        port.push(0x250a8bu, gp[edx]);
        gp[eax] = port.read(0x250a8cu, gp[ebp] + 0x10u, 4);
        port.push(0x250a8fu, gp[eax]);
        gp[ecx] = port.read(0x250a90u, gp[ebp] + 0xcu, 4);
        port.push(0x250a93u, gp[ecx]);
        gp[eax] = port.read(0x250a94u, gp[ebp] + 0x14u, 4);
        gp[ecx] = port.read(0x250a97u, gp[ebp] + emitter_slot, 4);
        call<0x250c60u>(0x250a9au, 0x250a9fu);

        x = scalar(0x250a9fu, gp[ebp] + fraction_slot);
        x = world::ordered_add_ss(x, scalar(0x250aa4u, gp[ebp] + interval_slot));
        spill(0x250aa9u, gp[ebp] + fraction_slot);
    }

    void run() {
        accumulate_emission();
        gp[ecx] = port.read(0x2509b3u, gp[ebp] + emitter_slot, 4);
        x = scalar(0x2509b6u, gp[ecx] + remainder);
        x = world::ordered_add_ss(x, scalar(0x2509bbu, epsilon));
        flags.scalar_compare(x, scalar(0x2509c3u, one), false);
        if (flags.cf) {
            reached(below_threshold);
        } else {
            prepare_births();
            while (birth_available()) {
                reached((hit & (1u << give_birth)) ? repeat_birth : give_birth);
                birth_particle();
            }
        }
        gp[esi] = port.pop(0x250ab0u);
        gp[ebx] = port.pop(0x250ab1u);
        gp[esp] = gp[ebp];
        gp[ebp] = port.pop(0x250ab4u);
        port.ret(last, 24u);
        port.publish(last);
    }
};

template<class State, class Reference, class Route>
bool bridge(State &state, Reference reference, Route route) {
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_EMITTER_EMISSION");
        return value && *value;
    }();
    if (ref || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, summary().calls, reference, [&](world::Port<State> &port) {
        Tail tail{port, route};
        tail.run();
        summary().coverage.fetch_or(tail.hit, std::memory_order_relaxed);
    });
    return true;
}
} // namespace halo2::native_emitter_emission_detail
