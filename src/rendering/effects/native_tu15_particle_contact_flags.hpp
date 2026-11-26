#pragma once

#include "rendering/effects/native_tu15_particle_contact_walk.hpp"

namespace halo2::native_particle_contact_flags_detail {
namespace walk = native_particle_contact_walk_detail;
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2c2ee0u, last = 0x2c2fcfu;
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};
inline constexpr std::uint32_t cached_mask = 0x94u, cached_system = 0x98u, cached_particle = 0xa0u;

enum class Resume { retained, motion, advance };

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle contact flags verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_flags() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CONTACT_FLAGS");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct>
struct ContactFlags {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    void update_particle_inputs() {
        gp[eax] = port.read(first, gp[esp] + cached_particle, 4);
        flags.minus(gp[ebp], gp[eax], 4);
        if (!flags.zf) {
            gp[ecx] = port.read(0x2c2eebu, gp[esp] + cached_mask, 4);
            gp[eax] = gp[ebp];
            gp[ecx] = flags.logic(gp[ecx] & 0xffff07f0u, 4);
            port.store(0x2c2efau, gp[esp] + cached_particle, 4, gp[eax]);
            port.store(0x2c2f01u, gp[esp] + cached_mask, 4, gp[ecx]);
        }
        gp[ecx] = port.read(0x2c2f08u, gp[esp] + 0x138u, 4);
        gp[esi] = port.read(0x2c2f0fu, gp[ecx] + 0x10u, 4);
        gp[ecx] = port.read(0x2c2f12u, gp[esp] + cached_system, 4);
        gp[edx] = gp[esp] + 0x50u;
        port.push(0x2c2f1du, gp[edx]);
        gp[edx] = port.read(0x2c2f1eu, gp[esp] + 0x98u, 4);
        port.push(0x2c2f25u, gp[eax]);
        gp[eax] = port.read(0x2c2f26u, gp[esp] + 0xa4u, 4);
        port.push(0x2c2f2du, gp[eax]);
        gp[edx] = ~gp[edx];
        gp[esi] = flags.logic(gp[esi] & 0xf80fu, 4);
        port.push(0x2c2f36u, gp[ecx]);
        gp[edx] = flags.logic(gp[edx] & gp[esi], 4);
        port.push(0x2c2f39u, gp[edx]);
        calls.template call_local<walk::enumerate>(0x2c2f3au, 0x2c2f3fu);
        gp[ecx] = port.read(0x2c2f3fu, gp[esp] + cached_mask, 4);
        gp[edx] = port.read(0x2c2f46u, gp[esp] + 0x148u, 4);
        gp[ecx] = flags.logic(gp[ecx] | gp[esi], 4);
        port.store(0x2c2f4fu, gp[esp] + cached_mask, 4, gp[ecx]);
    }

    void copy_collision_bit(std::uint32_t bit) {
        gp[eax] = flags.logic(flags.zf ? gp[eax] & ~bit : gp[eax] | bit, 4);
    }

    Resume run() {
        update_particle_inputs();
        gp[ecx] = flags.logic(gp[edx] & 2u, 4);
        gp[eax] = !flags.zf;
        gp[eax] = flags.plus(gp[eax], 0x800000u, 4);
        flags.logic(gp[ecx], 4);
        copy_collision_bit(4u);
        gp[edi] = flags.logic(gp[edx] & 4u, 4);
        copy_collision_bit(2u);
        flags.logic(gp[edx] & 0x10u, 1);
        copy_collision_bit(0x20u);
        gp[esi] = flags.logic(gp[edx] & 8u, 4);
        copy_collision_bit(0x400u);
        flags.logic(gp[edx] & 0x20u, 1);
        copy_collision_bit(0x10u);
        flags.logic(gp[eax] & 0x430u, 4);
        if (!flags.zf) gp[eax] = flags.logic(gp[eax] | 8u, 4);

        std::uint32_t exit_site{};
        if (flags.logic(gp[edx] & 1u, 1)) exit_site = 0x2c2fbdu;
        else if (flags.logic(gp[ecx], 4)) exit_site = 0x2c2fc1u;
        else if (flags.logic(gp[edi], 4)) exit_site = 0x2c2fc5u;
        else if (flags.logic(gp[esi], 4)) exit_site = 0x2c2fc9u;
        if (exit_site) {
            port.publish(exit_site);
            return Resume::motion;
        }
        const auto address = gp[ebp] + offsetof(walk::ParticleDatum, flags);
        const auto state = port.read(0x2c2fcbu, address, 1);
        port.store(0x2c2fcbu, address, 1, flags.logic(state & 0xfdu, 1));
        port.publish(last);
        return Resume::advance;
    }
};

template<class State, class Ref, class Direct>
Resume bridge_flags(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (reference_flags() || walk::reference_walk() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return Resume::retained;
    Resume result = Resume::retained;
    std::optional<Resume> replayed;
    world::execute(state, publication, summary().calls, [&](auto &value) { replayed = reference(value); },
                   [&](auto &port) { result = ContactFlags<State, Direct>{port, {port, direct}}.run(); });
    if (replayed && result != *replayed) throw std::runtime_error("Particle contact flags resumed at a different block");
    return result;
}
} // namespace halo2::native_particle_contact_flags_detail
