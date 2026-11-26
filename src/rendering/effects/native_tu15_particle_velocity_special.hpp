#pragma once

#include "rendering/effects/native_tu15_particle_velocity_bounds.hpp"

namespace halo2::native_particle_velocity_special_detail {
namespace angles = native_particle_velocity_angles_detail;
namespace bounds = native_particle_velocity_bounds_detail;
namespace cone = native_particle_velocity_cone_detail;
namespace properties = native_particle_velocity_properties_detail;
namespace world = native_world_leaves_detail;
namespace fp_math = physics_step_detail;
using world::eax;
using world::ebp;
using world::ecx;
using world::edi;
using world::esi;
using world::esp;
using properties::curve;
using properties::ordered_mul_ss;

inline constexpr std::uint32_t eighth = 0x252b19u, alternate = 0x252bd8u;
inline constexpr std::uint32_t eighth_exit = 0x252bd3u, alternate_exit = 0x252cc6u;
inline constexpr std::uint32_t table = 0x252d84u, selection = 0x2524cdu;
inline constexpr world::Function eighth_sites[]{{eighth, eighth_exit, 0u, 0u}};
inline constexpr world::Function alternate_sites[]{{alternate, alternate_exit, 0u, 0u}};
inline constexpr world::Function selection_site[]{{selection, selection, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> eighth{}, alternate{}, selection{};
    ~VerifySummary() {
        if (eighth || alternate || selection)
            std::fprintf(stderr, "Particle velocity special verify: %llu mode 8, %llu alternate, %llu selections\n",
                         static_cast<unsigned long long>(eighth.load()),
                         static_cast<unsigned long long>(alternate.load()),
                         static_cast<unsigned long long>(selection.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Dispatch, class Trig, class Spill>
struct Special {
    bounds::Bounds<State, Direct, Dispatch, Trig, Spill> &bound;
    cone::Cone<State, Direct, Dispatch, Trig, Spill> &sample = bound.sample;
    properties::Properties<State, Direct> &frame = bound.frame;
    angles::Angles<State, Direct, Trig, Spill> &angle = bound.angle;
    world::Port<State> &port = frame.port;
    std::array<std::uint32_t, 8> &gp = frame.gp;
    typename world::Port<State>::Flags &flags = frame.flags;
    decltype(port.s.xmm) &x = frame.x;
    decltype(port.s.fp) &fp = angle.fp;

    void mode_eight() {
        gp[ecx] = port.read(eighth, angles::random_state, 4);
        gp[eax] = port.read(0x252b1fu, gp[ecx] + 4u, 4);
        sample.advance_seed(0x252b2du, gp[ecx] + 4u);
        port.store(0x252b35u, gp[esp] + 0x30u, 4, gp[eax]);
        sample.unsigned_sample(0x252b39u, 0x30u, 0x252b3fu, 0x252b45u);
        gp[esi] = gp[ebp] + 0x7cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252b4eu, angles::angle_scale));
        angle.store_extended(0x252b54u, gp[esp] + 0x30u);
        fp.pop();
        frame.calls.template call_local<curve>(0x252b58u, 0x252b5du);

        gp[ecx] = port.read(0x252b5du, angles::random_state, 4);
        gp[eax] = port.read(0x252b63u, gp[ecx] + 4u, 4);
        sample.advance_seed(0x252b71u, gp[ecx] + 4u);
        port.store(0x252b79u, gp[esp] + 0x14u, 4, gp[eax]);
        frame.store(0x252b7du, gp[esp] + 0x18u, x[0]);
        sample.unsigned_sample(0x252b83u, 0x14u, 0x252b89u, 0x252b8fu);
        x[0] = _mm_setzero_ps();
        gp[ebp] = gp[edi] + angles::velocity;
        frame.store(0x252b9bu, gp[edi] + angles::position, x[0]);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252ba0u, gp[esp] + 0x18u));
        fp.push(angle.load(0x252ba4u, gp[esp] + 0x30u));
        frame.store(0x252ba8u, gp[ebp], x[0]);
        angle.trig_top(0x252badu, false);
        frame.store(0x252bafu, gp[edi] + angles::velocity + 4u, x[0]);
        x[0] = frame.scalar(0x252bb4u, properties::one);
        frame.store(0x252bbcu, gp[edi] + angles::velocity + 8u, x[0]);
        fp.st(0) = fp_math::mul(fp.st(0), fp.st(1));
        angle.store_extended(0x252bc3u, gp[edi] + angles::position + 4u);
        fp.pop();
        fp.push(angle.load(0x252bc6u, gp[esp] + 0x30u));
        angle.trig_top(0x252bcau, true);
        fp.st(0) = fp_math::mul(fp.st(0), fp.st(1));
        angle.store_extended(0x252bceu, gp[edi] + angles::position + 8u);
        fp.pop();
        fp.pop();
        port.publish(eighth_exit);
    }

    void prepare_alternate() {
        gp[ecx] = port.read(alternate, angles::random_state, 4);
        gp[eax] = port.read(0x252bdeu, gp[ecx] + 4u, 4);
        gp[ecx] = flags.plus(gp[ecx], 4u, 4);
        sample.advance_seed(0x252befu, gp[ecx]);
        port.store(0x252bf6u, gp[esp] + 0x18u, 4, gp[eax]);
        port.store(0x252bfau, gp[esp] + 0x30u, 4, gp[ecx]);
        sample.unsigned_sample(0x252bfeu, 0x18u, 0x252c04u, 0x252c0au);
        gp[esi] = gp[ebp] + 0x8cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252c16u, angles::angle_scale));
        angle.store_extended(0x252c1cu, gp[esp] + 0x14u);
        fp.pop();
        frame.calls.template call_local<curve>(0x252c20u, 0x252c25u);

        gp[ecx] = port.read(0x252c25u, gp[esp] + 0x30u, 4);
        gp[eax] = port.read(0x252c29u, gp[ecx], 4);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x252c2bu, angles::spread_scale));
        sample.advance_seed(0x252c3eu, gp[ecx]);
        port.store(0x252c45u, gp[esp] + 0x10u, 4, gp[eax]);
        frame.store(0x252c49u, gp[esp] + 0x18u, x[0]);
        sample.unsigned_sample(0x252c4fu, 0x10u, 0x252c55u, 0x252c5bu);
        gp[esi] = gp[ebp] + 0x7cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252c64u, gp[esp] + 0x18u));
        angle.store_extended(0x252c68u, gp[esp] + 0x18u);
        angle.trig_top(0x252c6cu, false);
        angle.store_extended(0x252c6eu, gp[esp] + 0x10u);
        fp.pop();
        frame.calls.template call_local<curve>(0x252c72u, 0x252c77u);
    }

    void mode_alternate() {
        prepare_alternate();
        x[1] = _mm_setzero_ps();
        frame.store(0x252c7au, gp[esp] + 0xcu, x[0]);
        gp[ecx] = port.read(0x252c80u, gp[esp] + 0xcu, 4);
        port.push(0x252c84u, gp[ecx]);
        port.push(0x252c85u, gp[ecx]);
        gp[ecx] = port.read(0x252c86u, gp[esp] + 0x38u, 4);
        frame.store(0x252c8au, gp[edi] + angles::position, x[1]);
        x[1] = _mm_sub_ss(x[1], x[0]);
        frame.store(0x252c93u, gp[esp], x[1]);
        bound.range(0x252c98u, 0x252c9du);
        angle.store_extended(0x252c9du, gp[edi] + angles::position + 4u);
        fp.pop();
        fp.push(angle.load(0x252ca0u, gp[esp] + 0x18u));
        gp[ebp] = gp[edi] + angles::velocity;
        angle.trig_top(0x252ca7u, true);
        x[0] = _mm_setzero_ps();
        angle.store_extended(0x252cacu, gp[ebp]);
        fp.pop();
        fp.push(angle.load(0x252cafu, gp[esp] + 0x14u));
        angle.trig_top(0x252cb3u, false);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252cb5u, gp[esp] + 0x10u));
        angle.store_extended(0x252cb9u, gp[edi] + angles::velocity + 4u);
        fp.pop();
        fp.push(angle.load(0x252cbcu, gp[esp] + 0x14u));
        angle.trig_top(0x252cc0u, true);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252cc2u, gp[esp] + 0x10u));
        angle.store_extended(alternate_exit, gp[edi] + angles::velocity + 8u);
        fp.pop();
        port.publish(alternate_exit);
    }
};

