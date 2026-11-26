#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_system_selection_detail {
namespace system = native_particle_system_update_detail;
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using world::eax;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t repeat = 0x177985u, first = 0x177990u, last = 0x177a29u;
inline constexpr std::uint32_t first_basis = 0x177ad8u, second_basis = 0x177b2au, last_basis = 0x177b77u;
inline constexpr std::uint32_t basis_shift = second_basis - first_basis;
inline constexpr std::uint32_t system_array = 0x51df74u, attachment_next = 0x38u, attached_particle = 0x4cu;
inline constexpr std::uint32_t contact_index = 0x24u, attachment_slot = 0x14u, next_slot = 0x2cu;
inline constexpr std::uint32_t interface_slot = 0x20u, contact_table = 0x24u, count_method = 0x20u;
inline constexpr std::uint32_t fixed_contact_type_a = 0x20000cau, fixed_contact_type_b = 0x70000c0u;
inline constexpr world::Function first_sites[]{{first, last_basis, 0u, 0u}};
inline constexpr world::Function repeat_sites[]{{repeat, last_basis, 0u, 0u}};
enum class Selection {retained, recursive_update, computed_basis, velocity_basis, prepared_basis};
enum Branch : unsigned {
    initial_attachment, subsequent_attachment, absent_particle, beyond_capacity, empty_particle, stale_particle,
    missing_contact, beyond_contacts, fixed_a, fixed_b, compute_basis, branch_count
};

struct VerifySummary {
    std::atomic<std::uint64_t> entries{}, repeats{}, coverage{};
    ~VerifySummary() {
        if (entries || repeats)
            std::fprintf(stderr, "Particle system selection verify: %llu first attachments, %llu subsequent attachments, coverage %llx\n",
                         static_cast<unsigned long long>(entries.load()), static_cast<unsigned long long>(repeats.load()),
                         static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool use_reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_SYSTEM_SELECTION");
        return value && *value;
    }();
    return enabled;
}

template<class State, class Dispatch>
struct AttachmentSelection {
    world::Port<State> &port;
    Dispatch &dispatch;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t{1} << branch; }

    Selection resume(Selection next, Branch branch, std::uint32_t site) {
        reached(branch);
        port.publish(site);
        return next;
    }

    void resolve_attachment() {
        gp[eax] = port.read(first, system_array, 4);
        gp[ecx] = port.read(0x177995u, gp[eax] + 0x44u, 4);
        gp[esi] = flags.logic(gp[edi] & 0xffffu, 4);
        gp[esi] = flags.product(gp[esi], sizeof(location::SystemDatum));
        gp[esi] = flags.plus(gp[esi], gp[ecx], 4);
        gp[ecx] = port.read(0x1779a5u, gp[esi] + attachment_next, 4);
        port.store(0x1779a8u, gp[esp] + next_slot, 4, gp[ecx]);
        gp[ecx] = port.read(0x1779acu, gp[esi] + attached_particle, 4);
        flags.minus(gp[ecx], system::none, 4);
    }

    bool check_particle_index() {
        gp[edx] = port.read(0x1779b8u, location::globals::particles, 4);
        gp[ebx] = port.read(0x1779beu, gp[edx] + 0x38u, 4);
        gp[eax] = flags.logic(gp[ecx] & 0xffffu, 4);
        flags.minus(gp[eax], gp[ebx], 4);
        return flags.sf != flags.of;
    }

    void resolve_particle() {
        gp[ebx] = port.read(0x1779d0u, gp[edx] + 0x24u, 4);
        gp[ebx] = flags.product(gp[ebx], gp[eax]);
        gp[ebx] = flags.plus(gp[ebx], port.read(0x1779d6u, gp[edx] + 0x44u, 4), 4);
        const auto salt = port.read(0x1779d9u, gp[ebx] + offsetof(location::ParticleDatum, salt), 2);
        gp[eax] = (gp[eax] & 0xffff0000u) | salt;
        flags.logic(salt, 2);
    }

