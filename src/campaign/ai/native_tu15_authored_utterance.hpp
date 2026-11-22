#pragma once

#include "audio/native_tu15_utterance.hpp"

namespace halo2::native_tu15_authored_utterance {
using native_tu15_utterance::Frame;
using native_tu15_utterance::no_datum;
using native_tu15_utterance::signed_word;
using namespace physics_world_runtime_detail;

inline bool resolve_line_sound(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    f.at(0x29bc5f);g[0]=r(s.memory,0x4ecc1c);
    f.at(0x29bc64);g[2]=s.flags.logic(0,4);
    f.at(0x29bc66);s.flags.logic(g[0],4);
    f.at(0x29bc68);
    if(!s.flags.zf) {
        g[7]=f.read(0x29bc6a,g[0]+0xc8);
        f.at(0x29bc70);s.flags.logic(g[7],4);
        f.at(0x29bc72);
        if(!s.flags.zf && s.flags.sf==s.flags.of) {
            g[0]=f.read(0x29bc74,g[0]+0xcc);
            g[2]=f.read(0x29bc7a,g[0]+0x68);
        }
    }
    g[0]=f.read(0x29bc7d,g[1]+0xc);
    f.at(0x29bc80);s.flags.logic(g[0],4);
    f.at(0x29bc82);
    bool override_line=false;
    if(!s.flags.zf) {
        f.at(0x29bc84);s.flags.minus(g[0],no_datum,4);
        f.write(0x29bc87,g[4]+0x14,g[0]);
        f.at(0x29bc8b);override_line=!s.flags.zf;
    }
    if(!override_line)f.write(0x29bc8d,g[4]+0x14,g[2]);
    g[0]=f.read(0x29bc91,0x4ecbd0);
    g[0]=f.read(0x29bc96,g[0]+0x44);
    f.at(0x29bc99);g[6]=s.flags.logic(g[6]&0xffffu,4);
    f.at(0x29bc9f);g[2]=g[6]*3;
    g[6]=f.read(0x29bca2,g[0]+g[2]*4+8);
    g[7]=signed_word(f.read(0x29bca6,g[6]+0x342,2));
    f.at(0x29bcad);g[7]=s.flags.plus(g[7],g[6],4);
    g[6]=f.read(0x29bcaf,g[7]);
    f.at(0x29bcb1);s.flags.minus(g[6],no_datum,4);
    f.at(0x29bcb4);
    if(s.flags.zf)return true;
    g[7]=f.read(0x29bcba,g[7]+8);
    f.at(0x29bcbd);g[6]=s.flags.logic(g[6]&0xffffu,4);
    f.at(0x29bcc3);g[6]=s.flags.shift(g[6],4,4);
    f.at(0x29bcc6);s.flags.logic(g[7],4);
    g[2]=f.read(0x29bcc8,g[6]+g[5]+8);
    f.at(0x29bccc);
    if(!s.flags.zf) {
        f.at(0x29bcce);s.flags.minus(g[7],no_datum,4);
        f.at(0x29bcd1);
        if(!s.flags.zf)f.write(0x29bcd3,g[4]+0x14,g[7]);
    }
    g[0]=f.read(0x29bcd7,g[4]+0x14);
    f.at(0x29bcdb);s.flags.minus(g[0],no_datum,4);
    f.at(0x29bcde);
    bool restore=s.flags.zf;
    if(!restore) {
        f.at(0x29bce0);s.flags.logic(g[0],4);
        f.at(0x29bce2);restore=s.flags.zf;
    }
    if(restore)f.write(0x29bce4,g[4]+0x14,g[7]);
    g[5]=f.read(0x29bce8,g[1]+4);
    f.at(0x29bceb);g[3]=s.flags.logic(0,4);
    f.at(0x29bced);s.flags.logic(g[5],4);
    f.at(0x29bcef);
    if(s.flags.zf || s.flags.sf!=s.flags.of)return true;
    g[0]=f.read(0x29bcf5,g[1]+8);
    g[7]=f.read(0x29bcf8,g[2]+0x14);
    f.at(0x29bcfb);g[6]=s.flags.logic(0,4);
    f.at(0x29bcfd);
    do {
        f.at(0x29bd00);g[6]=s.flags.shift(g[6],4,4);
        g[1]=f.read(0x29bd03,g[6]+g[0]);
        f.at(0x29bd06);g[6]=s.flags.plus(g[6],g[0],4);
        f.at(0x29bd08);s.flags.minus(g[1],g[7],4);
        f.at(0x29bd0a);
        if(s.flags.zf) {
            g[0]=f.read(0x29bd22,g[6]+8);
            f.at(0x29bd25);s.flags.minus(g[0],no_datum,4);
            f.at(0x29bd28);
            if(s.flags.zf)return true;
            g[6]=f.read(0x29bd2a,g[6]+0xc);
            f.at(0x29bd2d);s.flags.logic(g[6],4);
            f.at(0x29bd2f);
            if(!s.flags.zf) {
                f.at(0x29bd31);s.flags.minus(g[6],no_datum,4);
                f.at(0x29bd34);
                if(!s.flags.zf)f.write(0x29bd36,g[4]+0x14,g[6]);
                f.at(0x29bd3a);s.flags.logic(g[6],4);
                f.at(0x29bd3c);
                if(!s.flags.zf) {
                    f.at(0x29bd3e);s.flags.minus(g[6],no_datum,4);
                    f.at(0x29bd41);
                    if(!s.flags.zf)f.write(0x29bd43,g[4]+0x14,g[6]);
                }
            }
            g[1]=f.read(0x29bd47,g[4]+0x14);
            g[2]=f.read(0x29bd4b,g[4]+0x1c);
            f.push(0x29bd4f,0x3f800000);
            f.push(0x29bd54,0x3f800000);
            f.push(0x29bd59,g[0]);
            g[0]=f.read(0x29bd5a,g[4]+0x2c);
            f.call(0x29bd5e,0x27fdb0,0x29bd63);
            g[0]=f.read(0x29bd63,g[4]+0x24);
            f.at(0x29bd67);s.flags.logic(g[0],4);
            f.at(0x29bd69);
            if(!s.flags.zf)f.write(0x29bd6b,g[0],f.bits(0));
            f.write(0x29bd6f,g[4]+0x13,1,1);
            return true;
        }
        f.at(0x29bd0c);g[3]=s.flags.increment(g[3],4,false);
        f.at(0x29bd0d);g[6]=signed_word(s.reg(3,2));
        f.at(0x29bd10);s.flags.minus(g[6],g[5],4);
        f.at(0x29bd12);
    } while(s.flags.sf!=s.flags.of);
    return false;
}

enum class DialogueExit {
    two_saved_registers,
    all_saved_registers,
    no_variation,
    no_line,
    no_dialogue,
};
inline void finish_dialogue(Frame &f,DialogueExit end) {
    auto &s=f.state;
    auto &g=s.gp;
    // The failed search loops each use their own original return sequence.
    if(end==DialogueExit::no_line || end==DialogueExit::no_variation) {
        const std::uint32_t pc=end==DialogueExit::no_line?0x29bc51:0x29bd14;
        s.set(0,f.read(pc,g[4]+0x13,1),1);
        g[5]=f.pop(pc+4);
        g[3]=f.pop(pc+5);
        g[7]=f.pop(pc+6);
        g[6]=f.pop(pc+7);
        f.at(pc+8);g[4]=s.flags.plus(g[4],8,4);
        f.ret(pc+11,0xc);
        return;
    }
    if(end==DialogueExit::all_saved_registers) {
        g[5]=f.pop(0x29bd74);
        g[3]=f.pop(0x29bd75);
    }
    if(end!=DialogueExit::no_dialogue) {
        g[7]=f.pop(0x29bd76);
        g[6]=f.pop(0x29bd77);
    }
    s.set(0,f.read(0x29bd78,g[4]+3,1),1);
    f.at(0x29bd7c);g[4]=s.flags.plus(g[4],8,4);
    f.ret(0x29bd7f,0xc);
}

inline void select_authored_sound(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    f.at(0x29bbb0);g[4]=s.flags.minus(g[4],8,4);
    g[1]=f.read(0x29bbb3,g[4]+0x14);
    f.at(0x29bbb7);s.flags.logic(g[1],4);
    g[0]=f.read(0x29bbb9,0x4ecc20);
    f.write(0x29bbbe,g[4]+3,0,1);
    f.at(0x29bbc3);
    if(!s.flags.zf) {
        f.at(0x29bbc5);s.xmm[0]=_mm_setzero_ps();
        f.write(0x29bbc8,g[1],0);
    }
    g[1]=f.read(0x29bbcc,g[0]+0x3b0);
    f.at(0x29bbd2);s.flags.logic(g[1],4);
    f.at(0x29bbd4);
    if(s.flags.zf || s.flags.sf!=s.flags.of) {
        finish_dialogue(f,DialogueExit::no_dialogue);return;
    }
    g[1]=f.read(0x29bbda,g[4]+0xc);
    f.at(0x29bbde);g[1]=s.flags.logic(g[1]&0xffffu,4);
    f.at(0x29bbe4);g[1]=s.flags.product(g[1],0x888);
    f.push(0x29bbea,g[6]);
    g[6]=f.read(0x29bbeb,0x5020c8);
    f.push(0x29bbf1,g[7]);
    g[7]=f.read(0x29bbf2,g[6]+0x44);
    g[6]=f.read(0x29bbf5,g[1]+g[7]+0x18);
    f.at(0x29bbf9);g[1]=s.flags.plus(g[1],g[7],4);
    f.at(0x29bbfb);s.flags.minus(g[6],no_datum,4);
    f.at(0x29bbfe);
    if(s.flags.zf){finish_dialogue(f,DialogueExit::two_saved_registers);return;}
    g[0]=f.read(0x29bc04,g[0]+0x3b4);
    g[1]=f.read(0x29bc0a,g[0]+4);
    f.at(0x29bc0d);s.flags.minus(g[1],no_datum,4);
    f.at(0x29bc10);
    if(s.flags.zf){finish_dialogue(f,DialogueExit::two_saved_registers);return;}
    f.push(0x29bc16,g[3]);
    f.at(0x29bc17);g[1]=s.flags.logic(g[1]&0xffffu,4);
    f.push(0x29bc1d,g[5]);
    g[5]=f.read(0x29bc1e,0x4f0414);
    f.at(0x29bc24);g[1]=s.flags.shift(g[1],4,4);
    g[7]=f.read(0x29bc27,g[1]+g[5]+8);
    g[3]=f.read(0x29bc2b,g[7]);
    f.at(0x29bc2d);g[0]=s.flags.logic(0,4);
    f.at(0x29bc2f);s.flags.logic(g[3],4);
    f.at(0x29bc31);
    if(!s.flags.zf && s.flags.sf==s.flags.of) {
        g[7]=f.read(0x29bc37,g[7]+4);
        f.at(0x29bc3a);g[1]=s.flags.logic(0,4);
        f.at(0x29bc3c);
        do {
            f.at(0x29bc40);g[1]=s.flags.shift(g[1],4,4);
            f.at(0x29bc43);g[1]=s.flags.plus(g[1],g[7],4);
            s.flags.minus(f.read(0x29bc45,g[1]),g[2],4);
            f.at(0x29bc47);
            if(s.flags.zf) {
                const bool completed=resolve_line_sound(f);
                finish_dialogue(f,completed?DialogueExit::all_saved_registers:DialogueExit::no_variation);
                return;
            }
            f.at(0x29bc49);g[0]=s.flags.increment(g[0],4,false);
            f.at(0x29bc4a);g[1]=signed_word(s.reg(0,2));
            f.at(0x29bc4d);s.flags.minus(g[1],g[3],4);
            f.at(0x29bc4f);
            if(s.flags.sf==s.flags.of){finish_dialogue(f,DialogueExit::no_line);return;}
        } while(true);
    }
    finish_dialogue(f,DialogueExit::all_saved_registers);
}
} // namespace halo2::native_tu15_authored_utterance

