#include "halo2/native_campaign_ai_behavior_score_80_tu15.hpp"
#include "native_campaign_ai_behavior_score_frame_tu15.hpp"

namespace halo2 {
namespace {
using namespace native_campaign_ai_behavior_score_detail;
constexpr Word entry = 0x1c8590u, descriptor = 0x48b6d0u, none = 0xffffffffu;

void character_property(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.at(0x1ecd80u); eax = s.flags.logic(0, 4);
    s.at(0x1ecd82u); s.flags.minus(ecx, none, 4);
    s.at(0x1ecd85u);
    if (s.flags.zf) {
        s.finish(0x1ecdb8u, 0x1c85f5u);
        return;
    }
    edx = s.read(0x1ecd87u, 0x4f0414u);
    s.push_word(0x1ecd8du, esi);
    s.at(0x1ecd8eu);
    for (;;) {
        s.at(0x1ecd90u); ecx = s.flags.logic(ecx & 0xffffu, 4);
        s.at(0x1ecd96u); ecx = s.flags.shift(ecx, 4, 4);
        ecx = s.read(0x1ecd99u, ecx + edx + 8u);
        esi = s.read(0x1ecd9du, ecx + 0xbcu);
        s.at(0x1ecda3u); s.flags.logic(esi, 4);
        s.at(0x1ecda5u);
        if (!s.flags.zf && !s.flags.sf) {
            eax = s.read(0x1ecdb1u, ecx + 0xc0u);
            esi = s.pop_word(0x1ecdb7u);
            s.finish(0x1ecdb8u, 0x1c85f5u);
            return;
        }
        ecx = s.read(0x1ecda7u, ecx + 8u);
        s.at(0x1ecdaau); s.flags.minus(ecx, none, 4);
        s.at(0x1ecdad);
        if (s.flags.zf) break;
    }
    esi = s.pop_word(0x1ecdafu);
    s.finish(0x1ecdb0u, 0x1c85f5u);
}

void score_property(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.push_word(0x1c85f0u, 0x1c85f5u);
    character_property(s);
    s.at(0x1c85f5u); edi = eax;
    s.at(0x1c85f7u); s.flags.logic(edi, 4);
    s.at(0x1c85f9u);
    if (s.flags.zf) return;

    edx = s.read(0x1c85ffu, 0x50eef0u);
    eax = s.read(0x1c8605u, edx + 0x44u);
    s.at(0x1c8608u); esi = s.flags.logic(esi & 0xffffu, 4);
    s.at(0x1c860eu); esi = s.flags.product(esi, 0x3cu);
    s.at(0x1c8611u); esi = s.flags.plus(esi, eax, 4);
    s.set(0, s.read(0x1c8613u, esi + 0x24u, 2), 2);
    s.at(0x1c8617u); s.flags.minus(eax, 1, 2);
    s.at(0x1c861bu);
    if (s.flags.sf != s.flags.of) return;
    s.at(0x1c8621u); s.flags.minus(eax, 2, 2);
    s.at(0x1c8625u);
    if (!s.flags.zf && s.flags.sf == s.flags.of) return;

    s.at(0x1c862bu); eax = ebx;
    s.call(0x1c862du, 0x1ea2e0u, 0x1c8632u);
    s.at(0x1c8632u); s.flags.logic(eax, 1);
    s.at(0x1c8634u);
    if (!s.flags.zf) {
        s.xmm[0] = s.scalar(0x1c8636u, edi + 4u);
        const auto threshold = s.scalar(0x1c863bu, esi + 0x28u);
        s.flags.scalar_compare(s.xmm[0], threshold, false);
        s.at(0x1c863fu);
        if (s.flags.cf || s.flags.zf) return;
    }

    s.at(0x1c8645u); ecx = esi;
    s.call(0x1c8647u, 0x2673a0u, 0x1c864cu);
    s.at(0x1c864cu); ecx = esi;
    s.at(0x1c864eu); ebx = eax;
    s.call(0x1c8650u, 0x267450u, 0x1c8655u);
    s.at(0x1c8655u); s.flags.logic(eax, 4);
    s.at(0x1c8657u);
    if (s.flags.zf) return;
    eax = s.read(0x1c865du, ebx + 0x3cu);
    s.at(0x1c8660u); s.flags.minus(eax, none, 4);
    s.at(0x1c8663u);
    if (s.flags.zf) return;

    ecx = s.read(0x1c8669u, 0x4ecbd0u);
    ecx = s.read(0x1c866fu, ecx + 0x44u);
    s.xmm[3] = s.scalar(0x1c8672u, edi + 0xcu);
    s.at(0x1c8677u); eax = s.flags.logic(eax & 0xffffu, 4);
    s.at(0x1c867cu); edx = eax * 3u;
    eax = s.read(0x1c867fu, ecx + edx * 4u + 8u);
    s.xmm[2] = s.scalar(0x1c8683u, eax + 0x88u);
    s.xmm[1] = s.scalar(0x1c868bu, eax + 0x8cu);
    s.xmm[0] = s.scalar(0x1c8693u, eax + 0x90u);
    s.at(0x1c869bu); eax = s.flags.plus(eax, 0x88u, 4);
    s.at(0x1c86a0u); s.xmm[4] = s.xmm[2];
    s.at(0x1c86a3u); s.xmm[4] = _mm_mul_ss(s.xmm[4], s.xmm[2]);
    s.at(0x1c86a7u); s.xmm[2] = s.xmm[1];
    s.at(0x1c86aau); s.xmm[2] = _mm_mul_ss(s.xmm[2], s.xmm[1]);
    s.at(0x1c86aeu); s.xmm[1] = s.xmm[0];
    s.at(0x1c86b1u); s.xmm[1] = _mm_mul_ss(s.xmm[1], s.xmm[0]);
    s.at(0x1c86b5u); s.xmm[4] = _mm_add_ss(s.xmm[4], s.xmm[2]);
    s.at(0x1c86b9u); s.xmm[0] = s.xmm[3];
    s.at(0x1c86bcu); s.xmm[4] = _mm_add_ss(s.xmm[4], s.xmm[1]);
    s.at(0x1c86c0u); s.xmm[0] = _mm_mul_ss(s.xmm[0], s.xmm[3]);
    s.at(0x1c86c4u); s.flags.scalar_compare(s.xmm[0], s.xmm[4], false);
    s.at(0x1c86c7u);
    if (s.flags.cf || s.flags.zf) return;

    eax = s.read(0x1c86c9u, esi + 0x20u);
    s.at(0x1c86ccu); eax = s.flags.logic(eax & 0xffffu, 4);
    s.at(0x1c86d1u); eax *= 3u;
    ecx = s.read(0x1c86d4u, ecx + eax * 4u + 8u);
    eax = s.read(0x1c86d8u, ecx + 0x14u);
    s.at(0x1c86dbu); s.flags.minus(eax, none, 4);
    s.at(0x1c86deu);
    if (s.flags.zf) return;
    edx = s.read(0x1c86e0u, esp + 0x14u);
    s.set(3, s.read(0x1c86e4u, ecx + 0x1fcu, 2), 2);
    s.push_word(0x1c86ebu, eax);
    s.push_word(0x1c86ecu, edx);
    // Deliver this conditional call with its actual child frame. The live
    // required owner still refuses until the 1C83F0/1C1A90 work is provided.
    s.call(0x1c86edu, 0x1c83f0u, 0x1c86f2u, 8);
    s.at(0x1c86f2u); s.flags.logic(eax, 1);
    s.at(0x1c86f4u);
    if (!s.flags.zf) s.store(0x1c86f6u, esp + 0xcu, 3);
}

bool enter_property(Frame &s) {
    auto &eax = s.gp[0];
    auto &esi = s.gp[6];
    esi = s.read(0x1c85c6u, eax + 0x338u);
    s.at(0x1c85ccu); s.flags.minus(esi, none, 4);
    s.at(0x1c85cfu);
    if (s.flags.zf) return false;
    const auto state = s.read(0x1c85d5u, eax + 0x26cu);
    s.flags.minus(state, none, 4);
    s.at(0x1c85dcu);
    if (!s.flags.zf) return false;
    const auto object = s.read(0x1c85e2u, eax + 0x18u);
    s.flags.minus(object, none, 4);
    s.at(0x1c85e6u);
    return !s.flags.zf;
}

void run_score(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.push_word(entry, ecx);
    ecx = s.read(0x1c8591u, 0x5020c8u);
    edx = s.read(0x1c8597u, ecx + 0x44u);
    s.push_word(0x1c859au, ebx);
    ebx = s.read(0x1c859bu, esp + 0xcu);
    s.at(0x1c859fu); eax = ebx;
    s.at(0x1c85a1u); eax = s.flags.logic(eax & 0xffffu, 4);
    s.at(0x1c85a6u); eax = s.flags.product(eax, 0x888u);
    s.set(1, s.read(0x1c85acu, eax + edx + 0x224u, 1), 1);
    s.at(0x1c85b3u); eax = s.flags.plus(eax, edx, 4);
    s.at(0x1c85b5u); s.flags.logic(ecx, 1);
    s.store(0x1c85b7u, esp + 4u, 0);
    s.at(0x1c85bfu);
    if (s.flags.zf) {
        s.push_word(0x1c85c5u, esi);
        if (enter_property(s)) {
            ecx = s.read(0x1c85ecu, eax + 0x54u);
            s.push_word(0x1c85efu, edi);
            score_property(s);
            edi = s.pop_word(0x1c86feu);
        }
        esi = s.pop_word(0x1c86ffu);
    }
    s.set(0, s.read(0x1c8700u, esp + 4u, 2), 2);
    ebx = s.pop_word(0x1c8705u);
    ecx = s.pop_word(0x1c8706u);
    s.finish(0x1c8707u, 0x1b0197u, 4);
}
}

bool native_campaign_ai_behavior_score_80_tu15_try_run(SourcePhysicsContactMemory &memory,
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
