#pragma once

#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_system_update_detail {
namespace world = native_world_leaves_detail;
namespace location = native_effects::location_update;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x1778d0u, entry_last = 0x177983u;
inline constexpr std::uint32_t epilogue = 0x177dd7u, last = 0x177e18u;
inline constexpr std::uint32_t remove_inactive = 0x178290u, resolve_system = 0x178070u;
inline constexpr std::uint32_t visit_location = 0x251a60u, resolve_interface = 0x13a6b0u;
inline constexpr std::uint32_t drain = 0x178240u;
inline constexpr std::uint32_t none = 0xffffffffu, local_size = 0x60u;
inline constexpr std::uint32_t flags_offset = offsetof(location::SystemDatum, flags);
inline constexpr std::uint32_t first_location = 0x30u, next_location = 0xcu, first_attachment = 0x40u;
inline constexpr std::uint32_t location_array = 0x52d71cu, active_effects = 0x4d1ee0u;
inline constexpr world::Function entry_sites[]{{first, entry_last, 0u, 0u}};
inline constexpr world::Function drain_sites[]{{epilogue, last, 0u, 0u}};
enum class Resume {retained, attachments, drain};
enum Branch : unsigned {
    inherit_activity, clear_inherited_activity, explicit_activity, effect_active, effect_inactive,
    locations_empty, locations_present, location_loop, attachments_empty, attachments_present,
    drain_nonpositive, drain_positive, keep_for_update, keep_for_cycling, keep_for_attachment,
    keep_for_flag, reclaimed, branch_count
};

struct VerifySummary {
    std::atomic<std::uint64_t> entries{}, exits{}, coverage{};
    ~VerifySummary() {
        if (entries || exits)
            std::fprintf(stderr, "Particle system update boundary verify: %llu entries, %llu exits, coverage %llx\n",
                         static_cast<unsigned long long>(entries.load()), static_cast<unsigned long long>(exits.load()),
                         static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

inline bool reference_mode() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_SYSTEM_UPDATE");
        return value && *value;
    }();
    return enabled;
}

template<class State>
bool admit(State &state, std::span<const world::Function> sites, world::Publication &publication) {
    if (reference_mode() || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    return world::admitted(state, sites, publication, native_particle_birth_observer_detail::qualified);
}

template<class State, class Direct>
struct Step {
    world::Port<State> &port;
    Direct &direct;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
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

    void refresh_activity_flags() {
        gp[esp] = flags.minus(gp[esp], local_size, 4);
        port.push(0x1778d3u, gp[ebp]);
        gp[ebp] = gp[ecx];
        gp[eax] = flags.logic(0u, 4);
        gp[eax] = port.read(0x1778d8u, gp[ebp] + flags_offset, 2);
        flags.logic((gp[eax] >> 8u) & 1u, 1);
        if (flags.zf) {
            reached(clear_inherited_activity);
            gp[eax] = flags.logic(gp[eax] & 0xfdffu, 4);
        } else {
            reached(inherit_activity);
            gp[eax] = flags.logic(gp[eax] | 0x200u, 4);
        }
        port.store(0x1778edu, gp[ebp] + flags_offset, 2, gp[eax]);
        const auto upper_flags = port.read(0x1778f1u, gp[ebp] + flags_offset + 1u, 1);
        port.store(0x1778f1u, gp[ebp] + flags_offset + 1u, 1, flags.logic(upper_flags & 0xfeu, 1));
        gp[eax] = port.read(0x1778f5u, gp[ebp] + flags_offset, 2);
        flags.logic(gp[eax] & 0x20u, 1);
        port.push(0x1778fbu, gp[ebx]);
        port.push(0x1778fcu, gp[esi]);
        port.push(0x1778fdu, gp[edi]);

        if (!flags.zf) {
            reached(explicit_activity);
            return;
        }
        const auto effect = port.read(0x177900u, gp[ebp] + 0x20u, 2);
        gp[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(effect))));
        gp[ecx] = flags.logic(gp[eax] & 31u, 4);
        gp[edx] = flags.shift(1u, gp[ecx], 4);
        gp[eax] = flags.right(gp[eax], 5u, 4, true);
        flags.logic(port.read(0x177913u, active_effects + gp[eax] * 4u, 4) & gp[edx], 4);
        if (flags.zf) {
            reached(effect_inactive);
            gp[ebx] = gp[ebp];
            call_local<remove_inactive>(0x17791eu, 0x177923u);
        } else reached(effect_active);
    }

    void update_locations() {
        gp[ecx] = gp[ebp];
        call_local<resolve_system>(0x177925u, 0x17792au);
        gp[ebx] = port.read(0x17792au, gp[ebp] + first_location, 4);
        flags.minus(gp[ebx], none, 4);
        gp[esi] = gp[eax];
        if (flags.zf) {
            reached(locations_empty);
            return;
        }
        reached(locations_present);
        for (;;) {
            gp[eax] = port.read(0x177934u, location_array, 4);
            gp[ecx] = port.read(0x177939u, gp[esp] + 0x74u, 4);
            gp[edi] = port.read(0x17793du, gp[eax] + 0x44u, 4);
            gp[ebx] = flags.logic(gp[ebx] & 0xffffu, 4);
            gp[ebx] = flags.product(gp[ebx], sizeof(location::LocationDatum));
            port.push(0x177949u, gp[ecx]);
            gp[ebx] = flags.plus(gp[ebx], gp[edi], 4);
            port.push(0x17794cu, gp[ebp]);
            gp[eax] = gp[esi];
            call_local<visit_location>(0x17794fu, 0x177954u);
            gp[ebx] = port.read(0x177954u, gp[ebx] + next_location, 4);
            flags.minus(gp[ebx], none, 4);
            if (flags.zf) return;
            reached(location_loop);
        }
    }

