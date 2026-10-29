// Native campaign audio routines: the playback part (see
// native_tu15_sound_campaign_audio.hpp). 128650 is the retail sound
// manager's per-frame driver in campaign worlds. Once the three ducking
// bytes of the sound state ([4F2C50] +0x78..0x7A) allow it, it services the
// platform voices (225610), refreshes the channel list when the pause state
// changes (12CBC0, 2207A0, 225FB0), rebuilds the DSP routing masks (225D70),
// advances the ducking clocks (129500) and the sound-class fades (228310),
// runs the listener producer (129F10) and the channel heartbeat (2206D0),
// mixes (221AF0), steps the playing sounds (12BBE0), retires and starts
// playbacks (12AB30, 12ADE0), gathers the audible sounds of every local
// listener (12BC10, 228480 per listener), applies their spatial data
// (12BE90) and their gains and channels (12BF00, which calls the playback
// state 12C5B0 with its distance gain 12D840), commits the DSP state
// (225890), retires the channels (2207A0) and finally clears the cluster
// sound bounds ([50EBE4]).
//
// Each routine is a faithful decompilation of the retail TU1.5 function at
// register level: the same memory reads and stores (each store at its
// original instruction address), the same register values at every call and
// at the return, the same SSE scalar arithmetic and comparisons, and the
// same x87 operations under the caller's control word. Branches are taken in
// C++ from the operands the original compares, not through emulated EFLAGS.
// Every CALL pushes its original return address (published with the CALL's
// address) and resolves through campaign_audio_call: a native campaign
// routine, the lifted original of one not yet converted, or the live world
// for the core game helpers and the native sound kernels (220000/220200
// gain, 228220 class row, 12A810 listener). Indirect calls (the sound
// driver's callbacks and COM methods) report their own instruction address
// to the live world. Retail .rdata constants are literals read from the
// TU1.5 XBE; game globals are read from memory.
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include "halo2/source_campaign_audio_tu15.hpp"
#include <algorithm>

