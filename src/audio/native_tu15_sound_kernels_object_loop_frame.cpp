// Native object looping-sound update routines: the frame part of the 18EE50
// group (see native_tu15_sound_object_loop.hpp). 18EE50 is the per-frame
// object-audio entry the live world calls with the elapsed time. In order it
// marks the clusters audible from each camera (18F900), updates the object
// loops (18E970) and the ambience channels (22C560, which pushes the frame's
// DSP parameter blocks through 226370), retires expired impulse slots
// (18ED80, 18F510), switches the ducking state (129DD0), walks the looping
// sound pool (starting, updating or deleting each loop) and finally deletes
// the finished platform voices (223DA0). 18D390 and 18D3D0 are the pool's
// small mark helpers, entries of the same group.
//
// Each routine is a faithful decompilation of the retail TU1.5 function at
// register level: the same memory reads and stores (each store at its
// original instruction address), the same register values at every call and
// at the return, the same SSE scalar arithmetic and comparisons, and the same
// x87 operations under the caller's control word. Branches are evaluated in
// C++ from the operands the original compares, not through emulated EFLAGS.
// Every CALL pushes its original return address (published with the CALL's
// address) and resolves through object_loop_call: a native group routine, the
// lifted original of an unconverted one, or the live world for the core game
// helpers. None of these routines calls 220C60 directly. Retail .rdata
// constants are literals read from the TU1.5 XBE.
#include "audio/native_tu15_sound_object_loop.hpp"
#include <algorithm>

namespace halo2::native_tu15_sound {
namespace {

// Retail .rdata constants (TU1.5 XBE).
constexpr std::uint32_t thousandth=0x3a83126fu;   // 0x46A180: 0.001
constexpr std::uint32_t one=0x3f800000u;          // 0x46A0D0: 1.0
constexpr std::uint32_t ten=0x41200000u;          // 0x46A110: 10.0
constexpr std::uint32_t thirty=0x41f00000u;       // 0x46A20C: 30.0
constexpr std::uint32_t audible_scale=0x43800000u;// 0x46A26C: 256.0 (cluster scale cut-off)

// Retail globals (runtime data, read from memory).
constexpr std::uint32_t sound_state=0x4f2c50u;     // +0x78..0x7A ducking bytes, +0x7E frame flag, +0x88 listeners
                                                   // (0x48 apart; +4 cluster, +6 active), +0x1F8.. ducking state
constexpr std::uint32_t sound_structure=0x4ecc18u; // +0x9C cluster count
constexpr std::uint32_t player_loops=0x4f9b58u;    // +0x24 ducking end tick, +0x128 impulse slots (0x10 each)
constexpr std::uint32_t loop_pool=0x4f9b5cu;       // looping-sound data array (0x18-byte elements)
constexpr std::uint32_t ducking_override=0x51df50u;// +5 byte
constexpr std::uint32_t game_time=0x51df54u;       // +4 seconds per tick (float), +8 current tick
constexpr std::uint32_t voice_array=0x52d670u;     // platform voice data array (0x14-byte elements)
constexpr std::uint32_t channel_array=0x52d668u;   // channel data array (0x1C-byte elements)
constexpr std::uint32_t ambience_channels=0x52d684u;// two 0x14-byte ambience channels
constexpr std::uint32_t dsp_state=0x52d674u;       // +0x2AB0 DSP handle
constexpr std::uint32_t dsp_globals=0x51df90u;     // [+0] parameter records (0x20 each), +0x22 dirty mask
constexpr std::uint32_t listener_position=0x474ce8u;
constexpr std::uint32_t audible_clusters=0x55d730u;// one bit per cluster, rebuilt by 18F900

// --- memory, registers and SSE ---------------------------------------------
// Reads and stores go through the same accessors the lifted routines use.
inline std::uint32_t rd32(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,4);}
inline std::uint32_t rd16(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,2);}
inline std::uint32_t rd8(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,1);}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline std::uint32_t sar(std::uint32_t v,unsigned n){return std::uint32_t(std::int32_t(v)>>n);}
inline bool less(std::uint32_t x,std::uint32_t y){return std::int32_t(x)<std::int32_t(y);}   // CMP; JL
// MOV AL / MOV AX: only the low byte or word of the register changes.
inline void lo8(std::uint32_t &r,std::uint32_t v){r=(r&0xffffff00u)|(v&0xffu);}
inline void lo16(std::uint32_t &r,std::uint32_t v){r=(r&0xffff0000u)|(v&0xffffu);}
inline __m128 single(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}   // MOVSS from memory
inline std::uint32_t lane0(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
// COMISS x,y: CF (below or unordered) and ZF (equal or unordered).
struct Compare {
    bool cf,zf;
    bool above()const{return !cf && !zf;}    // JA
};
inline Compare comiss(__m128 x,__m128 y) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char cf,zf;
    asm volatile("comiss %3,%2\n\tsetb %0\n\tsete %1":"=qm"(cf),"=qm"(zf):"x"(x),"x"(y):"cc");
    return {cf!=0,zf!=0};
#else
    throw FormatError("Object loop scalar comparison backend unavailable");
#endif
}
// FLD m32 as its own load, so a denormal or signalling single signals there.
long double load_single(float x){volatile long double v=static_cast<long double>(x);return v;}

// The floating-point environment across a CALL, as WorldState::save()/load()
// carry it: the callee receives the live control word, MXCSR and x87 sticky
// flags in the ABI and the caller continues from what the callee left.
void fp_out(Abi &a) {
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    a.floating_point=FP{cw,_mm_getcsr()};
    a.x87_status=std::uint16_t((a.x87_status&~0x3fu)|source_x87_exception_flags());
}
void fp_in(Abi &a) {
    if(a.floating_point) {
        const auto p=*a.floating_point;
        asm volatile("fldcw %0"::"m"(p.x87_control_word):"memory");
        _mm_setcsr(p.mxcsr);
    }
    source_load_x87_exception_flags(a.x87_status);
}

