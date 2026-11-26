#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_particle_contact_walk.hpp"

namespace halo2::native_particle_contact_response_detail {
namespace walk = native_particle_contact_walk_detail;
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2c30b8u, last = 0x2c3243u;
inline constexpr std::uint32_t local_transform = 0x2c4110u, contact_offset = 0x46a784u;
inline constexpr auto position = offsetof(walk::ParticleDatum, position);
inline constexpr auto velocity = offsetof(walk::ParticleDatum, velocity);
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle contact response verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_response() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CONTACT_RESPONSE");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct>
struct ContactResponse {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;

    __m128 load(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void store(std::uint32_t site, std::uint32_t address, __m128 value) {
        port.store(site, address, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value)));
    }

    void cache_response_properties() {
        for (;;) {
            gp[eax] = port.read(first, gp[esp] + 0x10u, 4);
            flags.logic(gp[eax], 4);
            if (flags.zf) return;
            gp[edi] = flags.shift(1u, gp[ecx] & 0xffu, 4);
            flags.logic(gp[eax] & gp[edi], 4);
            if (!flags.zf) {
                gp[ebx] = gp[esp] + 0x50u;
                calls.template call_local<walk::fixed_lookup>(0x2c30cfu, 0x2c30d4u);
                gp[eax] = port.read(0x2c30d4u, gp[esp] + 0x10u, 4);
                gp[ecx] = port.read(0x2c30d8u, gp[esp] + 0x14u, 4);
                gp[edi] = ~gp[edi];
                gp[eax] = flags.logic(gp[eax] & gp[edi], 4);
                store(0x2c30e0u, gp[esp] + 0x2cu + gp[ecx] * 4u, x[0]);
                port.store(0x2c30e6u, gp[esp] + 0x10u, 4, gp[eax]);
            }
            gp[ecx] = flags.increment(gp[ecx], 4, false);
            gp[esi] = flags.plus(gp[esi], 0x14u, 4);
            flags.minus(gp[ecx], 2u, 4);
            port.store(0x2c30f1u, gp[esp] + 0x14u, 4, gp[ecx]);
            if (!flags.cf) return;
        }
    }

    // Resolve the normal component of incoming velocity and keep its
    // projections for restitution after the contact transform.
    void prepare_impulse() {
        using world::ordered_add_ss;
        using native_tu15_leaves::ordered_mul_ss;
        x[0] = load(0x2c30f7u, gp[ebp] + velocity);
        x[2] = load(0x2c30fcu, gp[esp] + 0xe0u);
        x[1] = load(0x2c3105u, gp[esp] + 0xd8u);
        x[7] = load(0x2c310eu, gp[ebp] + velocity);
        gp[edx] = port.read(0x2c3113u, gp[esp] + 0xd4u, 4);
        x[3] = load(0x2c311au, walk::location::globals::one);
        x[3] = _mm_sub_ss(x[3], load(0x2c3122u, gp[esp] + 0x2cu));
        gp[esi] = gp[ebp] + velocity;
        x[0] = ordered_mul_ss(x[0], x[1]);
        x[4] = x[2];
        x[4] = ordered_mul_ss(x[4], load(0x2c3132u, gp[esi] + 8u));
        x[0] = ordered_add_ss(x[0], x[4]);
        x[4] = load(0x2c313bu, gp[esp] + 0xdcu);
        x[5] = x[4];
        x[5] = ordered_mul_ss(x[5], load(0x2c3147u, gp[esi] + 4u));
        x[0] = ordered_add_ss(x[0], x[5]);
        x[5] = ordered_mul_ss(x[0], x[1]);
        x[7] = _mm_sub_ss(x[7], x[5]);
        x[6] = x[0];
        store(0x2c315eu, gp[esp] + 0x38u, x[7]);
        x[7] = load(0x2c3164u, gp[esi] + 4u);
        x[6] = ordered_mul_ss(x[6], x[4]);
        x[0] = ordered_mul_ss(x[0], x[2]);
        x[7] = _mm_sub_ss(x[7], x[6]);
        store(0x2c3175u, gp[esp] + 0x3cu, x[7]);
        x[7] = load(0x2c317bu, gp[esi] + 8u);
        store(0x2c3180u, gp[esp] + 0xacu, x[0]);
        x[7] = _mm_sub_ss(x[7], x[0]);
    }

    void place_at_contact() {
        using world::ordered_add_ss;
        using native_tu15_leaves::ordered_mul_ss;
        x[0] = load(0x2c318du, contact_offset);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], load(0x2c3199u, gp[esp] + 0xb8u));
        gp[ebx] = gp[ebp] + position;
        x[4] = ordered_mul_ss(x[4], x[0]);
        x[4] = ordered_add_ss(x[4], load(0x2c31a9u, gp[esp] + 0xbcu));
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_add_ss(x[2], load(0x2c31b6u, gp[esp] + 0xc0u));
        port.push(0x2c31bfu, gp[edx]);
        gp[edi] = gp[esp] + 0xdcu;
        gp[eax] = gp[esi];
        gp[ecx] = gp[esp] + 0x110u;
        port.store(0x2c31d0u, gp[esp] + 0x134u, 2, 0xffffu);
        store(0x2c31dau, gp[ebx], x[1]);
        store(0x2c31deu, gp[ebp] + position + 4u, x[4]);
        store(0x2c31e3u, gp[ebp] + position + 8u, x[2]);
        calls.template call_local<local_transform>(0x2c31e8u, 0x2c31edu);
    }

    void publish_new_velocity() {
        using native_tu15_leaves::ordered_mul_ss;
        x[1] = load(0x2c31edu, gp[esp] + 0x30u);
        x[0] = load(0x2c31f3u, gp[esp] + 0x38u);
        flags.logic(port.read(0x2c31f9u, gp[ebp] + offsetof(walk::ParticleDatum, flags), 1) & 2u, 1);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[5] = ordered_mul_ss(x[5], x[1]);
        x[0] = _mm_sub_ss(x[0], x[5]);
        store(0x2c3209u, gp[esi], x[0]);
        x[0] = load(0x2c320du, gp[esp] + 0x3cu);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[6] = ordered_mul_ss(x[6], x[1]);
        x[0] = _mm_sub_ss(x[0], x[6]);
        store(0x2c321fu, gp[ebp] + velocity + 4u, x[0]);
        x[0] = load(0x2c3224u, gp[esp] + 0xacu);
        x[7] = ordered_mul_ss(x[7], x[3]);
        x[3] = ordered_mul_ss(x[3], load(0x2c3231u, gp[ebp] + offsetof(walk::ParticleDatum, rotation_rate)));
        x[0] = ordered_mul_ss(x[0], x[1]);
        x[7] = _mm_sub_ss(x[7], x[0]);
        store(0x2c323eu, gp[ebp] + velocity + 8u, x[7]);
        store(last, gp[ebp] + offsetof(walk::ParticleDatum, rotation_rate), x[3]);
        port.publish(last);
    }
};

template<class State, class Ref, class Direct>
bool bridge_response(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (reference_response() || walk::reference_walk() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return false;
    world::execute(state, publication, summary().calls, reference, [&](auto &port) {
        ContactResponse<State, Direct> response{port, {port, direct}};
        response.cache_response_properties();
        response.prepare_impulse();
        response.place_at_contact();
        response.publish_new_velocity();
    });
    return true;
}
} // namespace halo2::native_particle_contact_response_detail
