#pragma once

#include "rendering/effects/native_tu15_particle_transform_basis.hpp"
#include <cmath>

namespace halo2::native_particle_transform_projection_detail {
namespace basis = native_particle_transform_basis_detail;
namespace world = native_world_leaves_detail;
using basis::eax;
using basis::ebx;
using basis::ecx;
using basis::edi;
using basis::edx;
using basis::esi;
using basis::esp;
using basis::ebp;

inline constexpr std::uint32_t project = 0x250b50u, rotation = 0x250bf8u,
    apply_rotation = 0x250c0cu, rotate_basis = 0x250c11u, apply_basis = 0x250c17u,
    read_projected = 0x250c1cu, add_translation = 0x250c2eu, leave = 0x250c4du,
    last = 0x250c50u, done = 1u;
inline constexpr world::Function sites[]{{project, last, 0u, 0u}};
inline constexpr std::uint32_t epsilon = 0x46a0ecu;

template<class State>
class Projection {
    State &state;
    std::uint32_t *position;
    const bool census;
    decltype(State::gp) &gp = state.gp;
    decltype(State::flags) &flags = state.flags;
    decltype(State::xmm) &x = state.xmm;
    decltype(State::fp) &fp = state.fp;

    void at(std::uint32_t site) const {
        if (position) *position = site;
    }

    void recheck(std::uint32_t next) const {
        if (position) {
            const auto *cursor = state.live_instruction_cursor();
            if (!cursor || cursor->position != position ||
                (cursor->first_trace && *cursor->first_trace) ||
                (cursor->second_trace && *cursor->second_trace))
                throw basis::Resume{next};
            if (cursor != state.immutable_cursor.cursor &&
                (!state.has_literal_service_sites(*cursor) ||
                 (state.observe && !cursor->observation_only_stores &&
                  !native_particle_birth_observer_detail::qualified(state.observe))))
                throw basis::Resume{next};
        } else if (state.services.instruction &&
                   (!census || !native_census_detail::uncursored(state, project, last))) {
            throw basis::Resume{next};
        }
    }

    std::uint32_t word(std::uint32_t site, std::uint32_t address) {
        at(site);
        return physics_step_detail::r(state.memory, address, 4);
    }

    void read(std::uint32_t site, std::uint32_t next, unsigned dest, std::uint32_t address) {
        gp[dest] = word(site, address);
        recheck(next);
    }

    void move_scalar(std::uint32_t site, std::uint32_t next, unsigned dest, std::uint32_t address) {
        x[dest] = world::scalar(std::bit_cast<float>(word(site, address)));
        recheck(next);
    }

    void multiply(std::uint32_t site, std::uint32_t next, unsigned dest, std::uint32_t address) {
        x[dest] = native_tu15_leaves::ordered_mul_ss(x[dest], world::scalar(std::bit_cast<float>(word(site, address))));
        recheck(next);
    }

    void add(std::uint32_t site, std::uint32_t next, unsigned dest, std::uint32_t address) {
        x[dest] = world::ordered_add_ss(x[dest], world::scalar(std::bit_cast<float>(word(site, address))));
        recheck(next);
    }

    void load_extended(std::uint32_t site, std::uint32_t next, std::uint32_t address) {
        const auto bits = word(site, address);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        long double value;
        asm volatile("flds %1" : "=t"(value) : "m"(bits));
        fp.push(value);
#else
        fp.push(physics_step_detail::ext(std::bit_cast<float>(bits)));
#endif
        recheck(next);
    }

    void discard_comparison() {
        state.compare_and_pop(fp.st(1));
        fp.status &= ~0x200u;
        fp.pop();
    }

    void store_scalar(std::uint32_t site, std::uint32_t next, std::uint32_t address, unsigned src) {
        at(site);
        physics_step_detail::w(state.memory, state.observe, site, address, 4, native_tu15_leaves::low_bits(x[src]));
        recheck(next);
    }

    void save(std::uint32_t site, std::uint32_t next, std::uint32_t value) {
        at(site);
        basis::push_saved(state, site, value);
        recheck(next);
    }

    void restore(std::uint32_t site, std::uint32_t next, unsigned dest) {
        gp[dest] = word(site, gp[esp]);
        gp[esp] += 4u;
        recheck(next);
    }

