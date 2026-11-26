#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_drain_counts_detail {
namespace system = native_particle_system_update_detail;
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using system::drain;
using system::first_location;
using system::location_array;
using system::next_location;
using system::none;
using world::eax;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;

inline constexpr std::uint32_t emitter_count = 0x252060u;
inline constexpr std::uint32_t last_locations = 0x178281u, last_emitters = 0x25209bu;
inline constexpr world::Function location_sites[]{{drain, last_locations, 0u, 0u}};
inline constexpr world::Function emitter_sites[]{{emitter_count, last_emitters, 0u, 0u}};
inline constexpr auto array_data = offsetof(native_effects::DataArray, data);

struct VerifySummary {
    std::atomic<std::uint64_t> locations{}, emitters{};
    ~VerifySummary() {
        if (locations || emitters)
            std::fprintf(stderr, "Particle drain counts verify: %llu locations, %llu emitters\n",
                         static_cast<unsigned long long>(locations.load()),
                         static_cast<unsigned long long>(emitters.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_counts() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_DRAIN_COUNTS");
        return value && *value;
    }();
    return enabled;
}

// Count particle totals for the system's linked list of locations.
template<class State, class Child>
void count_locations(world::Port<State> &port, Child &child) {
    auto &gp = port.gp;
    auto &flags = port.flags;
    port.push(0x178240u, gp[esi]);
    gp[esi] = port.read(0x178241u, gp[eax] + first_location, 4);
    port.push(0x178244u, gp[edi]);
    gp[edi] = flags.logic(0u, 4);
    flags.minus(gp[esi], none, 4);
    if (!flags.zf) {
        gp[ecx] = port.read(0x17824cu, location_array, 4);
        port.push(0x178252u, gp[ebx]);
        gp[ebx] = port.read(0x178253u, gp[ecx] + array_data, 4);
        do {
            gp[esi] = flags.logic(gp[esi] & 0xffffu, 4);
            gp[esi] = flags.product(gp[esi], sizeof(location::LocationDatum));
            gp[esi] = flags.plus(gp[esi], gp[ebx], 4);
            gp[eax] = gp[esi];
            port.push(0x17826du, 0x178272u);
            try {
                port.call_child(emitter_count, child);
            } catch (...) {
                gp = port.s.gp;
                flags = port.s.flags;
                throw;
            }
            gp[esi] = port.read(0x178272u, gp[esi] + next_location, 4);
            gp[edi] = flags.plus(gp[edi], gp[eax], 4);
            flags.minus(gp[esi], none, 4);
        } while (!flags.zf);
        gp[ebx] = port.pop(0x17827cu);
    }
    gp[eax] = gp[edi];
    gp[edi] = port.pop(0x17827fu);
    gp[esi] = port.pop(0x178280u);
    port.ret(last_locations, 0u);
    port.publish(last_locations);
}

// Count a location's particles from the unsigned emitter counts.
template<class State>
void count_emitters(world::Port<State> &port) {
    auto &gp = port.gp;
    auto &flags = port.flags;
    gp[ecx] = port.read(0x252060u, gp[eax] + offsetof(location::LocationDatum, first_emitter), 4);
    gp[eax] = flags.logic(0u, 4);
    flags.minus(gp[ecx], none, 4);
    if (!flags.zf) {
        gp[edx] = port.read(0x25206au, location::globals::emitters, 4);
        gp[edx] = port.read(0x252070u, gp[edx] + array_data, 4);
        port.push(0x252073u, gp[esi]);
        do {
            gp[ecx] = flags.logic(gp[ecx] & 0xffffu, 4);
            gp[ecx] = flags.product(gp[ecx], sizeof(location::EmitterDatum));
            gp[esi] = port.read(0x252089u, gp[ecx] + gp[edx] + offsetof(location::EmitterDatum, particle_count), 2);
            gp[ecx] = flags.plus(gp[ecx], gp[edx], 4);
            gp[ecx] = port.read(0x252090u, gp[ecx] + offsetof(location::EmitterDatum, next_emitter), 4);
            gp[eax] = flags.plus(gp[eax], gp[esi], 4);
            flags.minus(gp[ecx], none, 4);
        } while (!flags.zf);
        gp[esi] = port.pop(0x25209au);
    }
    port.ret(last_emitters, 0u);
    port.publish(last_emitters);
}

template<class State, class Ref, class Child>
bool bridge_locations(State &state, Ref reference, Child child) {
    world::Publication publication;
    if (reference_counts() || !system::admit(state, location_sites, publication)) return false;
    world::execute(state, publication, summary().locations, reference,
                   [&](auto &port) { count_locations(port, child); });
    return true;
}

template<class State, class Ref>
bool bridge_emitters(State &state, Ref reference) {
    world::Publication publication;
    if (reference_counts() || !system::admit(state, emitter_sites, publication)) return false;
    world::execute(state, publication, summary().emitters, reference,
                   [](auto &port) { count_emitters(port); });
    return true;
}
} // namespace halo2::native_particle_drain_counts_detail