namespace halo2::native_tu15_authored_utterance {
inline void publish_speech_attention(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    x[0]=f.scalar(0x27ff32,g[4]+0x10);
    s.flags.scalar_compare(x[0],f.scalar(0x27ff38,0x46a0e8),false);
    f.at(0x27ff3f);
    if(!s.flags.cf && !s.flags.zf)f.write(0x27ff41,g[4]+0xc,f.bits(0));
    g[1]=f.read(0x27ff47,0x5020c8);
    g[2]=f.read(0x27ff4d,g[1]+0x44);
    g[3]=f.read(0x27ff50,0x50eef0);
    x[2]=f.scalar(0x27ff56,0x46a178);
    x[3]=f.scalar(0x27ff5e,0x46aaf4);
    x[1]=f.scalar(0x27ff66,0x46a108);
    f.at(0x27ff6e);g[7]=s.flags.logic(g[7]&0xffffu,4);
    f.at(0x27ff74);g[7]=s.flags.product(g[7],0x888);
    g[1]=f.read(0x27ff7a,g[7]+g[2]+0x58);
    f.at(0x27ff7e);g[7]=s.flags.plus(g[7],g[2],4);
    f.write(0x27ff80,g[4]+0x10,g[7]);
    f.push(0x27ff84,g[5]);
    while(true) {
        f.at(0x27ff85);s.flags.minus(g[1],no_datum,4);
        f.at(0x27ff88);
        if(s.flags.zf)break;
        g[2]=f.read(0x27ff8a,g[3]+0x44);
        g[5]=f.read(0x27ff8d,0x50eef4);
        g[5]=f.read(0x27ff93,g[5]+0x44);
        f.at(0x27ff96);g[0]=g[1];
        f.at(0x27ff98);g[0]=s.flags.logic(g[0]&0xffffu,4);
        f.at(0x27ff9d);g[0]=s.flags.product(g[0],0x3c);
        f.at(0x27ffa0);g[0]=s.flags.plus(g[0],g[2],4);
        g[2]=f.read(0x27ffa2,g[0]+8);
        f.at(0x27ffa5);g[2]=s.flags.logic(g[2]&0xffffu,4);
        f.at(0x27ffab);g[2]=s.flags.product(g[2],0xc4);
        s.flags.minus(f.read(0x27ffb1,g[2]+g[5]+0x25,1),0,1);
        f.at(0x27ffb6);g[7]=g[1];
        g[1]=f.read(0x27ffb8,g[0]+0x2c);
        f.at(0x27ffbb);
        if(s.flags.zf)continue;
        s.set(2,f.read(0x27ffbd,g[0]+0x24,2),2);
        f.at(0x27ffc1);s.flags.minus(s.reg(2,2),1,2);
        f.at(0x27ffc5);x[0]=x[2];
        f.at(0x27ffc8);
        if(s.flags.sf==s.flags.of) {
            f.at(0x27ffca);s.flags.minus(s.reg(2,2),2,2);
            f.at(0x27ffce);
            if(s.flags.zf || s.flags.sf!=s.flags.of){f.at(0x27ffd0);x[0]=x[3];}
        }
        s.flags.scalar_compare(x[1],f.scalar(0x27ffd3,g[0]+0x28),false);
        f.at(0x27ffd7);
        if(!s.flags.cf && !s.flags.zf) {
            f.at(0x27ffd9);x[4]=x[1];
            x[4]=_mm_sub_ss(x[4],f.scalar(0x27ffdc,g[0]+0x28));
            f.at(0x27ffe1);x[4]=_mm_add_ss(x[4],x[0]);
            f.at(0x27ffe5);x[0]=x[4];
        }
        s.flags.scalar_compare(x[0],f.scalar(0x27ffe8,g[4]+0x18),false);
        f.at(0x27ffed);
        if(!s.flags.cf && !s.flags.zf) {
            f.write(0x27ffef,g[4]+0x18,f.bits(0));
            f.at(0x27fff5);g[6]=g[7];
            f.at(0x27fff7);
        }
    }
    f.at(0x27fff9);s.flags.minus(g[6],no_datum,4);
    g[5]=f.pop(0x27fffc);
    f.at(0x27fffd);
    if(!s.flags.zf) {
        g[7]=f.read(0x27ffff,g[3]+0x44);
        g[0]=f.read(0x280002,g[4]+0x10);
        f.at(0x280006);g[6]=s.flags.logic(g[6]&0xffffu,4);
        f.at(0x28000c);g[6]=s.flags.product(g[6],0x3c);
        f.at(0x28000f);g[1]=1;
        f.at(0x280014);g[6]=s.flags.plus(g[6],g[7],4);
        s.flags.minus(f.read(0x280016,g[0]+0x684,2),s.reg(1,2),2);
        f.at(0x28001d);
        if(s.flags.zf || s.flags.sf!=s.flags.of) {
            g[2]=f.read(0x28001f,0x51df54);
            g[2]=signed_word(f.read(0x280025,g[2]+2,2));
            f.at(0x280029);x[0]=_mm_cvtsi32_ss(x[0],si(g[2]));
            x[0]=_mm_mul_ss(x[0],f.scalar(0x28002d,0x46a11c));
            f.write(0x280035,g[4]+0x14,f.bits(0));
            s.fp.push(ext(std::bit_cast<float>(f.read(0x28003b,g[4]+0x14))));
            f.at(0x28003f);
            w(s.memory,s.observe,0x28003f,g[4]+0x10,4,integer_spill(s.fp.st(0),s.fp.status));
            s.fp.pop();
            s.set(2,f.read(0x280043,g[4]+0x10,2),2);
            f.write(0x280048,g[0]+0x686,s.reg(2,2),2);
            f.write(0x28004f,g[0]+0x688,6,2);
            g[2]=f.read(0x280058,g[6]+0x20);
            f.write(0x28005b,g[0]+0x68c,g[2]);
            f.write(0x280061,g[0]+0x684,s.reg(1,2),2);
        }
    }
    x[0]=f.scalar(0x280068,g[4]+0xc);
}

inline void speak_selected_line(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    f.at(0x27fee0);g[4]=s.flags.minus(g[4],0x3c,4);
    f.at(0x27fee3);x[0]=_mm_setzero_ps();
    f.push(0x27fee6,g[3]);
    f.at(0x27fee7);g[3]=g[0];
    f.at(0x27fee9);s.flags.minus(g[3],no_datum,4);
    f.write(0x27feec,g[4]+4,f.bits(0));
    f.at(0x27fef2);
    if(!s.flags.zf) {
        f.push(0x27fef8,g[6]);
        f.at(0x27fef9);g[6]=g[4]+0x14;
        f.call(0x27fefd,0x27caa0,0x27ff02);
        f.call(0x27ff02,0x27cb30,0x27ff07);
        f.at(0x27ff07);s.flags.logic(g[0],4);
        f.at(0x27ff09);
        if(!s.flags.zf) {
            f.push(0x27ff0f,g[7]);
            g[7]=f.read(0x27ff10,g[4]+0x20);
            f.at(0x27ff14);g[0]=g[4]+0x10;
            f.push(0x27ff18,g[0]);
            f.push(0x27ff19,no_datum);
            f.push(0x27ff1b,g[7]);
            f.at(0x27ff1c);g[2]=g[3];
            f.write(0x27ff1e,g[4]+0x20,f.bits(0));
            f.at(0x27ff24);g[6]=s.flags.logic(no_datum,4);
            f.write(0x27ff27,g[4]+0x1c,f.bits(0));
            f.push(0x27ff2d,0x27ff32);
            select_authored_sound(f);
            publish_speech_attention(f);
            g[7]=f.pop(0x28006e);
        }
        g[6]=f.pop(0x28006f);
    }
    x[0]=_mm_mul_ss(x[0],f.scalar(0x280070,0x46a20c));
    f.write(0x280078,g[4]+0xc,f.bits(0));
    s.fp.push(ext(std::bit_cast<float>(f.read(0x28007e,g[4]+0xc))));
    f.at(0x280082);
    w(s.memory,s.observe,0x280082,g[4]+8,4,integer_spill(s.fp.st(0),s.fp.status));
    s.fp.pop();
    s.set(0,f.read(0x280086,g[4]+8,2),2);
    g[3]=f.pop(0x28008b);
    f.at(0x28008c);g[4]=s.flags.plus(g[4],0x3c,4);
    f.ret(0x28008f);
}
inline void speak_actor_line(Frame &f) {
    auto &s=f.state;
    auto &g=s.gp;
    auto &x=s.xmm;
    f.at(0x27fe70);g[4]=s.flags.minus(g[4],0x38,4);
    f.at(0x27fe73);s.flags.minus(g[7],no_datum,4);
    f.at(0x27fe76);x[0]=_mm_setzero_ps();
    f.write(0x27fe79,g[4],f.bits(0));
    f.at(0x27fe7e);
    if(!s.flags.zf) {
        f.push(0x27fe80,g[6]);
        f.at(0x27fe81);g[6]=g[4]+0xc;
        f.call(0x27fe85,0x27caa0,0x27fe8a);
        f.call(0x27fe8a,0x27cb30,0x27fe8f);
        f.at(0x27fe8f);s.flags.logic(g[0],4);
        g[6]=f.pop(0x27fe91);
        f.at(0x27fe92);
        if(!s.flags.zf) {
            g[1]=f.read(0x27fe94,g[4]+0x10);
            f.at(0x27fe98);g[0]=g[4]+4;
            f.push(0x27fe9c,g[0]);
            f.push(0x27fe9d,no_datum);
            f.push(0x27fe9f,g[1]);
            f.at(0x27fea0);g[2]=g[7];
            f.push(0x27fea2,0x27fea7);
            select_authored_sound(f);
            x[0]=f.scalar(0x27fea7,g[4]+4);
            s.flags.scalar_compare(x[0],f.scalar(0x27fead,0x46a0e8),false);
            f.at(0x27feb4);
            if(!s.flags.cf && !s.flags.zf)f.write(0x27feb6,g[4],f.bits(0));
            x[0]=f.scalar(0x27febb,g[4]);
        }
    }
    x[0]=_mm_mul_ss(x[0],f.scalar(0x27fec0,0x46a20c));
    f.write(0x27fec8,g[4]+4,f.bits(0));
    s.fp.push(ext(std::bit_cast<float>(f.read(0x27fece,g[4]+4))));
    f.at(0x27fed2);
    w(s.memory,s.observe,0x27fed2,g[4],4,integer_spill(s.fp.st(0),s.fp.status));
    s.fp.pop();
    s.set(0,f.read(0x27fed5,g[4],2),2);
    f.at(0x27fed9);g[4]=s.flags.plus(g[4],0x38,4);
    f.ret(0x27fedc);
}

inline void run(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observer,
    bool actor_line=false) {
    if(!abi.floating_point || abi.active_mmx)throw std::runtime_error("TU1.5 authored utterance lacks its retained FP owner");
    FpScope controls(*abi.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    Frame frame{state};
    try {
        if(actor_line)speak_actor_line(frame);
        else speak_selected_line(frame);
    }catch(...) {abi=state.save();throw;}
    abi=state.save();
}
}
