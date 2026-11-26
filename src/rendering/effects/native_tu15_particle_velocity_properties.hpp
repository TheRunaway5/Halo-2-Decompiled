#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_velocity_properties_detail {
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::edi;
using world::esi;
using world::esp;
using native_tu15_leaves::ordered_mul_ss;
using world::ordered_add_ss;

inline constexpr std::uint32_t entry = 0x252480u, choose_mode = 0x2524cdu;
inline constexpr std::uint32_t store_position_z = 0x252cc9u, normalize_velocity = 0x252cceu, last = 0x252d81u;
inline constexpr std::uint32_t curve = 0x24ffe0u, minimum_lifetime = 0x46a18cu, rotation_scale = 0x46a7f4u;
inline constexpr std::uint32_t zero = 0x46a0e8u, one = 0x46a0d0u;
inline constexpr std::uint32_t frame_size = 0x1cu, argument_size = 8u;
inline constexpr world::Function prefix_sites[]{{entry, 0x2524cau, 0u, 0u}};
inline constexpr world::Function suffix_sites[]{{store_position_z, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> prefixes{}, suffixes{};
    ~VerifySummary() {
        if (prefixes || suffixes)
            std::fprintf(stderr, "Particle velocity properties verify: %llu prefixes, %llu suffixes\n",
                         static_cast<unsigned long long>(prefixes.load()),
                         static_cast<unsigned long long>(suffixes.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_VELOCITY_PROPERTIES");
        return value && *value;
    }();
    return enabled;
}

template<class State>
bool admit(State &state, std::span<const world::Function> sites, world::Publication &publication) {
    return !reference_mode() && !state.active_mmx &&
           (!state.instruction_cursor || state.live_instruction_cursor()) &&
           native_required_owner_policy_detail::preserves_instruction_policy(state.services) &&
           world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified);
}

template<class State, class Direct>
struct Properties {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }
    void store(std::uint32_t site, std::uint32_t address, __m128 value) {
        port.store(site, address, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value)));
    }

    void prefix() {
        gp[esp] = flags.minus(gp[esp], frame_size, 4);
        port.push(0x252483u, gp[ebx]);
        port.push(0x252484u, gp[ebp]);
        gp[ebp] = port.read(0x252485u, gp[esp] + 0x28u, 4);
        port.push(0x252489u, gp[esi]);
        gp[ebx] = gp[eax];
        gp[esi] = gp[ebp] + 0x18u;
        calls.template call_local<curve>(0x25248fu, 0x252494u);
        x[1] = scalar(0x252494u, minimum_lifetime);
        flags.scalar_compare(x[0], x[1], false);
        if (flags.cf || flags.zf) x[0] = x[1];
        x[1] = _mm_div_ss(scalar(0x2524a4u, one), x[0]);
        gp[esi] = gp[ebp] + 0x38u;
        store(0x2524b3u, gp[edi] + 0xcu, x[1]);
        calls.template call_local<curve>(0x2524b8u, 0x2524bdu);
        x[0] = ordered_mul_ss(x[0], scalar(0x2524bdu, rotation_scale));
        store(0x2524c5u, gp[edi] + 0x34u, x[0]);
        gp[eax] = port.read(0x2524cau, gp[ebp] + 0x78u, 4);
        port.publish(0x2524cau);
    }

    void squared_speed() {
        x[2] = scalar(normalize_velocity, gp[ebp] + 4u);
        x[1] = scalar(0x252cd3u, gp[ebp] + 8u);
        x[0] = scalar(0x252cd8u, gp[ebp]);
        x[3] = ordered_mul_ss(x[2], x[2]);
        x[0] = ordered_mul_ss(x[0], x[0]);
        x[2] = x[1];
        x[0] = ordered_add_ss(x[0], x[3]);
        x[2] = ordered_mul_ss(x[2], x[1]);
        x[0] = ordered_add_ss(x[0], x[2]);
        flags.scalar_compare(x[0], scalar(0x252cf7u, zero), true);
        gp[eax] = (gp[eax] & 0xffff00ffu) | ((flags.bits() | 2u) & 0xffu) << 8u;
        flags.logic((gp[eax] >> 8u) & 0x44u, 1);
    }

    template<class Estimate>
    void normalize(Estimate &estimate) {
        store(0x252d04u, gp[esp] + 0x30u, x[0]);
        const auto input = port.read(0x252d0au, gp[esp] + 0x30u, 4);
        x[0] = _mm_move_ss(x[0], world::scalar(std::bit_cast<float>(estimate(input))));
        store(0x252d10u, gp[esp] + 0x18u, x[0]);
        x[0] = scalar(0x252d16u, gp[esp] + 0x18u);
        x[1] = x[0];
        x[1] = ordered_mul_ss(x[1], scalar(0x252d1fu, gp[ebp]));
        store(0x252d24u, gp[ebp], x[1]);
        x[1] = ordered_mul_ss(scalar(0x252d29u, gp[ebp] + 4u), x[0]);
        store(0x252d32u, gp[ebp] + 4u, x[1]);
        x[1] = ordered_mul_ss(scalar(0x252d37u, gp[ebp] + 8u), x[0]);
        store(0x252d40u, gp[ebp] + 8u, x[1]);
    }

    template<class Estimate>
    void suffix(bool store_first, Estimate &estimate) {
        if (store_first) store(store_position_z, gp[edi] + 0x24u, x[0]);
        squared_speed();
        if (flags.pf) normalize(estimate);
        gp[esi] = port.read(0x252d45u, gp[esp] + 0x2cu, 4);
        gp[esi] = flags.plus(gp[esi], 0x28u, 4);
        calls.template call_local<curve>(0x252d4cu, 0x252d51u);
        x[1] = ordered_mul_ss(scalar(0x252d51u, gp[ebp]), x[0]);
        store(0x252d5au, gp[ebp], x[1]);
        x[1] = ordered_mul_ss(scalar(0x252d5fu, gp[ebp] + 4u), x[0]);
        store(0x252d68u, gp[ebp] + 4u, x[1]);
        x[1] = scalar(0x252d6du, gp[ebp] + 8u);
        gp[esi] = port.pop(0x252d72u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        store(0x252d77u, gp[ebp] + 8u, x[1]);
        gp[ebp] = port.pop(0x252d7cu);
        gp[ebx] = port.pop(0x252d7du);
        gp[esp] = flags.plus(gp[esp], frame_size, 4);
        port.ret(last, argument_size);
        port.publish(last);
    }
};