namespace halo2::native_tu15_sound {
namespace {

// Retail .rdata constants (TU1.5 XBE).
constexpr std::uint32_t thousandth=0x3a83126fu;    // 0x46A180: 0.001
constexpr std::uint32_t one=0x3f800000u;           // 0x46A0D0: 1.0
constexpr std::uint32_t zero=0x00000000u;          // 0x46A0E8: 0.0
constexpr std::uint32_t epsilon=0x38d1b717u;       // 0x46A0EC: 0.0001
constexpr std::uint32_t tiny_square=0x358637beu;   // 0x46A454: 1e-6 (squared distance floor)
constexpr std::uint32_t silence=0xc2800000u;       // 0x455B0C: -64.0 dB
constexpr std::uint32_t thousand=0x447a0000u;      // 0x46A1DC: 1000.0
constexpr std::uint32_t two_pi=0x40c90fdbu;        // 0x44923C: 2 pi
constexpr std::uint32_t minus_two_pi=0xc0c90fdbu;  // 0x46A580: -2 pi
constexpr std::uint32_t quarter_pi=0x3f490fdbu;    // 0x46A550: pi/4
constexpr std::uint32_t minus_quarter_pi=0xbf490fdbu;     // 0x46A518: -pi/4
constexpr std::uint32_t three_quarter_pi=0x4016cbe4u;     // 0x46A54C: 3pi/4
constexpr std::uint32_t seven_quarter_pi=0x40afede0u;     // 0x46A854: 7pi/4
constexpr std::uint32_t minus_five_quarter_pi=0xc07b53d2u;// 0x46A850: -5pi/4
constexpr std::uint32_t minus_three_quarter_pi=0xc016cbe4u;// 0x46A84C: -3pi/4
constexpr std::uint32_t minus_seven_quarter_pi=0xc0afede0u;// 0x46A848: -7pi/4
constexpr std::uint32_t five_quarter_pi=0x407b53d2u;      // 0x46A844: 5pi/4
constexpr std::uint32_t inverse_pi=0x3ea2f983u;    // 0x46A4B8: 1/pi
constexpr std::uint32_t two_thousand=0x44fa0000u;  // 0x46A2F8: 2000.0
constexpr std::uint32_t full_gain=0x00000000u;     // 0x44AD40: 0.0 dB
constexpr std::uint32_t floor_gain=0xc2800000u;    // 0x44AD44: -64.0 dB
constexpr std::uint32_t effect_slots[2]{9u,8u};    // 0x455CA8: the two DSP effect slots 225890 posts
// The jump tables 225610 and 225890 dispatch through (every entry the
// fall-through target).
constexpr std::uint32_t channel_table=0x2256dcu,channel_target=0x2256c4u;
constexpr std::uint32_t stream_table=0x225c80u,stream_target=0x225c3du;
constexpr std::uint32_t voice_table=0x225c90u,voice_target=0x225c5cu;

// Retail globals (runtime data, read from memory).
constexpr std::uint32_t sound_state=0x4f2c50u;      // +0x78..0x7A ducking bytes, +0x7B pause, +0x7D/+0x7E frame bytes,
                                                    // +0x80 last tick, +0x84 clock, +0x88 listeners (0x48 apart),
                                                    // +0x1E0.. frame time and fades, +0x208 playback count
constexpr std::uint32_t playbacks=0x4f2c48u;        // 0x24-byte playback slots
constexpr std::uint32_t sound_pool=0x4f2c4cu;       // playing-sound data array (0xBC-byte elements)
constexpr std::uint32_t local_players=0x4f54f0u;    // +0xC.. four local player indices
constexpr std::uint32_t game_time=0x51df54u;        // +0 active, +1 paused
constexpr std::uint32_t cluster_bounds=0x50ebe4u;   // +0x38 serial, +0x44..0x60 bounds
constexpr std::uint32_t class_fades=0x50ebf0u;      // 16-byte sound-class rows: +0 target, +4 gain, +8 time
constexpr std::uint32_t platform=0x52d674u;         // the platform sound state
constexpr std::uint32_t channel_array=0x52d668u;    // channel data array (0x1C-byte elements)
constexpr std::uint32_t channel_none=0x52d66cu;     // the channel a missing handle reads
constexpr std::uint32_t listener_vector=0x474ce8u;
constexpr std::uint32_t listener_forward=0x474d04u;
constexpr std::uint32_t sound_structure=0x4ecc18u;  // +0x9C cluster count, +0x224/+0x228 cluster bits
constexpr std::uint32_t pause_session=0x4f3218u;    // +8 mode, +0x1120/+0x1121 bytes
constexpr std::uint32_t activity_flag=0x4f9c6du;
constexpr std::uint32_t activity_bytes=0x556dfeu;   // seven bytes the ducking checks
constexpr std::uint32_t activity_counters[2]{0x4f2d40u,0x4f2d70u};
constexpr std::uint32_t dsp_dirty=0x47c778u;        // +4 the DSP block 225890 resends
constexpr std::uint32_t driver_serial=0x390bb8u;

// --- memory, registers, SSE and x87 ------------------------------------------
inline std::uint32_t rd32(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,4);}
inline std::uint32_t rd16(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,2);}
inline std::uint32_t rd8(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,1);}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline std::uint32_t sar(std::uint32_t v,unsigned n){return std::uint32_t(std::int32_t(v)>>n);}
inline bool less(std::uint32_t x,std::uint32_t y){return std::int32_t(x)<std::int32_t(y);}          // CMP; JL
inline bool less16(std::uint32_t x,std::uint32_t y){return std::int16_t(x)<std::int16_t(y);}        // CMP r16; JL
inline bool less8(std::uint32_t x,std::uint32_t y){return std::int8_t(x)<std::int8_t(y);}           // CMP r8; JL
inline std::uint32_t imul(std::uint32_t x,std::uint32_t y){return std::uint32_t(std::int64_t(std::int32_t(x))*std::int32_t(y));}
// MOV AL / MOV AX: only the low byte or word of the register changes.
inline void lo8(std::uint32_t &r,std::uint32_t v){r=(r&0xffffff00u)|(v&0xffu);}
inline void lo16(std::uint32_t &r,std::uint32_t v){r=(r&0xffff0000u)|(v&0xffffu);}
inline void hi8(std::uint32_t &r,std::uint32_t v){r=(r&0xffff00ffu)|((v&0xffu)<<8);}
inline __m128 single(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}   // MOVSS from memory
inline std::uint32_t lane0(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
// COMISS/UCOMISS x,y: CF (below or unordered), ZF (equal or unordered) and
// PF (unordered).
struct Compare {
    bool cf,zf,pf;
    bool above()const{return !cf && !zf;}    // JA
    bool below_equal()const{return cf || zf;}// JBE
};
inline Compare comiss(__m128 x,__m128 y) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char cf,zf,pf;
    asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(cf),"=qm"(zf),"=qm"(pf):"x"(x),"x"(y):"cc");
    return {cf!=0,zf!=0,pf!=0};
