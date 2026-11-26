#pragma once

#include "rendering/effects/native_tu15_particle_birth_observer.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"

namespace halo2::native_particle_location_emitters_detail {
namespace world = native_world_leaves_detail;
namespace layout = native_effects::location_update;
namespace fp = physics_step_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using layout::EmitterDatum;
using layout::EmitterDefinition;
using layout::LocationDatum;
using layout::SystemDefinition;

inline constexpr std::uint32_t first = 0x251e30u, last = 0x251f64u;
inline constexpr std::uint32_t allocate_emitter = 0x16e6a0u, update_emitter = 0x250780u;
inline constexpr world::Function functions[]{{first, last, 0u, 0u}};
enum Branch : unsigned {empty, existing, allocated, pool_full, set_head, link_predecessor,
    empty_predecessor, continued, finished, branch_count};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "Particle location emitter tail verify: %llu calls, coverage %llx\n",
            static_cast<unsigned long long>(calls.load()), static_cast<unsigned long long>(coverage().load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Dispatch, class Update>
struct Tail {
    world::Port<State> &port;
    Dispatch &dispatch;
    Update &update;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    std::uint64_t hit{};

    void reached(Branch branch) { hit |= std::uint64_t(1) << branch; }

    template<class Child>
    void call(std::uint32_t site, std::uint32_t returned, std::uint32_t target, Child &&child) {
        port.push(site, returned);
        try {
            port.call_child(target, child);
        } catch (...) {
            // On failure the real child has already published its partial
            // state. The continuation's fault exit must carry that state out.
            gp = port.s.gp;
            flags = port.s.flags;
            throw;
        }
    }

    std::uint32_t datum_offset(std::uint32_t handle) {
        return flags.product(flags.logic(handle & 0xffffu, 4), sizeof(EmitterDatum));
    }

    // Newly allocated emitters carry a handle composed from the actual datum
    // salt and its signed displacement in the data array. The high IMUL
    // result and following shift are part of the live register state.
    void rebuild_handle() {
        gp[ecx] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
            std::uint16_t(port.read(0x251eb6u, gp[edi], 2)))));
        gp[edx] = flags.minus(gp[edi], gp[ebp], 4);
        const auto product = std::int64_t(0x6bca1af3u) * fp::si(gp[edx]);
        gp[eax] = std::uint32_t(product);
        gp[edx] = std::uint32_t(std::uint64_t(product) >> 32u);
        flags.cf = flags.of = product != fp::si(gp[eax]);
        gp[edx] = flags.right(gp[edx], 5u, 4, true);
        gp[eax] = flags.right(gp[edx], 31u, 4, false);
        gp[eax] = flags.plus(gp[eax], gp[edx], 4);