    template<class Dispatch>
    Resume setup(Dispatch &dispatch) {
        refresh_activity_flags();
        update_locations();
        gp[edx] = port.read(0x17795cu, gp[esi] + offsetof(location::SystemDefinition, particle_tag), 4);
        call_local<resolve_interface>(0x17795fu, 0x177964u);
        gp[edx] = port.read(0x177964u, gp[eax], 4);
        gp[ecx] = gp[eax];
        port.store(0x177968u, gp[esp] + 0x20u, 4, gp[eax]);
        constexpr auto site = 0x17796cu;
        const auto target = port.read(site, gp[edx] + 0x24u, 4);
        auto child = [&](State &value) { dispatch(value, target, site); };
        call(target, site, 0x17796fu, child);
        gp[edi] = port.read(0x17796fu, gp[ebp] + first_attachment, 4);
        flags.minus(gp[edi], none, 4);
        port.store(0x177975u, gp[esp] + 0x24u, 4, gp[eax]);
        port.store(0x177979u, gp[esp] + 0x14u, 4, gp[edi]);
        if (flags.zf) {
            reached(attachments_empty);
            port.publish(0x17797du);
            return Resume::drain;
        }
        reached(attachments_present);
        port.publish(entry_last);
        return Resume::attachments;
    }

    bool drain_and_check_alive() {
        const auto current = port.read(epilogue, gp[ebp] + flags_offset, 1);
        port.store(epilogue, gp[ebp] + flags_offset, 1, flags.logic(current & 0xfeu, 1));
        gp[eax] = gp[ebp];
        call_local<drain>(0x177dddu, 0x177de2u);
        flags.logic(gp[eax], 4);
        gp[edi] = port.pop(0x177de4u);
        gp[esi] = port.pop(0x177de5u);
        gp[ebx] = port.pop(0x177de6u);
        if (!flags.zf && flags.sf == flags.of) {
            reached(drain_positive);
            const auto low = port.read(0x177de9u, gp[ebp] + flags_offset, 1);
            gp[ecx] = (gp[ecx] & 0xffffff00u) | low;
            gp[ecx] = (gp[ecx] & 0xffffff00u) | flags.right(low, 3u, 1, false);
            flags.logic(gp[ecx] & 1u, 1);
            if (!flags.zf) {
                reached(keep_for_update);
                return true;
            }
        } else reached(drain_nonpositive);

        flags.logic(port.read(0x177df4u, gp[ebp] + flags_offset, 1) & 1u, 1);
        if (!flags.zf) {
            reached(keep_for_cycling);
            return true;
        }
        flags.minus(port.read(0x177dfau, gp[ebp] + first_attachment, 4), none, 4);
        if (!flags.zf) {
            reached(keep_for_attachment);
            return true;
        }
        flags.logic(port.read(0x177e00u, gp[ebp] + flags_offset + 1u, 1) & 2u, 1);
        if (!flags.zf) {
            reached(keep_for_flag);
            return true;
        }
        reached(reclaimed);
        return false;
    }

    void finish() {
        if (drain_and_check_alive()) {
            gp[eax] = 1u;
            gp[ebp] = port.pop(0x177e14u);
            gp[esp] = flags.plus(gp[esp], local_size, 4);
            port.ret(last, 4u);
            port.publish(last);
            return;
        }
        gp[eax] = flags.logic(0u, 4);
        gp[ebp] = port.pop(0x177e08u);
        gp[esp] = flags.plus(gp[esp], local_size, 4);
        port.ret(0x177e0cu, 4u);
        port.publish(0x177e0cu);
    }
};

template<class State, class Ref, class Direct, class Dispatch>
Resume bridge_entry(State &state, Ref reference, Direct direct, Dispatch dispatch) {
    world::Publication publication;
    if (!admit(state, entry_sites, publication)) return Resume::retained;
    Resume next = Resume::retained;
    world::execute(state, publication, summary().entries, [&](State &original) {
        if (reference(original) != next) throw std::runtime_error("Native particle system entry selected the wrong continuation");
    }, [&](world::Port<State> &port) {
        Step step{port, direct};
        next = step.setup(dispatch);
        summary().coverage.fetch_or(step.hit, std::memory_order_relaxed);
    });
    return next;
}

template<class State, class Ref, class Direct>
bool bridge_drain(State &state, Ref reference, Direct direct) {
    world::Publication publication;
    if (!admit(state, drain_sites, publication)) return false;
    world::execute(state, publication, summary().exits, reference, [&](world::Port<State> &port) {
        Step step{port, direct};
        step.finish();
        summary().coverage.fetch_or(step.hit, std::memory_order_relaxed);
    });
    return true;
}
} // namespace halo2::native_particle_system_update_detail