// One running routine: the ABI registers (named), its memory, and the
// caller's floating-point environment for the routine's duration.
struct Cpu {
    LoopContext &c;Abi &a;M &m;const O &o;
    std::uint32_t &eax,&ecx,&edx,&ebx,&esp,&ebp,&esi,&edi;
    Floating floating;
    Cpu(LoopContext &context,Abi &abi):c(context),a(abi),m(context.m),o(context.o),
        eax(abi.gp[0]),ecx(abi.gp[1]),edx(abi.gp[2]),ebx(abi.gp[3]),
        esp(abi.gp[4]),ebp(abi.gp[5]),esi(abi.gp[6]),edi(abi.gp[7]),floating(abi,context.fp) {}
    Cpu(const Cpu &)=delete;
    std::uint32_t d(std::uint32_t p)const{return rd32(m,p);}
    std::uint32_t w16(std::uint32_t p)const{return rd16(m,p);}
    std::uint32_t b(std::uint32_t p)const{return rd8(m,p);}
    __m128 ld(std::uint32_t p)const{return single(rd32(m,p));}
    void st(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,4,v);}
    void st16(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,2,v);}
    void st8(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,1,v);}
    // OR/AND m8,imm8: read, combine, store.
    void or8(std::uint32_t site,std::uint32_t p,std::uint32_t v){st8(site,p,b(p)|v);}
    void and8(std::uint32_t site,std::uint32_t p,std::uint32_t v){st8(site,p,b(p)&v);}
    void push(std::uint32_t site,std::uint32_t v){esp-=4u;st(site,esp,v);}
    std::uint32_t pop(){const auto v=d(esp);esp+=4u;return v;}
    // RET n: pops the return address and n bytes of stack arguments.
    void ret(std::uint32_t arguments=0){pop();esp+=arguments;}
    __m128 x(unsigned n)const{return xmm(a,n);}
    void x(unsigned n,__m128 v){set_xmm(a,n,v);}
    // CALL rel32 at `site`: pushes the return address and runs the callee
    // with the live floating-point environment. Stack arguments are pushed
    // beforehand; a callee that pops them (RET n) does so itself.
    void call(std::uint32_t site,std::uint32_t target) {
        fp_out(a);
        push(site,site+5u);
        object_loop_call(c,a,target);
        fp_in(a);
    }
    bool down()const{return (a.eflags&0x400u)!=0;}   // DF
    // REP STOS: ECX elements of EAX (`width` bytes each) at EDI. Without a
    // store trace, an ascending fill writes the same bytes in runs.
    void rep_stos(std::uint32_t site,unsigned width) {
        if(!o && !down()) {
            std::array<std::byte,1024> run;
            for(unsigned i=0;i<run.size();++i)run[i]=std::byte((eax>>((i%width)*8u))&255u);
            while(ecx) {
                const auto n=std::min<std::uint32_t>(ecx,std::uint32_t(run.size()/width));
                m.write(edi,std::span<const std::byte>(run).first(n*width));
                edi+=n*width;ecx-=n;
            }
            return;
        }
        const auto step=down()?0u-width:width;
        while(ecx){physics_step_detail::w(m,o,site,edi,width,eax);edi+=step;--ecx;}
    }
    // REP MOVSD: ECX dwords from ESI to EDI.
    void rep_movsd(std::uint32_t site) {
        const auto step=down()?0xfffffffcu:4u;
        while(ecx){const auto v=d(esi);st(site,edi,v);esi+=step;edi+=step;--ecx;}
    }
    // An operand 18F900 reads from a listener record: with native camera
    // views bound, the view supplies it (as the lifted audio_view_read does).
    std::uint32_t view(std::uint32_t instruction,std::uint32_t p,unsigned size)const {
        if(c.views)if(const auto value=source_object_audio_view_operand_tu15(c.views,d(sound_state),instruction,p,size))
            return *value;
        return physics_step_detail::r(m,p,size);
    }
};

// =====================================================================
// 18D3D0: marks the looping sound EAX (a datum handle) as updated this frame
// (pool element +4 bit 1). Returns EAX = the flags byte's address. ret.
// =====================================================================
void r_18d3d0(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx;
    ecx=x.d(loop_pool);
    edx=x.d(ecx+0x44u);
    eax&=0xffffu;
    eax=eax+eax*2u;
    x.or8(0x18d3e1u,edx+eax*8u+4u,0x2u);
    eax=edx+eax*8u+4u;
    x.ret();
}

// =====================================================================
// 18D390: finds the looping sound owned by EAX (-1: none) through 18BD80
// (stack: -1, 0; ret 8) and, when found, marks it (+4 bit 0) and stores the
// gain in XMM0 at element +8. Returns EAX = the handle or -1. ret.
// =====================================================================
void r_18d390(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&esi=x.esi;
    x.push(0x18d390u,esi);
    esi=eax;
    eax=0xffffffffu;
    if(esi!=0xffffffffu) {
        x.push(0x18d39bu,0u);
        x.push(0x18d39du,0xffffffffu);
        x.call(0x18d39fu,0x18bd80u);
        if(eax!=0xffffffffu) {
            edx=x.d(loop_pool);
            edx=x.d(edx+0x44u);
            ecx=eax;
            ecx&=0xffffu;
            ecx=ecx+ecx*2u;
            x.or8(0x18d3bdu,edx+ecx*8u+4u,0x1u);
            ecx=edx+ecx*8u;
            x.st(0x18d3c5u,ecx+8u,a.xmm[0][0]);
        }
    }
    esi=x.pop();
    x.ret();
}

