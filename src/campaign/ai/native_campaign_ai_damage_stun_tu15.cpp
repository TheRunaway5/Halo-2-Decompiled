#include "halo2/native_campaign_ai_damage_stun_tu15.hpp"
#include "native_campaign_ai_behavior_score_frame_tu15.hpp"

#include <bit>

namespace halo2 {
using namespace native_campaign_ai_behavior_score_detail;

bool native_campaign_ai_damage_stun_tu15_try_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactImage image, const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    constexpr Word entry = 0x1d20c0u;
    if (image != SourcePhysicsContactImage::TitleUpdate15 || call.tail ||
        call.target != entry || (call.instruction != 0xcb8d4u && call.instruction != 0xcca54u))
        return false;

    const auto active = abi.floating_point.value_or(fp);
    FpScope scope(active.x87_control_word, active.mxcsr);
    Frame s(memory, image, services, observe);
    s.load(abi);
    const auto restore = [&] {
        if (services.restore_host_fp) services.restore_host_fp();
    };
    const auto finish_frame = [&] {
        abi = s.save();
        if (!abi.floating_point) abi.floating_point = active;
        abi.floating_point->mxcsr = _mm_getcsr();
        restore();
    };

    try {
        s.gp[1] = s.read(entry, 0x5020a8u);
        s.gp[2] = (s.gp[2] & ~0xffu) | s.read(0x1d20c6u, s.gp[1] + 1u, 1);
        s.at(0x1d20c9u); s.flags.logic(s.gp[2] & 0xffu, 1);
        s.at(0x1d20cbu);
        if (s.flags.zf) {
            s.finish(0x1d20f1u, 0xcca59u);
            finish_frame();
            return true;
        }
        s.at(0x1d20cdu); s.gp[0] = s.flags.logic(s.gp[0] & 0xffffu, 4);
        s.at(0x1d20d2u); s.gp[2] = s.gp[0] + s.gp[0] * 2u;
        s.gp[0] = s.read(0x1d20d5u, 0x4ecbd0u);
        s.gp[1] = s.read(0x1d20dau, s.gp[0] + 0x44u);
        s.gp[0] = s.read(0x1d20ddu, s.gp[1] + s.gp[2] * 4u + 8u);
        s.gp[0] = s.read(0x1d20e1u, s.gp[0] + 0x12cu);
        s.at(0x1d20e7u); s.flags.minus(s.gp[0], 0xffffffffu, 4);
        s.at(0x1d20eau);
        if (s.flags.zf) {
            s.finish(0x1d20f1u, 0xcca59u);
            finish_frame();
            return true;
        }
        s.at(0x1d20ecu); // tail jmp 0x1eab60

        s.gp[1] = s.read(0x1eab60u, 0x5020c8u);
        s.gp[2] = s.read(0x1eab66u, s.gp[1] + 0x44u);
        s.at(0x1eab69u); s.gp[0] = s.flags.logic(s.gp[0] & 0xffffu, 4);
        s.at(0x1eab6eu); s.gp[0] = s.flags.product(s.gp[0], 0x888u);
        s.at(0x1eab74u); s.gp[0] = s.flags.plus(s.gp[0], s.gp[2], 4);
        s.push_word(0x1eab76u, s.gp[6]);
        s.at(0x1eab77u); s.gp[6] = s.gp[0];
        s.gp[1] = s.read(0x1eab79u, s.gp[6] + 0x54u);
        s.call(0x1eab7cu, 0x1eccc0u, 0x1eab81u);
        s.at(0x1eab81u); s.flags.logic(s.gp[0], 4);
        s.at(0x1eab83u);
        if (!s.flags.zf) {
            const auto threshold = s.scalar(0x1eab85u, s.gp[0] + 0x50u);
            s.flags.scalar_compare(s.xmm[0], threshold, false);
            s.at(0x1eab89u);
            if (!s.flags.cf && !s.flags.zf) {
                s.at(0x1eab8bu); w(memory, observe, 0x1eab8bu, s.gp[6] + 0x3b8u, 1, 1u);
                s.store(0x1eab92u, s.gp[6] + 0x3bcu, std::bit_cast<Word>(_mm_cvtss_f32(s.xmm[0])));
                s.gp[0] = s.read(0x1eab9au, s.gp[7]);
                s.at(0x1eab9cu); s.gp[6] = s.flags.plus(s.gp[6], 0x3c0u, 4);
                s.at(0x1eaba2u); s.gp[2] = s.gp[6];
                s.store(0x1eaba4u, s.gp[2], s.gp[0]);
                s.gp[1] = s.read(0x1eaba6u, s.gp[7] + 4u);
                s.store(0x1eaba9u, s.gp[2] + 4u, s.gp[1]);
                s.gp[0] = s.read(0x1eabacu, s.gp[7] + 8u);
                s.store(0x1eabafu, s.gp[2] + 8u, s.gp[0]);
                s.at(0x1eabb2u); s.gp[0] = s.gp[6];
                s.call(0x1eabb4u, 0x30bd0u, 0x1eabb9u);
                const auto zero = s.scalar(0x1eabb9u, 0x46a0e8u);
                s.flags.scalar_compare(s.xmm[0], zero, true);
                s.at(0x1eabc0u);
                const Word ah = (s.flags.bits() & 0xd5u) | 2u;
                s.gp[0] = (s.gp[0] & ~0xff00u) | (ah << 8u);
                s.at(0x1eabc1u); s.flags.logic(ah & 0x44u, 1);
                s.at(0x1eabc4u);
                if (!s.flags.pf) {
                    s.gp[1] = s.read(0x1eabc6u, 0x474d08u);
                    s.gp[2] = s.read(0x1eabccu, s.gp[1]);
                    s.store(0x1eabceu, s.gp[6], s.gp[2]);
                    s.gp[0] = s.read(0x1eabd0u, s.gp[1] + 4u);
                    s.store(0x1eabd3u, s.gp[6] + 4u, s.gp[0]);
                    s.gp[1] = s.read(0x1eabd6u, s.gp[1] + 8u);
                    s.store(0x1eabd9u, s.gp[6] + 8u, s.gp[1]);
                }
            }
        }
        s.gp[6] = s.pop_word(0x1eabdcu);
        s.finish(0x1eabddu, 0xcca59u);
        finish_frame();
        return true;
    } catch (...) {
        abi = s.save();
        if (!abi.floating_point) abi.floating_point = active;
        abi.floating_point->mxcsr = _mm_getcsr();
        restore();
        throw;
    }
}

}