#else
    throw FormatError("Campaign audio scalar comparison backend unavailable");
#endif
}
inline Compare ucomiss(__m128 x,__m128 y) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char cf,zf,pf;
    asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(cf),"=qm"(zf),"=qm"(pf):"x"(x),"x"(y):"cc");
    return {cf!=0,zf!=0,pf!=0};
#else
    throw FormatError("Campaign audio scalar comparison backend unavailable");
#endif
}
// FLD m32 as its own load, so a denormal or signalling single signals there.
long double load_single(std::uint32_t bits){volatile long double v=static_cast<long double>(std::bit_cast<float>(bits));return v;}
long double fadd(long double x,long double y){volatile long double z=x+y;return z;}
long double fsub(long double x,long double y){volatile long double z=x-y;return z;}
long double fmul(long double x,long double y){volatile long double z=x*y;return z;}
long double load_log10_2() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double v;asm volatile("fldlg2":"=t"(v));return v;
#else
    throw FormatError("Campaign audio x87 backend unavailable");
#endif
}
long double y_log2_x(long double y,long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double v;asm volatile("fyl2x\n\tfnstsw %1":"=t"(v),"=m"(status):"0"(x),"u"(y):"st(1)");return v;
#else
    throw FormatError("Campaign audio x87 backend unavailable");
#endif
}
// FCOMIP ST(0),ST(1): CF and ZF of the extended comparison (any NaN signals).
Compare extended_compare(long double x,long double y) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char cf,zf,pf;
    asm volatile("fldt %4\n\tfldt %3\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tsete %1\n\tsetp %2\n\tfstp %%st(0)"
        :"=qm"(cf),"=qm"(zf),"=qm"(pf):"m"(x),"m"(y):"cc","st");
    return {cf!=0,zf!=0,pf!=0};
#else
    throw FormatError("Campaign audio x87 backend unavailable");
