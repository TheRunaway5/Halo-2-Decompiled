// Native campaign audio routines: the mix part (see
// native_tu15_sound_campaign_audio.hpp). 129F10 produces the four sound
// listener records from the local players' cameras (with 120060/120310, the
// rotation that blends a vehicle seat's listener toward its unit's aim) and,
// on frames that push the listener, hands the camera listener and the two
// sound environments to the DSP through 2271A0. 221AF0 is the per-frame class
// channel mixer: it fades each channel's permutation gains, routes multi-
// channel (surround) sounds over their channel slots and schedules the next
// random detail sound of each channel.
//
// Each routine is a faithful decompilation of the retail TU1.5 function at
// register level: the same memory reads and stores (each store published
// with its original instruction address), the same register values at every
// call and at the return, the same SSE scalar arithmetic, COMISS/UCOMISS
// comparisons and x87 operations under the caller's control word. Branches
// are evaluated in C++ from the operands the original compares, not through
// emulated EFLAGS. Every CALL pushes its original return address through
// campaign_audio_invoke (a native campaign routine, the lifted original of an
// unconverted one, or the live world for the core game helpers). Retail
// .rdata constants are literals read from the TU1.5 XBE.
#include "audio/native_tu15_sound_campaign_audio.hpp"

namespace halo2::native_tu15_sound {
namespace {

// Retail .rdata constants (TU1.5 XBE).
constexpr std::uint32_t one=0x3f800000u;          // 0x46A0D0: 1.0
constexpr std::uint32_t half=0x3f000000u;         // 0x46A0CC: 0.5
constexpr std::uint32_t two=0x40000000u;          // 0x46A11C: 2.0
constexpr std::uint32_t thousandth=0x3a83126fu;   // 0x46A180: 0.001
constexpr std::uint32_t orientation_step=0x3d4ccccdu; // 0x46A12C: 0.05
constexpr std::uint32_t gain_epsilon=0x38d1b717u; // 0x46A0EC: 0.0001
constexpr std::uint32_t two_to_32=0x4f800000u;    // 0x46A0C8: 4294967296.0
constexpr std::uint32_t thousand=0x447a0000u;     // 0x46A1DC: 1000.0
// 0x46A160 (1/65535) is random_word_scale (native_tu15_sound_kernel.hpp).
// 0x455CF8: the default sound environment (0x48 bytes), set on a DSP
// environment slot that no environment feeds.
constexpr std::uint32_t default_environment=0x455cf8u;
constexpr std::array<std::uint32_t,18> default_environment_words{
    0x00000000u,0x00008000u,0xc2c80000u,0xc2c80000u,0x00000000u,0x3f800000u,0x3f800000u,0xc2c80000u,0x00000000u,
    0xc2c80000u,0x00000000u,0x3f800000u,0x3f800000u,0x459c4000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
constexpr std::uint32_t none=0xffffffffu;

// Retail globals (runtime data, read from memory).
constexpr std::uint32_t game_globals=0x4f3218u;     // +0x1120 byte: the map is running
constexpr std::uint32_t local_players=0x4f54f0u;    // +0xC: the local players' player datums (4)
constexpr std::uint32_t players=0x4f54f4u;          // player data array (0x21C; +0x2C unit datum)
constexpr std::uint32_t objects=0x4ecbd0u;          // object header array (+0x44 data; 12-byte rows, +8 object)
constexpr std::uint32_t camera_override=0x474c24u;  // word: -1 unless the listener follows the observers
constexpr std::uint32_t observer_cameras=0x4f655cu; // four 0x358-byte observer cameras (to 0x4F72BC)
constexpr std::uint32_t identity_rotation=0x474d2cu;// pointer to the identity quaternion
constexpr std::uint32_t listener_position=0x474ce8u;// pointers to the camera listener vectors
constexpr std::uint32_t listener_velocity=0x474d04u;
constexpr std::uint32_t listener_forward=0x474d08u;
constexpr std::uint32_t listener_up=0x474d10u;
constexpr std::uint32_t sound_state=0x4f2c50u;      // +0x7F frame byte, +0x84 time (ms), +0x88 listeners (0x48),
                                                    // +0x1AC/+0x1C8 the two sound environments, +0x1E0 rate
constexpr std::uint32_t sound_structure=0x4ecc18u;  // +0x68 water planes (0x18), +0xA0 clusters (0xB0)
constexpr std::uint32_t dsp=0x52d674u;              // DSP listener cache: +0 initialized, +0x2918 position,
                                                    // +0x2924 orientation, +0x2950 environments (0x48),
                                                    // +0x29E0 mixes, +0x29E8 environment words (0x10),
                                                    // +0x2A08/+0x2A0A dirty bytes, +0x2AB0 listener handle
constexpr std::uint32_t class_channels=0x50ebe8u;   // class channel data array (0xD4-byte elements)
constexpr std::uint32_t permutation_slots=0x4f2c4cu;// permutation slot data array (0xBC-byte elements)
constexpr std::uint32_t loop_channels=0x52d66cu;    // default looping channel
constexpr std::uint32_t loop_channel_array=0x52d668u;// looping channel data array (0x1C-byte elements)
constexpr std::uint32_t random_globals_g=random_globals; // +4: the game random seed

// --- memory, registers and SSE ---------------------------------------------
// Reads and stores go through the same accessors the lifted routines use.
inline std::uint32_t rd32(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,4);}
inline std::uint32_t rd16(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,2);}
inline std::uint32_t rd8(const M &m,std::uint32_t p){return physics_step_detail::r(m,p,1);}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline std::uint32_t sar(std::uint32_t v,unsigned n){return std::uint32_t(std::int32_t(v)>>n);}
inline bool less(std::uint32_t x,std::uint32_t y){return std::int32_t(x)<std::int32_t(y);}   // CMP; JL
inline bool positive(std::uint32_t x){return std::int32_t(x)>0;}                         // TEST; JG
// MOV AL / MOV AX: only the low byte or word of the register changes.
inline void lo8(std::uint32_t &r,std::uint32_t v){r=(r&0xffffff00u)|(v&0xffu);}
inline void lo16(std::uint32_t &r,std::uint32_t v){r=(r&0xffff0000u)|(v&0xffffu);}
inline __m128 single(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}   // MOVSS from memory
inline std::uint32_t lane0(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
inline float flt(std::uint32_t bits){return std::bit_cast<float>(bits);}
// COMISS/UCOMISS x,y: the flags the compiled branches test, raising exactly
// the original instruction's MXCSR exceptions.
struct Compared {
    bool cf,zf,pf;
    bool above()const{return !cf && !zf;}          // JA
    bool ordered_equal()const{return zf && !pf;}   // LAHF; TEST AH,44h; JNP
    // LAHF after the comparison: SF ZF 0 AF 0 PF 1 CF (SF, AF clear).
    std::uint32_t lahf()const{return (cf?1u:0u)|2u|(pf?4u:0u)|(zf?0x40u:0u);}
};
Compared comiss(__m128 x,__m128 y,bool unordered=false) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char carry,zero,parity;
    if(unordered)asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(x),"x"(y):"cc");
    else asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(x),"x"(y):"cc");
    return {carry!=0,zero!=0,parity!=0};
#else
    throw FormatError("Campaign mix scalar comparison backend unavailable");
#endif
}
// FCOMIP ST(0),ST(1) on the extended operands: CF below, ZF equal, PF
// unordered (any NaN signals invalid).
Compared fcomip(long double st0,long double st1) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char carry,zero,parity;
    asm volatile("fldt %4\n\tfldt %3\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tsete %1\n\tsetp %2\n\tfstp %%st(0)"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(st0),"m"(st1):"cc","st");
    return {carry!=0,zero!=0,parity!=0};
#else
    throw FormatError("Campaign mix extended comparison backend unavailable");
