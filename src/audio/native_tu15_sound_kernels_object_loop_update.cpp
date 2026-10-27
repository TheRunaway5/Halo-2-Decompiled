// Native object looping-sound update routines: the update part of the 18EE50
// group (see native_tu15_sound_object_loop.hpp). These are the only lifted
// sound routines a multiplayer match runs every frame: 18E970 walks the
// active loop-sound slots and, per slot, updates the spatial bounds (129170),
// the obstruction/occlusion gain state (18DC70), the per-source mix for each
// listener (190A80) and the stereo pan/azimuth (18D960).
//
// Each routine is a faithful decompilation of the retail function: the same
// memory reads/stores (at their original instruction addresses), the same
// SSE scalar arithmetic and comparisons, and the same x87 operations under
// the caller's control word. Branch conditions are evaluated directly in
// C++ rather than through emulated EFLAGS. Calls to other routines (in or
// out of the group) go through object_loop_invoke exactly as the original
// CALL does. Retail .rdata constants (0x45xxxx/0x46xxxx) are read from the
// game image like any other float the original loads.
#include "audio/native_tu15_sound_object_loop.hpp"

namespace halo2::native_tu15_sound {
namespace {

// --- SSE helpers -----------------------------------------------------------
inline __m128 ld_ss(const M &m,std::uint32_t p){return _mm_set_ss(f32(m,p));}
inline std::uint32_t fb(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}

// COMISS (ordered): cf = a<b (also set on unordered), zf = a==b. Exactly the
// predicate the lifted scalar_compare(a,b,false) produces.
struct Cmp{bool cf,zf;};
inline Cmp comi(__m128 a,__m128 b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char cfb,zfb;
    asm volatile("comiss %3,%2\n\tsetb %0\n\tsete %1":"=qm"(cfb),"=qm"(zfb):"x"(a),"x"(b):"cc");
    return {bool(cfb),bool(zfb)};
#else
    throw FormatError("Object loop scalar comparison backend unavailable");
#endif
}

// --- x87 helpers: arithmetic leaves C1 untouched (the original model only
// updates C1 on loads, spills and transcendentals), loads clear it. -------
inline long double fadd(long double a,long double b){volatile long double z=a+b;return z;}
inline long double fsub(long double a,long double b){volatile long double z=a-b;return z;}
inline long double fmul(long double a,long double b){volatile long double z=a*b;return z;}
inline long double fext(const M &m,std::uint32_t p){return static_cast<long double>(f32(m,p));}

long double x_fldl2e(){
    long double v;asm volatile("fldl2e":"=t"(v));return v;}
long double x_fldlg2(){
    long double v;asm volatile("fldlg2":"=t"(v));return v;}
long double x_fpatan(long double y,long double x){
    long double r;asm volatile("fpatan":"=t"(r):"0"(x),"u"(y):"st(1)");return r;}
long double x_fyl2x(long double y,long double x,std::uint16_t &st){
    long double r;asm volatile("fyl2x\n\tfnstsw %1":"=t"(r),"=m"(st):"0"(x),"u"(y):"st(1)");return r;}
long double x_f2xm1(long double x,std::uint16_t &st){
    long double r;asm volatile("f2xm1\n\tfnstsw %1":"=t"(r),"=m"(st):"0"(x));return r;}
long double x_fscale(long double x,long double e,std::uint16_t &st){
    long double r;asm volatile("fldt %3\n\tfldt %2\n\tfscale\n\tfnstsw %1\n\tfstpt %0\n\tfstp %%st(0)"
        :"=m"(r),"=m"(st):"m"(x),"m"(e):"st","st(1)");return r;}
long double x_frndint(long double x,std::uint16_t &st){
    long double r;asm volatile("frndint\n\tfnstsw %1":"=t"(r),"=m"(st):"0"(x));return r;}

// FCOMIP st(0) with other, then one pop (removes the compared top). Mirrors
// WorldState::compare_and_pop: ordered FCOMIP on the extended operands.
inline Cmp fcomip(Floating &fp,long double other){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const long double value=fp.a.x87[0];unsigned char cfb,zfb,pfb;
    asm volatile("fldt %4\n\tfldt %3\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tsete %1\n\tsetp %2\n\tfstp %%st(0)"
        :"=qm"(cfb),"=qm"(zfb),"=qm"(pfb):"m"(value),"m"(other):"cc","st");
    fp.pop();
    return {bool(cfb),bool(zfb)};
#else
    throw FormatError("Object loop extended comparison backend unavailable");
#endif
}
inline std::uint32_t sp(Floating &fp,long double x){return std::bit_cast<std::uint32_t>(fp.store_single(x));}

// Serialize/restore the floating-point environment across a call, exactly as
// WorldState::save()/load() do: before the call publish the live control
// word, MXCSR and x87 sticky flags into the Abi (the callee owns them); after
// it, re-establish the host environment the callee left in the Abi.
inline void fp_out(Abi &a){
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    a.floating_point=FP{cw,_mm_getcsr()};
    a.x87_status=std::uint16_t((a.x87_status&~0x3fu)|source_x87_exception_flags());
}
inline void fp_in(Abi &a){
    if(a.floating_point){const auto p=*a.floating_point;
        asm volatile("fldcw %0"::"m"(p.x87_control_word):"memory");_mm_setcsr(p.mxcsr);}
    source_load_x87_exception_flags(a.x87_status);
}
inline void setc1(Abi &a,std::uint16_t st){a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(st&0x200u));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline std::uint32_t sar5(std::uint32_t v){return std::uint32_t(std::int32_t(v)>>5);}
// A width-1/width-2 register write keeps the register's upper bytes, exactly
// as the original MOV AL/AX / SETcc does.
inline void setlo8(std::uint32_t &r,std::uint32_t v){r=(r&0xffffff00u)|(v&0xffu);}
inline void setlo16(std::uint32_t &r,std::uint32_t v){r=(r&0xffff0000u)|(v&0xffffu);}

// A CALL that leaves the group (or an unconverted group routine): publish the
// full register/XMM/FP state, push the return address and run the callee
// through object_loop_call (which reaches a native routine, a lifted one, or
// the live world), then reload XMM. GP and x87 remain in the Abi. Any stdcall
// stack arguments are pushed by the caller beforehand and popped by the callee.
inline void ext_call(LoopContext &c,Abi &a,__m128 xm[8],std::uint32_t ret,std::uint32_t target){
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    fp_out(a);
    a.gp[4]-=4u; store(c.m,{},ret,a.gp[4],ret);
    object_loop_call(c,a,target);
    fp_in(a);
    for(int i=0;i<8;++i) xm[i]=xmm(a,i);
}

// =====================================================================
// 18D960  real_sound_looping_update_pan_azimuth
// EAX: global looping-sound state G2=[4ECC18]; ECX: source index (-1 none);
// ESI: a 3x4 listener orientation matrix; BL: rear flag; stack: arg0 the
// source-relative position (3 floats), arg1 a reference distance, arg2 the
// output record (+0/+4 azimuth pair, +8/+0xC gains). Rotates the delta into
// the listener frame (14DD70 gives the range), derives the stereo azimuths
// (FPATAN) and an elevation blend (32CCD8), and writes the output. ret 12.
// =====================================================================
void r_18d960(LoopContext &c,Abi &a){
    M &m=c.m; const O &o=c.o;
    Floating floating(a,c.fp);
    __m128 xm[8]; for(int i=0;i<8;++i) xm[i]=xmm(a,i);
    const std::uint32_t E=a.gp[4];
    auto &g=a.gp;
    Cmp fl{};
    g[2]=u32(m,g[4]+4u);                         // EDX = arg0 (position)
    g[4]=E-0x14u;
    bool zf=(g[1]==0xffffffffu);                 // cmp ECX,-1
    g[4]-=4u; store(m,o,0x18d96au,g[4],g[5]);    // push EBP
    g[5]=u32(m,g[4]+0x24u);                       // EBP = arg2 (output)
    g[4]-=4u; store(m,o,0x18d96fu,g[4],g[7]);    // push EDI
    if(zf) goto L18dc0a;
    g[7]=u32(m,0x60u+g[0]);                        // EDI = G2+0x60 (source array)
    xm[0]=ld_ss(m,g[2]+0u);
    xm[7]=ld_ss(m,g[2]+4u);
    g[1]=g[1]+g[1]*8u;
    xm[0]=_mm_sub_ss(xm[0],ld_ss(m,0x8u+g[7]+g[1]*4u));
    xm[7]=_mm_sub_ss(xm[7],ld_ss(m,0xcu+g[7]+g[1]*4u));
    g[7]=0x0u+g[7]+g[1]*4u;
    store(m,o,0x18d994u,g[4]+0x10u,fb(xm[0]));
    xm[0]=ld_ss(m,g[2]+8u);
    xm[0]=_mm_sub_ss(xm[0],ld_ss(m,0x10u+g[7]));
    g[1]=g[7];
    store(m,o,0x18d9a6u,g[4]+0x18u,fb(xm[0]));
    ext_call(c,a,xm,0x18d9b1u,0x14dd70u);
    xm[4]=ld_ss(m,g[4]+0x18u);
    xm[5]=ld_ss(m,g[4]+0x10u);
    xm[1]=ld_ss(m,0x14u+g[6]);
    xm[2]=ld_ss(m,0x10u+g[6]);
    xm[3]=ld_ss(m,0x20u+g[6]);
    xm[6]=ld_ss(m,0x24u+g[6]);
    xm[2]=_mm_mul_ss(xm[2],xm[7]);
    xm[1]=_mm_mul_ss(xm[1],xm[4]);
    xm[1]=_mm_add_ss(xm[1],xm[2]);
    xm[2]=ld_ss(m,0xcu+g[6]);
    xm[2]=_mm_mul_ss(xm[2],xm[5]);
    xm[1]=_mm_add_ss(xm[1],xm[2]);
    xm[2]=ld_ss(m,0x18u+g[6]);
    xm[3]=_mm_mul_ss(xm[3],xm[4]);
    xm[2]=_mm_mul_ss(xm[2],xm[5]);
    xm[2]=_mm_add_ss(xm[2],xm[3]);
    xm[3]=ld_ss(m,0x1cu+g[6]);
    xm[3]=_mm_mul_ss(xm[3],xm[7]);
    xm[2]=_mm_add_ss(xm[2],xm[3]);
    xm[3]=ld_ss(m,0x28u+g[6]);
    xm[6]=_mm_mul_ss(xm[6],xm[5]);
    xm[5]=ld_ss(m,0x2cu+g[6]);
    xm[3]=_mm_mul_ss(xm[3],xm[7]);
    xm[5]=_mm_mul_ss(xm[5],xm[4]);
    xm[3]=_mm_add_ss(xm[3],xm[6]);
    xm[3]=_mm_add_ss(xm[3],xm[5]);
    xm[5]=ld_ss(m,g[4]+0x24u);                    // arg1
    fl=comi(xm[5],xm[0]);
    store(m,o,0x18da2fu,g[4]+8u,fb(xm[0]));
    store(m,o,0x18da35u,g[4]+0x10u,fb(xm[1]));
    store(m,o,0x18da3bu,g[4]+0x14u,fb(xm[2]));
    store(m,o,0x18da41u,g[4]+0x18u,fb(xm[3]));
    if(fl.cf||fl.zf) goto L18dc0a;
    if((g[3]&0xffu)==0u) goto L18da63;
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    xm[1]=_mm_sub_ss(xm[1],xm[0]);
    xm[0]=xm[1];
    store(m,o,0x18da5bu,g[4]+8u,fb(xm[0]));
    goto L18da8a;
L18da63:
    xm[4]=_mm_xor_ps(xm[4],xm[4]);
    xm[6]=xm[4];
    xm[6]=_mm_sub_ss(xm[6],xm[1]);
    xm[1]=xm[4];
    xm[1]=_mm_sub_ss(xm[1],xm[2]);
    xm[4]=_mm_sub_ss(xm[4],xm[3]);
    store(m,o,0x18da78u,g[4]+0x10u,fb(xm[6]));
    store(m,o,0x18da7eu,g[4]+0x14u,fb(xm[1]));
    store(m,o,0x18da84u,g[4]+0x18u,fb(xm[4]));
L18da8a:
    xm[1]=ld_ss(m,0x46a180u);                     // 0.001
    fl=comi(xm[5],xm[1]);
    if(!fl.cf&&!fl.zf) goto L18da9a;
    xm[5]=xm[1];
L18da9a:
    xm[1]=ld_ss(m,0x46a0dcu);                      // -1.0
    xm[0]=_mm_div_ss(xm[0],xm[5]);
    fl=comi(xm[1],xm[0]);
    store(m,o,0x18daa9u,g[4]+0x28u,fb(xm[0]));
    if(fl.cf||fl.zf) goto L18dabc;
    xm[0]=xm[1];
    store(m,o,0x18dab4u,g[4]+0x28u,fb(xm[0]));
    goto L18daec;
L18dabc:
    xm[2]=ld_ss(m,0x46a0d0u);                       // 1.0
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18dad4;
    xm[0]=xm[2];
    store(m,o,0x18daccu,g[4]+0x28u,fb(xm[0]));
    goto L18daec;
L18dad4:
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L18dae1;
    store(m,o,0x18dad9u,g[4]+0x20u,fb(xm[1]));
    goto L18daf2;
L18dae1:
    fl=comi(xm[0],xm[2]);
    store(m,o,0x18dae4u,g[4]+0x20u,fb(xm[2]));
    if(!fl.cf&&!fl.zf) goto L18daf2;
L18daec:
    store(m,o,0x18daecu,g[4]+0x20u,fb(xm[0]));
L18daf2:
    floating.push(fext(m,g[4]+0x20u));
    ext_call(c,a,xm,0x18dafbu,0x32ccd8u);
    zf=(g[3]&0xffu)==0u;
    store(m,o,0x18dafdu,g[4]+0x24u,sp(floating,a.x87[0])); floating.pop();
    floating.push(fext(m,0x14u+g[7]));
    floating.push(fext(m,g[4]+8u));
    a.x87[1]=x_fpatan(a.x87[1],a.x87[0]); floating.pop();
    store(m,o,0x18db0au,g[4]+8u,sp(floating,a.x87[0])); floating.pop();
    floating.push(fext(m,g[4]+0x14u));
    floating.push(fext(m,g[4]+0x10u));
    a.x87[1]=x_fpatan(a.x87[1],a.x87[0]); floating.pop();
    store(m,o,0x18db18u,g[4]+0xcu,sp(floating,a.x87[0])); floating.pop();
    if(zf) goto L18db26;
    xm[1]=ld_ss(m,g[4]+0x24u);
    goto L18db75;
L18db26:
    floating.push(fext(m,g[4]+0x28u));
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    a.x87[0]=fsub(fext(m,0x46a0d0u),a.x87[0]);
    store(m,o,0x18db32u,g[4]+0x20u,sp(floating,a.x87[0]));
    floating.push(fext(m,0x46a0e8u));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(fl.cf||fl.zf) goto L18db47;
    xm[0]=_mm_xor_ps(xm[0],xm[0]);
    goto L18db5d;
L18db47:
    xm[0]=ld_ss(m,g[4]+0x20u);
    xm[1]=ld_ss(m,0x46a0d0u);
    fl=comi(xm[0],xm[1]);
    if(fl.cf||fl.zf) goto L18db5d;
    xm[0]=xm[1];
L18db5d:
    xm[1]=ld_ss(m,g[4]+8u);
    xm[2]=ld_ss(m,g[4]+0x24u);
    fl=comi(xm[1],xm[2]);
    if(!fl.cf&&!fl.zf) goto L18db71;
    xm[1]=xm[2];
L18db71:
    xm[1]=_mm_mul_ss(xm[1],xm[0]);
L18db75:
    xm[2]=ld_ss(m,g[4]+0xcu);
    xm[0]=xm[2];
    xm[0]=_mm_sub_ss(xm[0],xm[1]);
    xm[1]=_mm_add_ss(xm[1],xm[2]);
    xm[2]=ld_ss(m,0x46a580u);                      // -2pi
    fl=comi(xm[2],xm[0]);
    xm[2]=ld_ss(m,0x44923cu);                       // 2pi
    store(m,o,0x18db99u,g[5]+0u,fb(xm[0]));
    store(m,o,0x18db9eu,g[5]+4u,fb(xm[1]));
    if(fl.cf||fl.zf) goto L18dbaf;
    xm[0]=_mm_add_ss(xm[0],xm[2]);
    xm[1]=_mm_add_ss(xm[1],xm[2]);
    goto L18dbbc;
L18dbaf:
    fl=comi(xm[1],xm[2]);
    if(fl.cf||fl.zf) goto L18dbc6;
    xm[0]=_mm_sub_ss(xm[0],xm[2]);
    xm[1]=_mm_sub_ss(xm[1],xm[2]);
L18dbbc:
    store(m,o,0x18dbbcu,g[5]+0u,fb(xm[0]));
    store(m,o,0x18dbc1u,g[5]+4u,fb(xm[1]));
L18dbc6:
    zf=(g[3]&0xffu)==0u;
    floating.push(fext(m,g[4]+0x28u));
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    if(zf) goto L18dbd8;
    a.x87[0]=fadd(a.x87[0],fext(m,0x46a0d0u));
    goto L18dbde;
L18dbd8:
    a.x87[0]=fsub(fext(m,0x46a0d0u),a.x87[0]);
L18dbde:
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a0ccu));
    g[7]=u32(m,g[4]); g[4]+=4u;                     // pop EDI
    store(m,o,0x18dbe5u,g[5]+0xcu,sp(floating,a.x87[0])); floating.pop();
    floating.push(fext(m,g[4]+0x14u));
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    floating.push(a.x87[0]);
    a.x87[0]=fmul(a.x87[0],fext(m,g[5]+0xcu));
    store(m,o,0x18dbf3u,g[5]+0xcu,sp(floating,a.x87[0])); floating.pop();
    floating.push(fext(m,0x46a0d0u));
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    store(m,o,0x18dbfeu,g[5]+8u,sp(floating,a.x87[0])); floating.pop();
    g[5]=u32(m,g[4]); g[4]+=4u;                     // pop EBP
    floating.clear_c1(); floating.pop();
    g[4]+=0x14u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a,12);
    return;
