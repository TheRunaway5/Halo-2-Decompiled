#include "native_campaign_ai_vehicle_candidate_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_vehicle_movement_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cstring>

namespace halo2::vehicle_candidates_tu15 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
namespace {
using goal_tu15::Port;
using goal_tu15::Simd;
using physics_contact_runtime_detail::Flags;

class CandidateProducer {
    SourcePhysicsWorldRuntimeAbi &abi;
    const SourcePhysicsWorldRuntimeServices::RequiredOwner &acos_owner;
    Port port;
    NativeTu15PhysicsFrameFp fp;
    Simd simd;
    Flags flags;
    const u32 source_sp;
    const u32 frame;
    const u32 origin_xy;
    const u32 travel_xy;
    const SourcePhysicsContactRecordObserver &observe;

    void state() {
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};
        abi.x87_status = fp.status;
        abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits();
    }

    void restore() {
        const auto control = abi.floating_point.value();
        fp = {control.x87_control_word, abi.x87_status, control.mxcsr};
        flags.load(abi.eflags);
    }

    void store(u32 site, u32 offset, u32 value) {
        port.w32(site, frame + offset, value);
    }

    u32 word(u32 offset) const { return port.r32(frame + offset); }

    void push(u32 site, u32 value) {
        const auto address = abi.gp[4] - 4u;
        port.w32(site, address, value);
        abi.gp[4] = address;
    }

    u32 pop() {
        const auto value = port.r32(abi.gp[4]);
        abi.gp[4] += 4u;
        return value;
    }

    void set_xmm(unsigned index, u32 value) { abi.xmm[index] = {value}; }

    void load_x87(u32 value) {
        // An already selected finite Float32 operand widens exactly.
        if ((value & 0x7f800000u) == 0x7f800000u)
            throw std::runtime_error("2A2AA0 acos requires its original exceptional input owner");
        {
            detail::NativeTu15ArithmeticEnvironment env(fp);
            asm volatile("flds %1\n\tfstpt %0"
                : "=m"(abi.x87[0]) : "m"(value) : "memory", "st");
        }
        fp.status = (fp.status & ~0x3a00u) | ((fp.status - 0x800u) & 0x3800u);
        abi.x87_depth = 1;
    }

    u32 store_pop_x87(u32 site, u32 offset) {
        if (abi.x87_depth != 1)
            throw std::runtime_error("2A2AA0 acos lost its physical x87 result");
        u32 result;
        {
            detail::NativeTu15ArithmeticEnvironment env(fp);
            asm volatile("fldt %1\n\tfstps %0" : "=m"(result) : "m"(abi.x87[0]) : "memory", "st");
        }
        fp.status = (fp.status & ~0x3800u) | ((fp.status + 0x800u) & 0x3800u);
        abi.x87_depth = 0;
        store(site, offset, result);
        return result;
    }

    u32 acos_from_frame(u32 input, u32 call_site, u32 spill_site, u32 output) {
        load_x87(word(input));
        push(call_site, call_site + 5u);
        state();
        if (!acos_owner)
            throw std::runtime_error("2A2AA0 lacks its retained 32CCD8 owner");
        acos_owner(port.memory, {call_site, 0x32ccd8u, false}, abi, observe);
        restore();
        if (abi.gp[4] != frame)
            throw std::runtime_error("2A2AA0 acos lost its actual return frame");
        return store_pop_x87(spill_site, output);
    }

    u32 clamp(u32 value, u32 lo, u32 hi) {
        if (simd.above(lo, value)) return lo;
        if (simd.above(value, hi)) return hi;
        return value;
    }

