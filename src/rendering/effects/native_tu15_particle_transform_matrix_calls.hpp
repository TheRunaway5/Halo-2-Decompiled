#pragma once

#include "rendering/effects/native_tu15_particle_transform_basis.hpp"

namespace halo2::native_particle_transform_matrix_calls_detail {
namespace basis = native_particle_transform_basis_detail;
namespace world = native_world_leaves_detail;

inline constexpr std::uint32_t rotate = 0x250c0cu, rotated = 0x250c11u;
inline constexpr std::uint32_t apply = 0x250c17u, applied = 0x250c1cu;

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_MATRIX_CALLS");
        return value && *value;
    }();
    return enabled;
}

template<std::uint32_t site, class State, class MatrixOwner>
bool call(State &state, MatrixOwner owner) {
    static_assert(site == rotate || site == apply);
    constexpr world::Function literal[]{{site, site, 0u, 0u}};
    world::Publication publication;
    if (reference_mode() || (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !world::admitted(state, literal, publication, native_particle_birth_observer_detail::qualified))
        return false;

    // CALL commits its return word before the callee sees any ABI. The local
    // owner runs on the live state and observes any trace changes from the
    // stack provider, including those that happen before its first read.
    if (publication.cursor) *publication.cursor = site;
    basis::push_saved(state, site, site == rotate ? rotated : applied);
    owner(state);
    return true;
}
} // namespace halo2::native_particle_transform_matrix_calls_detail
