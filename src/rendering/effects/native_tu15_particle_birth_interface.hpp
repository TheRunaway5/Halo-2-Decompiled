#pragma once

#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_birth_interface_detail {
namespace world = native_world_leaves_detail;
using native_effects::location_update::ParticleDatum;
using native_effects::location_update::TagInstance;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t first = 0x25113au, epilogue = 0x2512c7u, last = 0x2512d1u;
inline constexpr std::uint32_t enum_attachments = 0x13a6b0u, create_attachment = 0x176e70u;
inline constexpr std::uint32_t assign_attachment = 0x177f10u, resolve = 0x178070u;
inline constexpr std::uint32_t local_transform = 0x2c4110u, notify = 0x250170u;
inline constexpr std::uint32_t invalid = 0xffffffffu, caller_args = 24u, locals = 0xb0u;
inline constexpr std::uint32_t definition = 0x1cu, system_slot = 0xc4u, attachment_index = 0x14u;
inline constexpr std::uint32_t handle_slot = 0x20u, current_interface = 0x10u;
inline constexpr std::uint32_t tag_instances = 0x4f0414u, standard_group = 0x5052544du, alternate_group = 0x70727433u;
inline constexpr std::uint32_t standard_interface = 0x486048u, alternate_interface = 0x48603cu;
inline constexpr std::uint32_t attachment_array = 0x51df74u, attachment_stride = 0x54u;
inline constexpr std::uint32_t notification_context = 0x474d10u, one = 0x46a0d0u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
enum Branch : unsigned {
    no_attachments, have_attachments, next_attachment, no_tag_interface, standard_tag_interface,
    alternate_tag_interface, no_attachment, have_attachment, nonpositive_motion, positive_motion,
    zero_motion_result, positive_motion_result, no_notification, send_notification, returned,
    early_epilogue, branch_count
};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{}, early_calls{}, coverage{};
    ~VerifySummary() {
        if (calls || early_calls)
            std::fprintf(stderr, "Particle birth interface and return verify: %llu interface calls, %llu early returns, coverage %llx\n",
                         static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(early_calls.load()),
                         static_cast<unsigned long long>(coverage.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Dispatch>
struct BirthInterface {
    world::Port<State> &port;
    Direct &direct;
    Dispatch &dispatch;
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
        auto child = [&](State &state) { direct(state, std::integral_constant<std::uint32_t, target>{}); };
        call(target, site, returned, child);
    }

    void call_dispatch(std::uint32_t target, std::uint32_t site, std::uint32_t returned) {
        auto child = [&](State &state) { dispatch(state, target, site); };
        call(target, site, returned, child);
    }

    void call_method(std::uint32_t address, std::uint32_t site, std::uint32_t returned) {
        call_dispatch(port.read(site, address, 4), site, returned);
    }

    __m128 scalar(std::uint32_t site, std::uint32_t address) {
        return world::scalar(std::bit_cast<float>(port.read(site, address, 4)));
    }

    bool begin_attachments() {
        gp[edx] = port.read(first, gp[edi] + 4u, 4);
        call_local<enum_attachments>(0x25113du, 0x251142u);
        gp[esi] = gp[eax];
        gp[edx] = port.read(0x251144u, gp[esi], 4);
        gp[ebx] = flags.logic(0u, 4);
        gp[ecx] = gp[esi];
        port.store(0x25114au, gp[esp] + 0xbcu, 2, 0xffffu);
        port.store(0x251154u, gp[esp] + attachment_index, 4, gp[ebx]);
        call_method(gp[edx] + 0x18u, 0x251158u, 0x25115bu);
        flags.logic(gp[eax], 4);
        return !flags.zf && flags.sf == flags.of;
    }

    void create_current_attachment() {
        gp[eax] = port.read(0x251170u, gp[edi] + 4u, 4);
        gp[ecx] = port.read(0x251173u, gp[esp] + system_slot, 4);
        gp[ecx] = port.read(0x25117au, gp[ecx] + 0x18u, 4);
        gp[edx] = port.read(0x25117du, gp[esi], 4);
        port.push(0x25117fu, invalid);
        port.push(0x251181u, gp[ebx]);
        port.push(0x251182u, gp[eax]);
        port.push(0x251183u, gp[ecx]);
        port.push(0x251184u, gp[ebx]);
        gp[ecx] = gp[esi];
        call_method(gp[edx] + 0x1cu, 0x251187u, 0x25118au);
        gp[edi] = gp[eax];
        call_local<create_attachment>(0x25118cu, 0x251191u);
        gp[edx] = port.read(0x251191u, tag_instances, 4);
        gp[esi] = gp[eax];
    }

    void select_group_interface() {
        gp[eax] = port.read(0x251199u, gp[esp] + definition, 4);
        gp[ecx] = port.read(0x25119du, gp[eax] + 4u, 4);
        gp[eax] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(gp[ecx]))));
        gp[eax] = flags.shift(gp[eax], 4u, 4);
        gp[eax] = flags.plus(gp[eax], gp[edx], 4);
        gp[eax] = port.read(0x2511a8u, gp[eax] + offsetof(TagInstance, group), 4);
        gp[edi] = flags.logic(0u, 4);
        flags.minus(gp[eax], standard_group, 4);
        if (flags.zf) {
            gp[edi] = standard_interface;
            reached(standard_tag_interface);
        } else {
            flags.minus(gp[eax], alternate_group, 4);
            if (!flags.zf) {
                reached(no_tag_interface);
                return;
            }
            gp[edi] = alternate_interface;
            reached(alternate_tag_interface);
        }
        gp[edx] = port.read(0x2511c6u, gp[edi], 4);
        port.push(0x2511c8u, gp[ecx]);
        gp[ecx] = gp[edi];
        call_method(gp[edx] + 0x50u, 0x2511cbu, 0x2511ceu);
    }

    void prepare_attachment_motion() {
        gp[eax] = port.read(0x2511d7u, attachment_array, 4);
        gp[edx] = port.read(0x2511dcu, gp[esp] + system_slot, 4);
        gp[ecx] = port.read(0x2511e3u, gp[eax] + 0x44u, 4);
        gp[ebx] = port.read(0x2511e6u, gp[esp] + system_slot, 4);
        gp[esi] = flags.logic(gp[esi] & 0xffffu, 4);
        gp[esi] = flags.product(gp[esi], attachment_stride);
        gp[edx] = flags.plus(gp[edx], 0x40u, 4);
        gp[esi] = flags.plus(gp[esi], gp[ecx], 4);
        gp[ebx] = flags.plus(gp[ebx], 0x44u, 4);
        port.push(0x2511feu, gp[edx]);
        call_local<assign_attachment>(0x2511ffu, 0x251204u);

        gp[eax] = port.read(0x251204u, gp[esp] + system_slot, 4);
        gp[ecx] = port.read(0x25120bu, gp[esp] + handle_slot, 4);
        x[1] = _mm_setzero_ps();
        x[2] = scalar(0x251212u, one);
        port.store(0x25121au, gp[esi] + 0x48u, 4, gp[eax]);
        port.store(0x25121du, gp[esi] + 0x4cu, 4, gp[ecx]);

        constexpr auto inverse_lifetime = offsetof(ParticleDatum, inverse_lifetime);
        x[0] = scalar(0x251220u, gp[ebp] + inverse_lifetime);
        flags.scalar_compare(x[0], x[1], false);
        if (flags.cf || flags.zf) {
            reached(nonpositive_motion);
            x[0] = x[1];
        } else {
            reached(positive_motion);
            x[0] = x[2];
            x[0] = _mm_div_ss(x[0], scalar(0x25122du, gp[ebp] + inverse_lifetime));
        }
        const auto attachment_flags = port.read(0x251237u, gp[esi] + 0xcu, 1);
        port.store(0x251237u, gp[esi] + 0xcu, 1, flags.logic(attachment_flags | 1u, 1));
        flags.scalar_compare(x[0], x[1], false);
        gp[ebx] = port.read(0x25123eu, gp[esp] + attachment_index, 4);
        port.store(0x251242u, gp[esi] + 4u, 4, native_tu15_leaves::low_bits(x[1]));
        if (flags.cf || flags.zf) reached(zero_motion_result);
        else {
            reached(positive_motion_result);
            x[2] = _mm_div_ss(x[2], x[0]);
        }
        port.store(0x25124du, gp[esi] + 8u, 4, native_tu15_leaves::low_bits(x[2]));
    }

    void visit_attachments() {
        if (!begin_attachments()) {
            reached(no_attachments);
            return;
        }
        reached(have_attachments);
        for (;;) {
            create_current_attachment();
            select_group_interface();
            flags.minus(gp[esi], invalid, 4);
            port.store(0x2511d1u, gp[esp] + current_interface, 4, gp[edi]);
            if (flags.zf) reached(no_attachment);
            else {
                reached(have_attachment);
                prepare_attachment_motion();
            }
            gp[edx] = port.read(0x251252u, gp[edi], 4);
            gp[ebx] = flags.increment(gp[ebx], 4, false);
            gp[ecx] = gp[edi];
            port.store(0x251257u, gp[esp] + attachment_index, 4, gp[ebx]);
            call_method(gp[edx] + 0x18u, 0x25125bu, 0x25125eu);
            flags.minus(gp[ebx], gp[eax], 4);
            gp[esi] = port.read(0x251260u, gp[esp] + current_interface, 4);
            if (flags.sf == flags.of) return;
            reached(next_attachment);
            gp[edi] = port.read(0x251165u, gp[esp] + definition, 4);
        }
    }

    void publish_notification() {
        gp[eax] = port.read(0x25126au, gp[esi], 4);
        gp[ecx] = gp[esi];
        call_method(gp[eax] + 0x14u, 0x25126eu, 0x251271u);
        flags.minus(gp[eax], invalid, 4);
        if (flags.zf) {
            reached(no_notification);
            return;
        }
        reached(send_notification);
        gp[edi] = port.read(0x251276u, notification_context, 4);
        port.push(0x25127cu, gp[ecx]);
        gp[eax] = gp[esp];
        port.store(0x25127fu, gp[eax], 2, 0u);
        gp[eax] = gp[ebp] + offsetof(ParticleDatum, velocity);
        gp[ebx] = gp[ebp] + offsetof(ParticleDatum, position);
        gp[ecx] = gp[esp] + 0x9cu;
        call_dispatch(local_transform, 0x251291u, 0x251296u);

        gp[ecx] = port.read(0x251296u, gp[esp] + system_slot, 4);
        call_local<resolve>(0x25129du, 0x2512a2u);
        gp[edx] = port.read(0x2512a2u, gp[esi], 4);
        gp[edi] = port.read(0x2512a4u, gp[eax] + 4u, 4);
        gp[ecx] = gp[esi];
        call_method(gp[edx] + 0x14u, 0x2512a9u, 0x2512acu);
        gp[ebx] = gp[eax];
        gp[eax] = port.read(0x2512aeu, gp[esi], 4);
        gp[ecx] = gp[esi];
        call_method(gp[eax] + 0x10u, 0x2512b2u, 0x2512b5u);
        port.push(0x2512b5u, gp[edi]);
        gp[edx] = gp[eax];
        port.push(0x2512b8u, gp[ebp]);
        gp[ecx] = gp[esp] + 0xa0u;
        gp[eax] = gp[ebx];
        call_local<notify>(0x2512c2u, epilogue);
    }

    void finish() {
        gp[edi] = port.pop(epilogue);
        gp[esi] = port.pop(0x2512c8u);
        gp[ebp] = port.pop(0x2512c9u);
        gp[ebx] = port.pop(0x2512cau);
        gp[esp] = flags.plus(gp[esp], locals, 4);
        port.ret(last, caller_args);
        reached(returned);
        port.publish(last);
    }

    template<bool Early>
    void run() {
        if constexpr (Early) reached(early_epilogue);
        else {
            visit_attachments();
            publish_notification();
        }
        finish();
    }
};

template<class State, class Reference, class Direct, class Dispatch, bool Early = false>
bool bridge(State &state, Reference reference, Direct direct, Dispatch dispatch, std::bool_constant<Early> = {}) {
    static const bool ref = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_BIRTH_INTERFACE");
        return value && *value;
    }();
    if (ref || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication, native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, Early ? summary().early_calls : summary().calls, reference,
        [&](world::Port<State> &port) {
            BirthInterface kernel{port, direct, dispatch};
            try {
                kernel.template run<Early>();
            } catch (...) {
                summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
                throw;
            }
            summary().coverage.fetch_or(kernel.hit, std::memory_order_relaxed);
        });
    return true;
}

template<class State, class Reference>
bool bridge_epilogue(State &state, Reference reference) {
    return bridge(state, reference, [](auto &, auto) {}, [](auto &, auto, auto) {}, std::true_type{});
}
} // namespace halo2::native_particle_birth_interface_detail