#endif
}
// FIADD m32: the extended sum and the status word (C1: rounded up).
long double integer_add(long double value,std::int32_t integer,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    asm volatile("fiaddl %3\n\tfnstsw %1":"=t"(result),"=m"(status):"0"(value),"m"(integer));
    return result;
#else
    throw FormatError("Campaign mix FIADD backend unavailable");
#endif
}
// An x87 m32 operand loaded as its own FLD, as the reference routines do.
long double load_single(float x){volatile long double v=static_cast<long double>(x);return v;}
// The original's x87 loads need `slots` free registers. A caller stack that
// would overflow is refused exactly as the reference routine refuses it (no
// game caller has one).
void require_x87_room(const Abi &a,unsigned slots) {
    if(a.x87_depth+slots>8u)throw FormatError("Required original solver x87 stack-fault owner unavailable");
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

// One running routine: the ABI registers (named), its XMM registers, its
// memory, and the caller's floating-point environment for its duration.
struct Cpu {
    CampaignContext &c;Abi &a;M &m;const O &o;
    std::uint32_t &eax,&ecx,&edx,&ebx,&esp,&ebp,&esi,&edi;
    __m128 x0,x1,x2,x3,x4,x5,x6,x7;
    Floating floating;
    bool live=true;    // false while a callee owns the ABI
    Cpu(CampaignContext &context,Abi &abi):c(context),a(abi),m(context.m),o(context.o),
        eax(abi.gp[0]),ecx(abi.gp[1]),edx(abi.gp[2]),ebx(abi.gp[3]),
        esp(abi.gp[4]),ebp(abi.gp[5]),esi(abi.gp[6]),edi(abi.gp[7]),floating(abi,context.fp) {load_xmm();}
    Cpu(const Cpu &)=delete;
    ~Cpu() {
        // A callee that faulted left its state in the ABI; Floating then
        // publishes the callee's floating-point environment.
        if(live)store_xmm();else fp_in(a);
    }
    void load_xmm() {
        x0=xmm(a,0);x1=xmm(a,1);x2=xmm(a,2);x3=xmm(a,3);x4=xmm(a,4);x5=xmm(a,5);x6=xmm(a,6);x7=xmm(a,7);
    }
    void store_xmm() {
        set_xmm(a,0,x0);set_xmm(a,1,x1);set_xmm(a,2,x2);set_xmm(a,3,x3);
        set_xmm(a,4,x4);set_xmm(a,5,x5);set_xmm(a,6,x6);set_xmm(a,7,x7);
    }
    std::uint32_t d(std::uint32_t p)const{return rd32(m,p);}
    std::uint32_t w16(std::uint32_t p)const{return rd16(m,p);}
    std::uint32_t b(std::uint32_t p)const{return rd8(m,p);}
    __m128 ld(std::uint32_t p)const{return single(rd32(m,p));}
    void st(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,4,v);}
    void st(std::uint32_t site,std::uint32_t p,__m128 v){physics_step_detail::w(m,o,site,p,4,lane0(v));}
    void st16(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,2,v);}
    void st8(std::uint32_t site,std::uint32_t p,std::uint32_t v){physics_step_detail::w(m,o,site,p,1,v);}
    void push(std::uint32_t site,std::uint32_t v){esp-=4u;st(site,esp,v);}
    std::uint32_t pop(){const auto v=d(esp);esp+=4u;return v;}
    // RET n: pops the return address and n bytes of stack arguments.
    void ret(std::uint32_t arguments=0){pop();esp+=arguments;}
    // CALL rel32 at `site`: pushes the return address and runs the callee
    // with the live registers and floating-point environment. Stack arguments
    // are pushed beforehand; a callee that pops them (RET n) does so itself.
    void call(std::uint32_t site,std::uint32_t target) {
        store_xmm();fp_out(a);live=false;
        campaign_audio_invoke(c,a,site+5u,target);
        live=true;fp_in(a);load_xmm();
    }
    // JMP rel32 (a tail call): the callee returns to this routine's caller.
    void jump(std::uint32_t target) {
        store_xmm();fp_out(a);live=false;
        campaign_audio_call(c,a,target);
        live=true;fp_in(a);load_xmm();
    }
    bool down()const{return (a.eflags&0x400u)!=0;}   // DF
    // A dword at p, the default environment (.rdata 0x455CF8) as literals.
    std::uint32_t rodata(std::uint32_t p)const {
        const auto offset=p-default_environment;
        if(offset<default_environment_words.size()*4u && !(offset&3u))return default_environment_words[offset>>2];
        return d(p);
    }
    // REPE CMPSD (ECX dwords, ESI against EDI): true when every dword
    // compared equal (ZF). ECX, ESI and EDI advance as the original's do.
    bool repe_cmpsd(bool literal_source=false) {
        const auto step=down()?0xfffffffcu:4u;
        bool equal=true;
        while(ecx) {
            const auto x=literal_source?rodata(esi):d(esi);
            equal=x==d(edi);
            esi+=step;edi+=step;--ecx;
            if(!equal)break;
        }
        return equal;
    }
    // REP MOVSD: ECX dwords from ESI to EDI.
    void rep_movsd(std::uint32_t site,bool literal_source=false) {
        const auto step=down()?0xfffffffcu:4u;
        while(ecx){const auto v=literal_source?rodata(esi):d(esi);st(site,edi,v);esi+=step;edi+=step;--ecx;}
    }
    // x87: ST(i) of the caller-visible stack.
    long double &st0(){return a.x87[0];}
    long double &sti(unsigned i){return a.x87[i];}
    void fld(std::uint32_t bits){floating.push(load_single(flt(bits)));}            // FLD m32
    void fld_constant(std::uint32_t bits){floating.push(static_cast<long double>(flt(bits)));}
    void fabs0(){st0()=std::fabs(st0());floating.clear_c1();}                       // FABS
    void fxch(unsigned i){std::swap(a.x87[0],a.x87[i]);floating.clear_c1();}       // FXCH ST(i)
    void fstp0() {                                                                  // FSTP ST(0)
        if(!a.x87_depth)throw FormatError("Required original solver x87 stack-fault owner unavailable");
        floating.clear_c1();floating.pop();
    }
    // FLD m32 constant; FCOMIP ST,ST(1); FSTP ST(0): the constant against
    // the value under it, both popped.
    Compared compare_below(std::uint32_t constant) {
        fld_constant(constant);
        const auto result=fcomip(sti(0),sti(1));
        floating.pop();fstp0();
        return result;
    }
};

// =====================================================================
// 120060: the rotation (quaternion x,y,z,w into [EAX]) that turns the unit
// vector [ECX] onto the unit vector [EDX]: s = sqrt(2(1 + a.b)); unless s is
// below 0.001 (opposite vectors: 11F9C0 picks the half-turn and +0xC gets
// XMM2 as that callee leaves it), the axis is a x b / s and w = s/2. The
// local dword it pushes (ECX's stack slot holds s) pops into ECX. ret.
// =====================================================================
void r_120060(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&esp=x.esp;
    auto &x0=x.x0,&x1=x.x1,&x2=x.x2,&x3=x.x3,&x4=x.x4,&x5=x.x5;
    x.push(0x120060u,ecx);
    x0=x.ld(ecx+4u);x0=_mm_mul_ss(x0,x.ld(edx+4u));
    x1=x.ld(ecx);x1=_mm_mul_ss(x1,x.ld(edx));
    x3=single(one);
    x2=_mm_setzero_ps();
    x0=_mm_add_ss(x0,x1);
    x1=x.ld(edx+8u);x1=_mm_mul_ss(x1,x.ld(ecx+8u));
    x0=_mm_add_ss(x0,x1);
    x0=_mm_add_ss(x0,x3);
    x0=_mm_mul_ss(x0,single(two));
    const bool above=comiss(x0,x2).above();
    x.st(0x12009fu,esp,x0);
    if(!above)x.st(0x1200a6u,esp,x2);
    // FLD [ESP]; FSQRT; FSTP [ESP]
    require_x87_room(a,1);
    x.fld(x.d(esp));
    x.st0()=root(x.st0());
    x.st(0x1200b0u,esp,std::bit_cast<std::uint32_t>(x.floating.store_single(x.st0())));
    x.floating.pop();
    // FLD 0.001; FLD [ESP]; FCOMIP ST,ST(1); FSTP ST(0); JB
    require_x87_room(a,2);
    x.fld_constant(thousandth);
    x.fld(x.d(esp));
    const auto small=fcomip(x.sti(0),x.sti(1)).cf;
    x.floating.pop();x.fstp0();
    if(small) {
        x.call(0x120138u,0x11f9c0u);
        x.st(0x12013du,eax+0xcu,x2);
        ecx=x.pop();
        x.ret();
        return;
    }
    x0=x.ld(ecx);x0=_mm_mul_ss(x0,x.ld(edx+4u));
    x1=x.ld(ecx+4u);x1=_mm_mul_ss(x1,x.ld(edx));
    x2=x.ld(edx+8u);x2=_mm_mul_ss(x2,x.ld(ecx));
    x4=x.ld(ecx+4u);x4=_mm_mul_ss(x4,x.ld(edx+8u));
    x5=x.ld(ecx+8u);x5=_mm_mul_ss(x5,x.ld(edx+4u));
    x0=_mm_sub_ss(x0,x1);
    x1=x.ld(edx);x1=_mm_mul_ss(x1,x.ld(ecx+8u));
    x1=_mm_sub_ss(x1,x2);
    x2=x.ld(esp);
    x3=_mm_div_ss(x3,x2);                        // 1/s
    x2=_mm_mul_ss(x2,single(half));              // w = s/2
    x4=_mm_sub_ss(x4,x5);
    x4=_mm_mul_ss(x4,x3);
    x1=_mm_mul_ss(x1,x3);
    x0=_mm_mul_ss(x0,x3);
    x.st(0x120123u,eax,x4);
    x.st(0x120127u,eax+4u,x1);
    x.st(0x12012cu,eax+8u,x0);
    x.st(0x120131u,eax+0xcu,x2);
    ecx=x.pop();
    x.ret();
}

// =====================================================================
// 120310: blends the quaternions [ECX] (weight XMM0 = t) and [EDX] (weight
// 1 - t) into [EAX], negating t when the two lie in opposite hemispheres
// (dot below zero), then normalizes [EAX] by jumping to 11FFD0.
// =====================================================================
void r_120310(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx;
    auto &x0=x.x0,&x1=x.x1,&x2=x.x2,&x3=x.x3;
    x3=x.ld(ecx+8u);x3=_mm_mul_ss(x3,x.ld(edx+8u));
    x2=x.ld(ecx+4u);x2=_mm_mul_ss(x2,x.ld(edx+4u));
    x1=single(one);
    x3=_mm_add_ss(x3,x2);
    x2=x.ld(edx+0xcu);x2=_mm_mul_ss(x2,x.ld(ecx+0xcu));
    x3=_mm_add_ss(x3,x2);
    x2=x.ld(edx);x2=_mm_mul_ss(x2,x.ld(ecx));
    x3=_mm_add_ss(x3,x2);
    x2=_mm_setzero_ps();
    const bool opposite=comiss(x2,x3).above();
    x1=_mm_sub_ss(x1,x0);                        // 1 - t
    if(opposite) {
        x2=_mm_sub_ss(x2,x0);
        x0=x2;                                   // -t
    }
    x2=x.ld(edx);x3=x.ld(ecx);
    x2=_mm_mul_ss(x2,x1);x3=_mm_mul_ss(x3,x0);x2=_mm_add_ss(x2,x3);
    x.st(0x120371u,eax,x2);
    x2=x.ld(ecx+4u);x2=_mm_mul_ss(x2,x0);
    x3=x1;x3=_mm_mul_ss(x3,x.ld(edx+4u));
    x2=_mm_add_ss(x2,x3);
    x.st(0x12038au,eax+4u,x2);
    x2=x.ld(ecx+8u);x2=_mm_mul_ss(x2,x0);
    x3=x1;x3=_mm_mul_ss(x3,x.ld(edx+8u));
    x2=_mm_add_ss(x2,x3);
    x.st(0x1203a4u,eax+8u,x2);
    x2=x.ld(edx+0xcu);
    x0=_mm_mul_ss(x0,x.ld(ecx+0xcu));
    x2=_mm_mul_ss(x2,x1);
    x2=_mm_add_ss(x2,x0);
    x.st(0x1203bbu,eax+0xcu,x2);
    x.jump(0x11ffd0u);
}