L18dc0a:
    zf=(g[3]&0xffu)==0u;
    xm[0]=ld_ss(m,0x44923cu);                       // 2pi
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    store(m,o,0x18dc17u,g[5]+4u,fb(xm[0]));
    xm[0]=ld_ss(m,0x46a0d0u);                        // 1.0
    store(m,o,0x18dc24u,g[5]+0u,fb(xm[1]));
    if(!zf) goto L18dc2e;
    xm[0]=xm[1];
L18dc2e:
    g[7]=u32(m,g[4]); g[4]+=4u;                     // pop EDI
    store(m,o,0x18dc2fu,g[5]+8u,fb(xm[0]));
    store(m,o,0x18dc34u,g[5]+0xcu,fb(xm[1]));
    g[5]=u32(m,g[4]); g[4]+=4u;                     // pop EBP
    g[4]+=0x14u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a,12);
}

// =====================================================================
// 129170  real_sound_looping_update_bounds
// EAX: a two-entry request array (each 0x1C bytes). Walks L=[4F2C50]'s two
// loop slots (L+0x1AC, 0x1C each; L+0x1E0 a rate), growing each slot's
// bounding box toward the request and decaying untouched slots. No returns
// used by callers; ret 0. Pure integer + SSE.
// =====================================================================
void r_129170(LoopContext &c,Abi &a){
    M &m=c.m; const O &o=c.o;
    Floating floating(a,c.fp);
    __m128 xm[8]; for(int i=0;i<8;++i) xm[i]=xmm(a,i);
    const std::uint32_t E=a.gp[4];
    auto &g=a.gp;
    Cmp fl{};
    g[4]=E-8u;
    xm[6]=ld_ss(m,0x46a180u);                   // 0.001
    xm[5]=ld_ss(m,0x46a2ecu);                   // pi
    xm[4]=_mm_xor_ps(xm[4],xm[4]);
    xm[7]=ld_ss(m,0x46a0d0u);                   // 1.0
    g[4]-=4u; store(m,o,0x12918eu,g[4],g[5]);   // push EBP
    g[5]=u32(m,0x4f2c50u);
    g[4]-=4u; store(m,o,0x129195u,g[4],g[6]);   // push ESI
    g[4]-=4u; store(m,o,0x129196u,g[4],g[7]);   // push EDI
    g[6]=g[0];                                  // ESI = request
    store(m,o,0x129199u,g[4]+0xCu,0u);          // [E-8] slot bitmap = 0
    g[6]+=8u;
    store(m,o,0x1291a4u,g[4]+0x10u,2u);         // [E-4] request counter = 2
    g[4]-=4u; store(m,o,0x1291acu,g[4],g[3]);   // push EBX

L1291b0:
    g[3]=u32(m,g[6]+0u);                         // EBX = request id
    if(g[3]==0xffffffffu) goto L12934e;
    g[7]=0xffffffffu;                            // EDI = chosen slot
    g[0]=0u;                                     // EAX = slot index
    g[2]=g[5]+0x1acu;                            // EDX = &slot[0]
L1291c6:
    g[1]=g[0]&0x1fu;
    g[5]=1u<<(g[1]&0x1fu);
    g[1]=g[0]>>5;
    if((u32(m,g[4]+0x10u+g[1]*4u)&g[5])!=0u) goto L1291e9;
    if(u32(m,g[2]+4u)==g[3]) goto L1291f4;
    fl=comi(xm[6],ld_ss(m,g[2]+0u));
    if(fl.cf) goto L1291e9;
    g[7]=g[0];
L1291e9:
    g[0]+=1u;
    g[2]+=0x1cu;
    if(std::int32_t(g[0])<2) goto L1291c6;
    goto L1291f6;
L1291f4:
    g[7]=g[0];
L1291f6:
    if(g[7]==0xffffffffu) goto L129348;
    xm[0]=ld_ss(m,g[6]+0xfffffff8u);
    fl=comi(xm[0],xm[6]);
    if(fl.cf||fl.zf) xm[0]=xm[6];                // xmm0 = max([ESI-8], 0.001)
    g[5]=u32(m,0x4f2c50u);
    xm[3]=ld_ss(m,0x1e0u+g[5]);
    g[2]=g[7]*0x1cu;
    fl=comi(xm[6],ld_ss(m,0x1acu+g[2]+g[5]));
    g[0]=0x1a8u+g[2]+g[5];                        // EAX = &slot[EDI] - 4
    xm[3]=_mm_div_ss(xm[3],xm[0]);
    if(fl.cf) goto L129244;
    g[1]=u32(m,g[6]+4u); store(m,o,0x129239u,0xcu+g[0],g[1]);
    g[2]=u32(m,g[6]+8u); store(m,o,0x12923fu,0x10u+g[0],g[2]);
    goto L1292a2;
L129244:
    xm[1]=ld_ss(m,g[6]+4u);
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0xcu+g[0]));
    xm[0]=xm[3];
    xm[0]=_mm_mul_ss(xm[0],xm[5]);
    xm[2]=xm[4];
    xm[2]=_mm_sub_ss(xm[2],xm[0]);
    fl=comi(xm[2],xm[1]);
    if(fl.cf||fl.zf) goto L129266;
    xm[1]=xm[2];
    goto L12926e;
L129266:
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L12926e;
    xm[1]=xm[0];
L12926e:
    xm[1]=_mm_add_ss(xm[1],ld_ss(m,0xcu+g[0]));
    store(m,o,0x129273u,0xcu+g[0],fb(xm[1]));
    xm[1]=ld_ss(m,g[6]+8u);
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0x10u+g[0]));
    fl=comi(xm[2],xm[1]);
    if(fl.cf||fl.zf) goto L12928c;
    xm[0]=xm[2];
    goto L129294;
L12928c:
    fl=comi(xm[1],xm[0]);
    if(!fl.cf&&!fl.zf) goto L129294;
    xm[0]=xm[1];
L129294:
    xm[1]=ld_ss(m,0x10u+g[0]);
    xm[1]=_mm_add_ss(xm[1],xm[0]);
    store(m,o,0x12929du,0x10u+g[0],fb(xm[1]));
L1292a2:
    xm[1]=ld_ss(m,g[6]+0x10u);
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0x18u+g[0]));
    xm[0]=xm[4];
    xm[0]=_mm_sub_ss(xm[0],xm[3]);
    fl=comi(xm[0],xm[1]);
    if(fl.cf||fl.zf) goto L1292bd;
    xm[1]=xm[0];
    goto L1292c5;
L1292bd:
    fl=comi(xm[1],xm[3]);
    if(fl.cf||fl.zf) goto L1292c5;
    xm[1]=xm[3];
L1292c5:
    xm[2]=ld_ss(m,0x18u+g[0]);
    xm[2]=_mm_add_ss(xm[2],xm[1]);
    store(m,o,0x1292ceu,0x18u+g[0],fb(xm[2]));
    xm[1]=ld_ss(m,g[6]+0xcu);
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0x14u+g[0]));
    fl=comi(xm[0],xm[1]);
    if(fl.cf||fl.zf) goto L1292e7;
    xm[1]=xm[0];
    goto L1292ef;
L1292e7:
    fl=comi(xm[1],xm[3]);
    if(fl.cf||fl.zf) goto L1292ef;
    xm[1]=xm[3];
L1292ef:
    xm[1]=_mm_add_ss(xm[1],ld_ss(m,0x14u+g[0]));
    store(m,o,0x1292f4u,0x14u+g[0],fb(xm[1]));
    xm[1]=xm[7];
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0x4u+g[0]));
    fl=comi(xm[0],xm[1]);
    if(!fl.cf&&!fl.zf) goto L129311;
    fl=comi(xm[1],xm[3]);
    xm[0]=xm[3];
    if(!fl.cf&&!fl.zf) goto L129311;
    xm[0]=xm[1];
L129311:
    xm[1]=ld_ss(m,0x4u+g[0]);
    xm[1]=_mm_add_ss(xm[1],xm[0]);
    store(m,o,0x12931au,0x4u+g[0],fb(xm[1]));
    g[1]=u32(m,g[6]+0xfffffff8u); store(m,o,0x129322u,0x0u+g[0],g[1]);
    g[2]=u32(m,g[6]+0u);          store(m,o,0x129326u,0x8u+g[0],g[2]);
    g[1]=g[7]&0x1fu;
    g[0]=g[7]>>5;
    g[2]=1u<<(g[1]&0x1fu);
    g[1]=u32(m,0x10u+g[4]+g[0]*4u);
    g[0]=0x10u+g[4]+g[0]*4u;
    g[1]=g[1]|g[2];
    store(m,o,0x129344u,g[0]+0u,g[1]);
    goto L12934e;
L129348:
    g[5]=u32(m,0x4f2c50u);
L12934e:
    g[0]=u32(m,g[4]+0x14u);
    g[6]+=0x1cu;
    g[0]-=1u;
    store(m,o,0x129356u,g[4]+0x14u,g[0]);
    if(g[0]!=0u) goto L1291b0;

    g[0]=0u;
    g[2]=g[5]+0x1acu;
    g[3]=u32(m,g[4]); g[4]+=4u;                   // pop EBX
L129370:
    g[1]=g[0]&0x1fu;
    g[6]=1u<<(g[1]&0x1fu);
    g[1]=g[0]>>5;
    if((u32(m,g[4]+0xcu+g[1]*4u)&g[6])!=0u) goto L1293d1;
    xm[0]=ld_ss(m,g[2]+0xfffffffcu);
    fl=comi(xm[0],xm[6]);
    if(fl.cf||fl.zf) goto L129396;
    xm[1]=xm[0];
    goto L129399;
L129396:
    xm[1]=xm[6];
L129399:
    xm[0]=ld_ss(m,0x1e0u+g[5]);
    xm[0]=_mm_div_ss(xm[0],xm[1]);
    xm[1]=xm[4];
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,g[2]+0u));
    xm[2]=xm[4];
    xm[2]=_mm_sub_ss(xm[2],xm[0]);
    fl=comi(xm[2],xm[1]);
    if(fl.cf||fl.zf) goto L1293bd;
    xm[0]=xm[2];
    goto L1293c5;
L1293bd:
    fl=comi(xm[1],xm[0]);
    if(!fl.cf&&!fl.zf) goto L1293c5;
    xm[0]=xm[1];
L1293c5:
    xm[1]=ld_ss(m,g[2]+0u);
    xm[1]=_mm_add_ss(xm[1],xm[0]);
    store(m,o,0x1293cdu,g[2]+0u,fb(xm[1]));
L1293d1:
    g[0]+=1u;
    g[2]+=0x1cu;
    if(std::int32_t(g[0])<2) goto L129370;

    g[7]=u32(m,g[4]); g[4]+=4u;                   // pop EDI
    g[6]=u32(m,g[4]); g[4]+=4u;                   // pop ESI
    g[5]=u32(m,g[4]); g[4]+=4u;                   // pop EBP
    g[4]+=8u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a);
}