    void setup_frame() {
        abi.gp[4] = flags.minus(source_sp, 0x64u, 4);
        const auto vehicle = port.r32(source_sp + 8u);
        const auto x = simd.add(port.r32(origin_xy), port.r32(travel_xy));
        push(0x2a2aafu, abi.gp[3]);
        push(0x2a2ab0u, abi.gp[5]);
        push(0x2a2ab1u, abi.gp[6]);
        const auto homes = port.r32(port.r32(0x4ecbd0u) + 0x44u);
        const auto vehicle_row = port.r32(homes + (vehicle & 0xffffu) * 12u + 8u);
        store(0x2a2ac9u, 0x54u, x);
        const auto y = port.r32(travel_xy + 4u);
        store(0x2a2ad7u, 0x38u, y);
        const auto dx = port.r32(travel_xy);
        store(0x2a2ae3u, 0x34u, dx);
        const auto shifted_y = simd.add(y, port.r32(origin_xy + 4u));
        const auto one = port.r32(0x46a0d0u);
        set_xmm(2, one);
        push(0x2a2afcu, abi.gp[7]);
        store(0x2a2afdu, 0x58u, shifted_y);
        const auto magnitude = compare_length(fp, {dx, y}, port.r32(0x46a0ecu));
        store(0x2a2b07u, 0x28u, magnitude.rounded);
        if (!magnitude.comparison) {
            const auto reciprocal = simd.div(one, magnitude.rounded);
            store(0x2a2b31u, 0x34u, simd.mul(reciprocal, word(0x34u)));
            store(0x2a2b37u, 0x38u, simd.mul(reciprocal, word(0x38u)));
        }

        const auto reference = word(0x88u), orientation = word(0x84u);
        const auto distance = word(0x90u);
        const auto opposite = simd.sub(0, distance);
        // The retained stores interleave the ordered endpoints.
        const auto minus_x = simd.add(simd.mul(port.r32(reference), opposite), port.r32(origin_xy));
        store(0x2a2b74u, 0x64u, minus_x);
        const auto minus_y = simd.add(simd.mul(port.r32(reference + 4u), opposite), port.r32(origin_xy + 4u));
        const auto plus_x = simd.add(simd.mul(port.r32(reference), distance), port.r32(origin_xy));
        store(0x2a2b9eu, 0x6cu, plus_x);
        const auto plus_y = simd.add(simd.mul(port.r32(reference + 4u), distance), port.r32(origin_xy + 4u));
        store(0x2a2bb6u, 0x68u, minus_y);
        store(0x2a2bbcu, 0x70u, plus_y);
        const auto forward = nonnegative_projection(fp, {word(0x34u), word(0x38u)},
            {port.r32(orientation), port.r32(orientation + 4u)}, port.r64(0x46a0f0u));
        port.w8(0x2a2bc2u, frame + 0x12u, 1u);
        if (!forward)
            port.w8(0x2a2bc9u, frame + 0x12u, 0u);
        const auto vx = simd.mul(port.r32(orientation), port.r32(vehicle_row + 0x88u));
        const auto vy = simd.mul(port.r32(vehicle_row + 0x8cu), port.r32(orientation + 4u));
        set_xmm(0, simd.add(vy, vx));
        store(0x2a2bf0u, 0x2cu, abi.xmm[0][0]);
        abi.gp[5] &= 0xffff0000u;
        abi.gp[3] = 0;
        store(0x2a2bf6u, 0x28u, 0);
    }

    u32 score_row(u32 row, u32 row_index, u32 call_site, u32 x_site, u32 y_site,
                  u32 score_push, u32 gap_push, u32 property_push, const Planar &direction) {
        push(score_push, word(0x2cu));
        const auto property = word(0x80u);
        port.w32(x_site, row + 0x0cu, direction.x);
        port.w32(y_site, row + 0x10u, direction.y);
        const auto gap = word(0x1cu);
        push(gap_push, gap);
        push(property_push, property);
        abi.gp[0] = word(0x7cu);
        abi.gp[1] = row;
        abi.gp[2] = gap;
        abi.gp[3] = direction.x;
        abi.gp[5] = (abi.gp[5] & 0xffff0000u) | row_index;
        abi.gp[6] = word(0x8cu);
        abi.gp[7] = row_index * stride;
        push(call_site, call_site + 5u);
        state();
        const auto returned = native_campaign_ai_vehicle_movement_score_tu15(port.memory, abi, observe);
        restore();
        if (returned != call_site + 5u || abi.gp[4] != frame)
            throw std::runtime_error("Native candidate score lost its original retained return");
        return row;
    }

