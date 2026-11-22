#include "halo2/native_campaign_ai_behavior_score_17_tu15.hpp"
#include "native_campaign_ai_behavior_score_frame_tu15.hpp"

namespace halo2 {
namespace {
using namespace native_campaign_ai_behavior_score_detail;
constexpr Word entry = 0x1b3600u, descriptor = 0x48a310u, none = 0xffffffffu;

void store_probability(Frame &s) {
    constexpr Word pc = 0x1b3732u;
    s.at(pc);
    s.require_x87();
    const auto address = s.gp[4] + 0x10u;
    const auto &extended = s.fp.st(0);
    const auto before_exceptions = physics_contact_runtime_detail::source_x87_exception_flags();
    Word stored{};
    std::uint16_t status{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(stored), "=m"(status) : "m"(extended) : "memory", "st");
#else
    (void)extended;
    throw std::runtime_error("Original behavior score x87 single spill unavailable");
#endif
    bool committed = false;
    const auto commit = [&] {
        s.fp.pop();
        s.fp.status = (s.fp.status & ~0x200u) | (status & 0x200u);
        committed = true;
    };
    try {
        if (s.observe) {
            const SourcePhysicsContactRecordObserver completed = [&](const auto &store) {
                commit();
                s.observe(store);
            };
            w(s.memory, completed, pc, address, 4, stored);
        } else {
            w(s.memory, s.observe, pc, address, 4, stored);
            commit();
        }
    } catch (...) {
        if (!committed) physics_contact_runtime_detail::source_load_x87_exception_flags(before_exceptions);
        throw;
    }
}

void roll_probability(Frame &s) {
    auto &ecx = s.gp[1];
    auto &esp = s.gp[4];
    ecx = s.read(0x1b3729u, s.gp[5] + 0x24u);
    s.push_word(0x1b372cu, ecx);
    s.call(0x1b372du, 0x1d2190u, 0x1b3732u, 4);
    store_probability(s);
    ecx = s.read(0x1b3736u, 0x4f3cd8u);
    s.call(0x1b373cu, 0x25980u, 0x1b3741u);
    const auto word = s.read(0x1b3741u, esp + 0x10u);
    s.require_x87();
    long double operand{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %1\n\tfstpt %0" : "=m"(operand) : "m"(word) : "memory", "st");
#else
    (void)word;
    throw std::runtime_error("Original behavior score x87 single load unavailable");
#endif
    s.fp.push(operand);
    s.at(0x1b3745u);
    s.compare_and_pop(s.fp.st(1));
    s.fp.status &= ~0x200u;
    s.at(0x1b3747u);
    s.fp.pop();
    s.at(0x1b3749u);
    if (!s.flags.cf && !s.flags.zf) s.store(0x1b374bu, esp + 0x18u, 3);
}

bool score_four(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.at(0x1b3653u); s.flags.logic(ebp, 4);
    s.at(0x1b3655u);
    if (s.flags.zf) return false;
    s.flags.minus(s.read(0x1b365bu, esi + 0x338u), none, 4);
    s.at(0x1b3662u);
    if (s.flags.zf) return false;
    s.xmm[0] = s.scalar(0x1b3668u, ebp + 0x24u);
    const auto minimum = s.scalar(0x1b366du, 0x46a0e8u);
    s.flags.scalar_compare(s.xmm[0], minimum, false);
    s.at(0x1b3674u);
    if (s.flags.cf) return false;
    s.set(0, s.read(0x1b367au, esi + 0x229u, 1), 1);
    s.at(0x1b3680u); s.flags.logic(eax, 1);
    s.at(0x1b3682u);
    if (!s.flags.zf) return false;
    eax = s.read(0x1b3688u, esi + 0x1f8u);
    s.at(0x1b368eu); s.flags.minus(eax, none, 4);
    s.at(0x1b3691u);
    if (!s.flags.zf) {
        ecx = s.read(0x1b3693u, 0x51df54u);
        edx = s.read(0x1b3699u, ecx + 8u);
        s.at(0x1b369cu); edx = s.flags.minus(edx, eax, 4);
        s.at(0x1b369eu); s.xmm[0] = _mm_cvtsi32_ss(s.xmm[0], std::bit_cast<std::int32_t>(edx));
        const auto seconds_per_tick = s.scalar(0x1b36a2u, ecx + 4u);
        s.xmm[0] = _mm_mul_ss(s.xmm[0], seconds_per_tick);
        const auto cooldown = s.scalar(0x1b36a7u, ebp + 0x34u);
        s.flags.scalar_compare(s.xmm[0], cooldown, false);
        s.at(0x1b36abu);
        if (s.flags.cf || s.flags.zf) return false;
    }

    eax = s.read(0x1b36b1u, esi + 0x18u);
    s.push_word(0x1b36b4u, eax);
    s.call(0x1b36b5u, 0x113190u, 0x1b36bau, 4);
    s.at(0x1b36bau); s.flags.logic(eax, 1);
    s.at(0x1b36bcu);
    if (!s.flags.zf) return false;
    esi = s.read(0x1b36c2u, esi + 0x338u);
    eax = s.read(0x1b36c8u, 0x50eef0u);
    edx = s.read(0x1b36cdu, eax + 0x44u);
    s.at(0x1b36d0u); esi = s.flags.logic(esi & 0xffffu, 4);
    s.at(0x1b36d6u); esi = s.flags.product(esi, 0x3cu);
    s.at(0x1b36d9u); esi = s.flags.plus(esi, edx, 4);
    s.at(0x1b36dbu); ecx = esi;
    s.call(0x1b36ddu, 0x2673a0u, 0x1b36e2u);
    s.flags.minus(s.read(0x1b36e2u, esi + 0x27u, 1), 1, 1);
    s.at(0x1b36e6u);
    if (s.flags.sf != s.flags.of) return false;
    s.set(0, s.read(0x1b36e8u, eax + 0x64u, 1), 1);
    s.at(0x1b36ebu); s.flags.logic(eax, 1);
    s.at(0x1b36edu);
    if (s.flags.zf) {
        s.xmm[0] = s.scalar(0x1b36efu, esi + 0x28u);
        const auto minimum_range = s.scalar(0x1b36f4u, ebp + 0x1cu);
        s.flags.scalar_compare(s.xmm[0], minimum_range, false);
        s.at(0x1b36f8u);
        if (s.flags.cf || s.flags.zf) return false;
    }
    s.xmm[0] = s.scalar(0x1b36fau, ebp + 0x20u);
    const auto range = s.scalar(0x1b36ffu, esi + 0x28u);
    s.flags.scalar_compare(s.xmm[0], range, false);
    s.at(0x1b3703u);
    if (s.flags.cf || s.flags.zf) return false;
    s.at(0x1b3705u); s.flags.logic(eax, 1);
    s.at(0x1b3707u);
    if (!s.flags.zf) {
        s.at(0x1b3709u); eax = ebx;
        s.call(0x1b370bu, 0x1ea2e0u, 0x1b3710u);
        s.at(0x1b3710u); s.flags.logic(eax, 1);
        s.at(0x1b3712u);
        if (s.flags.zf) return true;
    }
    roll_probability(s);
    return false;
}

void run_score(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.push_word(entry, ecx);
    eax = s.read(0x1b3601u, 0x5020c8u);
    edx = s.read(0x1b3606u, eax + 0x44u);
    s.push_word(0x1b3609u, ebx);
    ebx = s.read(0x1b360au, esp + 0xcu);
    s.push_word(0x1b360eu, ebp);
    s.push_word(0x1b360fu, esi);
    s.at(0x1b3610u); esi = ebx;
    s.at(0x1b3612u); esi = s.flags.logic(esi & 0xffffu, 4);
    s.at(0x1b3618u); esi = s.flags.product(esi, 0x888u);
    eax = s.read(0x1b361eu, esi + edx + 0x18u);
    s.at(0x1b3622u); esi = s.flags.plus(esi, edx, 4);
    edx = s.read(0x1b3624u, 0x4ecbd0u);
    s.at(0x1b362au); eax = s.flags.logic(eax & 0xffffu, 4);
    s.at(0x1b362fu); ecx = eax * 3u;
    eax = s.read(0x1b3632u, edx + 0x44u);
    s.push_word(0x1b3635u, edi);
    edi = s.read(0x1b3636u, eax + ecx * 4u + 8u);
    s.call(0x1b363au, 0x1ed240u, 0x1b363fu);
    s.at(0x1b363fu); ebp = eax;
    s.flags.logic(s.read(0x1b3641u, edi + 0x19u, 1) & 1u, 1);
    // This is the caller's consumed actor argument slot, not a private result.
    s.store(0x1b3645u, esp + 0x18u, 0);
    s.at(0x1b364du);
    if (s.flags.zf && score_four(s)) {
        edi = s.pop_word(0x1b3714u);
        esi = s.pop_word(0x1b3715u);
        s.store(0x1b3716u, esp + 0x10u, 4);
        s.set(0, s.read(0x1b371eu, esp + 0x10u, 2), 2);
        ebp = s.pop_word(0x1b3723u);
        ebx = s.pop_word(0x1b3724u);
        ecx = s.pop_word(0x1b3725u);
        s.finish(0x1b3726u, 0x1b0197u, 4);
        return;
    }
    s.set(0, s.read(0x1b3753u, esp + 0x18u, 2), 2);
    edi = s.pop_word(0x1b3758u);
    esi = s.pop_word(0x1b3759u);
    ebp = s.pop_word(0x1b375au);
    ebx = s.pop_word(0x1b375bu);
    ecx = s.pop_word(0x1b375cu);
    s.finish(0x1b375du, 0x1b0197u, 4);
}
}

bool native_campaign_ai_behavior_score_17_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call, SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint controls, const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0195u ||
        call.target != entry || call.tail || abi.gp[0] != entry || abi.gp[7] != descriptor)
        return false;

    FpScope scope(abi.floating_point.value_or(controls));
    Frame state(memory, observe, services, image);
    state.load(abi);
    try {
        run_score(state);
    } catch (...) {
        abi = state.save();
        throw;
    }
    abi = state.save();
    return true;
}
}