// =====================================================================
// 190A80  real_sound_looping_update_source_mix
// Per looping-sound voice, for each listener cluster member, recomputes the
// per-source mix: range (14DD70), a listener/cluster match (253070), a
// distance->gain smoothstep and an exp10 gain curve, then commits the sorted
// source set (140910) and the gain of each. Stack args (ret 36): arg0 G2,
// arg1 voice index, arg2..arg8 pointers into the caller frame, arg3 FLT_MAX.
// =====================================================================
void r_190a80(LoopContext &c,Abi &a){
    M &m=c.m; const O &o=c.o;
    Floating floating(a,c.fp);
    __m128 xm[8]; for(int i=0;i<8;++i) xm[i]=xmm(a,i);
    const std::uint32_t E=a.gp[4];
    auto &g=a.gp;
    Cmp fl{}; std::uint16_t st;
    g[4]=E-0x428u;
    g[1]=u32(m,0x430u+g[4]);
    xm[0]=ld_ss(m,0x45695cu);
    g[4]-=4u; store(m,o,0x190a95u,g[4],g[3]);     // push EBX
    g[4]-=4u; store(m,o,0x190a96u,g[4],g[5]);     // push EBP
    g[5]=g[1];
    g[5]=g[5]*0xb0u;
    g[4]-=4u; store(m,o,0x190a9fu,g[4],g[6]);     // push ESI
    g[6]=u32(m,0x438u+g[4]);                       // ESI = arg0 (G2)
    g[2]=u32(m,0xa0u+g[6]);
    g[0]=0u;
    g[5]=g[5]+g[2];
    g[2]=0xffffffffu;
    bool zf=(u16(m,0x74u+g[5])==0xffffu);
    store(m,o,0x190ab8u,0x20u+g[4],g[0]);
    g[4]-=4u; store(m,o,0x190abcu,g[4],g[7]);     // push EDI
    store(m,o,0x190abdu,0x28u+g[4],g[0]);
    store(m,o,0x190ac1u,0x1cu+g[4],fb(xm[0]));
    if(zf) goto L190b00;
    g[0]=sx16(u16(m,0x74u+g[5]));
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    g[0]=g[0]<<2;
    store(m,o,0x190ad3u,0x338u+g[4]+g[0],g[1]);
    store(m,o,0x190adau,0x38u+g[4]+g[0],fb(xm[1]));
    g[1]=sx16(u16(m,0x74u+g[5]));
    store(m,o,0x190ae4u,0x138u+g[4]+g[0],g[2]);
    g[2]=g[1];
    g[2]=sar5(g[2]);
    g[0]=0x24u+g[4]+g[2]*4u;
    g[1]=g[1]&0x1fu;
    g[2]=1u;
    g[2]=1u<<(g[1]&0x1fu);
    store(m,o,0x190afeu,g[0]+0u,u32(m,g[0]+0u)|g[2]);
L190b00:
    g[0]=sx16(u16(m,0x7au+g[5]));
    g[1]=u32(m,0x154u+g[6]);
    g[0]=g[0]+g[0]*4u;
    g[2]=g[1]+g[0]*4u;
    g[0]=u32(m,0x4u+g[2]);
    store(m,o,0x190b15u,0x18u+g[4],g[2]);
    store(m,o,0x190b19u,0x14u+g[4],0u);
    if(std::int32_t(g[0])<=0) goto L190c90;
L190b30:
    g[1]=u32(m,0x14u+g[4]);
    g[0]=u32(m,0x8u+g[2]);
    g[0]=sx16(u16(m,0x0u+g[0]+g[1]*2u));
    g[6]=u32(m,0x60u+g[6]);
    g[1]=g[0];
    g[1]=g[1]&0x7fffu;
    g[3]=sx16(g[1]);
    g[0]=g[0]>>15;
    g[1]=g[3]+g[3]*8u;
    g[7]=0x0u+g[6]+g[1]*4u;
    g[0]=g[0]&0x1u;
    g[1]=sx16(u16(m,0x0u+g[7]+g[0]*2u));
    g[0]=u32(m,0x43cu+g[4]);
    g[6]=g[1];
    g[6]=g[6]*0xb0u;
    g[6]=g[6]+u32(m,0xa0u+g[0]);
    store(m,o,0x190b72u,0x20u+g[4],g[1]);
    if((u8(m,0x18u+g[7])&0x28u)!=0u) goto L190c6d;
    g[0]=u16(m,0x74u+g[6]);
    if((g[0]&0xffffu)==u16(m,0x74u+g[5])) goto L190c6d;
    g[0]=u32(m,0x440u+g[4]);
    g[2]=u32(m,0x43cu+g[4]);
    ext_call(c,a,xm,0x190b9du,0x253070u);
    g[2]=u32(m,0x444u+g[4]);
    g[1]=g[7];
    g[7]=u32(m,0x43cu+g[4]);
    g[0]=g[7];
    xm[7]=xm[0];
    ext_call(c,a,xm,0x190bb7u,0x14dd70u);
    xm[1]=ld_ss(m,0x1cu+g[4]);
    xm[0]=_mm_add_ss(xm[0],xm[7]);
    fl=comi(xm[1],xm[0]);
    if(fl.cf) goto L190be0;
    g[0]=u16(m,0x74u+g[5]);
    if((g[0]&0xffffu)==0xffffu) goto L190be0;
    g[1]=sx16(g[0]);
    store(m,o,0x190bd3u,0x138u+g[4]+g[1]*4u,g[3]);
    store(m,o,0x190bdau,0x1cu+g[4],fb(xm[0]));
L190be0:
    g[0]=u16(m,0x74u+g[6]);
    if((g[0]&0xffffu)==0xffffu) goto L190c69;
    g[1]=u32(m,0xe0u+g[7]);
    g[0]=sx16(g[0]);
    g[2]=g[0]+g[0]*8u;
    g[1]=0x0u+g[1]+g[2]*8u;
    if(g[1]==0u) goto L190c04;
    xm[1]=ld_ss(m,0x28u+g[1]);
    goto L190c0d;
L190c04:
    xm[1]=ld_ss(m,0x448u+g[4]);
L190c0d:
    fl=comi(xm[1],xm[7]);
    if(fl.cf) goto L190c69;
    g[1]=g[0];
    g[1]=g[1]&0x1fu;
    g[2]=1u;
    g[2]=1u<<(g[1]&0x1fu);
    g[1]=g[0];
    g[1]=sar5(g[1]);
    if((u32(m,0x24u+g[4]+g[1]*4u)&g[2])==0u) goto L190c2f;
    xm[1]=ld_ss(m,0x38u+g[4]+g[0]*4u);
L190c2f:
    fl=comi(xm[1],xm[0]);
    if(fl.cf) goto L190c69;
    g[2]=sx16(u16(m,0x74u+g[6]));
    g[0]=u32(m,0x20u+g[4]);
    store(m,o,0x190c3cu,0x38u+g[4]+g[2]*4u,fb(xm[0]));
    g[1]=sx16(u16(m,0x74u+g[6]));
    g[2]=g[1];
    g[2]=sar5(g[2]);
    store(m,o,0x190c4bu,0x338u+g[4]+g[1]*4u,g[0]);
    g[0]=0x24u+g[4]+g[2]*4u;
    store(m,o,0x190c56u,0x138u+g[4]+g[1]*4u,g[3]);
    g[1]=g[1]&0x1fu;
    g[2]=1u;
    g[2]=1u<<(g[1]&0x1fu);
    store(m,o,0x190c67u,g[0]+0u,u32(m,g[0]+0u)|g[2]);
L190c69:
    g[2]=u32(m,0x18u+g[4]);
L190c6d:
    g[0]=u32(m,0x14u+g[4]);
    g[1]=u32(m,0x4u+g[2]);
    g[6]=u32(m,0x43cu+g[4]);
    g[0]=g[0]+1u;
    store(m,o,0x190c7eu,0x14u+g[4],g[0]);
    if(std::int32_t(g[0])<std::int32_t(g[1])) goto L190b30;
    xm[0]=ld_ss(m,0x45695cu);
L190c90:
    g[0]=u32(m,0xdcu+g[6]);
    g[2]=0u;
    if(std::int32_t(g[0])<=0) goto L190cc9;
L190ca0:
    g[1]=g[2];
    g[1]=g[1]&0x1fu;
    g[7]=1u;
    g[7]=1u<<(g[1]&0x1fu);
    g[1]=g[2];
    g[1]=sar5(g[1]);
    (void)(u32(m,0x24u+g[4]+g[1]*4u)&g[7]);
    store(m,o,0x190cb5u,0x238u+g[4]+g[2]*4u,g[2]);
    if((u32(m,0x24u+g[4]+g[1]*4u)&g[7])!=0u) goto L190cc4;
    store(m,o,0x190cbeu,0x38u+g[4]+g[2]*4u,fb(xm[0]));
L190cc4:
    g[2]=g[2]+1u;
    if(std::int32_t(g[2])<std::int32_t(g[0])) goto L190ca0;
L190cc9:
    g[2]=0x38u+g[4];
    g[4]-=4u; store(m,o,0x190ccdu,g[4],g[2]);       // push &local(0x38)
    g[4]-=4u; store(m,o,0x190cceu,g[4],0x18dc40u);  // push comparator
    g[1]=0x34u+g[4];
    g[4]-=4u; store(m,o,0x190cd7u,g[4],g[1]);       // push &local(0x34)
    g[1]=0x244u+g[4];
    ext_call(c,a,xm,0x190ce4u,0x140910u);            // RET 0xC
    g[0]=u32(m,0x45cu+g[4]);
    if(g[0]==0u) goto L190cf5;
    store(m,o,0x190cefu,g[0]+0u,0u);
L190cf5:
    g[0]=u32(m,0xdcu+g[6]);
    store(m,o,0x190cfeu,0x30u+g[4],0u);
    if(std::int32_t(g[0])>=2) goto L190d6f;
    store(m,o,0x190d0au,0x20u+g[4],g[0]);
    if(std::int32_t(g[0])<=0) goto L190fc6;
L190d14:
    g[3]=u32(m,0x450u+g[4]);
    g[1]=u32(m,0x454u+g[4]);
    g[0]=u32(m,0x44cu+g[4]);
    xm[7]=ld_ss(m,0x46a180u);
    xm[4]=ld_ss(m,0x46a0d0u);
    xm[5]=ld_ss(m,0x46a0e4u);
    xm[6]=ld_ss(m,0x46a11cu);
    g[1]=g[1]-g[3];
    g[5]=0x238u+g[4];
    store(m,o,0x190d52u,0x1cu+g[4],g[1]);
    g[1]=u32(m,0x458u+g[4]);
    g[5]=g[5]-g[0];
    g[0]=g[0]-g[3];
    g[1]=g[1]-g[3];
    g[6]=g[3];
    store(m,o,0x190d65u,0x34u+g[4],g[0]);
    store(m,o,0x190d69u,0x14u+g[4],g[1]);
    goto L190d80;
L190d6f:
    store(m,o,0x190d6fu,0x20u+g[4],2u);
    goto L190d14;
L190d79:
    g[0]=u32(m,0x34u+g[4]);
    g[1]=g[1];
L190d80:
    g[0]=g[0]+g[6];
    g[2]=u32(m,0x0u+g[0]+g[5]*1u);
    g[1]=g[2];
    g[1]=g[1]&0x1fu;
    g[7]=1u;
    g[7]=1u<<(g[1]&0x1fu);
    g[1]=g[2];
    g[1]=sar5(g[1]);
    if((u32(m,0x24u+g[4]+g[1]*4u)&g[7])==0u) goto L190f67;
    store(m,o,0x190da2u,g[0]+0u,g[2]);
    if(g[3]==0u) goto L190f26;
    if(g[2]==0xffffffffu) goto L190f20;
    g[1]=u32(m,0x43cu+g[4]);
    g[1]=u32(m,0xe0u+g[1]);
    g[0]=g[2]+g[2]*8u;
    xm[0]=ld_ss(m,0x28u+g[1]+g[0]*8u);
    fl=comi(xm[0],xm[7]);
    g[0]=0x0u+g[1]+g[0]*8u;
    if(fl.cf||fl.zf) goto L190dd3;
    goto L190dd6;
L190dd3:
    xm[0]=xm[7];
L190dd6:
    xm[2]=ld_ss(m,0x38u+g[4]+g[2]*4u);
    xm[3]=_mm_xor_ps(xm[3],xm[3]);
    store(m,o,0x190ddfu,0x18u+g[4],fb(xm[0]));
    floating.push(fext(m,0x18u+g[4]));
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L190e10;
    xm[1]=xm[3];
    xm[2]=_mm_sub_ss(xm[2],xm[0]);
    xm[1]=_mm_sub_ss(xm[1],xm[0]);
    xm[2]=_mm_div_ss(xm[2],xm[1]);
    fl=comi(xm[3],xm[2]);
    if(fl.cf||fl.zf) goto L190e1f;
    xm[2]=xm[3];
    goto L190e36;
L190e10:
    fl=comi(xm[0],xm[2]);
    if(!fl.cf&&!fl.zf) goto L190e1a;
    xm[2]=_mm_xor_ps(xm[2],xm[2]);
    goto L190e24;
L190e1a:
    xm[2]=xm[4];
    goto L190e24;
L190e1f:
    fl=comi(xm[2],xm[4]);
    if(!fl.cf&&!fl.zf) goto L190e33;
L190e24:
    fl=comi(xm[3],xm[2]);
    if(fl.cf||fl.zf) goto L190e2e;
    xm[2]=xm[3];
    goto L190e36;
L190e2e:
    fl=comi(xm[2],xm[4]);
    if(fl.cf||fl.zf) goto L190e36;
L190e33:
    xm[2]=xm[4];
L190e36:
    xm[1]=ld_ss(m,0x455b0cu);
    store(m,o,0x190e3eu,0x10u+g[4],0xc2800000u);
    xm[0]=ld_ss(m,0x10u+g[4]);
    fl=comi(xm[1],xm[0]);
    if(!fl.cf&&!fl.zf) goto L190e5a;
    fl=comi(xm[0],xm[3]);
    if(fl.cf||fl.zf) goto L190e5a;
    g[0]=0u;
    goto L190e5f;
L190e5a:
    g[0]=0xc2800000u;
L190e5f:
    store(m,o,0x190e5fu,0x10u+g[4],g[0]);
    floating.push(fext(m,0x10u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a12cu));
    store(m,o,0x190e6du,0x10u+g[4],0u);
    fl=comi(xm[1],ld_ss(m,0x10u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x455b14u));
    floating.push(x_fldl2e());
    a.x87[1]=fmul(a.x87[1],a.x87[0]); floating.pop();
    floating.push(a.x87[0]);
    a.x87[0]=x_frndint(a.x87[0],st); setc1(a,st);
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    a.x87[0]=x_f2xm1(a.x87[0],st); setc1(a,st);
    floating.push(1.0L);
    a.x87[1]=fadd(a.x87[1],a.x87[0]); floating.pop();
    a.x87[0]=x_fscale(a.x87[0],a.x87[1],st); setc1(a,st);
    a.x87[1]=a.x87[0]; floating.clear_c1(); floating.pop();
    store(m,o,0x190e96u,0x2cu+g[4],sp(floating,a.x87[0])); floating.pop();
    if(fl.cf||fl.zf) goto L190ea3;
    g[0]=0xc2800000u;
    goto L190ea5;
L190ea3:
    g[0]=0u;
L190ea5:
    store(m,o,0x190ea5u,0x10u+g[4],g[0]);
    floating.push(fext(m,0x10u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a12cu));
    xm[0]=xm[2];
    xm[0]=_mm_mul_ss(xm[0],xm[2]);
    xm[1]=xm[0];
    a.x87[0]=fmul(a.x87[0],fext(m,0x455b14u));
    floating.push(x_fldl2e());
    xm[0]=_mm_mul_ss(xm[0],xm[2]);
    xm[1]=_mm_mul_ss(xm[1],xm[5]);
    xm[0]=_mm_mul_ss(xm[0],xm[6]);
    xm[1]=_mm_sub_ss(xm[1],xm[0]);
    fl=comi(xm[3],xm[1]);
    a.x87[1]=fmul(a.x87[1],a.x87[0]); floating.pop();
    floating.push(a.x87[0]);
    a.x87[0]=x_frndint(a.x87[0],st); setc1(a,st);
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    a.x87[0]=x_f2xm1(a.x87[0],st); setc1(a,st);
    floating.push(1.0L);
    a.x87[1]=fadd(a.x87[1],a.x87[0]); floating.pop();
    a.x87[0]=x_fscale(a.x87[0],a.x87[1],st); setc1(a,st);
    a.x87[1]=a.x87[0]; floating.clear_c1(); floating.pop();
    store(m,o,0x190eecu,0x10u+g[4],sp(floating,a.x87[0])); floating.pop();
    if(fl.cf||fl.zf) goto L190ef7;
    xm[1]=xm[3];
    goto L190eff;
L190ef7:
    fl=comi(xm[1],xm[4]);
    if(fl.cf||fl.zf) goto L190eff;
    xm[1]=xm[4];
L190eff:
    xm[2]=ld_ss(m,0x2cu+g[4]);
    xm[0]=ld_ss(m,0x10u+g[4]);
    xm[0]=_mm_sub_ss(xm[0],xm[2]);
    xm[0]=_mm_mul_ss(xm[0],xm[1]);
    xm[0]=_mm_add_ss(xm[0],xm[2]);
    ext_call(c,a,xm,0x190f1cu,0x220200u);
    store(m,o,0x190f1cu,0x0u+g[6],g[0]);
    goto L190f26;
L190f20:
    store(m,o,0x190f20u,0x0u+g[6],0xc2800000u);
L190f26:
    g[0]=u32(m,0x454u+g[4]);
    if(g[0]==0u) goto L190f3f;
    g[0]=u32(m,0x138u+g[4]+g[2]*4u);
    g[1]=u32(m,0x1cu+g[4]);
    store(m,o,0x190f3cu,0x0u+g[1]+g[6]*1u,g[0]);
L190f3f:
    g[0]=u32(m,0x458u+g[4]);
    if(g[0]==0u) goto L190f58;
    g[2]=u32(m,0x338u+g[4]+g[2]*4u);
    g[0]=u32(m,0x14u+g[4]);
    store(m,o,0x190f55u,0x0u+g[0]+g[6]*1u,g[2]);
L190f58:
    g[0]=u32(m,0x45cu+g[4]);
    if(g[0]==0u) goto L190fae;
    store(m,o,0x190f63u,g[0]+0u,u32(m,g[0]+0u)+1u);
    goto L190fae;
L190f67:
    g[1]=u32(m,0x45cu+g[4]);
    if(g[1]!=0u) goto L190fc6;
    (void)g[3];
    store(m,o,0x190f74u,g[0]+0u,0xffffffffu);
    if(g[3]==0u) goto L190f82;
    store(m,o,0x190f7cu,0x0u+g[6],0xc2800000u);
L190f82:
    g[0]=u32(m,0x454u+g[4]);
    if(g[0]==0u) goto L190f98;
    g[1]=u32(m,0x1cu+g[4]);
    store(m,o,0x190f91u,0x0u+g[1]+g[6]*1u,0xffffffffu);
L190f98:
    g[0]=u32(m,0x458u+g[4]);
    if(g[0]==0u) goto L190fae;
    g[2]=u32(m,0x14u+g[4]);
    store(m,o,0x190fa7u,0x0u+g[2]+g[6]*1u,0xffffffffu);
L190fae:
    g[0]=u32(m,0x30u+g[4]);
    g[1]=u32(m,0x20u+g[4]);
    g[0]=g[0]+1u;
    g[6]=g[6]+4u;
    store(m,o,0x190fbcu,0x30u+g[4],g[0]);
    if(std::int32_t(g[0])<std::int32_t(g[1])) goto L190d79;
L190fc6:
    g[7]=u32(m,g[4]); g[4]+=4u;                     // pop EDI
    g[6]=u32(m,g[4]); g[4]+=4u;                     // pop ESI
    g[5]=u32(m,g[4]); g[4]+=4u;                     // pop EBP
    g[3]=u32(m,g[4]); g[4]+=4u;                     // pop EBX
    g[4]+=0x428u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a,36);
}

