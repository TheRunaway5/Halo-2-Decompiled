#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_location_prepare_detail {
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
namespace location = native_effects::location_update;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x178110u, last = 0x178231u;
inline constexpr std::uint32_t allocate_location = 0x251930u, initialize_location = 0x2520a0u;
inline constexpr std::uint32_t enumerate = 0x176a40u, update_location = 0x251c80u;
inline constexpr std::uint32_t frame_size = 0x54u, argument_size = 12u;
inline constexpr std::uint32_t current_location = 8u, cached_mask = 0x50u;
inline constexpr std::uint32_t cached_system = 0x54u, cached_location = 0x58u;
inline constexpr std::uint32_t location_fields = 0x1ffffu;
inline constexpr auto array_data = offsetof(native_effects::DataArray, data);
inline constexpr world::Function sites[]{{first, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle location prepare verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_LOCATION_PREPARE");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct>
struct PrepareLocation {
    world::Port<State> &port;
    system::Step<State, Direct> calls;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    void open_frame() {
        gp[esp] = flags.minus(gp[esp], frame_size, 4);
        const auto address = gp[ebx] + 0xcu;
        const auto state = port.read(0x178113u, address, 2);
        port.store(0x178113u, address, 2, flags.logic(state | 0x101u, 2));
        port.push(0x178119u, gp[ebp]);
        gp[ebp] = port.read(0x17811au, gp[esp] + 0x5cu, 4);
        port.push(0x17811eu, gp[esi]);
        gp[esi] = port.read(0x17811fu, gp[ebp] + current_location, 4);
        gp[edx] = flags.logic(0u, 4);
        gp[eax] = flags.logic(0u, 4);
        gp[ecx] = flags.logic(0u, 4);
        flags.minus(gp[esi], system::none, 4);
        port.push(0x17812bu, gp[edi]);
        port.store(0x17812cu, gp[esp] + cached_mask, 4, gp[eax]);
        port.store(0x178130u, gp[esp] + cached_system, 4, gp[ecx]);
        port.store(0x178134u, gp[esp] + cached_location, 4, gp[edx]);
        port.store(0x178138u, gp[esp] + 0x5cu, 4, gp[edx]);
    }

    bool create_location() {
        calls.template call_local<allocate_location>(0x17813eu, 0x178143u);
        flags.minus(gp[eax], system::none, 4);
        port.store(0x178146u, gp[ebp] + current_location, 4, gp[eax]);
        if (flags.zf) return false;
        gp[ecx] = port.read(0x17814fu, system::location_array, 4);
        gp[edx] = port.read(0x178155u, gp[ecx] + array_data, 4);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.product(gp[eax], sizeof(location::LocationDatum));
        gp[eax] = flags.plus(gp[eax], gp[edx], 4);
        gp[edx] = gp[ebx] + 0x34u;
        gp[esi] = gp[eax];
        port.push(0x178167u, gp[edx]);
        gp[edi] = gp[ebx] + 0x30u;
        calls.template call_local<initialize_location>(0x17816bu, 0x178170u);
        gp[eax] = port.read(0x178170u, gp[esp] + 0x68u, 4);
        gp[eax] = flags.plus(gp[eax], 0x28u, 4);
        gp[ecx] = port.read(0x178177u, gp[eax], 4);
        gp[esi] = flags.plus(gp[esi], offsetof(location::LocationDatum, position), 4);
        port.store(0x17817cu, gp[esi], 4, gp[ecx]);
        gp[edx] = port.read(0x17817eu, gp[eax] + 4u, 4);
        port.store(0x178181u, gp[esi] + 4u, 4, gp[edx]);
        gp[eax] = port.read(0x178184u, gp[eax] + 8u, 4);
        port.store(0x178187u, gp[esi] + 8u, 4, gp[eax]);
        gp[edx] = port.read(0x17818au, gp[esp] + cached_location, 4);
        gp[ecx] = port.read(0x17818eu, gp[esp] + cached_system, 4);
        gp[eax] = port.read(0x178192u, gp[esp] + cached_mask, 4);
        return true;
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void prepare_update() {
        auto &value = port.s.xmm[0];
        value = scalar(0x178196u, gp[ebp] + 4u);
        value = world::ordered_add_ss(value, scalar(0x17819bu, gp[ebp]));
        value = native_tu15_leaves::ordered_mul_ss(value, scalar(0x1781a0u, gp[ebx] + 8u));
        gp[edi] = port.read(0x1781a5u, system::location_array, 4);
        port.store(0x1781abu, gp[ebx] + 4u, 4, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value)));
        gp[esi] = port.read(0x1781b0u, gp[ebp] + current_location, 4);
        gp[esi] = flags.logic(gp[esi] & 0xffffu, 4);
        gp[esi] = flags.product(gp[esi], sizeof(location::LocationDatum));
        gp[esi] = flags.plus(gp[esi], port.read(0x1781bcu, gp[edi] + array_data, 4), 4);
        flags.minus(gp[ebx], gp[ecx], 4);
        if (!flags.zf) {
            gp[ecx] = gp[ebx];
            gp[eax] = flags.logic(gp[eax] & 0xfffff98fu, 4);
            port.store(0x1781cau, gp[esp] + cached_system, 4, gp[ecx]);
            port.store(0x1781ceu, gp[esp] + cached_mask, 4, gp[eax]);
        }
        flags.minus(gp[esi], gp[edx], 4);
        if (!flags.zf) {
            gp[edx] = gp[esi];
            gp[eax] = flags.logic(gp[eax] & 0xfffecf7fu, 4);
            port.store(0x1781ddu, gp[esp] + cached_location, 4, gp[edx]);
            port.store(0x1781e1u, gp[esp] + cached_mask, 4, gp[eax]);
        }
    }

    void update_emitters() {
        gp[edi] = gp[esp] + 0xcu;
        port.push(0x1781e9u, gp[edi]);
        gp[edi] = port.read(0x1781eau, gp[esp] + 0x60u, 4);
        port.push(0x1781eeu, gp[edi]);
        port.push(0x1781efu, gp[edx]);
        gp[eax] = ~gp[eax];
        port.push(0x1781f2u, gp[ecx]);
        gp[eax] = flags.logic(gp[eax] & location_fields, 4);
        port.push(0x1781f8u, gp[eax]);
        calls.template call_local<enumerate>(0x1781f9u, 0x1781feu);
        gp[ecx] = port.read(0x1781feu, gp[esp] + cached_mask, 4);
        gp[eax] = port.read(0x178202u, gp[ebp] + 4u, 4);
        gp[ecx] = flags.logic(gp[ecx] | location_fields, 4);
        port.store(0x17820bu, gp[esp] + cached_mask, 4, gp[ecx]);
        gp[ecx] = port.read(0x17820fu, gp[esp] + 0x68u, 4);
        port.push(0x178213u, gp[ecx]);
        gp[edx] = gp[esp] + 0x10u;
        port.push(0x178218u, gp[edx]);
        port.push(0x178219u, gp[ebx]);
        port.push(0x17821au, gp[eax]);
        const auto mode = port.read(0x17821bu, gp[esp] + 0x7cu, 1);
        gp[eax] = (gp[eax] & 0xffffff00u) | mode;
        port.push(0x17821fu, gp[esi]);
        calls.template call_local<update_location>(0x178220u, 0x178225u);
        gp[ecx] = port.read(0x178225u, gp[esi] + system::next_location, 4);
        port.store(0x178228u, gp[ebp] + current_location, 4, gp[ecx]);
    }

    void run() {
        open_frame();
        if (!flags.zf || create_location()) {
            prepare_update();
            update_emitters();
        }
        gp[edi] = port.pop(0x17822bu);
        gp[esi] = port.pop(0x17822cu);
        gp[ebp] = port.pop(0x17822du);
        gp[esp] = flags.plus(gp[esp], frame_size, 4);
        port.ret(last, argument_size);
        port.publish(last);
    }
};

template<class State, class Ref, class Direct>
bool bridge(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (reference_mode() || state.active_mmx ||
        (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified))
        return false;
    world::execute(state, publication, summary().calls, reference, [&](auto &port) {
        PrepareLocation<State, Direct>{port, {port, direct}}.run();
    });
    return true;
}
} // namespace halo2::native_particle_location_prepare_detail