template<class State, class Ref, class Direct>
bool bridge_prefix(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (!admit(state, prefix_sites, publication)) return false;
    world::execute(state, publication, summary().prefixes, reference, [&](auto &port) {
        Properties<State, Direct>{port, {port, direct}}.prefix();
    });
    return true;
}

template<class State, class Ref, class Direct, class Estimate>
bool bridge_suffix(State &state, bool store_first, Ref reference, Direct direct, Estimate estimate) {
    world::Publication publication;
    if (!admit(state, suffix_sites, publication)) return false;
    struct ReciprocalResult { std::uint32_t input, output; };
    std::optional<ReciprocalResult> reciprocal;
    bool replayed{};
    auto live = [&](std::uint32_t input) {
        const auto output = estimate(state, input);
        reciprocal = ReciprocalResult{input, output};
        return output;
    };
    auto observed = [&](std::uint32_t input) {
        if (!reciprocal || replayed || input != reciprocal->input)
            throw std::runtime_error("Particle velocity estimate replay differs from its live operand");
        replayed = true;
        return reciprocal->output;
    };
    world::execute(state, publication, summary().suffixes,
        [&](auto &replay) { reference(replay, store_first, observed); },
        [&](auto &port) { Properties<State, Direct>{port, {port, direct}}.suffix(store_first, live); });
    if (world::verify_mode() && reciprocal && !replayed)
        throw std::runtime_error("Particle velocity estimate omitted in replay");
    return true;
}
} // namespace halo2::native_particle_velocity_properties_detail