// =====================================================================
// 2271A0: pushes a listener to the DSP ([52D674]). Stack arg0: the listener
// block (+0 position, +0xC forward, +0x18 up, +0x2C velocity word, +0x30
// environment count, +0x34 environments: 0x18 each, +0 environment record
// pointer, +4 16 bytes, +0x14 mix). The position (387678: handle, x, z, y,
// 1) is set once, before the DSP listener is initialized; the orientation
// (38762E: handle, forward, up, 1) whenever a component moved by 0.05 or
// more, or while uninitialized. Up to two environment slots cache their
// 0x48-byte record (+0x2950), 16 bytes (+0x29E8) and mix (+0x29E0, 0.0001
// apart) and raise a dirty byte (+0x2A08/+0x2A0A + slot) on change; the
// slots past the count get the default environment and a zero mix. ret 4.
// =====================================================================
void r_2271a0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    auto &x0=x.x0;
    eax=x.d(dsp);
    lo8(ecx,x.b(eax));
    esp-=0x18u;
    const bool initialized=(ecx&0xffu)!=0u;
    x.push(0x2271acu,ebx);
    x.push(0x2271adu,ebp);
    x.push(0x2271aeu,esi);
    esi=x.d(esp+0x28u);                          // arg0
    x.push(0x2271b3u,edi);
    if(!initialized) {
        ecx=x.d(esi+4u);
        edx=x.d(esi+8u);
        x.push(0x2271bcu,1u);
        x.push(0x2271beu,ecx);
        ecx=x.d(esi);
        x.push(0x2271c1u,edx);
        edx=x.d(eax+0x2ab0u);
        x.push(0x2271c8u,ecx);
        x.push(0x2271c9u,edx);
        x.call(0x2271cau,0x387678u);             // set position (ret 20)
        eax=x.d(dsp);
        edx=esi;
        edi=x.d(edx);
        ecx=eax+0x2918u;
        x.st(0x2271deu,ecx,edi);
        edi=x.d(edx+4u);
        x.st(0x2271e3u,ecx+4u,edi);
        edx=x.d(edx+8u);
        x.st(0x2271e9u,ecx+8u,edx);
    }
    // FLD [ESI+k]; FSUB [EAX+0x2924+k]; FABS against 0.05 for the six
    // orientation components: JBE (moved or unordered) sets the orientation.
    bool orient=false;
    require_x87_room(a,2);
    for(std::uint32_t k=0;k<0x18u && !orient;k+=4u) {
        x.fld(x.d(esi+0xcu+k));
        x.st0()=sub(x.st0(),load_single(flt(x.d(eax+0x2924u+k))));
        x.fabs0();
        const auto moved=x.compare_below(orientation_step);
        orient=moved.cf||moved.zf;
    }
    if(!orient)orient=x.b(eax)==0u;
    if(orient) {
        ecx=x.d(esi+0x1cu);
        edx=x.d(esi+0x20u);
        eax=x.d(eax+0x2ab0u);
        x.push(0x227287u,1u);
        x.push(0x227289u,ecx);
        ecx=x.d(esi+0x18u);
        x.push(0x22728du,edx);
        edx=x.d(esi+0x10u);
        x.push(0x227291u,ecx);
        ecx=x.d(esi+0x14u);
        x.push(0x227295u,edx);
        edx=x.d(esi+0xcu);
        x.push(0x227299u,ecx);
        x.push(0x22729au,edx);
        x.push(0x22729bu,eax);
        x.call(0x22729cu,0x38762eu);             // set orientation (ret 32)
        edx=x.d(esi+0xcu);
        eax=x.d(dsp);
        ecx=eax+0x2924u;
        x.st(0x2272afu,ecx,edx);
        edx=x.d(esi+0x10u);
        x.st(0x2272b4u,ecx+4u,edx);
        edx=x.d(esi+0x14u);
        x.st(0x2272bau,ecx+8u,edx);
        edx=x.d(esi+0x18u);
        ecx=eax+0x2930u;
        x.st(0x2272c6u,ecx,edx);
        edx=x.d(esi+0x1cu);
        x.st(0x2272cbu,ecx+4u,edx);
        edx=x.d(esi+0x20u);
        x.st(0x2272d1u,ecx+8u,edx);
    }
    // The environments the listener feeds (at most two).
    ecx=x.d(esi+0x30u);
    if(!less(ecx,2u))ecx=2u;
    edx=0u;
    x.st(0x2272e5u,esp+0x20u,ecx);
    if(positive(ecx)) {
        ecx=eax+0x29e0u;
        x.st(0x2272f5u,esp+0x1cu,ecx);           // the slot's mix
        ecx=eax+0x29e8u;
        edi=eax+0x2950u;
        x.st(0x227305u,esp+0x14u,edx);           // the environment's offset (0x18 each)
        x.st(0x227309u,esp+0x10u,ecx);           // the slot's 16 bytes
        x.st(0x22730du,esp+0x18u,edi);           // the slot's record
        bool first=true;
        do {
            if(!first)esi=x.d(esp+0x2cu);        // arg0
            first=false;
            ebp=x.d(esi+0x34u);
            ecx=x.d(esp+0x14u);
            esi=x.d(ebp+ecx);
            ebp+=ecx;
            ecx=0x12u;
            ebx=0u;
            bool same=x.repe_cmpsd();
            x.st(0x227336u,esp+0x24u,ebp);
            if(!same || x.b(eax)==0u) {
                esi=x.d(ebp);
                edi=x.d(esp+0x18u);
                ecx=0x12u;
                x.rep_movsd(0x22734du);
                x.st8(0x22734fu,eax+0x2a08u+edx,1u);
            }
            esi=x.d(esp+0x10u);
            ebp+=4u;
            ecx=4u;
            edi=ebp;
            ebx=0u;
            same=x.repe_cmpsd();
            if(!same || x.b(eax)==0u) {
                esi=x.d(ebp);
                ecx=x.d(esp+0x10u);
                x.st(0x227377u,ecx,esi);
                esi=x.d(ebp+4u);
                x.st(0x22737cu,ecx+4u,esi);
                esi=x.d(ebp+8u);
                x.st(0x227382u,ecx+8u,esi);
                esi=x.d(ebp+0xcu);
                x.st(0x227388u,ecx+0xcu,esi);
                x.st8(0x22738bu,eax+0x2a0au+edx,1u);
            }
            ecx=x.d(esp+0x1cu);
            require_x87_room(a,2);
            x.fld(x.d(ecx));
            esi=x.d(esp+0x24u);
            x.st0()=sub(x.st0(),load_single(flt(x.d(esi+0x14u))));
            x.fabs0();
            const auto mixed=x.compare_below(gain_epsilon);
            if(mixed.cf||mixed.zf||x.b(eax)==0u) {
                esi=x.d(esi+0x14u);
                x.st(0x2273b6u,ecx,esi);
                x.st8(0x2273b8u,eax+0x2a0au+edx,1u);
            }
            edi=x.d(esp+0x18u);
            ebx=x.d(esp+0x10u);
            ecx+=4u;
            x.st(0x2273cbu,esp+0x1cu,ecx);
            x.st(0x2273cfu,esp+0x14u,x.d(esp+0x14u)+0x18u);
            ecx=x.d(esp+0x20u);
            ++edx;
            edi+=0x48u;
            ebx+=0x10u;
            x.st(0x2273e1u,esp+0x18u,edi);
            x.st(0x2273e5u,esp+0x10u,ebx);
        } while(less(edx,ecx));
    }
    // The slots no environment feeds return to the default.
    edx=ecx;
    if(less(ecx,2u)) {
        x0=_mm_setzero_ps();
        esi=eax+0x29e0u+ecx*4u;
        ecx=ecx+ecx*8u;
        x.st(0x227403u,esp+0x2cu,esi);
        ebp=eax+0x2950u+ecx*8u;
        do {
            ecx=0x12u;
            edi=ebp;
            esi=default_environment;
            ebx=0u;
            if(!x.repe_cmpsd(true) || x.b(eax)==0u) {
                ecx=0x12u;
                esi=default_environment;
                edi=ebp;
                x.rep_movsd(0x227433u,true);
                x.st8(0x227435u,eax+0x2a08u+edx,1u);
            }
            ecx=x.d(esp+0x2cu);
            require_x87_room(a,2);
            x.fld(x.d(ecx));
            x.fabs0();
            const auto mixed=x.compare_below(gain_epsilon);
            if(mixed.cf||mixed.zf||x.b(eax)==0u) {
                x.st(0x227456u,ecx,x0);
                x.st8(0x22745au,eax+0x2a0au+edx,1u);
            }
            ++edx;
            ecx+=4u;
            ebp+=0x48u;
            x.st(0x22746cu,esp+0x2cu,ecx);
        } while(less(edx,2u));
    }
    edi=x.pop();
    esi=x.pop();
    ebp=x.pop();
    ebx=x.pop();
    esp+=0x18u;
    x.ret(4);
}