    Planar at(u32 xy) const { return {port.r32(xy), port.r32(xy + 4u)}; }

    void heading_angles(u32 pass, u32 endpoint, u32 gap) {
        const auto ratio = compare_ratio(fp, word(0x90u), gap, port.r64(0x46a1d0u));
        store(0x2a2d7du, 0x18u, ratio.rounded);
        const auto one = abi.xmm[2][0], minus_one = port.r32(0x46a0dcu);
        if (ratio.comparison) store(0x2a2dbau, 0x20u, one);
        else if (simd.above(minus_one, ratio.rounded)) store(0x2a2da7u, 0x20u, minus_one);
        else {
            store(0x2a2db2u, 0x20u, one);
            if (!simd.above(ratio.rounded, one)) store(0x2a2dbau, 0x20u, ratio.rounded);
        }
        const auto half_width = acos_from_frame(0x20u, 0x2a2dc4u, 0x2a2dc9u, 0x30u);

        const auto start = frame + endpoint;
        set_xmm(1, port.r32(0x46a0d0u));
        store(0x2a2ddbu, 0x3cu, subtract_extended(fp, word(0x54u), port.r32(start)));
        store(0x2a2de6u, 0x40u, subtract_extended(fp, word(0x58u), port.r32(start + 4u)));
        const auto magnitude = compare_length(fp, {word(0x3cu), word(0x40u)}, port.r32(0x46a0ecu));
        store(0x2a2dfau, 0x18u, magnitude.rounded);
        if (!magnitude.comparison) {
            const auto reciprocal = simd.div(abi.xmm[1][0], magnitude.rounded);
            store(0x2a2e1eu, 0x3cu, simd.mul(reciprocal, word(0x3cu)));
            store(0x2a2e2eu, 0x40u, simd.mul(word(0x40u), reciprocal));
        }
        const auto reference = word(0x88u);
        const auto projection = compare_projection(fp, {word(0x3cu), word(0x40u)},
                                                    at(reference), port.r64(0x46a1d0u));
        store(0x2a2e4au, 0x18u, projection.rounded);
        if (projection.comparison) store(0x2a2ea1u, 0x24u, one);
        else if (bound_exceeds_word(fp, port.r64(0x46a4e0u), projection.rounded))
            store(0x2a2ea1u, 0x24u, minus_one);
        else if (simd.above(minus_one, projection.rounded))
            store(0x2a2e8eu, 0x24u, minus_one);
        else {
            store(0x2a2e99u, 0x24u, one);
            if (!simd.above(projection.rounded, one)) store(0x2a2ea1u, 0x24u, projection.rounded);
        }
        const auto arc = acos_from_frame(0x24u, 0x2a2eabu, 0x2a2ebfu, 0x18u);
        const auto period = port.r32(0x44923cu);
        const auto forward = port.r8(frame + 0x12u);
        u32 center;
        if (pass) {
            const auto pi = port.r32(0x46a2ecu);
            center = forward ? simd.sub(pi, arc) : simd.add(arc, pi);
        } else center = forward ? arc : simd.sub(period, arc);
        auto upper = simd.add(center, half_width);
        const bool wrap = simd.above(upper, period);
        store(0x2a2f0eu, 0x5cu, upper);
        if (wrap) store(0x2a2f1au, 0x5cu, simd.sub(upper, period));
        store(0x2a2f28u, 0x60u, simd.sub(center, half_width));
        store(0x2a2f2eu, 0x18u, frame + 0x5cu);
        store(0x2a2f32u, 0x30u, 2);
    }

    bool reverse_bias(bool alternate, u32 angle, Planar direction) {
        if (simd.above(0, angle)) {
            const auto negate = abi.xmm[4][0];
            direction.x = simd.mul(direction.x, negate);
            direction.y = simd.mul(direction.y, negate);
            if (alternate) {
                set_xmm(6, direction.x);
                set_xmm(5, direction.y);
            }
        } else {
            store(alternate ? 0x2a30a1u : 0x2a2ff2u, 0x4cu, direction.x);
            store(alternate ? 0x2a30abu : 0x2a2ffcu, 0x50u, direction.y);
        }
        const auto y = simd.mul(direction.y, word(0x38u));
        const auto x = simd.mul(direction.x, word(0x34u));
        set_xmm(0, simd.add(y, x));
        set_xmm(1, x);
        return simd.above(0, abi.xmm[0][0]);
    }

