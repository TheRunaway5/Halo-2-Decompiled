#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"

#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_birth_prefix_detail {
namespace world = native_world_leaves_detail;
namespace fp = physics_step_detail;
namespace location = native_effects::location_update;
namespace inputs = native_effects::particle_inputs;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using location::ParticleDatum;

inline constexpr std::uint32_t first = 0x250c60u, last = 0x250e64u;
inline constexpr std::uint32_t allocate = 0x16e6a0u, resolve = 0x178070u;
inline constexpr std::uint32_t random_state = 0x4f3cd8u, random_seed = 4u;
inline constexpr std::uint32_t seed_multiplier = 0x19660du, seed_increment = 0x3c6ef35fu;
inline constexpr std::uint32_t low_unsigned_correction = 0x46a0c8u, random_word_range = 0x46a1b4u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
enum class Resume { retained, evaluate_properties, return_without_birth };
enum Branch : unsigned {unowned, owned, blocked, exhausted, born, cache_hit, cache_miss, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle birth allocation and RNG verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

struct SeedSites { std::uint32_t load, store; };
struct RandomSites {
    SeedSites seed;
    std::uint32_t spill_integer, reload_integer, correction, unit_scale, word_range;
    std::uint32_t spill_single, reload_single, store_result;
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State, class Dispatch, class Resolve>
struct Entry {
    world::Port<State> &port;
    Dispatch &dispatch;
    Resolve &resolver;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    __m128 &x = port.s.xmm[0];
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t(1) << branch; }

