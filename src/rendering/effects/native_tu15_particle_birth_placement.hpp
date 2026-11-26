#pragma once

#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_birth_placement_detail {
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using location::EmitterDatum;
using location::ParticleDatum;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using world::ordered_add_ss;
using native_tu15_leaves::ordered_mul_ss;

inline constexpr std::uint32_t first = 0x250e67u, last = 0x251136u;
inline constexpr std::uint32_t prepare_inputs = 0x176a40u, evaluate_properties = 0x252480u;
inline constexpr std::uint32_t transform_vector = 0x145b70u, collision_query = 0x16c620u;
inline constexpr std::uint32_t input_mask = 0xf80fu, inputs = 0x44u;
inline constexpr std::uint32_t particle_definition = 0xc8u, origin = 0x14u;
inline constexpr std::uint32_t position = offsetof(ParticleDatum, position);
inline constexpr std::uint32_t velocity = offsetof(ParticleDatum, velocity);
inline constexpr std::uint32_t particle_flags = offsetof(ParticleDatum, flags);
inline constexpr std::uint32_t one = 0x46a0d0u, ray_plane_z = 0x46a0dcu, ray_offset = 0x46a784u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
enum Branch : unsigned {
    alternate_position, interpolated_position, default_origin, indexed_origin,
    collision_skipped, collision_hit, collision_miss, velocity_unchanged, velocity_offset, branch_count
};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle birth properties and placement verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

struct AxisSites {
    std::uint32_t load, add, store;
};

