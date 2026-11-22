#pragma once

#include "audio/native_tu15_utterance.hpp"

namespace halo2::native_tu15_object_utterance {
using native_tu15_utterance::ClockAdmission;
using native_tu15_utterance::Frame;
using native_tu15_utterance::no_datum;
using native_tu15_utterance::signed_word;
using namespace physics_world_runtime_detail;

// The object selector shares the scenario's mission dialogue data but uses
// its first variant directly, with no actor search or attention publication.
inline void select_line(Frame &f,ClockAdmission admission) {
    auto &s=f.state;
    auto &g=s.gp;
    g[1]=f.read(0x29bd90,0x4ecc20);
    f.push(0x29bd96,g[5]);
    g[5]=f.read(0x29bd97,g[4]+0xc);
    f.at(0x29bd9b);s.set(0,s.flags.logic(0,1),1);
    f.at(0x29bd9d);s.flags.logic(g[5],4);
    f.at(0x29bd9f);
    if(!s.flags.zf) {
        f.at(0x29bda1);s.xmm[0]=_mm_setzero_ps();
        f.write(0x29bda4,g[5],0);
    }
    g[2]=f.read(0x29bda9,g[1]+0x3b0);
    f.at(0x29bdaf);s.flags.logic(g[2],4);
    f.at(0x29bdb1);
    if(!s.flags.zf && s.flags.sf==s.flags.of) {
        g[1]=f.read(0x29bdb7,g[1]+0x3b4);
        g[1]=f.read(0x29bdbd,g[1]+4);
        f.at(0x29bdc0);s.flags.minus(g[1],no_datum,4);
        f.at(0x29bdc3);
        if(!s.flags.zf) {
            g[2]=f.read(0x29bdc5,0x4f0414);
            f.at(0x29bdcb);g[1]=s.flags.logic(g[1]&0xffffu,4);
            f.at(0x29bdd1);g[1]=s.flags.shift(g[1],4,4);
            g[2]=f.read(0x29bdd4,g[1]+g[2]+8);
            f.push(0x29bdd8,g[6]);
            f.push(0x29bdd9,g[7]);
            g[7]=f.read(0x29bdda,g[2]);
            f.at(0x29bddc);g[6]=s.flags.logic(0,4);
            f.at(0x29bdde);s.flags.logic(g[7],4);
            f.at(0x29bde0);
            if(!s.flags.zf && s.flags.sf==s.flags.of) {
                g[2]=f.read(0x29bde2,g[2]+4);
                f.at(0x29bde5);g[1]=s.flags.logic(0,4);
                for(;;) {
                    f.at(0x29bde7);g[1]=s.flags.shift(g[1],4,4);
                    f.at(0x29bdea);g[1]=s.flags.plus(g[1],g[2],4);
                    s.flags.minus(f.read(0x29bdec,g[1]),g[3],4);
                    f.at(0x29bdee);
                    if(s.flags.zf)break;
                    f.at(0x29bdf0);g[6]=s.flags.increment(g[6],4,false);
                    f.at(0x29bdf1);g[1]=signed_word(g[6]);
                    f.at(0x29bdf4);s.flags.minus(g[1],g[7],4);
                    f.at(0x29bdf6);
                    if(s.flags.sf==s.flags.of) {
                        g[7]=f.pop(0x29bdf8);g[6]=f.pop(0x29bdf9);g[5]=f.pop(0x29bdfa);
                        f.ret(0x29bdfb,8);return;
                    }
                }
                g[6]=f.read(0x29bdfe,g[1]+4);
                f.at(0x29be01);s.flags.logic(g[6],4);
                g[2]=f.read(0x29be03,g[1]+0xc);
                f.at(0x29be06);
                if(!s.flags.zf && s.flags.sf==s.flags.of) {
                    g[1]=f.read(0x29be08,g[1]+8);
                    g[0]=f.read(0x29be0b,g[1]+0xc);
                    f.at(0x29be0e);s.flags.logic(g[0],4);
                    f.at(0x29be10);
                    if(!s.flags.zf) {
                        f.at(0x29be12);s.flags.minus(g[0],no_datum,4);
                        f.at(0x29be15);
                        if(!s.flags.zf){f.at(0x29be17);g[2]=g[0];}
                    }
                    g[0]=f.read(0x29be19,0x44ebd8);
                    g[1]=f.read(0x29be1e,g[1]+8);
                    f.push(0x29be21,g[5]);
                    f.push(0x29be22,g[2]);
                    f.push(0x29be23,g[0]);
                    f.push(0x29be24,g[0]);
                    g[0]=f.read(0x29be25,g[4]+0x20);
                    f.push(0x29be29,0x3f800000);
                    f.push(0x29be2e,g[0]);
                    f.push(0x29be2f,g[1]);
                    f.push(0x29be30,0x29be35);
                    native_tu15_utterance::utterance(f,admission);
                    f.at(0x29be35);s.set(0,1,1);
                }
            }
            g[7]=f.pop(0x29be37);
            g[6]=f.pop(0x29be38);
        }
    }
    g[5]=f.pop(0x29be39);
    f.ret(0x29be3a,8);
}

// The native ScriptVm has already evaluated the two operands and completes
// the expression itself. Keep the literal 2B0200 output arithmetic and the
// same retained 29BD90 call and stores between those two VM operations.
inline void script_line(Frame &f,std::uint32_t object,std::uint32_t line,ClockAdmission admission) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    f.at(0x2b0229);x[0]=_mm_setzero_ps();
    f.push(0x2b022c,g[3]);
    f.at(0x2b022d);g[3]=line;
    f.at(0x2b0230);s.flags.minus(g[3],no_datum,4);
    f.write(0x2b0233,g[4]+0xc,0);
    f.at(0x2b0239);
    if(!s.flags.zf) {
        f.at(0x2b023b);g[2]=object;
        f.at(0x2b023d);g[1]=g[4]+0xc;
        f.push(0x2b0241,g[1]);
        f.push(0x2b0242,g[2]);
        f.push(0x2b0243,0x2b0248);
        select_line(f,admission);
        x[0]=f.scalar(0x2b0248,g[4]+0xc);
    }
    x[0]=_mm_mul_ss(x[0],f.scalar(0x2b024e,0x46a20c));
    f.write(0x2b0256,g[4]+0xc,f.bits(0));
    s.fp.push(ext(std::bit_cast<float>(f.read(0x2b025c,g[4]+0xc))));
    f.at(0x2b0260);
    w(s.memory,s.observe,0x2b0260,g[4]+0x14,4,integer_spill(s.fp.st(0),s.fp.status));
    s.fp.pop();
    g[0]=signed_word(f.read(0x2b0264,g[4]+0x14,2));
    f.at(0x2b0269);x[0]=_mm_cvtsi32_ss(x[0],si(g[0]));
    f.write(0x2b026d,g[4]+0xc,f.bits(0));
    g[0]=f.read(0x2b0273,g[4]+0xc);
    g[3]=f.pop(0x2b027e);
}

inline void run(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observer,
    std::uint32_t object,std::uint32_t line,ClockAdmission admission) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth)
        throw std::runtime_error("TU1.5 object utterance lacks its retained FP owner");
    FpScope controls(*abi.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    Frame frame{state};
    try {script_line(frame,object,line,admission);}catch(...) {abi=state.save();throw;}
    abi=state.save();
}
} // namespace halo2::native_tu15_object_utterance
