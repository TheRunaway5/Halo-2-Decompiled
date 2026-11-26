#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "halo2/particle_emitter.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_emitter_transform_detail {
namespace world = native_world_leaves_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x250780u, last = 0x2508a2u;
inline constexpr std::uint32_t resolver = 0x178070u, orientation = 0x250400u, scales_position = 0x176a30u;
inline constexpr std::uint32_t emitter_slot = 0xffffffb4u, scale_slot = 0xfffffffcu, transform_arg = 0x18u;
inline constexpr std::uint32_t one = 0x46a0d0u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
static_assert(sizeof(Vec3) == 12u && offsetof(ParticleEmitterFrame, rotation) == 4u &&
    offsetof(ParticleEmitterFrame, position) == 0x28u && offsetof(ParticleEmitterState, rotation) == 0x10u &&
    offsetof(ParticleEmitterState, position) == 0x34u && offsetof(ParticleEmitterState, previous_position) == 0x40u);

enum class Resume { retained, transform, emission };
enum Branch : unsigned {no_transform, use_unit_scale, use_authored_scale, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle emitter transform verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

struct CopySites {
    std::array<std::uint32_t, 3> loads, stores;
};

template<class State, class Resolve, class Orientation, class Scale>
struct Entry {
    world::Port<State> &port;
    Resolve &resolve;
    Orientation &orient;
    Scale &scale;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;