// =====================================================================
// 18E970  real_sound_looping_update_frame
// The per-frame driver: for each active listener (records at [4F2C50]+0x88,
// 0x48 apart, up to index -72..0x120/0x48), builds the per-listener working
// set, recomputes obstruction/occlusion (18DC70), the source mix (190A80),
// the pan/azimuth (18D960) and the spatial bounds (129170), and starts or
// stops the platform voices (18CDD0). One stack arg; ret 4.
// =====================================================================
void r_18e970(LoopContext &c,Abi &a){
    M &m=c.m; const O &o=c.o;
    Floating floating(a,c.fp);
    __m128 xm[8]; for(int i=0;i<8;++i) xm[i]=xmm(a,i);
    const std::uint32_t E=a.gp[4];
    auto &g=a.gp;
    Cmp fl{};
    bool eq,src_eq,ecx_m1,cont,edi_m1,edi_m1b,a0,loop;
    g[4]=E-0x148u;
    g[4]-=4u; store(m,o,0x18e976u,g[4],g[3]);     // push EBX
    g[4]-=4u; store(m,o,0x18e977u,g[4],g[5]);     // push EBP
    g[5]=0xffffffffu;
    g[0]=g[5];
    store(m,o,0x18e97du,0x48u+g[4],g[0]);
    store(m,o,0x18e981u,0x4cu+g[4],g[0]);
    store(m,o,0x18e985u,0x50u+g[4],g[0]);
    g[4]-=4u; store(m,o,0x18e989u,g[4],g[6]);     // push ESI
    g[6]=u32(m,0x4ecc18u);
    store(m,o,0x18e990u,0x58u+g[4],g[0]);
    g[4]-=4u; store(m,o,0x18e994u,g[4],g[7]);     // push EDI
    g[3]=0u;
    g[7]=0u;
    g[0]=0xcu;
    g[1]=0x50u+g[4];
    g[0]=g[0]-g[1];
    store(m,o,0x18e9a4u,0x14u+g[4],g[6]);
    store(m,o,0x18e9a8u,0x18u+g[4],g[3]);
    store(m,o,0x18e9acu,0x1cu+g[4],g[7]);
    store(m,o,0x18e9b0u,0x48u+g[4],g[0]);
    goto L18e9c0;
L18e9b6:
    g[0]=u32(m,0x48u+g[4]);
    g[3]=g[3];
L18e9c0:
    if(g[7]==0xffffffb8u) goto L18ed54;
    g[1]=u32(m,0x4f54f0u);
    g[2]=0x0u+g[0]+g[3]*4u;
    g[2]=g[2]+g[1];
    setlo8(g[0],(u32(m,0x50u+g[4]+g[2]*1u)!=g[5])?1u:0u);    // SETNZ AL
    if((g[0]&0xffu)==0u) goto L18ed54;
    g[0]=0u;
    g[2]=g[5];
    g[1]=g[1]+0xcu;
    g[3]=0x0u+g[3];
L18e9f0:
    if(u32(m,0x0u+g[1])!=g[5]) goto L18e9ff;
    g[0]=g[0]+1u;
    g[1]=g[1]+0x4u;
    if(std::int32_t(g[0])<4) goto L18e9f0;
    goto L18ea01;
L18e9ff:
    g[2]=g[0];
L18ea01:
    eq=(g[3]==g[2]);
    g[0]=u32(m,0x4f2c50u);
    g[1]=0x88u+g[7]+g[0]*1u;
    store(m,o,0x18ea0fu,0x44u+g[4],g[1]);
    if(!eq) goto L18ec44;
    g[0]=u8(m,0x6u+g[1]);
    if((g[0]&0xffu)==0u) goto L18ec44;
    g[0]=u16(m,0x4u+g[1]);
    if((g[0]&0xffffu)==(g[5]&0xffffu)) goto L18ec44;
    g[1]=g[1]+0x30u;
    g[2]=u32(m,0x0u+g[1]);
    g[7]=sx16(g[0]);
    g[0]=u32(m,0x4u+g[1]);
    g[1]=u32(m,0x8u+g[1]);
    store(m,o,0x18ea3fu,0x38u+g[4],g[2]);
    g[2]=u32(m,0x15cu+g[4]);
    g[4]-=4u; store(m,o,0x18ea4au,g[4],g[2]);     // push arg
    g[4]-=4u; store(m,o,0x18ea4bu,g[4],g[5]);     // push EBP
    g[4]-=4u; store(m,o,0x18ea4cu,g[4],g[7]);     // push EDI
    store(m,o,0x18ea4du,0x48u+g[4],g[0]);
    g[0]=0x44u+g[4];
    g[4]-=4u; store(m,o,0x18ea55u,g[4],g[0]);     // push &local(0x44)
    g[4]-=4u; store(m,o,0x18ea56u,g[4],g[3]);     // push EBX
    g[4]-=4u; store(m,o,0x18ea57u,g[4],g[6]);     // push ESI
    store(m,o,0x18ea58u,0x64u+g[4],g[7]);
    store(m,o,0x18ea5cu,0x58u+g[4],g[1]);
    ext_call(c,a,xm,0x18ea65u,0x18dc70u);          // 6 args, RET 24
    if(g[7]==g[5]) goto L18eafd;
    g[1]=0x10u+g[4];
    g[4]-=4u; store(m,o,0x18ea71u,g[4],g[1]);
    g[2]=0x64u+g[4];
    g[4]-=4u; store(m,o,0x18ea76u,g[4],g[2]);
    g[0]=0x28u+g[4];
    g[4]-=4u; store(m,o,0x18ea7bu,g[4],g[0]);
    g[4]-=4u; store(m,o,0x18ea7cu,g[4],0x0u);
    g[1]=0x78u+g[4];
    g[4]-=4u; store(m,o,0x18ea82u,g[4],g[1]);
    g[4]-=4u; store(m,o,0x18ea83u,g[4],0x7f7fffffu);
    g[2]=0x50u+g[4];
    g[4]-=4u; store(m,o,0x18ea8cu,g[4],g[2]);
    g[4]-=4u; store(m,o,0x18ea8du,g[4],g[7]);
    g[4]-=4u; store(m,o,0x18ea8eu,g[4],g[6]);
    ext_call(c,a,xm,0x18ea94u,0x190a80u);          // 9 args, RET 36
    g[3]=u32(m,0x10u+g[4]);
    if(g[3]!=1u && std::int32_t(g[3])>=1) goto L18eaed;  // g3>1
    if(std::int32_t(g[3])>=2) goto L18eae1;              // unreachable here (g3<=1)
    g[2]=0x2u;
    g[2]=g[2]-g[3];
    g[2]=g[2]<<2;
    g[0]=0xffffffffu;
    g[1]=g[2];
    g[1]=g[1]>>2;
    g[7]=0x60u+g[4]+g[3]*4u;
    while(g[1]){store(m,o,0x18eab8u,g[7],g[0]);const auto d=(a.eflags&0x400u)?0xfffffffcu:4u;g[7]+=d;--g[1];}
    g[5]=0x20u+g[4]+g[3]*4u;
    g[1]=g[2];
    g[1]=g[1]>>2;
    g[0]=0xffffffffu;
    g[7]=g[5];
    while(g[1]){store(m,o,0x18eac8u,g[7],g[0]);const auto d=(a.eflags&0x400u)?0xfffffffcu:4u;g[7]+=d;--g[1];}
    g[6]=0x68u+g[4]+g[3]*4u;
    g[1]=g[2];
    g[1]=g[1]>>2;
    g[0]=0xffffffffu;
    g[7]=g[6];
    g[6]=u32(m,0x14u+g[4]);
    while(g[1]){store(m,o,0x18eadcu,g[7],g[0]);const auto d=(a.eflags&0x400u)?0xfffffffcu:4u;g[7]+=d;--g[1];}
    g[5]=0xffffffffu;
L18eae1:
    store(m,o,0x18eae1u,0x10u+g[4],0x2u);
    g[3]=u32(m,0x10u+g[4]);
L18eaed:
    if(u32(m,0x20u+g[4])!=g[5]) goto L18eb01;
    g[0]=u32(m,0x24u+g[4]);
    store(m,o,0x18eaf7u,0x20u+g[4],g[0]);
    goto L18eb01;
L18eafd:
    g[3]=u32(m,0x10u+g[4]);
L18eb01:
    g[0]=0x128u+g[4];
    g[1]=0x2u;
    g[1]=0x0u+g[1];
L18eb10:
    store(m,o,0x18eb10u,0x0u+g[0],g[5]);
    g[0]=g[0]+0x1cu;
    g[1]=g[1]-1u;
    if(g[1]!=0u) goto L18eb10;
    g[7]=0u;
    if(g[3]==0u||std::int32_t(g[3])<0) goto L18ec34;
    g[5]=0x128u+g[4];
    g[4]=0x0u+g[4];
L18eb30:
    g[0]=u32(m,0x68u+g[4]+g[7]*4u);
    g[1]=0xffffffffu;
    if(g[0]==0xffffffffu) goto L18eb49;
    g[2]=u32(m,0xe0u+g[6]);
    g[1]=0x0u+g[0]+g[0]*8u;
    g[1]=u32(m,0x24u+g[2]+g[1]*8u);
L18eb49:
    edi_m1=(g[0]==0xffffffffu);
    xm[0]=ld_ss(m,0x46a11cu);
    if(edi_m1) goto L18eb83;
    g[6]=u32(m,0x4ecc18u);
    g[6]=u32(m,0xe0u+g[6]);
    g[2]=0x0u+g[0]+g[0]*8u;
    floating.push(fext(m,0x2cu+g[6]+g[2]*8u));
    g[2]=0x0u+g[6]+g[2]*8u;
    g[6]=u32(m,0x14u+g[4]);
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18eb83;
    xm[0]=ld_ss(m,0x2cu+g[2]);
L18eb83:
    edi_m1b=(g[0]==0xffffffffu);
    store(m,o,0x18eb86u,0xfffffff8u+g[5],fb(xm[0]));
    xm[0]=ld_ss(m,0x46a0d0u);
    store(m,o,0x18eb93u,0x0u+g[5],g[1]);
    store(m,o,0x18eb96u,0xfffffffcu+g[5],fb(xm[0]));
    if(edi_m1b) goto L18ebd9;
    g[2]=u32(m,0x4cu+g[4]);
    g[1]=0x28u+g[4];
    g[4]-=4u; store(m,o,0x18eba5u,g[4],g[1]);     // push &local(0x28)
    src_eq=(g[2]==u32(m,0x64u+g[4]+g[7]*4u));  // note g[4] now after push
    g[1]=u32(m,0xe0u+g[6]);
    g[0]=0x0u+g[0]+g[0]*8u;
    g[2]=u32(m,0x28u+g[1]+g[0]*8u);
    g[1]=u32(m,0x24u+g[4]+g[7]*4u);
    g[4]-=4u; store(m,o,0x18ebbbu,g[4],g[2]);     // push
    g[0]=0x40u+g[4];
    g[4]-=4u; store(m,o,0x18ebc0u,g[4],g[0]);     // push &local(0x40)
    g[0]=g[6];
    g[6]=u32(m,0x50u+g[4]);
    setlo8(g[3],src_eq?1u:0u);                              // SETE BL from the cmp above
    ext_call(c,a,xm,0x18ebcfu,0x18d960u);          // 3 args, RET 12
    g[3]=u32(m,0x10u+g[4]);
    g[6]=u32(m,0x14u+g[4]);
    goto L18ec07;
L18ebd9:
    ecx_m1=(g[1]==0xffffffffu);
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    xm[2]=ld_ss(m,0x44923cu);
    store(m,o,0x18ebe7u,0x28u+g[4],fb(xm[1]));
    store(m,o,0x18ebedu,0x2cu+g[4],fb(xm[2]));
    store(m,o,0x18ebf3u,0x30u+g[4],fb(xm[1]));
    if(ecx_m1) goto L18ec01;
    store(m,o,0x18ebfbu,0x30u+g[4],fb(xm[0]));
L18ec01:
    store(m,o,0x18ec01u,0x34u+g[4],fb(xm[1]));
L18ec07:
    g[2]=u32(m,0x28u+g[4]);
    g[0]=u32(m,0x2cu+g[4]);
    g[1]=0x4u+g[5];
    store(m,o,0x18ec12u,0x0u+g[1],g[2]);
    g[2]=u32(m,0x30u+g[4]);
    store(m,o,0x18ec18u,0x4u+g[1],g[0]);
    g[0]=u32(m,0x34u+g[4]);
    g[7]=g[7]+1u;
    store(m,o,0x18ec20u,0x8u+g[1],g[2]);
    g[5]=g[5]+0x1cu;
    cont=std::int32_t(g[7])<std::int32_t(g[3]);
    store(m,o,0x18ec28u,0xcu+g[1],g[0]);
    if(cont) goto L18eb30;
    g[5]=0xffffffffu;
L18ec34:
    g[0]=0x120u+g[4];
    ext_call(c,a,xm,0x18ec40u,0x129170u);
    g[3]=u32(m,0x18u+g[4]);
L18ec44:
    g[7]=u32(m,0x50u+g[4]+g[3]*4u);
    g[1]=u32(m,0x4f9b58u);
    g[0]=u32(m,0x4u+g[1]+g[3]*8u);
    a0=(g[0]==g[7]);
    g[3]=0x4u+g[1]+g[3]*8u;
    if(a0) goto L18ed4c;
    if(g[0]==g[5]) goto L18ecdc;
    g[2]=u32(m,0x4f0414u);
    g[0]=g[0]&0xffffu;
    g[0]=g[0]<<4;
    g[0]=u32(m,0x8u+g[0]+g[2]*1u);
    g[0]=u32(m,0x5cu+g[0]);
    if(g[0]==g[5]) goto L18ecdc;
    xm[0]=ld_ss(m,0x46a0d0u);
    g[1]=0u;
    g[4]-=4u; store(m,o,0x18ec85u,g[4],g[0]);     // push tag
    g[0]=0xccu+g[4];
    store8(m,o,0x18ec8du,0xceu+g[4],0x0u);
    store16(m,o,0x18ec95u,0xccu+g[4],g[1]);
    store8(m,o,0x18ec9du,0xcfu+g[4],g[1]);
    store(m,o,0x18eca4u,0xd0u+g[4],fb(xm[0]));
    store(m,o,0x18ecadu,0xd4u+g[4],g[1]);
    store(m,o,0x18ecb4u,0x114u+g[4],g[5]);
    store(m,o,0x18ecbbu,0x110u+g[4],g[5]);
    store(m,o,0x18ecc2u,0x11cu+g[4],g[1]);
    store(m,o,0x18ecc9u,0x118u+g[4],g[1]);
    store(m,o,0x18ecd0u,0x120u+g[4],g[1]);
    ext_call(c,a,xm,0x18ecdcu,0x18cdd0u);          // 1 arg, RET 4
L18ecdc:
    if(g[7]==g[5]) goto L18ed4a;
    g[1]=u32(m,0x4f0414u);
    g[0]=g[7];
    g[0]=g[0]&0xffffu;
    g[0]=g[0]<<4;
    g[0]=u32(m,0x8u+g[0]+g[1]*1u);
    g[0]=u32(m,0x54u+g[0]);
    if(g[0]==g[5]) goto L18ed4a;
    xm[0]=ld_ss(m,0x46a0d0u);
    g[1]=0u;
    g[4]-=4u; store(m,o,0x18ed05u,g[4],g[0]);     // push tag
    g[0]=0x74u+g[4];
    store8(m,o,0x18ed0au,0x76u+g[4],0x0u);
    store16(m,o,0x18ed0fu,0x74u+g[4],g[1]);
    store8(m,o,0x18ed14u,0x77u+g[4],g[1]);
    store(m,o,0x18ed18u,0x78u+g[4],fb(xm[0]));
    store(m,o,0x18ed1eu,0x7cu+g[4],g[1]);
    store(m,o,0x18ed22u,0xbcu+g[4],g[5]);
    store(m,o,0x18ed29u,0xb8u+g[4],g[5]);
    store(m,o,0x18ed30u,0xc4u+g[4],g[1]);
    store(m,o,0x18ed37u,0xc0u+g[4],g[1]);
    store(m,o,0x18ed3eu,0xc8u+g[4],g[1]);
    ext_call(c,a,xm,0x18ed4au,0x18cdd0u);          // 1 arg, RET 4
L18ed4a:
    store(m,o,0x18ed4au,0x0u+g[3],g[7]);
L18ed4c:
    g[3]=u32(m,0x18u+g[4]);
    g[7]=u32(m,0x1cu+g[4]);
L18ed54:
    g[7]=g[7]+0x48u;
    g[3]=g[3]+1u;
    loop=std::int32_t(g[7])<0x120;
    store(m,o,0x18ed5eu,0x18u+g[4],g[3]);
    store(m,o,0x18ed62u,0x1cu+g[4],g[7]);
    if(loop) goto L18e9b6;
    g[7]=u32(m,g[4]); g[4]+=4u;                     // pop EDI
    g[6]=u32(m,g[4]); g[4]+=4u;                     // pop ESI
    g[5]=u32(m,g[4]); g[4]+=4u;                     // pop EBP
    g[3]=u32(m,g[4]); g[4]+=4u;                     // pop EBX
    g[4]+=0x148u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a,4);
}

