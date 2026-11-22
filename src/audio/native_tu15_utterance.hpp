#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_tu15_utterance {
using namespace physics_world_runtime_detail;
constexpr std::uint32_t no_datum=0xffffffffu;

struct Frame {
    WorldState &state;
    void at(std::uint32_t pc) {state.instruction(pc);}
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        at(pc);return r(state.memory,address,width);
    }
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        at(pc);w(state.memory,state.observe,pc,address,width,value);
    }
    void push(std::uint32_t pc,std::uint32_t value) {
        at(pc);state.push(pc,value);
    }
    std::uint32_t pop(std::uint32_t pc) {
        at(pc);return state.pop();
    }
    void call(std::uint32_t pc,std::uint32_t target,std::uint32_t resume) {
        push(pc,resume);state.required(pc,target,false);
    }
    void ret(std::uint32_t pc,unsigned bytes=0) {
        (void)pop(pc);state.gp[4]+=bytes;
    }
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(read(pc,address)));
    }
    std::uint32_t bits(unsigned index) const {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(state.xmm[index])));
    }
};
inline std::uint32_t signed_word(std::uint32_t word) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(word))));
}

enum class ClockAdmission { empty, first_row_may_expire };

inline bool first_clock_row_expired(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    f.at(0x18cd2a);g[2]=g[6]+g[7]+0x134;
    f.call(0x18cd31,0x213690,0x18cd36);
    g[0]=f.read(0x18cd36,0x51df54);
    g[1]=signed_word(f.read(0x18cd3b,g[0]+2,2));
    f.at(0x18cd3f);s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(g[1]));
    f.at(0x18cd43);s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
    f.write(0x18cd47,g[4]+0x14,f.bits(1));
    s.fp.push(ext(std::bit_cast<float>(f.read(0x18cd4d,g[4]+0x14))));
    f.at(0x18cd51);
    w(s.memory,s.observe,0x18cd51,g[4]+0x18,4,integer_spill(s.fp.st(0),s.fp.status));
    s.fp.pop();
    g[1]=f.read(0x18cd55,g[4]+0x18);
    g[2]=f.read(0x18cd59,g[6]+g[7]+0x12c);
    f.at(0x18cd60);g[2]=s.flags.plus(g[2],g[1],4);
    s.flags.minus(g[2],f.read(0x18cd62,g[0]+8),4);
    f.at(0x18cd65);
    return s.flags.zf || s.flags.sf!=s.flags.of;
}

// 18CCD0 owns the retained selector call before it publishes the row.
inline void select_clock(Frame &f,ClockAdmission admission) {
    auto &s=f.state;
    auto &g=s.gp;
    f.at(0x18ccd0);g[4]=s.flags.minus(g[4],0xc,4);
    f.push(0x18ccd3,g[3]);
    f.push(0x18ccd4,g[5]);
    f.push(0x18ccd5,g[6]);
    f.at(0x18ccd6);g[3]=s.flags.logic(no_datum,4);
    f.push(0x18ccd9,g[7]);
    g[7]=f.read(0x18ccda,0x4f9b58);
    f.at(0x18cce0);g[5]=s.flags.logic(0,4);
    f.write(0x18cce2,g[4]+0x10,g[3]);
    f.at(0x18cce6);g[6]=s.flags.logic(0,4);
    f.at(0x18cce8);
    do {
        g[0]=f.read(0x18ccf0,g[6]+g[7]+0x128);
        s.flags.minus(g[0],f.read(0x18ccf7,g[4]+0x20),4);
        f.at(0x18ccfb);
        if(s.flags.zf || (g[0]!=no_datum &&
            (admission!=ClockAdmission::first_row_may_expire || g[6]!=0)))
            throw std::runtime_error("TU1.5 script sound clock row changed before selection");
        s.flags.minus(f.read(0x18cd1e,g[4]+0x10),no_datum,4);
        f.at(0x18cd23);
        if(s.flags.zf) {
            f.at(0x18cd25);s.flags.minus(g[0],no_datum,4);
            f.at(0x18cd28);
            if(s.flags.zf || first_clock_row_expired(f))f.write(0x18cd67,g[4]+0x10,g[5]);
        }
        f.at(0x18cd6b);g[6]=s.flags.plus(g[6],0x10,4);
        f.at(0x18cd6e);g[5]=s.flags.increment(g[5],4,false);
        f.at(0x18cd6f);s.flags.minus(g[6],0x100,4);
        f.at(0x18cd75);
    } while(s.flags.sf!=s.flags.of);
    f.at(0x18cd7b);s.flags.minus(g[3],no_datum,4);
    f.at(0x18cd7e);
    g[0]=f.read(0x18cd80,g[4]+0x10);
    f.at(0x18cd84);s.flags.minus(g[0],g[3],4);
    f.at(0x18cd86);
    f.at(0x18cd88);g[3]=g[0];
    f.at(0x18cd8a);g[0]=g[3];
    f.at(0x18cd8c);
    const bool auxiliary=s.flags.af;
    g[0]=s.flags.shift(g[0],4,4);
    // Preserve the observed auxiliary bit at the original selector call.
    // SHL does not define AF and the fixed unused-row route does not test it.
    s.flags.af=auxiliary;
    f.at(0x18cd8f);g[6]=g[0]+g[7]+0x128;
    g[0]=f.read(0x18cd96,g[4]+0x20);
    f.push(0x18cd9a,0);
    f.at(0x18cd9c);g[1]=g[6]+0xc;
    f.push(0x18cd9f,g[1]);
    f.call(0x18cda0,0x213410,0x18cda5);
    g[2]=f.read(0x18cda5,0x51df54);
    g[0]=f.read(0x18cdab,g[2]+8);
    g[1]=f.read(0x18cdae,g[4]+0x20);
    f.write(0x18cdb2,g[6]+4,g[0]);
    f.write(0x18cdb5,g[6],g[1]);
    f.write(0x18cdb7,g[6]+0xe,0,1);
    g[7]=f.pop(0x18cdbb);
    g[6]=f.pop(0x18cdbc);
    g[5]=f.pop(0x18cdbd);
    f.at(0x18cdbe);g[0]=g[3];
    g[3]=f.pop(0x18cdc0);
    f.at(0x18cdc1);g[4]=s.flags.plus(g[4],0xc,4);
    f.ret(0x18cdc4,4);
}

