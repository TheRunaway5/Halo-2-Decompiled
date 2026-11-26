#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_contact_walk_detail {
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
namespace location = native_effects::location_update;
using location::ParticleDatum;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2c2d90u, particle = 0x2c2e92u;
inline constexpr std::uint32_t collision = 0x2c2ee0u, countdown = 0x2c3366u;
inline constexpr std::uint32_t advance = 0x2c3375u, epilogue = 0x2c3384u, last = 0x2c338eu;
inline constexpr std::uint32_t fixed_lookup = 0x24ffe0u, enumerate = 0x176a40u;
inline constexpr std::uint32_t local_size = 0x124u, caller_args = 20u;
inline constexpr std::uint32_t next_slot = 0x1cu, element_index = 0xffffu, end_of_chain = 0xffffffffu;
inline constexpr world::Function setup_sites[]{{first, 0x2c2e8cu, 0u, 0u}};
inline constexpr world::Function walk_sites[]{{particle, last, 0u, 0u}};

enum class Start { particle, advance, epilogue };
enum class Resume { retained, particle, epilogue, collision, returned };

struct VerifySummary {
    std::atomic<std::uint64_t> setup{}, walk{};
    ~VerifySummary() {
        if (setup || walk)
            std::fprintf(stderr, "Particle contact walk verify: %llu setup, %llu traversal\n",
                         static_cast<unsigned long long>(setup.load()),
                         static_cast<unsigned long long>(walk.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_walk() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CONTACT_WALK");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct>
struct Setup {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    void resolve_inputs() {
        gp[esp] = flags.minus(gp[esp], local_size, 4);
        port.push(0x2c2d96u, gp[ebx]);
        port.push(0x2c2d97u, gp[ebp]);
        port.push(0x2c2d98u, gp[esi]);
        gp[esi] = port.read(0x2c2d99u, gp[esp] + 0x138u, 4);
        port.push(0x2c2da0u, gp[edi]);
        gp[edi] = port.read(0x2c2da1u, gp[esp] + 0x138u, 4);
        gp[ebx] = port.read(0x2c2da8u, gp[edi] + 8u, 4);
        gp[ecx] = gp[esi];
        calls.template call_local<system::resolve_system>(0x2c2dadu, 0x2c2db2u);
        gp[edx] = port.read(0x2c2db2u, gp[eax] + 4u, 4);
        calls.template call_local<system::resolve_interface>(0x2c2db5u, 0x2c2dbau);
        port.set_xmm(0, _mm_setzero_ps());
        gp[ecx] = port.read(0x2c2dbdu, gp[edi] + 0xcu, 4);
        port.store(0x2c2dc0u, gp[esp] + 0x18u, 4, gp[eax]);
        gp[eax] = port.read(0x2c2dc4u, gp[esp] + 0x140u, 4);
        gp[ebp] = flags.logic(0u, 4);
        port.store(0x2c2dcdu, gp[esp] + next_slot, 4, gp[eax]);
        gp[eax] = flags.logic(0u, 4);
        flags.minus(gp[esi], gp[ebp], 4);
        port.store(0x2c2dd5u, gp[esp] + 0x94u, 4, gp[ebp]);
        port.store(0x2c2ddcu, gp[esp] + 0x98u, 4, gp[ebp]);
        port.store(0x2c2de3u, gp[esp] + 0x9cu, 4, gp[ebp]);
        port.store(0x2c2deau, gp[esp] + 0xa0u, 4, gp[ebp]);
        port.store(0x2c2df1u, gp[esp] + 0x2cu, 4, 0u);
        port.store(0x2c2df7u, gp[esp] + 0x30u, 4, 0u);
        port.store(0x2c2dfdu, gp[esp] + 0x10u, 4, gp[ecx]);
        if (!flags.zf) {
            gp[eax] = gp[esi];
            port.store(0x2c2e05u, gp[esp] + 0x98u, 4, gp[eax]);
            port.store(0x2c2e0cu, gp[esp] + 0x94u, 4, gp[ebp]);
        }
    }

    void prepare_inputs() {
        gp[esi] = port.read(0x2c2e13u, gp[edi] + 0x10u, 4);
        gp[edx] = gp[esp] + 0x50u;
        port.push(0x2c2e1au, gp[edx]);
        port.push(0x2c2e1bu, gp[ebp]);
        port.push(0x2c2e1cu, gp[ebp]);
        port.push(0x2c2e1du, gp[eax]);
        gp[esi] = flags.logic(gp[esi] & 0x107f0u, 4);
        port.push(0x2c2e24u, gp[esi]);
        calls.template call_local<enumerate>(0x2c2e25u, 0x2c2e2au);
        gp[eax] = port.read(0x2c2e2au, gp[esp] + 0x94u, 4);
        gp[eax] = flags.logic(gp[eax] | gp[esi], 4);
        gp[esi] = gp[ebx] + 4u;
        port.store(0x2c2e36u, gp[esp] + 0x94u, 4, gp[eax]);
        gp[edi] = flags.logic(0u, 4);
        port.store(0x2c2e3fu, gp[esp] + 0x34u, 4, gp[esi]);
    }

    void cache_fixed_lookups() {
        for (;;) {
            gp[eax] = port.read(0x2c2e47u, gp[esp] + 0x10u, 4);
            flags.minus(gp[eax], gp[ebp], 4);
            if (flags.zf) break;
            gp[ebp] = 1u;
            gp[ecx] = gp[edi];
            gp[ebp] = flags.shift(gp[ebp], gp[ecx] & 0xffu, 4);
            flags.logic(gp[eax] & gp[ebp], 4);
            if (!flags.zf) {
                gp[ebx] = gp[esp] + 0x50u;
                calls.template call_local<fixed_lookup>(0x2c2e60u, 0x2c2e65u);
                gp[eax] = port.read(0x2c2e65u, gp[esp] + 0x10u, 4);
                gp[ebp] = ~gp[ebp];
                gp[eax] = flags.logic(gp[eax] & gp[ebp], 4);
                port.store(0x2c2e6du, gp[esp] + 0x2cu + gp[edi] * 4u, 4,
                           std::bit_cast<std::uint32_t>(_mm_cvtss_f32(port.s.xmm[0])));
                port.store(0x2c2e73u, gp[esp] + 0x10u, 4, gp[eax]);
            }
            gp[edi] = flags.increment(gp[edi], 4, false);
            gp[esi] = flags.plus(gp[esi], 0x14u, 4);
            flags.minus(gp[edi], 2u, 4);
            if (!flags.cf) break;
            gp[ebp] = flags.logic(0u, 4);
        }
    }

    Resume run() {
        resolve_inputs();
        prepare_inputs();
        cache_fixed_lookups();
        gp[eax] = port.read(0x2c2e80u, gp[esp] + 0x140u, 4);
        gp[ebx] = flags.logic(end_of_chain, 4);
        flags.minus(gp[eax], gp[ebx], 4);
        port.publish(0x2c2e8cu);
        return flags.zf ? Resume::epilogue : Resume::particle;
    }
};

template<class State>
struct Walk {
    world::Port<State> &port;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    enum class Particle { skip, age_countdown, collide };

    Particle next_particle() {
        gp[ebp] = port.read(particle, gp[esp] + next_slot, 4);
        gp[ecx] = port.read(0x2c2e96u, location::globals::particles, 4);
        gp[edx] = port.read(0x2c2e9cu, gp[ecx] + offsetof(native_effects::DataArray, data), 4);
        gp[ebp] = flags.logic(gp[ebp] & element_index, 4);
        gp[ebp] = flags.shift(gp[ebp], 6u, 4);
        const auto state = port.read(0x2c2ea8u, gp[ebp] + gp[edx] + offsetof(ParticleDatum, flags), 2);
        gp[eax] = (gp[eax] & 0xffff0000u) | state;
        gp[ebp] = flags.plus(gp[ebp], gp[edx], 4);
        flags.logic((gp[eax] & 0xffu) & (location::particle_flags::dead | location::particle_flags::resting), 1);
        gp[edx] = port.read(0x2c2eb1u, gp[ebp] + offsetof(ParticleDatum, next), 4);
        port.store(0x2c2eb4u, gp[esp] + next_slot, 4, gp[edx]);
        if (!flags.zf) return Particle::skip;

        const auto one = port.read(0x2c2ebeu, location::globals::one, 4);
        port.set_xmm(0, world::scalar(std::bit_cast<float>(one)));
        const auto age = port.read(0x2c2ec6u, gp[ebp] + offsetof(ParticleDatum, age), 4);
        flags.scalar_compare(port.s.xmm[0], world::scalar(std::bit_cast<float>(age)), false);
        if (flags.cf) return Particle::skip;

        gp[ecx] = (gp[ecx] & 0xffff0000u) | (gp[eax] & 0xffffu);
        const auto remaining = flags.right(gp[ecx] & 0xffffu, 13u, 2, false);
        gp[ecx] = (gp[ecx] & 0xffff0000u) | remaining;
        flags.logic(remaining, 2);
        if (!flags.zf) return Particle::age_countdown;
        port.publish(0x2c2edau);
        return Particle::collide;
    }

    void decrement_countdown() {
        gp[ecx] = flags.increment(gp[ecx], 4, true);
        gp[ecx] = flags.shift(gp[ecx], 13u, 4);
        gp[eax] = flags.logic(gp[eax] & 0x1fffu, 4);
        gp[ecx] = flags.logic(gp[ecx] | gp[eax], 4);
        port.store(0x2c3371u, gp[ebp] + offsetof(ParticleDatum, flags), 2, gp[ecx] & 0xffffu);
    }

    bool advance_particle() {
        gp[eax] = port.read(advance, gp[esp] + next_slot, 4);
        gp[ebx] = flags.logic(end_of_chain, 4);
        flags.minus(gp[eax], gp[ebx], 4);
        return !flags.zf;
    }

    Resume finish() {
        gp[edi] = port.pop(epilogue);
        gp[esi] = port.pop(0x2c3385u);
        gp[ebp] = port.pop(0x2c3386u);
        gp[ebx] = port.pop(0x2c3387u);
        gp[esp] = flags.plus(gp[esp], local_size, 4);
        port.ret(last, caller_args);
        port.publish(last);
        return Resume::returned;
    }

    Resume run(Start start) {
        if (start == Start::epilogue || (start == Start::advance && !advance_particle())) return finish();
        do {
            const auto status = next_particle();
            if (status == Particle::collide) return Resume::collision;
            if (status == Particle::age_countdown) decrement_countdown();
        } while (advance_particle());
        return finish();
    }
};

template<class State, class Ref, class Kernel>
Resume execute(State &state, std::span<const world::Function> sites, std::atomic<std::uint64_t> &counter,
               Ref reference, Kernel kernel) {
    world::Publication publication;
    if (reference_walk() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return Resume::retained;
    Resume result = Resume::retained;
    std::optional<Resume> replayed;
    world::execute(state, publication, counter, [&](auto &value) { replayed = reference(value); },
                   [&](auto &port) { result = kernel(port); });
    if (replayed && result != *replayed) throw std::runtime_error("Particle contact walk resumed at a different block");
    return result;
}

template<class State, class Ref, class Direct>
Resume bridge_setup(State &state, Ref reference, Direct direct) {
    return execute(state, setup_sites, summary().setup, reference, [&](auto &port) {
        return Setup<State, Direct>{port, {port, direct}}.run();
    });
}

template<class State, class Ref>
Resume bridge_walk(State &state, Start start, Ref reference) {
    return execute(state, walk_sites, summary().walk, [&](auto &value) { return reference(value, start); },
                   [&](auto &port) { return Walk<State>{port}.run(start); });
}
} // namespace halo2::native_particle_contact_walk_detail