// =====================================================================
// 129F10: the listener producer. Stack arg0 (byte): push the listener to
// the DSP this frame. While the map runs ([4F3218] +0x1120), each local
// player p (player datum at [4F54F0]+0xC+4p, observer camera 4F655C+p*0x358,
// listener record [4F2C50]+0x88+p*0x48) gets its listener: the camera's
// (the observer cameras follow [474C24]), or, for a unit in a vehicle seat
// seen in first person (158290 camera mode 0/1), the camera blended toward
// the unit (CD440) by the seat's factor (seat +0x40): the camera position
// interpolated, and its forward/up rotated by the slerp (120060, 120310)
// between identity and the rotation from the camera to the unit's aim. The
// listener's location comes from BC200/11E600 (the camera's when that has
// no cluster). The record gets +0 location, +4 cluster, +6 active, +7
// below the cluster's water plane, +8 scale 1.0, +0xC forward, +0x18 left,
// +0x24 up, +0x30 position and +0x3C the camera velocity in that basis; a
// slot without a player gets location and cluster -1. With arg0 set, the
// camera listener ([474CE8] position, [474D08] forward, [474D10] up,
// [474D04] velocity) and the two sound environments ([4F2C50] +0x1AC,
// +0x1C8) go to 2271A0. ret 4.
// =====================================================================
void r_129f10(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    auto &x0=x.x0,&x1=x.x1,&x2=x.x2,&x3=x.x3,&x4=x.x4,&x5=x.x5,&x6=x.x6,&x7=x.x7;
    // MOVSD [ESP+o] from a 3-dword vector at p, through the registers the
    // original uses, is written out below exactly as compiled.
    eax=x.d(game_globals);
    esp-=0xe8u;
    x.push(0x129f1bu,edi);
    edi=0u;
    if(eax!=0u) lo8(ecx,x.b(eax+0x1120u));
    if(eax==0u || (ecx&0xffu)==0u) goto done;
    eax=x.d(local_players);
    edx=x.d(sound_state);
    x.push(0x129f3fu,ebx);
    x.push(0x129f40u,ebp);
    eax+=0xcu;
    x.push(0x129f44u,esi);
    x.st(0x129f45u,esp+0x64u,edi);               // player index
    x.st(0x129f49u,esp+0x38u,observer_cameras);  // the player's observer camera
    x.st(0x129f51u,esp+0x44u,eax);               // the player's datum
    ebp=edx+0xa8u;                               // listener record + 0x20
    x.st(0x129f5bu,esp+0x48u,edi);               // index * 0x358
    do {
        eax=x.d(esp+0x48u);
        esi=none;
        if(eax==0xfffffca8u) goto no_player;
        ecx=x.d(esp+0x44u);
        eax=x.d(ecx);
        lo8(ecx,eax!=esi?1u:0u);                 // SETNE CL
        if(eax==esi) goto no_player;
        {
            edx=x.d(players);
            ecx=x.d(edx+0x44u);
            eax&=0xffffu;
            eax*=0x21cu;
            esi=x.d(eax+ecx+0x2cu);              // the player's unit
            eax=edi;
            x.st(0x129f9fu,esp+0x34u,esi);
            x.call(0x129fa3u,0x158290u);         // camera mode of player EAX
            ebx=eax;
            if(esi==none) goto camera;
            eax=edi;
            x.call(0x129fb5u,0x158290u);
            edx=1u;
            ecx=eax;
            edx<<=ecx&31u;
            if((edx&3u)==0u) goto camera;
            eax=x.d(objects);
            ecx=x.d(eax+0x44u);
            eax=esi;
            eax&=0xffffu;
            edx=eax+eax*2u;
            edx=x.d(ecx+edx*4u+8u);              // the unit
            eax=x.d(edx+0x14u);                  // its vehicle (-1: none)
            edi=0u;
            {
                const bool cameras=x.w16(camera_override)!=0xffffu;
                x.st(0x129fefu,esp+0x7cu,edx);
                const auto observer=x.d(esp+0x38u);   // CMOVNE EDI,[ESP+38h]
                if(cameras)edi=observer;
            }
            if(eax==none) goto unit_camera;
            lo16(edx,x.w16(edx+0x1fcu));         // the unit's seat
            if((edx&0xffffu)==0xffffu) goto unit_camera;
            if((ebx&0xffffu)==0u) goto unit_camera;
            eax&=0xffffu;
            eax=eax*3u;
            ecx=x.d(ecx+eax*4u+8u);              // the vehicle
            eax=x.d(ecx);
            ecx=x.d(tag_instances);
            ebx=sx16(edx);
            eax&=0xffffu;
            ebx*=0xb0u;
            eax<<=4;
            eax=x.d(eax+ecx+8u);                 // the vehicle's tag
            edx=x.d(eax+0x1ccu);
            eax=x.d(esp+0x34u);
            esi=esp+0x84u;
            ebx+=edx;                            // the seat
            x.call(0x12a057u,0xcd440u);          // the unit's listener point into [ESI]
            // The camera position moved toward the unit by the seat factor.
            x1=x.ld(edi);
            x0=x.ld(esp+0x84u);
            edx=x.d(esp+0x7cu);
            x0=_mm_sub_ss(x0,x1);
            x0=_mm_mul_ss(x0,x.ld(ebx+0x40u));
            x0=_mm_add_ss(x0,x1);
            x1=x.ld(edi+4u);
            x.st(0x12a07fu,esp+0x28u,x0);
            x0=x.ld(esp+0x88u);
            x0=_mm_sub_ss(x0,x1);
            x0=_mm_mul_ss(x0,x.ld(ebx+0x40u));
            x0=_mm_add_ss(x0,x1);
            x1=x.ld(edi+8u);
            x.st(0x12a0a0u,esp+0x2cu,x0);
            x0=x.ld(esp+0x8cu);
            x0=_mm_sub_ss(x0,x1);
            x0=_mm_mul_ss(x0,x.ld(ebx+0x40u));
            esi=edi+0x20u;
            x0=_mm_add_ss(x0,x1);
            edx+=0x180u;
            ecx=esi;
            eax=esp+0x6cu;
            x.st(0x12a0cbu,esp+0x30u,x0);
            x.call(0x12a0d1u,0x120060u);         // camera forward onto the unit's aim
            edx=x.d(identity_rotation);
            x0=x.ld(ebx+0x40u);
            ecx=eax;
            x.call(0x12a0e3u,0x120310u);         // slerp from identity by the seat factor
            // The camera's forward ([ESI]) and up ([EDI+0x2C]) rotated by
            // the quaternion at [ESP+0x6C] (2(q.v)q + (w^2 - q.q)v + 2w(q x v)).
            x0=x.ld(esp+0x74u);
            x2=x.ld(esp+0x70u);
            x1=x.ld(esi+4u);
            x6=x.ld(esi+8u);
            x3=x.ld(esp+0x78u);
            x4=x3;
            x4=_mm_mul_ss(x4,x3);
            x4=_mm_mul_ss(x4,single(two));
            x4=_mm_sub_ss(x4,single(one));
            x1=_mm_mul_ss(x1,x2);
            x6=_mm_mul_ss(x6,x0);
            x1=_mm_add_ss(x1,x6);
            x6=x.ld(esp+0x6cu);
            x7=x6;
            x7=_mm_mul_ss(x7,x.ld(esi));
            x1=_mm_add_ss(x1,x7);
            x7=single(two);
            x3=_mm_mul_ss(x3,x7);
            x.st(0x12a144u,esp+0x60u,x3);
            x.st(0x12a14au,esp+0x68u,x3);
            x3=x.ld(esi+8u);
            x1=_mm_mul_ss(x1,x7);
            x7=x.ld(esi+4u);
            x3=_mm_mul_ss(x3,x2);
            x7=_mm_mul_ss(x7,x0);
            x3=_mm_sub_ss(x3,x7);
            x3=_mm_mul_ss(x3,x.ld(esp+0x68u));
            x7=x6;
            x7=_mm_mul_ss(x7,x1);
            x3=_mm_add_ss(x3,x7);
            x5=x4;
            x7=x5;
            x7=_mm_mul_ss(x7,x.ld(esi));
            x3=_mm_add_ss(x3,x7);
            x.st(0x12a189u,esp+0x10u,x3);
            x3=x0;
            x3=_mm_mul_ss(x3,x.ld(esi));
            x.st(0x12a196u,esp+0x80u,x5);
            x7=x.ld(esi+8u);
            x7=_mm_mul_ss(x7,x6);
            x3=_mm_sub_ss(x3,x7);
            x3=_mm_mul_ss(x3,x.ld(esp+0x68u));
            x.st(0x12a1b2u,esp+0x50u,x4);
            x7=x2;
            x7=_mm_mul_ss(x7,x1);
            x3=_mm_add_ss(x3,x7);
            x7=x.ld(esi+4u);
            x7=_mm_mul_ss(x7,x5);
            x5=x.ld(esi+4u);
            x3=_mm_add_ss(x3,x7);
            x5=_mm_mul_ss(x5,x6);
            x7=x2;
            x7=_mm_mul_ss(x7,x.ld(esi));
            x5=_mm_sub_ss(x5,x7);
            x7=x.ld(esp+0x60u);
            x5=_mm_mul_ss(x5,x7);
            x7=x0;
            x7=_mm_mul_ss(x7,x1);
            x1=x.ld(esi+8u);
            x1=_mm_mul_ss(x1,x4);
            x4=x.ld(edi+0x34u);
            x4=_mm_mul_ss(x4,x0);
            x5=_mm_add_ss(x5,x7);
            x7=x.ld(edi+0x30u);
            x5=_mm_add_ss(x5,x1);
            x1=x.ld(edi+0x30u);
            x1=_mm_mul_ss(x1,x2);
            x1=_mm_add_ss(x1,x4);
            x4=x.ld(edi+0x2cu);
            x4=_mm_mul_ss(x4,x6);
            x1=_mm_add_ss(x1,x4);
            x4=x.ld(esp+0x60u);
            x1=_mm_mul_ss(x1,single(two));
            x.st(0x12a23cu,esp+0x4cu,x4);
            x4=x.ld(edi+0x34u);
            x4=_mm_mul_ss(x4,x2);
            x7=_mm_mul_ss(x7,x0);
            x4=_mm_sub_ss(x4,x7);
            x4=_mm_mul_ss(x4,x.ld(esp+0x4cu));
            eax=x.d(esp+0x34u);
            x7=x1;
            x7=_mm_mul_ss(x7,x6);
            x6=x.ld(edi+0x2cu);
            x6=_mm_mul_ss(x6,x.ld(esp+0x50u));
            x4=_mm_add_ss(x4,x7);
            x7=x.ld(edi+0x2cu);
            x4=_mm_add_ss(x4,x6);
            x6=x.ld(edi+0x2cu);
            x.st(0x12a281u,esp+0x1cu,x4);
            x4=x.ld(edi+0x34u);
            x4=_mm_mul_ss(x4,x.ld(esp+0x6cu));
            x6=_mm_mul_ss(x6,x0);
            x6=_mm_sub_ss(x6,x4);
            x6=_mm_mul_ss(x6,x.ld(esp+0x4cu));
            x4=x2;
            x4=_mm_mul_ss(x4,x1);
            x6=_mm_add_ss(x6,x4);
            x4=x.ld(edi+0x30u);
            x4=_mm_mul_ss(x4,x.ld(esp+0x50u));
            x6=_mm_add_ss(x6,x4);
            x4=x.ld(edi+0x30u);
            x4=_mm_mul_ss(x4,x.ld(esp+0x6cu));
            x0=_mm_mul_ss(x0,x1);
            x7=_mm_mul_ss(x7,x2);
            x4=_mm_sub_ss(x4,x7);
            x4=_mm_mul_ss(x4,x.ld(esp+0x4cu));
            x4=_mm_add_ss(x4,x0);
            x0=x.ld(edi+0x34u);
            x0=_mm_mul_ss(x0,x.ld(esp+0x50u));
            x4=_mm_add_ss(x4,x0);
            x.push(0x12a2eau,0u);
            ecx=esp+0x58u;
            x.st(0x12a2f0u,esp+0x18u,x3);        // forward
            x.st(0x12a2f6u,esp+0x1cu,x5);
            x.st(0x12a2fcu,esp+0x24u,x6);        // up
            x.st(0x12a302u,esp+0x28u,x4);
            x.call(0x12a308u,0xbc200u);          // (ret 4)
            edx=esp+0x28u;
            esi=esp+0x3cu;
            x.call(0x12a315u,0x11e600u);         // the position's location into [ESI]
            ecx=x.d(esp+0x40u);
            if((ecx&0xffffu)!=0xffffu) goto located;
            ecx=x.d(edi+0xcu);                   // no cluster: the camera's location
            x.st(0x12a32bu,esp+0x3cu,ecx);
            ecx=x.d(edi+0x10u);
            goto location;
        }
    unit_camera:
        // The camera (EDI) as it is: position +0x20, forward +0x2C, up +0,
        // velocity +0x14, location +0xC.
        edx=edi+0x20u;
        eax=x.d(edx);
        ecx=x.d(edx+4u);
        edx=x.d(edx+8u);
        x.st(0x12a342u,esp+0x10u,eax);
        x.st(0x12a346u,esp+0x14u,ecx);
        x.st(0x12a34au,esp+0x18u,edx);
        eax=edi+0x2cu;
        ecx=x.d(eax);
        edx=x.d(eax+4u);
        eax=x.d(eax+8u);
        x.st(0x12a359u,esp+0x1cu,ecx);
        x.st(0x12a35du,esp+0x20u,edx);
        ecx=edi;
        edx=x.d(ecx);
        x.st(0x12a365u,esp+0x24u,eax);
        eax=x.d(ecx+4u);
        ecx=x.d(ecx+8u);
        x.st(0x12a36fu,esp+0x28u,edx);
        edx=edi+0x14u;
        x.st(0x12a376u,esp+0x2cu,eax);
        eax=x.d(edx);
        x.st(0x12a37cu,esp+0x30u,ecx);
        ecx=x.d(edx+4u);
        x.st(0x12a383u,esp+0x54u,eax);
        eax=x.d(edi+0xcu);
        x.st(0x12a38au,esp+0x58u,ecx);
        ecx=x.d(edi+0x10u);
        x.st(0x12a391u,esp+0x3cu,eax);
        goto velocity;
    camera:
        // No unit in first person: the observer camera, or the null camera.
        eax=0u;
        {
            const bool cameras=x.w16(camera_override)!=0xffffu;
            const auto observer=x.d(esp+0x38u);    // CMOVNE EAX,[ESP+38h]
            if(cameras)eax=observer;
        }
        ecx=eax+0x20u;
        edx=x.d(ecx);
        x.st(0x12a3abu,esp+0x10u,edx);
        edx=x.d(ecx+4u);
        ecx=x.d(ecx+8u);
        x.st(0x12a3b5u,esp+0x14u,edx);
        x.st(0x12a3b9u,esp+0x18u,ecx);
        edx=eax+0x2cu;
        ecx=x.d(edx);
        x.st(0x12a3c2u,esp+0x1cu,ecx);
        ecx=x.d(edx+4u);
        edx=x.d(edx+8u);
        x.st(0x12a3ccu,esp+0x20u,ecx);
        x.st(0x12a3d0u,esp+0x24u,edx);
        ecx=eax;
        edx=x.d(ecx);
        x.st(0x12a3d8u,esp+0x28u,edx);
        edx=x.d(ecx+4u);
        ecx=x.d(ecx+8u);
        x.st(0x12a3e2u,esp+0x2cu,edx);
        x.st(0x12a3e6u,esp+0x30u,ecx);
        edx=eax+0x14u;
        ecx=x.d(edx);
        x.st(0x12a3efu,esp+0x54u,ecx);
        ecx=x.d(edx+4u);
        x.st(0x12a3f6u,esp+0x58u,ecx);
        ecx=x.d(eax+0xcu);
        x.st(0x12a3fdu,esp+0x3cu,ecx);
        ecx=x.d(eax+0x10u);
    velocity:
        edx=x.d(edx+8u);
        x5=x.ld(esp+0x18u);
        x3=x.ld(esp+0x14u);
        x4=x.ld(esp+0x24u);
        x6=x.ld(esp+0x20u);
        x.st(0x12a41fu,esp+0x5cu,edx);
    location:
        x.st(0x12a423u,esp+0x40u,ecx);
    located:
        // Below the cluster's water plane (cluster +0x70: plane index, the
        // high bit set when the plane decides; 0xFF none).
        lo8(edx,0u);
        x.st8(0x12a42du,ebp-0x1au,1u);           // active
        if((ecx&0xffffu)==0xffffu) goto dry;
        edi=x.d(sound_structure);
        ebx=x.d(edi+0xa0u);
        eax=sx16(ecx);
        eax*=0xb0u;
        eax+=ebx;
        lo8(eax,x.b(eax+0x70u));
        if((eax&0xffu)==0xffu) goto dry;
        edi=x.d(edi+0x68u);
        ebx=0u;
        lo8(ebx,eax);
        ebx&=0x7fu;
        esi=ebx;
        esi=esi+esi*2u;
        {
            const bool unused=x.w16(edi+esi*8u+2u)==0xffffu;
            esi=edi+esi*8u;
            if(unused) goto dry;
        }
        if((eax&0x80u)==0u) goto wet;
        x1=x.ld(esi+0xcu);
        x1=_mm_mul_ss(x1,x.ld(esp+0x30u));
        x0=x.ld(esi+8u);
        x0=_mm_mul_ss(x0,x.ld(esp+0x2cu));
        x1=_mm_add_ss(x1,x0);
        x0=x.ld(esi+4u);
        x0=_mm_mul_ss(x0,x.ld(esp+0x28u));
        x1=_mm_add_ss(x1,x0);
        x1=_mm_sub_ss(x1,x.ld(esi+0x10u));
        x0=_mm_setzero_ps();
        if(comiss(x0,x1).above())lo8(edx,1u);
        goto record;
    wet:
        lo8(edx,1u);
    dry:
        x0=_mm_setzero_ps();
    record:
        esi=x.d(esp+0x10u);
        x7=single(one);
        edi=x.d(esp+0x1cu);
        x.st8(0x12a4c2u,ebp-0x19u,edx);
        edx=ebp-0x14u;
        eax=edx;
        x.st(0x12a4cau,eax,esi);                 // forward
        esi=x.d(esp+0x14u);
        x.st(0x12a4d0u,eax+4u,esi);
        esi=x.d(esp+0x18u);
        x.st(0x12a4d7u,eax+8u,esi);
        x.st(0x12a4dau,ebp-0x18u,x7);            // scale
        // left = up x forward
        x2=x4;
        x2=_mm_mul_ss(x2,x3);
        x1=x6;
        x1=_mm_mul_ss(x1,x5);
        x1=_mm_sub_ss(x1,x2);
        x2=x.ld(esp+0x1cu);
        x.st(0x12a4f7u,ebp-8u,x1);
        x1=x.ld(esp+0x10u);
        x4=_mm_mul_ss(x4,x1);
        x3=_mm_mul_ss(x3,x2);
        x5=_mm_mul_ss(x5,x2);
        x2=x.ld(esp+0x58u);
        x6=_mm_mul_ss(x6,x1);
        esi=ebp+4u;
        x4=_mm_sub_ss(x4,x5);
        x.st(0x12a51fu,ebp-4u,x4);
        x4=x.ld(esp+0x54u);
        x3=_mm_sub_ss(x3,x6);
        x.st(0x12a52eu,ebp,x3);
        x3=x.ld(esp+0x5cu);
        eax=esi;
        x.st(0x12a53bu,eax,edi);                 // up
        edi=x.d(esp+0x20u);
        x.st(0x12a541u,eax+4u,edi);
        edi=x.d(esp+0x24u);
        x.st(0x12a548u,eax+8u,edi);
        eax=x.d(esp+0x28u);
        x.st(0x12a54fu,ebp+0x10u,x0);
        x.st(0x12a554u,ebp+0x14u,x0);
        x.st(0x12a559u,ebp+0x18u,x0);
        x.st(0x12a55eu,ebp+0x10u,eax);           // position
        eax=x.d(esp+0x2cu);
        x.st(0x12a565u,ebp+0x14u,eax);
        eax=x.d(esp+0x30u);
        x.st(0x12a56cu,ebp+0x18u,eax);
        // The velocity, divided by the scale unless it is 1, in the basis.
        x0=x.ld(ebp-0x18u);
        {
            const auto unit=comiss(x0,x7,true);
            eax=(eax&0xffff00ffu)|(unit.lahf()<<8);
            x1=x4;
            if(!unit.ordered_equal()) {
                x0=x7;
                x0=_mm_div_ss(x0,x.ld(ebp-0x18u));
                x1=x0;
                x1=_mm_mul_ss(x1,x4);
                x2=_mm_mul_ss(x2,x0);
                x3=_mm_mul_ss(x3,x0);
            }
        }
        x0=x.ld(ebp-0x10u);
        x4=x.ld(ebp-0xcu);
        x0=_mm_mul_ss(x0,x2);
        edi=x.d(esp+0x64u);
        x4=_mm_mul_ss(x4,x3);
        x0=_mm_add_ss(x0,x4);
        x4=x1;
        x4=_mm_mul_ss(x4,x.ld(edx));
        edx=x.d(esp+0x3cu);
        x0=_mm_add_ss(x0,x4);
        x.st(0x12a5c0u,ebp+0x1cu,x0);
        x0=x.ld(ebp-8u);
        x4=x.ld(ebp-4u);
        x4=_mm_mul_ss(x4,x2);
        x0=_mm_mul_ss(x0,x1);
        x0=_mm_add_ss(x0,x4);
        x4=x.ld(ebp);
        x4=_mm_mul_ss(x4,x3);
        x0=_mm_add_ss(x0,x4);
        x.st(0x12a5e8u,ebp+0x20u,x0);
        x0=x.ld(ebp+8u);
        x0=_mm_mul_ss(x0,x2);
        x2=x.ld(ebp+0xcu);
        x2=_mm_mul_ss(x2,x3);
        x0=_mm_add_ss(x0,x2);
        x2=x.ld(esi);
        x2=_mm_mul_ss(x2,x1);
        x0=_mm_add_ss(x0,x2);
        x.st(0x12a60fu,ebp+0x24u,x0);
        x.st(0x12a614u,ebp-0x20u,edx);           // location
        edx=x.d(sound_state);
        x.st16(0x12a61du,ebp-0x1cu,ecx);         // cluster
        goto next;
    no_player:
        x.st(0x12a623u,ebp-0x20u,esi);
        x.st16(0x12a626u,ebp-0x1cu,esi);
    next:
        eax=x.d(esp+0x44u);
        ecx=x.d(esp+0x48u);
        eax+=4u;
        x.st(0x12a635u,esp+0x44u,eax);
        eax=x.d(esp+0x38u);
        eax+=0x358u;
        ++edi;
        ecx+=0x358u;
        ebp+=0x48u;
        x.st(0x12a651u,esp+0x64u,edi);
        x.st(0x12a655u,esp+0x48u,ecx);
        x.st(0x12a659u,esp+0x38u,eax);
    } while(less(eax,0x4f72bcu));
    lo8(eax,x.b(esp+0xfcu));                     // arg0
    esi=x.pop();
    ebp=x.pop();
    ebx=x.pop();
    if((eax&0xffu)==0u) goto done;
    // The DSP listener block (at ESP+0xB4 once pushed): the two
    // environments (0x18 each at ESP+0x84), then the camera listener.
    eax=x.d(edx+0x1b0u);
    if(eax==none)eax=default_environment;
    else {
        ecx=x.d(tag_instances);
        eax&=0xffffu;
        eax<<=4;
        eax=x.d(eax+ecx+8u);
    }
    x0=x.ld(edx+0x1acu);
    x.st(0x12a6a1u,esp+0x84u,eax);
    eax=edx+0x1b4u;
    ecx=x.d(eax);
    x.st(0x12a6b0u,esp+0x88u,ecx);
    ecx=x.d(eax+4u);
    x.st(0x12a6bau,esp+0x8cu,ecx);
    ecx=x.d(eax+8u);
    eax=x.d(eax+0xcu);
    x.st(0x12a6c7u,esp+0x94u,eax);
    eax=x.d(edx+0x1ccu);
    x.st(0x12a6d7u,esp+0x90u,ecx);
    x.st(0x12a6deu,esp+0x98u,x0);
    if(eax==none)eax=default_environment;
    else {
        ecx=x.d(tag_instances);
        eax&=0xffffu;
        eax<<=4;
        eax=x.d(eax+ecx+8u);
    }
    x0=x.ld(edx+0x1c8u);
    x.st(0x12a70au,esp+0x9cu,eax);
    eax=edx+0x1d0u;
    ecx=x.d(eax);
    x.st(0x12a719u,esp+0xa0u,ecx);
    ecx=x.d(eax+4u);
    x.st(0x12a723u,esp+0xa4u,ecx);
    ecx=x.d(eax+8u);
    eax=x.d(eax+0xcu);
    x.st(0x12a730u,esp+0xa8u,ecx);
    ecx=x.d(listener_forward);
    edx=x.d(ecx);
    x.st(0x12a73fu,esp+0xacu,eax);
    eax=x.d(ecx+4u);
    ecx=x.d(ecx+8u);
    x.st(0x12a74cu,esp+0xc0u,edx);
    edx=x.d(listener_up);
    x.st(0x12a759u,esp+0xc4u,eax);
    eax=x.d(edx);
    x.st(0x12a762u,esp+0xc8u,ecx);
    ecx=x.d(edx+4u);
    edx=x.d(edx+8u);
    x.st(0x12a76fu,esp+0xccu,eax);
    eax=x.d(listener_position);
    x.st(0x12a77bu,esp+0xd0u,ecx);
    ecx=x.d(eax);
    x.st(0x12a784u,esp+0xd4u,edx);
    edx=x.d(eax+4u);
    eax=x.d(eax+8u);
    x.st(0x12a791u,esp+0xb4u,ecx);
    ecx=x.d(listener_velocity);
    x.st(0x12a79eu,esp+0xb8u,edx);
    edx=x.d(ecx);
    x.st(0x12a7a7u,esp+0xbcu,eax);
    eax=x.d(ecx+4u);
    ecx=x.d(ecx+8u);
    x.st(0x12a7b4u,esp+0xdcu,eax);
    x.st(0x12a7bbu,esp+0xd8u,edx);
    eax=esp+0xb4u;
    edx=esp+0x84u;
    x.push(0x12a7d0u,eax);
    x.st(0x12a7d1u,esp+0xb4u,x0);
    x.st(0x12a7dau,esp+0xe4u,ecx);
    x.st(0x12a7e1u,esp+0xe8u,2u);                // environment count
    x.st(0x12a7ecu,esp+0xecu,edx);               // environments
    x.call(0x12a7f3u,0x2271a0u);                 // (ret 4)
done:
    edi=x.pop();
    esp+=0xe8u;
    x.ret(4);
}

