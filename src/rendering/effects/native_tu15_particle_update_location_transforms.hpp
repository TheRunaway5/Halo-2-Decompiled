#pragma once

#include "rendering/effects/native_tu15_particle_transform_basis.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_update_location_transforms_detail {
namespace basis = native_particle_transform_basis_detail;
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using world::eax;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using world::ebp;

inline constexpr std::uint32_t first = 0x251f70u, last = 0x251fcau, done = 1u;
inline constexpr std::uint32_t none = 0xffffffffu;
inline constexpr std::uint32_t authored_emitter_count = 0x30u, authored_emitters = 0x34u, authored_emitter_stride = 0xb8u;
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

template<class State>
class UpdateTransforms {
    State &state;
    std::uint32_t *position;
    const bool census;
    decltype(State::gp) &gp = state.gp;
    decltype(State::flags) &flags = state.flags;

    void at(std::uint32_t site) const {
        if (position) *position = site;
    }

    void recheck(std::uint32_t next) const {
        if (position && state.instruction_cursor == state.immutable_cursor.cursor) {
            const auto &cursor = *state.instruction_cursor;
            if ((cursor.first_trace && *cursor.first_trace) || (cursor.second_trace && *cursor.second_trace))
                throw basis::Resume{next};
            return;
        }
        if (!position && !state.services.instruction) return;
        if (!position && (!census || !native_census_detail::uncursored(state,first,last)))
            throw basis::Resume{next};
        world::Publication current;
        if ((position && !state.live_instruction_cursor()) ||
            !world::admitted(state, sites, current, native_particle_birth_observer_detail::qualified) || current.cursor != position)
            throw basis::Resume{next};
    }

    std::uint32_t word(std::uint32_t site, std::uint32_t address, unsigned width = 4) {
        at(site);
        return physics_step_detail::r(state.memory,address,width);
    }

    void read(std::uint32_t site,std::uint32_t next,unsigned dest,std::uint32_t address) {
        gp[dest] = word(site,address);
        recheck(next);
    }

    void save(std::uint32_t site,std::uint32_t next,unsigned reg) {
        at(site);
        basis::push_saved(state,site,gp[reg]);
        recheck(next);
    }

    void restore(std::uint32_t site,std::uint32_t next,unsigned reg) {
        gp[reg] = word(site,gp[esp]);
        gp[esp] += 4u;
        recheck(next);
    }

    template<class Child>
    void invoke(std::uint32_t site,std::uint32_t next,Child &child) {
        at(site);
        basis::push_saved(state,site,next);
        child(state);
        recheck(next);
    }

    void publish_location_mode() {
        gp[ebp] = gp[eax];
        state.set(eax,word(0x251f80u,gp[esp]+0x18u,1),1);
        recheck(0x251f84u);
        gp[ebx] = flags.logic(0u,4);
        flags.minus(gp[esi],none,4);
        at(0x251f89u);
        physics_step_detail::w(state.memory,state.observe,0x251f89u,gp[edi]+2u,1,gp[eax]&0xffu);
        recheck(0x251f8cu);
    }

    template<class Transform>
    void update_emitters(Transform &transform) {
        gp[edi] = flags.logic(0u,4);
        do {
            // The authored count, pool backing and linked successor can all
            // change during a child call. Read each at its original use.
            const auto count = word(0x251f90u,gp[ebp]+authored_emitter_count);
            flags.minus(gp[ebx],count,4);
            recheck(0x251f93u);
            if (flags.sf == flags.of) break;
            read(0x251f95u,0x251f9bu,ecx,location::globals::emitters);
            read(0x251f9bu,0x251f9eu,edx,gp[ecx]+offsetof(native_effects::DataArray,data));
            read(0x251f9eu,0x251fa1u,ecx,gp[ebp]+authored_emitters);
            read(0x251fa1u,0x251fa5u,eax,gp[esp]+0x14u);
            gp[esi] = flags.logic(gp[esi]&0xffffu,4);
            gp[esi] = flags.product(gp[esi],sizeof(location::EmitterDatum));
            gp[esi] = flags.plus(gp[esi],gp[edx],4);
            gp[ecx] = flags.plus(gp[ecx],gp[edi],4);
            invoke(0x251fb2u,0x251fb7u,transform);
            read(0x251fb7u,0x251fbau,esi,gp[esi]+offsetof(location::EmitterDatum,next_emitter));
            gp[ebx] = flags.increment(gp[ebx],4,false);
            gp[edi] = flags.plus(gp[edi],authored_emitter_stride,4);
            flags.minus(gp[esi],none,4);
        } while (!flags.zf);
    }

public:
    UpdateTransforms(State &state,std::uint32_t *position)
        : state(state), position(position), census(!position && state.services.instruction &&
              native_census_detail::uncursored(state,first,last)) {}

    template<class Resolve,class Transform>
    void run(Resolve &resolve,Transform &transform) {
        save(first,0x251f71u,ebx);
        save(0x251f71u,0x251f72u,ebp);
        save(0x251f72u,0x251f73u,esi);
        save(0x251f73u,0x251f74u,edi);
        gp[edi] = gp[eax];
        read(0x251f76u,0x251f79u,esi,gp[edi]+offsetof(location::LocationDatum,first_emitter));
        invoke(0x251f79u,0x251f7eu,resolve);
        publish_location_mode();
        if (!flags.zf) update_emitters(transform);
        restore(0x251fc6u,0x251fc7u,edi);
        restore(0x251fc7u,0x251fc8u,esi);
        restore(0x251fc8u,0x251fc9u,ebp);
        restore(0x251fc9u,last,ebx);
        word(last,gp[esp]);
        gp[esp] += 12u;
    }
};

template<class State,class Resolve,class Transform>
std::uint32_t bridge(State &state,Resolve resolve,Transform transform) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_LOCATION_TRANSFORMS");
        return value && *value;
    }();
    world::Publication publication;
    if (reference || (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return 0;
    try {
        UpdateTransforms<State>(state,publication.cursor).run(resolve,transform);
        return done;
    } catch (basis::Resume next) {
        return next.site;
    }
}
} // namespace halo2::native_particle_update_location_transforms_detail