#endif
}
[[noreturn]] void x87_fault(){throw FormatError("Required original solver x87 stack-fault owner unavailable");}
// An indirect jump through a retail table entry other than the one every
// TU1.5 entry holds is refused as the reference routine refuses it.
void require_branch(std::uint32_t target,std::uint32_t expected) {
    if(target==expected)return;
    if(target==channel_target||target==stream_target||target==voice_target)
        throw FormatError("Required original solver dispatch target unavailable");
    throw FormatError("Unbound TU1.5 contact dispatch target");
}

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
    CampaignContext &c;Abi &a;M &m;const O &o;
    std::uint32_t &eax,&ecx,&edx,&ebx,&esp,&ebp,&esi,&edi;
    Floating floating;
    Cpu(CampaignContext &context,Abi &abi):c(context),a(abi),m(context.m),o(context.o),
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
    // OR/AND/INC m8: read, combine, store.
    void or8(std::uint32_t site,std::uint32_t p,std::uint32_t v){st8(site,p,b(p)|v);}
    void and8(std::uint32_t site,std::uint32_t p,std::uint32_t v){st8(site,p,b(p)&v);}
    void push(std::uint32_t site,std::uint32_t v){esp-=4u;st(site,esp,v);}
    std::uint32_t pop(){const auto v=d(esp);esp+=4u;return v;}
    // RET n: pops the return address and n bytes of stack arguments.
    void ret(std::uint32_t arguments=0){pop();esp+=arguments;}
    __m128 x(unsigned n)const{return xmm(a,n);}
    void x(unsigned n,__m128 v){set_xmm(a,n,v);}
    void movss(unsigned n,__m128 v){x(n,_mm_move_ss(x(n),v));}     // MOVSS xmm,xmm keeps the upper lanes
    void zero(unsigned n){x(n,_mm_setzero_ps());}                    // XORPS xmm,xmm
    void store_lane(std::uint32_t site,std::uint32_t p,unsigned n){st(site,p,a.xmm[n][0]);}   // MOVSS m32,xmm
    // CALL rel32 at `site`: pushes the return address and runs the callee
    // with the live floating-point environment. Stack arguments are pushed
    // beforehand; a callee that pops them (RET n) does so itself.
    void call(std::uint32_t site,std::uint32_t target) {
        fp_out(a);
        push(site,site+5u);
        campaign_audio_call(c,a,target);
        fp_in(a);
    }
    // CALL r32 / CALL [r32+disp] at `site` (`length` bytes long): a driver
    // callback or COM method. A target outside the group reaches the live
    // world with the call's own instruction address.
    void call_indirect(std::uint32_t site,unsigned length,std::uint32_t target) {
        fp_out(a);
        push(site,site+length);
        if(source_campaign_audio_tu15_owns_entry(target))campaign_audio_call(c,a,target);
        else {
            if(!c.services.required_owner)throw FormatError("Campaign audio callee has no live owner");
            c.services.required_owner(m,{site,target,false},a,o);
        }
        fp_in(a);
    }
    bool down()const{return (a.eflags&0x400u)!=0;}   // DF
    // REP STOS: ECX elements of EAX (`width` bytes each) at EDI.
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
    // x87, as the reference routines model it: a load needs a free register
    // and clears C1; ST(i) must hold a value.
    long double &st0(unsigned i=0){if(i>=a.x87_depth)x87_fault();return a.x87[i];}
    void fld(long double v){if(a.x87_depth>=8u)x87_fault();floating.push(v);}
    void fld32(std::uint32_t p){fld(load_single(d(p)));}
    void fpop(){st0();floating.pop();}
    // FSTP m32: the rounded single (C1 its rounding direction), then the pop.
    void fstp32(std::uint32_t site,std::uint32_t p) {
        const auto v=std::bit_cast<std::uint32_t>(floating.store_single(st0()));
        st(site,p,v);floating.pop();
    }
    // FSTP ST(0).
    void fstp0(){st0();floating.clear_c1();floating.pop();}
    void fabs0(){auto &v=st0();v=std::fabs(v);floating.clear_c1();}
    // FLD m32; FABS; FLD m32 (the bound); FCOMIP ST,ST(1); FSTP ST(0): the
    // bound against |x| (CF, ZF).
    Compare bound_above_abs(std::uint32_t value,std::uint32_t bound_bits) {
        fld(load_single(value));
        fabs0();
        fld(load_single(bound_bits));
        const auto other=st0(1);
        const auto result=extended_compare(st0(),other);
        floating.pop();
        fstp0();
        return result;
    }
};

// =====================================================================
// 12D6F0: the position of the playing sound ECX (element +0x18 location
// block) into [EAX]: kind (+2 byte) 0 copies the explicit point (+0xC), 1
// the point at +0xC/+0x10 with the listener ESI's cluster (listener +0xC0).
// Other kinds leave [EAX]. ret.
// =====================================================================
void r_12d6f0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&esi=x.esi;
    edx=sx8(x.b(ecx+2u));
    if(edx==0u) {
        ecx+=0xcu;
        edx=x.d(ecx);
        x.st(0x12d720u,eax,edx);
        edx=x.d(ecx+4u);
        x.st(0x12d725u,eax+4u,edx);
        ecx=x.d(ecx+8u);
        x.st(0x12d72bu,eax+8u,ecx);
        x.ret();
        return;
    }
    --edx;
    if(edx!=0u){x.ret();return;}
    edx=x.d(ecx+0xcu);
    x.st(0x12d6ffu,eax,edx);
    ecx=x.d(ecx+0x10u);
    x.st(0x12d704u,eax+4u,ecx);
    ecx=x.d(sound_state);
    edx=esi+esi*8u;
    edx=x.d(ecx+edx*8u+0xc0u);
    x.st(0x12d717u,eax+8u,edx);
    x.ret();
}

