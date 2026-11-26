#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_system_attachment_detail {
namespace system = native_particle_system_update_detail;
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using location::ParticleDatum;
using location::SystemDefinition;
using location::TagInstance;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x177cd9u, prepared = 0x177d9eu;
inline constexpr std::uint32_t recursive = 0x177da2u, last = 0x177dd1u;
inline constexpr std::uint32_t resolve_definition = 0x178070u, update_attributes = 0x178920u;
inline constexpr std::uint32_t update_transform = 0x178110u, update_system = 0x1778d0u;
inline constexpr std::uint32_t unlink_system = 0x177fa0u, destroy_system = 0x177020u;
inline constexpr std::uint32_t next_slot = 0x2cu, attachment_slot = 0x14u, step_slot = 0x74u;
inline constexpr std::uint32_t transform_slot = 0x38u, basis_slot = 0x40u, parent_field = 0x50u;
inline constexpr world::Function preparation_sites[]{{first, prepared, 0u, 0u}};
inline constexpr world::Function recursive_sites[]{{recursive, last, 0u, 0u}};
enum class Completion {retained, next_attachment, drain};
enum Branch : unsigned {model, sprite, unsupported_tag, child_alive, child_drained, next_attachment, drain, branch_count};

struct VerifySummary {
    std::atomic<std::uint64_t> preparations{}, recursions{}, coverage{};
    ~VerifySummary() {
        if (preparations || recursions)
            std::fprintf(stderr, "Particle system attachment verify: %llu preparations, %llu recursive updates, coverage %llx\n",
                         static_cast<unsigned long long>(preparations.load()), static_cast<unsigned long long>(recursions.load()),
                         static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool use_reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_SYSTEM_ATTACHMENT");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Direct>
struct Attachment {
    world::Port<State> &port;
    Direct &direct;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t{1} << branch; }

    template<class Child>
    void call(std::uint32_t target, std::uint32_t site, std::uint32_t returned, Child &child) {
        port.push(site, returned);
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    template<std::uint32_t target>
    void call_local(std::uint32_t site, std::uint32_t returned) {
        auto child = [&](State &value) { direct(value, std::integral_constant<std::uint32_t, target>{}); };
        call(target, site, returned, child);
    }

    template<class Dispatch>
    void call_method(Dispatch &dispatch, std::uint32_t address, std::uint32_t site, std::uint32_t returned) {
        const auto target = port.read(site, address, 4);
        auto child = [&](State &value) { dispatch(value, target, site); };
        call(target, site, returned, child);
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    void spill(std::uint32_t site, std::uint32_t offset, unsigned reg) {
        port.store(site, gp[esp] + offset, 4, native_tu15_leaves::low_bits(x[reg]));
    }

    void build_transform_arguments() {
        x[0] = scalar(first, location::globals::one);
        gp[edx] = gp[ebx] + offsetof(ParticleDatum, position);
        gp[eax] = port.read(0x177ce4u, gp[edx], 4);
        port.store(0x177ce6u, gp[esp] + 0x64u, 4, gp[eax]);
        gp[ecx] = port.read(0x177ceau, gp[edx] + 4u, 4);
        gp[eax] = port.read(0x177cedu, gp[ebp] + parent_field, 4);
        port.store(0x177cf0u, gp[esp] + 0x68u, 4, gp[ecx]);
        gp[edx] = port.read(0x177cf4u, gp[edx] + 8u, 4);
        spill(0x177cf7u, 0x3cu, 0u);
        x[0] = scalar(0x177cfdu, gp[esp] + step_slot);
        port.store(0x177d03u, gp[esp] + 0x6cu, 4, gp[edx]);
        gp[ecx] = port.read(0x177d07u, gp[esi] + system::first_location, 4);
        x[1] = scalar(0x177d0au, gp[ebx] + offsetof(ParticleDatum, age));
        x[1] = _mm_div_ss(x[1], scalar(0x177d0fu, gp[ebx] + offsetof(ParticleDatum, inverse_lifetime)));
        port.store(0x177d14u, gp[esp] + transform_slot, 4, gp[ecx]);
        x[1] = _mm_sub_ss(x[1], x[0]);
        gp[ecx] = gp[esi];
        port.store(0x177d1eu, gp[esp] + 0x28u, 4, gp[eax]);
        spill(0x177d22u, 0x30u, 1u);
        spill(0x177d28u, 0x34u, 0u);
    }

    template<class Dispatch>
    void select_interface(Dispatch &dispatch) {
        call_local<resolve_definition>(0x177d2eu, 0x177d33u);
        gp[ecx] = port.read(0x177d33u, gp[eax] + offsetof(SystemDefinition, particle_tag), 4);
        gp[eax] = port.read(0x177d36u, location::globals::tag_instances, 4);
        gp[edx] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(gp[ecx]))));
        gp[edx] = flags.shift(gp[edx], 4u, 4);
        gp[eax] = port.read(0x177d41u, gp[edx] + gp[eax] + offsetof(TagInstance, group), 4);
        gp[ebx] = flags.logic(0u, 4);
        flags.minus(gp[eax], location::model_group, 4);
        if (flags.zf) {
            reached(model);
            gp[ebx] = location::globals::model_interface;
        } else {
            flags.minus(gp[eax], location::sprite_group, 4);
            if (!flags.zf) {
                reached(unsupported_tag);
                return;
            }
            reached(sprite);
            gp[ebx] = location::globals::sprite_interface;
        }
        gp[edx] = port.read(0x177d60u, gp[ebx], 4);
        port.push(0x177d62u, gp[ecx]);
        gp[ecx] = gp[ebx];
        call_method(dispatch, gp[edx] + location::interface_methods::select_tag, 0x177d65u, 0x177d68u);
    }

    template<class Dispatch>
    void prepare(Dispatch &dispatch) {
        build_transform_arguments();
        select_interface(dispatch);
        gp[eax] = port.read(0x177d68u, gp[ebx], 4);
        gp[ecx] = gp[ebx];
        call_method(dispatch, gp[eax] + 0x48u, 0x177d6cu, 0x177d6fu);
        gp[edx] = port.read(0x177d6fu, gp[ebx], 4);
        gp[ecx] = gp[ebx];
        port.store(0x177d73u, gp[esp] + 0x13u, 1, gp[eax] & 0xffu);
        call_method(dispatch, gp[edx] + 0x44u, 0x177d77u, 0x177d7au);
        gp[ecx] = port.read(0x177d7au, gp[esp] + 0x28u, 4);
        gp[edi] = flags.logic(system::none, 4);
        port.push(0x177d81u, gp[eax]);
        const auto method_result = port.read(0x177d82u, gp[esp] + 0x17u, 1);
        gp[eax] = (gp[eax] & 0xffffff00u) | method_result;
        call_local<update_attributes>(0x177d86u, 0x177d8bu);
        port.push(0x177d8bu, 0u);
        gp[eax] = gp[esp] + basis_slot;
        port.push(0x177d91u, gp[eax]);
        gp[ecx] = gp[esp] + transform_slot;
        port.push(0x177d96u, gp[ecx]);
        gp[ebx] = gp[esi];
        call_local<update_transform>(0x177d99u, prepared);
        gp[edi] = port.read(prepared, gp[esp] + attachment_slot, 4);
        port.publish(prepared);
    }

    Completion recurse() {
        gp[edx] = port.read(recursive, gp[esp] + step_slot, 4);
        port.push(0x177da6u, gp[edx]);
        gp[ecx] = gp[esi];
        call_local<update_system>(0x177da9u, 0x177daeu);
        flags.logic(gp[eax] & 0xffu, 1);
        if (flags.zf) {
            reached(child_drained);
            gp[eax] = gp[ebp] + 0x44u;
            port.push(0x177db5u, gp[eax]);
            gp[ebx] = gp[ebp] + system::first_attachment;
            gp[ecx] = gp[esi];
            call_local<unlink_system>(0x177dbbu, 0x177dc0u);
            port.push(0x177dc0u, gp[edi]);
            call_local<destroy_system>(0x177dc1u, 0x177dc6u);
        } else reached(child_alive);
        gp[eax] = port.read(0x177dc6u, gp[esp] + next_slot, 4);
        flags.minus(gp[eax], system::none, 4);
        port.store(0x177dcdu, gp[esp] + attachment_slot, 4, gp[eax]);
        port.publish(last);
        if (flags.zf) {
            reached(drain);
            return Completion::drain;
        }
        reached(next_attachment);
        return Completion::next_attachment;
    }
};

template<class State, class Ref, class Direct, class Dispatch>
bool bridge_prepare(State &state, Ref reference, Direct direct, Dispatch dispatch) {
    world::Publication publication;
    if (use_reference() || !system::admit(state, preparation_sites, publication)) return false;
    world::execute(state, publication, summary().preparations, reference, [&](world::Port<State> &port) {
        Attachment kernel{port, direct};
        try {
            kernel.prepare(dispatch);
        } catch (...) {
            summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
            throw;
        }
        summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
    });
    return true;
}

template<class State, class Ref, class Direct>
Completion bridge_recurse(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (use_reference() || !system::admit(state, recursive_sites, publication)) return Completion::retained;
    Completion next = Completion::retained;
    world::execute(state, publication, summary().recursions, [&](State &original) {
        if (reference(original) != next) throw std::runtime_error("Native particle system recursion selected the wrong continuation");
    }, [&](world::Port<State> &port) {
        Attachment kernel{port, direct};
        next = kernel.recurse();
        summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
    });
    return next;
}
} // namespace halo2::native_particle_system_attachment_detail
