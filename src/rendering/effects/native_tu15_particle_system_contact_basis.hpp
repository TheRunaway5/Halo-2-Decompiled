#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_system_contact_basis_detail {
namespace system = native_particle_system_update_detail;
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebx;
using world::ecx;
using world::esp;
using native_tu15_leaves::ordered_mul_ss;

inline constexpr std::uint32_t first = 0x177a2fu, last = 0x177ad3u;
inline constexpr std::uint32_t normalize = 0x30bd0u, perpendicular = 0x11f9c0u;
inline constexpr std::uint32_t negative_one = 0x46a0dcu;
inline constexpr auto velocity = offsetof(native_effects::location_update::ParticleDatum, velocity);
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle system contact basis verify: %llu calls\n",
                               static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct>
struct ContactBasis {
    world::Port<State> &port;
    Direct &direct;
    std::array<std::uint32_t, 8> &gp = port.gp;
    decltype(port.s.xmm) &x = port.s.xmm;

    template<std::uint32_t target>
    void call(std::uint32_t site, std::uint32_t returned) {
        port.push(site, returned);
        auto child = [&](State &value) { direct(value, std::integral_constant<std::uint32_t, target>{}); };
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            port.flags = port.s.flags;
            throw;
        }
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void spill(std::uint32_t site, std::uint32_t offset, unsigned reg) {
        port.store(site, gp[esp] + offset, 4, native_tu15_leaves::low_bits(x[reg]));
    }

    void prepare_axes() {
        x[1] = scalar(first, gp[ebx] + velocity);
        x[0] = scalar(0x177a34u, negative_one);
        x[1] = ordered_mul_ss(x[1], x[0]);
        spill(0x177a40u, 0x40u, 1u);
        x[1] = scalar(0x177a46u, gp[ebx] + velocity + 4u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        spill(0x177a4fu, 0x44u, 1u);
        x[1] = scalar(0x177a55u, gp[ebx] + velocity + 8u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        gp[eax] = gp[esp] + 0x40u;
        spill(0x177a62u, 0x48u, 1u);
        call<normalize>(0x177a68u, 0x177a6du);
        gp[eax] = gp[esp] + 0x58u;
        gp[ecx] = gp[esp] + 0x40u;
        call<perpendicular>(0x177a75u, 0x177a7au);
    }

    void run() {
        prepare_axes();
        x[1] = scalar(0x177a7au, gp[esp] + 0x60u);
        x[0] = scalar(0x177a80u, gp[esp] + 0x5cu);
        x[3] = scalar(0x177a86u, gp[esp] + 0x48u);
        x[2] = scalar(0x177a8cu, gp[esp] + 0x44u);
        x[4] = ordered_mul_ss(x[1], x[2]);
        x[5] = ordered_mul_ss(x[0], x[3]);
        x[4] = _mm_sub_ss(x[4], x[5]);
        spill(0x177aa4u, 0x4cu, 4u);
        x[4] = scalar(0x177aaau, gp[esp] + 0x58u);
        x[5] = ordered_mul_ss(x[4], x[3]);
        x[3] = scalar(0x177ab7u, gp[esp] + 0x40u);
        x[1] = ordered_mul_ss(x[1], x[3]);
        x[5] = _mm_sub_ss(x[5], x[1]);
        spill(0x177ac5u, 0x50u, 5u);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[2] = ordered_mul_ss(x[2], x[4]);
        port.publish(last);
    }
};

template<class State, class Ref, class Direct>
bool bridge(State &state, Ref reference, Direct direct) {
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_SYSTEM_CONTACT_BASIS");
        return value && *value;
    }();
    world::Publication publication;
    if (ref || !system::admit(state, sites, publication)) return false;
    world::execute(state, publication, summary().calls, reference,
        [&](world::Port<State> &port) { ContactBasis{port, direct}.run(); });
    return true;
}
} // namespace halo2::native_particle_system_contact_basis_detail