// =====================================================================
// 127AD0: the playing sound's (ECX: its +0x18 block, EAX: its tag datum)
// minimum distance in XMM0: the override (+0x34) when block bit 2 is set,
// else the class's (228220, definition +4 bit 2: class +0x18) or the
// playback row's (+0). ret.
// =====================================================================
void r_127ad0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx;
    lo8(edx,x.b(ecx));
    lo8(edx,(edx&0xffu)>>2);
    if((edx&1u)!=0u) {
        x.x(0,x.ld(ecx+0x34u));
        x.ret();
        return;
    }
    ecx=x.d(tag_instances);
    eax&=0xffffu;
    eax<<=4;
    eax=x.d(eax+ecx+8u);
    if((x.b(eax+1u)&4u)!=0u) {
        lo16(edx,sx8(x.b(eax+2u)));
        x.push(0x127afdu,edx);
        x.call(0x127afeu,0x228220u);
        x.x(0,x.ld(eax+0x18u));
        x.ret();
        return;
    }
    eax=sx16(x.w16(eax+6u));
    ecx=x.d(sound_gestalt);
    eax=imul(eax,0x38u);
    edx=x.d(ecx+4u);
    x.x(0,x.ld(eax+edx));
    x.ret();
}

// =====================================================================
// 127B70: whether the playing sound (EAX: its +0x18 block, ECX: tag datum)
// is spatialized: AL 0 only when block bits 4 and 5 are clear and the
// definition has flag +1 bit 4. EAX 0 or 1. ret.
// =====================================================================
void r_127b70(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx;
    lo16(eax,x.w16(eax));
    lo8(edx,eax);
    lo8(edx,(edx&0xffu)>>4);
    if((edx&1u)==0u) {
        lo8(eax,(eax&0xffu)>>5);
        if((eax&1u)==0u) {
            eax=x.d(tag_instances);
            ecx&=0xffffu;
            ecx<<=4;
            ecx=x.d(ecx+eax+8u);
            if((x.b(ecx+1u)&0x10u)!=0u){eax=0u;x.ret();return;}
        }
    }
    eax=1u;
    x.ret();
}

// =====================================================================
// 12D730: the distance gain of the playing sound (EDI: its +0x18 block,
// EAX: tag datum) at the distance arg0: the minimum distance (127AD0, at
// least 0.001; the override +0x34, the class +0x18 or the playback row +0)
// over the clamped fraction (12DB00, CL 0) of the distance between it and
// the maximum distance (override +0x38, class +0x1C or row +4). XMM0. ret 4.
// =====================================================================
void r_12d730(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    esp-=8u;
    x.push(0x12d733u,ebx);
    ebx=x.d(tag_instances);
    x.push(0x12d73au,ebp);
    ebp=x.d(sound_gestalt);
    x.push(0x12d741u,esi);
    esi=eax;
    lo8(eax,x.b(edi));
    lo8(eax,(eax&0xffu)>>2);
    if((eax&1u)!=0u)x.x(0,x.ld(edi+0x34u));
    else {
        ecx=esi;
        ecx&=0xffffu;
        ecx<<=4;
        eax=x.d(ecx+ebx+8u);
        if((x.b(eax+1u)&4u)!=0u) {
            lo16(edx,sx8(x.b(eax+2u)));
            x.push(0x12d76eu,edx);
            x.call(0x12d76fu,0x228220u);
            x.x(0,x.ld(eax+0x18u));
        } else {
            eax=sx16(x.w16(eax+6u));
            ecx=x.d(ebp+4u);
            eax=imul(eax,0x38u);
            x.x(0,x.ld(eax+ecx));
        }
    }
    x.x(1,single(thousandth));
    if(comiss(x.x(0),x.x(1)).above()) {
        eax=esi;
        ecx=edi;
        x.call(0x12d79bu,0x127ad0u);
        x.store_lane(0x12d7a0u,esp+0x10u,0);
    } else x.store_lane(0x12d7a8u,esp+0x10u,1);
    lo8(edx,x.b(edi));
    lo8(edx,(edx&0xffu)>>3);
    if((edx&1u)!=0u)x.x(0,x.ld(edi+0x38u));
    else {
        esi&=0xffffu;
        esi<<=4;
        esi=x.d(esi+ebx+8u);
        if((x.b(esi+1u)&8u)!=0u) {
            lo16(eax,sx8(x.b(esi+2u)));
            x.push(0x12d7d7u,eax);
            x.call(0x12d7d8u,0x228220u);
            x.x(0,x.ld(eax+0x1cu));
        } else {
            ecx=sx16(x.w16(esi+6u));
            edx=x.d(ebp+4u);
            ecx=imul(ecx,0x38u);
            x.x(0,x.ld(ecx+edx+4u));
        }
    }
    eax=x.d(esp+0x18u);
    ecx=x.d(esp+0x10u);
    x.push(0x12d7fcu,eax);
    x.store_lane(0x12d7fdu,esp+0x10u,0);
    edx=x.d(esp+0x10u);
    x.push(0x12d807u,ecx);
    x.push(0x12d808u,edx);
    lo8(ecx,0u);
    x.call(0x12d80bu,0x12db00u);
    x.x(1,x.ld(esp+0x18u));
    esi=x.pop();
    x.x(2,x.x(0));
    x.x(0,x.ld(esp+0xcu));
    const auto farther=comiss(x.x(0),x.x(1));
    ebp=x.pop();
    ebx=x.pop();
    if(farther.above())x.x(1,x.x(0));
    x.x(0,_mm_div_ss(x.x(0),x.x(1)));
    x.x(0,_mm_mul_ss(x.x(0),x.x(2)));
    esp+=8u;
    x.ret(4);
}