// =====================================================================
// 129DD0: selects the ducking type ECX (-1: none) in the sound state
// ([4F2C50] +0x1F8 active type, +0x1FC its clock, +0x200 the type fading
// out, +0x204 its clock). Selecting the type that is fading out swaps it
// back in with its clock; any other change starts the new type at clock 0
// and fades the old one from 0. XMM0 = 0 whenever the state changes. ret.
// =====================================================================
void r_129dd0(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&esi=x.esi;
    eax=x.d(sound_state);
    x.push(0x129dd5u,esi);
    esi=x.d(eax+0x1f8u);
    if(ecx!=esi) {
        if(ecx!=0xffffffffu) {
            edx=x.d(eax+0x200u);
            if(ecx==edx) {
                ecx=x.d(eax+0x204u);
                x.x(0,_mm_setzero_ps());
                x.st(0x129df8u,eax+0x1f8u,edx);
                x.st(0x129dfeu,eax+0x1fcu,ecx);
                x.st(0x129e04u,eax+0x204u,0u);
                esi=x.pop();
                x.ret();
                return;
            }
        }
        x.x(0,_mm_setzero_ps());
        x.st(0x129e11u,eax+0x200u,esi);
        x.st(0x129e17u,eax+0x1f8u,ecx);
        x.st(0x129e1du,eax+0x1fcu,0u);
        x.st(0x129e25u,eax+0x204u,0u);
    }
    esi=x.pop();
    x.ret();
}

// =====================================================================
// 18F900: rebuilds the audible-cluster bits ([55D730], one bit per cluster
// of the sound structure [4ECC18]). For each of the four listeners that is
// active in a known cluster (the native camera views when bound), every
// cluster whose scale from the listener's cluster (253070: EAX, ECX
// clusters, EDX structure; XMM0) is below 256 becomes audible. ret.
// =====================================================================
void r_18f900(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    x.x(3,single(audible_scale));
    x.push(0x18f908u,ebx);
    ebx=x.d(sound_structure);
    ecx=x.d(ebx+0x9cu);
    ecx+=0x1fu;
    ecx=sar(ecx,5);
    ecx<<=2;                                   // bytes of cluster bits
    x.push(0x18f91eu,ebp);
    edx=ecx;
    ecx>>=2;
    x.push(0x18f924u,esi);
    x.push(0x18f925u,edi);
    eax=0u;
    edi=audible_clusters;
    x.rep_stos(0x18f92du,4);
    ecx=edx;
    ecx&=3u;
    x.rep_stos(0x18f934u,1);
    edi=x.d(sound_state);
    edi+=0x8cu;                                // listener record +4
    ebp=4u;
    do {
        lo8(eax,x.view(0x18f947u,edi+2u,1));     // active
        if((eax&0xffu)!=0u && x.view(0x18f94eu,edi,2)!=0xffffu) {
            eax=x.d(ebx+0x9cu);
            esi=0u;
            if(std::int32_t(eax)>0) {
                do {
                    ecx=sx16(x.view(0x18f960u,edi,2));
                    eax=esi;
                    edx=ebx;
                    x.call(0x18f967u,0x253070u);
                    if(comiss(x.x(3),x.x(0)).above()) {
                        ecx=esi;
                        ecx&=0x1fu;
                        eax=esi;
                        eax=sar(eax,5);
                        edx=1u;
                        edx<<=ecx&31u;
                        ecx=x.d(audible_clusters+eax*4u);
                        eax=audible_clusters+eax*4u;
                        ecx|=edx;
                        x.st(0x18f992u,eax,ecx);
                    }
                    eax=x.d(ebx+0x9cu);
                    ++esi;
                } while(less(esi,eax));
            }
        }
        edi+=0x48u;
    } while(--ebp!=0u);
    edi=x.pop();
    esi=x.pop();
    ebp=x.pop();
    ebx=x.pop();
    x.ret();
}

// =====================================================================
// 18F510: plays an impulse sound's pitch-range permutation. EDI: the sound
// tag datum; stack: arg0 the playback handle, arg1 the pitch range, arg2 the
// permutation, arg3 the elapsed time (float). Unless the sound is muted
// (definition +1 bit 1) or its class (228220) is not impulse-timed, finds
// the definition's playback row (gestalt +0x54, 0x2C each; 130980 checks
// it); the permutation's start (time * 30 as an FIST integer) feeds 21F860,
// then 18F7A0 and the permutation row go to 18F810. ret 16.
// =====================================================================
void r_18f510(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    ecx=x.d(tag_instances);
    eax=edi;
    eax&=0xffffu;
    x.push(0x18f51du,ebp);
    ebp=x.d(esp+0x10u);                        // arg2
    eax<<=4;
    x.push(0x18f525u,esi);
    esi=x.d(eax+ecx+8u);                       // the sound definition
    if((x.b(esi+1u)&0x2u)==0u) {
        lo16(edx,sx8(x.b(esi+2u)));              // MOVSX DX, class
        x.push(0x18f539u,edx);
        x.call(0x18f53au,0x228220u);
        lo8(eax,x.b(eax+8u));
        lo8(eax,(eax&0xffu)>>1);
        if((eax&0x1u)!=0u) {
            lo16(eax,x.w16(esi+0xeu));
            if((eax&0xffffu)!=0xffffu) {
                ecx=x.d(sound_gestalt);
                x.push(0x18f560u,ebx);
                ebx=sx16(eax);
                eax=x.d(ecx+0x54u);
                ebx*=0x2cu;
                ebx+=eax;
                if(ebx!=0u) {
                    ecx=ebx+8u;
                    eax=2u;
                    x.call(0x18f57au,0x130980u);
                    if((eax&0xffu)!=0u || std::int32_t(x.d(ebx))>0) {
                        eax=sx16(x.w16(esi+8u));
                        edx=x.d(esp+0x14u);        // arg1
                        esi=x.d(sound_gestalt);
                        ecx=x.d(esi+0x24u);
                        x.x(0,x.ld(esp+0x1cu));    // arg3
                        x.x(0,_mm_mul_ss(x.x(0),single(thirty)));
                        eax+=edx;
                        eax=eax+eax*2u;
                        ebx=ecx+eax*4u;
                        x.st(0x18f5afu,esp+0x18u,lane0(x.x(0)));
                        // FLD m32; FISTP m32 (the caller's rounding control).
                        if(a.x87_depth>=8u)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
                        x.floating.push(load_single(std::bit_cast<float>(x.d(esp+0x18u))));
                        x.floating.clear_c1();
                        std::uint16_t status=a.x87_status;
                        const auto start=integer_spill(a.x87[0],status);
                        a.x87_status=status;
                        x.st(0x18f5b9u,esp+0x14u,start);
                        x.floating.pop();
                        eax=x.d(esp+0x14u);
                        x.push(0x18f5c1u,eax);
                        ecx=ebp;
                        eax=edi;
                        x.call(0x18f5c6u,0x21f860u);
                        eax=x.d(esp+0x10u);        // arg0
                        x.call(0x18f5cfu,0x18f7a0u);
                        edx=sx16(x.w16(ebx+8u));
                        ecx=x.d(esp+0x1cu);
                        ebx=x.d(esi+0x2cu);
                        eax=x.d(esp+0x10u);
                        edx+=ebp;
                        x.push(0x18f5e5u,ecx);
                        edx<<=4;
                        edx+=ebx;
                        x.push(0x18f5ebu,edx);
                        x.push(0x18f5ecu,edi);
                        x.push(0x18f5edu,eax);
                        x.call(0x18f5eeu,0x18f810u);
                    }
                }
                ebx=x.pop();
            }
        }
    }
    esi=x.pop();
    ebp=x.pop();
    x.ret(16);
}

