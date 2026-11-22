#include "halo2/native_campaign_ai_vehicle_movement_tu15.hpp"
#include "campaign/ai/native_campaign_ai_goal_port_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
namespace {
using goal_tu15::Port;
using goal_tu15::Simd;
using goal_tu15::u32;
using physics_contact_runtime_detail::Flags;

bool turn_above_threshold(NativeTu15PhysicsFrameFp &fp,
                          const std::array<std::byte, 8> &threshold, u32 turn) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    unsigned char above;
    asm volatile("fldl %1\n\tflds %2\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
        : "=q"(above) : "m"(threshold), "m"(turn) : "cc", "memory", "st", "st(1)");
    return above != 0;
}

u32 penalize_turn(NativeTu15PhysicsFrameFp &fp, u32 angle, u32 angle_weight,
                  u32 one, u32 score) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    asm volatile("flds %1\n\tfabs\n\tfmuls %2\n\tfadds %3\n\tfdivrs %4\n\tfstps %0"
        : "=m"(result) : "m"(angle), "m"(angle_weight), "m"(one), "m"(score) : "memory", "st");
    return result;
}

void compare_scalar(NativeTu15PhysicsFrameFp &fp, Flags &flags, u32 left, u32 right) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto as_scalar = [](u32 word) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word)));
    };
    flags.scalar_compare(as_scalar(left), as_scalar(right), false);
}
}
#endif

std::uint32_t native_campaign_ai_vehicle_movement_score_tu15(
    SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if (!abi.floating_point || abi.x87_depth > 6 || abi.active_mmx)
        throw std::invalid_argument("Native 2A2940 requires retained FP capacity");
    const auto control = *abi.floating_point;
    NativeTu15PhysicsFrameFp fp{control.x87_control_word, abi.x87_status, control.mxcsr};
    goal_tu15::require_masked(fp, "Native vehicle movement score");
    const Port port{memory, observe};
    const Simd simd(fp);
    Flags flags;
    flags.load(abi.eflags);
    const auto publish = [&] {
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};
        abi.x87_status = fp.status;
        abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits();
    };
    auto &eax = abi.gp[0], &ecx = abi.gp[1], &edx = abi.gp[2], &ebx = abi.gp[3];
    auto &sp = abi.gp[4], &esi = abi.gp[6], &edi = abi.gp[7];
    auto &x0 = abi.xmm[0], &x1 = abi.xmm[1], &x2 = abi.xmm[2], &x3 = abi.xmm[3];
    const auto push = [&](u32 site, u32 value) {
        const auto next = sp - 4u;
        if (observe) {
            physics_step_detail::w(memory, [&](const auto &store) {
                sp = next;
                publish();
                observe(store);
            }, site, next, 4, value);
        } else port.w32(site, next, value);
        sp = next;
    };
    const auto flag_byte = [&] {
        const auto value = port.r8(ecx + 1u);
        eax = (eax & 0xffffff00u) | value;
        flags.logic(value, 1);
        return value;
    };

    try {
        push(0x2a2940u, ecx);
        edx = port.r32(0x4ecbd0u);
        edx = port.r32(edx + 0x44u);
        x2 = {};
        eax = flags.logic(eax & 0xffffu, 4);
        compare_scalar(fp, flags, x2[0], port.r32(ecx + 4u));
        eax *= 3u;
        eax = port.r32(edx + eax * 4u + 8u);
        push(0x2a295du, ebx);
        const auto reverse = !flags.cf && !flags.zf;
        if (!reverse) flags.logic(0, 1);
        ebx = (ebx & 0xffffff00u) | u32(reverse);
        const auto threshold = port.r64(0x46a0f0u);
        const auto turn = port.r32(sp + 0x14u);
        const auto larger_turn = turn_above_threshold(fp, threshold, turn);
        if (!larger_turn) flags.logic(0, 1);
        edx = (edx & 0xffffff00u) | u32(larger_turn);
        const bool is_vehicle = flags.minus(port.r8(eax + 0xaau), 1u, 1) == 0;
        x0 = {port.r32(0x46a110u)};
        port.w32(0x2a298bu, sp + 4u, x0[0]);
        if (is_vehicle) {
            eax = port.r32(eax);
            eax = flags.logic(eax & 0xffffu, 4);
            push(0x2a299au, edi);
            edi = port.r32(0x4f0414u);
            eax = flags.shift(eax, 4, 4);
            eax = port.r32(eax + edi + 8u);
            x1 = {port.r32(eax + 0x1f4u)};
            compare_scalar(fp, flags, x2[0], x1[0]);
            edi = port.r32(sp);
            sp += 4u;
        }
        if (!is_vehicle || !flags.cf) x1 = {port.r32(0x46a108u)};
        flags.logic(ebx & 0xffu, 1);
        x2 = {port.r32(0x46a0d0u)};
        if (reverse || flag_byte()) {
            flags.logic(edx & 0xffu, 1);
            if (larger_turn) {
                x3 = {port.r32(sp + 0x14u)};
                x3[0] = simd.div(x3[0], x1[0]);
                x3[0] = simd.mul(x3[0], port.r32(0x46a0ccu));
                x3[0] = simd.add(x3[0], x2[0]);
                x0[0] = simd.div(x0[0], x3[0]);
                port.w32(0x2a29efu, sp + 4u, x0[0]);
            }
            flags.logic(ebx & 0xffu, 1);
            if (reverse || flag_byte()) {
                edx = port.r32(sp + 0xcu);
                x1 = {port.r32(edx + 0x20u)};
                x1[0] = simd.mul(x1[0], port.r32(sp + 0x10u));
                x1[0] = simd.mul(x1[0], port.r32(0x46a17cu));
                x1[0] = simd.add(x1[0], x2[0]);
                x0[0] = simd.div(x0[0], x1[0]);
                port.w32(0x2a2a1fu, sp + 4u, x0[0]);
            }
        }
        const auto weighted = flag_byte() != 0;
        ebx = port.r32(sp);
        sp += 4u;
        if (weighted) {
            x0[0] = simd.mul(x0[0], port.r32(0x456910u));
            port.w32(0x2a2a35u, sp, x0[0]);
        }
        flags.logic(esi, 4);
        const auto angle = port.r32(ecx + 4u);
        const auto angle_weight = port.r32(0x46a16cu);
        const auto one = port.r32(0x46a0d0u);
        const auto score = penalize_turn(fp, angle, angle_weight, one, port.r32(sp));
        port.w32(0x2a2a50u, sp, score);
        if (esi) {
            x0 = {port.r32(ecx + 0x10u)};
            x0[0] = simd.mul(x0[0], port.r32(esi + 4u));
            x1 = {port.r32(ecx + 0xcu)};
            x1[0] = simd.mul(x1[0], port.r32(esi));
            x0[0] = simd.add(x0[0], x1[0]);
            x0[0] = simd.add(x0[0], port.r32(0x46a0e4u));
            x0[0] = simd.mul(x0[0], port.r32(sp));
            x0[0] = simd.mul(x0[0], port.r32(0x46a130u));
            port.w32(0x2a2a81u, ecx + 8u, x0[0]);
        } else {
            x0 = {port.r32(sp)};
            port.w32(0x2a2a8fu, ecx + 8u, x0[0]);
        }
        ecx = port.r32(sp);
        sp += 4u;
        const auto continuation = port.r32(sp);
        sp += 0x10u;
        publish();
        return continuation;
    } catch (...) {
        publish();
        throw;
    }
#else
    (void)memory;
    (void)abi;
    (void)observe;
    throw std::runtime_error("Exact native vehicle movement score backend unavailable");
#endif
}

} // namespace halo2