inline void build_marker_request(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    g[0]=f.read(0x18cb16,0x4f0414);
    f.at(0x18cb1b);g[2]=g[5];
    f.at(0x18cb1d);g[2]=s.flags.logic(g[2]&0xffffu,4);
    f.at(0x18cb23);g[2]=s.flags.shift(g[2],4,4);
    g[1]=f.read(0x18cb26,g[2]+g[0]+8);
    g[0]=signed_word(f.read(0x18cb2a,g[1]+6,2));
    g[2]=f.read(0x18cb2e,0x52d664);
    f.at(0x18cb34);g[0]=s.flags.product(g[0],0x38);
    g[0]=s.flags.plus(g[0],f.read(0x18cb37,g[2]+4),4);
    s.set(1,f.read(0x18cb3a,g[0]+0x28,1),1);
    f.at(0x18cb3d);g[0]=s.flags.plus(g[0],0x28,4);
    f.at(0x18cb40);g[7]=s.flags.logic(0,4);
    f.at(0x18cb42);s.flags.logic(s.reg(1,1)&7,1);
    f.at(0x18cb45);
    bool directional=false;
    if(!s.flags.zf) {
        f.at(0x18cb47);g[7]=g[0];
        f.at(0x18cb49);s.flags.logic(g[7],4);
        f.at(0x18cb4b);
        if(!s.flags.zf) {
            s.flags.logic(f.read(0x18cb4d,g[7],1)&1,1);
            f.write(0x18cb50,g[4]+0x13,1,1);
            f.at(0x18cb55);directional=!s.flags.zf;
        }
    }
    if(!directional)f.write(0x18cb57,g[4]+0x13,0,1);
    x[0]=f.scalar(0x18cb5c,g[4]+0xbc);
    f.at(0x18cb65);x[1]=_mm_setzero_ps();
    f.at(0x18cb68);s.flags.scalar_compare(x[1],x[0],false);
    f.at(0x18cb6b);
    bool clamp=!(s.flags.cf||s.flags.zf);
    if(!clamp) {
        x[1]=f.scalar(0x18cb6d,0x46a0d0);
        f.at(0x18cb75);s.flags.scalar_compare(x[0],x[1],false);
        f.at(0x18cb78);clamp=!(s.flags.cf||s.flags.zf);
    }
    if(clamp) {
        f.at(0x18cb7a);x[0]=x[1];
        f.write(0x18cb7d,g[4]+0xbc,f.bits(0));
    }
    g[6]=f.read(0x18cb86,g[4]+0xb8);
    f.at(0x18cb8d);s.flags.minus(g[6],no_datum,4);
    f.at(0x18cb90);
    if(s.flags.zf)throw std::runtime_error("TU1.5 attached speech lost its unit");
    f.at(0x18cc02);s.flags.minus(g[6],no_datum,4);
    f.write(0x18cc05,g[4]+0x19,0,1);
    f.at(0x18cc0a);
    f.push(0x18cc0c,1);
    f.push(0x18cc0e,0x04000095);
    f.push(0x18cc13,g[6]);
    f.at(0x18cc14);s.set(1,s.flags.logic(0,1),1);
    f.at(0x18cc16);g[3]=g[4]+0x4c;
    f.call(0x18cc1a,0xbad60,0x18cc1f);
}

