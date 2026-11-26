#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_location_entry_detail {
namespace world = native_world_leaves_detail;
namespace layout = native_effects::location_update;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using layout::LocationDatum;
using layout::SystemDefinition;

inline constexpr std::uint32_t entry_site = 0x251c80u, forced_lod_site = 0x251e2bu;
inline constexpr std::uint32_t resolver = 0x178070u;
inline constexpr world::Function functions[]{{entry_site, forced_lod_site, 0u, 0u}};
enum class Resume { retained, observers, emitters };
enum Branch : unsigned {scan_observers, no_observers, force_lod, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle location entry verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Resolve>
struct Entry {
    world::Port<State> &port;
    Resolve &resolve;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    void resolve_definition() {
        gp[esp] = flags.minus(gp[esp], 0x14u, 4);
        gp[ecx] = port.read(0x251c83u, gp[esp] + 0x20u, 4);
        port.push(0x251c87u, gp[ebx]);
        port.push(0x251c88u, gp[ebp]);
        port.push(0x251c89u, gp[esi]);
        port.push(0x251c8au, gp[edi]);
        gp[edi] = port.read(0x251c8bu, gp[esp] + 0x28u, 4);
        gp[ebx] = (gp[ebx] & 0xffffff00u) | (gp[eax] & 0xffu);
        gp[eax] = port.read(0x251c91u, gp[edi] + offsetof(LocationDatum, first_emitter), 4);
        port.store(0x251c94u, gp[esp] + 0x1cu, 4, gp[eax]);
        port.push(0x251c98u, 0x251c9du);
        try {
            port.call_child(resolver, resolve);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    void publish_location() {
        gp[edx] = port.read(0x251c9du, gp[esp] + 0x38u, 4);
        port.s.xmm[3] = _mm_setzero_ps();
        gp[ebp] = gp[eax];
        gp[ecx] = port.read(0x251ca6u, gp[ebp] + offsetof(SystemDefinition, emitter_count), 4);
        gp[edx] = flags.plus(gp[edx], 0x28u, 4);
        const auto mode = gp[ebx] & 0xffu;
        flags.logic(mode, 1);
        gp[esi] = port.read(0x251caeu, gp[edx], 4);
        port.store(0x251cb0u, gp[esp] + 0x18u, 4, gp[ecx]);
        gp[ecx] = gp[edi] + offsetof(LocationDatum, position);
        gp[eax] = gp[ecx];
        port.store(0x251cb9u, gp[eax], 4, gp[esi]);
        gp[esi] = port.read(0x251cbbu, gp[edx] + 4u, 4);
        port.store(0x251cbeu, gp[eax] + 4u, 4, gp[esi]);
        gp[edx] = port.read(0x251cc1u, gp[edx] + 8u, 4);
        port.store(0x251cc4u, gp[esp] + 0x20u, 4, gp[ebp]);
        port.store(0x251cc8u, gp[eax] + 8u, 4, gp[edx]);
        port.store(0x251ccbu, gp[edi] + 2u, 1, mode);
    }

    Resume run() {
        resolve_definition();
        publish_location();
        if (!flags.zf) {
            const auto one = port.read(0x251e23u, layout::globals::one, 4);
            port.s.xmm[0] = world::scalar(std::bit_cast<float>(one));
            port.store(forced_lod_site, gp[edi] + offsetof(LocationDatum, lod), 4, one);
            port.publish(forced_lod_site);
            summary().coverage.fetch_or(1u << force_lod, std::memory_order_relaxed);
            return Resume::emitters;
        }

        gp[eax] = port.read(0x251cd4u, 0x4f54f0u, 4);
        gp[esi] = flags.logic(0u, 4);
        port.store(0x251cdbu, gp[edi] + offsetof(LocationDatum, lod), 4, 0u);
        flags.minus(port.read(0x251ce0u, gp[eax] + 8u, 2), gp[esi] & 0xffffu, 2);
        if (flags.zf || flags.sf != flags.of) {
            port.publish(0x251ce4u);
            summary().coverage.fetch_or(1u << no_observers, std::memory_order_relaxed);
            return Resume::emitters;
        }

        const auto one = port.read(0x251ceau, layout::globals::one, 4);
        port.s.xmm[2] = world::scalar(std::bit_cast<float>(one));
        gp[ebx] = 0x4f655cu;
        gp[edx] = flags.logic(0u, 4);
        port.publish(0x251cf9u);
        summary().coverage.fetch_or(1u << scan_observers, std::memory_order_relaxed);
        return Resume::observers;
    }
};

template<class State, class Reference, class Resolve>
Resume bridge(State &state, Reference reference, Resolve resolve) {
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_LOCATION_ENTRY");
        return value && *value;
    }();
    if (reference_mode || state.active_mmx) return Resume::retained;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return Resume::retained;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return Resume::retained;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return Resume::retained;
    Resume next = Resume::retained;
    auto replay = [&](State &original) {
        if (reference(original) != next)
            throw std::runtime_error("Native particle location entry resumed at a different continuation");
    };
    world::execute(state, publication, summary().calls, replay, [&](world::Port<State> &port) {
        next = Entry{port, resolve}.run();
    });
    return next;
}
} // namespace halo2::native_particle_location_entry_detail