template<bool Alternate, class State, class Ref, class Direct, class Dispatch, class Trig, class Spill>
bool bridge(State &s, Ref reference, Direct direct, Dispatch dispatch, Trig trig, Spill spill) {
    world::Publication publication;
    if (!properties::admit(s, Alternate ? alternate_sites : eighth_sites, publication)) return false;
    auto &counter = Alternate ? summary().alternate : summary().eighth;
    world::execute(s, publication, counter, reference, [&](auto &port) {
        properties::Properties<State, Direct> frame{port, {port, direct}};
        angles::Angles<State, Direct, Trig, Spill> angle{frame, trig, spill};
        cone::Cone<State, Direct, Dispatch, Trig, Spill> sample{frame, angle, dispatch};
        bounds::Bounds<State, Direct, Dispatch, Trig, Spill> bound{sample};
        Special<State, Direct, Dispatch, Trig, Spill> special{bound};
        if constexpr (Alternate) special.mode_alternate();
        else special.mode_eight();
    });
    return true;
}

template<class State, class Ref, class Canonical>
bool select(State &s, std::uint32_t &target, Ref reference, Canonical canonical_branch) {
    world::Publication publication;
    if (!properties::admit(s, selection_site, publication)) return false;
    world::execute(s, publication, summary().selection, [&](State &original) {
        if (reference(original) != target)
            throw std::runtime_error("Particle velocity mode dispatch target differs from the translated body");
    }, [&](auto &port) {
        target = canonical_branch(port.read(selection, table + port.gp[eax] * 4u, 4), s.image);
        port.publish(selection);
    });
    return true;
}
} // namespace halo2::native_particle_velocity_special_detail