// =====================================================================
// 12D840: the gain of the playing sound (ECX: its +0x18 block, EAX: the
// listener-relative offset; stack: arg0 the tag datum, arg1 the listener
// forward vector). The distance gain (12D730) of |offset|; unless the block
// is unspatialized (bits 4, 5 clear and definition +1 bit 4) or the
// distance gain is within 0.0001 of zero, and the offset is not at the
// listener (|offset|^2 >= 1e-6) and points ahead of it (30BD0 of the
// negated offset), the cone gain (127CB0 angle, 11F910, 127C30/127BB0
// bounds, 12DB00 with CL 1) interpolates 1 to the cone's outer gain
// (220000). The product with the distance gain, inverted (1 - gain) when
// block bit 9 is set. XMM0. ret 8.
// =====================================================================
void r_12d840(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    esp-=0x10u;
    x.push(0x12d843u,ebx);
    x.push(0x12d844u,ebp);
    ebp=x.d(esp+0x1cu);
    x.push(0x12d849u,esi);
    esi=eax;
    x.x(2,x.ld(esi));
    x.x(1,x.ld(esi+4u));
    x.x(0,x.ld(esi+8u));
    x.x(3,x.x(2));
    x.x(3,_mm_mul_ss(x.x(3),x.x(2)));
    x.x(2,x.x(1));
    x.x(2,_mm_mul_ss(x.x(2),x.x(1)));
    x.x(1,x.x(0));
    x.x(3,_mm_add_ss(x.x(3),x.x(2)));
    x.x(1,_mm_mul_ss(x.x(1),x.x(0)));
    x.x(3,_mm_add_ss(x.x(3),x.x(1)));
    x.store_lane(0x12d877u,esp+0x20u,3);
    x.fld32(esp+0x20u);
    x.push(0x12d881u,edi);
    {auto &v=x.st0();v=root(v);}
    x.push(0x12d884u,ecx);
    edi=ecx;
    eax=ebp;
    x.fstp32(0x12d889u,esp);
    x.call(0x12d88cu,0x12d730u);
    lo16(ebx,x.w16(edi));
    x.x(1,single(one));
    lo8(eax,ebx);
    lo8(eax,(eax&0xffu)>>4);
    x.store_lane(0x12d8a3u,esp+0x10u,0);
    x.x(0,x.x(1));
    x.store_lane(0x12d8acu,esp+0x24u,0);
    bool done=false;
    if((eax&1u)==0u) {
        lo8(ecx,ebx);
        lo8(ecx,(ecx&0xffu)>>5);
        if((ecx&1u)==0u) {
            eax=x.d(tag_instances);
            edx=ebp;
            edx&=0xffffu;
            edx<<=4;
            ecx=x.d(edx+eax+8u);
            done=(x.b(ecx+1u)&0x10u)!=0u;
        }
    }
    if(!done)done=x.bound_above_abs(x.d(esp+0x10u),epsilon).above();
    if(!done) {
        x.x(3,x.ld(esi+4u));
        x.x(2,x.ld(esi+8u));
        x.x(4,x.ld(esi));
        x.x(5,x.x(3));
        x.x(5,_mm_mul_ss(x.x(5),x.x(3)));
        x.x(3,x.x(2));
        x.x(3,_mm_mul_ss(x.x(3),x.x(2)));
        x.x(2,x.x(4));
        x.x(5,_mm_add_ss(x.x(5),x.x(3)));
        x.x(2,_mm_mul_ss(x.x(2),x.x(4)));
        x.x(5,_mm_add_ss(x.x(5),x.x(2)));
        done=comiss(x.x(5),single(tiny_square)).cf;
    }
    if(!done) {
        edx=x.d(esi);
        x.zero(0);
        eax=x.d(esi+4u);
        ecx=x.d(esi+8u);
        x.st(0x12d935u,esp+0x14u,edx);
        x.x(1,x.x(0));
        x.x(1,_mm_sub_ss(x.x(1),x.ld(esp+0x14u)));
        x.st(0x12d942u,esp+0x18u,eax);
        x.store_lane(0x12d946u,esp+0x14u,1);
        x.st(0x12d94cu,esp+0x1cu,ecx);
        x.x(1,x.x(0));
        x.x(1,_mm_sub_ss(x.x(1),x.ld(esp+0x18u)));
        x.x(0,_mm_sub_ss(x.x(0),x.ld(esp+0x1cu)));
        eax=esp+0x14u;
        x.store_lane(0x12d963u,esp+0x18u,1);
        x.store_lane(0x12d969u,esp+0x1cu,0);
        x.call(0x12d96fu,0x30bd0u);
        if(comiss(x.x(0),single(zero)).above()) {
            eax=ebp;
            ecx=edi;
            x.call(0x12d981u,0x127cb0u);
            ecx=x.d(esp+0x28u);
            esi=eax;
            eax=esp+0x14u;
            x.call(0x12d990u,0x11f910u);
            x.push(0x12d995u,ecx);
            eax=ebp;
            ecx=edi;
            x.store_lane(0x12d99au,esp,0);
            x.call(0x12d99fu,0x127c30u);
            x.push(0x12d9a4u,ecx);
            eax=ebp;
            ecx=edi;
            x.store_lane(0x12d9a9u,esp,0);
            x.call(0x12d9aeu,0x127bb0u);
            x.push(0x12d9b3u,ecx);
            lo8(ecx,1u);
            x.store_lane(0x12d9b6u,esp,0);
            x.call(0x12d9bbu,0x12db00u);
            x.store_lane(0x12d9c0u,esp+0x24u,0);
            x.fld32(esp+0x24u);
            x.push(0x12d9cau,esi);
            x.fstp32(0x12d9cbu,esp+0x28u);
            x.call(0x12d9cfu,0x220000u);
            {auto &v=x.st0();v=fsub(v,load_single(one));}
            {auto &v=x.st0();v=fmul(v,load_single(x.d(esp+0x24u)));}
            {auto &v=x.st0();v=fadd(v,load_single(one));}
            x.fstp32(0x12d9e4u,esp+0x24u);
        }
        x.x(1,single(one));
        x.x(0,x.ld(esp+0x24u));
    }
    x.x(0,_mm_mul_ss(x.x(0),x.ld(esp+0x10u)));
    edi=x.pop();
    esi=x.pop();
    ebx>>=9;
    ebp=x.pop();
    const bool inverted=(ebx&1u)!=0u;
    ebx=x.pop();
    if(inverted) {
        x.x(1,_mm_sub_ss(x.x(1),x.x(0)));
        x.x(0,x.x(1));
    }
    esp+=0x10u;
    x.ret(8);
}