    template<class Child>
    void call(std::uint32_t site, std::uint32_t returned, std::uint32_t target, Child &child) {
        port.push(site, returned);
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    void prepare_frame() {
        gp[esp] = flags.minus(gp[esp], 0xb0u, 4);
        port.push(0x250c66u, gp[ebx]);
        port.push(0x250c67u, gp[ebp]);
        port.push(0x250c68u, gp[esi]);
        port.push(0x250c69u, gp[edi]);
        gp[edi] = port.read(0x250c6au, gp[esp] + 0xc4u, 4);
        gp[esi] = gp[eax];
        gp[eax] = port.read(0x250c73u, gp[edi] + offsetof(inputs::Emitter, effect), 4);
        flags.minus(gp[eax], 0xffffffffu, 4);
        gp[ebx] = gp[ecx];
        port.store(0x250c7bu, gp[esp] + 0x14u, 4, gp[eax]);
    }

    bool effect_blocks_birth() {
        if (flags.zf) {
            reached(unowned);
            return false;
        }
        reached(owned);
        gp[ecx] = port.read(0x250c81u, inputs::globals::effects, 4);
        gp[edx] = port.read(0x250c87u, gp[ecx] + 0x44u, 4);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.product(gp[eax], sizeof(inputs::EffectDatum));
        gp[eax] = flags.plus(gp[eax], gp[edx], 4);
        const auto flags_byte = port.read(0x250c97u, gp[eax] + 2u, 1);
        const auto shifted = flags.right(flags_byte, 3u, 1, false);
        const auto inhibit = flags.logic(shifted & 1u, 1);
        gp[edx] = (gp[edx] & 0xffffff00u) | inhibit;
        port.store(0x250ca0u, gp[esp] + 0x1bu, 1, inhibit);
        return !flags.zf;
    }

    bool allocate_particle() {
        gp[edx] = port.read(0x250caau, location::globals::particles, 4);
        auto allocator = [&](State &value) { dispatch(value, allocate, 0x250cb0u); };
        call(0x250cb0u, 0x250cb5u, allocate, allocator);
        flags.minus(gp[eax], 0xffffffffu, 4);
        port.store(0x250cb8u, gp[esp] + 0x20u, 4, gp[eax]);
        return !flags.zf;
    }

    void link_particle() {
        gp[ebp] = gp[eax];
        gp[eax] = port.read(0x250cc4u, gp[edx] + 0x44u, 4);
        gp[ebp] = flags.logic(gp[ebp] & 0xffffu, 4);
        gp[ebp] = flags.shift(gp[ebp], 6u, 4);
        gp[ecx] = gp[edi];
        gp[ebp] = flags.plus(gp[ebp], gp[eax], 4);
        call(0x250cd4u, 0x250cd9u, resolve, resolver);

        gp[ecx] = port.read(0x250cd9u, gp[esp] + 0x20u, 4);
        port.store(0x250cddu, gp[esp] + 0x1cu, 4, gp[eax]);
        gp[eax] = port.read(0x250ce1u, gp[ebx] + offsetof(location::EmitterDatum, first_particle), 4);
        port.store(0x250ce4u, gp[ebp] + offsetof(ParticleDatum, next), 4, gp[eax]);
        port.store(0x250ce7u, gp[ebx] + offsetof(location::EmitterDatum, first_particle), 4, gp[ecx]);
    }

    void advance_seed(SeedSites sites) {
        gp[eax] = port.read(sites.load, gp[ecx] + random_seed, 4);
        gp[eax] = flags.product(gp[eax], seed_multiplier);
        gp[eax] = flags.plus(gp[eax], seed_increment, 4);
        port.store(sites.store, gp[ecx] + random_seed, 4, gp[eax]);
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
    }

    void initialize_age_and_variant() {
        gp[ecx] = port.read(0x250ceau, random_state, 4);
        gp[edx] = flags.logic(0u, 4);
        gp[edx] = port.read(0x250cf2u, gp[ebp] + offsetof(ParticleDatum, flags), 2);
        x = _mm_setzero_ps();
        gp[edx] = flags.logic(gp[edx] & 0xe004u, 4);
        gp[edx] = flags.logic(gp[edx] | 4u, 4);
        port.store(0x250d02u, gp[ebp] + offsetof(ParticleDatum, flags), 2, gp[edx]);
        advance_seed({0x250d06u, 0x250d14u});
        gp[edx] = flags.logic(0u, 4);
        gp[edx] = port.read(0x250d1cu, gp[ebp] + offsetof(ParticleDatum, flags), 2);
        gp[eax] *= 5u;
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
        gp[eax] = flags.shift(gp[eax], 13u, 4);
        port.store(0x250d29u, gp[ebp] + offsetof(ParticleDatum, age), 4, 0u);
        gp[edx] = flags.logic(gp[edx] & 0x1fffu, 4);
        gp[eax] = flags.logic(gp[eax] | gp[edx], 4);
        port.store(0x250d36u, gp[ebp] + offsetof(ParticleDatum, flags), 2, gp[eax]);
    }

    void random_word(unsigned index, const RandomSites &sites) {
        advance_seed(sites.seed);
        flags.logic(gp[eax], 4);
        const auto temporary = gp[esp] + 0x10u;
        port.store(sites.spill_integer, temporary, 4, gp[eax]);
        auto &stack = port.s.fp;
        stack.push(static_cast<long double>(fp::si(port.read(sites.reload_integer, temporary, 4))));
        if (flags.sf != flags.of)
            stack.st(0) = fp::add(stack.st(0), fp::ext(fp::f(port.read(sites.correction, low_unsigned_correction, 4))));
        stack.st(0) = fp::mul(stack.st(0), fp::ext(fp::f(port.read(sites.unit_scale, inputs::globals::word_scale, 4))));
        stack.st(0) = fp::mul(stack.st(0), fp::ext(fp::f(port.read(sites.word_range, random_word_range, 4))));
        const auto spill = native_live_support::x87::store_single(stack.st(0));
        stack.status = (stack.status & ~0x200u) | (spill.rounded_up ? 0x200u : 0u);
        port.store(sites.spill_single, temporary, 4, std::uint32_t(spill.bits));
        stack.pop();
        x = world::scalar(fp::f(port.read(sites.reload_single, temporary, 4)));
        gp[eax] = std::uint32_t(_mm_cvttss_si32(x));
        port.store(sites.store_result, gp[ebp] + offsetof(ParticleDatum, random) + 2u*index, 2, gp[eax]);
    }

    void initialize_random_words() {
        initialize_age_and_variant();
        random_word(0u, {{0x250d3au, 0x250d48u}, 0x250d50u, 0x250d54u, 0x250d5au,
                         0x250d60u, 0x250d66u, 0x250d6cu, 0x250d70u, 0x250d7au});
        random_word(1u, {{0x250d7eu, 0x250d8cu}, 0x250d94u, 0x250d98u, 0x250d9eu,
                         0x250da4u, 0x250daau, 0x250db0u, 0x250db4u, 0x250dbeu});
        random_word(2u, {{0x250dc2u, 0x250dd0u}, 0x250dd8u, 0x250ddcu, 0x250de2u,
                         0x250de8u, 0x250deeu, 0x250df4u, 0x250df8u, 0x250e02u});
        random_word(3u, {{0x250e06u, 0x250e14u}, 0x250e1cu, 0x250e20u, 0x250e26u,
                         0x250e2cu, 0x250e32u, 0x250e38u, 0x250e3cu, 0x250e46u});
    }

    Resume run() {
        prepare_frame();
        if (effect_blocks_birth()) {
            reached(blocked);
            port.publish(0x250ca4u);
            return Resume::return_without_birth;
        }
        if (!allocate_particle()) {
            reached(exhausted);
            port.publish(0x250cbcu);
            return Resume::return_without_birth;
        }
        reached(born);
        link_particle();
        initialize_random_words();

        x = world::scalar(fp::f(port.read(0x250e4au, gp[edi] + 4u, 4)));
        port.store(0x250e4fu, gp[ebp] + offsetof(ParticleDatum, system_fraction), 4, native_tu15_leaves::low_bits(x));
        flags.minus(gp[ebp], port.read(0x250e54u, gp[esi] + 0x50u, 4), 4);
        if (flags.zf) {
            reached(cache_hit);
            port.publish(0x250e57u);
        } else {
            reached(cache_miss);
            gp[eax] = port.read(0x250e59u, gp[esi] + 0x44u, 4);
            gp[eax] = flags.logic(gp[eax] & 0xffff07f0u, 4);
            port.store(0x250e61u, gp[esi] + 0x50u, 4, gp[ebp]);
            port.store(last, gp[esi] + 0x44u, 4, gp[eax]);
            port.publish(last);
        }
        return Resume::evaluate_properties;
    }
};
#endif

template<class State, class Reference, class Dispatch, class Resolve>
Resume bridge(State &state, Reference reference, Dispatch dispatch, Resolve resolver) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_BIRTH_PREFIX");
        return value && *value;
    }();
    if (ref || state.active_mmx) return Resume::retained;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return Resume::retained;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return Resume::retained;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return Resume::retained;
    Resume next = Resume::retained;
    auto replay = [&](State &original) {
        if (reference(original) != next) throw std::runtime_error("Native particle birth resumed at the wrong site");
    };
    world::execute(state, publication, summary().calls, replay, [&](world::Port<State> &port) {
        Entry entry{port, dispatch, resolver};
        next = entry.run();
        summary().coverage.fetch_or(entry.hit, std::memory_order_relaxed);
    });
    return next;
#else
    (void)state;
    (void)reference;
    (void)dispatch;
    (void)resolver;
    return Resume::retained;
#endif
}
} // namespace halo2::native_particle_birth_prefix_detail