template<class State, class Route>
struct Placement {
    world::Port<State> &port;
    Route &route;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t{1} << branch; }

    template<std::uint32_t target>
    void call(std::uint32_t site, std::uint32_t returned) {
        port.push(site, returned);
        auto child = [&](State &state) { route(state, std::integral_constant<std::uint32_t, target>{}, site); };
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }


    void set_al(std::uint32_t low) { gp[eax] = (gp[eax] & 0xffffff00u) | (low & 0xffu); }
    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }
    void spill(std::uint32_t site, std::uint32_t address, unsigned reg) {
        port.store(site, address, 4, native_tu15_leaves::low_bits(x[reg]));
    }

    void evaluate() {
        gp[eax] = port.read(first, gp[esi] + 0x50u, 4);
        gp[ecx] = port.read(0x250e6au, gp[esi] + 0x4cu, 4);
        gp[edx] = port.read(0x250e6du, gp[esi] + 0x48u, 4);
        port.push(0x250e70u, gp[esi]);
        port.push(0x250e71u, gp[eax]);
        gp[eax] = port.read(0x250e72u, gp[esi] + inputs, 4);
        port.push(0x250e75u, gp[ecx]);
        gp[eax] = ~gp[eax];
        port.push(0x250e78u, gp[edx]);
        gp[eax] = flags.logic(gp[eax] & input_mask, 4);
        port.push(0x250e7eu, gp[eax]);
        call<prepare_inputs>(0x250e7fu, 0x250e84u);
        const auto evaluated = port.read(0x250e84u, gp[esi] + inputs, 4);
        port.store(0x250e84u, gp[esi] + inputs, 4, flags.logic(evaluated | input_mask, 4));
        gp[ecx] = port.read(0x250e8bu, gp[esp] + particle_definition, 4);
        port.push(0x250e92u, gp[edi]);
        port.push(0x250e93u, gp[ecx]);
        gp[edi] = gp[ebp];
        gp[eax] = gp[esi];
        call<evaluate_properties>(0x250e98u, 0x250e9du);
    }

    bool scale_position_and_age() {
        x[0] = scalar(0x250e9du, gp[esp] + 0xd8u);
        x[1] = scalar(0x250ea6u, gp[esp] + 0xd0u);
        x[5] = scalar(0x250eafu, gp[esp] + 0xd4u);
        gp[edi] = port.read(0x250eb8u, gp[esp] + 0x1cu, 4);
        x[2] = x[0];
        x[2] = ordered_mul_ss(x[2], scalar(0x250ebfu, gp[ebp] + position));
        x[3] = x[0];
        x[0] = ordered_mul_ss(x[0], scalar(0x250ec7u, gp[ebp] + position + 8u));
        x[3] = ordered_mul_ss(x[3], scalar(0x250eccu, gp[ebp] + position + 4u));
        x[4] = x[0];
        gp[esi] = gp[ebp] + position;
        x[0] = ordered_mul_ss(x[1], x[5]);
        x[0] = ordered_mul_ss(x[0], scalar(0x250edeu, gp[ebp] + offsetof(ParticleDatum, inverse_lifetime)));
        x[0] = ordered_add_ss(x[0], scalar(0x250ee3u, gp[ebp] + offsetof(ParticleDatum, age)));
        spill(0x250ee8u, gp[esi], 2u);
        spill(0x250eecu, gp[ebp] + position + 4u, 3u);
        spill(0x250ef1u, gp[ebp] + position + 8u, 4u);
        spill(0x250ef6u, gp[ebp] + offsetof(ParticleDatum, age), 0u);
        const auto placement_mode = port.read(0x250efbu, gp[edi] + 0xcu, 2);
        gp[eax] = (gp[eax] & 0xffff0000u) | placement_mode;
        flags.logic(placement_mode, 2);
        return flags.zf;
    }

    void resolve_origin() {
        flags.minus(port.read(0x250f08u, gp[esp] + origin, 4), 0xffffffffu, 4);
        if (flags.zf) {
            reached(default_origin);
            gp[edx] = port.read(0x250f38u, 0x474d04u, 4);
            port.store(0x250f3eu, gp[esp] + origin, 4, gp[edx]);
            return;
        }
        reached(indexed_origin);
        gp[edx] = port.read(0x250f0fu, gp[esp] + 0xc4u, 4);
        gp[eax] = port.read(0x250f16u, gp[edx] + 0x18u, 4);
        gp[ecx] = port.read(0x250f19u, 0x4f720cu, 4);
        gp[edx] = port.read(0x250f1fu, gp[ecx] + 0x44u, 4);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.product(gp[eax], 0x190u);
        gp[eax] = flags.plus(gp[eax], gp[edx], 4);
        gp[eax] = flags.plus(gp[eax], 0x30u, 4);
        port.store(0x250f32u, gp[esp] + origin, 4, gp[eax]);
    }

    void interpolate_emitter_basis() {
        x[5] = scalar(0x250f42u, one);
        x[2] = scalar(0x250f4au, gp[ebx] + 0x34u);
        x[3] = scalar(0x250f4fu, gp[ebx] + 0x40u);
        x[4] = scalar(0x250f54u, gp[ebx] + 0x38u);
        x[0] = _mm_sub_ss(x[5], x[1]);
        x[3] = ordered_mul_ss(x[3], x[0]);
        x[2] = ordered_mul_ss(x[2], x[1]);
        x[2] = ordered_add_ss(x[2], x[3]);
        x[3] = scalar(0x250f6cu, gp[ebx] + 0x44u);
        x[4] = ordered_mul_ss(x[4], x[1]);
        x[3] = ordered_mul_ss(x[3], x[0]);
        x[3] = ordered_add_ss(x[3], x[4]);
        x[4] = scalar(0x250f7du, gp[ebx] + 0x48u);
        x[4] = ordered_mul_ss(x[4], x[0]);
        x[0] = scalar(0x250f86u, gp[ebx] + 0x3cu);
        x[0] = ordered_mul_ss(x[0], x[1]);
        gp[edx] = gp[ebx] + 0x10u;
        gp[eax] = gp[esi];
        gp[ecx] = gp[esi];
        x[4] = ordered_add_ss(x[4], x[0]);
        call<transform_vector>(0x250f9au, 0x250f9fu);
        x[2] = ordered_add_ss(x[2], scalar(0x250f9fu, gp[esi]));
        x[3] = ordered_add_ss(x[3], scalar(0x250fa3u, gp[ebp] + position + 4u));
        x[4] = ordered_add_ss(x[4], scalar(0x250fa8u, gp[ebp] + position + 8u));
        gp[eax] = gp[ebp] + velocity;
        gp[ecx] = gp[eax];
        gp[edx] = gp[ebx] + 0x10u;
        spill(0x250fb5u, gp[esi], 2u);
        spill(0x250fb9u, gp[ebp] + position + 4u, 3u);
        spill(0x250fbeu, gp[ebp] + position + 8u, 4u);
        call<transform_vector>(0x250fc3u, 0x250fc8u);
    }

    bool query_collision() {
        x[0] = _mm_setzero_ps();
        gp[ecx] = gp[esi];
        gp[edx] = port.read(0x250fdeu, gp[ecx], 4);
        gp[eax] = port.read(0x250fe0u, gp[ecx] + 4u, 4);
        gp[ecx] = port.read(0x250fe3u, gp[ecx] + 8u, 4);
        port.store(0x250fe6u, gp[esp] + 0x24u, 4, gp[edx]);
        gp[edx] = gp[esp] + 0x3cu;
        port.push(0x250feeu, gp[edx]);
        port.push(0x250fefu, 0xffffffffu);
        port.push(0x250ff1u, 0xffffffffu);
        port.store(0x250ff3u, gp[esp] + 0x34u, 4, gp[eax]);
        port.store(0x250ff7u, gp[esp] + 0x38u, 4, gp[ecx]);
        spill(0x250ffbu, gp[esp] + 0x3cu, 0u);
        spill(0x251001u, gp[esp] + 0x40u, 0u);
        x[0] = scalar(0x251007u, ray_plane_z);
        gp[eax] = gp[esp] + 0x3cu;
        port.push(0x251013u, gp[eax]);
        gp[ecx] = gp[esp] + 0x34u;
        spill(0x251018u, gp[esp] + 0x48u, 0u);
        x[0] = scalar(0x25101eu, gp[esp] + 0x3cu);
        port.push(0x251024u, gp[ecx]);
        x[0] = ordered_add_ss(x[0], x[5]);
        port.push(0x251029u, 0x800005u);
        port.store(0x25102eu, gp[esp] + 0x78u, 2, 0xffffu);
        spill(0x251035u, gp[esp] + 0x44u, 0u);
        call<collision_query>(0x25103bu, 0x251040u);
        flags.logic(gp[eax] & 0xffu, 1);
        return !flags.zf;
    }

    void set_particle_flags(std::uint32_t site, std::uint32_t mask) {
        const auto before = port.read(site, gp[ebp] + particle_flags, 1);
        port.store(site, gp[ebp] + particle_flags, 1, flags.logic(before | mask, 1));
    }

    void apply_collision() {
        if (!query_collision()) {
            reached(collision_miss);
            set_particle_flags(0x2510a7u, location::particle_flags::dead);
            return;
        }
        reached(collision_hit);
        x[0] = scalar(0x251044u, ray_offset);
        x[1] = scalar(0x25104cu, gp[esp] + 0x64u);
        gp[edx] = port.read(0x251052u, gp[esp] + 0x64u, 4);
        gp[ecx] = port.read(0x251056u, gp[esp] + 0x68u, 4);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], scalar(0x25105eu, gp[esp] + 0x44u));
        spill(0x251064u, gp[esi], 1u);
        x[1] = scalar(0x251068u, gp[esp] + 0x68u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], scalar(0x251072u, gp[esp] + 0x48u));
        spill(0x251078u, gp[esi] + 4u, 1u);
        x[1] = scalar(0x25107du, gp[esp] + 0x6cu);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], scalar(0x251087u, gp[esp] + 0x4cu));
        spill(0x25108du, gp[esi] + 8u, 1u);
        set_particle_flags(0x251092u, 0xau);
        gp[eax] = gp[ebp] + velocity;
        port.store(0x251099u, gp[eax], 4, gp[edx]);
        gp[edx] = port.read(0x25109bu, gp[esp] + 0x6cu, 4);
        port.store(0x25109fu, gp[eax] + 4u, 4, gp[ecx]);
        port.store(0x2510a2u, gp[eax] + 8u, 4, gp[edx]);
    }

    void offset_velocity() {
        set_al(port.read(0x2510abu, gp[edi] + 0x16u, 1));
        set_al(flags.right(gp[eax] & 0xffu, 4u, 1, false));
        flags.logic(gp[eax] & 1u, 1);
        if (flags.zf) {
            reached(velocity_unchanged);
            return;
        }
        reached(velocity_offset);
        gp[eax] = port.read(0x2510b5u, gp[esp] + origin, 4);
        x[0] = scalar(0x2510b9u, gp[ebp] + velocity);
        x[0] = ordered_add_ss(x[0], scalar(0x2510beu, gp[eax]));
        spill(0x2510c2u, gp[ebp] + velocity, 0u);
        constexpr AxisSites sites[]{{0x2510c7u, 0x2510ccu, 0x2510d1u}, {0x2510d6u, 0x2510dbu, 0x2510e0u}};
        for (unsigned axis = 1; axis != 3; ++axis) {
            const auto offset = 4u * axis;
            const auto &slot = sites[axis - 1u];
            x[0] = scalar(slot.load, gp[eax] + offset);
            x[0] = ordered_add_ss(x[0], scalar(slot.add, gp[ebp] + velocity + offset));
            spill(slot.store, gp[ebp] + velocity + offset, 0u);
        }
    }

    void place_in_emitter_basis() {
        reached(interpolated_position);
        resolve_origin();
        interpolate_emitter_basis();
        gp[eax] = port.read(0x250fc8u, gp[esp] + particle_definition, 4);
        flags.minus(port.read(0x250fcfu, gp[eax] + 0x78u, 4), 8u, 4);
        if (flags.zf) apply_collision();
        else reached(collision_skipped);
        offset_velocity();
    }

    void advance_along_velocity() {
        reached(alternate_position);
        x[0] = scalar(0x2510e7u, one);
        x[0] = _mm_sub_ss(x[0], x[1]);
        x[1] = scalar(0x2510f3u, gp[ebp] + velocity);
        x[0] = ordered_mul_ss(x[0], x[5]);
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], x[2]);
        spill(0x251104u, gp[esi], 1u);
        constexpr AxisSites sites[]{{0x251108u, 0x251111u, 0x251115u}, {0x25111au, 0x251123u, 0x251127u}};
        for (unsigned axis = 1; axis != 3; ++axis) {
            const auto offset = 4u * axis;
            const auto &slot = sites[axis - 1u];
            x[1] = scalar(slot.load, gp[ebp] + velocity + offset);
            x[1] = ordered_mul_ss(x[1], x[0]);
            x[1] = ordered_add_ss(x[1], x[2u + axis]);
            spill(slot.store, gp[ebp] + position + offset, 1u);
        }
    }

    void run() {
        evaluate();
        if (scale_position_and_age()) place_in_emitter_basis();
        else advance_along_velocity();
        gp[ecx] = port.read(0x25112cu, gp[esp] + 0xccu, 4);
        port.store(0x251133u, gp[ebp] + offsetof(ParticleDatum, seed_color), 4, gp[ecx]);
        const auto count_address = gp[ebx] + offsetof(EmitterDatum, particle_count);
        const auto count = port.read(last, count_address, 2);
        port.store(last, count_address, 2, flags.increment(count, 2, false));
        port.publish(last);
        summary().coverage.fetch_or(hit, std::memory_order_relaxed);
    }
};

template<class State, class Reference, class Route>
bool bridge(State &state, Reference reference, Route route) {
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_BIRTH_PLACEMENT");
        return value && *value;
    }();
    if (ref || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication, native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, summary().calls, reference, [&](world::Port<State> &port) {
        Placement{port, route}.run();
    });
    return true;
}
} // namespace halo2::native_particle_birth_placement_detail