// =====================================================================
// 150900: the first local player index ([4F54F0] +0xC, four slots) that is
// not -1, or -1. EAX; ECX the slot address, EDX -1. ret.
// =====================================================================
void r_150900(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx;
    ecx=x.d(local_players);
    edx=0xffffffffu;
    eax=0u;
    ecx+=0xcu;
    do {
        if(x.d(ecx)!=0xffffffffu){x.ret();return;}
        ++eax;
        ecx+=4u;
    } while(less(eax,4u));
    eax=edx;
    x.ret();
}

// =====================================================================
// 12E9A0: whether any sound activity holds the ducking (AL 1): the flag
// [4F9C6D], any of seven state bytes ([556DFE..556E06]) or a positive
// count at [4F2D40] or [4F2D70]. ECX the last value read. ret.
// =====================================================================
void r_12e9a0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx;
    lo8(ecx,x.b(activity_flag));
    lo8(eax,0u);
    if((ecx&0xffu)!=0u)lo8(eax,1u);
    bool active=false;
    for(const auto offset:{5u,0u,1u,4u,2u,3u,8u}) {
        lo8(ecx,x.b(activity_bytes+offset));
        if((ecx&0xffu)!=0u){active=true;break;}
    }
    if(!active) {
        ecx=x.d(activity_counters[0]);
        if(std::int32_t(ecx)>0)active=true;
        else {
            ecx=x.d(activity_counters[1]);
            active=std::int32_t(ecx)>0;
        }
    }
    if(active)lo8(eax,1u);
    x.ret();
}

