#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_particle_contact_walk.hpp"

namespace halo2::native_particle_contact_query_detail {
namespace walk = native_particle_contact_walk_detail;
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x2c2fd4u, query = 0x16c620u, call = 0x2c308bu;
inline constexpr std::uint32_t unconfirmed = 0x2c334au, missed = 0x2c3350u, last = 0x2c3364u;
inline constexpr std::uint32_t collision_scale = 0x46a124u, collision_limit = 0x56e0e8u;
inline constexpr auto position = offsetof(walk::ParticleDatum, position);
inline constexpr auto velocity = offsetof(walk::ParticleDatum, velocity);
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

enum class Resume { retained, response, advance };

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle contact query verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_query() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_CONTACT_QUERY");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Dispatch>
struct ContactQuery {
    world::Port<State> &port;
    Dispatch &dispatch;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;

    __m128 load(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void store(std::uint32_t site, std::uint32_t offset, __m128 value) {
        port.store(site, gp[esp] + offset, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value)));
    }

    void prepare_query() {
        using native_tu15_leaves::ordered_mul_ss;
        x[3] = load(first, gp[esp] + 0x144u);
        x[0] = load(0x2c2fddu, gp[ebp] + velocity);
        x[4] = load(0x2c2fe2u, collision_scale);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[1] = x[0];
        gp[ecx] = gp[ebp] + position;
        gp[edx] = port.read(0x2c2ff4u, gp[ecx], 4);
        port.store(0x2c2ff6u, gp[esp] + 0x20u, 4, gp[edx]);
        gp[edx] = port.read(0x2c2ffau, gp[ecx] + 4u, 4);
        gp[ecx] = port.read(0x2c2ffdu, gp[ecx] + 8u, 4);
        x[1] = ordered_mul_ss(x[1], x[4]);
        store(0x2c3004u, 0x44u, x[1]);
        x[1] = load(0x2c300au, gp[ebp] + velocity + 4u);
        x[1] = ordered_mul_ss(x[1], x[3]);
        port.store(0x2c3013u, gp[esp] + 0x24u, 4, gp[edx]);
        x[2] = ordered_mul_ss(x[1], x[4]);
        store(0x2c301eu, 0x48u, x[2]);
        x[2] = load(0x2c3024u, gp[ebp] + velocity + 8u);
        x[2] = ordered_mul_ss(x[2], x[3]);
        gp[edx] = gp[esp] + 0xb0u;
        port.push(0x2c3034u, gp[edx]);
        port.push(0x2c3035u, gp[ebx]);
        port.store(0x2c3036u, gp[esp] + 0x30u, 4, gp[ecx]);
        x[3] = ordered_mul_ss(x[2], x[4]);
        store(0x2c3041u, 0x54u, x[3]);
        x[3] = load(0x2c3047u, gp[esp] + 0x28u);
        x[3] = _mm_sub_ss(x[3], x[0]);
        x[0] = load(0x2c3051u, gp[esp] + 0x2cu);
        port.push(0x2c3057u, gp[ebx]);
        gp[ecx] = gp[esp] + 0x50u;
        x[0] = _mm_sub_ss(x[0], x[1]);
        port.push(0x2c3060u, gp[ecx]);
        gp[edx] = gp[esp] + 0x30u;
        store(0x2c3065u, 0x34u, x[0]);
        x[0] = load(0x2c306bu, gp[esp] + 0x38u);
        port.push(0x2c3071u, gp[edx]);
        x[0] = _mm_sub_ss(x[0], x[2]);
        port.push(0x2c3076u, gp[eax]);
        port.store(0x2c3077u, gp[esp] + 0xecu, 2, gp[ebx] & 0xffffu);
        store(0x2c307fu, 0x38u, x[3]);
        store(0x2c3085u, 0x40u, x[0]);
    }

    void trace_collision() {
        port.push(call, 0x2c3090u);
        auto child = [&](State &state) { dispatch(state, query, call); };
        try {
            port.call_child(query, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    Resume run() {
        prepare_query();
        trace_collision();
        flags.logic(gp[eax] & 0xffu, 1);
        const auto particle_flags = gp[ebp] + offsetof(walk::ParticleDatum, flags);
        if (flags.zf) {
            gp[eax] = flags.logic(0u, 4);
            gp[eax] = port.read(0x2c3352u, particle_flags, 2);
            gp[eax] = flags.logic(gp[eax] & 0x1ffdu, 4);
            gp[eax] = flags.logic(gp[eax] | 0xa000u, 4);
            port.store(0x2c3360u, particle_flags, 2, gp[eax] & 0xffffu);
            port.publish(last);
            return Resume::advance;
        }
        x[0] = load(0x2c3098u, collision_limit);
        flags.scalar_compare(x[0], load(0x2c30a0u, gp[esp] + 0xb4u), false);
        if (flags.cf || flags.zf) {
            const auto bits = port.read(unconfirmed, particle_flags + 1u, 1);
            port.store(unconfirmed, particle_flags + 1u, 1, flags.logic(bits & 0x1fu, 1));
            port.publish(0x2c334eu);
            return Resume::advance;
        }
        gp[esi] = port.read(0x2c30aeu, gp[esp] + 0x34u, 4);
        gp[ecx] = flags.logic(0u, 4);
        port.store(0x2c30b4u, gp[esp] + 0x14u, 4, gp[ecx]);
        port.publish(0x2c30b4u);
        return Resume::response;
    }
};

template<class State, class Ref, class Dispatch>
Resume bridge_query(State &state, Ref reference, Dispatch dispatch) {
    world::Publication publication;
    if (reference_query() || walk::reference_walk() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return Resume::retained;
    Resume result = Resume::retained;
    std::optional<Resume> replayed;
    world::execute(state, publication, summary().calls, [&](auto &value) { replayed = reference(value); },
                   [&](auto &port) { result = ContactQuery<State, Dispatch>{port, dispatch}.run(); });
    if (replayed && result != *replayed) throw std::runtime_error("Particle contact query resumed at a different block");
    return result;
}
} // namespace halo2::native_particle_contact_query_detail
