#include "halo2/physics_world_step.hpp"
#include "halo2/physics_contact_batches.hpp"
#include "physics_step_internal.hpp"
#include <algorithm>

namespace halo2 {
using namespace physics_step_detail;
namespace {
using S = SourcePhysicsWorldStepServices;
std::uint32_t required(M &m, const S &s, std::uint32_t site, std::uint32_t entry,
    std::optional<std::uint32_t> ecx, std::uint32_t esp,
    std::vector<std::uint32_t> args, const O &o) {
    if (!s.required_owner) throw std::runtime_error("Required original physics world/solver owner unavailable");
    return s.required_owner(m, {site, entry, esp, ecx, std::move(args)}, o);
}
struct Profile {
    std::uint32_t label_site, label, counter_site, counter_address, value_site, cursor_site;
};
void profile(M &m, const S &s, const Profile &p, const O &o) {
    const auto cursor = r(m, 0x487674);
    if (cursor >= r(m, 0x487678)) return;
    w(m, o, p.label_site, cursor, 4, p.label);
    if (!s.timestamp_counter) throw std::runtime_error("Required original physics profiling RDTSC owner unavailable");
    const auto ticks = std::uint32_t(s.timestamp_counter());
    w(m, o, p.counter_site, p.counter_address, 4, ticks);
    w(m, o, p.value_site, cursor + 4, 4, r(m, p.counter_address));
    w(m, o, p.cursor_site, 0x487674, 4, cursor + 12u);
}
// Original reverse traversal, followed by stable NULL removal. Providers can
// modify the bank; reads are deliberately repeated at the source sites.
void listeners(M &m, std::uint32_t world, std::uint32_t step,
    std::optional<std::uint32_t> island, std::uint32_t entry, std::uint32_t esp,
    const S &s, const O &o) {
    std::uint32_t offset, callsite, countsite, copysite;
    switch (entry) {
    case 0x31ed30: offset = 0x90; callsite = 0x31ed55; countsite = 0x31ed7b; copysite = 0x31ed9d; break;
    case 0x31ec90: offset = 0x84; callsite = 0x31ecb5; countsite = 0x31ecdb; copysite = 0x31ecfd; break;
    case 0x31ee50: offset = 0xa8; callsite = 0x31ee81; countsite = 0x31eea8; copysite = 0x31eebf; break;
    case 0x31edc0: offset = 0x9c; callsite = 0x31edf1; countsite = 0x31ee18; copysite = 0x31ee2f; break;
    default: throw std::runtime_error("Unknown original physics world listener owner");
    }
    for (auto i = r(m, world + offset + 4) - 1u; si(i) >= 0; --i) {
        const auto listener = r(m, r(m, world + offset) + i * 4u);
        if (listener) required(m, s, callsite, r(m, r(m, listener) + 4), listener,
            esp - (island ? 28u : 24u), {island.value_or(world), step}, o);
    }
    for (auto i = r(m, world + offset + 4) - 1u; si(i) >= 0; --i) {
        if (!r(m, r(m, world + offset) + i * 4u)) {
            w(m, o, countsite, world + offset + 4, 4, r(m, world + offset + 4) - 1u);
            for (auto k = i; si(k) < si(r(m, world + offset + 4)); ++k) {
                const auto bank = r(m, world + offset);
                w(m, o, copysite, bank + k * 4u, 4, r(m, bank + k * 4u + 4u));
            }
        }
    }
    if (entry == 0x31ec90) {
        const auto listener = r(m, world + 0xc0); // Complete3126E0.
        if (listener) required(m, s, 0x31ed1e, r(m, r(m, listener) + 4), listener,
            esp - 24u, {world, step}, o);
    }
}
void step_fields(M &m, std::uint32_t world, std::uint32_t step, bool collision, const O &o) {
    const auto base = collision ? 0x316200u : 0x316440u;
    w(m, o, base + (collision ? 0x3fu : 0x38u), world + 0x150, 4, r(m, step));
    w(m, o, base + (collision ? 0x48u : 0x41u), world + 0x154, 4, r(m, step + 4));
    if (collision) {
        const auto from = r(m, world + 0xcc);
        constexpr std::array<std::uint32_t, 4> sites{0x316258, 0x31625d, 0x316263, 0x316269};
        for (unsigned i = 0; i < 4; ++i)
            w(m, o, sites[i], world + 0x140u + i * 4u, 4, r(m, from + i * 4u));
    }
    const auto dt = ext(f(r(m, step)));
    w(m, o, collision ? 0x316275 : 0x31648f, world + 0x170, 4,
        u(spill(mul(dt, ext(f(r(m, world + 0x180)))))));
    const auto scaled_dt = r(m, world + 0x170);
    w(m, o, collision ? 0x31628a : 0x3164a4, world + 0x174, 4,
        u(spill(mul(static_cast<long double>(si(r(m, world + 0x17c))), ext(f(r(m, step + 4)))))));
    w(m, o, collision ? 0x316290 : 0x3164aa, world + 0x198, 4, scaled_dt);
    w(m, o, collision ? 0x31629c : 0x3164b6, world + 0x19c, 4, r(m, world + 0x174));
    w(m, o, collision ? 0x3162ae : 0x3164c8, world + 0x1a0, 4,
        u(spill(mul(static_cast<long double>(si(r(m, world + 0x17c))), ext(f(r(m, world + 0x170)))))));
    w(m, o, collision ? 0x3162c0 : 0x3164da, world + 0x1a4, 4,
        u(spill(mul(ext(f(r(m, world + 0x180))), ext(f(r(m, world + 0x174)))))));
}
void pending(M &m, const S &s, std::uint32_t world, std::uint32_t esp,
    std::uint32_t count, std::uint32_t site, std::uint32_t owner, const O &o) {
    if (r(m, world + count)) required(m, s, site, owner, world, esp, {}, o);
}
}
void source_integrate_physics_island_1_1(M &m, std::uint32_t island,
    std::uint32_t step, std::uint32_t esp, FP fp, const S &s, const O &o) {
    FpScope scope(fp); const auto frame = esp - 20u;
    auto cursor = r(m, island + 0x4c);
    const auto end = cursor + r(m, island + 0x50) * 4u;
    while (cursor != end) {
        const auto action = r(m, cursor);
        required(m, s, 0x31d005, r(m, r(m, action) + 20), action, frame - 8u, {step + 16u}, o);
        cursor += 4u;
    }
    const auto constraints = r(m, island + 0x5c);
    if (!constraints) {
        profile(m, s, {0x31d028, 0x41b5f8, 0x31d030, frame + 16, 0x31d038, 0x31d03e}, o);
        for (auto i = r(m, island + 0x40) - 1u; si(i) >= 0; --i) {
            const auto body = r(m, r(m, island + 0x3c) + i * 4u), motion = r(m, body + 0x3c);
            const auto gravity = r(m, island + 0x20) + 0xe0, force_owner = r(m, r(m, motion) + 8);
            if (force_owner == 0x321760) source_apply_physics_motion_forces_1_1(m, motion, step + 16, gravity, frame - 12u, fp, o);
            else required(m, s, 0x31d069, force_owner, motion, frame - 12u, {step + 16, gravity}, o);
            const auto integrate_owner = r(m, r(m, motion) + 4);
            if (integrate_owner == 0x321600) source_integrate_physics_motion_1_1(m, motion, step + 16, frame - 8u, fp, s.reciprocal_sqrt, o);
            else required(m, s, 0x31d071, integrate_owner, motion, frame - 8u, {step + 16}, o);
        }
        profile(m, s, {0x31d085, 0x4188f4, 0x31d08d, frame + 24, 0x31d097, 0x31d09e}, o);
    } else {
        const SourcePhysicsConstraintStepInput input{step + 16u, step + 32u, step + 80u,
            r(m, island + 0x20) + 0xe0, island, r(m, island + 0x3c), r(m, island + 0x40), r(m, island + 0x58), constraints};
        if (s.constrained_islands)
            source_step_physics_constraints_1_1(m, input, frame - 40u, fp, *s.constrained_islands, o);
        else required(m, s, 0x31d0d1, 0x322690, std::nullopt, frame - 40u,
            {input.step, input.solver_configuration, input.jacobian_context, input.gravity,
             input.island, input.bodies, input.body_count, input.constraints, input.constraint_count}, o);
    }
}
std::uint32_t source_check_physics_island_deactivation_1_1(M &m, std::uint32_t island,
    std::uint32_t output, std::uint32_t step, std::uint32_t esp, FP fp, const S &s, const O &o) {
    FpScope scope(fp); const auto frame = esp - 12u;
    const auto dt = ext(f(r(m, step)));
    w(m, o, 0x31d101, island + 0x34, 4, u(spill(add(dt, ext(f(r(m, island + 0x34)))))));
    const auto world = r(m, island + 0x20);
    w(m, o, 0x31d109, frame + 20, 4, u(spill(add(ext(f(r(m, island + 0x38))), dt))));
    const auto age = r(m, frame + 20);
    w(m, o, 0x31d114, island + 0x38, 4, age);
    const bool short_elapsed = ext(f(r(m, island + 0x34))) > ext(f(r(m, world + 0x30)));
    const bool long_elapsed = ext(f(age)) > ext(f(r(m, world + 0x34)));
    const auto all_ready = [&](bool short_test) {
        bool ready = true;
        for (std::uint32_t i = 0; si(i) < si(r(m, island + 0x40)); ++i) {
            const auto body = r(m, r(m, island + 0x3c) + i * 4u), deactivator = r(m, body + 0x54);
            if (!deactivator) { ready = false; continue; }
            const auto result = required(m, s, short_test ? 0x31d170 : 0x31d1ed,
                r(m, r(m, deactivator) + (short_test ? 20u : 24u)), deactivator,
                frame - 12u, {frame + 20u, body}, o);
            if (!r(m, result, 1)) ready = false;
        }
        return ready;
    };
    if (short_elapsed) {
        w(m, o, 0x31d14e, island + 0x34, 4,
            u(spill(sub(ext(f(r(m, island + 0x34))), ext(f(r(m, world + 0x30)))))));
        if (all_ready(true)) {
            const auto counter = std::bit_cast<std::int8_t>(std::uint8_t(r(m, island + 0x2c, 1)));
            if (counter < 5) {
                w(m, o, 0x31d18f, island + 0x2c, 1, std::uint8_t(counter) + 1u);
                w(m, o, 0x31d198, output, 1, 0); return output;
            }
            w(m, o, 0x31d19f, island + 0x2c, 1, 0);
            w(m, o, 0x31d1a9, output, 1, 1); return output;
        }
    } else if (long_elapsed) {
        w(m, o, 0x31d1c4, island + 0x38, 4, u(spill(sub(ext(f(age)), ext(f(r(m, world + 0x34)))))));
        if (all_ready(false)) { w(m, o, 0x31d1a9, output, 1, 1); return output; }
    }
    w(m, o, 0x31d209, output, 1, 0); return output;
}
void source_collide_physics_world_1_1(M &m, std::uint32_t world, std::uint32_t step,
    std::uint32_t esp, FP fp, const S &s, const O &o) {
    FpScope scope(fp); const auto frame = esp - 20u;
    auto agents = s.agents;
    if (s.current_broadphase || s.contact_dispatch) {
        agents.required_owner = [&, original = s.agents.required_owner](M &memory,
            const SourcePhysicsAgentOwnerCall &call, const O &observer) -> std::uint32_t {
            if (call.entry == 0x31db70 && s.current_broadphase) {
                if (call.cdecl || call.arguments.size() != 2)
                    throw std::runtime_error("Invalid original current broadphase ABI");
                source_update_physics_island_broadphase_1_1(memory, call.receiver,
                    call.arguments[0], call.arguments[1], call.entry_esp, fp,
                    *s.current_broadphase, observer);
                // Original31DB70 returns void;31E2C0 discards the generic
                // callback value. No collision/admission result is supplied.
                return {};
            }
            if (call.entry == 0x300870 && s.contact_dispatch) {
                if (call.cdecl || call.arguments.size() != 4)
                    throw std::runtime_error("Invalid original contact dispatcher ABI");
                std::array<std::uint32_t, 4> args{};
                std::copy_n(call.arguments.begin(), 4, args.begin());
                source_dispatch_physics_contacts_1_1(memory, call.receiver, args,
                    call.entry_esp, fp, s.reciprocal_sqrt, *s.contact_dispatch, observer);
                return {}; // Original void agent virtual; no admission result.
            }
            if (!original) throw std::runtime_error("Required original physics agent owner unavailable");
            return original(memory, call, observer);
        };
    }
    profile(m, s, {0x316216, 0x419f64, 0x31621e, frame + 16, 0x316226, 0x31622c}, o);
    step_fields(m, world, step, true, o);
    w(m, o, 0x3162cb, world + 0x12c, 1, 1);
    if (si(r(m, world + 0x24)) > 0) required(m, s, 0x3162d6, 0x315c00, world, frame - 4u, {}, o);
    w(m, o, 0x3162e2, world + 0x12d, 1, 1);
    for (std::uint32_t i = 0; si(i) < si(r(m, world + 12)); ++i) {
        const auto island = r(m, r(m, world + 8) + i * 4u);
        source_schedule_physics_island_agents_1_1(m, island, r(m, world + 0xc4), world + 0x140,
            frame - 12u, fp, agents, o);
        if (r(m, world + 0xac)) {
            profile(m, s, {0x31631b, 0x419f4c, 0x316323, frame + 24, 0x31632b, 0x316331}, o);
            w(m, o, 0x31633a, world + 0x12d, 1, 0);
            listeners(m, world, step, r(m, r(m, world + 8) + i * 4u), 0x31ee50, frame - 16u, s, o);
            w(m, o, 0x31634c, world + 0x12d, 1, 1);
            profile(m, s, {0x316365, 0x4188f4, 0x31636d, frame + 16, 0x316375, 0x31637b}, o);
        }
    }
    const auto operations = r(m, world + 0x24);
    w(m, o, 0x316392, world + 0x12d, 1, 0); w(m, o, 0x316399, world + 0x12c, 1, 0);
    if (si(operations) > 0) required(m, s, 0x3163a4, 0x315c00, world, frame - 4u, {}, o);
    pending(m, s, world, frame - 4u, 0xf4, 0x3163b5, 0x316180, o);
    pending(m, s, world, frame - 4u, 0x100, 0x3163c6, 0x313290, o);
    if (si(r(m, world + 0x24)) > 0) required(m, s, 0x3163d4, 0x315c00, world, frame - 4u, {}, o);
    profile(m, s, {0x3163e7, 0x419f3c, 0x3163ef, frame + 24, 0x3163f7, 0x3163fd}, o);
    listeners(m, world, step, std::nullopt, 0x31ed30, frame - 12u, s, o);
    profile(m, s, {0x316420, 0x41903c, 0x316428, esp + 4u, 0x316430, 0x316436}, o);
}
void source_integrate_physics_world_1_1(M &m, std::uint32_t world, std::uint32_t step,
    std::uint32_t esp, FP fp, const S &s, const O &o) {
    FpScope scope(fp); const auto frame = esp - 16u;
    profile(m, s, {0x316456, 0x419fa0, 0x31645e, frame + 8, 0x316466, 0x31646c}, o);
    step_fields(m, world, step, false, o); // Original187550 is a bare RET.
    if (si(r(m, world + 0x24)) > 0) required(m, s, 0x3164ee, 0x315c00, world, frame - 4u, {}, o);
    pending(m, s, world, frame - 4u, 0x10c, 0x3164ff, 0x313310, o);
    auto index = r(m, world + 12) - 1u;
    w(m, o, 0x31650a, world + 0x10c, 4, 0); w(m, o, 0x316514, world + 0x12c, 1, 1);
    for (; si(index) >= 0; --index) {
        const auto island = r(m, r(m, world + 8) + index * 4u);
        bool deactivate = false;
        if (r(m, world + 0x23d, 1)) {
            const auto result = source_check_physics_island_deactivation_1_1(m, island,
                frame + 4u, step, frame - 20u, fp, s, o);
            deactivate = r(m, result, 1) != 0;
        }
        if (deactivate) required(m, s, 0x316546, 0x313150, world, frame - 16u, {island}, o);
        else {
            source_integrate_physics_island_1_1(m, island, world + 0x140, frame - 16u, fp, s, o);
            w(m, o, 0x31655b, island + 0x30, 1, 0);
        }
        if (r(m, world + 0xa0)) {
            profile(m, s, {0x316577, 0x419f8c, 0x31657f, frame + 8, 0x316587, 0x31658d}, o);
            listeners(m, world, step, island, 0x31edc0, frame - 24u, s, o);
            profile(m, s, {0x3165ad, 0x4188f4, 0x3165b5, frame + 12, 0x3165bd, 0x3165c3}, o);
        }
    }
    if (r(m, world + 0x23c, 1)) required(m, s, 0x3165dc, 0x314c70, world, frame - 12u, {}, o);
    const auto pending_count = r(m, world + 0xf4);
    w(m, o, 0x3165e9, world + 0x12c, 1, 0);
    if (pending_count) required(m, s, 0x3165f4, 0x316180, world, frame - 12u, {}, o);
    pending(m, s, world, frame - 12u, 0x100, 0x316605, 0x313290, o);
    for (std::uint32_t i = 0; si(i) < si(r(m, world + 0x118)); ++i) {
        const auto body = r(m, r(m, world + 0x114) + i * 4u);
        required(m, s, 0x316629, 0x313950, body, frame - 12u, {}, o);
        const auto retained = r(m, r(m, world + 0x114) + i * 4u);
        w(m, o, 0x316637, retained + 6, 2, r(m, retained + 6, 2) - 1u);
        if (!r(m, retained + 6, 2)) {
            const auto resident = required(m, s, 0x316643, 0x27f9f0, std::nullopt, frame - 16u, {retained}, o);
            if (resident & 255u) w(m, o, 0x31664f, retained + 6, 2, 1);
            else required(m, s, 0x31665d, r(m, r(m, retained)), retained, frame - 16u, {1}, o);
        }
    }
    w(m, o, 0x31666a, world + 0x118, 4, 0);
    // The original second profiling entry is nested under the first capacity check.
    const auto available = r(m, 0x487674) < r(m, 0x487678);
    if (available) {
        profile(m, s, {0x316684, 0x41903c, 0x31668c, frame + 20, 0x316694, 0x3166a1}, o);
        profile(m, s, {0x3166a9, 0x419f78, 0x3166b1, frame + 20, 0x3166b9, 0x3166bf}, o);
    }
    listeners(m, world, step, std::nullopt, 0x31ec90, frame - 12u, s, o);
    profile(m, s, {0x3166e0, 0x4188f4, 0x3166e8, esp + 4u, 0x3166f0, 0x3166f6}, o);
}
void source_step_physics_world_1_1(M &m, std::uint32_t world, std::uint32_t step,
    std::uint32_t esp, FP fp, const S &s, const O &o) {
    source_collide_physics_world_1_1(m, world, step, esp - 16u, fp, s, o);
    source_integrate_physics_world_1_1(m, world, step, esp - 16u, fp, s, o);
}
} // namespace halo2
