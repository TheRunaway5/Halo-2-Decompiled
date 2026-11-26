#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_location_update_detail {
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

inline constexpr std::uint32_t first = 0x17bb10u, last = 0x17bd23u;
inline constexpr std::uint32_t transform_source = 0x178020u, update_setup = 0x178920u;
inline constexpr std::uint32_t next_effect_location = 0x17dce0u, transform_location = 0x17ba50u;
inline constexpr std::uint32_t update_location = 0x178110u;
inline constexpr std::uint32_t scene = 0x4ecc18u, location_modes = 0x17bd28u;
inline constexpr std::uint32_t unbound = 0xffffu, absent_node = 0xffu;
inline constexpr std::uint32_t frame_size = 0x44u, argument_size = 20u;
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle location update verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_LOCATION_UPDATE");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct, class Dispatch, class Branch>
struct LocationUpdate {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    Dispatch &dispatch;
    Branch &resolve_branch;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;

    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }
    void low_byte(unsigned reg, std::uint32_t value) { gp[reg] = (gp[reg] & 0xffffff00u) | (value & 0xffu); }
    void low_word(unsigned reg, std::uint32_t value) { gp[reg] = (gp[reg] & 0xffff0000u) | (value & 0xffffu); }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void method(std::uint32_t site, std::uint32_t returned, std::uint32_t address) {
        const auto target = port.read(site, address, 4);
        auto child = [&](State &state) { dispatch(state, target, site); };
        calls.call(target, site, returned, child);
    }

    void resolve_inputs() {
        gp[esp] = flags.minus(gp[esp], frame_size, 4);
        gp[eax] = port.read(0x17bb13u, gp[esp] + 0x50u, 4);
        gp[eax] = port.read(0x17bb17u, gp[eax] + 8u, 4);
        port.push(0x17bb1au, gp[ebx]);
        port.push(0x17bb1bu, gp[ebp]);
        gp[ebp] = port.read(0x17bb1cu, gp[esp] + 0x50u, 4);
        flags.minus(port.read(0x17bb20u, gp[ebp] + 0x20u, 2), unbound, 2);
        gp[ecx] = port.read(0x17bb25u, gp[ebp] + 0x7cu + gp[eax] * 4u, 4);
        port.push(0x17bb29u, gp[esi]);
        gp[esi] = port.read(0x17bb2au, gp[esp] + 0x58u, 4);
        port.push(0x17bb2eu, gp[edi]);
        port.store(0x17bb2fu, gp[esp] + 0x58u, 4, gp[ecx]);
        if (!flags.zf) {
            gp[edx] = gp[ebp] + 0x1cu;
            port.push(0x17bb38u, gp[edx]);
            gp[ecx] = gp[esi];
            calls.template call_local<transform_source>(0x17bb3bu, 0x17bb40u);
        }
        gp[ecx] = port.read(0x17bb40u, gp[esi] + 0x30u, 4);
        gp[eax] = port.read(0x17bb43u, gp[ebp] + 0x104u, 4);
        gp[edi] = port.read(0x17bb49u, gp[ebp] + 0x108u, 4);
        port.store(0x17bb4fu, gp[esp] + 0x14u, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x[0])));
        x[0] = scalar(0x17bb55u, gp[esp] + 0x64u);
        port.store(0x17bb5bu, gp[esp] + 0x1cu, 4, gp[ecx]);
        gp[ecx] = gp[esi];
        port.store(0x17bb61u, gp[esp] + 0x10u, 4, gp[eax]);
        port.store(0x17bb65u, gp[esp] + 0x18u, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x[0])));
        calls.template call_local<system::resolve_system>(0x17bb6bu, 0x17bb70u);
        gp[edx] = port.read(0x17bb70u, gp[eax] + 4u, 4);
        calls.template call_local<system::resolve_interface>(0x17bb73u, 0x17bb78u);
        gp[esi] = gp[eax];
        gp[edx] = port.read(0x17bb7au, gp[esi], 4);
        gp[ecx] = gp[esi];
        method(0x17bb7eu, 0x17bb81u, gp[edx] + 0x48u);
        low_byte(ebx, gp[eax]);
        gp[eax] = port.read(0x17bb83u, gp[esi], 4);
        gp[ecx] = gp[esi];
        method(0x17bb87u, 0x17bb8au, gp[eax] + 0x44u);
        gp[ecx] = port.read(0x17bb8au, gp[esp] + 0x10u, 4);
        gp[esi] = port.read(0x17bb8eu, gp[esp] + 0x5cu, 4);
        port.push(0x17bb92u, gp[eax]);
        low_byte(eax, gp[ebx]);
        calls.template call_local<update_setup>(0x17bb95u, 0x17bb9au);
    }

    bool next_location(std::uint32_t site) {
        gp[edi] = port.read(site, gp[esp] + 0x60u, 4);
        gp[eax] = flags.logic(0u, 4);
        gp[eax] = port.read(site + 6u, gp[edi] + 0x12u, 2);
        gp[ecx] = gp[esp] + 0x58u;
        port.push(site + 0xeu, gp[eax]);
        port.push(site + 0xfu, gp[ecx]);
        port.push(site + 0x10u, gp[ebp]);
        calls.template call_local<next_effect_location>(site + 0x11u, site + 0x16u);
        gp[ebx] = gp[eax];
        flags.logic(gp[ebx], 4);
        return !flags.zf;
    }

    // Modes 1 and 2 share the same encoded node lookup and plane
    // comparison. Each mode publishes its own original load/store sites.
    bool node_matches_plane(std::uint32_t offset) {
        using native_tu15_leaves::ordered_mul_ss;
        using world::ordered_add_ss;
        low_word(eax, port.read(0x17bbcbu + offset, gp[ebp] + 0x20u, 2));
        low_byte(edx, flags.logic(0u, 1));
        flags.minus(gp[eax] & unbound, unbound, 2);
        if (flags.zf) return false;
        gp[edi] = port.read(0x17bbd7u + offset, gp[esi] + 0xa0u, 4);
        gp[eax] = flags.product(signed_word(gp[eax]), 0xb0u);
        gp[eax] = flags.plus(gp[eax], gp[edi], 4);
        low_byte(eax, port.read(0x17bbe8u + offset, gp[eax] + 0x70u, 1));
        flags.minus(gp[eax] & 0xffu, absent_node, 1);
        if (flags.zf) return false;
        gp[edi] = port.read(0x17bbefu + offset, gp[esi] + 0x68u, 4);
        gp[ecx] = flags.logic(0u, 4);
        gp[ecx] = flags.logic(gp[eax] & 0x7fu, 4) * 3u;
        flags.minus(port.read(0x17bbfcu + offset, gp[edi] + gp[ecx] * 8u + 2u, 2), unbound, 2);
        gp[ecx] = gp[edi] + gp[ecx] * 8u;
        if (flags.zf) return false;
        flags.logic(gp[eax] & 0xffu, 1);
        if (flags.sf) {
            x[0] = scalar(0x17bc0bu + offset, gp[ecx] + 0xcu);
            x[0] = ordered_mul_ss(x[0], scalar(0x17bc10u + offset, gp[ebx] + 0x38u));
            x[1] = scalar(0x17bc15u + offset, gp[ecx] + 8u);
            x[1] = ordered_mul_ss(x[1], scalar(0x17bc1au + offset, gp[ebx] + 0x34u));
            x[0] = ordered_add_ss(x[0], x[1]);
            x[1] = scalar(0x17bc23u + offset, gp[ecx] + 4u);
            x[1] = ordered_mul_ss(x[1], scalar(0x17bc28u + offset, gp[ebx] + 0x30u));
            x[0] = ordered_add_ss(x[0], x[1]);
            x[0] = _mm_sub_ss(x[0], scalar(0x17bc31u + offset, gp[ecx] + 0x10u));
            x[1] = _mm_setzero_ps();
            flags.scalar_compare(x[1], x[0], false);
            if (flags.cf || flags.zf) return false;
        }
        low_byte(edx, 1u);
        return true;
    }

    bool reaches_location() {
        gp[edx] = signed_word(port.read(0x17bbc0u, gp[edi] + 0xeu, 2));
        const auto route = port.read(0x17bbc4u, location_modes + gp[edx] * 4u, 4);
        switch (route) {
        case 0x17bcfcu: return false;
        case 0x17bcc2u: return true;
        case 0x17bbcbu:
            low_byte(eax, !node_matches_plane(0u));
            break;
        case 0x17bc47u:
            low_byte(eax, node_matches_plane(0x7cu));
            break;
        default:
            // Preserve the original rejection of a corrupted jump-table word.
            // All four admitted destinations are TU1.5 identity mappings.
            (void)resolve_branch(port.s, route);
            throw std::runtime_error("Required original solver dispatch target unavailable");
        }
        flags.logic(gp[eax] & 0xffu, 1);
        return !flags.zf;
    }

    void update_one_location() {
        gp[edx] = port.read(0x17bcc2u, gp[esp] + 0x68u, 4);
        port.push(0x17bcc6u, gp[edx]);
        gp[edi] = gp[esp] + 0x24u;
        gp[esi] = gp[ebp];
        calls.template call_local<transform_location>(0x17bccdu, 0x17bcd2u);
        low_word(ebx, port.read(0x17bcd2u, gp[ebx] + 2u, 2));
        flags.minus(gp[ebx] & unbound, unbound, 2);
        if (!flags.zf) flags.logic((gp[ebx] >> 8u) & 0xffu, 1);
        if (flags.sf && !flags.zf) low_byte(ecx, 1u);
        else low_byte(ecx, flags.logic(0u, 1));
        gp[ebx] = port.read(0x17bce6u, gp[esp] + 0x5cu, 4);
        port.push(0x17bceau, gp[ecx]);
        port.push(0x17bcebu, gp[eax]);
        gp[eax] = gp[esp] + 0x1cu;
        port.push(0x17bcf0u, gp[eax]);
        calls.template call_local<update_location>(0x17bcf1u, 0x17bcf6u);
        gp[esi] = port.read(0x17bcf6u, scene, 4);
    }

    void run() {
        resolve_inputs();
        if (next_location(0x17bb9au)) {
            gp[esi] = port.read(0x17bbbau, scene, 4);
            do {
                if (reaches_location()) update_one_location();
            } while (next_location(0x17bcfcu));
        }
        gp[edi] = port.pop(0x17bd1cu);
        gp[esi] = port.pop(0x17bd1du);
        gp[ebp] = port.pop(0x17bd1eu);
        gp[ebx] = port.pop(0x17bd1fu);
        gp[esp] = flags.plus(gp[esp], frame_size, 4);
        port.ret(last, argument_size);
        port.publish(last);
    }
};

template<class State, class Ref, class Direct, class Dispatch, class Branch>
bool bridge(State &state, Ref ref, Direct direct, Dispatch dispatch, Branch resolve_branch) {
    world::Publication publication;
    if (reference_mode() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return false;
    world::execute(state, publication, summary().calls, ref, [&](auto &port) {
        LocationUpdate<State, Direct, Dispatch, Branch>{port, {port, direct}, dispatch, resolve_branch}.run();
    });
    return true;
}
} // namespace halo2::native_particle_location_update_detail