    void steer_pair(u32 pass, u32 phase, u32 &row_index) {
        const auto angle_pointer = word(0x18u), angle = port.r32(angle_pointer);
        const auto period = port.r32(0x44923cu);
        const auto wrapped = simd.above_equal(angle, 0) ? simd.sub(angle, period) : simd.add(angle, period);
        set_xmm(7, wrapped);
        set_xmm(3, 0);
        store(0x2a2f6cu, 0x14u, wrapped);
        const auto direction = rotate_candidates(fp, at(word(0x84u)), wrapped, pass == 0);
        store(0x2a2fa1u, 0x44u, direction.x);
        set_xmm(6, direction.x);
        set_xmm(4, port.r32(0x46a0dcu));
        auto row = rows + row_index * stride;
        port.w32(0x2a2fc4u, row + 4u, angle);
        store(0x2a2fd0u, 0x48u, direction.y);
        set_xmm(5, direction.y);
        port.w8(0x2a2fdau, row, pass);
        port.w8(0x2a302fu, row + 1u, u32(reverse_bias(false, angle, direction)));
        score_row(row, row_index, 0x2a305au, 0x2a3041u, 0x2a3047u,
                  0x2a3039u, 0x2a3051u, 0x2a3052u, direction);
        port.w16(0x2a3066u, row + 0x18u, row_index++);
        // The second row uses the same pair of coordinates, with the
        // alternate winding and its own reverse-score predicate.
        set_xmm(2, 0);
        row = rows + row_index * stride;
        port.w32(0x2a3081u, row + 4u, wrapped);
        port.w8(0x2a3089u, row, pass);
        port.w8(0x2a30deu, row + 1u, u32(reverse_bias(true, wrapped, direction)));
        score_row(row, row_index, 0x2a3109u, 0x2a30f0u, 0x2a30f6u,
                  0x2a30e8u, 0x2a3100u, 0x2a3101u, direction);
        port.w16(0x2a311au, row + 0x18u, row_index++);
        abi.gp[1] = angle_pointer + 4u;
        abi.gp[0] = 1u - phase;
        store(0x2a312eu, 0x18u, abi.gp[1]);
        store(0x2a3132u, 0x30u, abi.gp[0]);
    }

    void geometric_pass(u32 pass, u32 &row_index) {
        const auto endpoint = 0x64u + pass * 8u;
        port.w8(0x2a2c24u, frame + 0x13u, pass);
        const auto gap = compare_displaced_length(fp, {word(0x54u), word(0x58u)},
                                                  at(frame + endpoint), port.r32(word(0x80u) + 0x24u));
        store(0x2a2c38u, 0x1cu, gap.rounded);
        if (gap.comparison)
            throw std::runtime_error("2A2AA0 requires the original 2A2C49 fixed-angle branch");
        heading_angles(pass, endpoint, gap.rounded);
        steer_pair(pass, 0, row_index);
        steer_pair(pass, 1, row_index);
        set_xmm(2, port.r32(0x46a0d0u));
        store(0x2a3149u, 0x28u, pass + 1u);
    }

public:
    CandidateProducer(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &current,
                      const SourcePhysicsWorldRuntimeServices::RequiredOwner &acos,
                      const SourcePhysicsContactRecordObserver &observer):
        abi(current), acos_owner(acos), port{memory, observer},
        fp{current.floating_point->x87_control_word, current.x87_status, current.floating_point->mxcsr}, simd(fp),
        source_sp(current.gp[4]), frame(source_sp - 0x74u), origin_xy(current.gp[1]), travel_xy(current.gp[2]),
        observe(observer) {
        flags.load(abi.eflags);
    }

