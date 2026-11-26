#pragma once

#include "rendering/effects/native_tu15_particle_velocity_properties.hpp"

namespace halo2::native_particle_velocity_direction_detail {
namespace properties = native_particle_velocity_properties_detail;
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebp;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using properties::curve;
using properties::ordered_mul_ss;
using native_effects::location_update::ParticleDatum;

inline constexpr std::uint32_t outward = 0x252628u, inward = 0x252672u, random_direction = 0x148de0u;
inline constexpr std::uint32_t outward_exit = 0x25266du, inward_exit = 0x2526e9u, random_state = 0x4f3cd8u;
inline constexpr auto position = offsetof(ParticleDatum, position);
inline constexpr auto velocity = offsetof(ParticleDatum, velocity);
inline constexpr world::Function outward_sites[]{{outward, outward_exit, 0u, 0u}};
inline constexpr world::Function inward_sites[]{{inward, inward_exit, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> outward{}, inward{};
    ~VerifySummary() {
        if (outward || inward)
            std::fprintf(stderr, "Particle velocity directions verify: %llu outward, %llu inward\n",
                         static_cast<unsigned long long>(outward.load()),
                         static_cast<unsigned long long>(inward.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Dispatch>
struct Direction {
    properties::Properties<State, Direct> &frame;
    Dispatch &dispatch;
    world::Port<State> &port = frame.port;
    std::array<std::uint32_t, 8> &gp = frame.gp;
    typename world::Port<State>::Flags &flags = frame.flags;
    decltype(port.s.xmm) &x = frame.x;

    void generate(std::uint32_t site, std::uint32_t returned) {
        auto child = [&](State &s) { dispatch(s, random_direction, site); };
        frame.calls.call(random_direction, site, returned, child);
    }

    void from_outward() {
        gp[esi] = gp[ebp] + 0x7cu;
        frame.calls.template call_local<curve>(0x25262bu, 0x252630u);
        gp[edx] = port.read(0x252630u, random_state, 4);
        gp[ebp] = gp[edi] + velocity;
        gp[edx] = flags.plus(gp[edx], 4u, 4);
        gp[eax] = gp[ebp];
        generate(0x25263eu, 0x252643u);
        x[1] = ordered_mul_ss(frame.scalar(0x252643u, gp[ebp]), x[0]);
        frame.store(0x25264cu, gp[edi] + position, x[1]);
        x[1] = ordered_mul_ss(frame.scalar(0x252651u, gp[edi] + velocity + 4u), x[0]);
        frame.store(0x25265au, gp[edi] + position + 4u, x[1]);
        x[1] = ordered_mul_ss(frame.scalar(0x25265fu, gp[edi] + velocity + 8u), x[0]);
        frame.store(0x252668u, gp[edi] + position + 8u, x[1]);
        port.publish(outward_exit);
    }

    void from_inward() {
        gp[edx] = port.read(inward, random_state, 4);
        gp[edx] = flags.plus(gp[edx], 4u, 4);
        gp[eax] = gp[esp] + 0x1cu;
        generate(0x25267fu, 0x252684u);
        gp[esi] = gp[ebp] + 0x7cu;
        frame.calls.template call_local<curve>(0x252687u, 0x25268cu);
        x[1] = frame.scalar(0x25268cu, gp[esp] + 0x1cu);
        x[2] = ordered_mul_ss(x[1], x[0]);
        frame.store(0x252699u, gp[edi] + position, x[2]);
        x[2] = frame.scalar(0x25269eu, gp[esp] + 0x20u);
        x[3] = ordered_mul_ss(x[2], x[0]);
        frame.store(0x2526abu, gp[edi] + position + 4u, x[3]);
        x[3] = frame.scalar(0x2526b0u, gp[esp] + 0x24u);
        x[4] = ordered_mul_ss(x[3], x[0]);
        x[0] = _mm_setzero_ps();
        frame.store(0x2526c0u, gp[edi] + position + 8u, x[4]);
        x[4] = _mm_sub_ss(x[0], x[1]);
        x[1] = x[0];
        gp[ebp] = gp[edi] + velocity;
        x[1] = _mm_sub_ss(x[1], x[2]);
        x[0] = _mm_sub_ss(x[0], x[3]);
        frame.store(0x2526dau, gp[ebp], x[4]);
        frame.store(0x2526dfu, gp[edi] + velocity + 4u, x[1]);
        frame.store(0x2526e4u, gp[edi] + velocity + 8u, x[0]);
        port.publish(inward_exit);
    }
};

template<bool Inward, class State, class Ref, class Direct, class Dispatch>
bool bridge(State &s, Ref reference, Direct direct, Dispatch dispatch) {
    world::Publication publication;
    if (!properties::admit(s, Inward ? inward_sites : outward_sites, publication)) return false;
    auto &counter = Inward ? summary().inward : summary().outward;
    world::execute(s, publication, counter, reference, [&](auto &port) {
        properties::Properties<State, Direct> frame{port, {port, direct}};
        Direction<State, Direct, Dispatch> direction{frame, dispatch};
        if constexpr (Inward) direction.from_inward();
        else direction.from_outward();
    });
    return true;
}
} // namespace halo2::native_particle_velocity_direction_detail