// =====================================================================
// 18ED80: the per-player impulse slots ([4F9B58] +0x128, 16 x 0x10 bytes:
// +0 tag datum, +4 start tick, +8 playback handle, +0xC/+0xD pitch range and
// permutation, +0xE active). An active slot whose playback is gone (BCDF0
// with 3) is cleared; otherwise its elapsed time (ticks * seconds per tick,
// [51DF54]) is played (213690 gives the duration, 18F510 plays it) and the
// slot clears once the elapsed time exceeds the duration plus 10 ticks. ret.
// =====================================================================
void r_18ed80(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    esp-=8u;
    x.push(0x18ed83u,ebp);
    x.push(0x18ed84u,esi);
    x.push(0x18ed85u,edi);
    edi=x.d(game_time);
    ebp=0u;
    do {
        eax=x.d(player_loops);
        esi=eax+ebp+0x128u;
        lo8(eax,x.b(esi+0xeu));
        if((eax&0xffu)!=0u) {
            eax=x.d(esi+8u);
            x.push(0x18edaau,3u);
            x.call(0x18edacu,0xbcdf0u);
            bool clear=eax==0u;
            if(!clear) {
                ecx=x.d(edi+8u);
                edx=x.d(esi+4u);
                eax=x.d(esi);
                ecx-=edx;
                x.x(0,_mm_cvtsi32_ss(x.x(0),std::int32_t(ecx)));
                x.x(0,_mm_mul_ss(x.x(0),x.ld(edi+4u)));
                edi=esi+0xcu;
                edx=edi;
                x.st(0x18edcdu,esp+0xcu,lane0(x.x(0)));
                x.call(0x18edd3u,0x213690u);
                edx=x.d(esp+0xcu);
                eax=sx8(x.b(esi+0xdu));
                ecx=sx8(x.b(edi));
                edi=x.d(esi);
                x.push(0x18ede5u,edx);
                edx=x.d(esi+8u);
                x.push(0x18ede9u,eax);
                x.push(0x18edeau,ecx);
                x.push(0x18edebu,edx);
                x.st(0x18edecu,esp+0x20u,a.xmm[0][0]);
                x.call(0x18edf2u,0x18f510u);
                edi=x.d(game_time);
                x.x(0,x.ld(edi+4u));
                x.x(0,_mm_mul_ss(x.x(0),single(ten)));
                x.x(0,_mm_add_ss(x.x(0),x.ld(esp+0x10u)));
                x.x(1,x.ld(esp+0xcu));
                clear=comiss(x.x(1),x.x(0)).above();
            }
            if(clear) {
                x.st(0x18ee1bu,esi,0xffffffffu);
                x.st8(0x18ee21u,esi+0xeu,0u);
                x.st(0x18ee25u,esi+8u,0xffffffffu);
            }
        }
        ebp+=0x10u;
    } while(less(ebp,0x100u));
    edi=x.pop();
    esi=x.pop();
    ebp=x.pop();
    esp+=8u;
    x.ret();
}