inline void play_attached_request(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    g[2]=f.read(0x18cc1f,g[4]+0x40);
    f.push(0x18cc23,g[6]);
    f.push(0x18cc24,g[5]);
    f.at(0x18cc25);g[6]=g[4]+0x20;
    f.at(0x18cc29);g[0]=g[4]+0x50;
    f.at(0x18cc2d);g[1]=g[4]+0x74;
    f.call(0x18cc31,0x18c090,0x18cc36);
    g[3]=f.read(0x18cc36,g[4]+0x14);
    f.at(0x18cc3a);s.flags.logic(g[7],4);
    s.set(0,f.read(0x18cc3c,g[4]+0x19,1),1);
    f.at(0x18cc40);
    if(s.flags.zf) {
        f.at(0x18cc46);s.set(0,s.flags.logic(s.reg(0,1)&0xfeu,1),1);
    } else {
        f.at(0x18cc42);s.set(0,s.flags.logic(s.reg(0,1)|1u,1),1);
        f.at(0x18cc44);
    }
    f.write(0x18cc48,g[4]+0x19,s.reg(0,1),1);
    g[0]=f.read(0x18cc4c,g[4]+0xc8);
    f.at(0x18cc53);g[3]=s.flags.plus(g[3],0xc,4);
    f.push(0x18cc56,g[3]);
    f.call(0x18cc57,0x1903a0,0x18cc5c);
    s.set(3,f.read(0x18cc5c,g[4]+0x17,1),1);
    g[1]=f.read(0x18cc60,g[4]+0xc0);
    f.push(0x18cc67,g[0]);
    f.at(0x18cc68);g[0]=s.flags.logic(0,4);
    f.at(0x18cc6a);s.flags.logic(s.reg(3,1),1);
    g[3]=f.read(0x18cc6c,g[4]+0xc0);
    f.at(0x18cc73);s.set(0,!s.flags.zf,1);
    f.push(0x18cc76,g[1]);
    f.at(0x18cc77);g[2]=g[4]+0x24;
    f.push(0x18cc7b,g[2]);
    f.at(0x18cc7c);g[7]=0x44ec34;
    f.at(0x18cc81);g[6]=g[3];
    f.at(0x18cc83);g[0]=s.flags.increment(g[0],4,false);
    f.push(0x18cc84,g[0]);
    f.push(0x18cc85,g[5]);
    f.call(0x18cc86,0x18c130,0x18cc8b);
    f.at(0x18cc8b);g[6]=g[0];
    f.at(0x18cc8d);s.flags.minus(g[6],no_datum,4);
    f.at(0x18cc90);
    if(!s.flags.zf) {
        f.at(0x18cc92);s.flags.minus(g[3],no_datum,4);
        f.at(0x18cc95);
        if(!s.flags.zf) {
            f.push(0x18cc97,g[6]);
            f.push(0x18cc98,g[5]);
            f.call(0x18cc99,0x10e2d0,0x18cc9e);
        }
    }
    g[3]=f.read(0x18cc9e,g[4]+0x14);
    g[7]=f.pop(0x18cca2);
    f.write(0x18cca3,g[3]+8,g[6]);
    f.at(0x18cca6);s.flags.minus(g[6],no_datum,4);
    g[6]=f.pop(0x18cca9);
    g[3]=f.pop(0x18ccaa);
    f.at(0x18ccab);
}

inline void utterance(Frame &f,ClockAdmission admission=ClockAdmission::empty) {
    auto &s=f.state;
    auto &g=s.gp;
    f.at(0x18cac0);g[4]=s.flags.minus(g[4],0xa0,4);
    f.push(0x18cac6,g[5]);
    g[5]=f.read(0x18cac7,g[4]+0xa8);
    f.at(0x18cace);s.flags.minus(g[5],no_datum,4);
    f.at(0x18cad1);
    bool accepted=!s.flags.zf;
    if(accepted) {
        f.push(0x18cad7,g[5]);
        f.push(0x18cad8,0x18cadd);
        select_clock(f,admission);
        f.at(0x18cadd);s.flags.minus(g[0],no_datum,4);
        f.at(0x18cae0);accepted=!s.flags.zf;
    }
    if(accepted) {
        g[1]=f.read(0x18cae6,0x4f9b58);
        f.push(0x18caec,g[3]);
        f.push(0x18caed,g[6]);
        g[6]=f.read(0x18caee,g[4]+0xc8);
        f.at(0x18caf5);g[0]=s.flags.shift(g[0],4,4);
        f.at(0x18caf8);s.flags.logic(g[6],4);
        f.at(0x18cafa);g[3]=g[0]+g[1]+0x128;
        f.push(0x18cb01,g[7]);
        f.write(0x18cb02,g[4]+0x14,g[3]);
        f.at(0x18cb06);
        if(!s.flags.zf) {
            f.at(0x18cb08);g[2]=g[3]+0xc;
            f.at(0x18cb0b);g[0]=g[5];
            f.call(0x18cb0d,0x213690,0x18cb12);
            f.write(0x18cb12,g[6],f.bits(0));
        }
        build_marker_request(f);
        play_attached_request(f);
        accepted=!s.flags.zf;
    }
    if(!accepted) {
        g[0]=f.read(0x18ccad,g[4]+0xc0);
        f.at(0x18ccb4);s.flags.logic(g[0],4);
        f.at(0x18ccb6);
        if(!s.flags.zf) {
            f.at(0x18ccb8);s.xmm[0]=_mm_setzero_ps();
            f.write(0x18ccbb,g[0],0);
        }
    }
    g[5]=f.pop(0x18ccbf);
    f.at(0x18ccc0);g[4]=s.flags.plus(g[4],0xa0,4);
    f.ret(0x18ccc6,0x1c);
}