    u32 run() {
        try {
            setup_frame();
            u32 rows = 0;
            for (u32 pass = 0; pass < 2; ++pass) geometric_pass(pass, rows);
            abi.gp[7] = pop();
            abi.gp[6] = pop();
            port.w16(0x2a3155u, count, rows);
            abi.gp[5] = pop();
            abi.gp[3] = pop();
            abi.gp[4] = flags.plus(abi.gp[4], 0x64u, 4);
            const auto returned = port.r32(abi.gp[4]);
            abi.gp[4] += 0x20u;
            state();
            return returned;
        } catch (...) {
            state();
            throw;
        }
    }
};
}

u32 native_campaign_ai_vehicle_movement_candidates_tu15(
    SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices::RequiredOwner &acos, const SourcePhysicsContactRecordObserver &observer) {
    if (!abi.floating_point || abi.x87_depth || abi.active_mmx)
        throw std::invalid_argument("Native 2A2AA0 requires the retained empty FP stack");
    return CandidateProducer(memory, abi, acos, observer).run();
}
#else
std::uint32_t native_campaign_ai_vehicle_movement_candidates_tu15(
    SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices::RequiredOwner &, const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native vehicle candidate backend unavailable");
}
#endif

bool admits(const SourcePhysicsContactMemory &memory, const SourcePhysicsContactRuntimeCall &call,
            const SourcePhysicsWorldRuntimeAbi &abi) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if (call.tail || call.instruction != 0x2a4327u || call.target != 0x2a2aa0u ||
        !abi.floating_point || abi.floating_point->x87_control_word != 0x027fu ||
        (abi.floating_point->mxcsr & 0xffff1f80u) != 0x1f80u ||
        abi.x87_depth || abi.active_mmx || (abi.x87_status & 0x8080u))
        return false;

    const auto &gp = abi.gp;
    const auto sp = gp[4];
    if (sp < 0x60000140u || sp > 0x600fff80u || gp[1] != sp + 0x5cu || gp[1] != gp[6] ||
        gp[2] != gp[5] + 0x5ecu || gp[7] != gp[3])
        return false;

    const auto frame = memory.try_read_view(sp, 0x20);
    if (!frame || !memory.try_write_extent(sp - 0x140u, 0x140u) ||
        !memory.try_write_extent(rows, bytes))
        return false;

    const auto word = [](std::span<const std::byte> data, std::size_t offset = 0) {
        std::uint32_t value;
        std::memcpy(&value, data.data() + offset, sizeof(value));
        return value;
    };
    if (word(*frame) != 0x2a432cu || word(*frame, 4) != gp[3] ||
        word(*frame, 8) != gp[0] || word(*frame, 0x10u) != sp + 0x3cu ||
        word(*frame, 0x14u) != sp + 0x78u || word(*frame, 0x18u) != 0)
        return false;

    const auto property = word(*frame, 0x0cu);
    if (property > 0xffffffdbu) return false;
    const auto origin = memory.try_read_view(gp[1], 8u);
    const auto travel = memory.try_read_view(gp[2], 8u);
    const auto reference = memory.try_read_view(word(*frame, 0x14u), 8u);
    const auto bound = memory.try_read_view(property + 0x24u, 4u);
    if (!origin || !travel || !reference || !bound) return false;

    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,
                                abi.x87_status, abi.floating_point->mxcsr};
    const Simd simd(fp);
    const Planar base{word(*origin), word(*origin, 4)};
    const Planar end{simd.add(base.x, word(*travel)), simd.add(word(*travel, 4), base.y)};
    const auto distance = word(*frame, 0x1cu);
    for (const auto multiplier : {simd.sub(0, distance), distance}) {
        const Planar corner{simd.add(simd.mul(word(*reference), multiplier), base.x),
                            simd.add(simd.mul(word(*reference, 4), multiplier), base.y)};
        if (compare_displaced_length(fp, end, corner, word(*bound)).comparison) return false;
    }
    return true;
#else
    (void)memory;
    (void)call;
    (void)abi;
    return false;
#endif
}

} // namespace halo2::vehicle_candidates_tu15
