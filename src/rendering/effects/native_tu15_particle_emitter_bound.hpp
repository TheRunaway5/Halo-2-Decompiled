#pragma once

#include "rendering/effects/native_tu15_particle_transform_basis.hpp"

namespace halo2::native_particle_emitter_bound_detail {
namespace basis = native_particle_transform_basis_detail;
namespace world = native_world_leaves_detail;

inline constexpr std::uint32_t first = 0x22ad60u, last = 0x22ada4u, done = 1u;
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct Resume { std::uint32_t site; };

template<class State>
class Bound {
    State &state;
    std::uint32_t *position;
    const bool census;
    decltype(State::gp) &gp = state.gp;
    decltype(State::flags) &flags = state.flags;
    decltype(State::xmm) &x = state.xmm;

    void at(std::uint32_t site) const {
        if (position) *position = site;
    }

    void recheck(std::uint32_t next) const {
        if (position && state.instruction_cursor == state.immutable_cursor.cursor) {
            const auto &cursor = *state.instruction_cursor;
            if ((cursor.first_trace && *cursor.first_trace) || (cursor.second_trace && *cursor.second_trace))
                throw Resume{next};
            return;
        }
        if (!position && !state.services.instruction) return;
        if (!position && (!census || !native_census_detail::uncursored(state, first, last)))
            throw Resume{next};
        world::Publication live;
        if ((position && !state.live_instruction_cursor()) ||
            !world::admitted(state, sites, live, native_particle_birth_observer_detail::qualified) || live.cursor != position)
            throw Resume{next};
    }

    std::uint32_t read(std::uint32_t site, std::uint32_t address, unsigned width = 4) {
        at(site);
        return physics_step_detail::r(state.memory, address, width);
    }

    void load_scalar(std::uint32_t site, std::uint32_t next, unsigned dest, std::uint32_t address) {
        x[dest] = world::scalar(std::bit_cast<float>(read(site,address)));
        recheck(next);
    }

    void store_candidate(std::uint32_t site, std::uint32_t next, unsigned src) {
        at(site);
        physics_step_detail::w(state.memory, state.observe, site, gp[basis::esp], 4, native_tu15_leaves::low_bits(x[src]));
        recheck(next);
    }

public:
    Bound(State &state, std::uint32_t *position)
        : state(state), position(position), census(!position && state.services.instruction &&
              native_census_detail::uncursored(state, first, last)) {}

    void run() {
        auto &result = gp[basis::eax];
        auto &emitter = gp[basis::ecx];
        auto &stack = gp[basis::esp];
        at(first);
        basis::push_saved(state, first, emitter);
        recheck(0x22ad61u);
        result = read(0x22ad61u, emitter + 8u);
        recheck(0x22ad64u);
        result = flags.plus(result, 0x44u, 4);
        result = read(0x22ad67u, result + 4u);
        recheck(0x22ad6au);
        state.set(basis::ecx, read(0x22ad6au, result + 1u, 1), 1);
        recheck(0x22ad6du);
        const auto kind = flags.right(emitter & 0xffu, 4, 1, false);
        state.set(basis::ecx, kind, 1);
        flags.logic(kind, 1);
        if (kind == 0) load_scalar(0x22ad74u, 0x22ad79u, 1, result + 4u);
        else x[1] = _mm_setzero_ps();
        flags.logic(kind, 1);
        if (kind == 0) load_scalar(0x22ad82u, 0x22ad87u, 0, result + 8u);
        else load_scalar(0x22ad89u, 0x22ad91u, 0, 0x46a0d0u);

        // Unordered COMISS selects the right lane too. Both original writes
        // are observable, and the x87 return rereads the live stack word.
        flags.scalar_compare(x[1], x[0], false);
        store_candidate(0x22ad94u, 0x22ad99u, 1);
        if (flags.cf || flags.zf) store_candidate(0x22ad9bu, 0x22ada0u, 0);
        const auto bits = read(0x22ada0u, stack);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        long double value;
        asm volatile("flds %1" : "=t"(value) : "m"(bits));
        state.fp.push(value);
#else
        state.fp.push(physics_step_detail::ext(std::bit_cast<float>(bits)));
#endif
        recheck(0x22ada3u);
        emitter = read(0x22ada3u, stack);
        stack += 4u;
        recheck(0x22ada4u);
        read(last, stack);
        stack += 4u;
    }
};

template<class State>
std::uint32_t bridge(State &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_EMITTER_BOUND");
        return value && *value;
    }();
    world::Publication publication;
    if (reference || state.active_mmx || state.fp.count >= 8 ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return 0;
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    if ((control & 0x3fu) != 0x3fu) return 0;
    try {
        Bound<State>(state, publication.cursor).run();
        return done;
    } catch (Resume next) {
        return next.site;
    }
#else
    return 0;
#endif
}
} // namespace halo2::native_particle_emitter_bound_detail
