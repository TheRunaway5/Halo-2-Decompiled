#pragma once

#include "rendering/effects/native_tu15_particle_velocity_cone.hpp"

namespace halo2::native_particle_velocity_bounds_detail {
namespace cone = native_particle_velocity_cone_detail;
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

inline constexpr std::uint32_t sixth = 0x2528b0u, seventh = 0x252a12u;
inline constexpr std::uint32_t sixth_x_exit = 0x2529b3u, sixth_y_exit = 0x252a0du, seventh_exit = 0x252b14u;
inline constexpr std::uint32_t effects = 0x4f720cu, fallback_bounds = 0x474cc0u, random_range = 0x259b0u;
inline constexpr std::uint32_t array_offset = offsetof(native_effects::DataArray, data);
inline constexpr world::Function sixth_sites[]{{sixth, sixth_y_exit, 0u, 0u}};
inline constexpr world::Function seventh_sites[]{{seventh, seventh_exit, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> sixth{}, seventh{};
    ~VerifySummary() {
        if (sixth || seventh)
            std::fprintf(stderr, "Particle velocity bounds verify: %llu mode 6, %llu mode 7\n",
                         static_cast<unsigned long long>(sixth.load()),
                         static_cast<unsigned long long>(seventh.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct, class Dispatch, class Trig, class Spill>
struct Bounds {
    cone::Cone<State, Direct, Dispatch, Trig, Spill> &sample;
    properties::Properties<State, Direct> &frame = sample.frame;
    angles::Angles<State, Direct, Trig, Spill> &angle = sample.angle;
    world::Port<State> &port = frame.port;
    std::array<std::uint32_t, 8> &gp = frame.gp;
    typename world::Port<State>::Flags &flags = frame.flags;
    decltype(port.s.xmm) &x = frame.x;
    decltype(port.s.fp) &fp = angle.fp;

    void range(std::uint32_t at, std::uint32_t returned) {
        auto child = [&](State &s) { sample.dispatch(s, random_range, at); };
        frame.calls.call(random_range, at, returned, child);
    }

    // Mode 6 chooses an axis and a side, then samples the other coordinate.
    bool choose_face() {
        gp[ecx] = port.read(sixth, gp[esp] + 0x30u, 4);
        gp[eax] = port.read(0x2528b4u, gp[ecx] + 0x18u, 4);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf) {
            gp[eax] = port.read(0x2528dau, fallback_bounds, 4);
            port.store(0x2528dfu, gp[esp] + 0x30u, 4, gp[eax]);
        } else {
            gp[edx] = port.read(0x2528bcu, effects, 4);
            gp[ecx] = port.read(0x2528c2u, gp[edx] + array_offset, 4);
            gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
            gp[eax] = flags.product(gp[eax], 0x190u);
            gp[edx] = gp[eax] + gp[ecx] + 0x74u;
            port.store(0x2528d4u, gp[esp] + 0x30u, 4, gp[edx]);
        }
        gp[esi] = gp[ebp] + 0x8cu;
        frame.calls.template call_local<curve>(0x2528e9u, 0x2528eeu);
        gp[ecx] = port.read(0x2528eeu, angles::random_state, 4);
        gp[eax] = port.read(0x2528f4u, gp[ecx] + 4u, 4);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x2528f7u, angles::spread_scale));
        gp[eax] = flags.product(gp[eax], 0x19660du);
        x[1] = _mm_setzero_ps();
        gp[ecx] = flags.plus(gp[ecx], 4u, 4);
        gp[eax] = flags.plus(gp[eax], 0x3c6ef35fu, 4);
        port.store(0x252910u, gp[ecx], 4, gp[eax]);
        gp[eax] = flags.right(gp[eax], 16u, 4, false);
        gp[eax] = flags.logic(gp[eax] & 1u, 4);
        frame.store(0x252918u, gp[esp] + 0xcu, x[0]);
        x[0] = _mm_cvtsi32_ss(x[0], fp_math::si(gp[eax]));
        flags.scalar_compare(x[0], x[1], true);
        gp[eax] = (gp[eax] & 0xffff00ffu) | ((flags.bits() | 2u) & 0xffu) << 8u;
        flags.logic((gp[eax] >> 8u) & 0x44u, 1);
        gp[eax] = (gp[eax] & 0xffffff00u) | (flags.pf ? flags.logic(0u, 1) : 1u);
        fp.push(angle.load(0x252931u, gp[esp] + 0xcu));
        x[0] = frame.scalar(0x252935u, 0x46a0dcu);
        angle.trig_top(0x25293du, true);
        gp[ebp] = gp[edi] + angles::velocity;
        frame.store(0x252942u, gp[edi] + angles::position, x[1]);
        angle.store_extended(0x252947u, gp[ebp]);
        fp.pop();
        gp[esi] = port.read(0x25294au, gp[ecx], 4);
        gp[esi] = flags.product(gp[esi], 0x19660du);
        gp[esi] = flags.plus(gp[esi], 0x3c6ef35fu, 4);
        flags.logic(gp[eax] & 0xffu, 1);
        port.store(0x25295au, gp[ecx], 4, gp[esi]);
        return !flags.zf;
    }

    void first_axis() {
        gp[edx] = flags.right(gp[esi], 16u, 4, false);
        flags.logic(gp[edx] & 1u, 1);
        if (flags.zf) x[0] = frame.scalar(0x252968u, properties::one);
        gp[eax] = port.read(0x252970u, gp[esp] + 0x30u, 4);
        x[2] = ordered_mul_ss(frame.scalar(0x252974u, gp[eax]), x[0]);
        frame.store(0x25297cu, gp[edi] + angles::position + 4u, x[2]);
        gp[edx] = port.read(0x252981u, gp[eax] + 4u, 4);
        x[1] = _mm_sub_ss(x[1], frame.scalar(0x252984u, gp[eax] + 4u));
        port.push(0x252989u, gp[edx]);
        port.push(0x25298au, gp[ecx]);
        frame.store(0x25298bu, gp[esp] + 0x1cu, x[0]);
        frame.store(0x252991u, gp[esp], x[1]);
        range(0x252996u, 0x25299bu);
        angle.store_extended(0x25299bu, gp[edi] + angles::position + 8u);
        fp.pop();
        fp.push(angle.load(0x25299eu, gp[esp] + 0xcu));
        x[0] = _mm_setzero_ps();
        angle.trig_top(0x2529a5u, false);
        frame.store(0x2529a7u, gp[edi] + angles::velocity + 8u, x[0]);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x2529acu, gp[esp] + 0x14u));
        angle.store_extended(0x2529b0u, gp[edi] + angles::velocity + 4u);
        fp.pop();
        port.publish(sixth_x_exit);
    }

    void second_axis() {
        gp[eax] = flags.right(gp[esi], 16u, 4, false);
        flags.logic(gp[eax] & 1u, 1);
        if (flags.zf) x[0] = frame.scalar(0x2529c1u, properties::one);
        gp[esi] = port.read(0x2529c9u, gp[esp] + 0x30u, 4);
        gp[edx] = port.read(0x2529cdu, gp[esi], 4);
        x[1] = _mm_sub_ss(x[1], frame.scalar(0x2529cfu, gp[esi]));
        port.push(0x2529d3u, gp[edx]);
        port.push(0x2529d4u, gp[ecx]);
        frame.store(0x2529d5u, gp[esp] + 0x18u, x[0]);
        frame.store(0x2529dbu, gp[esp], x[1]);
        range(0x2529e0u, 0x2529e5u);
        angle.store_extended(0x2529e5u, gp[edi] + angles::position + 4u);
        fp.pop();
        fp.push(angle.load(0x2529e8u, gp[esp] + 0xcu));
        x[0] = frame.scalar(0x2529ecu, gp[esi] + 4u);
        angle.trig_top(0x2529f1u, false);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x2529f3u, gp[esp] + 0x10u));
        frame.store(0x2529f9u, gp[edi] + angles::position + 8u, x[0]);
        x[0] = _mm_setzero_ps();
        frame.store(0x252a01u, gp[edi] + angles::velocity + 4u, x[0]);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252a06u, gp[esp] + 0x10u));
        angle.store_extended(0x252a0au, gp[edi] + angles::velocity + 8u);
        fp.pop();
        port.publish(sixth_y_exit);
    }

    void mode_six() {
        if (choose_face()) first_axis();
        else second_axis();
    }

    void prepare_seventh() {
        gp[eax] = port.read(seventh, gp[esp] + 0x30u, 4);
        gp[eax] = port.read(0x252a16u, gp[eax] + 0x18u, 4);
        flags.minus(gp[eax], 0xffffffffu, 4);
        if (flags.zf) {
            gp[ecx] = port.read(0x252a3cu, fallback_bounds, 4);
            port.store(0x252a42u, gp[esp] + 0x14u, 4, gp[ecx]);
        } else {
            gp[ecx] = port.read(0x252a1eu, effects, 4);
            gp[edx] = port.read(0x252a24u, gp[ecx] + array_offset, 4);
            gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
            gp[eax] = flags.product(gp[eax], 0x190u);
            gp[eax] += gp[edx] + 0x74u;
            port.store(0x252a36u, gp[esp] + 0x14u, 4, gp[eax]);
        }
        gp[ecx] = port.read(0x252a46u, angles::random_state, 4);
        gp[eax] = port.read(0x252a4cu, gp[ecx] + 4u, 4);
        gp[ecx] = flags.plus(gp[ecx], 4u, 4);
        sample.advance_seed(0x252a5du, gp[ecx]);
        port.store(0x252a64u, gp[esp] + 0x18u, 4, gp[eax]);
        port.store(0x252a68u, gp[esp] + 0x30u, 4, gp[ecx]);
        sample.unsigned_sample(0x252a6cu, 0x18u, 0x252a72u, 0x252a78u);
        gp[esi] = gp[ebp] + 0x8cu;
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252a84u, angles::angle_scale));
        angle.store_extended(0x252a8au, gp[esp] + 0xcu);
        fp.pop();
        frame.calls.template call_local<curve>(0x252a8eu, 0x252a93u);
        x[0] = ordered_mul_ss(x[0], frame.scalar(0x252a93u, angles::spread_scale));
        frame.store(0x252a9bu, gp[esp] + 0x10u, x[0]);
    }

    void sample_seventh_coordinates() {
        fp.push(angle.load(0x252aa1u, gp[esp] + 0x10u));
        x[0] = _mm_setzero_ps();
        angle.trig_top(0x252aa8u, false);
        gp[esi] = port.read(0x252aaau, gp[esp] + 0x14u, 4);
        gp[ebp] = port.read(0x252aaeu, gp[esp] + 0x30u, 4);
        frame.store(0x252ab2u, gp[edi] + angles::position, x[0]);
        gp[edx] = port.read(0x252ab7u, gp[esi], 4);
        x[0] = _mm_sub_ss(x[0], frame.scalar(0x252ab9u, gp[esi]));
        port.push(0x252abdu, gp[edx]);
        port.push(0x252abeu, gp[ecx]);
        gp[ecx] = gp[ebp];
        frame.store(0x252ac1u, gp[esp], x[0]);
        angle.store_extended(0x252ac6u, gp[esp] + 0x20u);
        fp.pop();
        range(0x252acau, 0x252acfu);
        angle.store_extended(0x252acfu, gp[edi] + angles::position + 4u);
        fp.pop();
        gp[eax] = port.read(0x252ad2u, gp[esi] + 4u, 4);
        x[0] = _mm_setzero_ps();
        x[0] = _mm_sub_ss(x[0], frame.scalar(0x252ad8u, gp[esi] + 4u));
        port.push(0x252addu, gp[eax]);
        port.push(0x252adeu, gp[ecx]);
        gp[ecx] = gp[ebp];
        frame.store(0x252ae1u, gp[esp], x[0]);
        range(0x252ae6u, 0x252aebu);
        angle.store_extended(0x252aebu, gp[edi] + angles::position + 8u);
        fp.pop();
    }

    void mode_seven() {
        prepare_seventh();
        sample_seventh_coordinates();
        fp.push(angle.load(0x252aeeu, gp[esp] + 0x10u));
        gp[ebp] = gp[edi] + angles::velocity;
        angle.trig_top(0x252af5u, true);
        angle.store_extended(0x252af7u, gp[ebp]);
        fp.pop();
        fp.push(angle.load(0x252afau, gp[esp] + 0xcu));
        angle.trig_top(0x252afeu, false);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252b00u, gp[esp] + 0x18u));
        angle.store_extended(0x252b04u, gp[edi] + angles::velocity + 4u);
        fp.pop();
        fp.push(angle.load(0x252b07u, gp[esp] + 0xcu));
        angle.trig_top(0x252b0bu, true);
        fp.st(0) = fp_math::mul(fp.st(0), angle.load(0x252b0du, gp[esp] + 0x18u));
        angle.store_extended(0x252b11u, gp[edi] + angles::velocity + 8u);
        fp.pop();
        port.publish(seventh_exit);
    }
};

template<bool Seventh, class State, class Ref, class Direct, class Dispatch, class Trig, class Spill>
bool bridge(State &s, Ref reference, Direct direct, Dispatch dispatch, Trig trig, Spill spill) {
    world::Publication publication;
    if (!properties::admit(s, Seventh ? seventh_sites : sixth_sites, publication)) return false;
    auto &counter = Seventh ? summary().seventh : summary().sixth;
    world::execute(s, publication, counter, reference, [&](auto &port) {
        properties::Properties<State, Direct> frame{port, {port, direct}};
        angles::Angles<State, Direct, Trig, Spill> angle{frame, trig, spill};
        cone::Cone<State, Direct, Dispatch, Trig, Spill> cone{frame, angle, dispatch};
        Bounds<State, Direct, Dispatch, Trig, Spill> bounds{cone};
        if constexpr (Seventh) bounds.mode_seven();
        else bounds.mode_six();
    });
    return true;
}
} // namespace halo2::native_particle_velocity_bounds_detail
