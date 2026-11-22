#include "halo2/native_campaign_animation_distance_tu15.hpp"
#include "native_campaign_ai_behavior_score_frame_tu15.hpp"

namespace halo2 {
namespace {
using namespace native_campaign_ai_behavior_score_detail;
constexpr Word entry = 0x1e2f60u, continuation = 0x112320u;

Word sign_byte(Word value) { return Word(std::bit_cast<std::int8_t>(std::uint8_t(value))); }
Word sign_short(Word value) { return Word(std::bit_cast<std::int16_t>(std::uint16_t(value))); }

void prepare_codec(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    s.at(entry); esp = s.flags.minus(esp, 0x14u, 4);
    s.at(0x1e2f63u); s.xmm[0] = _mm_setzero_ps();
    s.push_word(0x1e2f66u, ebx);
    s.push_word(0x1e2f67u, esi);
    s.push_word(0x1e2f68u, edi);
    edi = s.read(0x1e2f69u, esp + 0x24u);
    s.at(0x1e2f6du); esi = eax;
    s.push_word(0x1e2f6fu, edi);
    s.at(0x1e2f70u); edx = esi;
    s.store(0x1e2f72u, esp + 0x10u, 0);
    s.store(0x1e2f78u, esp + 0x14u, 0);
    s.call(0x1e2f7eu, 0x1e3150u, 0x1e2f83u, 4);
    s.push_word(0x1e2f83u, edi);
    s.push_word(0x1e2f84u, esi);
    s.at(0x1e2f85u); edi = esp + 0x1cu;
    s.at(0x1e2f89u); ebx = eax;
    s.store(0x1e2f8bu, esp + 0x1cu, 0);
    s.store(0x1e2f93u, esp + 0x20u, 0);
    s.call(0x1e2f9bu, 0x1e5df0u, 0x1e2fa0u, 8);
}

void find_first_marker(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    edx = s.read(0x1e2fc3u, ebx + 0x4cu);
    s.at(0x1e2fc6u); eax = s.flags.plus(eax, ecx, 4);
    s.at(0x1e2fc8u); ecx = s.flags.logic(0, 4);
    s.at(0x1e2fcau); s.flags.logic(edx, 4);
    s.at(0x1e2fccu);
    if (!s.flags.zf && s.flags.sf == s.flags.of) {
        esi = s.read(0x1e2fceu, ebx + 0x50u);
        for (;;) {
            s.flags.minus(s.read(0x1e2fd1u, esi, 2), 0, 2);
            s.at(0x1e2fd5u);
            if (s.flags.zf) {
                s.set(6, s.read(0x1e3052u, esi + 2u, 2), 2);
                s.at(0x1e3056u);
                return;
            }
            s.at(0x1e2fd7u); ecx = s.flags.increment(ecx, 4, false);
            s.at(0x1e2fd8u); esi = s.flags.plus(esi, 4u, 4);
            s.at(0x1e2fdbu); s.flags.minus(ecx, edx, 4);
            s.at(0x1e2fddu);
            if (s.flags.sf == s.flags.of) break;
        }
    }
    s.at(0x1e2fdfu); esi = s.flags.logic(0xffffffffu, 4);
}

void accumulate_frames(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    eax = s.read(0x1e2fa0u, esp + 0x18u);
    s.flags.minus(s.read(0x1e2fa4u, eax + 2u, 2), 0, 2);
    s.at(0x1e2fa9u);
    if (s.flags.zf) return;
    edx = sign_byte(s.read(0x1e2fabu, eax + 1u, 1));
    ecx = sign_short(s.read(0x1e2fafu, eax + 6u, 2));
    esi = s.read(0x1e2fb3u, eax + 0xcu);
    eax = sign_byte(s.read(0x1e2fb6u, eax, 1));
    s.at(0x1e2fb9u); ecx = s.flags.plus(ecx, edx, 4);
    edx = s.read(0x1e2fbbu, esp + 0x14u);
    s.at(0x1e2fbfu); ecx = s.flags.plus(ecx, esi, 4);
    s.at(0x1e2fc1u); ecx = s.flags.plus(ecx, edx, 4);
    find_first_marker(s);

    s.set(7, s.read(0x1e2fe2u, ebx + 0x14u, 2), 2);
    s.at(0x1e2fe6u); edx = s.flags.logic(0, 4);
    s.at(0x1e2fe8u); s.flags.logic(edi & 0xffffu, 2);
    s.at(0x1e2febu);
    if (s.flags.zf || s.flags.sf != s.flags.of) return;
    ebx = sign_byte(s.read(0x1e2fedu, ebx + 0x11u, 1));
    for (;;) {
        s.at(0x1e2ff1u); ecx = ebx;
        s.at(0x1e2ff3u); ecx = s.flags.increment(ecx, 4, true);
        s.at(0x1e2ff4u);
        if (!s.flags.zf) {
            s.at(0x1e2ff6u); ecx = s.flags.increment(ecx, 4, true);
            s.at(0x1e2ff7u);
            if (!s.flags.zf) {
                s.at(0x1e2ff9u); ecx = s.flags.increment(ecx, 4, true);
                s.at(0x1e2ffau);
            }
        }
        if (s.flags.zf) {
            s.xmm[0] = s.scalar(0x1e2ffcu, eax);
            s.xmm[0] = _mm_add_ss(s.xmm[0], s.scalar(0x1e3000u, esp + 0xcu));
            s.store(0x1e3006u, esp + 0xcu, std::bit_cast<Word>(_mm_cvtss_f32(s.xmm[0])));
            s.at(0x1e300cu); eax = s.flags.plus(eax, 4u, 4);
        }
        s.at(0x1e300fu); s.flags.minus(edx & 0xffffu, esi & 0xffffu, 2);
        s.at(0x1e3012u);
        if (s.flags.zf) {
            s.xmm[0] = s.scalar(0x1e3014u, esp + 0xcu);
            s.store(0x1e301au, esp + 0x10u, std::bit_cast<Word>(_mm_cvtss_f32(s.xmm[0])));
        }
        s.at(0x1e3020u); s.set(2, s.flags.increment(edx & 0xffffu, 2, false), 2);
        s.at(0x1e3021u); s.flags.minus(edx & 0xffffu, edi & 0xffffu, 2);
        s.at(0x1e3024u);
        if (s.flags.sf == s.flags.of) return;
    }
}

void publish_outputs(Frame &s) {
    auto &[eax, ecx, edx, ebx, esp, ebp, esi, edi] = s.gp;
    eax = s.read(0x1e3026u, esp + 0x2cu);
    s.at(0x1e302au); s.flags.logic(eax, 4);
    edi = s.pop_word(0x1e302cu);
    esi = s.pop_word(0x1e302du);
    ebx = s.pop_word(0x1e302eu);
    s.at(0x1e302fu);
    if (!s.flags.zf) {
        s.xmm[0] = s.scalar(0x1e3031u, esp);
        s.store(0x1e3036u, eax, std::bit_cast<Word>(_mm_cvtss_f32(s.xmm[0])));
    }
    eax = s.read(0x1e303au, esp + 0x1cu);
    s.at(0x1e303eu); s.flags.logic(eax, 4);
    s.at(0x1e3040u);
    if (!s.flags.zf) {
        s.xmm[0] = s.scalar(0x1e3042u, esp + 4u);
        s.store(0x1e3048u, eax, std::bit_cast<Word>(_mm_cvtss_f32(s.xmm[0])));
    }
    s.at(0x1e304cu); esp = s.flags.plus(esp, 0x14u, 4);
    s.finish(0x1e304fu, continuation, 12);
}
}

bool native_campaign_animation_distance_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint controls,
    const SourcePhysicsWorldRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe) {
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.instruction != 0x11231bu ||
        call.target != entry || call.tail || abi.active_mmx)
        return false;

    FpScope scope(abi.floating_point.value_or(controls));
    Frame state(memory, observe, services, image);
    state.load(abi);
    try {
        prepare_codec(state);
        accumulate_frames(state);
        publish_outputs(state);
    } catch (...) {
        abi = state.save();
        throw;
    }
    abi = state.save();
    return true;
}

}
