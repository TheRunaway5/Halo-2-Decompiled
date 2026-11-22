#include "halo2/native_campaign_ai_behavior_score_50_tu15.hpp"
#include "native_campaign_ai_behavior_score_frame_tu15.hpp"

namespace halo2 {
namespace {
using namespace native_campaign_ai_behavior_score_detail;
constexpr Word entry = 0x1bfbf0u, descriptor = 0x48af44u;
constexpr Word actor_pool = 0x5020c8u, initial_behavior = 0x47c1a4u, none = 0xffffffffu;

bool select_behavior(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.call(0x1bfc26u, 0x1ecfc0u, 0x1bfc2bu);
    s.at(0x1bfc2bu); ebp = eax;
    s.call(0x1bfc2du, 0x1ed240u, 0x1bfc32u);
    s.at(0x1bfc32u); s.xmm[0] = _mm_setzero_ps();
    s.at(0x1bfc35u); edi = eax;
    s.at(0x1bfc37u); s.flags.logic(edi, 4);
    s.at(0x1bfc39u);
    if (!s.flags.zf) {
        eax = s.read(0x1bfc3bu, 0x4f3218u);
        edx = s.read(0x1bfc40u, eax + 8u);
        s.at(0x1bfc43u); ecx = 1;
        s.at(0x1bfc48u); s.flags.minus(edx, ecx, 4);
        s.at(0x1bfc4au);
        if (s.flags.zf) {
            const auto configured = s.read(0x1bfc4cu, eax + 0x132u, 2);
            ecx = Word(std::bit_cast<std::int16_t>(std::uint16_t(configured)));
        }
        s.push_word(0x1bfc53u, 0x15u);
        s.call(0x1bfc55u, 0x1f1950u, 0x1bfc5au, 4);
        s.xmm[0] = _mm_mul_ss(s.xmm[0], s.scalar(0x1bfc5au, edi + 0x18u));
    }

    s.at(0x1bfc5fu); s.flags.logic(ebp, 4);
    s.at(0x1bfc61u);
    if (s.flags.zf) return false;
    ecx = s.read(0x1bfc63u, esi + 0x338u);
    edx = s.read(0x1bfc69u, 0x50eef0u);
    edi = s.read(0x1bfc6fu, edx + 0x44u);
    s.at(0x1bfc72u); ecx = s.flags.logic(ecx & 0xffffu, 4);
    s.at(0x1bfc78u); ecx = s.flags.product(ecx, 0x3cu);
    s.set(0, s.read(0x1bfc7bu, ecx + edi + 0x27u, 1), 1);
    s.at(0x1bfc7fu); ecx = s.flags.plus(ecx, edi, 4);
    s.at(0x1bfc81u); s.flags.minus(eax, 2, 1);
    s.at(0x1bfc83u);
    if (s.flags.sf != s.flags.of) return false;
    s.xmm[1] = s.scalar(0x1bfc85u, ebp + 0x34u);
    const auto range = s.scalar(0x1bfc8au, ecx + 0x28u);
    s.flags.scalar_compare(s.xmm[1], range, false);
    s.at(0x1bfc8eu);
    if (s.flags.cf || s.flags.zf) return false;

    s.call(0x1bfc90u, 0x2673a0u, 0x1bfc95u);
    s.flags.minus(s.read(0x1bfc95u, eax + 0x3cu), none, 4);
    s.at(0x1bfc99u);
    if (!s.flags.zf) return false;
    esi = s.read(0x1bfc9bu, esi + 0x1f4u);
    s.at(0x1bfca1u); s.flags.minus(esi, none, 4);
    s.at(0x1bfca4u);
    if (s.flags.zf) return true;
    eax = s.read(0x1bfca6u, 0x51df54u);
    ecx = s.read(0x1bfcabu, eax + 8u);
    s.at(0x1bfcaeu); ecx = s.flags.minus(ecx, esi, 4);
    s.at(0x1bfcb0u); s.xmm[1] = _mm_cvtsi32_ss(s.xmm[1], std::bit_cast<std::int32_t>(ecx));
    s.xmm[1] = _mm_mul_ss(s.xmm[1], s.scalar(0x1bfcb4u, eax + 4u));
    s.at(0x1bfcb9u); s.flags.scalar_compare(s.xmm[1], s.xmm[0], false);
    s.at(0x1bfcbcu);
    return !s.flags.cf && !s.flags.zf;
}

void run_score(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    ecx = s.read(entry, actor_pool);
    edx = s.read(0x1bfbf6u, ecx + 0x44u);
    eax = s.read(0x1bfbf9u, initial_behavior);
    s.push_word(0x1bfbfeu, ebx);
    ebx = s.read(0x1bfbffu, esp + 8u);
    s.push_word(0x1bfc03u, esi);
    s.at(0x1bfc04u); esi = ebx;
    s.at(0x1bfc06u); esi = s.flags.logic(esi & 0xffffu, 4);
    s.at(0x1bfc0cu); esi = s.flags.product(esi, 0x888u);
    ecx = s.read(0x1bfc12u, esi + edx + 0x338u);
    s.at(0x1bfc19u); esi = s.flags.plus(esi, edx, 4);
    s.at(0x1bfc1bu); s.flags.minus(ecx, none, 4);
    s.at(0x1bfc1eu);
    if (!s.flags.zf) {
        s.push_word(0x1bfc24u, ebp);
        s.push_word(0x1bfc25u, edi);
        if (select_behavior(s)) {
            edi = s.pop_word(0x1bfcbeu);
            ebp = s.pop_word(0x1bfcbfu);
            esi = s.pop_word(0x1bfcc0u);
            s.at(0x1bfcc1u); eax = 0x10u;
            ebx = s.pop_word(0x1bfcc6u);
            s.finish(0x1bfcc7u, 0x1b012bu, 8);
            return;
        }
        s.set(0, s.read(0x1bfccau, initial_behavior, 2), 2);
        edi = s.pop_word(0x1bfcd0u);
        ebp = s.pop_word(0x1bfcd1u);
    }
    esi = s.pop_word(0x1bfcd2u);
    ebx = s.pop_word(0x1bfcd3u);
    s.finish(0x1bfcd4u, 0x1b012bu, 8);
}
}

bool native_campaign_ai_behavior_score_50_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint controls,
    const SourcePhysicsWorldRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe) {
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x1b0128u ||
        call.target != entry || call.tail || abi.gp[7] != descriptor)
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