    bool form_projection() {
        // Keep each scalar rounding step and the dot-product operand order.
        // A provider can alias the basis, vector and temporary stack lanes.
        move_scalar(0x250b50u, 0x250b55u, 0, gp[edx] + 0xcu);
        load_extended(0x250b55u, 0x250b5bu, gp[ecx] + 0xacu);
        multiply(0x250b5bu, 0x250b60u, 0, gp[eax] + 4u);
        fp.st(0) = std::fabs(fp.st(0));
        move_scalar(0x250b62u, 0x250b67u, 1, gp[edx] + 0x18u);
        multiply(0x250b67u, 0x250b6cu, 1, gp[eax] + 8u);
        move_scalar(0x250b6cu, 0x250b71u, 2, gp[edx] + 4u);
        multiply(0x250b71u, 0x250b75u, 2, gp[eax]);
        move_scalar(0x250b75u, 0x250b7au, 3, gp[edx] + 8u);
        multiply(0x250b7au, 0x250b7eu, 3, gp[eax]);
        x[0] = world::ordered_add_ss(x[0], x[1]);
        move_scalar(0x250b82u, 0x250b86u, 1, gp[edx]);
        multiply(0x250b86u, 0x250b8au, 1, gp[eax]);
        x[0] = world::ordered_add_ss(x[0], x[1]);
        move_scalar(0x250b8eu, 0x250b93u, 1, gp[edx] + 0x10u);
        multiply(0x250b93u, 0x250b98u, 1, gp[eax] + 4u);
        x[1] = world::ordered_add_ss(x[1], x[2]);
        move_scalar(0x250b9cu, 0x250ba1u, 2, gp[edx] + 0x1cu);
        multiply(0x250ba1u, 0x250ba6u, 2, gp[eax] + 8u);
        x[1] = world::ordered_add_ss(x[1], x[2]);
        move_scalar(0x250baau, 0x250bafu, 2, gp[edx] + 0x14u);
        multiply(0x250bafu, 0x250bb4u, 2, gp[eax] + 4u);
        x[2] = world::ordered_add_ss(x[2], x[3]);
        move_scalar(0x250bb8u, 0x250bbdu, 3, gp[edx] + 0x20u);
        multiply(0x250bbdu, 0x250bc2u, 3, gp[eax] + 8u);
        load_extended(0x250bc2u, 0x250bc8u, epsilon);
        x[2] = world::ordered_add_ss(x[2], x[3]);
        discard_comparison();
        store_scalar(0x250bd0u, 0x250bd6u, gp[esp] + 0xcu, 0);
        store_scalar(0x250bd6u, 0x250bdcu, gp[esp] + 0x10u, 1);
        store_scalar(0x250bdcu, 0x250be2u, gp[esp] + 0x14u, 2);
        if (flags.cf || flags.zf) return true;

        load_extended(0x250be4u, 0x250beau, gp[ecx] + 0xa8u);
        fp.st(0) = std::fabs(fp.st(0));
        load_extended(0x250becu, 0x250bf2u, epsilon);
        discard_comparison();
        return flags.cf || flags.zf;
    }

public:
    Projection(State &state, std::uint32_t *position)
        : state(state), position(position), census(!position && state.services.instruction &&
              native_census_detail::uncursored(state, project, last)) {}

    void finish() {
        gp[esp] = flags.plus(gp[esp], 0x3cu, 4);
        word(last, gp[esp]);
        gp[esp] += 4u;
    }

    void write_translation() {
        add(0x250c2eu, 0x250c32u, 0, gp[edi]);
        add(0x250c32u, 0x250c37u, 1, gp[esi] + 0x38u);
        add(0x250c37u, 0x250c3cu, 2, gp[esi] + 0x3cu);
        store_scalar(0x250c3cu, 0x250c40u, gp[edi], 0);
        restore(0x250c40u, 0x250c41u, edi);
        restore(0x250c41u, 0x250c42u, ebp);
        store_scalar(0x250c42u, 0x250c47u, gp[esi] + 0x38u, 1);
        store_scalar(0x250c47u, 0x250c4cu, gp[esi] + 0x3cu, 2);
        restore(0x250c4cu, leave, ebx);
        finish();
    }

    template<std::uint32_t entry>
    std::uint32_t run() {
        if constexpr (entry == project) {
            if (form_projection()) return rotation;
            write_translation();
        } else if constexpr (entry == rotation) {
            read(0x250bf8u, 0x250bfeu, eax, gp[ecx] + 0xacu);
            read(0x250bfeu, 0x250c04u, ecx, gp[ecx] + 0xa8u);
            save(0x250c04u, 0x250c06u, 0u);
            save(0x250c06u, 0x250c07u, gp[eax]);
            save(0x250c07u, 0x250c08u, gp[ecx]);
            gp[eax] = gp[esp] + 0x30u;
            return apply_rotation;
        } else if constexpr (entry == rotate_basis) {
            gp[eax] = gp[edx];
            gp[ecx] = gp[esp] + 0x24u;
            return apply_basis;
        } else if constexpr (entry == read_projected) {
            move_scalar(0x250c1cu, 0x250c22u, 2, gp[esp] + 0x14u);
            move_scalar(0x250c22u, 0x250c28u, 1, gp[esp] + 0x10u);
            move_scalar(0x250c28u, add_translation, 0, gp[esp] + 0xcu);
            write_translation();
        } else if constexpr (entry == add_translation) {
            write_translation();
        } else {
            static_assert(entry == leave);
            finish();
        }
        return done;
    }
};

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_TRANSFORM_PROJECTION");
        return value && *value;
    }();
    return enabled;
}

template<std::uint32_t entry, class State>
std::uint32_t bridge(State &state) {
    world::Publication publication;
    if (reference_mode() || (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return 0;
    if constexpr (entry == project) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        std::uint16_t control;
        asm volatile("fnstcw %0" : "=m"(control));
        if (state.active_mmx || state.fp.count > 6 || (control & 0x3fu) != 0x3fu) return 0;
#else
        return 0;
#endif
    }
    try {
        return Projection<State>(state, publication.cursor).template run<entry>();
    } catch (basis::Resume next) {
        return next.site;
    }
}
} // namespace halo2::native_particle_transform_projection_detail