// =====================================================================
// 221AF0: the class channel mixer. Stack arg0 (byte): the sound state's
// frame byte ([4F2C50] +0x7E) (permutation gains fade and surround routing
// happen only on such a frame). Walks the live class channels ([50EBE8]:
// 16EA60 then the used bitmap). A channel whose frame (+0x53) is stale and
// that is stopping (+0x50 bit 1 and no +0x51 hold) stops through 221A80.
// Otherwise, for each of the channel's permutation slots (definition +0x1C
// count; slot 0x14 apart at +0x88, sound row 0x58 apart):
//  - its slot +0xE clears, and unless the channel is paused (+0x52 == 2):
//  - a playing slot's gain (+0x10) moves toward the channel's (+0x10) by at
//    most the frame's rate ([4F2C50] +0x1E0) times the playback row's
//    fade (+0xC), or snaps to it when that fade is below 0.0001;
//  - on a frame, its gain modifier interpolates the sound's scale bounds
//    (gestalt +0xC, definition +0xB) and offsets by the slot's +0x90;
//    the looping channel (+0x54, [52D668]) reseeds when unclaimed; 21F960
//    picks the pitch range. A surround sound (definition +1 bit 5) routes
//    its permutations (220990 per channel) over the channel slots of its
//    tag (129E30 stops a stale slot, 222350 starts one, gain -64 dB to
//    start); any other sound restarts its slot (222280 / 222350, 129900
//    with 0.75) when the pitch range changed.
// Unless the channel is paused or detail-less (+0x50 bit 2), each detail
// sound (definition +0x24 count, +0x28 rows of 0x34) whose time (+0x58
// per detail) is due ([4F2C50] +0x84) and whose flags allow it (223670,
// 128A80) gets its next time: min + (max - min) * random/65535 seconds,
// in ms (32CC0C). ret 4.
// =====================================================================
void r_221af0(CampaignContext &c,Abi &a) {
    Cpu x(c,a);
    auto &eax=x.eax,&ecx=x.ecx,&edx=x.edx,&ebx=x.ebx,&esp=x.esp,&ebp=x.ebp,&esi=x.esi,&edi=x.edi;
    auto &x0=x.x0,&x1=x.x1,&x2=x.x2;
    esp-=0x128u;
    x.push(0x221af6u,esi);
    esi=x.d(class_channels);
    x.push(0x221afdu,edi);
    edx=0u;
    ecx=esi;
    x.call(0x221b02u,0x16ea60u);                 // the first live channel's index
    ecx=none;
    if(eax!=none) {
        ecx=x.d(esi+0x24u);
        edx=x.d(esi+0x44u);
        ecx*=eax;
        ecx=sx16(x.w16(ecx+edx));                // its salt
        ecx<<=16;
        ecx|=eax;
    }
    edi=ecx;
    x.st(0x221b26u,esp+0x14u,edi);
    if(ecx==none) goto leave;
    x.push(0x221b30u,ebx);
    x.push(0x221b31u,ebp);
    goto channel;
next_channel:
    edi=x.d(esp+0x1cu);
    esi=x.d(class_channels);
channel:
    edx=x.d(esi+0x44u);
    lo8(ecx,x.b(esp+0x13cu));                    // arg0
    eax=edi;
    eax&=0xffffu;
    x.st(0x221b51u,esp+0x44u,eax);               // the channel's index
    eax*=0xd4u;
    eax+=edx;
    x.st(0x221b5fu,esp+0x14u,eax);               // the channel
    if((ecx&0xffu)!=0u) {
        edx=x.d(sound_state);
        lo8(ecx,x.b(eax+0x53u));
        if((ecx&0xffu)!=x.b(edx+0x7fu)) {
            if((x.b(eax+0x50u)&2u)!=0u) {
                lo8(ecx,x.b(eax+0x51u));
                if((ecx&0xffu)!=0u) goto mix;
            }
            x.call(0x221b80u,0x221a80u);         // stop the channel (EAX)
            goto advance;
        }
    }
mix:
    eax=x.d(eax+4u);
    ecx=x.d(tag_instances);
    eax&=0xffffu;
    eax<<=4;
    eax=x.d(eax+ecx+8u);                         // the channel's definition
    ecx=x.d(eax+0x1cu);
    x.st(0x221ba4u,esp+0x2cu,eax);
    x.st(0x221ba8u,esp+0x24u,0u);                // permutation index
    if(positive(ecx)) {
        edx=x.d(esp+0x14u);
        edx+=0x88u;
        x.st(0x221bc0u,esp+0x18u,0u);            // row offset (0x58 each)
        x.st(0x221bc8u,esp+0x10u,edx);           // the permutation's slot
        do {
            eax=x.d(esp+0x2cu);
            eax=x.d(eax+0x20u);
            ecx=x.d(esp+0x18u);
            edx=x.d(esp+0x14u);
            eax+=ecx;
            ecx=x.d(esp+0x10u);
            x.st(0x221be5u,esp+0x28u,eax);       // the permutation's row
            x.st8(0x221be9u,ecx+0xeu,0u);
            if(x.b(edx+0x52u)==2u) goto next_permutation;
            eax=x.d(ecx);
            if(eax!=none) {
                edx=x.d(permutation_slots);
                edx=x.d(edx+0x44u);
                eax&=0xffffu;
                eax*=0xbcu;
                eax=x.d(eax+edx+0xcu);
                edx=x.d(tag_instances);
                eax&=0xffffu;
                eax<<=4;
                eax=x.d(eax+edx+8u);
                eax=sx16(x.w16(eax+6u));
                edx=x.d(sound_gestalt);
                eax*=0x38u;
                eax+=x.d(edx+4u);                // the playback row
                // FLD [EAX+0xC]; FABS against 0.0001: JA (fades) or snaps.
                require_x87_room(a,2);
                x.fld(x.d(eax+0xcu));
                x.fabs0();
                if(x.compare_below(gain_epsilon).above()) {
                    edx=x.d(esp+0x14u);
                    eax=x.d(edx+0x10u);
                    x.st(0x221ca4u,ecx+0x10u,eax);
                } else {
                    edx=x.d(sound_state);
                    x0=x.ld(edx+0x1e0u);
                    x0=_mm_mul_ss(x0,x.ld(eax+0xcu));    // the step
                    eax=x.d(esp+0x14u);
                    x1=x.ld(eax+0x10u);
                    x2=_mm_setzero_ps();
                    x1=_mm_sub_ss(x1,x.ld(ecx+0x10u));   // the distance
                    x2=_mm_sub_ss(x2,x0);
                    if(comiss(x2,x1).above()) {
                        x0=x2;
                        x0=_mm_add_ss(x0,x.ld(ecx+0x10u));
                        x.st(0x221c82u,ecx+0x10u,x0);
                    } else {
                        if(!comiss(x1,x0).above())x0=x1;
                        x0=_mm_add_ss(x0,x.ld(ecx+0x10u));
                        x.st(0x221c96u,ecx+0x10u,x0);
                    }
                }
            }
            lo8(eax,x.b(esp+0x13cu));
            if((eax&0xffu)==0u) goto next_permutation;
            ecx=x.d(esp+0x10u);
            eax=x.d(ecx);
            if(eax==none) goto next_permutation;
            edx=x.d(permutation_slots);
            ebp=x.d(edx+0x44u);
            ecx=x.d(tag_instances);
            eax&=0xffffu;
            eax*=0xbcu;
            eax+=ebp;
            esi=eax;                             // the permutation slot record
            eax=x.d(esi+0xcu);
            ebx=x.d(esp+0x10u);
            x.st(0x221ceau,esp+0x3cu,eax);       // its sound tag
            eax&=0xffffu;
            eax<<=4;
            edi=x.d(eax+ecx+8u);                 // the sound definition
            eax=sx8(x.b(edi+0xbu));
            edx=eax+eax*4u;
            eax=x.d(sound_gestalt);
            ecx=x.d(eax+0xcu);
            eax=ecx+edx*4u+8u;                   // the scale's bounds
            edx=sx16(x.w16(eax));
            eax=sx16(x.w16(eax+2u));
            ecx=x.d(esp+0x14u);
            x0=_mm_cvtsi32_ss(x0,std::int32_t(edx));
            x1=_mm_cvtsi32_ss(x1,std::int32_t(eax));
            eax=x.d(ecx+0x54u);
            ecx=x.d(loop_channels);
            x1=_mm_sub_ss(x1,x0);
            x1=_mm_mul_ss(x1,x.ld(ebx+0x10u));
            x1=_mm_add_ss(x1,x.ld(esi+0x90u));
            x1=_mm_add_ss(x1,x0);
            x.st(0x221d41u,esp+0x40u,x1);        // the gain modifier
            if(eax!=none) {
                edx=x.d(loop_channel_array);
                ecx=x.d(edx+0x44u);
                eax&=0xffffu;
                eax*=0x1cu;
                eax+=ecx;
                ecx=eax;
            }
            if(x.d(ecx+4u)==none) {
                eax=x.d(ecx+8u);
                eax=next_seed(eax);
                x.st(0x221d72u,ecx+8u,eax);
            }
            lo16(ebp,sx8(x.b(esi+0x9cu)));       // MOVSX BP, the slot's channel
            ecx=x.d(ecx+8u);
            x.st(0x221d80u,esp+0x20u,ecx);       // the seed
            ecx=edi;
            eax=ebp;
            x.call(0x221d88u,0x21f960u);         // the pitch range
            {
                const bool surround=(x.b(edi+1u)&0x20u)!=0u;
                x.st(0x221d91u,esp+0x34u,eax);
                if(!surround) goto restart;
            }
            // Surround: the channel slots already playing this tag.
            ebx=x.d(ebx);
            ebp=x.d(esp+0x3cu);
            esi=0u;
            edx=none;
            x.st(0x221da6u,esp+0x50u,edx);
            x.st(0x221daau,esp+0x54u,edx);
            x.st(0x221daeu,esp+0x58u,edx);
            x.st(0x221db2u,esp+0x5cu,edx);
            x.st(0x221db6u,esp+0x60u,edx);
            eax=0xc2800000u;                     // -64 dB
            x.st(0x221dbfu,esp+0x64u,edx);
            x.st(0x221dc3u,esp+0x68u,edx);
            x.st(0x221dc7u,esp+0x6cu,edx);
            x.st(0x221dcbu,esp+0x70u,edx);
            edx=x.d(permutation_slots);
            x.st(0x221dd5u,esp+0x30u,esi);       // the matched channels' mask
            x.st(0x221dd9u,esp+0x74u,eax);
            x.st(0x221dddu,esp+0x78u,eax);
            x.st(0x221de1u,esp+0x7cu,eax);
            x.st(0x221de5u,esp+0x80u,eax);
            x.st(0x221decu,esp+0x84u,eax);
            x.st(0x221df3u,esp+0x88u,eax);
            x.st(0x221dfau,esp+0x8cu,eax);
            x.st(0x221e01u,esp+0x90u,eax);
            x.st(0x221e08u,esp+0x94u,eax);
            ecx=ebx;
            for(;;) {
                if(positive(esi) && ecx!=none) {
                    eax=x.d(edx+0x44u);
                    ecx&=0xffffu;
                    ecx*=0xbcu;
                    eax=ecx+eax+0x5cu;
                    ecx=x.d(eax+0x10u);          // the next slot in the chain
                    if(ecx==ebx)ecx=none;
                }
                ++esi;
                if(ecx==none)break;
                eax=ecx;
                eax&=0xffffu;
                eax*=0xbcu;
                eax+=x.d(edx+0x44u);
                if(x.d(eax+0xcu)!=ebp)continue;
                if(x.b(eax+2u)!=2u)continue;
                eax=sx8(x.b(eax+0x9cu));
                x.st(0x221e5fu,esp+0x50u+eax*4u,ecx);
            }
            // Each channel's gain (220990) and whether it is at its bound.
            eax=sx8(x.b(edi+0xau));
            esi=0u;
            x.st(0x221e6du,esp+0x38u,eax);       // the definition's channel count
            if(positive(eax)) {
                ebp=x.d(sound_gestalt);
                do {
                    eax=sx16(x.w16(edi+8u));
                    edx=x.d(ebp+0x24u);
                    eax+=esi;
                    ecx=eax+eax*2u;
                    eax=sx16(x.w16(edx+ecx*4u+2u));
                    edx=edx+ecx*4u;
                    ecx=x.d(ebp+0x1cu);
                    eax=eax+eax*4u;
                    ebx=ecx+eax*2u+2u;           // the channel's gain bounds
                    eax=x.d(esp+0x40u);
                    x.push(0x221ea2u,eax);
                    ecx=edi;
                    x.call(0x221ea5u,0x220990u); // (ret 4)
                    ecx=sx16(x.w16(ebx));
                    x1=x.ld(esp+0x40u);
                    x0=_mm_cvtsi32_ss(x0,std::int32_t(ecx));
                    {
                        const bool above=comiss(x0,x1).above();
                        x.st(0x221ebau,esp+0x74u+esi*4u,eax);
                        if(!above) {
                            edx=sx16(x.w16(ebx+2u));
                            x0=_mm_cvtsi32_ss(x0,std::int32_t(edx));
                            if(!comiss(x1,x0).above())x0=x1;
                        }
                    }
                    {
                        const auto clamped=comiss(x0,x1,true);
                        eax=(eax&0xffff00ffu)|(clamped.lahf()<<8);
                        if(clamped.ordered_equal()) {
                            ecx=esi;
                            eax=1u;
                            eax<<=ecx&31u;
                            x.st(0x221ee2u,esp+0x30u,x.d(esp+0x30u)|eax);
                        }
                    }
                    eax=x.d(esp+0x38u);
                    ++esi;
                } while(less(esi,eax));
            }
            eax=sx8(x.b(edi+0xau));
            esi=0u;
            x.st(0x221ef7u,esp+0x38u,eax);
            if(!positive(eax)) goto next_permutation;
            ebp=x.d(esp+0x28u);
            ebx=none;
            do {
                eax=x.d(esp+0x30u);
                edi=1u;
                ecx=esi;
                edi<<=ecx&31u;
                edi&=eax;
                if(edi==0u) {
                    eax=x.d(esp+0x50u+esi*4u);
                    if(eax!=ebx) {
                        x.push(0x221f29u,8u);
                        x.call(0x221f2bu,0x129e30u);   // stop the slot (ret 4)
                        x.st(0x221f30u,esp+0x50u+esi*4u,ebx);
                    }
                }
                if(edi!=0u && x.d(esp+0x50u+esi*4u)==ebx) {
                    edx=x.d(esp+0x24u);
                    eax=x.d(esp+0x3cu);
                    ecx=esp+0x20u;
                    x.push(0x221f4au,ecx);
                    ecx=x.d(esp+0x20u);
                    x.push(0x221f4fu,2u);
                    x.push(0x221f51u,edx);
                    x.push(0x221f52u,esi);
                    x.push(0x221f53u,eax);
                    x.push(0x221f54u,ecx);
                    x.call(0x221f55u,0x222350u);       // start a slot (ret 24)
                    if(eax!=ebx) {
                        edx=sx16(x.w16(esp+0x34u));
                        if(edx==esi) {
                            ecx=x.d(esp+0x10u);
                            x.st(0x221f6bu,ecx,eax);
                        }
                        ecx=x.d(permutation_slots);
                        ecx=x.d(ecx+0x44u);
                        edx=eax;
                        edx&=0xffffu;
                        edx*=0xbcu;
                        x.st(0x221f84u,edx+ecx+0x94u,0xc2800000u);
                        x.st(0x221f8fu,esp+0x50u+esi*4u,eax);
                    }
                }
                ecx=x.d(esp+0x50u+esi*4u);
                if(ecx!=ebx) {
                    edx=x.d(permutation_slots);
                    edx=x.d(edx+0x44u);
                    edi=x.d(esp+0x74u+esi*4u);
                    x0=x.ld(ebp+8u);
                    eax=ecx;
                    eax&=0xffffu;
                    eax*=0xbcu;
                    eax+=edx;
                    ecx&=0xffffu;
                    ecx*=0xbcu;
                    x.st(0x221fc8u,esp+0x28u,edi);
                    x0=_mm_add_ss(x0,x.ld(esp+0x28u));
                    x.st(0x221fd2u,esp+0x4cu,x0);
                    edi=x.d(esp+0x4cu);
                    x.st(0x221fdcu,ecx+edx+0x94u,edi);
                    x.st8(0x221fe3u,eax+0x63u,0xffu);
                }
                eax=x.d(esp+0x38u);
                ++esi;
            } while(less(esi,eax));
            goto next_permutation;
        restart:
            // A sound on one slot: restarted when its pitch range changed.
            if(x.w16(esp+0x34u)==(ebp&0xffffu)) goto next_permutation;
            edi=sx16(x.w16(esp+0x34u));
            ebx=x.d(esi+0xcu);
            eax=x.d(esi+0x10u);
            ecx=x.d(esp+0x24u);
            x.push(0x22200fu,edi);
            x.call(0x222010u,0x222280u);         // (ret 4)
            ebp=eax;
            if(ebp==none) {
                ecx=x.d(esp+0x24u);
                eax=esp+0x20u;
                x.push(0x222024u,eax);
                eax=x.d(esi+0x10u);
                x.push(0x222028u,2u);
                x.push(0x22202au,ecx);
                x.push(0x22202bu,edi);
                x.push(0x22202cu,ebx);
                x.push(0x22202du,eax);
                x.call(0x22202eu,0x222350u);     // (ret 24)
                ebp=eax;
                if(ebp==none) goto next_permutation;
            }
            eax=x.d(permutation_slots);
            ecx=x.d(eax+0x44u);
            eax=x.d(esi+0x90u);
            edx=ebp;
            edx&=0xffffu;
            edx*=0xbcu;
            x.st(0x222056u,edx+ecx+0x90u,eax);
            ecx=x.d(esp+0x10u);
            edi=x.d(ecx);
            x.push(0x222063u,0x3f400000u);       // 0.75
            eax=ebp;
            ebx=1u;
            x.call(0x22206fu,0x129900u);         // (ret 4)
            edx=x.d(esp+0x10u);
            x.st(0x222078u,edx,ebp);
        next_permutation:
            eax=x.d(esp+0x24u);
            edi=x.d(esp+0x18u);
            esi=x.d(esp+0x10u);
            ecx=x.d(esp+0x2cu);
            edx=x.d(ecx+0x1cu);
            ++eax;
            edi+=0x58u;
            esi+=0x14u;
            x.st(0x222096u,esp+0x24u,eax);
            x.st(0x22209au,esp+0x18u,edi);
            x.st(0x22209eu,esp+0x10u,esi);
        } while(less(eax,edx));
    }
    // The channel's detail sounds.
    edi=x.d(esp+0x14u);
    if(x.b(edi+0x52u)==2u) goto advance;
    if((x.b(edi+0x50u)&4u)!=0u) goto advance;
    ecx=x.d(esp+0x2cu);
    eax=x.d(ecx+0x24u);
    edx=0u;
    x.st(0x2220cbu,esp+0x20u,edx);               // detail index
    if(!positive(eax)) goto advance;
    x.st(0x2220d5u,esp+0x18u,edx);               // detail offset (0x34 each)
    ebp=edi+0x58u;                               // the detail's next time
    for(bool first=true;;first=false) {
        if(!first)edx=0u;
        esi=x.d(ecx+0x28u);
        ecx=x.d(esp+0x18u);
        eax=x.d(esi+ecx+8u);
        esi+=ecx;                                // the detail row
        if(eax!=none) {
            ecx=x.d(sound_state);
            eax=x.d(ebp);
            if(less(eax,x.d(ecx+0x84u))) {
                eax=x.d(esi+0x18u);
                lo8(ecx,1u);
                bool allowed=true;
                if((eax&1u)!=0u && (x.b(edi+0x50u)&1u)!=0u)allowed=false;
                else if((eax&2u)!=0u && (x.b(edi+0x50u)&1u)==0u)allowed=false;
                if(allowed) {
                    eax=x.d(esp+0x1cu);
                    x.st16(0x22212eu,esp+0xa4u,edx);
                    edx=esp+0x98u;
                    x.push(0x22213du,edx);
                    x.push(0x22213eu,eax);
                    x.push(0x22213fu,esi);
                    x.call(0x222140u,0x223670u); // (ret 12)
                    if((eax&0xffu)!=0u) {
                        ebx=x.d(esi+8u);
                        ecx=esp+0x98u;
                        x.push(0x222157u,ecx);
                        edi=esp+0x4cu;
                        x.call(0x22215cu,0x128a80u);   // (ret 4)
                        if(eax!=none || x.d(esp+0x48u)==1u) {
                            // The next time: [ESI+0xC] + ([ESI+0x10] - [ESI+0xC]) *
                            // (seed >> 16)/65535 seconds, in ms after now.
                            ecx=x.d(random_globals_g);
                            require_x87_room(a,3);
                            x.fld(x.d(esi+0x10u));
                            eax=x.d(ecx+4u);
                            x.fld(x.d(esi+0xcu));
                            eax=next_seed(eax);
                            x.st(0x222187u,ecx+4u,eax);
                            eax>>=16;
                            x.st(0x22218fu,esp+0x28u,eax);
                            x.floating.push(static_cast<long double>(std::int32_t(x.d(esp+0x28u))));   // FILD
                            if(std::int32_t(eax)<0)x.st0()=add(x.st0(),static_cast<long double>(flt(two_to_32)));
                            x.st0()=mul(x.st0(),static_cast<long double>(random_word_scale));
                            edx=x.d(sound_state);
                            x.fxch(2);
                            x.st0()=sub(x.st0(),x.sti(1));
                            x.sti(2)=mul(x.sti(2),x.st0());x.floating.pop();   // FMULP ST(2),ST
                            x.fxch(1);
                            x.st0()=add(x.st0(),x.sti(1));
                            x.st0()=mul(x.st0(),static_cast<long double>(flt(thousand)));
                            {
                                std::uint16_t status;
                                x.st0()=integer_add(x.st0(),std::int32_t(x.d(edx+0x84u)),status);
                                a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(status&0x200u));
                            }
                            x.call(0x2221c1u,0x32cc0cu);     // to an integer (EAX)
                            x.fstp0();
                            x.st(0x2221c8u,ebp,eax);
                        }
                        edi=x.d(esp+0x14u);
                    }
                }
            }
        }
        eax=x.d(esp+0x20u);
        esi=x.d(esp+0x18u);
        ecx=x.d(esp+0x2cu);
        edx=x.d(ecx+0x24u);
        ++eax;
        esi+=0x34u;
        ebp+=4u;
        x.st(0x2221e7u,esp+0x20u,eax);
        x.st(0x2221ebu,esp+0x18u,esi);
        if(!less(eax,edx))break;
    }