// =====================================================================
// 223DA0: walks the platform voice array ([52D670]). A voice whose channel
// is done (+4 bit 0) releases its channel reference (the channel's +2 count,
// low 7 bits; 16E8A0 deletes the channel at zero), clears +8 and is deleted
// (16E8A0: EAX handle, EDX array); any other voice that 223FB0 (ECX) reports
// finished is stopped (224040: handle, 1; ret 8). ret.
// =====================================================================
void r_223da0(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    esp-=0x10u;
    x.push(0x223da3u,ebx);
    x.push(0x223da4u,ebp);
    x.push(0x223da5u,esi);
    x.push(0x223da6u,edi);
    edi=x.d(voice_array);
    x.st(0x223dadu,esp+0x14u,edi);
    ebx=0xffffffffu;
    for(;;) {
        // The next used element after EBX (data array iteration).
        eax=ebx+1u;
        if(std::int32_t(eax)<0)break;
        esi=x.d(edi+0x38u);
        if(!less(eax,esi))break;
        edx=x.d(edi+0x48u);
        bool used=false;
        do {
            ecx=eax;
            ecx&=0x1fu;
            ebx=1u;
            ebx<<=ecx;
            ecx=eax;
            ecx=sar(ecx,5);
            if((x.d(edx+ecx*4u)&ebx)!=0u){used=true;break;}
            ++eax;
        } while(less(eax,esi));
        if(!used || eax==0xffffffffu)break;
        ecx=x.d(edi+0x24u);
        edx=x.d(edi+0x44u);
        ecx*=eax;
        esi=sx16(x.w16(ecx+edx));
        ecx+=edx;
        esi<<=16;
        esi|=eax;                              // the voice's datum handle
        ebx=eax;
        if((x.b(ecx+4u)&0x1u)!=0u) {
            eax=esi;
            eax&=0xffffu;
            edx=eax+eax*4u;
            eax=x.d(voice_array);
            ecx=x.d(eax+0x44u);
            edi=x.d(ecx+edx*4u+8u);            // its channel
            ebp=ecx+edx*4u;
            if(edi!=0xffffffffu) {
                edx=x.d(channel_array);
                eax=x.d(edx+0x44u);
                ecx=edi;
                ecx&=0xffffu;
                ecx*=0x1cu;
                ecx+=eax;
                lo8(eax,x.b(ecx+2u));
                std::uint32_t count=std::uint8_t((eax&0xffu)-1u);   // DEC DL
                count^=eax&0xffu;
                count&=0x7fu;
                count^=eax&0xffu;
                lo8(edx,count);
                x.st8(0x223e4eu,ecx+2u,count);
                if((count&0x7fu)==0u) {
                    edx=x.d(channel_array);
                    eax=edi;
                    x.call(0x223e5bu,0x16e8a0u);
                }
                x.st(0x223e60u,ebp+8u,0xffffffffu);
            }
            edx=x.d(voice_array);
            eax=esi;
            x.call(0x223e6fu,0x16e8a0u);
            edi=x.d(esp+0x14u);
        } else {
            ecx=esi;
            x.call(0x223e7fu,0x223fb0u);
            if((eax&0xffu)!=0u) {
                x.push(0x223e8cu,1u);
                x.push(0x223e8eu,esi);
                x.call(0x223e8fu,0x224040u);
            }
        }
    }
    edi=x.pop();
    esi=x.pop();
    ebp=x.pop();
    ebx=x.pop();
    esp+=0x10u;
    x.ret();
}

// =====================================================================
// 226370: applies a DSP parameter list. EAX: the list's byte count; stack:
// arg0 the list (records of dwords: +4 dirty mask, +8 flags, +0xC offset,
// +0xE byte length, +0x10 data). For each record not flagged +8 bit 1,
// every set mask bit either posts the data (flag bit 0: 385B4F with the
// bit, the data and 0) or, when the data differs from the bit's parameter
// record (([51DF90] records, 0x20 each: +0x10 data, +0x14 dword size) at the
// offset, posts it to the DSP (3856ED); then the mask is OR'd into the
// parameter dirty word ([51DF90] +0x22). ret 4.
// =====================================================================
void r_226370(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    esp-=0x18u;
    x.push(0x226373u,esi);
    esi=x.d(esp+0x20u);                        // arg0
    if(esi!=0u && eax!=0u) {
        eax+=0xfffffffcu;
        eax>>=2;
        x.push(0x22638au,ebp);
        ebp=0u;
        x.st(0x22638fu,esp+0x1cu,eax);         // the list's dword count
        if(std::int32_t(eax)>0) {
            x.push(0x226395u,ebx);
            x.push(0x226396u,edi);
            do {
                if((x.b(esi+ebp*4u+8u)&0x2u)==0u) {
                    edi=x.d(esi+ebp*4u+4u);
                    x.st(0x2263a2u,esp+0x14u,edi);
                    x.st(0x2263a6u,esp+0x18u,0xffffffffu);
                    for(;;) {
                        // The next set mask bit after [ESP+18h].
                        eax=x.d(esp+0x18u);
                        ++eax;
                        bool set=false;
                        if(eax<0x20u) {
                            for(;;) {
                                edx=1u;
                                ecx=eax;
                                edx<<=ecx&31u;
                                if(edi<edx)break;
                                if((edi&edx)!=0u){set=true;break;}
                                ++eax;
                                if(!(eax<0x20u))break;
                            }
                        }
                        if(!set)break;
                        const bool post=(x.b(esi+ebp*4u+8u)&0x1u)!=0u;
                        x.st(0x226409u,esp+0x18u,eax);
                        if(post) {
                            x.push(0x22640fu,0u);
                            ecx=esi+ebp*4u+0x10u;
                            x.push(0x226415u,ecx);
                            x.push(0x226416u,eax);
                            x.call(0x226417u,0x385b4fu);
                            continue;
                        }
                        edi=x.d(dsp_globals);
                        edi=x.d(edi);
                        edx=x.w16(esi+ebp*4u+0xeu);
                        ebx=x.w16(esi+ebp*4u+0xcu);
                        ecx=eax;
                        ecx<<=5;
                        ecx+=edi;
                        x.st(0x226437u,esp+0x10u,ecx);
                        ecx=x.d(ecx+0x14u);
                        edi=ebx+edx;
                        ecx<<=2;
                        if(edi<=ecx) {
                            edi=x.d(esp+0x10u);
                            edi=x.d(edi+0x10u);
                            edi+=ebx;
                            esi=esi+ebp*4u+0x10u;
                            ecx=edx;
                            ebx=0u;
                            // REPE CMPSB (ZF stays set from XOR EBX,EBX for no bytes).
                            bool same=true;
                            const auto step=x.down()?0xffffffffu:1u;
                            while(ecx) {
                                const auto p=x.b(esi),q=x.b(edi);
                                same=p==q;
                                esi+=step;edi+=step;--ecx;
                                if(!same)break;
                            }
                            esi=x.d(esp+0x2cu);           // arg0
                            if(!same) {
                                x.push(0x226469u,ebx);
                                x.push(0x22646au,edx);
                                edx=x.d(dsp_state);
                                ecx=esi+ebp*4u+0x10u;
                                x.push(0x226475u,ecx);
                                ecx=x.w16(esi+ebp*4u+0xcu);
                                x.push(0x22647bu,ecx);
                                x.push(0x22647cu,eax);
                                eax=x.d(edx+0x2ab0u);
                                x.push(0x226483u,eax);
                                x.call(0x226484u,0x3856edu);
                            }
                        }
                        edi=x.d(esp+0x14u);
                    }
                    eax=x.d(dsp_globals);
                    lo16(ecx,x.w16(esi+ebp*4u+4u));
                    x.st16(0x2263e1u,eax+0x22u,x.w16(eax+0x22u)|(ecx&0xffffu));
                    eax=x.d(esp+0x24u);
                }
                edx=x.w16(esi+ebp*4u+0xeu);
                edx+=0xfu;
                edx=sar(edx,2);
                ebp+=edx;
            } while(less(ebp,eax));
            edi=x.pop();
            ebx=x.pop();
        }
        ebp=x.pop();
    }
    esi=x.pop();
    esp+=0x18u;
    x.ret(4);
}

