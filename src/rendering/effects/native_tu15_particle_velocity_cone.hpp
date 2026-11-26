#pragma once

#include "rendering/effects/native_tu15_particle_velocity_angles.hpp"

namespace halo2::native_particle_velocity_cone_detail {
namespace angles = native_particle_velocity_angles_detail;
namespace properties = native_particle_velocity_properties_detail;
namespace world = native_world_leaves_detail;
namespace fp_math = physics_step_detail;
using world::eax;
using world::ebp;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;
using properties::curve;
using properties::ordered_mul_ss;

inline constexpr std::uint32_t fourth = 0x2526eeu, fifth = 0x2527acu;
inline constexpr std::uint32_t fourth_exit = 0x2527a7u, fifth_exit = 0x2528abu;
inline constexpr std::uint32_t random_direction = 0x148de0u;
inline constexpr world::Function fourth_sites[]{{fourth, fourth_exit, 0u, 0u}};
inline constexpr world::Function fifth_sites[]{{fifth, fifth_exit, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> fourth{}, fifth{};
    ~VerifySummary() {
        if (fourth || fifth)
            std::fprintf(stderr, "Particle velocity cone verify: %llu mode 4, %llu mode 5\n",
                         static_cast<unsigned long long>(fourth.load()),
                         static_cast<unsigned long long>(fifth.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Dispatch, class Trig, class Spill>
struct Cone {
    properties::Properties<State, Direct> &frame;
    angles::Angles<State, Direct, Trig, Spill> &angle;
    Dispatch &dispatch;
    world::Port<State> &port = frame.port;
    std::array<std::uint32_t, 8> &gp = frame.gp;
    typename world::Port<State>::Flags &flags = frame.flags;
    decltype(port.s.xmm) &x = frame.x;
    decltype(port.s.fp) &fp = angle.fp;

    void advance_seed(std::uint32_t store_at, std::uint32_t address) {
        gp[eax] = flags.product(gp[eax], 0x19660du);
        gp[eax] = flags.plus(gp[eax], 0x3c6ef35fu, 4);
        port.store(store_at, address, 4, gp[eax]);
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
        flags.logic(gp[eax], 4);
    }

    void unsigned_sample(std::uint32_t at, std::uint32_t offset, std::uint32_t adjustment, std::uint32_t scale) {
        fp.push(static_cast<long double>(fp_math::si(port.read(at, gp[esp] + offset, 4))));
        if (flags.sf != flags.of)
            fp.st(0) = fp_math::add(fp.st(0), angle.load(adjustment, angles::unsigned_adjustment));
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(scale, angles::random_scale));
    }

    void begin_fourth() {
        gp[ecx] = port.read(fourth, angles::random_state, 4);
        gp[eax] = port.read(0x2526f4u, gp[ecx] + 4u, 4);
        advance_seed(0x252702u, gp[ecx] + 4u);
        port.store(0x25270au, gp[esp] + 0x30u, 4, gp[eax]);
        unsigned_sample(0x25270eu, 0x30u, 0x252714u, 0x25271au);
        gp[esi] = gp[ebp] + 0x8cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252726u, angles::angle_scale));
        angle.store_extended(0x25272cu, gp[esp] + 0x30u);
        fp.pop();
        frame.calls.template call_local<curve>(0x252730u, 0x252735u);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x252735u, angles::spread_scale));
        gp[esi] = gp[ebp] + 0x7cu;
        frame.store(0x252740u, gp[esp] + 0x10u, x[0]);
        frame.calls.template call_local<curve>(0x252746u, 0x25274bu);
    }

    void mode_four() {
        begin_fourth();
        fp.push(angle.load(0x25274bu, gp[esp] + 0x30u));
        angle.trig_top(0x25274fu, false);
        x[1] = _mm_setzero_ps();
        gp[ebp] = gp[edi] + angles::velocity;
        frame.store(0x252757u, gp[edi] + angles::position, x[1]);
        angle.store_extended(0x25275cu, gp[esp] + 0xcu);
        fp.pop();
        fp.push(angle.load(0x252760u, gp[esp] + 0x30u));
        x[1] = frame.scalar(0x252764u, gp[esp] + 0xcu);
        angle.trig_top(0x25276au, true);
        x[1] = ordered_mul_ss(x[1], x[0]);
        frame.store(0x252770u, gp[edi] + angles::position + 4u, x[1]);
        angle.store_extended(0x252775u, gp[esp] + 0x30u);
        fp.pop();
        fp.push(angle.load(0x252779u, gp[esp] + 0x10u));
        x[1] = frame.scalar(0x25277du, gp[esp] + 0x30u);
        angle.trig_top(0x252783u, false);
        x[1] = ordered_mul_ss(x[1], x[0]);
        frame.store(0x252789u, gp[edi] + angles::position + 8u, x[1]);
        fp.push(angle.load(0x25278eu, gp[esp] + 0x10u));
        angle.trig_top(0x252792u, true);
        angle.store_extended(0x252794u, gp[ebp]);
        fp.pop();
        fp.push(angle.load(0x252797u, gp[esp] + 0xcu));
        fp.st(0) = fp_math::mul(fp.st(0), fp.st(1));
        angle.store_extended(0x25279du, gp[edi] + angles::velocity + 4u);
        fp.pop();
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x2527a0u, gp[esp] + 0x30u));
        angle.store_extended(0x2527a4u, gp[edi] + angles::velocity + 8u);
        fp.pop();
        port.publish(fourth_exit);
    }

    void begin_fifth() {
        gp[ecx] = port.read(fifth, angles::random_state, 4);
        gp[eax] = port.read(0x2527b2u, gp[ecx] + 4u, 4);
        gp[ecx] = flags.plus(gp[ecx], 4u, 4);
        advance_seed(0x2527c3u, gp[ecx]);
        port.store(0x2527cau, gp[esp] + 0x18u, 4, gp[eax]);
        port.store(0x2527ceu, gp[esp] + 0x30u, 4, gp[ecx]);
        unsigned_sample(0x2527d2u, 0x18u, 0x2527d8u, 0x2527deu);
        gp[esi] = gp[ebp] + 0x8cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x2527eau, angles::angle_scale));
        angle.store_extended(0x2527f0u, gp[esp] + 0xcu);
        fp.pop();
        frame.calls.template call_local<curve>(0x2527f4u, 0x2527f9u);
        gp[ecx] = port.read(0x2527f9u, gp[esp] + 0x30u, 4);
        gp[eax] = port.read(0x2527fdu, gp[ecx], 4);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x2527ffu, angles::spread_scale));
        advance_seed(0x252812u, gp[ecx]);
        port.store(0x252819u, gp[esp] + 0x14u, 4, gp[eax]);
        frame.store(0x25281du, gp[esp] + 0x18u, x[0]);
        unsigned_sample(0x252823u, 0x14u, 0x252829u, 0x25282fu);
        gp[esi] = gp[ebp] + 0x7cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252838u, gp[esp] + 0x18u));
        angle.store_extended(0x25283cu, gp[esp] + 0x10u);
        angle.trig_top(0x252840u, false);
        angle.store_extended(0x252842u, gp[esp] + 0x14u);
        fp.pop();
        frame.calls.template call_local<curve>(0x252846u, 0x25284bu);
    }

    void mode_five() {
        begin_fifth();
        fp.push(angle.load(0x25284bu, gp[esp] + 0x10u));
        angle.trig_top(0x25284fu, true);
        gp[ebp] = gp[edi] + angles::velocity;
        gp[edx] = port.read(0x252854u, gp[esp] + 0x30u, 4);
        gp[eax] = gp[esp] + 0x1cu;
        angle.store_extended(0x25285cu, gp[ebp]);
        fp.pop();
        fp.push(angle.load(0x25285fu, gp[esp] + 0xcu));
        angle.trig_top(0x252863u, false);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252865u, gp[esp] + 0x14u));
        angle.store_extended(0x252869u, gp[edi] + angles::velocity + 4u);
        fp.pop();
        fp.push(angle.load(0x25286cu, gp[esp] + 0xcu));
        angle.trig_top(0x252870u, true);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252872u, gp[esp] + 0x14u));
        angle.store_extended(0x252876u, gp[edi] + angles::velocity + 8u);
        fp.pop();
        auto child = [&](State &s) { dispatch(s, random_direction, 0x252879u); };
        frame.calls.call(random_direction, 0x252879u, 0x25287eu, child);
        x[1] = ordered_mul_ss(frame.scalar(0x25287eu, gp[esp] + 0x1cu), x[0]);
        frame.store(0x252888u, gp[edi] + angles::position, x[1]);
        x[1] = ordered_mul_ss(frame.scalar(0x25288du, gp[esp] + 0x20u), x[0]);
        frame.store(0x252897u, gp[edi] + angles::position + 4u, x[1]);
        x[1] = ordered_mul_ss(frame.scalar(0x25289cu, gp[esp] + 0x24u), x[0]);
        frame.store(0x2528a6u, gp[edi] + angles::position + 8u, x[1]);
        port.publish(fifth_exit);
    }
};

template<bool Fifth, class State, class Ref, class Direct, class Dispatch, class Trig, class Spill>
bool bridge(State &s, Ref reference, Direct direct, Dispatch dispatch, Trig trig, Spill spill) {
    world::Publication publication;
    if (!properties::admit(s, Fifth ? fifth_sites : fourth_sites, publication)) return false;
    auto &counter = Fifth ? summary().fifth : summary().fourth;
    world::execute(s, publication, counter, reference, [&](auto &port) {
        properties::Properties<State, Direct> frame{port, {port, direct}};
        angles::Angles<State, Direct, Trig, Spill> angle{frame, trig, spill};
        Cone<State, Direct, Dispatch, Trig, Spill> cone{frame, angle, dispatch};
        if constexpr (Fifth) cone.mode_five();
        else cone.mode_four();
    });
    return true;
}
} // namespace halo2::native_particle_velocity_cone_detail