// Retains the original actor-owned deadline branch. The separate attention
// record form is outside this entry's admission and stays with its own caller.
inline void actor_utterance(Frame &f,ClockAdmission admission=ClockAdmission::empty) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    f.push(0x27fdb0,g[6]);
    f.push(0x27fdb1,g[7]);
    f.at(0x27fdb2);g[7]=g[0];
    f.at(0x27fdb4);g[6]=g[2];
    g[2]=f.read(0x27fdb6,g[4]+0x14);
    f.at(0x27fdba);g[0]=g[4]+0x14;
    f.push(0x27fdbe,g[0]);
    g[0]=f.read(0x27fdbf,0x44ebd8);
    f.push(0x27fdc4,g[1]);
    f.at(0x27fdc5);g[6]=s.flags.logic(g[6]&0xffffu,4);
    f.push(0x27fdcb,g[0]);
    f.at(0x27fdcc);g[6]=s.flags.product(g[6],0x888);
    f.push(0x27fdd2,g[0]);
    g[0]=f.read(0x27fdd3,0x5020c8);
    g[1]=f.read(0x27fdd8,g[0]+0x44);
    g[0]=f.read(0x27fddb,g[4]+0x1c);
    f.push(0x27fddf,g[2]);
    g[2]=f.read(0x27fde0,g[1]+g[6]+0x18);
    f.push(0x27fde4,g[2]);
    f.push(0x27fde5,g[0]);
    f.push(0x27fde6,0x27fdeb);
    utterance(f,admission);
    g[1]=f.read(0x27fdeb,0x51df54);
    g[2]=signed_word(f.read(0x27fdf1,g[1]+2,2));
    x[0]=f.scalar(0x27fdf5,g[4]+0x14);
    x[0]=_mm_mul_ss(x[0],f.scalar(0x27fdfb,g[4]+0x10));
    f.at(0x27fe01);x[1]=_mm_cvtsi32_ss(x[1],si(g[2]));
    f.at(0x27fe05);x[1]=_mm_mul_ss(x[1],x[0]);
    f.write(0x27fe09,g[4]+0xc,f.bits(1));
    s.fp.push(ext(std::bit_cast<float>(f.read(0x27fe0f,g[4]+0xc))));
    f.at(0x27fe13);
    w(s.memory,s.observe,0x27fe13,g[4]+0x14,4,integer_spill(s.fp.st(0),s.fp.status));
    s.fp.pop();
    f.at(0x27fe17);s.flags.minus(g[7],no_datum,4);
    f.at(0x27fe1a);
    if(!s.flags.zf)throw std::runtime_error("TU1.5 actor utterance lost its deadline owner");
    g[1]=f.read(0x27fe48,0x5020c8);
    g[0]=f.read(0x27fe4e,g[1]+0x44);
    g[1]=f.read(0x27fe51,g[4]+0x14);
    f.at(0x27fe55);g[0]=s.flags.plus(g[0],g[6],4);
    s.flags.minus(f.read(0x27fe57,g[0]+0x620,2),s.reg(1,2),2);
    f.at(0x27fe5e);
    if(s.flags.sf!=s.flags.of)f.write(0x27fe60,g[0]+0x620,s.reg(1,2),2);
    g[7]=f.pop(0x27fe67);
    g[6]=f.pop(0x27fe68);
    f.ret(0x27fe69,0xc);
}
inline void run(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observer,
    ClockAdmission admission=ClockAdmission::empty) {
    if(!abi.floating_point || abi.active_mmx)throw std::runtime_error("TU1.5 actor utterance lacks its retained FP owner");
    FpScope controls(*abi.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    Frame frame{state};
    try {actor_utterance(frame,admission);}catch(...) {abi=state.save();throw;}
    abi=state.save();
}
} // namespace halo2::native_tu15_utterance