// =====================================================================
// 22C560: the two ambience channels ([52D684], 0x14 each: +0 active, +4
// voice handle, +8 time (float), +0xC tag datum, +0x10 index or gain). Stack:
// arg0 the elapsed time (float). Channel 0 plays a looping sound definition
// row (tag +0x70 rows, 0x4C each): its gain follows the row's fade curve
// (13DE70 over time / max(fade time, 0.001), interpolated between +4 and +8
// when the curve has no flags) and it finishes once the time passes the
// row's length; channel 1 uses its stored gain. 1903A0 resolves the
// playback (EAX), the gain is stored on the voice, and 18FE80 tells whether
// the channel's DSP parameters must be rebuilt (2292E0 fills a 0x404-byte
// block; a non-empty block replaces the frame's). The time advances by the
// elapsed time; a finished channel stops its voice (224040: handle, 2) and
// clears. Finally the frame's DSP block goes to 226370. ret 4.
// =====================================================================
void r_22c560(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    x.push(0x22c560u,ebp);
    ebp=esp;
    esp&=0xfffffff8u;
    esp-=0x844u;
    x.push(0x22c56cu,ebx);
    x.push(0x22c56du,esi);
    edx=0u;
    x.push(0x22c570u,edi);
    eax=0u;
    ecx=0xffu;
    edi=esp+0x44cu;
    x.st(0x22c57fu,esp+0x448u,edx);
    x.rep_stos(0x22c586u,4);                   // the frame's DSP block
    x.st(0x22c588u,esp+0x848u,edx);
    x.st(0x22c58fu,esp+0x1cu,eax);
    do {
        edx=x.d(ambience_channels);
        ecx=eax+eax*4u;
        ebx=edx+ecx*4u;
        if(x.b(ebx)!=0u) {
            x.st8(0x22c5abu,esp+0x13u,0u);     // finished
            x.st(0x22c5b0u,esp+0x18u,0xffffffffu);
            if(eax==0u) {
                esi=x.d(ebx+0x10u);
                eax=x.d(ebx+0xcu);
                esi*=0x4cu;
                ecx=x.d(tag_instances);
                x.x(0,single(thousandth));
                eax&=0xffffu;
                eax<<=4;
                edx=x.d(eax+ecx+8u);
                eax=x.d(edx+0x70u);
                x.x(1,x.ld(esi+eax+0x40u));
                esi+=eax;                      // the definition row
                if(comiss(x.x(1),x.x(0)).above())x.x(0,x.x(1));
                eax=x.d(esi+0x48u);
                x.x(1,x.ld(ebx+8u));
                edi=esi+0x44u;
                x.x(1,_mm_div_ss(x.x(1),x.x(0)));
                x.st(0x22c624u,esp+0x20u,lane0(x.x(1)));
                if(eax!=0u && std::int32_t(x.d(edi))>0) {
                    eax=x.d(esp+0x20u);
                    x.push(0x22c635u,0u);
                    x.push(0x22c637u,eax);
                    x.call(0x22c638u,0x13de70u);
                    edi=x.d(edi+4u);
                    if((x.b(edi+1u)&0xf0u)==0u) {
                        x.x(3,_mm_setzero_ps());
                        const auto negative=comiss(x.x(3),x.x(0));
                        x.x(2,x.ld(edi+4u));
                        x.x(1,x.ld(edi+8u));
                        bool clamp=negative.above();
                        if(!clamp) {
                            x.x(3,single(one));
                            clamp=comiss(x.x(0),x.x(3)).above();
                        }
                        if(clamp)x.x(0,x.x(3));
                        x.x(1,_mm_sub_ss(x.x(1),x.x(2)));
                        x.x(1,_mm_mul_ss(x.x(1),x.x(0)));
                        x.x(1,_mm_add_ss(x.x(1),x.x(2)));
                        x.x(0,x.x(1));
                    }
                } else {
                    x.x(0,_mm_setzero_ps());
                }
                eax=x.d(esi+0x3cu);
                x.st(0x22c67fu,esp+0x14u,lane0(x.x(0)));   // the gain
                x.call(0x22c685u,0x1903a0u);
                x.x(0,x.ld(ebx+8u));
                const auto past=comiss(x.x(0),x.ld(esi+0x40u));
                x.st(0x22c693u,esp+0x18u,eax);
                x.st8(0x22c697u,esp+0x13u,1u);
                if(!past.above())x.st8(0x22c69eu,esp+0x13u,0u);
            } else if(--eax==0u) {
                x.x(0,x.ld(ebx+0x10u));
                eax=x.d(ebx+0xcu);
                x.st(0x22c5c9u,esp+0x14u,lane0(x.x(0)));
                x.call(0x22c5cfu,0x1903a0u);
                x.st(0x22c5d4u,esp+0x18u,eax);
            }
            eax=x.d(ebx+4u);
            if(eax!=0xffffffffu) {
                edx=x.d(voice_array);
                x.x(0,x.ld(esp+0x14u));
                eax&=0xffffu;
                ecx=eax+eax*4u;
                eax=x.d(edx+0x44u);
                x.st(0x22c6c2u,eax+ecx*4u+0xcu,lane0(x.x(0)));
            }
            edx=x.d(esp+0x18u);
            ecx=0xffffffffu;
            x.call(0x22c6cfu,0x18fe80u);
            if(eax!=0u) {
                x.x(0,_mm_setzero_ps());
                edx=0u;
                x.st(0x22c6e1u,esp+0x40u,edx);
                eax=0u;
                ecx=0xffu;
                edi=esp+0x44u;
                x.rep_stos(0x22c6f0u,4);
                ecx=x.d(listener_position);
                x.st(0x22c6f8u,esp+0x440u,edx);
                edx=x.d(ecx);
                eax=x.d(ecx+4u);
                ecx=x.d(ecx+8u);
                x.st(0x22c707u,esp+0x24u,lane0(x.x(0)));
                x.x(0,x.ld(ebx+8u));
                x.st(0x22c712u,esp+0x34u,edx);
                x.st(0x22c716u,esp+0x28u,lane0(x.x(0)));
                x.x(0,x.ld(esp+0x14u));
                edx=esp+0x24u;
                x.st(0x22c726u,esp+0x2cu,lane0(x.x(0)));
                x.x(0,single(one));
                x.push(0x22c734u,edx);
                edx=x.d(esp+0x1cu);
                esi=esp+0x44u;
                x.st(0x22c73du,esp+0x34u,lane0(x.x(0)));
                x.st(0x22c743u,esp+0x3cu,eax);
                x.st(0x22c747u,esp+0x40u,ecx);
                x.call(0x22c74bu,0x2292e0u);
                eax=x.d(esp+0x440u);
                if(std::int32_t(eax)>0) {
                    ecx=0x101u;
                    edi=esp+0x448u;
                    x.rep_movsd(0x22c767u);
                }
            }
            x.x(0,x.ld(ebx+8u));
            lo8(eax,x.b(esp+0x13u));
            x.x(0,_mm_add_ss(x.x(0),x.ld(ebp+8u)));   // + arg0
            x.st(0x22c779u,ebx+8u,lane0(x.x(0)));
            if((eax&0xffu)!=0u) {
                ecx=x.d(ebx+4u);
                if(ecx!=0xffffffffu) {
                    edx=x.d(voice_array);
                    edx=x.d(edx+0x44u);
                    eax=ecx;
                    eax&=0xffffu;
                    eax=eax+eax*4u;
                    x.or8(0x22c79bu,edx+eax*4u+4u,0x18u);
                    x.push(0x22c7a0u,2u);
                    esi=edx+eax*4u;
                    x.push(0x22c7a5u,ecx);
                    x.call(0x22c7a6u,0x224040u);
                    x.or8(0x22c7abu,esi+4u,0x1u);
                }
                x.st8(0x22c7afu,ebx,0u);
            }
            eax=x.d(esp+0x1cu);
        }
        ++eax;
        x.st(0x22c7bau,esp+0x1cu,eax);
    } while(less(eax,2u));
    eax=esp+0x448u;
    x.push(0x22c7cbu,eax);
    eax=x.d(esp+0x84cu);                       // the block's byte count
    x.call(0x22c7d3u,0x226370u);
    edi=x.pop();
    esi=x.pop();
    ebx=x.pop();
    esp=ebp;
    ebp=x.pop();
    x.ret(4);
}

