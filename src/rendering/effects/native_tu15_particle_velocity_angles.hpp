#pragma once

#include "rendering/effects/native_tu15_particle_velocity_properties.hpp"

namespace halo2::native_particle_velocity_angles_detail {
namespace properties = native_particle_velocity_properties_detail;
namespace world = native_world_leaves_detail;
namespace fp_math = physics_step_detail;
using world::eax;
using world::ebp;
using world::ecx;
using world::edi;
using world::esi;
using world::esp;
using properties::curve;
using properties::ordered_mul_ss;
using native_effects::location_update::ParticleDatum;

inline constexpr std::uint32_t spatial = 0x2524d4u, planar = 0x252597u;
inline constexpr std::uint32_t spatial_exit = 0x252592u, planar_exit = 0x252623u, random_state = 0x4f3cd8u;
inline constexpr std::uint32_t unsigned_adjustment = 0x46a0c8u, random_scale = 0x46a160u;
inline constexpr std::uint32_t angle_scale = 0x44923cu, spread_scale = 0x46a164u;
inline constexpr auto position = offsetof(ParticleDatum, position), velocity = offsetof(ParticleDatum, velocity);
inline constexpr world::Function spatial_sites[]{{spatial, spatial_exit, 0u, 0u}};
inline constexpr world::Function planar_sites[]{{planar, planar_exit, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> spatial{}, planar{};
    ~VerifySummary() {
        if (spatial || planar)
            std::fprintf(stderr, "Particle velocity angles verify: %llu spatial, %llu planar\n",
                         static_cast<unsigned long long>(spatial.load()),
                         static_cast<unsigned long long>(planar.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Trig, class Spill>
struct Angles {
    properties::Properties<State, Direct> &frame;
    Trig &trig;
    Spill &spill;
    world::Port<State> &port = frame.port;
    std::array<std::uint32_t, 8> &gp = frame.gp;
    typename world::Port<State>::Flags &flags = frame.flags;
    decltype(port.s.xmm) &x = frame.x;
    decltype(port.s.fp) &fp = port.s.fp;

    long double load(std::uint32_t site, std::uint32_t address) {
        return fp_math::ext(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void store_extended(std::uint32_t site, std::uint32_t address) {
        port.at(site);
        port.store(site, address, 4, spill(fp.st(0), fp.status));
    }

    void trig_top(std::uint32_t site, bool cosine) {
        port.at(site);
        std::uint16_t status;
        fp.st(0) = trig(fp.st(0), cosine, status);
        fp.status = (fp.status & ~0x600u) | (status & 0x600u);
    }

    // Both authored modes take their bearing from the same 16-bit RNG scale.
    void bearing(std::uint32_t site, std::uint32_t descriptor) {
        gp[ecx] = port.read(site, random_state, 4);
        gp[eax] = port.read(site + 6u, gp[ecx] + 4u, 4);
        gp[eax] = flags.product(gp[eax], 0x19660du);
        gp[eax] = flags.plus(gp[eax], 0x3c6ef35fu, 4);
        port.store(site + 0x14u, gp[ecx] + 4u, 4, gp[eax]);
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
        flags.logic(gp[eax], 4);
        port.store(site + 0x1cu, gp[esp] + 0x30u, 4, gp[eax]);
        fp.push(static_cast<long double>(fp_math::si(port.read(site + 0x20u, gp[esp] + 0x30u, 4))));
        if (flags.sf != flags.of) fp.st(0) = fp_math::add(fp.st(0), load(site + 0x26u, unsigned_adjustment));
        fp.st(0) = fp_math::mul(fp.st(0), load(site + 0x2cu, random_scale));
        gp[esi] = gp[ebp] + descriptor;
        const auto angle_site = site + (site == spatial ? 0x38u : 0x35u);
        fp.st(0) = fp_math::mul(fp.st(0), load(angle_site, angle_scale));
        store_extended(angle_site + 6u, gp[esp] + 0x30u);
        fp.pop();
        frame.calls.template call_local<curve>(angle_site + 10u, angle_site + 15u);
    }

    void spatial_vector() {
        bearing(spatial, 0x8cu);
        gp[ecx] = port.read(0x25251bu, random_state, 4);
        gp[eax] = port.read(0x252521u, gp[ecx] + 4u, 4);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x252524u, spread_scale));
        gp[eax] = flags.product(gp[eax], 0x19660du);
        gp[eax] = flags.plus(gp[eax], 0x3c6ef35fu, 4);
        port.store(0x252537u, gp[ecx] + 4u, 4, gp[eax]);
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
        flags.logic(gp[eax], 4);
        port.store(0x25253fu, gp[esp] + 0x14u, 4, gp[eax]);
        frame.store(0x252543u, gp[esp] + 0x18u, x[0]);
        fp.push(static_cast<long double>(fp_math::si(port.read(0x252549u, gp[esp] + 0x14u, 4))));
        if (flags.sf != flags.of) fp.st(0) = fp_math::add(fp.st(0), load(0x25254fu, unsigned_adjustment));
        fp.st(0) = fp_math::mul(fp.st(0), load(0x252555u, random_scale));
        gp[ebp] = gp[edi] + velocity;
        x[0] = _mm_setzero_ps();
        frame.store(0x252561u, gp[edi] + position, x[0]);
        fp.st(0) = fp_math::mul(fp.st(0), load(0x252566u, gp[esp] + 0x18u));
        frame.store(0x25256au, gp[edi] + position + 4u, x[0]);
        port.at(0x25256fu);
        fp.push(fp.st(0));
        trig_top(0x252571u, false);
        port.at(0x252573u);
        std::swap(fp.st(0), fp.st(1));
        trig_top(0x252575u, true);
        store_extended(0x252577u, gp[ebp]);
        fp.pop();
        fp.push(load(0x25257au, gp[esp] + 0x30u));
        trig_top(0x25257eu, false);
        fp.st(0) = fp_math::mul(fp.st(0), fp.st(1));
        store_extended(0x252582u, gp[edi] + velocity + 4u);
        fp.pop();
        fp.push(load(0x252585u, gp[esp] + 0x30u));
        trig_top(0x252589u, true);
        fp.st(0) = fp_math::mul(fp.st(0), fp.st(1));
        store_extended(0x25258du, gp[edi] + velocity + 8u);
        fp.pop();
        port.at(0x252590u);
        (void)fp.st(0);
        fp.pop();
        port.publish(spatial_exit);
    }

    void planar_vector() {
        bearing(planar, 0x7cu);
        fp.push(load(0x2525dbu, gp[esp] + 0x30u));
        trig_top(0x2525dfu, false);
        x[1] = _mm_setzero_ps();
        frame.store(0x2525e4u, gp[edi] + velocity, x[1]);
        frame.store(0x2525e9u, gp[edi] + position, x[1]);
        gp[ebp] = gp[edi] + velocity;
        store_extended(0x2525f1u, gp[esp] + 0x18u);
        store_extended(0x2525f5u, gp[edi] + velocity + 4u);
        fp.pop();
        x[1] = frame.scalar(0x2525f8u, gp[esp] + 0x18u);
        fp.push(load(0x2525feu, gp[esp] + 0x30u));
        x[1] = ordered_mul_ss(x[1], x[0]);
        trig_top(0x252606u, true);
        frame.store(0x252608u, gp[edi] + position + 4u, x[1]);
        store_extended(0x25260du, gp[esp] + 0x30u);
        x[1] = frame.scalar(0x252611u, gp[esp] + 0x30u);
        store_extended(0x252617u, gp[edi] + velocity + 8u);
        fp.pop();
        x[1] = ordered_mul_ss(x[1], x[0]);
        frame.store(0x25261eu, gp[edi] + position + 8u, x[1]);
        port.publish(planar_exit);
    }
};

template<bool Planar, class State, class Ref, class Direct, class Trig, class Spill>
bool bridge(State &s, Ref reference, Direct direct, Trig trig, Spill spill) {
    world::Publication publication;
    if (!properties::admit(s, Planar ? planar_sites : spatial_sites, publication)) return false;
    auto &counter = Planar ? summary().planar : summary().spatial;
    world::execute(s, publication, counter, reference, [&](auto &port) {
        properties::Properties<State, Direct> frame{port, {port, direct}};
        Angles<State, Direct, Trig, Spill> direction{frame, trig, spill};
        if constexpr (Planar) direction.planar_vector();
        else direction.spatial_vector();
    });
    return true;
}
} // namespace halo2::native_particle_velocity_angles_detail