// =====================================================================
// 18DC70  real_sound_looping_update_obstruction
// The heaviest per-frame routine: builds the active device/channel set for a
// listener (190520), computes each channel's distance gain (14DD70) and the
// obstruction/occlusion dB curve (log10 via FYL2X, exp10 via F2XM1/FSCALE),
// resolves source handles (16E6A0) and commits the per-channel mix. 6 stack
// args; ret 24.
// =====================================================================
void r_18dc70(LoopContext &c,Abi &a){
    M &m=c.m; const O &o=c.o;
    Floating floating(a,c.fp);
    __m128 xm[8]; for(int i=0;i<8;++i) xm[i]=xmm(a,i);
    const std::uint32_t E=a.gp[4];
    auto &g=a.gp;
    Cmp fl{}; std::uint16_t st;
    bool z,al,le1,eqg,eqg2,eqg3,be;
    g[4]=E-0x4e0u;
    g[0]=u32(m,0x4f0u+g[4]);
    g[4]-=4u; store(m,o,0x18dc7du,g[4],g[3]);     // push EBX
    g[4]-=4u; store(m,o,0x18dc7eu,g[4],g[5]);     // push EBP
    g[4]-=4u; store(m,o,0x18dc7fu,g[4],g[6]);     // push ESI
    g[6]=0u;
    z=(g[0]==0xffffffffu);
    g[4]-=4u; store(m,o,0x18dc85u,g[4],g[7]);     // push EDI
    store(m,o,0x18dc86u,0x88u+g[4],g[6]);
    store(m,o,0x18dc8du,0x28u+g[4],0xffffffffu);
    if(z) goto L18e11c;
    g[1]=0x10u+g[4];
    g[4]-=4u; store(m,o,0x18dc9fu,g[4],g[1]);
    g[2]=0x3f4u+g[4];
    g[4]-=4u; store(m,o,0x18dca7u,g[4],g[2]);
    g[1]=0x2f8u+g[4];
    g[4]-=4u; store(m,o,0x18dcafu,g[4],g[1]);
    g[2]=0xf8u+g[4];
    g[4]-=4u; store(m,o,0x18dcb7u,g[4],g[2]);
    g[2]=u32(m,0x50cu+g[4]);
    g[1]=0x200u+g[4];
    g[4]-=4u; store(m,o,0x18dcc6u,g[4],g[1]);
    g[4]-=4u; store(m,o,0x18dcc7u,g[4],0x41a00000u);
    g[4]-=4u; store(m,o,0x18dcccu,g[4],g[2]);
    g[4]-=4u; store(m,o,0x18dccdu,g[4],g[0]);
    g[0]=u32(m,0x514u+g[4]);
    g[4]-=4u; store(m,o,0x18dcd5u,g[4],g[0]);
    store(m,o,0x18dcd6u,0x34u+g[4],g[6]);
    ext_call(c,a,xm,0x18dcdfu,0x190520u);          // 9 args, RET 24
    g[0]=u32(m,0x10u+g[4]);
    g[7]=0u;
    store(m,o,0x18dce7u,0x34u+g[4],g[6]);
    store(m,o,0x18dcebu,0x14u+g[4],g[7]);
    if(std::int32_t(g[0])<=std::int32_t(g[6])) goto L18de53;
    xm[7]=ld_ss(m,0x47365cu);
    g[1]=0x0u+g[1];
L18dd00:
    if(g[6]==2u) goto L18de53;
    g[3]=u32(m,0x1f0u+g[4]+g[7]*4u);
    if(g[3]==0xffffffffu) goto L18de42;
    g[2]=u32(m,0x4f4u+g[4]);
    g[0]=u32(m,0xd8u+g[2]);
    g[1]=g[3];
    g[1]=g[1]*0x64u;
    g[5]=0x0u+g[1]+g[0]*1u;
    g[1]=u32(m,0x24u+g[5]);
    if(g[1]==0xffffffffu) goto L18de42;
    g[2]=g[2]&0xffffff00u;                          // XOR DL,DL (low byte 0)
    g[0]=0u;
    if(std::int32_t(g[6])<=0) goto L18dd67;
    g[7]=0x4cu+g[4];
L18dd46:
    if((g[2]&0xffu)!=0u) goto L18de3e;
    g[2]=(g[2]&0xffffff00u)|(u32(m,0x0u+g[7])==g[1]?1u:0u);  // CMP; SETE DL
    g[0]=g[0]+1u;
    g[7]=g[7]+0x28u;
    if(std::int32_t(g[0])<std::int32_t(g[6])) goto L18dd46;
    if((g[2]&0xffu)!=0u) goto L18de3e;
    g[7]=u32(m,0x14u+g[4]);
L18dd67:
    g[0]=u32(m,0x48u+g[5]);
    g[2]=u32(m,0x3f0u+g[4]+g[7]*4u);
    g[6]=g[6]+1u;
    z=((g[0]&0x1u)==0u);
    store(m,o,0x18dd74u,0x34u+g[4],g[6]);
    g[6]=0x0u+g[6]+g[6]*4u;
    g[6]=0x10u+g[4]+g[6]*8u;
    store(m,o,0x18dd7fu,0x14u+g[6],g[1]);
    g[1]=u32(m,0xecu+g[4]+g[7]*4u);
    store(m,o,0x18dd89u,0x10u+g[6],g[3]);
    store(m,o,0x18dd8cu,0x20u+g[6],g[1]);
    store(m,o,0x18dd8fu,0x18u+g[6],g[2]);
    if(z) goto L18dda8;
    g[1]=u32(m,0x4cu+g[5]);
    g[2]=u32(m,0x50u+g[5]);
    store(m,o,0x18dd9au,0x4u+g[6],g[1]);
    g[1]=u32(m,0x54u+g[5]);
    store(m,o,0x18dda0u,0x8u+g[6],g[2]);
    store(m,o,0x18dda3u,0xcu+g[6],g[1]);
    goto L18ddbf;
L18dda8:
    xm[0]=ld_ss(m,0x46a0d0u);
    store(m,o,0x18ddb0u,0x4u+g[6],fb(xm[0]));
    store(m,o,0x18ddb5u,0x8u+g[6],fb(xm[0]));
    store(m,o,0x18ddbau,0xcu+g[6],fb(xm[0]));
L18ddbf:
    z=((g[0]&0x8u)==0u);
    if(z) goto L18ddfb;
    xm[1]=ld_ss(m,0x4u+g[6]);
    g[0]=u32(m,0x515f00u);
    xm[0]=xm[7];
    xm[1]=_mm_mul_ss(xm[1],xm[0]);
    store(m,o,0x18ddd4u,0x4u+g[6],fb(xm[1]));
    xm[1]=ld_ss(m,0x8u+g[6]);
    z=(g[0]==0u);
    xm[1]=_mm_mul_ss(xm[1],xm[0]);
    xm[0]=_mm_mul_ss(xm[0],ld_ss(m,0xcu+g[6]));
    g[2]=z?1u:0u;
    store8(m,o,0x18ddecu,0x24u+g[6],g[2]);
    store(m,o,0x18ddefu,0x8u+g[6],fb(xm[1]));
    store(m,o,0x18ddf4u,0xcu+g[6],fb(xm[0]));
    goto L18ddff;
L18ddfb:
    store8(m,o,0x18ddfbu,0x24u+g[6],0x0u);
L18ddff:
    g[0]=u32(m,0x2f0u+g[4]+g[7]*4u);
    if(g[0]==0xffffffffu) goto L18de31;
    g[1]=0x0u+g[0]+g[0]*8u;
    g[0]=u32(m,0x4f4u+g[4]);
    g[2]=u32(m,0x60u+g[0]);
    g[1]=0x0u+g[2]+g[1]*4u;
    g[2]=u32(m,0x4fcu+g[4]);
    ext_call(c,a,xm,0x18de27u,0x14dd70u);
    store(m,o,0x18de27u,0x0u+g[6],fb(xm[0]));
    g[6]=u32(m,0x34u+g[4]);
    goto L18de42;
L18de31:
    xm[0]=_mm_xor_ps(xm[0],xm[0]);
    store(m,o,0x18de34u,0x0u+g[6],fb(xm[0]));
    g[6]=u32(m,0x34u+g[4]);
    goto L18de42;
L18de3e:
    g[7]=u32(m,0x14u+g[4]);
L18de42:
    g[0]=u32(m,0x10u+g[4]);
    g[7]=g[7]+1u;
    store(m,o,0x18de49u,0x14u+g[4],g[7]);
    if(std::int32_t(g[7])<std::int32_t(g[0])) goto L18dd00;
L18de53:
    g[0]=u32(m,0x500u+g[4]);
    g[2]=u32(m,0x3f0u+g[4]);
    g[1]=u32(m,0x10u+g[4]);
    al=(g[0]==g[2]);
    le1=(std::int32_t(g[1])<=1);
    store8(m,o,0x18de6du,0x1fu+g[4],al?1u:0u);
    if(le1) goto L18dec6;
    if(g[6]==0u) goto L18dec6;
    if(!al) goto L18dec6;
    g[5]=u32(m,0x48u+g[4]);
    g[1]=u32(m,0x4f4u+g[4]);
    g[5]=g[5]*0x64u;
    g[1]=u32(m,0xd8u+g[1]);
    g[2]=u32(m,0x10u+g[4]);
    g[0]=1u;
    g[5]=g[5]+g[1];
    if(std::int32_t(g[2])<=1) goto L18dec6;
    g[2]=u32(m,0x4cu+g[4]);
L18dea2:
    g[7]=u32(m,0x1f0u+g[4]+g[0]*4u);
    if(g[7]==0xffffffffu) goto L18debd;
    g[3]=g[7];
    g[3]=g[3]*0x64u;
    if(u32(m,0x24u+g[3]+g[1]*1u)==g[2]) goto L18df58;
L18debd:
    g[7]=u32(m,0x10u+g[4]);
    g[0]=g[0]+1u;
    if(std::int32_t(g[0])<std::int32_t(g[7])) goto L18dea2;
L18dec6:
    xm[5]=ld_ss(m,0x46a0d0u);
    g[3]=u32(m,0x4f4u+g[4]);
L18ded5:
    g[7]=0u;
    store(m,o,0x18ded9u,0x10u+g[4],g[6]);
    if(std::int32_t(g[6])<=0) goto L18e106;
    g[1]=u32(m,0xd8u+g[3]);
    g[2]=u32(m,0x4ecc18u);
    g[3]=u32(m,0xd8u+g[2]);
    xm[4]=ld_ss(m,0x46a0e4u);
    store(m,o,0x18defdu,0x18u+g[4],g[1]);
L18df01:
    g[2]=u32(m,0x18u+g[4]);
    g[0]=u32(m,0x88u+g[4]);
    g[1]=0x0u+g[7]+g[7]*4u;
    g[5]=u32(m,0x48u+g[4]+g[1]*8u);
    xm[0]=ld_ss(m,0x38u+g[4]+g[1]*8u);
    g[1]=0x38u+g[4]+g[1]*8u;
    g[6]=g[5];
    g[6]=g[6]*0x64u;
    floating.push(fext(m,0x44u+g[6]+g[2]*1u));
    store(m,o,0x18df26u,0x14u+g[4],sp(floating,a.x87[0]));
    a.x87[0]=-a.x87[0]; floating.clear_c1();
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18dff0;
    xm[0]=_mm_div_ss(xm[0],ld_ss(m,0x14u+g[4]));
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L18dffd;
    xm[0]=xm[1];
    goto L18e005;
L18df58:
    g[0]=u32(m,0x2f0u+g[4]+g[0]*4u);
    g[3]=u32(m,0x4f4u+g[4]);
    g[2]=0x0u+g[0]+g[0]*8u;
    g[0]=u32(m,0x60u+g[3]);
    g[1]=0x0u+g[0]+g[2]*4u;
    g[2]=u32(m,0x4fcu+g[4]);
    g[0]=g[3];
    ext_call(c,a,xm,0x18df7du,0x14dd70u);
    g[1]=u32(m,0x48u+g[5]);
    g[0]=g[1];
    g[0]=g[0]&0x4u;
    store(m,o,0x18df85u,0x38u+g[4],fb(xm[0]));
    if(g[0]!=0u) goto L18df96;
    if((g[1]&0x2u)==0u) goto L18dec6;
L18df96:
    g[5]=u32(m,0xd8u+g[3]);
    g[7]=g[7]*0x64u;
    xm[5]=ld_ss(m,0x46a0d0u);
    g[7]=g[7]+g[5];
    if(g[0]==0u) goto L18dfc3;
    if((u8(m,0x48u+g[7])&0x1u)==0u) goto L18dfba;
    xm[0]=ld_ss(m,0x4cu+g[7]);
    goto L18dfbd;
L18dfba:
    xm[0]=xm[5];
L18dfbd:
    store(m,o,0x18dfbdu,0x44u+g[4],fb(xm[0]));
L18dfc3:
    if((g[1]&0x2u)==0u) goto L18ded5;
    if((u8(m,0x48u+g[7])&0x1u)==0u) goto L18dfe2;
    xm[0]=ld_ss(m,0x4cu+g[7]);
    store(m,o,0x18dfd7u,0x40u+g[4],fb(xm[0]));
    goto L18ded5;
L18dfe2:
    xm[0]=xm[5];
    store(m,o,0x18dfe5u,0x40u+g[4],fb(xm[0]));
    goto L18ded5;
L18dff0:
    xm[1]=_mm_xor_ps(xm[1],xm[1]);
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L18e002;
    xm[0]=xm[1];
    goto L18e005;
L18dffd:
    fl=comi(xm[0],xm[5]);
    if(fl.cf||fl.zf) goto L18e005;
L18e002:
    xm[0]=xm[5];
L18e005:
    xm[3]=ld_ss(m,0x46a11cu);
    xm[2]=xm[0];
    xm[2]=_mm_mul_ss(xm[2],xm[0]);
    xm[0]=_mm_mul_ss(xm[0],xm[2]);
    xm[1]=xm[2];
    xm[0]=_mm_mul_ss(xm[0],xm[3]);
    xm[1]=_mm_mul_ss(xm[1],xm[4]);
    xm[1]=_mm_sub_ss(xm[1],xm[0]);
    xm[0]=_mm_xor_ps(xm[0],xm[0]);
    fl=comi(xm[0],xm[1]);
    if(fl.cf||fl.zf) goto L18e034;
    xm[1]=xm[0];
    goto L18e03c;
L18e034:
    fl=comi(xm[1],xm[5]);
    if(fl.cf||fl.zf) goto L18e03c;
    xm[1]=xm[5];
L18e03c:
    setlo8(g[2],u8(m,0x1fu+g[4]));
    if((g[2]&0xffu)==0u) goto L18e04b;
    xm[0]=ld_ss(m,0x4u+g[1]);
    goto L18e050;
L18e04b:
    xm[0]=ld_ss(m,0xcu+g[1]);
L18e050:
    g[2]=u32(m,0x14u+g[1]);
    xm[2]=ld_ss(m,0x8u+g[1]);
    g[0]=g[0]+1u;
    store(m,o,0x18e059u,0x88u+g[4],g[0]);
    g[0]=g[0]<<5;
    z=(g[7]==0u);
    g[0]=0x6cu+g[4]+g[0]*1u;
    xm[0]=_mm_sub_ss(xm[0],xm[2]);
    store(m,o,0x18e06du,0x0u+g[0],g[2]);
    g[2]=u32(m,0x20u+g[1]);
    xm[0]=_mm_mul_ss(xm[0],xm[1]);
    xm[0]=_mm_add_ss(xm[0],xm[2]);
    store(m,o,0x18e07au,0x1cu+g[1],fb(xm[0]));
    store(m,o,0x18e07fu,0xcu+g[0],g[2]);
    store(m,o,0x18e082u,0x4u+g[0],fb(xm[0]));
    if(!z) goto L18e0af;
    g[2]=u32(m,0x51df50u);
    if(g[2]==0u) goto L18e0a8;
    if(u8(m,0x5u+g[2])==0u) goto L18e0a8;
    g[2]=u32(m,0x4f9b58u);
    if(u8(m,0x240u+g[2])==0u) goto L18e0af;
L18e0a8:
    g[2]=1u;
    goto L18e0b1;
L18e0af:
    g[2]=0u;
L18e0b1:
    floating.push(fext(m,0x58u+g[3]+g[6]*1u));
    store8(m,o,0x18e0b5u,0x1cu+g[0],g[2]);
    g[2]=u32(m,0x18u+g[1]);
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    store(m,o,0x18e0bdu,0x14u+g[0],g[2]);
    g[2]=0x58u+g[3]+g[6]*1u;
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    store(m,o,0x18e0ceu,0x10u+g[0],g[7]);
    store(m,o,0x18e0d1u,0x18u+g[0],g[5]);
    xm[0]=xm[3];
    if(!fl.cf&&!fl.zf) goto L18e0dd;
    xm[0]=ld_ss(m,0x0u+g[2]);
L18e0dd:
    xm[1]=ld_ss(m,0x508u+g[4]);
    setlo8(g[1],u8(m,0x24u+g[1]));
    xm[1]=_mm_div_ss(xm[1],xm[0]);
    store(m,o,0x18e0edu,0x8u+g[0],fb(xm[1]));
    store8(m,o,0x18e0f2u,0x1du+g[0],g[1]);
    g[0]=u32(m,0x10u+g[4]);
    g[7]=g[7]+1u;
    if(std::int32_t(g[7])<std::int32_t(g[0])) goto L18df01;
    g[6]=u32(m,0x34u+g[4]);
L18e106:
    setlo8(g[0],u8(m,0x1fu+g[4]));
    if((g[0]&0xffu)==0u) goto L18e124;
    if(g[6]==0u) goto L18e124;
    g[2]=u32(m,0x48u+g[4]);
    store(m,o,0x18e116u,0x28u+g[4],g[2]);
    goto L18e124;
L18e11c:
    xm[5]=ld_ss(m,0x46a0d0u);
L18e124:
    g[0]=u32(m,0x504u+g[4]);
    z=(g[0]==0xffffffffu);
    xm[4]=ld_ss(m,0x455b0cu);
    if(z) goto L18e2be;
    g[1]=u32(m,0x4f0414u);
    g[2]=u32(m,0x88u+g[4]);
    g[0]=g[0]&0xffffu;
    g[0]=g[0]<<4;
    g[6]=u32(m,0x8u+g[0]+g[1]*1u);
    g[1]=0u;
    z=(g[2]==0u);
    if(z||std::int32_t(g[2])<0) goto L18e20f;
    xm[1]=xm[5];
    xm[1]=_mm_sub_ss(xm[1],ld_ss(m,0x48u+g[6]));
    g[0]=0x98u+g[4];
    g[7]=g[7];
L18e170:
    xm[2]=_mm_xor_ps(xm[2],xm[2]);
    fl=comi(xm[2],xm[1]);
    if(fl.cf||fl.zf) goto L18e180;
    store(m,o,0x18e178u,0x14u+g[4],fb(xm[2]));
    goto L18e191;
L18e180:
    fl=comi(xm[1],xm[5]);
    store(m,o,0x18e183u,0x14u+g[4],fb(xm[5]));
    if(!fl.cf&&!fl.zf) goto L18e191;
    store(m,o,0x18e18bu,0x14u+g[4],fb(xm[1]));
L18e191:
    fl=comi(xm[1],xm[2]);
    if(fl.cf||fl.zf) goto L18e1c6;
    floating.push(fext(m,0x14u+g[4]));
    floating.push(x_fldlg2());
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[1]=x_fyl2x(a.x87[1],a.x87[0],st); floating.pop(); setc1(a,st);
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a118u));
    store(m,o,0x18e1a6u,0x10u+g[4],sp(floating,a.x87[0]));
    floating.push(fext(m,0x455b0cu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18e1c6;
    xm[0]=ld_ss(m,0x10u+g[4]);
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e1c9;
    xm[0]=xm[2];
    goto L18e1c9;
L18e1c6:
    xm[0]=xm[4];
L18e1c9:
    fl=comi(xm[4],xm[0]);
    store(m,o,0x18e1ccu,0x18u+g[4],fb(xm[0]));
    if(fl.cf||fl.zf) goto L18e1db;
    g[2]=0xc2800000u;
    goto L18e1e8;
L18e1db:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e1e4;
    g[2]=0u;
    goto L18e1e8;
L18e1e4:
    g[2]=u32(m,0x18u+g[4]);
L18e1e8:
    xm[0]=ld_ss(m,0x0u+g[0]);
    store(m,o,0x18e1ecu,0x10u+g[4],g[2]);
    xm[0]=_mm_add_ss(xm[0],ld_ss(m,0x10u+g[4]));
    store(m,o,0x18e1f6u,0x0u+g[0],fb(xm[0]));
    g[2]=u32(m,0x88u+g[4]);
    g[1]=g[1]+1u;
    g[0]=g[0]+0x20u;
    if(std::int32_t(g[1])<std::int32_t(g[2])) goto L18e170;
    goto L18e212;
L18e20f:
    xm[2]=_mm_xor_ps(xm[2],xm[2]);
L18e212:
    g[1]=u32(m,0x3cu+g[6]);
    if(g[1]==0xffffffffu) goto L18e2c1;
    xm[0]=ld_ss(m,0x4cu+g[6]);
    g[2]=g[2]+1u;
    store(m,o,0x18e224u,0x88u+g[4],g[2]);
    g[2]=g[2]<<5;
    fl=comi(xm[2],xm[0]);
    g[0]=0x6cu+g[4]+g[2]*1u;
    store(m,o,0x18e235u,0x0u+g[0],g[1]);
    if(fl.cf||fl.zf) goto L18e241;
    store(m,o,0x18e239u,0x14u+g[4],fb(xm[2]));
    goto L18e252;
L18e241:
    fl=comi(xm[0],xm[5]);
    store(m,o,0x18e244u,0x14u+g[4],fb(xm[5]));
    if(!fl.cf&&!fl.zf) goto L18e252;
    store(m,o,0x18e24cu,0x14u+g[4],fb(xm[0]));
L18e252:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e287;
    floating.push(fext(m,0x14u+g[4]));
    floating.push(x_fldlg2());
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[1]=x_fyl2x(a.x87[1],a.x87[0],st); floating.pop(); setc1(a,st);
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a118u));
    store(m,o,0x18e267u,0x14u+g[4],sp(floating,a.x87[0]));
    floating.push(fext(m,0x455b0cu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18e287;
    xm[0]=ld_ss(m,0x14u+g[4]);
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e28a;
    xm[0]=xm[2];
    goto L18e28a;
L18e287:
    xm[0]=xm[4];
L18e28a:
    fl=comi(xm[4],xm[0]);
    store(m,o,0x18e28du,0x18u+g[4],fb(xm[0]));
    if(fl.cf||fl.zf) goto L18e29c;
    g[1]=0xc2800000u;
    goto L18e2a9;
L18e29c:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e2a5;
    g[1]=0u;
    goto L18e2a9;
L18e2a5:
    g[1]=u32(m,0x18u+g[4]);
L18e2a9:
    store(m,o,0x18e2a9u,0xcu+g[0],g[1]);
    store(m,o,0x18e2acu,0x4u+g[0],fb(xm[5]));
    store8(m,o,0x18e2b1u,0x1cu+g[0],0x1u);
    store(m,o,0x18e2b5u,0x10u+g[0],0x2u);
    goto L18e2c1;
L18e2be:
    xm[2]=_mm_xor_ps(xm[2],xm[2]);
L18e2c1:
    g[7]=u32(m,0x4f9b5cu);
    g[2]=0u;
    store(m,o,0x18e2c9u,0x2cu+g[4],g[2]);
    store(m,o,0x18e2cdu,0xecu+g[4],0x0u);
    store(m,o,0x18e2d8u,0x30u+g[4],g[2]);
    g[6]=0xffffffffu;
    g[5]=0xfdffu;
L18e2e4:
    g[6]=g[6]+1u;
    if(std::int32_t(g[6])<0) goto L18e310;
    g[2]=u32(m,0x38u+g[7]);
    if(std::int32_t(g[6])>=std::int32_t(g[2])) goto L18e310;
    g[0]=u32(m,0x48u+g[7]);
L18e2f1:
    g[1]=g[6];
    g[1]=g[1]&0x1fu;
    g[3]=1u;
    g[3]=1u<<(g[1]&0x1fu);
    g[1]=g[6];
    g[1]=sar5(g[1]);
    if((u32(m,0x0u+g[0]+g[1]*4u)&g[3])!=0u) goto L18e43a;
    g[6]=g[6]+1u;
    if(std::int32_t(g[6])<std::int32_t(g[2])) goto L18e2f1;
L18e310:
    g[0]=u32(m,0x88u+g[4]);
    g[3]=0xffffffffu;
    if(std::int32_t(g[0])<=0) goto L18e66d;
    g[5]=0x8cu+g[4];
    store(m,o,0x18e329u,0x18u+g[4],g[0]);
    g[1]=0x0u+g[1];
L18e330:
    g[0]=u32(m,0xecu+g[4]);
    g[1]=0u;
    if(std::int32_t(g[0])<=0) goto L18e371;
    g[1]=0u;
L18e340:
    g[6]=u32(m,0xf0u+g[4]+g[1]*4u);
    if(g[6]==g[3]) goto L18e365;
    g[0]=g[6];
    g[0]=g[0]&0xffffu;
    g[2]=0x0u+g[0]+g[0]*2u;
    g[0]=u32(m,0x44u+g[7]);
    g[2]=u32(m,0xcu+g[0]+g[2]*8u);
    if(g[2]==u32(m,0x0u+g[5])) goto L18e499;
L18e365:
    g[0]=u32(m,0xecu+g[4]);
    g[1]=g[1]+1u;
    if(std::int32_t(g[1])<std::int32_t(g[0])) goto L18e340;
L18e371:
    g[6]=u32(m,0x0u+g[5]);
    eqg=(g[6]==g[3]);
    xm[0]=ld_ss(m,0x4u+g[5]);
    g[1]=g[3];
    if(eqg) goto L18e3e4;
    g[2]=g[7];
    ext_call(c,a,xm,0x18e386u,0x16e6a0u);
    if(g[0]==g[3]) goto L18e3be;
    g[2]=u32(m,0x44u+g[7]);
    g[1]=g[0];
    g[1]=g[1]&0xffffu;
    g[1]=0x0u+g[1]+g[1]*2u;
    g[1]=0x0u+g[2]+g[1]*8u;
    store(m,o,0x18e39bu,0xcu+g[1],g[6]);
    store8(m,o,0x18e39eu,0x2u+g[1],0x3u);
    g[2]=0u;
    store16(m,o,0x18e3a4u,0x4u+g[1],g[2]);
    store8(m,o,0x18e3a8u,0x5u+g[1],u8(m,0x5u+g[1])|0x1u);
    store16(m,o,0x18e3acu,0x6u+g[1],g[3]);
    store8(m,o,0x18e3b0u,0x3u+g[1],g[2]);
    store(m,o,0x18e3b3u,0x10u+g[1],g[2]);
    store8(m,o,0x18e3b6u,0x14u+g[1],0xffu);
    store16(m,o,0x18e3bau,0x16u+g[1],g[3]);
L18e3be:
    eqg2=(g[0]==g[3]);
    g[1]=g[0];
    if(eqg2) goto L18e3dc;
    g[2]=u32(m,0x44u+g[7]);
    g[0]=g[0]&0xffffu;
    g[0]=0x0u+g[0]+g[0]*2u;
    store8(m,o,0x18e3cfu,0x4u+g[2]+g[0]*8u,u8(m,0x4u+g[2]+g[0]*8u)|0x1u);
    g[0]=0x0u+g[2]+g[0]*8u;
    store(m,o,0x18e3d7u,0x8u+g[0],fb(xm[0]));
L18e3dc:
    xm[4]=ld_ss(m,0x455b0cu);
L18e3e4:
    eqg3=(g[1]==g[3]);
    g[6]=g[1];
    if(eqg3) goto L18e65b;
    g[1]=g[1]&0xffffu;
    g[0]=0x0u+g[1]+g[1]*2u;
    g[1]=u32(m,0x44u+g[7]);
    store(m,o,0x18e3fau,0x10u+g[1]+g[0]*8u,0xc2800000u);
    xm[4]=ld_ss(m,0x455b0cu);
L18e40a:
    g[0]=u32(m,0x44u+g[7]);
    g[6]=g[6]&0xffffu;
    g[2]=0x0u+g[6]+g[6]*2u;
    g[1]=u32(m,0x10u+g[0]+g[2]*8u);
    g[0]=0x0u+g[0]+g[2]*8u;
    store(m,o,0x18e41du,0x10u+g[4],g[1]);
    xm[0]=ld_ss(m,0x10u+g[4]);
    fl=comi(xm[4],xm[0]);
    if(fl.cf||fl.zf) goto L18e4b9;
    g[1]=0xc2800000u;
    goto L18e4c0;
L18e43a:
    if(g[6]==0xffffffffu) goto L18e310;
    g[0]=u32(m,0x24u+g[7]);
    g[3]=u32(m,0x44u+g[7]);
    g[0]=g[0]*g[6];
    g[1]=sx16(u16(m,0x0u+g[0]+g[3]*1u));
    setlo8(g[2],u8(m,0x3u+g[0]+g[3]*1u));
    g[0]=g[0]+g[3];
    g[1]=g[1]<<0x10;
    g[1]=g[1]|g[6];
    if((g[2]&0xffu)!=0u) goto L18e2e4;
    g[2]=sx8(u8(m,0x14u+g[0]));   // movsx from byte
    if(g[2]==0xffffffffu) goto L18e2e4;
    g[2]=u32(m,0xecu+g[4]);
    store16(m,o,0x18e477u,0x4u+g[0],u16(m,0x4u+g[0])&g[5]);
    xm[4]=ld_ss(m,0x455b0cu);
    g[0]=g[2];
    g[2]=g[2]+1u;
    store(m,o,0x18e486u,0xecu+g[4],g[2]);
    store(m,o,0x18e48du,0xf0u+g[4]+g[0]*4u,g[1]);
    goto L18e2e4;
L18e499:
    g[0]=g[1];
    g[1]=g[1]&0x1fu;
    g[0]=sar5(g[0]);
    g[2]=1u;
    g[2]=1u<<(g[1]&0x1fu);
    g[1]=u32(m,0x2cu+g[4]+g[0]*4u);
    g[0]=0x2cu+g[4]+g[0]*4u;
    g[1]=g[1]|g[2];
    store(m,o,0x18e4b2u,0x0u+g[0],g[1]);
    goto L18e40a;
L18e4b9:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e4c0;
    g[1]=0u;
L18e4c0:
    store(m,o,0x18e4c0u,0x10u+g[4],g[1]);
    floating.push(fext(m,0x10u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a12cu));
    g[1]=u32(m,0xcu+g[5]);
    store(m,o,0x18e4d1u,0x20u+g[4],g[1]);
    xm[0]=ld_ss(m,0x20u+g[4]);
    fl=comi(xm[4],xm[0]);
    a.x87[0]=fmul(a.x87[0],fext(m,0x455b14u));
    floating.push(x_fldl2e());
    a.x87[1]=fmul(a.x87[1],a.x87[0]); floating.pop();
    floating.push(a.x87[0]);
    a.x87[0]=x_frndint(a.x87[0],st); setc1(a,st);
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    a.x87[0]=x_f2xm1(a.x87[0],st); setc1(a,st);
    floating.push(1.0L);
    a.x87[1]=fadd(a.x87[1],a.x87[0]); floating.pop();
    a.x87[0]=x_fscale(a.x87[0],a.x87[1],st); setc1(a,st);
    a.x87[1]=a.x87[0]; floating.clear_c1(); floating.pop();
    store(m,o,0x18e4fau,0x10u+g[4],sp(floating,a.x87[0])); floating.pop();
    if(fl.cf||fl.zf) goto L18e507;
    g[1]=0xc2800000u;
    goto L18e50e;
L18e507:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e50e;
    g[1]=0u;
L18e50e:
    store(m,o,0x18e50eu,0x20u+g[4],g[1]);
    floating.push(fext(m,0x20u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a12cu));
    xm[1]=ld_ss(m,0x8u+g[5]);
    xm[0]=xm[2];
    xm[0]=_mm_sub_ss(xm[0],xm[1]);
    a.x87[0]=fmul(a.x87[0],fext(m,0x455b14u));
    floating.push(x_fldl2e());
    a.x87[1]=fmul(a.x87[1],a.x87[0]); floating.pop();
    floating.push(a.x87[0]);
    a.x87[0]=x_frndint(a.x87[0],st); setc1(a,st);
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    a.x87[0]=x_f2xm1(a.x87[0],st); setc1(a,st);
    floating.push(1.0L);
    a.x87[1]=fadd(a.x87[1],a.x87[0]); floating.pop();
    a.x87[0]=x_fscale(a.x87[0],a.x87[1],st); setc1(a,st);
    a.x87[1]=a.x87[0]; floating.clear_c1(); floating.pop();
    a.x87[0]=fsub(a.x87[0],fext(m,0x10u+g[4]));
    store(m,o,0x18e548u,0x20u+g[4],sp(floating,a.x87[0])); floating.pop();
    xm[3]=ld_ss(m,0x20u+g[4]);
    fl=comi(xm[0],xm[3]);
    if(!fl.cf&&!fl.zf) goto L18e562;
    fl=comi(xm[3],xm[1]);
    xm[0]=xm[1];
    if(!fl.cf&&!fl.zf) goto L18e562;
    xm[0]=xm[3];
L18e562:
    xm[1]=ld_ss(m,0x10u+g[4]);
    xm[1]=_mm_add_ss(xm[1],xm[0]);
    fl=comi(xm[2],xm[1]);
    if(fl.cf||fl.zf) goto L18e579;
    store(m,o,0x18e571u,0x14u+g[4],fb(xm[2]));
    goto L18e58a;
L18e579:
    fl=comi(xm[1],xm[5]);
    store(m,o,0x18e57cu,0x14u+g[4],fb(xm[5]));
    if(!fl.cf&&!fl.zf) goto L18e58a;
    store(m,o,0x18e584u,0x14u+g[4],fb(xm[1]));
L18e58a:
    fl=comi(xm[1],xm[2]);
    if(fl.cf||fl.zf) goto L18e5bf;
    floating.push(fext(m,0x14u+g[4]));
    floating.push(x_fldlg2());
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[1]=x_fyl2x(a.x87[1],a.x87[0],st); floating.pop(); setc1(a,st);
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a118u));
    store(m,o,0x18e59fu,0x10u+g[4],sp(floating,a.x87[0]));
    floating.push(fext(m,0x455b0cu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18e5bf;
    xm[0]=ld_ss(m,0x10u+g[4]);
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e5c2;
    xm[0]=xm[2];
    goto L18e5c2;
L18e5bf:
    xm[0]=xm[4];
L18e5c2:
    fl=comi(xm[4],xm[0]);
    store(m,o,0x18e5c5u,0x24u+g[4],fb(xm[0]));
    if(fl.cf||fl.zf) goto L18e5d4;
    g[1]=0xc2800000u;
    goto L18e5e1;
L18e5d4:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e5dd;
    g[1]=0u;
    goto L18e5e1;
L18e5dd:
    g[1]=u32(m,0x24u+g[4]);
L18e5e1:
    store(m,o,0x18e5e1u,0x10u+g[0],g[1]);
    setlo8(g[1],u8(m,0x1cu+g[5]));
    store8(m,o,0x18e5e7u,0x5u+g[0],u8(m,0x5u+g[0])|0x2u);
    if((g[1]&0xffu)!=0u) goto L18e5f5;
    store8(m,o,0x18e5efu,0x4u+g[0],u8(m,0x4u+g[0])|0x80u);
    goto L18e5f9;
L18e5f5:
    store8(m,o,0x18e5f5u,0x4u+g[0],u8(m,0x4u+g[0])&0x7fu);
L18e5f9:
    setlo8(g[1],u8(m,0x1du+g[5]));
    if((g[1]&0xffu)==0u) goto L18e606;
    store8(m,o,0x18e600u,0x4u+g[0],u8(m,0x4u+g[0])|0x10u);
    goto L18e60a;
L18e606:
    store8(m,o,0x18e606u,0x4u+g[0],u8(m,0x4u+g[0])&0xefu);
L18e60a:
    setlo8(g[1],u8(m,0x474c24u));
    setlo16(g[2],u16(m,0x14u+g[5]));
    xm[3]=ld_ss(m,0x8u+g[5]);
    xm[0]=ld_ss(m,0x4u+g[5]);
    store8(m,o,0x18e61eu,0x15u+g[0],g[1]);
    store16(m,o,0x18e621u,0x16u+g[0],g[2]);
    xm[0]=_mm_sub_ss(xm[0],ld_ss(m,0x8u+g[0]));
    xm[1]=xm[2];
    xm[1]=_mm_sub_ss(xm[1],xm[3]);
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L18e63b;
    xm[0]=xm[1];
    goto L18e643;
L18e63b:
    fl=comi(xm[0],xm[3]);
    if(fl.cf||fl.zf) goto L18e643;
    xm[0]=xm[3];
L18e643:
    xm[0]=_mm_add_ss(xm[0],ld_ss(m,0x8u+g[0]));
    setlo8(g[1],u8(m,0x18u+g[5]));
    store(m,o,0x18e64bu,0x8u+g[0],fb(xm[0]));
    store8(m,o,0x18e650u,0x14u+g[0],g[1]);
    xm[4]=ld_ss(m,0x455b0cu);
L18e65b:
    g[0]=u32(m,0x18u+g[4]);
    g[5]=g[5]+0x20u;
    g[0]=g[0]-1u;
    store(m,o,0x18e663u,0x18u+g[4],g[0]);
    if(g[0]!=0u) goto L18e330;
L18e66d:
    g[5]=g[3];
    g[3]=u32(m,0x4ecc18u);
L18e675:
    g[0]=0x1u+g[5];
    if(std::int32_t(g[0])<0) goto L18e6a5;
    g[6]=u32(m,0x38u+g[7]);
    if(std::int32_t(g[0])>=std::int32_t(g[6])) goto L18e6a5;
    g[2]=u32(m,0x48u+g[7]);
L18e686:
    g[1]=g[0];
    g[1]=g[1]&0x1fu;
    g[5]=1u;
    g[5]=1u<<(g[1]&0x1fu);
    g[1]=g[0];
    g[1]=sar5(g[1]);
    if((u32(m,0x0u+g[2]+g[1]*4u)&g[5])!=0u) goto L18e76a;
    g[0]=g[0]+1u;
    if(std::int32_t(g[0])<std::int32_t(g[6])) goto L18e686;
L18e6a5:
    g[0]=u32(m,0x28u+g[4]);
    g[5]=0xffffffffu;
    if(g[0]==0xffffffffu) goto L18e6c5;
    g[2]=u32(m,0x4f4u+g[4]);
    g[0]=g[0]*0x64u;
    g[1]=u32(m,0xd8u+g[2]);
    g[5]=u32(m,0x2cu+g[1]+g[0]*1u);
L18e6c5:
    g[6]=u32(m,0x4f8u+g[4]);
    g[3]=u32(m,0x4f9b58u);
    g[0]=u32(m,0x0u+g[3]+g[6]*8u);
    if(g[0]==0xffffffffu) goto L18e6fb;
    g[0]=g[0]&0xffffu;
    g[2]=0x0u+g[0]+g[0]*2u;
    g[0]=u32(m,0x44u+g[7]);
    g[1]=u32(m,0xcu+g[0]+g[2]*8u);
    be=(g[1]==g[5]);
    g[0]=0x0u+g[0]+g[2]*8u;
    if(be) goto L18e75d;
    store8(m,o,0x18e6f0u,0x4u+g[0],u8(m,0x4u+g[0])|0x2u);
    store(m,o,0x18e6f4u,0x0u+g[3]+g[6]*8u,0xffffffffu);
L18e6fb:
    if(g[5]==0xffffffffu) goto L18e75d;
    g[2]=g[7];
    ext_call(c,a,xm,0x18e707u,0x16e6a0u);
    if(g[0]==0xffffffffu) goto L18e75a;
    g[1]=g[0];
    g[1]=g[1]&0xffffu;
    g[6]=0x0u+g[1]+g[1]*2u;
    g[1]=u32(m,0x44u+g[7]);
    g[6]=g[6]<<0x3;
    g[1]=g[1]+g[6];
    store(m,o,0x18e71fu,0xcu+g[1],g[5]);
    store8(m,o,0x18e722u,0x2u+g[1],0x3u);
    g[2]=0u;
    store16(m,o,0x18e728u,0x4u+g[1],g[2]);
    store8(m,o,0x18e72cu,0x5u+g[1],u8(m,0x5u+g[1])|0x1u);
    g[5]=0xffffffffu;
    store16(m,o,0x18e733u,0x6u+g[1],g[5]);
    store8(m,o,0x18e737u,0x3u+g[1],g[2]);
    store(m,o,0x18e73au,0x10u+g[1],g[2]);
    store8(m,o,0x18e73du,0x14u+g[1],0xffu);
    store16(m,o,0x18e741u,0x16u+g[1],g[5]);
    g[1]=u32(m,0x44u+g[7]);
    g[1]=g[1]+g[6];
    store8(m,o,0x18e74au,0x4u+g[1],u8(m,0x4u+g[1])|0x1u);
    g[6]=u32(m,0x4f8u+g[4]);
    store(m,o,0x18e755u,0x8u+g[1],fb(xm[5]));
L18e75a:
    store(m,o,0x18e75au,0x0u+g[3]+g[6]*8u,g[0]);
L18e75d:
    g[7]=u32(m,g[4]); g[4]+=4u;                     // pop EDI
    g[6]=u32(m,g[4]); g[4]+=4u;                     // pop ESI
    g[5]=u32(m,g[4]); g[4]+=4u;                     // pop EBP
    g[3]=u32(m,g[4]); g[4]+=4u;                     // pop EBX
    g[4]+=0x4e0u;
    for(int i=0;i<8;++i) set_xmm(a,i,xm[i]);
    pop_return(m,a,24);
    return;
L18e76a:
    if(g[0]==0xffffffffu) goto L18e6a5;
    g[1]=u32(m,0x24u+g[7]);
    g[6]=u32(m,0x44u+g[7]);
    g[1]=g[1]*g[0];
    g[1]=g[1]+g[6];
    g[6]=sx16(u16(m,0x0u+g[1]));
    g[6]=g[6]<<0x10;
    g[6]=g[6]|g[0];
    g[5]=g[0];
    setlo8(g[0],u8(m,0x3u+g[1]));
    if((g[0]&0xffu)!=0u) goto L18e675;
    g[0]=sx8(u8(m,0x14u+g[1]));
    if(g[0]==0xffffffffu) goto L18e675;
    g[2]=0u;
    setlo16(g[2],u16(m,0x4u+g[1]));
    g[2]=g[2]>>0x9;
    if((g[2]&0x1u)!=0u) goto L18e675;
    g[2]=u32(m,0xd8u+g[3]);
    g[0]=g[0]*0x64u;
    floating.push(fext(m,0x58u+g[0]+g[2]*1u));
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    xm[0]=ld_ss(m,0x46a11cu);
    g[0]=g[0]+g[2];
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18e7dc;
    xm[0]=ld_ss(m,0x58u+g[0]);
L18e7dc:
    g[0]=u32(m,0x10u+g[1]);
    xm[3]=ld_ss(m,0x508u+g[4]);
    xm[3]=_mm_div_ss(xm[3],xm[0]);
    store(m,o,0x18e7ecu,0x24u+g[4],g[0]);
    xm[0]=ld_ss(m,0x24u+g[4]);
    fl=comi(xm[4],xm[0]);
    if(fl.cf||fl.zf) goto L18e802;
    g[0]=0xc2800000u;
    goto L18e809;
L18e802:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e809;
    g[0]=0u;
L18e809:
    store(m,o,0x18e809u,0x24u+g[4],g[0]);
    floating.push(fext(m,0x24u+g[4]));
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a12cu));
    xm[0]=xm[2];
    xm[1]=xm[2];
    xm[1]=_mm_sub_ss(xm[1],xm[3]);
    a.x87[0]=fmul(a.x87[0],fext(m,0x455b14u));
    floating.push(x_fldl2e());
    a.x87[1]=fmul(a.x87[1],a.x87[0]); floating.pop();
    floating.push(a.x87[0]);
    a.x87[0]=x_frndint(a.x87[0],st); setc1(a,st);
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[0]=fsub(a.x87[0],a.x87[1]);
    a.x87[0]=x_f2xm1(a.x87[0],st); setc1(a,st);
    floating.push(1.0L);
    a.x87[1]=fadd(a.x87[1],a.x87[0]); floating.pop();
    a.x87[0]=x_fscale(a.x87[0],a.x87[1],st); setc1(a,st);
    a.x87[1]=a.x87[0]; floating.clear_c1(); floating.pop();
    store(m,o,0x18e83du,0x14u+g[4],sp(floating,a.x87[0])); floating.pop();
    xm[4]=ld_ss(m,0x14u+g[4]);
    xm[0]=_mm_sub_ss(xm[0],xm[4]);
    fl=comi(xm[1],xm[0]);
    if(fl.cf||fl.zf) goto L18e855;
    xm[0]=xm[1];
    goto L18e85d;