// =====================================================================
// 18EE50: the per-frame object-audio update. Stack: arg0 the elapsed time
// (float). With all three ducking bytes of the sound state set, 129F10
// (stack 0) runs first. Then 18F900, 18E970 (time), 22C560 (time), 18ED80,
// and 129DD0 with the ducking type: 0 while the override ([51DF50] +5) is
// on, else 1 until the players' ducking end tick passes, then -1. Then each
// used element of the looping-sound pool ([4F9B5C]) by mode (+3):
//  - mode 4 sounds owned by an object index not past the current tick are
//    marked updated (+4 bit 1); every mode other than 1 updates (18D5A0);
//  - mode 1 (object-attached, +0x10 object): a detached loop (+4 bit 0)
//    whose object is gone (BCDF0 -1 = 0) clears its owner's reference
//    (18FFB0 of the owner +0xC; bit 5) and is deleted (16E8A0); a loop on a
//    live object (BBD50) whose cluster (BC330) is audible updates
//    (18D5A0), otherwise drops its +5 bit 0.
// Finally the frame flag (+0x7E) is set and 223DA0 retires voices. ret 4.
// =====================================================================
void r_18ee50(LoopContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    eax=x.d(sound_state);
    lo8(ecx,x.b(eax+0x78u));
    esp-=0x18u;
    if((ecx&0xffu)!=0u) {
        lo8(ecx,x.b(eax+0x79u));
        if((ecx&0xffu)!=0u) {
            lo8(ecx,x.b(eax+0x7au));
            if((ecx&0xffu)!=0u) {
                x.push(0x18ee6du,0u);
                x.call(0x18ee6fu,0x129f10u);
            }
        }
    }
    x.push(0x18ee74u,ebx);
    x.push(0x18ee75u,ebp);
    x.push(0x18ee76u,esi);
    x.push(0x18ee77u,edi);
    x.call(0x18ee78u,0x18f900u);
    esi=x.d(esp+0x2cu);                        // arg0
    x.push(0x18ee81u,esi);
    x.call(0x18ee82u,0x18e970u);
    x.push(0x18ee87u,esi);
    x.call(0x18ee88u,0x22c560u);
    x.call(0x18ee8du,0x18ed80u);
    eax=x.d(ducking_override);
    ecx=0xffffffffu;
    bool overridden=false;
    if(eax!=0u) {
        lo8(edx,x.b(eax+5u));
        overridden=(edx&0xffu)!=0u;
    }
    if(overridden)ecx=0u;
    else {
        eax=x.d(player_loops);
        edx=x.d(eax+0x24u);
        eax=x.d(game_time);
        if(!less(edx,x.d(eax+8u)))ecx=1u;
    }
    x.call(0x18eec0u,0x129dd0u);
    ebx=x.d(loop_pool);
    ebp=ebx;
    x.st(0x18eecdu,esp+0x1cu,ebp);
    x.st(0x18eed1u,esp+0x24u,0xffffffffu);     // the pool iterator
    goto L18eee0;
L18eedb:
    ebp=x.d(esp+0x1cu);
L18eee0:
    // The next used pool element after the iterator.
    eax=x.d(esp+0x24u);
    ++eax;
    if(std::int32_t(eax)<0)goto L18ef0c;
    esi=x.d(ebp+0x38u);
    if(!less(eax,esi))goto L18ef0c;
    edx=x.d(ebp+0x48u);
    do {
        ecx=eax;
        ecx&=0x1fu;
        edi=1u;
        edi<<=ecx;
        ecx=eax;
        ecx=sar(ecx,5);
        if((x.d(edx+ecx*4u)&edi)!=0u)goto L18ef24;
        ++eax;
    } while(less(eax,esi));
L18ef0c:
    eax=x.d(sound_state);
    x.st8(0x18ef11u,eax+0x7eu,1u);
    x.call(0x18ef15u,0x223da0u);
    edi=x.pop();
    esi=x.pop();
    ebp=x.pop();
    ebx=x.pop();
    esp+=0x18u;
    x.ret(4);
    return;
L18ef24:
    if(eax==0xffffffffu)goto L18ef0c;
    esi=x.d(ebp+0x24u);
    edi=x.d(ebp+0x44u);
    esi*=eax;
    esi+=edi;                                  // the element
    edi=sx16(x.w16(esi));
    edi<<=16;
    edi|=eax;                                  // its datum handle
    x.st(0x18ef3cu,esp+0x24u,eax);
    if(x.b(esi+3u)==4u) {
        eax=x.d(esi+0x10u);
        if(eax!=0xffffffffu) {
            edx=x.d(game_time);
            if(!(std::int32_t(eax)>std::int32_t(x.d(edx+8u)))) {
                ecx=x.d(ebx+0x44u);
                eax=edi;
                eax&=0xffffu;
                eax=eax+eax*2u;
                x.or8(0x18ef66u,ecx+eax*8u+4u,0x2u);
                eax=ecx+eax*8u+4u;
            }
        }
    }
    if(x.b(esi+3u)!=1u) {
        x.push(0x18ef75u,edi);
        x.call(0x18ef76u,0x18d5a0u);
        ebx=x.d(loop_pool);
        goto L18eee0;
    }
    if((x.b(esi+4u)&0x1u)!=0u) {
        eax=x.d(esi+0x10u);
        x.push(0x18ef8fu,0xffffffffu);
        x.call(0x18ef91u,0xbcdf0u);
        if(eax==0u) {
            edx=x.d(esi+0xcu);
            x.push(0x18ef9du,edx);
            x.call(0x18ef9eu,0x18ffb0u);
            if(eax==edi) {
                esi=x.d(esi+0xcu);
                x.push(0x18efaau,esi);
                x.call(0x18efabu,0x18ffb0u);
                if(eax!=0xffffffffu) {
                    ecx=x.d(ebx+0x44u);
                    eax&=0xffffu;
                    eax=eax+eax*2u;
                    eax=ecx+eax*8u;
                    if(x.d(eax+0xcu)==esi)x.and8(0x18efc8u,eax+4u,0xdfu);
                }
            }
            eax=edi;
            edx=ebx;
            x.call(0x18efd0u,0x16e8a0u);
            goto L18eee0;
        }
    }
    ebp=x.d(esi+0x10u);
    eax=ebp;
    x.call(0x18efdfu,0xbbd50u);
    if((eax&0xffu)==0u)goto L18eedb;
    eax=esp+0x10u;
    ecx=ebp;
    x.call(0x18eff2u,0xbc330u);
    eax=x.d(esp+0x14u);
    if((eax&0xffffu)!=0xffffu) {
        eax=sx16(eax);
        ecx=eax;
        ecx&=0x1fu;
        edx=1u;
        edx<<=ecx;
        eax=sar(eax,5);
        if((x.d(audible_clusters+eax*4u)&edx)!=0u) {
            x.push(0x18f01cu,edi);
            x.call(0x18f01du,0x18d5a0u);
            ebx=x.d(loop_pool);
            goto L18eedb;
        }
    }
    x.and8(0x18f02du,esi+5u,0xfeu);
    goto L18eedb;
}

} // namespace

std::span<const LoopRoutineEntry> object_loop_frame_routines() {
    static constexpr LoopRoutineEntry table[]{
        {0x18ee50u,r_18ee50},
        {0x223da0u,r_223da0},
        {0x22c560u,r_22c560},
        {0x226370u,r_226370},
        {0x18ed80u,r_18ed80},
        {0x18f510u,r_18f510},
        {0x18f900u,r_18f900},
        {0x129dd0u,r_129dd0},
        {0x18d390u,r_18d390},
        {0x18d3d0u,r_18d3d0},
    };
    return table;
}
} // namespace halo2::native_tu15_sound
