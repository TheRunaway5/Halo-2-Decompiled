#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_birth_observer.hpp"

namespace halo2::native_particle_transform_basis_detail {
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using world::ebp;

inline constexpr std::uint32_t first = 0x250ac0u, transform = 0x250b50u, leave = 0x250c4du;
inline constexpr world::Function sites[]{{first, 0x250b4cu, 0u, 0u}};

template<class State>
void push_saved(State &state, std::uint32_t site, std::uint32_t value) {
    const auto address = state.gp[esp] - 4u;
    const std::array bytes{
        std::byte(value), std::byte(value >> 8), std::byte(value >> 16), std::byte(value >> 24)};
    if (state.observe) {
        SourcePhysicsContactRecordStore store{site, address, std::vector<std::byte>(4), {bytes.begin(), bytes.end()}};
        state.memory.read(address, store.before);
        state.memory.write(address, bytes);
        state.gp[esp] = address;
        state.observe(store);
    } else {
        state.memory.write(address, bytes);
        state.gp[esp] = address;
    }
}

struct Resume { std::uint32_t site; };

template<class State>
class BasisSetup {
    State &state;
    std::uint32_t *position;
    const bool census;
    decltype(State::gp) &gp = state.gp;
    decltype(State::flags) &flags = state.flags;

    void at(std::uint32_t site) const {
        if (position) *position = site;
    }

    void recheck(std::uint32_t next) const {
        if (position) {
            const auto *cursor = state.live_instruction_cursor();
            if (!cursor || cursor->position != position ||
                (cursor->first_trace && *cursor->first_trace) ||
                (cursor->second_trace && *cursor->second_trace))
                throw Resume{next};
            if (cursor != state.immutable_cursor.cursor &&
                (!state.has_literal_service_sites(*cursor) ||
                 (state.observe && !cursor->observation_only_stores &&
                  !native_particle_birth_observer_detail::qualified(state.observe))))
                throw Resume{next};
        } else if (state.services.instruction &&
                   (!census || !native_census_detail::uncursored(state, first, sites[0].last))) {
            throw Resume{next};
        }
    }

    void save(std::uint32_t site, std::uint32_t next, std::uint32_t value) {
        at(site);
        push_saved(state, site, value);
        recheck(next);
    }

    void read(std::uint32_t site, std::uint32_t next, unsigned destination, std::uint32_t address) {
        at(site);
        gp[destination] = physics_step_detail::r(state.memory, address, 4);
        recheck(next);
    }

    void store(std::uint32_t site, std::uint32_t next, std::uint32_t address, std::uint32_t value) {
        at(site);
        physics_step_detail::w(state.memory, state.observe, site, address, 4, value);
        recheck(next);
    }

    void copy_axes() {
        // Copy each coordinate before loading its neighbor. Either object can
        // alias the other or saved stack lanes in the original frame.
        gp[edi] = gp[eax] + 4u;
        read(0x250ad1u, 0x250ad3u, ebp, gp[edi]);
        gp[edx] = gp[esi] + 0x10u;
        gp[ebx] = gp[edx];
        store(0x250ad8u, 0x250adau, gp[ebx], gp[ebp]);
        read(0x250adau, 0x250addu, ebp, gp[edi] + 4u);
        store(0x250addu, 0x250ae0u, gp[ebx] + 4u, gp[ebp]);
        read(0x250ae0u, 0x250ae3u, edi, gp[edi] + 8u);
        store(0x250ae3u, 0x250ae6u, gp[ebx] + 8u, gp[edi]);

        gp[edi] = gp[eax] + 0x10u;
        read(0x250ae9u, 0x250aebu, ebp, gp[edi]);
        gp[ebx] = gp[esi] + 0x1cu;
        store(0x250aeeu, 0x250af0u, gp[ebx], gp[ebp]);
        read(0x250af0u, 0x250af3u, ebp, gp[edi] + 4u);
        store(0x250af3u, 0x250af6u, gp[ebx] + 4u, gp[ebp]);
        read(0x250af6u, 0x250af9u, edi, gp[edi] + 8u);
        store(0x250af9u, 0x250afcu, gp[ebx] + 8u, gp[edi]);

        gp[edi] = gp[eax] + 0x1cu;
        read(0x250affu, 0x250b01u, ebp, gp[edi]);
        gp[ebx] = gp[esi] + 0x28u;
        store(0x250b04u, 0x250b06u, gp[ebx], gp[ebp]);
        read(0x250b06u, 0x250b09u, ebp, gp[edi] + 4u);
        store(0x250b09u, 0x250b0cu, gp[ebx] + 4u, gp[ebp]);
        read(0x250b0cu, 0x250b0fu, edi, gp[edi] + 8u);
        store(0x250b0fu, 0x250b12u, gp[ebx] + 8u, gp[edi]);

        gp[eax] = flags.plus(gp[eax], 0x28u, 4);
        read(0x250b15u, 0x250b17u, ebp, gp[eax]);
        gp[edi] = gp[esi] + 0x34u;
        gp[ebx] = gp[edi];
        store(0x250b1cu, 0x250b1eu, gp[ebx], gp[ebp]);
        read(0x250b1eu, 0x250b21u, ebp, gp[eax] + 4u);
        store(0x250b21u, 0x250b24u, gp[ebx] + 4u, gp[ebp]);
        read(0x250b24u, 0x250b27u, eax, gp[eax] + 8u);
        store(0x250b27u, 0x250b2au, gp[ebx] + 8u, gp[eax]);
    }

    void preserve_shared_translation() {
        gp[eax] = gp[ecx] + 0x9cu;
        gp[ebx] = gp[esp] + 0xcu;
        flags.minus(gp[eax], gp[ebx], 4);
        if (!flags.zf) return;
        read(0x250b38u, 0x250b3au, ebx, gp[eax]);
        store(0x250b3au, 0x250b3eu, gp[esp] + 0x18u, gp[ebx]);
        read(0x250b3eu, 0x250b41u, ebx, gp[eax] + 4u);
        read(0x250b41u, 0x250b44u, eax, gp[eax] + 8u);
        store(0x250b44u, 0x250b48u, gp[esp] + 0x20u, gp[eax]);
        store(0x250b48u, 0x250b4cu, gp[esp] + 0x1cu, gp[ebx]);
        gp[eax] = gp[esp] + 0x18u;
    }

public:
    BasisSetup(State &state, std::uint32_t *position)
        : state(state), position(position), census(!position && state.services.instruction &&
              native_census_detail::uncursored(state, first, sites[0].last)) {}

    std::uint32_t run() {
        gp[esp] = flags.minus(gp[esp], 0x3cu, 4);
        flags.logic(gp[eax], 4);
        if (flags.zf) return leave;
        save(0x250acbu, 0x250accu, gp[ebx]);
        save(0x250accu, 0x250acdu, gp[ebp]);
        save(0x250acdu, 0x250aceu, gp[edi]);
        copy_axes();
        preserve_shared_translation();
        return transform;
    }
};

template<class State>
std::uint32_t bridge(State &state) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_TRANSFORM_BASIS");
        return value && *value;
    }();
    world::Publication publication;
    if (reference || (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return 0;
    try {
        return BasisSetup<State>(state, publication.cursor).run();
    } catch (Resume next) {
        return next.site;
    }
}
} // namespace halo2::native_particle_transform_basis_detail