advance:
    // The next live channel after this one (the used bitmap).
    eax=x.d(esp+0x1cu);
    esi=none;
    if(eax==esi)eax=0u;
    else {
        eax=x.d(esp+0x44u);
        ++eax;
    }
    ebx=x.d(class_channels);
    if(!less(eax,0u)) {
        edi=x.d(ebx+0x38u);
        if(less(eax,edi)) {
            edx=x.d(ebx+0x48u);
            do {
                ecx=eax;
                ecx&=0x1fu;
                ebp=1u;
                ebp<<=ecx;
                ecx=eax;
                ecx=sar(ecx,5);
                if((x.d(edx+ecx*4u)&ebp)!=0u){esi=eax;break;}
                ++eax;
            } while(less(eax,edi));
        }
    }
    eax=none;
    if(esi!=none) {
        edx=x.d(ebx+0x24u);
        eax=x.d(ebx+0x44u);
        edx*=esi;
        eax=sx16(x.w16(edx+eax));
        eax<<=16;
        eax|=esi;
    }
    x.st(0x22225cu,esp+0x1cu,eax);
    if(eax!=none) goto next_channel;
    ebp=x.pop();
    ebx=x.pop();
leave:
    edi=x.pop();
    esi=x.pop();
    esp+=0x128u;
    x.ret(4);
}

constexpr CampaignRoutineEntry routines[]{
    {0x120060u,r_120060},{0x120310u,r_120310},{0x129f10u,r_129f10},{0x221af0u,r_221af0},{0x2271a0u,r_2271a0}};
} // namespace

std::span<const CampaignRoutineEntry> campaign_mix_routines(){return routines;}
} // namespace halo2::native_tu15_sound