    bool matching_salt() {
        gp[edx] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(gp[eax]))));
        gp[ecx] = flags.right(gp[ecx], 16u, 4, true);
        flags.minus(gp[edx], gp[ecx], 4);
        return flags.zf;
    }

    bool valid_contact() {
        gp[ecx] = port.read(0x1779ffu, gp[esp] + interface_slot, 4);
        gp[edi] = gp[eax];
        gp[eax] = port.read(0x177a05u, gp[ecx], 4);
        constexpr auto site = 0x177a07u;
        const auto target = port.read(site, gp[eax] + count_method, 4);
        port.push(site, 0x177a0au);
        auto child = [&](State &state) { dispatch(state, target, site); };
        try {
            port.call_child(target, child);
        } catch (...) {
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
        flags.minus(gp[edi], gp[eax], 4);
        return flags.sf != flags.of;
    }

    // The two fixed contact bases have matching access schedules, including
    // their pointer loads between the component stores.
    void copy_basis(std::uint32_t delta, std::uint32_t first_vector, std::uint32_t second_vector) {
        gp[edx] = port.read(first_basis + delta, first_vector, 4);
        gp[eax] = port.read(0x177adeu + delta, gp[edx], 4);
        port.store(0x177ae0u + delta, gp[esp] + 0x40u, 4, gp[eax]);
        gp[ecx] = port.read(0x177ae4u + delta, gp[edx] + 4u, 4);
        gp[eax] = port.read(0x177ae7u + delta, second_vector, 4);
        port.store(0x177aecu + delta, gp[esp] + 0x44u, 4, gp[ecx]);
        gp[edx] = port.read(0x177af0u + delta, gp[edx] + 8u, 4);
        port.store(0x177af3u + delta, gp[esp] + 0x48u, 4, gp[edx]);
        gp[ecx] = port.read(0x177af7u + delta, gp[eax], 4);
        port.store(0x177af9u + delta, gp[esp] + 0x58u, 4, gp[ecx]);
        gp[edx] = port.read(0x177afdu + delta, gp[eax] + 4u, 4);
        gp[ecx] = port.read(0x177b00u + delta, 0x474d0cu, 4);
        port.store(0x177b06u + delta, gp[esp] + 0x5cu, 4, gp[edx]);
        gp[eax] = port.read(0x177b0au + delta, gp[eax] + 8u, 4);
        port.store(0x177b0du + delta, gp[esp] + 0x60u, 4, gp[eax]);
        gp[edx] = port.read(0x177b11u + delta, gp[ecx], 4);
        port.store(0x177b13u + delta, gp[esp] + 0x4cu, 4, gp[edx]);
        gp[eax] = port.read(0x177b17u + delta, gp[ecx] + 4u, 4);
        port.store(0x177b1au + delta, gp[esp] + 0x50u, 4, gp[eax]);
        gp[ecx] = port.read(0x177b1eu + delta, gp[ecx] + 8u, 4);
        port.store(0x177b21u + delta, gp[esp] + 0x54u, 4, gp[ecx]);
    }

    Selection select_basis() {
        gp[eax] = port.read(0x1779f3u, gp[esi] + contact_index, 4);
        flags.minus(gp[eax], system::none, 4);
        if (flags.zf) return resume(Selection::velocity_basis, missing_contact, 0x1779f9u);
        if (!valid_contact()) return resume(Selection::velocity_basis, beyond_contacts, 0x177a0cu);
        gp[ecx] = port.read(0x177a12u, gp[esp] + contact_table, 4);
        gp[eax] = port.read(0x177a16u, gp[ecx] + gp[edi] * 4u, 4);
        flags.minus(gp[eax], fixed_contact_type_a, 4);
        if (flags.zf) {
            reached(fixed_a);
            copy_basis(basis_shift, 0x474d10u, 0x474d08u);
            port.publish(last_basis);
            return Selection::prepared_basis;
        }
        flags.minus(gp[eax], fixed_contact_type_b, 4);
        if (flags.zf) {
            reached(fixed_b);
            copy_basis(0u, 0x474d1cu, 0x474d14u);
            port.publish(0x177b25u);
            return Selection::prepared_basis;
        }
        return resume(Selection::computed_basis, compute_basis, last);
    }

    template<bool Repeat>
    Selection run() {
        if constexpr (Repeat) {
            reached(subsequent_attachment);
            gp[edi] = port.read(repeat, gp[esp] + attachment_slot, 4);
        } else reached(initial_attachment);
        resolve_attachment();
        if (flags.zf) return resume(Selection::recursive_update, absent_particle, 0x1779b2u);
        if (!check_particle_index()) return resume(Selection::recursive_update, beyond_capacity, 0x1779cau);
        resolve_particle();
        if (flags.zf) return resume(Selection::recursive_update, empty_particle, 0x1779dfu);
        if (!matching_salt()) return resume(Selection::recursive_update, stale_particle, 0x1779edu);
        return select_basis();
    }
};

template<bool Repeat, class State, class Ref, class Dispatch>
Selection bridge(State &state, Ref reference, Dispatch dispatch) {
    world::Publication publication;
    const std::span<const world::Function> sites = Repeat ? repeat_sites : first_sites;
    if (use_reference() || !system::admit(state, sites, publication)) return Selection::retained;
    Selection next = Selection::retained;
    world::execute(state, publication, Repeat ? summary().repeats : summary().entries, [&](State &original) {
        if (reference(original) != next) throw std::runtime_error("Native attachment selection chose the wrong continuation");
    }, [&](world::Port<State> &port) {
        AttachmentSelection kernel{port, dispatch};
        next = kernel.template run<Repeat>();
        summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
    });
    return next;
}
} // namespace halo2::native_particle_system_selection_detail