L18e855:
    fl=comi(xm[0],xm[3]);
    if(fl.cf||fl.zf) goto L18e85d;
    xm[0]=xm[3];
L18e85d:
    xm[0]=_mm_add_ss(xm[0],xm[4]);
    xm[4]=xm[0];
    xm[0]=xm[2];
    xm[0]=_mm_sub_ss(xm[0],ld_ss(m,0x8u+g[1]));
    fl=comi(xm[1],xm[0]);
    store(m,o,0x18e86fu,0x14u+g[4],fb(xm[4]));
    if(fl.cf||fl.zf) goto L18e87c;
    xm[0]=xm[1];
    goto L18e884;
L18e87c:
    fl=comi(xm[0],xm[3]);
    if(fl.cf||fl.zf) goto L18e884;
    xm[0]=xm[3];
L18e884:
    floating.push(fext(m,0x14u+g[4]));
    xm[1]=ld_ss(m,0x8u+g[1]);
    a.x87[0]=std::fabs(a.x87[0]); floating.clear_c1();
    xm[1]=_mm_add_ss(xm[1],xm[0]);
    floating.push(fext(m,0x46a0ecu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    store(m,o,0x18e89du,0x8u+g[1],fb(xm[1]));
    if(fl.cf||fl.zf) goto L18e8c6;
    g[1]=u32(m,0x44u+g[7]);
    g[6]=g[6]&0xffffu;
    g[0]=0x0u+g[6]+g[6]*2u;
    store8(m,o,0x18e8b0u,0x4u+g[1]+g[0]*8u,u8(m,0x4u+g[1]+g[0]*8u)|0x2u);
    xm[4]=ld_ss(m,0x455b0cu);
    g[0]=0x4u+g[1]+g[0]*8u;
    goto L18e675;
L18e8c6:
    fl=comi(xm[2],xm[4]);
    if(fl.cf||fl.zf) goto L18e8d3;
    store(m,o,0x18e8cbu,0x10u+g[4],fb(xm[2]));
    goto L18e8e4;
L18e8d3:
    fl=comi(xm[4],xm[5]);
    store(m,o,0x18e8d6u,0x10u+g[4],fb(xm[5]));
    if(!fl.cf&&!fl.zf) goto L18e8e4;
    store(m,o,0x18e8deu,0x10u+g[4],fb(xm[4]));
L18e8e4:
    fl=comi(xm[4],xm[2]);
    xm[1]=ld_ss(m,0x455b0cu);
    if(fl.cf||fl.zf) goto L18e921;
    floating.push(fext(m,0x10u+g[4]));
    floating.push(x_fldlg2());
    std::swap(a.x87[0],a.x87[1]); floating.clear_c1();
    a.x87[1]=x_fyl2x(a.x87[1],a.x87[0],st); floating.pop(); setc1(a,st);
    a.x87[0]=fmul(a.x87[0],fext(m,0x46a118u));
    store(m,o,0x18e901u,0x18u+g[4],sp(floating,a.x87[0]));
    floating.push(fext(m,0x455b0cu));
    fl=fcomip(floating,a.x87[1]);
    floating.clear_c1(); floating.pop();
    if(!fl.cf&&!fl.zf) goto L18e921;
    xm[0]=ld_ss(m,0x18u+g[4]);
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e924;
    xm[0]=xm[2];
    goto L18e924;
L18e921:
    xm[0]=xm[1];
L18e924:
    fl=comi(xm[1],xm[0]);
    store(m,o,0x18e927u,0x20u+g[4],fb(xm[0]));
    if(fl.cf||fl.zf) goto L18e944;
    g[0]=0xc2800000u;
    store(m,o,0x18e934u,0x10u+g[1],g[0]);
    xm[4]=ld_ss(m,0x455b0cu);
    goto L18e675;
L18e944:
    fl=comi(xm[0],xm[2]);
    if(fl.cf||fl.zf) goto L18e95b;
    g[0]=0u;
    store(m,o,0x18e94bu,0x10u+g[1],g[0]);
    xm[4]=ld_ss(m,0x455b0cu);
    goto L18e675;
L18e95b:
    g[0]=u32(m,0x20u+g[4]);
    store(m,o,0x18e95fu,0x10u+g[1],g[0]);
    xm[4]=ld_ss(m,0x455b0cu);
    goto L18e675;
}

} // namespace

std::span<const LoopRoutineEntry> object_loop_update_routines(){
    static constexpr LoopRoutineEntry table[]{
        {0x129170u,r_129170},
        {0x18d960u,r_18d960},
        {0x190a80u,r_190a80},
        {0x18dc70u,r_18dc70},
        {0x18e970u,r_18e970},
    };
    return table;
}
} // namespace halo2::native_tu15_sound
