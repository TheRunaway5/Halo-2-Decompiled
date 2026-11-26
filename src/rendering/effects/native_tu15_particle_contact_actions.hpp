#pragma once

#include "rendering/effects/native_tu15_particle_contact_walk.hpp"

namespace halo2::native_particle_contact_actions_detail {
namespace walk = native_particle_contact_walk_detail;
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2c3248u, last = 0x2c3348u;
inline constexpr std::uint32_t notify_sound = 0x250310u, notify_footstep = 0x250380u, notify_effect = 0x2502b0u;
inline constexpr std::uint32_t sound_group = 0x736e6421u, footstep_group = 0x666f6f74u, effect_group = 0x65666665u;
inline constexpr std::uint32_t interface_slot = 0x18u, contact_kind = 0xb0u;
inline constexpr std::uint32_t landing_normal = 0x456910u;
inline constexpr auto particle_flags = offsetof(walk::ParticleDatum, flags);
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle contact actions verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_actions() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CONTACT_ACTIONS");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct, class Dispatch>
struct ContactActions {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    Dispatch &dispatch;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    void method(std::uint32_t site, std::uint32_t returned, std::uint32_t address) {
        const auto target = port.read(site, address, 4);
        auto child = [&](State &state) { dispatch(state, target, site); };
        calls.call(target, site, returned, child);
    }

    std::uint32_t attachment_notification() {
        constexpr std::uint32_t generic = 0x2c32d9u;
        gp[edi] = port.read(0x2c324eu, gp[esp] + 0x13cu, 4);
        gp[ecx] = gp[edi];
        calls.template call_local<system::resolve_system>(0x2c3257u, 0x2c325cu);
        flags.minus(port.read(0x2c325cu, gp[eax] + 0x12u, 2), 1u, 2);
        gp[esi] = port.read(0x2c3261u, gp[esp] + interface_slot, 4);
        if (flags.zf) return generic;

        gp[eax] = port.read(0x2c3267u, gp[esi], 4);
        gp[ecx] = gp[esi];
        method(0x2c326bu, 0x2c326eu, gp[eax] + 4u);
        flags.minus(gp[eax], walk::end_of_chain, 4);
        if (flags.zf) return generic;

        gp[ecx] = gp[edi];
        calls.template call_local<system::resolve_system>(0x2c3275u, 0x2c327au);
        gp[edx] = port.read(0x2c327au, gp[esi], 4);
        gp[ebx] = port.read(0x2c327cu, gp[eax] + 4u, 4);
        gp[ecx] = gp[esi];
        method(0x2c3281u, 0x2c3284u, gp[edx] + 4u);
        gp[edi] = gp[eax];
        gp[eax] = port.read(0x2c3286u, gp[esi], 4);
        gp[ecx] = gp[esi];
        method(0x2c328au, 0x2c328cu, gp[eax]);
        flags.minus(gp[edi], walk::end_of_chain, 4);
        if (flags.zf) return generic;

        flags.minus(gp[eax], effect_group, 4);
        if (flags.zf) {
            port.push(0x2c32cau, gp[edi]);
            gp[ecx] = gp[esp] + 0x110u;
            gp[edi] = gp[ebp];
            calls.template call_local<notify_effect>(0x2c32d4u, generic);
            return generic;
        }
        flags.minus(gp[eax], footstep_group, 4);
        if (flags.zf) {
            port.push(0x2c32b4u, gp[edi]);
            port.push(0x2c32b5u, gp[ebx]);
            gp[edi] = gp[esp] + 0x114u;
            gp[ebx] = gp[ebp];
            calls.template call_local<notify_footstep>(0x2c32bfu, 0x2c32c4u);
            return 0x2c32c4u;
        }
        flags.minus(gp[eax], sound_group, 4);
        if (flags.zf) {
            port.push(0x2c32a6u, gp[edi]);
            gp[edi] = gp[ebp];
            calls.template call_local<notify_sound>(0x2c32a9u, 0x2c32aeu);
            return 0x2c32aeu;
        }
        return generic;
    }

    void record_landing() {
        const auto normal = port.read(0x2c32e3u, gp[esp] + 0xe0u, 4);
        port.set_xmm(0, world::scalar(std::bit_cast<float>(normal)));
        const auto threshold = port.read(0x2c32ecu, landing_normal, 4);
        flags.scalar_compare(port.s.xmm[0], world::scalar(std::bit_cast<float>(threshold)), false);
        const auto address = gp[ebp] + particle_flags;
        if (flags.cf || flags.zf) {
            const auto state = port.read(0x2c32fbu, address, 1);
            port.store(0x2c32fbu, address, 1, flags.logic(state & 0xefu, 1));
        } else {
            const auto state = port.read(0x2c32f5u, address, 1);
            port.store(0x2c32f5u, address, 1, flags.logic(state | 0x10u, 1));
        }
    }

    std::uint32_t surface_method() {
        gp[eax] = port.read(0x2c32ffu, gp[esp] + contact_kind, 4);
        gp[edi] = 1u;
        flags.minus(gp[eax], gp[edi], 4);
        if (flags.zf) {
            gp[edx] = port.read(0x2c330fu, gp[esi], 4);
            gp[ecx] = gp[esi];
            method(0x2c3313u, 0x2c3316u, gp[edx] + 0x38u);
            flags.logic(gp[eax] & 0xffu, 1);
            if (!flags.zf) return 0u;
        }
        flags.minus(port.read(0x2c331au, gp[esp] + contact_kind, 4), 3u, 4);
        if (flags.zf) {
            gp[eax] = port.read(0x2c3324u, gp[esi], 4);
            gp[ecx] = gp[esi];
            method(0x2c3328u, 0x2c332bu, gp[eax] + 0x38u);
            flags.logic(gp[eax] & 0xffu, 1);
            if (!flags.zf) return 0u;
        }
        flags.minus(port.read(0x2c332fu, gp[esp] + contact_kind, 4), 2u, 4);
        if (!flags.zf) return 0x2c3337u;
        gp[edx] = port.read(0x2c3339u, gp[esi], 4);
        gp[ecx] = gp[esi];
        method(0x2c333du, 0x2c3340u, gp[edx] + 0x3cu);
        flags.logic(gp[eax] & 0xffu, 1);
        return flags.zf ? 0x2c3342u : 0u;
    }

    void run() {
        if (flags.zf) {
            const auto site = attachment_notification();
            const auto address = gp[ebp] + particle_flags;
            const auto state = port.read(site, address, 1);
            port.store(site, address, 1, flags.logic(state | 2u, 1));
        } else {
            gp[esi] = port.read(0x2c32dfu, gp[esp] + interface_slot, 4);
        }
        record_landing();
        auto exit = surface_method();
        if (!exit) {
            constexpr std::uint32_t site = 0x2c3344u;
            const auto address = gp[ebp] + particle_flags;
            const auto state = port.read(site, address, 2);
            port.store(site, address, 2, flags.logic(state | (gp[edi] & 0xffffu), 2));
            exit = last;
        }
        port.publish(exit);
    }
};

template<class State, class Ref, class Direct, class Dispatch>
bool bridge_actions(State &state, Ref reference, Direct direct, Dispatch dispatch) {
    world::Publication publication;
    if (reference_actions() || walk::reference_walk() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return false;
    world::execute(state, publication, summary().calls, reference, [&](auto &port) {
        ContactActions<State, Direct, Dispatch>{port, {port, direct}, dispatch}.run();
    });
    return true;
}
} // namespace halo2::native_particle_contact_actions_detail