    template<class Child>
    void call(std::uint32_t site, std::uint32_t returned, std::uint32_t target, Child &child) {
        port.push(site, returned);
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void save_scalar(std::uint32_t site, std::uint32_t address) {
        port.store(site, address, 4, native_tu15_leaves::low_bits(port.s.xmm[0]));
    }

    // The bank can alias the two rows. Each source word is consumed before
    // its store, and the final load replaces the source address register.
    void copy_vector(unsigned source, unsigned destination, unsigned scratch, const CopySites &sites) {
        for (unsigned lane = 0; lane < 3u; ++lane) {
            const auto value = lane == 2u ? source : scratch;
            const auto offset = 4u * lane;
            gp[value] = port.read(sites.loads[lane], gp[source] + offset, 4);
            port.store(sites.stores[lane], gp[destination] + offset, 4, gp[value]);
        }
    }

    void copy_transform_row(unsigned source, unsigned destination, unsigned scratch,
        std::uint32_t source_offset, std::uint32_t destination_offset,
        std::uint32_t source_site, std::uint32_t destination_site, const CopySites &sites) {
        gp[source] = port.read(source_site, gp[ebp] + transform_arg, 4);
        gp[source] = flags.plus(gp[source], source_offset, 4);
        gp[destination] = port.read(destination_site, gp[ebp] + emitter_slot, 4);
        gp[destination] = flags.plus(gp[destination], destination_offset, 4);
        copy_vector(source, destination, scratch, sites);
    }

    void scale_position_axis(unsigned source, unsigned destination, unsigned axis,
                             std::array<std::uint32_t, 5> sites) {
        auto &xmm = port.s.xmm;
        const auto offset = std::uint32_t(offsetof(ParticleEmitterState, position)) + 4u*axis;
        gp[source] = port.read(sites[0], gp[ebp] + emitter_slot, 4);
        xmm[0] = scalar(sites[1], gp[source] + offset);
        xmm[0] = native_tu15_leaves::ordered_mul_ss(xmm[0], scalar(sites[2], gp[ebp] + scale_slot));
        gp[destination] = port.read(sites[3], gp[ebp] + emitter_slot, 4);
        save_scalar(sites[4], gp[destination] + offset);
    }

    void publish_transform() {
        constexpr auto matrix_basis = offsetof(ParticleEmitterFrame, rotation);
        constexpr auto emitter_basis = offsetof(ParticleEmitterState, rotation);
        gp[eax] = flags.logic(0u, 4);
        copy_transform_row(ecx, edx, eax, matrix_basis, emitter_basis, 0x2507f3u, 0x2507f9u,
            {{0x2507ffu, 0x250803u, 0x250809u}, {0x250801u, 0x250806u, 0x25080cu}});
        copy_transform_row(edx, eax, ecx, matrix_basis + 12u, emitter_basis + 12u, 0x25080fu, 0x250815u,
            {{0x25081bu, 0x25081fu, 0x250825u}, {0x25081du, 0x250822u, 0x250828u}});
        copy_transform_row(eax, ecx, edx, matrix_basis + 24u, emitter_basis + 24u, 0x25082bu, 0x250831u,
            {{0x250837u, 0x25083bu, 0x250841u}, {0x250839u, 0x25083eu, 0x250844u}});
        copy_transform_row(ecx, edx, eax, offsetof(ParticleEmitterFrame, position),
            offsetof(ParticleEmitterState, position), 0x250847u, 0x25084du,
            {{0x250853u, 0x250857u, 0x25085du}, {0x250855u, 0x25085au, 0x250860u}});
        scale_position_axis(edx, eax, 0u, {0x250863u, 0x250866u, 0x25086bu, 0x250870u, 0x250873u});
        scale_position_axis(ecx, edx, 1u, {0x250878u, 0x25087bu, 0x250880u, 0x250885u, 0x250888u});
        scale_position_axis(eax, ecx, 2u, {0x25088du, 0x250890u, 0x250895u, 0x25089au, 0x25089du});
    }

    Resume run() {
        port.push(first, gp[ebp]);
        gp[ebp] = gp[esp];
        gp[esp] = flags.minus(gp[esp], 0x4cu, 4);
        port.push(0x250786u, gp[ebx]);
        port.push(0x250787u, gp[esi]);
        port.store(0x250788u, gp[ebp] + emitter_slot, 4, gp[ecx]);
        gp[ecx] = port.read(0x25078bu, gp[ebp] + 0xcu, 4);
        call(0x25078eu, 0x250793u, resolver, resolve);
        port.store(0x250793u, gp[ebp] - 8u, 4, gp[eax]);
        port.s.xmm[0] = scalar(0x250796u, one);
        save_scalar(0x25079eu, gp[ebp] + scale_slot);

        gp[eax] = port.read(0x2507a3u, gp[ebp] + emitter_slot, 4);
        gp[eax] = flags.plus(gp[eax], offsetof(ParticleEmitterState, position), 4);
        gp[ecx] = port.read(0x2507a9u, gp[ebp] + emitter_slot, 4);
        gp[ecx] = flags.plus(gp[ecx], offsetof(ParticleEmitterState, previous_position), 4);
        copy_vector(eax, ecx, edx, {{0x2507afu, 0x2507b3u, 0x2507b9u}, {0x2507b1u, 0x2507b6u, 0x2507bcu}});

        flags.minus(port.read(0x2507bfu, gp[ebp] + transform_arg, 4), 0u, 4);
        if (flags.zf) {
            summary().coverage.fetch_or(1u << no_transform, std::memory_order_relaxed);
            port.publish(0x2507c3u);
            return Resume::emission;
        }

        gp[eax] = port.read(0x2507c9u, gp[ebp] + 0x10u, 4);
        call(0x2507ccu, 0x2507d1u, orientation, orient);
        port.store(0x2507d1u, gp[ebp] - 0x18u, 4, gp[eax]);
        gp[eax] = port.read(0x2507d4u, gp[ebp] + 0xcu, 4);
        call(0x2507d7u, 0x2507dcu, scales_position, scale);
        gp[ecx] = gp[eax] & 0xffu;
        flags.logic(gp[ecx], 4);
        const bool authored_scale = !flags.zf;
        if (authored_scale) {
            gp[edx] = port.read(0x2507e3u, gp[ebp] + transform_arg, 4);
            port.s.xmm[0] = scalar(0x2507e6u, gp[edx] + offsetof(ParticleEmitterFrame, scale));
            save_scalar(0x2507eau, gp[ebp] + scale_slot);
        }
        publish_transform();
        gp[eax] = port.read(last, gp[ebp] + 0x10u, 4);
        port.publish(last);
        summary().coverage.fetch_or(1u << (authored_scale ? use_authored_scale : use_unit_scale),
                                    std::memory_order_relaxed);
        return Resume::transform;
    }
};

template<class State, class Reference, class Resolve, class Orientation, class Scale>
Resume bridge(State &state, Reference reference, Resolve resolve, Orientation orient, Scale scale) {
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_EMITTER_TRANSFORM");
        return value && *value;
    }();
    if (reference_mode || state.active_mmx) return Resume::retained;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return Resume::retained;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return Resume::retained;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return Resume::retained;
    Resume next = Resume::retained;
    auto replay = [&](State &original) {
        if (reference(original) != next) throw std::runtime_error("Native emitter transform resumed at the wrong site");
    };
    world::execute(state, publication, summary().calls, replay, [&](world::Port<State> &port) {
        next = Entry{port, resolve, orient, scale}.run();
    });
    return next;
}
} // namespace halo2::native_emitter_transform_detail