// =====================================================================
// 225FB0: stops the platform state's two streams ([52D674] +0x2A90 and
// +0x2A94) through 38581F (stream, 0; ret 8). ret.
// =====================================================================
void r_225fb0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&esi=x.esi;
    x.push(0x225fb0u,esi);
    esi=0x2a90u;
    do {
        eax=x.d(platform);
        ecx=x.d(esi+eax);
        x.push(0x225fbeu,0u);
        x.push(0x225fc0u,ecx);
        x.call(0x225fc1u,0x38581fu);
        esi+=4u;
    } while(less(esi,0x2a98u));
    esi=x.pop();
    x.ret();
}

// =====================================================================
// 12BBE0: steps each playback slot ([4F2C50] +0x208 count) through 12CF60
// (EAX the slot index). ret.
// =====================================================================
void r_12bbe0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&esi=x.esi;
    eax=x.d(sound_state);
    x.push(0x12bbe5u,esi);
    esi=0u;
    if(std::int16_t(x.w16(eax+0x208u))>0) {
        do {
            eax=esi;
            x.call(0x12bbf3u,0x12cf60u);
            ecx=x.d(sound_state);
            ++esi;
        } while(less16(esi,x.w16(ecx+0x208u)));
    }
    esi=x.pop();
    x.ret();
}

// =====================================================================
// 12BE90: hands each active playback slot (handle at +0, not stopping +9)
// its gathered spatial record: the listener record EBX[slot +0xE] or, for
// a slot without one, the audible record EDI[slot +0xC]; 12C2A0 (EAX the
// slot; stack: the record; ret 4). ret.
// =====================================================================
void r_12be90(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esi=x.esi,&edi=x.edi;
    eax=x.d(sound_state);
    x.push(0x12be95u,esi);
    esi=0u;
    if(std::int16_t(x.w16(eax+0x208u))>0) {
        do {
            edx=x.d(playbacks);
            eax=sx16(esi);
            ecx=eax+eax*8u;
            eax=edx+ecx*4u;
            if(x.d(eax)!=0xffffffffu) {
                lo8(ecx,x.b(eax+9u));
                if((ecx&0xffu)==0u) {
                    lo16(ecx,x.w16(eax+0xeu));
                    bool play=true;
                    if((ecx&0xffffu)!=0xffffu) {
                        eax=sx16(ecx);
                        ecx=x.d(ebx+eax*4u);
                        x.push(0x12beccu,ecx);
                    } else {
                        lo16(eax,x.w16(eax+0xcu));
                        if((eax&0xffffu)==0xffffu)play=false;
                        else {
                            edx=sx16(eax);
                            eax=x.d(edi+edx*4u);
                            x.push(0x12bedfu,eax);
                        }
                    }
                    if(play) {
                        eax=esi;
                        x.call(0x12bee2u,0x12c2a0u);
                    }
                }
            }
            ecx=x.d(sound_state);
            edx=sx16(x.w16(ecx+0x208u));
            ++esi;
        } while(less(esi,edx));
    }
    esi=x.pop();
    x.ret();
}

} // namespace

std::span<const CampaignRoutineEntry> campaign_playback_routines() {
    static constexpr CampaignRoutineEntry table[]{
        {0x12d6f0u,r_12d6f0},
        {0x127ad0u,r_127ad0},
        {0x127b70u,r_127b70},
        {0x12d730u,r_12d730},
        {0x12d840u,r_12d840},
        {0x150900u,r_150900},
        {0x12e9a0u,r_12e9a0},
        {0x225fb0u,r_225fb0},
        {0x12bbe0u,r_12bbe0},
        {0x12be90u,r_12be90},
    };
    return table;
}
} // namespace halo2::native_tu15_sound