        gp[edx] = port.read(0x251eceu, gp[esp] + 0x28u, 4);
        gp[ecx] = flags.shift(gp[ecx], 16u, 4);
        gp[ebp] = flags.logic(0xffffffffu, 4);
        gp[ecx] = flags.logic(gp[ecx] | gp[eax], 4);
    }

    bool append_emitter() {
        gp[edx] = gp[ebx];
        auto allocator = [&](State &state) { dispatch(state, allocate_emitter, 0x251e73u); };
        call(0x251e73u, 0x251e78u, allocate_emitter, allocator);

        flags.minus(gp[eax], gp[edi], 4);
        if (!flags.zf) {
            gp[edx] = port.read(0x251e7cu, gp[ebx] + 0x44u, 4);
            gp[ecx] = datum_offset(gp[eax]);
            gp[ecx] = flags.plus(gp[ecx], gp[edx], 4);
            port.store(0x251e8cu, gp[ecx] + offsetof(EmitterDatum, next_emitter), 4, gp[edi]);
            port.store(0x251e8fu, gp[ecx] + offsetof(EmitterDatum, particle_count), 2, 0u);
            port.store(0x251e95u, gp[ecx] + offsetof(EmitterDatum, first_particle), 4, gp[edi]);
            port.store(0x251e98u, gp[ecx] + 0xcu, 4, fp::u(_mm_cvtss_f32(port.s.xmm[3])));
        }
        flags.minus(gp[eax], gp[edi], 4);
        gp[esi] = gp[eax];
        if (flags.zf) {
            reached(pool_full);
            return false;
        }
        reached(allocated);

        gp[ebp] = port.read(0x251ea7u, gp[ebx] + 0x44u, 4);
        gp[eax] = datum_offset(gp[eax]);
        gp[eax] = flags.plus(gp[eax], gp[ebp], 4);
        gp[edi] = gp[eax];
        rebuild_handle();
        port.store(0x251edau, gp[edi] + offsetof(EmitterDatum, next_emitter), 4, gp[ebp]);
        flags.minus(port.read(0x251eddu, gp[edx] + offsetof(LocationDatum, first_emitter), 4), gp[ebp], 4);
        if (flags.zf) {
            reached(set_head);
            port.store(0x251ee2u, gp[edx] + offsetof(LocationDatum, first_emitter), 4, gp[ecx]);
        }
        // +8 is the current emitter tail of this LocationDatum. Maintain
        // its predecessor link before replacing it with the new handle.
        gp[eax] = port.read(0x251ee5u, gp[edx] + 8u, 4);
        flags.minus(gp[eax], gp[ebp], 4);
        if (!flags.zf) {
            reached(link_predecessor);
            gp[edi] = port.read(0x251eecu, gp[ebx] + 0x44u, 4);
            gp[eax] = datum_offset(gp[eax]);
            port.store(0x251ef7u, gp[edi] + gp[eax] + offsetof(EmitterDatum, next_emitter), 4, gp[ecx]);
        } else {
            reached(empty_predecessor);
        }
        gp[ebp] = port.read(0x251efbu, gp[esp] + 0x20u, 4);
        port.store(0x251effu, gp[edx] + 8u, 4, gp[ecx]);
        return true;
    }

    bool advance_emitter() {
        gp[ebx] = port.read(0x251e60u, layout::globals::emitters, 4);
        gp[edi] = flags.logic(0xffffffffu, 4);
        flags.minus(gp[esi], gp[edi], 4);
        if (flags.zf) {
            if (!append_emitter()) return false;
        } else {
            reached(existing);
        }

        gp[eax] = port.read(0x251f02u, gp[ebx] + 0x44u, 4);
        gp[edx] = port.read(0x251f05u, gp[esp] + 0x38u, 4);
        gp[ebx] = port.read(0x251f09u, gp[esp] + 0x10u, 4);
        gp[esi] = datum_offset(gp[esi]);
        gp[esi] = flags.plus(gp[esi], gp[eax], 4);
        gp[eax] = port.read(0x251f18u, gp[esp] + 0x30u, 4);
        gp[ecx] = port.read(0x251f1cu, gp[eax] + 0x50u, 4);
        port.push(0x251f1fu, gp[ecx]);
        gp[ecx] = port.read(0x251f20u, gp[esp] + 0x38u, 4);
        port.push(0x251f24u, gp[edx]);
        gp[edx] = port.read(0x251f25u, gp[ebp] + offsetof(SystemDefinition, emitters), 4);
        port.push(0x251f28u, gp[ecx]);
        gp[edx] = flags.plus(gp[edx], gp[ebx], 4);
        port.push(0x251f2bu, gp[edx]);
        port.push(0x251f2cu, gp[eax]);
        gp[eax] = port.read(0x251f2du, gp[esp] + 0x40u, 4);
        port.push(0x251f31u, gp[eax]);
        gp[ecx] = gp[esi];
        call(0x251f34u, 0x251f39u, update_emitter, update);

        gp[eax] = port.read(0x251f39u, gp[esp] + 0x14u, 4);
        gp[ecx] = port.read(0x251f3du, gp[esp] + 0x18u, 4);
        gp[esi] = port.read(0x251f41u, gp[esi] + offsetof(EmitterDatum, next_emitter), 4);
        gp[eax] = flags.increment(gp[eax], 4, false);
        gp[edx] = flags.plus(gp[ebx], sizeof(EmitterDefinition), 4);
        flags.minus(gp[eax], gp[ecx], 4);
        port.store(0x251f4fu, gp[esp] + 0x14u, 4, gp[eax]);
        port.store(0x251f53u, gp[esp] + 0x10u, 4, gp[edx]);
        if (flags.sf == flags.of) {
            reached(finished);
            return false;
        }
        reached(continued);
        port.s.xmm[3] = _mm_setzero_ps();
        return true;
    }

    void run() {
        gp[ecx] = port.read(0x251e30u, gp[edi] + offsetof(LocationDatum, lod), 4);
        gp[edx] = port.read(0x251e33u, gp[esp] + 0x34u, 4);
        port.store(0x251e37u, gp[edx] + offsetof(LocationDatum, lod), 4, gp[ecx]);
        gp[ecx] = port.read(0x251e3au, gp[esp] + 0x18u, 4);
        gp[eax] = flags.logic(0u, 4);
        flags.minus(gp[ecx], gp[eax], 4);
        port.store(0x251e42u, gp[esp] + 0x14u, 4, gp[eax]);
        if (!flags.zf && flags.sf == flags.of) {
            gp[esi] = port.read(0x251e4cu, gp[esp] + 0x1cu, 4);
            port.store(0x251e50u, gp[esp] + 0x10u, 4, 0u);
            while (advance_emitter()) {}
        } else {
            reached(empty);
        }

        gp[edi] = port.pop(0x251f5du);
        gp[esi] = port.pop(0x251f5eu);
        gp[ebp] = port.pop(0x251f5fu);
        gp[ebx] = port.pop(0x251f60u);
        gp[esp] = flags.plus(gp[esp], 0x14u, 4);
        port.ret(last, 20u);
        port.publish(last);
    }
};

template<class State, class Reference, class Dispatch, class Update>
bool bridge(State &state, Reference reference, Dispatch dispatch, Update update) {
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_LOCATION_EMITTERS");
        return value && *value;
    }();
    if (reference_mode || state.active_mmx) return false;
    if (state.instruction_cursor && !state.live_instruction_cursor()) return false;
    if (!native_required_owner_policy_detail::preserves_instruction_policy(state.services)) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication,
        native_particle_birth_observer_detail::qualified)) return false;
    world::execute(state, publication, summary().calls, reference, [&](world::Port<State> &port) {
        Tail tail{port, dispatch, update};
        tail.run();
        coverage().fetch_or(tail.hit, std::memory_order_relaxed);
    });
    return true;
}
} // namespace halo2::native_particle_location_emitters_detail
