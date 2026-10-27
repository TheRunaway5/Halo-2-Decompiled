// Native object looping-sound update routines: the sources part of the 18EE50
// group (see native_tu15_sound_object_loop.hpp). These are the per-source
// routines the object-loop update walks every frame: 18D5A0 (per-source
// loop-sound update), 220C60 (the loop-sound request builder reached through
// the live world), and their helpers 18F9B0, 18FFB0, 190460, 1904D0, 220640
// and 18B580.
//
// The routines are faithful translations of the lifted TU1.5 bodies. Each
// reproduces every memory store at its original instruction address, the full
// register state the original leaves (all GP and XMM registers, including
// scratch), and the x87/MXCSR state. Calls to routines in this group run
// natively through object_loop_invoke; the core-game helpers and the two
// 220C60 loop-sound requests run through the live world (call_out / the
// group resolver's required_owner route).
#include "audio/native_tu15_sound_object_loop.hpp"

namespace halo2::native_tu15_sound {
namespace {

// ---- small register/value helpers -----------------------------------------
inline std::uint32_t lo8(std::uint32_t r,std::uint32_t v){return (r&0xffffff00u)|(v&0xffu);}
inline std::uint32_t lo16(std::uint32_t r,std::uint32_t v){return (r&0xffff0000u)|(v&0xffffu);}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}
inline __m128 ss(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}
inline std::uint32_t ssb(__m128 x){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x));}
inline std::int32_t simul(std::uint32_t a,std::uint32_t b){return std::int32_t(std::uint32_t(std::int32_t(a)*std::int32_t(b)));}

// A memory read of 1/2/4 bytes, zero-extended (the lifted r(memory,addr,n)).
inline std::uint32_t rd(const M &m,std::uint32_t p,unsigned n){
    if(n==1)return u8(m,p);if(n==2)return u16(m,p);return u32(m,p);
}

// Retail .rdata constants (TU1.5 XBE), folded as literals.
constexpr std::uint32_t c_pi=0x40490fdbu;        // 0x46A2EC: pi
constexpr std::uint32_t c_rad_to_short=0x46a2f8e0u; // 0x46A584: 65536/2pi scale (20860.4375)
constexpr std::uint32_t c_four=0x40800000u;      // 0x46A120: 4.0
constexpr std::uint32_t c_hundredth=0x3c23d70au; // 0x46A18C: 0.01
constexpr std::uint32_t c_thousandth=0x3a83126fu;// 0x46A180: 0.001
constexpr std::uint32_t c_one=0x3f800000u;       // 0x46A0D0: 1.0
constexpr std::uint32_t c_zero=0u;               // 0x46A0E8: 0.0
constexpr std::uint32_t c_thousand=0x447a0000u;  // 0x46A1DC: 1000.0
constexpr std::uint32_t c_half=0x3f000000u;      // immediate 0.5 (0x2213F5)

// Game globals.
constexpr std::uint32_t tag_instances_g=0x4f0414u;
constexpr std::uint32_t loop_pool_g=0x4f9b5cu;    // object looping-sound storage
constexpr std::uint32_t sound_state_g=0x4f2c50u;  // listener/sound-state globals
constexpr std::uint32_t sound_globals_g=0x4ecc1cu;
constexpr std::uint32_t random_seed_g=0x4f3cd8u;

// ---- COMISS/UCOMISS flags, exactly as the lifted scalar_compare sets them --
struct Cmp{bool cf,zf,pf;};
inline Cmp comiss(__m128 x,__m128 y){
    unsigned char c,z,p;
    asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(c),"=qm"(z),"=qm"(p):"x"(x),"x"(y):"cc");
    return {c!=0,z!=0,p!=0};
}
inline Cmp ucomiss(__m128 x,__m128 y){
    unsigned char c,z,p;
    asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2":"=qm"(c),"=qm"(z),"=qm"(p):"x"(x),"x"(y):"cc");
    return {c!=0,z!=0,p!=0};
}

// The original's x87 loads need room. Refuse where the reference routine does,
// with the same message, so faults match.
inline void require_x87_room(const Abi &a,unsigned slots){
    if(a.x87_depth+slots>8u)throw FormatError("Required original solver x87 stack-fault owner unavailable");
}
// FISTP m32: the rounded int32 and the C1 (round direction) it reports.
inline std::uint32_t integer_spill(Abi &a,long double x){
    std::uint32_t result;std::uint16_t status;
    asm volatile("fldt %2\n\tfistpl %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(x):"st");
    a.x87_status=std::uint16_t((a.x87_status&~0x200u)|(status&0x200u));
    return result;
}
// An x87 m32 operand as its own extended value (a plain cast, as the lifted
// ext(float) is: the software x87 model does not FLD, so no denormal signal).
inline long double ext(float x){return static_cast<long double>(x);}

// A COMISS under the caller's FP environment (sticky MXCSR flags accumulate
// into the ABI, exactly as the lifted scalar_compare does over the run).
inline Cmp comiss_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return comiss(x,y);}
inline Cmp ucomiss_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return ucomiss(x,y);}
// Scalar SSE arithmetic under the caller's FP environment, each op isolated so
// the ABI is synced between it and the surrounding integer work and calls.
inline __m128 mul_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return _mm_mul_ss(x,y);}
inline __m128 div_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return _mm_div_ss(x,y);}
inline __m128 add_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return _mm_add_ss(x,y);}
inline __m128 sub_fp(LoopContext &c,Abi &a,__m128 x,__m128 y){Floating f(a,c.fp);return _mm_sub_ss(x,y);}
inline __m128 cvtsi_fp(LoopContext &c,Abi &a,__m128 dst,std::int32_t v){Floating f(a,c.fp);return _mm_cvtsi32_ss(dst,v);}
inline std::int32_t cvttss_fp(LoopContext &c,Abi &a,__m128 x){Floating f(a,c.fp);return _mm_cvttss_si32(x);}

// The native camera-view operand at an audio read, else the original memory.
inline std::uint32_t view_read(LoopContext &c,std::uint32_t instr,std::uint32_t addr,unsigned size){
    if(c.views){
        const auto state=u32(c.m,sound_state_g);
        if(const auto v=source_object_audio_view_operand_tu15(c.views,state,instr,addr,size))return *v;
    }
    return rd(c.m,addr,size);
}

// ===========================================================================
// 18B580 (15 instrs): clamp XMM1 to [0, pi], scale by 65536/2pi, round to an
// int and return its low 16 bits in AX. x87 FLD/FISTP via the caller's CW.
// Clobbers XMM0 and its stack scratch; AX is the only GP result.
void run_18b580(LoopContext &c,Abi &a){
    Floating floating(a,c.fp);
    auto &m=c.m;
    __m128 x0=_mm_setzero_ps();set_xmm(a,0,x0);        // xorps xmm0,xmm0
    a.gp[4]-=8u;                                        // sub esp,8
    const __m128 x1=xmm(a,1);
    bool take=!comiss(x0,x1).cf && !comiss(x0,x1).zf;   // comiss xmm0,xmm1; ja
    // Re-evaluate once (two comiss calls above would raise IE twice): collapse.
    {const auto f=comiss(x0,x1);take=!f.cf&&!f.zf;}
    if(!take){
        x0=ss(c_pi);                                    // movss xmm0,[pi]
        const auto f=comiss(x1,x0);take=!f.cf&&!f.zf;   // comiss xmm1,xmm0; ja
        if(!take)x0=x1;                                 // movaps xmm0,xmm1
    }
    x0=_mm_mul_ss(x0,ss(c_rad_to_short));               // mulss xmm0,[scale]
    set_xmm(a,0,x0);
    store(m,c.o,0x18b5a3u,a.gp[4],ssb(x0));             // movss [esp],xmm0
    const long double loaded=ext(f32(m,a.gp[4]));       // fld [esp]
    require_x87_room(a,1);floating.push(loaded);        // (clears C1)
    const std::uint32_t spilled=integer_spill(a,a.x87[0]); // fistp [esp+4]
    store(m,c.o,0x18b5abu,a.gp[4]+4u,spilled);
    floating.pop();
    a.gp[0]=lo16(a.gp[0],u16(m,a.gp[4]+4u));            // mov ax,[esp+4]
    a.gp[4]+=8u;                                        // add esp,8
    pop_return(m,a);
}

// ===========================================================================
// 1904D0 (17 instrs): the sound globals' default max distance (a .rdata 4.0
// unless a sound-class datum overrides it). Returns XMM0. Leaf, SSE moves only.
void run_1904d0(LoopContext &c,Abi &a){
    auto &m=c.m;
    a.gp[0]=u32(m,sound_globals_g);                     // eax = sound globals
    a.gp[1]=u32(m,a.gp[0]+0xc0u);                       // ecx = count
    set_xmm(a,0,ss(c_four));                            // xmm0 = 4.0
    if(a.gp[1]!=0u)a.gp[0]=u32(m,a.gp[0]+0xc4u);        // eax = class ref (count!=0)
    else a.gp[0]=0u;                                    // xor eax,eax
    a.gp[0]=u32(m,a.gp[0]+0x14u);                       // eax = [0x14+eax]
    if(a.gp[0]!=0xffffffffu){                           // != -1
        a.gp[1]=u32(m,tag_instances_g);                 // ecx = tag instances
        a.gp[0]=(a.gp[0]&0xffffu)<<4;
        a.gp[2]=u32(m,a.gp[0]+a.gp[1]+8u);              // edx = class data
        set_xmm(a,0,ss(u32(m,a.gp[2]+0x54u)));          // xmm0 = [0x54+edx]
    }
    pop_return(m,a);
}

// ===========================================================================
// 220640 (32 instrs): if all three listener-device bytes are set and a sound
// datum (EAX) is live, fetch its platform handle (220370/220320) and notify
// (2208E0) with the stack argument. RET 4. EBX/ESI/EDI preserved.
void run_220640(LoopContext &c,Abi &a){
    auto &m=c.m;auto &g=a.gp;
    g[4]-=4;store(m,c.o,0x220640u,g[4],g[3]);           // push ebx
    g[4]-=4;store(m,c.o,0x220641u,g[4],g[6]);           // push esi
    g[6]=u32(m,sound_state_g);                          // esi = sound-state globals
    g[3]=g[0];                                          // ebx = eax
    bool done=false;
    g[0]=lo8(g[0],u8(m,g[6]+0x78u));if((g[0]&0xffu)==0)done=true;
    if(!done){g[0]=lo8(g[0],u8(m,g[6]+0x79u));if((g[0]&0xffu)==0)done=true;}
    if(!done){g[0]=lo8(g[0],u8(m,g[6]+0x7au));if((g[0]&0xffu)==0)done=true;}
    if(!done && g[3]==0xffffffffu)done=true;
    if(!done){
        object_loop_invoke(c,a,0x220669u,0x220370u);    // call 220370 -> eax
        if(g[0]==0xffffffffu)done=true;
    }
    if(!done){
        g[4]-=4;store(m,c.o,0x22066eu,g[4],g[7]);        // push edi
        object_loop_invoke(c,a,0x220674u,0x220320u);     // call 220320 -> eax
        g[7]=g[0];                                        // edi = eax
        if(g[7]!=0u){
            g[0]=lo8(g[0],u8(m,g[6]+0x7fu));
            g[1]=u32(m,g[4]+0x10u);                       // ecx = [0x10+esp] (arg0)
            g[0]=lo8(g[0],g[0]&1u);                       // al &= 1
            const std::uint32_t args[]{g[1]};
            object_loop_invoke(c,a,0x220689u,0x2208e0u,args,true); // call 2208e0 (ret 4)
        }
        g[7]=u32(m,g[4]);g[4]+=4;                          // pop edi
    }
    g[6]=u32(m,g[4]);g[4]+=4;                              // pop esi
    g[3]=u32(m,g[4]);g[4]+=4;                              // pop ebx
    pop_return(m,a,4);
}

// ===========================================================================
// 190460 (34 instrs): whether any active camera view is within XMM3 (squared)
// of the source position at ESI. Reads native views. Returns AL (1 found),
// EDX the view count scanned. SSE arithmetic; no calls.
void run_190460(LoopContext &c,Abi &a){
    Floating floating(a,c.fp);
    auto &m=c.m;auto &g=a.gp;
    g[1]=u32(m,sound_state_g);                           // ecx = sound-state
    g[0]=lo8(g[0],0u);                                   // al = 0
    g[2]=0u;                                             // edx = 0
    set_xmm(a,3,_mm_mul_ss(xmm(a,3),xmm(a,3)));          // xmm3 *= xmm3
    g[1]+=0xc0u;                                         // ecx += 0xc0
    g[4]-=4;store(m,c.o,0x190474u,g[4],g[3]);            // push ebx
    bool found=false;
    for(;;){
        g[3]=lo8(g[3],view_read(c,0x190475u,g[1]-0x32u,1)); // bl = view active
        if((g[3]&0xffu)!=0u){
            __m128 x0=_mm_sub_ss(ss(view_read(c,0x19047cu,g[1]-8u,4)),ss(u32(m,g[6])));
            set_xmm(a,0,x0);
            __m128 x1=_mm_sub_ss(ss(view_read(c,0x190485u,g[1]-4u,4)),ss(u32(m,g[6]+4u)));
            set_xmm(a,1,x1);
            __m128 x2=_mm_sub_ss(ss(view_read(c,0x19048fu,g[1],4)),ss(u32(m,g[6]+8u)));
            set_xmm(a,2,x2);
            __m128 x4=_mm_mul_ss(x0,x0);                 // xmm4 = dx*dx
            x0=_mm_mul_ss(x1,x1);x4=_mm_add_ss(x4,x0);   // + dy*dy
            x0=_mm_mul_ss(x2,x2);x4=_mm_add_ss(x4,x0);   // + dz*dz
            set_xmm(a,0,x0);set_xmm(a,4,x4);
            if(!comiss(xmm(a,3),x4).cf){found=true;break;} // comiss xmm3,xmm4; jnb
        }
        g[2]+=1u;g[1]+=0x48u;                            // edx++, ecx += 0x48
        if(std::int32_t(g[2])>=4)break;                  // cmp edx,4; jge exits loop
    }
    if(found)g[0]=lo8(g[0],1u);                          // al = 1
    g[3]=u32(m,g[4]);g[4]+=4;                            // pop ebx
    pop_return(m,a);
}

// ===========================================================================
// 18FFB0 (75 instrs): map a loop-sound pool slot to its live object-record
// datum (16EA60 picks the first slot; then scan the matching object-component
// bitfield for a slot whose owner differs). RET 4. Returns EAX (datum or -1).
void run_18ffb0(LoopContext &c,Abi &a){
    auto &m=c.m;auto &g=a.gp;
    g[4]-=4;store(m,c.o,0x18ffb0u,g[4],g[5]);            // push ebp
    g[5]=u32(m,loop_pool_g);                             // ebp = loop pool
    g[2]=0u;                                             // xor edx,edx
    g[1]=g[5];                                           // ecx = ebp
    object_loop_invoke(c,a,0x18ffc0u,0x16ea60u);         // call 16ea60 -> eax
    g[1]=g[0];                                           // ecx = eax
    g[0]=0xffffffffu;                                    // eax |= -1
    if(g[1]!=0xffffffffu){                               // cmp ecx,-1; jnz
        g[0]=u32(m,g[5]+0x24u);                          // eax = [0x24+ebp]
        g[2]=u32(m,g[5]+0x44u);                          // edx = [0x44+ebp]
        g[0]=std::uint32_t(simul(g[0],g[1]));            // imul eax,ecx
        g[0]=sx16(u16(m,g[0]+g[2]));                     // movsx eax,word[eax+edx]
        g[0]<<=16;
        g[0]|=g[1];
    }
    // L18ffdc
    if(g[0]==0xffffffffu){                               // cmp eax,-1; jz L190079
        g[5]=u32(m,g[4]);g[4]+=4;pop_return(m,a,4);return;
    }
    g[4]-=4;store(m,c.o,0x18ffe5u,g[4],g[3]);            // push ebx
    g[4]-=4;store(m,c.o,0x18ffe6u,g[4],g[6]);            // push esi
    g[4]-=4;store(m,c.o,0x18ffe7u,g[4],g[7]);            // push edi
    for(;;){                                             // L18fff0
        g[2]=u32(m,g[5]+0x44u);                          // edx = rows
        g[7]=u32(m,g[4]+0x14u);                          // edi = arg0
        g[1]=g[0]&0xffffu;                               // ecx = eax & 0xffff
        g[6]=g[1]+g[1]*2u;                               // esi = ecx*3
        g[2]=g[2]+g[6]*8u;                               // edx = rows + esi*8
        g[6]=u32(m,g[2]+0xcu);                           // esi = [0xc+edx]
        bool to_190076=false;
        if(g[6]==g[7]){                                  // cmp esi,edi; jz
            g[2]=lo8(g[2],u8(m,g[2]+4u));                // dl = [4+edx]
            g[2]=lo8(g[2],(g[2]&0xffu)>>5);              // shr dl,5
            if((g[2]&1u)!=0u)to_190076=true;             // test dl,1; jnz L190076
        }
        if(!to_190076){
            // L190017
            if(g[0]==0xffffffffu)g[0]=0u;                // eax==-1 -> xor eax,eax
            else g[0]=1u+g[1];                           // else eax = 1+ecx
            g[2]=0xffffffffu;                            // edx |= -1
            bool to_190053=false;
            if(std::int32_t(g[0])<0)to_190053=true;      // test eax; js L190053
            if(!to_190053){
                g[7]=u32(m,g[5]+0x38u);                  // edi = [0x38+ebp]
                if(std::int32_t(g[0])>=std::int32_t(g[7]))to_190053=true; // cmp eax,edi; jge
            }
            if(!to_190053){
                g[6]=u32(m,g[5]+0x48u);                  // esi = [0x48+ebp]
                for(;;){                                 // L190034
                    g[1]=g[0]&0x1fu;
                    g[3]=1u<<(g[1]&31u);                 // ebx = 1<<cl
                    g[1]=std::uint32_t(std::int32_t(g[0])>>5); // ecx = eax>>5 (arith)
                    if((u32(m,g[6]+g[1]*4u)&g[3])!=0u){  // test [esi+ecx*4],ebx; jnz
                        g[2]=g[0];                        // L190051: edx = eax
                        break;
                    }
                    g[0]+=1u;                             // inc eax
                    if(std::int32_t(g[0])>=std::int32_t(g[7]))break; // cmp eax,edi; jge L190053
                }
            }
            // L190053
            g[0]=0xffffffffu;                            // eax |= -1
            if(g[2]!=0xffffffffu){                       // cmp edx,-1; jnz
                g[1]=u32(m,g[5]+0x24u);                  // ecx = [0x24+ebp]
                g[0]=u32(m,g[5]+0x44u);                  // eax = [0x44+ebp]
                g[1]=std::uint32_t(simul(g[1],g[2]));    // imul ecx,edx
                g[0]=sx16(u16(m,g[1]+g[0]));             // movsx eax,word[ecx+eax]
                g[0]<<=16;
                g[0]|=g[2];
            }
            // L19006d
            if(g[0]!=0xffffffffu)continue;               // cmp eax,-1; jnz L18fff0
        }
        // L190076
        g[7]=u32(m,g[4]);g[4]+=4;                        // pop edi
        g[6]=u32(m,g[4]);g[4]+=4;                        // pop esi
        g[3]=u32(m,g[4]);g[4]+=4;                        // pop ebx
        break;
    }
    // L190079
    g[5]=u32(m,g[4]);g[4]+=4;                            // pop ebp
    pop_return(m,a,4);
}

// ===========================================================================
// 18F9B0 (324 instrs): build the per-source emitter record (output at ECX/EDI)
// for the loop sound whose pool index is in AX, from its sound definition and
// spatial mode. Writes the emitter struct and its filter parameters; calls
// the core game helpers for name resolution and parameter blending. RET 0.
void run_18f9b0(LoopContext &c,Abi &a){
    auto &m=c.m;auto &g=a.gp;
    g[4]-=0x80u;                                     // sub esp,0x80
    g[4]-=4;store(m,c.o,0x18f9b6u,g[4],g[3]);        // push ebx
    g[4]-=4;store(m,c.o,0x18f9b7u,g[4],g[5]);        // push ebp
    g[0]&=0xffffu;                                   // eax &= 0xffff
    g[4]-=4;store(m,c.o,0x18f9bdu,g[4],g[6]);        // push esi
    g[0]=g[0]+g[0]*2u;                               // eax *= 3
    g[4]-=4;store(m,c.o,0x18f9c1u,g[4],g[7]);        // push edi
    g[7]=g[1];                                       // edi = ecx (output ptr)
    g[1]=u32(m,loop_pool_g);                         // ecx = loop pool
    g[2]=u32(m,g[1]+0x44u);                          // edx = rows
    g[1]=u32(m,tag_instances_g);                     // ecx = tag instances
    g[6]=g[2]+g[0]*8u;                               // esi = loop record
    g[0]=(u32(m,g[6]+0xcu)&0xffffu)<<4;              // eax = (def datum & 0xffff) << 4
    g[5]=u32(m,g[0]+g[1]+8u);                        // ebp = definition data
    store16(m,c.o,0x18f9e5u,g[7],0u);               // [edi] word = 0
    {const auto mode=u8(m,g[6]+3u);g[0]=g[6]+0x10u;if(mode!=0u)g[0]=0x44ebd8u;} // position ptr / zero vec
    g[0]=u32(m,g[0]);                                // eax = [eax]
    store(m,c.o,0x18f9fbu,g[7]+8u,g[0]);            // [8+edi] = eax
    store8(m,c.o,0x18f9feu,g[7]+2u,0u);             // [2+edi] = 0
    g[1]=lo8(g[1],u8(m,g[6]+3u));                    // cl = mode byte
    g[0]=sx8(g[1]);                                  // eax = sign_extend(cl)
    if(g[0]==0u)goto L18fb6b;                        // mode 0
    g[0]-=2u;if(g[0]==0u)goto L18faf5;               // mode 2
    g[0]-=1u;if(g[0]!=0u)goto L18fb9b;               // mode != 3
    // mode 3
    g[2]=lo8(g[2],(u8(m,g[7]+3u)&0xf1u)|1u);        // dl = ([3+edi]&0xf1)|1
    store8(m,c.o,0x18fa2au,g[7]+3u,g[2]);           // [3+edi] = dl
    g[0]=lo8(g[0],(u8(m,g[6]+4u)>>1));              // al = [4+esi] >> 1
    if((g[0]&1u)!=0u)goto L18faa5;
    g[0]=u32(m,g[6]+0x10u);                          // eax = [0x10+esi]
    g[1]=g[7]+4u;                                    // ecx = 4+edi
    {const auto edx=g[4]+0x14u;g[2]=edx;g[3]=g[7]+0xcu;   // edx = 0x18+esp(after push ecx); ebx = 0xc+edi
     const std::uint32_t args[]{edx,g[1]};
     object_loop_invoke(c,a,0x18fa4au,0x17e160u,args,true);}  // call 17e160 (ret 8)
    {const auto neg=(g[0]&0xffu)==0u?1u:0u;g[0]=lo8(g[0],neg);  // neg/sbb/inc: al = (ret==0)?1:0
     store8(m,c.o,0x18fa50u,g[4]+0x13u,neg);
     if(neg!=0u)goto L18faa5;}
    g[0]=g[4]+0x14u;                                 // lea eax,[esp+0x14]
    object_loop_invoke(c,a,0x18fa5fu,0x18bbc0u);     // call 18bbc0 (ret 0)
    g[1]=u32(m,0x4f720cu);                           // ecx = pool2
    store(m,c.o,0x18fa65u,g[7]+0x18u,g[0]);         // [0x18+edi] = eax
    g[0]=u32(m,g[6]+0x10u)&0xffffu;                  // eax = [0x10+esi] & 0xffff
    g[2]=u32(m,g[1]+0x44u);                          // edx = rows2
    g[0]=std::uint32_t(simul(g[0],0x190u));          // imul eax,0x190
    g[0]=0x30u+g[0]+g[2];                            // lea eax,[eax+edx+0x30]
    g[2]=u32(m,g[0]);                                // edx = [eax]
    g[1]=g[7]+0x1cu;                                 // ecx = 0x1c+edi
    store(m,c.o,0x18fa82u,g[1],g[2]);               // [ecx] = edx
    g[2]=u32(m,g[0]+4u);store(m,c.o,0x18fa87u,g[1]+4u,g[2]);  // [4+ecx] = [4+eax]
    g[0]=u32(m,g[0]+8u);                             // eax = [8+eax]
    g[6]=g[7]+0x28u;                                 // esi = 0x28+edi
    g[2]=g[3];                                       // edx = ebx (0xc+edi)
    store(m,c.o,0x18fa92u,g[1]+8u,g[0]);            // [8+ecx] = eax
    object_loop_invoke(c,a,0x18fa9au,0x11e600u);     // call 11e600 (ret 0)
    g[7]=u32(m,g[4]);g[4]+=4;                        // pop edi
    g[6]=u32(m,g[4]);g[4]+=4;                        // pop esi
    g[5]=u32(m,g[4]);g[4]+=4;                        // pop ebp
    g[3]=u32(m,g[4]);g[4]+=4;                        // pop ebx
    g[4]+=0x80u;pop_return(m,a);return;

  L18faa5:
    store8(m,c.o,0x18faa5u,g[7],u8(m,g[7])|2u);     // [edi] |= 2
    set_xmm(a,0,_mm_setzero_ps());
    store(m,c.o,0x18faabu,g[7]+4u,ssb(xmm(a,0)));   // [4+edi] = 0.0
    g[1]=0u;                                         // ecx = 0
    g[2]=g[7]+0xcu;                                  // edx = 0xc+edi
    for(unsigned k=0;k<=0x20u;k+=4u)store(m,c.o,0x18fab5u,g[2]+k,g[1]); // 9 dwords = 0
  L18facf:
    g[2]=u32(m,0x474ce8u);                           // edx = [0x474ce8]
    g[6]=g[7]+0x28u;                                 // esi = 0x28+edi
    object_loop_invoke(c,a,0x18faddu,0x11e600u);     // call 11e600 (ret 0)
    g[0]=u32(m,0x474d10u);                           // eax = [0x474d10]
    object_loop_invoke(c,a,0x18fae7u,0x18bbc0u);     // call 18bbc0 (ret 0)
    store(m,c.o,0x18fae7u,g[7]+0x18u,g[0]);         // [0x18+edi] = eax
    g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
    g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x80u;pop_return(m,a);return;

  L18faf5:
    g[0]=lo8(g[0],(u8(m,g[7]+3u)&0xf1u)|1u);        // al = ([3+edi]&0xf1)|1
    store8(m,c.o,0x18fafcu,g[7]+3u,g[0]);           // [3+edi] = al
    g[1]=lo8(g[1],u8(m,g[6]+4u)>>1);                // cl = [4+esi] >> 1
    if((g[1]&1u)!=0u)goto L18fb3c;
    {
        g[6]=u32(m,g[6]+0x10u);                      // esi = [0x10+esi]
        g[1]=u32(m,0x52d68cu);                       // ecx = [0x52d68c]
        g[2]=g[7]+4u;                                // edx = 4+edi
        g[6]&=0xffffu;
        g[4]-=4;store(m,c.o,0x18fb1bu,g[4],g[2]);    // push edx (0x4+edi)
        g[2]=u32(m,g[1]+0x44u);                      // edx = rows
        g[0]=g[6]+g[6]*4u;                           // eax = esi*5
        g[7]+=0xcu;                                  // edi += 0xc
        g[0]<<=5;                                    // eax <<= 5
        g[4]-=4;store(m,c.o,0x18fb28u,g[4],g[7]);    // push edi
        g[0]+=g[2];                                  // eax += edx
        g[4]-=4;store(m,c.o,0x18fb2bu,g[4],g[0]);    // push eax
        // 3 args pushed; call 230b10 (ret 0xc pops them through the live world)
        g[4]-=4;store(m,c.o,0x18fb2cu,g[4],0x18fb31u);
        object_loop_call(c,a,0x230b10u);
        g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
        g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
        g[4]+=0x80u;pop_return(m,a);return;
    }
    // fallthrough to L18fb3c
  L18fb3c:
    store8(m,c.o,0x18fb3cu,g[7],u8(m,g[7])|2u);     // [edi] |= 2
    set_xmm(a,0,_mm_setzero_ps());
    store(m,c.o,0x18fb42u,g[7]+4u,ssb(xmm(a,0)));   // [4+edi] = 0.0
    g[2]=0u;                                         // edx = 0
    g[0]=g[7]+0xcu;                                  // eax = 0xc+edi
    for(unsigned k=0;k<=0x20u;k+=4u)store(m,c.o,0x18fb4cu,g[0]+k,g[2]);
    goto L18facf;

  L18fb6b:
    if((g[1]&0xffu)!=0u)goto L18fb9b;                // test cl,cl; jnz
    g[0]=sx16(u16(m,g[6]+0x16u));                    // eax = movsx [0x16+esi]
    if(g[0]==0xffffffffu)goto L18fb9b;               // cmp eax,-1; jz
    store16(m,c.o,0x18fb78u,g[7]+0x2cu,g[0]);       // [0x2c+edi] = ax
    store(m,c.o,0x18fb7cu,g[7]+0x28u,0xffffffffu);  // [0x28+edi] = -1
    if(u8(m,g[6]+3u)!=0u)g[0]=0xffffffffu;           // [3+esi]!=0 -> eax = -1
    else g[0]=sx8(u8(m,g[6]+0x15u));                 // else eax = movsx [0x15+esi]
    store8(m,c.o,0x18fb93u,g[7]+1u,u8(m,g[7]+1u)|0x10u); // [1+edi] |= 0x10
    store16(m,c.o,0x18fb97u,g[7]+0x2eu,g[0]);       // [0x2e+edi] = ax
    // fallthrough to L18fb9b

  L18fb9b:
    if((u8(m,g[6]+4u)&1u)!=0u)goto L18fcb7;          // [4+esi]&1 -> L18fcb7
    {
        if(u8(m,g[6]+3u)!=1u)g[0]=0xffffffffu;       // [3+esi]!=1 -> eax=-1
        else g[0]=u32(m,g[6]+0x10u);                 // else eax = [0x10+esi]
        g[1]=u32(m,0x4ecbd0u);                       // ecx = [0x4ecbd0]
        g[2]=u32(m,g[1]+0x44u);                      // edx = rows3
        g[0]&=0xffffu;g[0]=g[0]+g[0]*2u;             // eax = (eax&0xffff)*3
        g[1]=lo8(g[1],u8(m,g[2]+g[0]*4u+3u));        // cl = [3+edx+eax*4]
        g[3]=1u<<(g[1]&0xffu&31u);                   // ebx = 1 << cl
        if(((g[3]>>8)&0x4u)==0u)goto L18fc8b;        // test bh,4; jz
        g[0]=u32(m,g[6]+0x10u);                      // eax = [0x10+esi]
        set_xmm(a,0,_mm_setzero_ps());               // xmm0 = 0
        g[0]&=0xffffu;
        g[1]=g[0]+g[0]*2u;                           // ecx = eax*3
        g[3]=u32(m,g[2]+g[1]*4u+8u);                 // ebx = [8+edx+ecx*4] (record3)
        g[2]=lo8(g[2],u8(m,g[3]+0x12cu));            // dl = [0x12c+ebx]
        store8(m,c.o,0x18fbf0u,g[7]+2u,g[2]);        // [2+edi] = dl
        g[0]=u32(m,g[3]+0x130u);                     // eax = [0x130+ebx]
        store(m,c.o,0x18fbf9u,g[7]+0x30u,g[0]);      // [0x30+edi] = eax
        set_xmm(a,1,ss(u32(m,g[3]+0x134u)));         // xmm1 = [0x134+ebx]
        {const auto f=comiss_fp(c,a,xmm(a,1),xmm(a,0));
         if(!(f.cf||f.zf)){
             store8(m,c.o,0x18fc09u,g[7],u8(m,g[7])|4u);
             g[1]=u32(m,g[3]+0x134u);store(m,c.o,0x18fc12u,g[7]+0x34u,g[1]);}}
        // L18fc15
        set_xmm(a,1,ss(u32(m,g[3]+0x138u)));
        {const auto f=comiss_fp(c,a,xmm(a,1),xmm(a,0));
         if(!(f.cf||f.zf)){
             store8(m,c.o,0x18fc22u,g[7],u8(m,g[7])|8u);
             g[2]=u32(m,g[3]+0x138u);store(m,c.o,0x18fc2bu,g[7]+0x38u,g[2]);}}
        // L18fc2e
        set_xmm(a,1,ss(u32(m,g[3]+0x13cu)));
        {const auto f=comiss_fp(c,a,xmm(a,1),xmm(a,0));
         if(!(f.cf||f.zf)){
             store8(m,c.o,0x18fc3bu,g[7],u8(m,g[7])|0x10u);
             set_xmm(a,1,ss(u32(m,g[3]+0x13cu)));
             object_loop_invoke(c,a,0x18fc4bu,0x18b580u,{},true);   // call 18b580
             set_xmm(a,0,_mm_setzero_ps());
             store16(m,c.o,0x18fc4eu,g[7]+0x3cu,g[0]);}}
        // L18fc52
        set_xmm(a,1,ss(u32(m,g[3]+0x140u)));
        {const auto f=comiss_fp(c,a,xmm(a,1),xmm(a,0));
         if(!(f.cf||f.zf)){
             store8(m,c.o,0x18fc5fu,g[7],u8(m,g[7])|0x20u);
             set_xmm(a,1,ss(u32(m,g[3]+0x140u)));
             object_loop_invoke(c,a,0x18fc6fu,0x18b580u,{},true);
             set_xmm(a,0,_mm_setzero_ps());
             store16(m,c.o,0x18fc72u,g[7]+0x3eu,g[0]);}}
        // L18fc76
        {const auto f=comiss_fp(c,a,xmm(a,0),ss(u32(m,g[3]+0x144u)));
         if(!(f.cf||f.zf)){
             store8(m,c.o,0x18fc7fu,g[7],u8(m,g[7])|0x40u);
             g[0]=u32(m,g[3]+0x144u);store(m,c.o,0x18fc88u,g[7]+0x40u,g[0]);}}
    }
  L18fc8b:
    if(u8(m,g[6]+3u)==1u){
        g[2]=u32(m,g[6]+0x14u);                      // edx = [0x14+esi]
        g[0]=u32(m,g[6]+0x10u);                      // eax = [0x10+esi]
        g[1]=g[7]+4u;                                // ecx = 4+edi
        const std::uint32_t args[]{g[0],g[2],g[1]};  // push ecx,edx,eax -> arg0=eax,arg1=edx,arg2=ecx
        object_loop_invoke(c,a,0x18fca2u,0xbcb70u,args,true);
    } else {
        g[2]=u32(m,g[6]+0x14u);                      // edx = [0x14+esi]
        g[1]=g[7]+4u;                                // ecx = 4+edi
        g[0]=0xffffffffu;                            // eax |= -1
        const std::uint32_t args[]{g[0],g[2],g[1]};
        object_loop_invoke(c,a,0x18fcb5u,0xbcb70u,args,true);
    }
    goto L18fcbd;
  L18fcb7:
    g[0]=u32(m,g[6]+8u);                             // eax = [8+esi]
    store(m,c.o,0x18fcbau,g[7]+4u,g[0]);            // [4+edi] = eax
  L18fcbd:
    if(u8(m,g[6]+3u)!=1u)goto L18fd94;
    g[1]=0u;                                         // ecx = 0
    g[1]=lo16(g[1],u16(m,g[6]+6u));                 // cx = [6+esi]
    g[0]=0u;                                         // eax = 0
    if(std::uint16_t(g[1])!=0xffffu){                // cmp cx,-1; jnz
        g[0]=g[1];                                   // eax = ecx
        g[1]=u32(m,g[6]+0x10u);                      // ecx = [0x10+esi]
        object_loop_invoke(c,a,0x18fcdfu,0xbac70u);  // call bac70 (ret 0)
    }
    // L18fcdf
    g[1]=u32(m,g[6]+0x10u);                          // ecx = [0x10+esi]
    g[4]-=4;store(m,c.o,0x18fce2u,g[4],1u);          // push 1
    g[4]-=4;store(m,c.o,0x18fce4u,g[4],g[0]);        // push eax
    g[4]-=4;store(m,c.o,0x18fce5u,g[4],g[1]);        // push ecx
    g[1]=lo8(g[1],0u);                               // cl = 0
    g[3]=g[4]+0x2cu;                                 // ebx = 0x2c+esp (after 3 pushes)
    g[4]-=4;store(m,c.o,0x18fcecu,g[4],0x18fcf1u);
    object_loop_call(c,a,0xbad60u);                  // call bad60 (ret 0xc pops the 3 args)
    g[2]=u32(m,g[4]+0x80u);                          // edx = [0x80+esp]
    g[0]=u32(m,g[4]+0x84u);                          // eax = [0x84+esp]
    g[1]=g[7]+0xcu;                                  // ecx = 0xc+edi
    store(m,c.o,0x18fd02u,g[1],g[2]);               // [ecx] = edx
    g[2]=u32(m,g[4]+0x88u);                          // edx = [0x88+esp]
    store(m,c.o,0x18fd0bu,g[1]+4u,g[0]);            // [4+ecx] = eax
    g[0]=g[4]+0x5cu;                                 // eax = 0x5c+esp
    store(m,c.o,0x18fd12u,g[1]+8u,g[2]);            // [8+ecx] = edx
    object_loop_invoke(c,a,0x18fd1au,0x18bbc0u);     // call 18bbc0 (ret 0)
    store(m,c.o,0x18fd1au,g[7]+0x18u,g[0]);         // [0x18+edi] = eax
    g[0]=lo8(g[0],u8(m,g[6]+3u));                    // al = [3+esi]
    g[3]=lo8(g[3],1u);                               // bl = 1
    if((g[0]&0xffu)==1u)g[0]=u32(m,g[6]+0x10u);      // al==1 -> eax=[0x10+esi]
    else g[0]=0xffffffffu;                           // else eax=-1
    g[4]-=4;store(m,c.o,0x18fd2eu,g[4],0u);          // push 0 (arg)
    g[1]=g[7]+0x1cu;                                 // ecx = 0x1c+edi
    g[4]-=4;store(m,c.o,0x18fd33u,g[4],0x18fd38u);
    object_loop_call(c,a,0xbc200u);                  // call bc200 (ret 4 pops the arg)
    if(u8(m,g[6]+3u)==1u)g[1]=u32(m,g[6]+0x10u);     // ecx = [0x10+esi] / -1
    else g[1]=0xffffffffu;
    g[0]=g[7]+0x28u;                                 // eax = 0x28+edi
    object_loop_invoke(c,a,0x18fd4du,0xbc330u);      // call bc330 (ret 0)
    if(u8(m,g[6]+3u)==1u)g[0]=u32(m,g[6]+0x10u);     // eax = [0x10+esi] / -1
    else g[0]=0xffffffffu;
    g[4]-=4;store(m,c.o,0x18fd5au,g[4],0u);          // push 0
    g[1]=g[4]+0x17u;                                 // ecx = 0x17+esp (esp after push 0)
    g[4]-=4;store(m,c.o,0x18fd60u,g[4],g[1]);        // push ecx
    g[4]-=4;store(m,c.o,0x18fd61u,g[4],g[0]);        // push eax
    store8(m,c.o,0x18fd62u,g[4]+0x1fu,g[3]);        // [0x1f+esp] = bl
    g[4]-=4;store(m,c.o,0x18fd66u,g[4],0x18fd6bu);
    object_loop_call(c,a,0x1902e0u);                 // call 1902e0 (ret 0xc pops the 3 args)
    g[2]=lo8(g[2],u8(m,g[7]+3u));                    // dl = [3+edi]
    g[1]=lo8(g[1],u8(m,g[4]+0x13u));                // cl = [0x13+esp]
    g[3]=lo8(g[3],u8(m,g[7]+3u));                    // bl = [3+edi]
    g[2]=lo8(g[2],(g[2]^g[1])&0xffu);               // dl ^= cl
    g[2]=lo8(g[2],g[2]&0xfu);                        // dl &= 0xf
    g[3]=lo8(g[3],(g[3]^g[2])&0xffu);               // bl ^= dl
    store8(m,c.o,0x18fd7cu,g[7]+3u,g[3]);           // [3+edi] = bl
    g[0]=lo8(g[0],g[3]&0xffu);                       // al = bl
    if((u8(m,g[5])&8u)==0u)goto L18fd9c;             // test [ebp]&8; jz
    if((g[0]&0xfu)!=0u)goto L18fd9c;                 // test al,0xf; jnz
    g[0]=lo8(g[0],(g[0]&0xf1u)|1u);                 // al = (al&0xf1)|1
    store8(m,c.o,0x18fd8fu,g[7],u8(m,g[7])|1u);     // [edi] |= 1
    goto L18fd99;
  L18fd94:
    g[0]=lo8(g[0],u8(m,g[7]+3u)&0xf0u);             // al = [3+edi]&0xf0
  L18fd99:
    store8(m,c.o,0x18fd99u,g[7]+3u,g[0]);           // [3+edi] = al
  L18fd9c:
    g[0]=lo8(g[0],u8(m,g[6]+4u)>>5);               // al = [4+esi]>>5
    if((g[0]&1u)!=0u)store8(m,c.o,0x18fda6u,g[7]+1u,u8(m,g[7]+1u)|4u); // [1+edi]|=4
    // L18fdaa epilogue
    g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
    g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x80u;pop_return(m,a);
}

// ===========================================================================
// 18D5A0 (301 instrs): the per-source object loop-sound update. For the loop
// whose index is the stack argument, it resolves the source's definition and
// spatial state (18F9B0), applies camera-view culling (190460), publishes the
// loop-sound request through the live world (220C60, at two sites), and starts
// or stops the platform voice. RET 4.
void run_18d5a0(LoopContext &c,Abi &a){
    auto &m=c.m;auto &g=a.gp;
    g[4]-=0xacu;                                     // sub esp,0xac
    g[2]=u32(m,tag_instances_g);                     // edx = tag instances
    g[4]-=4;store(m,c.o,0x18d5acu,g[4],g[3]);        // push ebx
    g[4]-=4;store(m,c.o,0x18d5adu,g[4],g[5]);        // push ebp
    g[4]-=4;store(m,c.o,0x18d5aeu,g[4],g[6]);        // push esi
    g[4]-=4;store(m,c.o,0x18d5afu,g[4],g[7]);        // push edi
    g[7]=u32(m,g[4]+0xc0u);                          // edi = arg0 (loop index)
    g[0]=g[7]&0xffffu;
    g[6]=g[0]+g[0]*2u;                               // esi = idx*3
    g[0]=u32(m,loop_pool_g);                         // eax = loop pool
    g[5]=u32(m,g[0]+0x44u);                          // ebp = rows
    g[6]<<=3;                                        // esi = idx*0x18
    g[1]=u32(m,g[5]+g[6]+0xcu);                      // ecx = def datum
    set_xmm(a,0,ss(u32(m,g[5]+g[6]+8u)));            // xmm0 = [8+rows+esi]
    g[5]=g[5]+g[6];                                  // ebp = loop record
    g[1]=(g[1]&0xffffu)<<4;
    g[0]=u32(m,g[1]+g[2]+8u);                        // eax = definition data
    store(m,c.o,0x18d5e5u,g[4]+0x14u,g[0]);         // [0x14+esp] = def data
    store(m,c.o,0x18d5e9u,g[4]+0x24u,ssb(xmm(a,0))); // [0x24+esp] = xmm0
    g[0]=lo16(g[0],u16(m,g[5]+4u));                 // ax = [4+ebp]
    g[3]=lo8(g[3],((g[0]>>8)&1u));                   // bl = ah & 1
    g[1]=lo8(g[1],g[0]&0xffu);                       // cl = al
    store8(m,c.o,0x18d5fdu,g[4]+0x1bu,g[3]);        // [0x1b+esp] = bl
    if((g[1]&1u)!=0u)goto L18d631;                   // test cl,1; jnz
    if(u8(m,g[5]+3u)!=1u)goto L18d61d;               // cmp [3+ebp],1; jnz
    g[2]=u32(m,g[5]+0x14u);                          // edx = [0x14+ebp]
    g[0]=u32(m,g[5]+0x10u);                          // eax = [0x10+ebp]
    g[1]=g[4]+0x24u;                                 // ecx = 0x24+esp
    {const std::uint32_t args[]{g[0],g[2],g[1]};     // push ecx,edx,eax
     object_loop_invoke(c,a,0x18d61bu,0xbcb70u,args,true);}
    goto L18d637;
  L18d61d:
    g[2]=u32(m,g[5]+0x14u);                          // edx = [0x14+ebp]
    g[1]=g[4]+0x24u;                                 // ecx = 0x24+esp
    g[0]=0xffffffffu;                                // eax |= -1
    {const std::uint32_t args[]{g[0],g[2],g[1]};
     object_loop_invoke(c,a,0x18d62fu,0xbcb70u,args,true);}
    goto L18d637;
  L18d631:
    g[0]=lo8(g[0],(g[0]&0xffu)>>1);                 // shr al,1
    g[0]=lo8(g[0],~g[0]);                            // not al
    g[0]=lo8(g[0],g[0]&1u);                          // and al,1
  L18d637:
    store8(m,c.o,0x18d637u,g[4]+0x13u,g[0]);        // [0x13+esp] = al
    g[0]=u32(m,loop_pool_g);                         // eax = loop pool
    g[0]=u32(m,g[0]+0x44u);                          // eax = rows
    g[1]=lo8(g[1],u8(m,g[0]+g[6]+3u));              // cl = [3+rows+esi]
    g[0]=g[0]+g[6];                                  // eax = rows+esi (record)
    store(m,c.o,0x18d64bu,g[4]+0x20u,g[7]);         // [0x20+esp] = edi (loop index)
    if((g[1]&0xffu)==0u){                            // test cl,cl; jnz L18d666 (so block runs when cl==0)
        g[1]=sx8(u8(m,g[0]+0x14u));                   // movsx ecx,[0x14+rows+esi]
        if(g[1]!=0xffffffffu){                        // cmp ecx,-1; jz L18d666
            g[0]=u32(m,g[0]+0xcu)|0x8000u;            // eax = [0xc+rec] | 0x8000
            store(m,c.o,0x18d662u,g[4]+0x20u,g[0]);  // [0x20+esp] = eax
        }
    }
  L18d666:
    g[1]=u32(m,g[4]+0x14u);                          // ecx = [0x14+esp] (def data)
    g[2]=u32(m,g[1]);                                // edx = [ecx]
    store(m,c.o,0x18d66fu,g[4]+0x1cu,0xffffffffu);  // [0x1c+esp] = -1
    if((g[2]&0x10u)!=0u && u8(m,g[5]+3u)==1u)goto L18d68a;  // dl&0x10 and [3+ebp]==1
  L18d67f:
    if((g[2]&0x20u)==0u)goto L18d69d;                // test dl,0x20; jz
    if(u8(m,g[5]+3u)!=1u)goto L18d692;               // cmp [3+ebp],1; jnz
  L18d68a:
    g[0]=u32(m,g[5]+0x10u);                          // eax = [0x10+ebp]
    if(g[0]!=0xffffffffu)goto L18d699;               // cmp eax,-1; jnz
  L18d692:
    g[0]=g[7]|0x2000u;                               // eax = edi | 0x2000
  L18d699:
    store(m,c.o,0x18d699u,g[4]+0x1cu,g[0]);         // [0x1c+esp] = eax
  L18d69d:
    g[0]=lo8(g[0],u8(m,g[4]+0x13u));               // al = [0x13+esp]
    if((g[0]&0xffu)!=0u)goto L18d6c5;                // test al,al; jnz
    g[0]=lo8(g[0],u8(m,g[5]+2u));                    // al = [2+ebp]
    if((g[0]&0xffu)==3u)goto L18d816;                // cmp al,3; jz
    if((g[3]&0xffu)!=0u)goto L18d6c5;                // test bl,bl; jnz
    if((g[0]&0xffu)==3u)goto L18d816;                // cmp al,3; jz
    store8(m,c.o,0x18d6bcu,g[5]+2u,3u);             // [2+ebp] = 3
    goto L18d816;
  L18d6c5:
    g[1]=lo8(g[1],u8(m,g[5]+3u));                    // cl = [3+ebp]
    {const auto notdl=(~(g[2]&0xffu))&0xffu;         // al = ~dl
     g[0]=lo8(g[0],notdl);
     g[3]=notdl;}                                     // ebx = movzx al
    g[0]=1u<<((g[1]&0xffu)&31u);                      // eax = 1 << cl
    g[3]=(g[3]>>3)&1u;                                // shr ebx,3; and ebx,1
    if((g[0]&0xeu)!=0u && (g[2]&0x40u)==0u)g[3]|=2u;  // or ebx,2
    else g[3]&=0xfffffffdu;                           // else clear bit 1
  L18d6ed:
    g[1]=g[4]+0x34u;                                 // ecx = 0x34+esp (output ptr)
    g[0]=g[7];                                        // eax = edi (loop index)
    store16(m,c.o,0x18d6f3u,g[4]+0x34u,0u);         // [0x34+esp] word = 0
    object_loop_invoke(c,a,0x18d6ffu,0x18f9b0u,{},true); // call 18f9b0
    // 18d6ff
    g[1]=lo8(g[1],u8(m,g[4]+0x34u));                // cl = [0x34+esp]
    g[2]=lo8(g[2],(g[1]&0xffu)>>3);                 // dl = cl >> 3
    if((g[2]&1u)!=0u)set_xmm(a,3,ss(u32(m,g[4]+0x6cu))); // xmm3 = [0x6c+esp]
    else{g[0]=u32(m,g[4]+0x14u);set_xmm(a,3,ss(u32(m,g[0]+0x10u)));} // eax=[0x14+esp]; xmm3=[0x10+eax]
    // L18d71e
    g[0]=0u;g[0]=lo16(g[0],u16(m,g[5]+4u));         // eax=0; ax=[4+ebp]
    g[7]=(g[0]&0xffu)>>4;                            // edi = al >> 4
    g[2]=lo8(g[2],(g[0]&0xffu)>>7);                 // dl = al >> 7
    g[7]&=1u;                                        // edi &= 1
    store(m,c.o,0x18d735u,g[4]+0x14u,g[0]);         // [0x14+esp] = eax
    if((g[2]&1u)!=0u)g[7]|=4u;                       // edi |= 4
    else g[7]&=0xfffffffbu;                          // edi &= ~4
    // L18d743
    if(u8(m,g[5]+3u)==4u)g[7]|=0x10u;               // [3+ebp]==4 -> edi|=0x10
    else g[7]&=0xffffffefu;                          // else edi &= ~0x10
    // L18d751
    if((g[3]&2u)!=0u){                               // test bl,2
        g[1]=lo8(g[1],(g[1]&0xffu)>>1);             // cl >>= 1
        if((g[1]&1u)==0u){                           // test cl,1; jz -> run 190460
            g[6]=g[4]+0x40u;                          // esi = 0x40+esp
            object_loop_invoke(c,a,0x18d766u,0x190460u,{},true); // call 190460
            if((g[0]&0xffu)==0u){                     // test al,al; jz
                store8(m,c.o,0x18d76au,g[5]+5u,u8(m,g[5]+5u)&0xfeu); // [5+ebp]&=0xfe
                goto L18d80f;
            }
            g[0]=u32(m,g[4]+0x14u);                   // L18d773: eax=[0x14+esp]
        }
    }
  L18d777:
    g[1]=lo8(g[1],u8(m,g[4]+0x13u));               // cl = [0x13+esp]
    if((g[1]&0xffu)==0u)goto L18d8ea;               // test cl,cl; jz
    {
     g[0]=u32(m,0x51df50u);                          // eax = [0x51df50]
     bool to7c4=(g[0]==0u);
     if(!to7c4){
         g[1]=lo8(g[1],u8(m,g[0]+5u));              // cl = [5+eax]
         if((g[1]&0xffu)==0u)to7c4=true;
         else if(u8(m,g[5]+3u)!=1u)to7c4=true;
     }
     if(!to7c4){
         g[1]=u32(m,0x4ecbd0u);                       // ecx = [0x4ecbd0]
         g[0]=u32(m,g[5]+0x10u);                      // eax = [0x10+ebp]
         g[2]=u32(m,g[1]+0x44u);                      // edx = [0x44+ecx]
         g[0]=(g[0]&0xffffu);g[0]=g[0]+g[0]*2u;       // eax = (eax&0xffff)*3
         g[1]=1u;store(m,c.o,0x18d7b2u,g[4]+0x14u,1u); // ecx=1; [0x14+esp]=1
         g[1]=lo8(g[1],u8(m,g[2]+g[0]*4u+3u));        // cl = [3+edx+eax*4]
         g[0]=u32(m,g[4]+0x14u);                      // eax = [0x14+esp] (=1)
         g[0]=g[0]<<((g[1]&0xffu)&31u);               // shl eax,cl
         if((g[0]&2u)!=0u)goto L18d7da;               // test al,2; jnz
     }
    }
  L18d7c4:
    g[1]=lo8(g[1],u8(m,g[5]+2u));                    // cl = [2+ebp]
    g[2]=1u<<((g[1]&0xffu)&31u);                     // edx = 1 << cl
    if((g[2]&3u)!=0u)goto L18d7da;                   // test dl,3; jnz
    g[0]=0u;store8(m,c.o,0x18d7d5u,g[5]+2u,0u);     // eax=0; [2+ebp]=0
    goto L18d7e2;
  L18d7da:
    g[0]=1u;store8(m,c.o,0x18d7dfu,g[5]+2u,1u);     // eax=1; [2+ebp]=1
  L18d7e2:
    g[6]=u32(m,g[4]+0x1cu);                          // esi = [0x1c+esp]
    g[1]=u32(m,g[4]+0x20u);                          // ecx = [0x20+esp]
    g[2]=u32(m,g[5]+0xcu);                           // edx = [0xc+ebp]
    {const auto lea=g[4]+0x34u;
     const std::uint32_t args[]{g[2],g[3],g[1],g[6],lea,g[0],g[7],0u};
     g[0]=lea;                                        // eax = 0x40+esp (18d7f1)
     call_out(c.m,a,c.services,c.o,0x18d7fau,0x220c60u,0x18d7ffu,args,true);}
    if((g[0]&0xffu)==0u)goto L18d878;               // test al,al; jz
    store8(m,c.o,0x18d803u,g[5]+4u,u8(m,g[5]+4u)|2u); // [4+ebp] |= 2
  L18d807:
    store8(m,c.o,0x18d807u,g[5]+2u,3u);             // [2+ebp] = 3
  L18d80b:
    store8(m,c.o,0x18d80bu,g[5]+5u,u8(m,g[5]+5u)|1u); // [5+ebp] |= 1
  L18d80f:
    g[7]=u32(m,g[4]+0xc0u);                          // edi = [0xc0+esp] (loop index)
  L18d816:
    if(u8(m,g[5]+2u)!=3u)goto L18d86b;               // cmp [2+ebp],3; jnz
    g[0]=lo8(g[0],u8(m,g[5]+4u));                    // al = [4+ebp]
    if((g[0]&0xeu)==0u)goto L18d86b;                 // test al,0xe; jz
    if((g[0]&1u)==0u)goto L18d86b;                   // test al,1; jz
    g[1]=u32(m,g[5]+0xcu);                           // ecx = [0xc+ebp]
    {const std::uint32_t args[]{g[1]};
     object_loop_invoke(c,a,0x18d830u,0x18ffb0u,args,true);} // call 18ffb0 (ret 4)
    if(g[0]!=g[7])goto L18d85e;                      // cmp eax,edi; jnz
    g[5]=u32(m,g[5]+0xcu);                           // ebp = [0xc+ebp]
    {const std::uint32_t args[]{g[5]};
     object_loop_invoke(c,a,0x18d83du,0x18ffb0u,args,true);} // call 18ffb0 (ret 4)
    if(g[0]==0xffffffffu)goto L18d85e;               // cmp eax,-1; jz
    g[0]&=0xffffu;g[2]=g[0]+g[0]*2u;                // eax&=0xffff; edx=eax*3
    g[0]=u32(m,loop_pool_g);                         // eax = loop pool
    g[1]=u32(m,g[0]+0x44u);                          // ecx = rows
    g[0]=g[1]+g[2]*8u;                               // eax = rows + edx*8
    if(u32(m,g[0]+0xcu)!=g[5])goto L18d85e;          // cmp [0xc+eax],ebp; jnz
    store8(m,c.o,0x18d85au,g[0]+4u,u8(m,g[0]+4u)&0xdfu); // [4+eax] &= 0xdf
  L18d85e:
    g[2]=u32(m,loop_pool_g);                          // edx = loop pool
    g[0]=g[7];                                        // eax = edi
    object_loop_invoke(c,a,0x18d86bu,0x16e8a0u);      // call 16e8a0 (ret 0)
  L18d86b:
    g[7]=u32(m,g[4]);g[4]+=4;                         // pop edi
    g[6]=u32(m,g[4]);g[4]+=4;                         // pop esi
    g[5]=u32(m,g[4]);g[4]+=4;                         // pop ebp
    g[3]=u32(m,g[4]);g[4]+=4;                         // pop ebx
    g[4]+=0xacu;pop_return(m,a,4);return;             // add esp,0xac; ret 4
  L18d878:
    if(g[6]==0xffffffffu)goto L18d80b;               // cmp esi,-1; jz
    {const bool mode1=(u8(m,g[5]+3u)==1u);           // cmp [3+ebp],1
     g[1]=0x11u;g[6]=g[4]+0x34u;g[7]=g[4]+0x78u;     // rep movsd setup
     const auto df=(a.eflags&0x400u)!=0u;
     while(g[1]){const auto x=u32(m,g[6]);store(m,c.o,0x18d88eu,g[7],x);
         const auto d=df?0xfffffffcu:4u;g[6]+=d;g[7]+=d;--g[1];}
     if(!mode1)goto L18d8d7;}                         // [3+ebp]!=1 -> L18d8d7
    g[3]=u32(m,g[5]+0x10u);                           // ebx = [0x10+ebp]
    g[6]=g[4]+0x84u;                                  // esi = 0x84+esp
    g[0]=g[3];                                        // eax = ebx
    object_loop_invoke(c,a,0x18d8a3u,0xbbe00u);       // call bbe00 (ret 0)
    g[7]=0u;                                          // edi = 0
    g[6]=g[4]+0x28u;                                  // esi = 0x28+esp
    g[0]=g[3];                                        // eax = ebx
    object_loop_invoke(c,a,0x18d8b0u,0xbbff0u);       // call bbff0 (ret 0)
    g[0]=g[6];                                        // eax = esi
    object_loop_invoke(c,a,0x18d8b7u,0x18bbc0u);      // call 18bbc0 (ret 0)
    store(m,c.o,0x18d8b7u,g[4]+0x4cu,g[0]);          // [0x4c+esp] = eax
    if(u8(m,g[5]+3u)==1u)g[0]=u32(m,g[5]+0x10u);      // [3+ebp]==1 -> eax=[0x10+ebp]
    else g[0]=0xffffffffu;                            // else eax=-1
    g[4]-=4;store(m,c.o,0x18d8c9u,g[4],0u);           // push 0
    g[1]=g[4]+0x98u;                                  // ecx = 0x98+esp
    g[4]-=4;store(m,c.o,0x18d8d2u,g[4],0x18d8d7u);
    object_loop_call(c,a,0xbc200u);                   // call bc200 (ret 4)
  L18d8d7:
    g[0]=g[4]+0x78u;                                  // eax = 0x78+esp
    g[4]-=4;store(m,c.o,0x18d8dbu,g[4],g[0]);         // push eax (arg)
    g[0]=u32(m,g[4]+0x20u);                           // eax = [0x20+esp]
    g[4]-=4;store(m,c.o,0x18d8e0u,g[4],0x18d8e5u);
    object_loop_call(c,a,0x220640u);                  // call 220640 (ret 4)
    goto L18d80b;
  L18d8ea:
    g[1]=lo8(g[1],g[0]&0xffu);                        // cl = al
    g[1]=lo8(g[1],(g[1]&0xffu)>>2);                  // cl >>= 2
    if((g[1]&1u)!=0u){
        object_loop_invoke(c,a,0x18d8f9u,0x1904d0u,{},true); // call 1904d0
    } else {
        set_xmm(a,0,ss(c_hundredth));                 // xmm0 = 0.01
        g[0]=lo8(g[0],(g[0]&0xffu)>>3);              // al >>= 3
        if((g[0]&1u)==0u)set_xmm(a,0,_mm_setzero_ps()); // test al,1; jz -> xmm0=0
    }
    // L18d90d
    g[0]=lo8(g[0],u8(m,g[4]+0x1bu));                // al = [0x1b+esp]
    store(m,c.o,0x18d913u,g[4]+0x14u,ssb(xmm(a,0))); // [0x14+esp] = xmm0
    if((g[0]&0xffu)==0u)goto L18d807;               // test al,al; jz
    {const auto S2=g[4];
     const std::uint32_t args[]{u32(m,g[5]+0xcu),g[3],u32(m,S2+0x20u),u32(m,S2+0x1cu),
         S2+0x34u,2u,g[7],u32(m,S2+0x14u)};
     g[2]=u32(m,S2+0x20u);                            // edx = [0x24+esp]
     g[1]=u32(m,S2+0x1cu);                            // ecx = [0x1c+esp]
     g[0]=u32(m,g[5]+0xcu);                           // eax = [0xc+ebp] (18d934)
     call_out(c.m,a,c.services,c.o,0x18d93bu,0x220c60u,0x18d940u,args,true);}
    if((g[0]&0xffu)!=0u)goto L18d807;               // test al,al; jnz
    store8(m,c.o,0x18d948u,g[5]+2u,2u);             // [2+ebp] = 2
    goto L18d80b;
}

// ===========================================================================
// 220C60 (763 instrs, RET 32 / 8 stack args): the object loop-sound request
// builder, reached from 18D5A0 through the live world's loop-sound hook. It
// resolves the source channel/gain/pitch and permutation state, selects the
// listener (12A810) and submits the request to the sound channels. Faithful
// translation; the core-game helpers it calls run through the live world.
void run_220c60(LoopContext &c,Abi &a){
    auto &m=c.m;auto &g=a.gp;
    g[4]-=0x3cu;                                     // sub esp,0x3c
    g[4]-=4;store(m,c.o,0x220c63u,g[4],g[3]);        // push ebx
    g[4]-=4;store(m,c.o,0x220c64u,g[4],g[5]);        // push ebp
    g[5]=u32(m,g[4]+0x48u);                          // ebp = arg0 (source index)
    g[4]-=4;store(m,c.o,0x220c69u,g[4],g[6]);        // push esi
    g[6]=u32(m,g[4]+0x60u);                          // esi = arg (mode)
    store8(m,c.o,0x220c71u,g[4]+0xdu,g[6]==2u?1u:0u);
    g[4]-=4;store(m,c.o,0x220c79u,g[4],g[7]);        // push edi
    if(g[5]==0xffffffffu)goto L221623;               // cmp ebp,-1; jz
    g[0]=lo8(g[0],u8(m,g[4]+0x68u));                // al = [0x68+esp]
    g[3]=1u;                                         // ebx = 1
    g[0]=lo8(g[0],g[0]&1u);                          // al &= bl
    store8(m,c.o,0x220c8bu,g[4]+0x13u,g[0]);
    g[0]=u32(m,sound_state_g);
    g[1]=lo8(g[1],u8(m,g[0]+0x78u));if((g[1]&0xffu)==0u)goto L221628;  // cl=[0x78+eax]; test
    g[1]=lo8(g[1],u8(m,g[0]+0x79u));if((g[1]&0xffu)==0u)goto L221628;
    g[1]=lo8(g[1],u8(m,g[0]+0x7au));if((g[1]&0xffu)==0u)goto L221628;
    g[1]=u32(m,tag_instances_g);
    g[2]=u32(m,g[4]+0x60u);                          // edx = [0x60+esp]
    g[0]=(g[5]&0xffffu)<<4;
    g[0]=u32(m,g[0]+g[1]+8u);                        // eax = source def data
    {const bool nib=(u8(m,g[2]+3u)&0xfu)!=0u;
     store(m,c.o,0x220cd1u,g[4]+0x18u,g[0]);
     if(!nib)goto L220cea;}
    if((u8(m,g[0])&0x80u)==0u)goto L220cea;          // cmp byte[eax],0; jns
    g[7]=g[5];
    store8(m,c.o,0x220cdeu,g[4]+0x12u,g[3]);
    g[7]|=0x6000u;
    goto L220cf3;
  L220cea:
    g[7]=u32(m,g[4]+0x58u);
    store8(m,c.o,0x220ceeu,g[4]+0x12u,0u);
  L220cf3:
    g[4]-=4;store(m,c.o,0x220cf3u,g[4],g[7]);        // push edi
    g[4]-=4;store(m,c.o,0x220cf4u,g[4],g[5]);        // push ebp
    g[4]-=4;store(m,c.o,0x220cf5u,g[4],0x220cfau);
    object_loop_call(c,a,0x223990u);                 // call 223990 (ret 8)
    store(m,c.o,0x220cfdu,g[4]+0x14u,g[0]);
    store8(m,c.o,0x220d01u,g[4]+0x58u,0u);
    store8(m,c.o,0x220d06u,g[4]+0x11u,g[3]);
    if(g[0]==0xffffffffu)goto L220d1d;               // cmp eax,-1; jz
    if((u8(m,g[4]+0x68u)&0x10u)==0u)goto L220d4e;
    if(g[6]!=0u)goto L220d4e;
    store(m,c.o,0x220d17u,g[4]+0x64u,g[3]);
    goto L220d4e;
  L220d1d:
    g[0]=g[3];                                        // eax = ebx(1)
    g[1]=g[6];                                        // ecx = esi
    g[0]=1u<<(g[6]&31u);                             // shl eax,cl
    if((g[0]&3u)==0u)goto L221628;                    // test al,3; jz
    g[1]=u32(m,g[4]+0x5cu);
    g[2]=u32(m,g[4]+0x54u);
    g[4]-=4;store(m,c.o,0x220d33u,g[4],g[1]);
    g[4]-=4;store(m,c.o,0x220d34u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x220d35u,g[4],g[2]);
    g[0]=g[5];
    g[4]-=4;store(m,c.o,0x220d38u,g[4],0x220d3du);
    object_loop_call(c,a,0x2218c0u);                 // call 2218c0 (ret 0xc)
    store(m,c.o,0x220d40u,g[4]+0x14u,g[0]);
    store8(m,c.o,0x220d44u,g[4]+0x58u,g[3]);
    if(g[0]==0xffffffffu)goto L221610;
  L220d4e:
    g[1]=lo16(g[1],u16(m,g[4]+0x54u));
    g[2]=u32(m,g[4]+0x60u);
    g[5]=g[0];
    g[0]=u32(m,0x50ebe8u);
    g[6]=u32(m,g[0]+0x44u);
    g[5]=(g[5]&0xffffu)*0xd4u;
    g[5]=g[5]+g[6];                                   // ebp = channel record
    store16(m,c.o,0x220d6fu,g[5]+2u,g[1]);
    {const auto v=u8(m,g[2])>>1;                      // al = [edx]>>1
     g[0]=lo8(g[0],v);
     store(m,c.o,0x220d79u,g[4]+0x20u,g[5]);
     if(((g[3]&v)&1u)==0u){                           // test bl,al; jnz skips -> copy when zero
         g[7]=g[5]+0xcu;g[1]=0x11u;g[6]=g[2];
         const auto df=(a.eflags&0x400u)!=0u;
         while(g[1]){const auto x=u32(m,g[6]);store(m,c.o,0x220d89u,g[7],x);
             const auto d=df?0xfffffffcu:4u;g[6]+=d;g[7]+=d;--g[1];}
     }}
  L220d8b:
    g[0]=u32(m,g[4]+0x50u);
    g[3]=g[5]+0xcu;
    g[4]-=4;store(m,c.o,0x220d92u,g[4],0x220d97u);
    object_loop_call(c,a,0x223a70u);                 // call 223a70 (ret 0)
    if(u8(m,g[4]+0x12u)==0u)goto L220f25;
    g[2]=u32(m,sound_state_g);
    g[1]=lo8(g[1],u8(m,g[5]+0x53u));
    if((g[1]&0xffu)==u8(m,g[2]+0x7fu))goto L220dcb;
    g[0]=0u;
    store(m,c.o,0x220db3u,g[5]+0xc4u,g[0]);
    store(m,c.o,0x220db9u,g[5]+0xc8u,g[0]);
    store(m,c.o,0x220dbfu,g[5]+0xccu,g[0]);
    store(m,c.o,0x220dc5u,g[5]+0xd0u,g[0]);
  L220dcb:
    g[1]=lo8(g[1],u8(m,g[3])>>3);
    if((g[1]&1u)!=0u)set_xmm(a,0,ss(u32(m,g[5]+0x44u)));
    else{g[2]=u32(m,g[4]+0x18u);set_xmm(a,0,ss(u32(m,g[2]+0x10u)));}
    store(m,c.o,0x220de5u,g[4]+0x60u,ssb(xmm(a,0)));
    g[7]=u32(m,g[4]+0x60u);
    g[0]=g[4]+0x54u;
    g[4]-=4;store(m,c.o,0x220df3u,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x220df4u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x220df5u,g[4],g[3]);
    g[4]-=4;store(m,c.o,0x220df6u,g[4],0x220dfbu);
    object_loop_call(c,a,0x12a810u);                 // call 12a810 (ret 0xc)
    g[6]=g[0];
    if(g[6]==0xffffffffu)goto L220f21;
    g[1]=lo8(g[1],u8(m,g[3])>>2);
    if((g[1]&1u)!=0u)set_xmm(a,0,ss(u32(m,g[5]+0x40u)));
    else{g[2]=u32(m,g[4]+0x18u);set_xmm(a,0,ss(u32(m,g[2]+0xcu)));}
    store(m,c.o,0x220e20u,g[4]+0x60u,ssb(xmm(a,0)));
    set_xmm(a,0,ss(c_thousandth));
    set_xmm(a,1,ss(u32(m,g[4]+0x60u)));
    {const auto fc=comiss_fp(c,a,xmm(a,1),xmm(a,0));
     if(fc.cf||fc.zf)store(m,c.o,0x220e39u,g[4]+0x60u,ssb(xmm(a,0)));}
    g[0]=u32(m,g[4]+0x54u);
    g[1]=u32(m,g[4]+0x60u);
    g[4]-=4;store(m,c.o,0x220e47u,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x220e48u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x220e49u,g[4],g[1]);
    g[4]-=4;store(m,c.o,0x220e4au,g[4],0x220e4fu);
    object_loop_call(c,a,0x198d80u);                 // call 198d80 (ret 0xc)
    g[0]=g[4]+0x30u;
    g[1]=g[3];
    store(m,c.o,0x220e55u,g[4]+0x50u,ssb(xmm(a,0)));
    g[4]-=4;store(m,c.o,0x220e5bu,g[4],0x220e60u);
    object_loop_call(c,a,0x12d6f0u);                 // call 12d6f0 (ret 0)
    g[2]=lo8(g[2],u8(m,g[5]+0xfu)&0xfu);
    if((g[2]&0xffu)==1u){
        g[1]=u32(m,sound_state_g);
        g[0]=0x12u+g[6]+g[6]*8u;
        g[2]=g[4]+0x30u;
        g[1]=g[1]+g[0]*8u;
        g[6]=g[2];
        g[4]-=4;store(m,c.o,0x220e7eu,g[4],0x220e83u);
        object_loop_call(c,a,0x145200u);             // call 145200 (ret 0)
    }
  L220e83:
    set_xmm(a,0,ss(u32(m,g[4]+0x54u)));
    {const auto fc=comiss_fp(c,a,xmm(a,0),ss(u32(m,g[4]+0x60u)));
     if(!fc.cf&&!fc.zf){g[0]=g[4]+0x30u;
         g[4]-=4;store(m,c.o,0x220e94u,g[4],0x220e99u);object_loop_call(c,a,0x30bd0u);}}
    g[4]-=4;store(m,c.o,0x220e99u,g[4],1u);
    g[2]=g[4]+0x40u;
    g[4]-=4;store(m,c.o,0x220e9fu,g[4],g[2]);
    g[3]=g[4]+0x38u;
    g[4]-=4;store(m,c.o,0x220ea4u,g[4],0x220ea9u);
    object_loop_call(c,a,0x198cf0u);                 // call 198cf0 (ret 8)
    set_xmm(a,2,ss(c_one));
    set_xmm(a,1,_mm_setzero_ps());
    g[0]=0u;
  L220ec0:
    set_xmm(a,0,mul_fp(c,a,ss(u32(m,g[4]+0x3cu+g[0]*4u)),ss(u32(m,g[4]+0x50u))));
    set_xmm(a,3,xmm(a,0));
    g[0]+=1u;
    set_xmm(a,3,mul_fp(c,a,xmm(a,3),xmm(a,0)));
    set_xmm(a,3,add_fp(c,a,xmm(a,3),xmm(a,1)));
    set_xmm(a,1,xmm(a,3));
    if(std::int32_t(g[0])<4)goto L220ec0;
    {const auto fc=comiss_fp(c,a,xmm(a,1),xmm(a,2));
     if(!fc.cf&&!fc.zf)set_xmm(a,2,div_fp(c,a,xmm(a,2),xmm(a,1)));}
    g[0]=0u;
    g[1]=g[5]+0xc4u;
  L220f00:
    set_xmm(a,0,mul_fp(c,a,ss(u32(m,g[4]+0x3cu+g[0]*4u)),xmm(a,2)));
    set_xmm(a,0,mul_fp(c,a,xmm(a,0),ss(u32(m,g[4]+0x50u))));
    set_xmm(a,0,add_fp(c,a,xmm(a,0),ss(u32(m,g[1]))));
    store(m,c.o,0x220f14u,g[1],ssb(xmm(a,0)));
    g[0]+=1u;g[1]+=4u;
    if(std::int32_t(g[0])<4)goto L220f00;
  L220f21:
    store8(m,c.o,0x220f21u,g[5]+2u,u8(m,g[5]+2u)&0xfdu);
  L220f25:
    g[0]=u32(m,sound_state_g);
    g[3]=u32(m,g[4]+0x64u);
    g[1]=lo8(g[1],u8(m,g[0]+0x7fu));
    store8(m,c.o,0x220f34u,g[5]+0x53u,g[1]);
    if(g[3]!=2u){ if((u8(m,g[5]+0x50u)&8u)==0u)goto L220f5d; }
    if(u8(m,g[5]+0x51u)!=0u)goto L220f5d;
  L220f46:
    g[7]=u32(m,g[4]+0x14u);
    g[4]-=4;store(m,c.o,0x220f4au,g[4],0x220f4fu);
    object_loop_call(c,a,0x221a80u);                 // call 221a80 (ret 4)
    g[0]=lo8(g[0],u8(m,g[4]+0x11u));
    g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
    g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x3cu;pop_return(m,a,32);return;
  L220f5d:
    g[2]=u32(m,g[4]+0x18u);
    g[0]=u32(m,g[2]+0x1cu);
    store8(m,c.o,0x220f66u,g[4]+0x11u,0u);
    store(m,c.o,0x220f6bu,g[4]+0x50u,0u);
    if(g[0]==0u||std::int32_t(g[0])<0)goto L221555;
  L220f80:
    g[1]=u32(m,g[4]+0x50u);
    g[0]=u32(m,g[4]+0x18u);
    g[7]=u32(m,g[0]+0x20u);
    g[2]=g[1]*0x58u;
    g[6]=g[1]+g[1]*4u;
    g[1]=u32(m,g[4]+0x20u);
    g[5]=0x88u+g[1]+g[6]*4u;                          // ebp = permutation slot
    g[6]=u32(m,random_seed_g);
    g[2]=g[2]+g[7];
    g[6]+=4u;
    {const bool z=(u8(m,g[0])&0x30u)==0u;
     store(m,c.o,0x220facu,g[4]+0x54u,g[2]);
     store(m,c.o,0x220fb0u,g[4]+0x5cu,g[6]);
     if(!z){g[0]=0x8u+g[5];store(m,c.o,0x220fb9u,g[4]+0x5cu,g[0]);}}
    g[0]=lo8(g[0],u8(m,g[4]+0x13u));
    store8(m,c.o,0x220fc1u,g[4]+0x60u,g[0]);
    g[0]=u32(m,g[2]+0x44u);
    g[6]=0xffffffffu;
    if(g[0]==g[6] && u32(m,g[2]+0x30u)==g[6] && u32(m,g[2]+0x38u)==g[6]
       && u32(m,g[2]+0x4cu)==g[6] && (u8(m,g[2]+4u)&0x14u)==0u)
        store8(m,c.o,0x220fe4u,g[4]+0x60u,0u);
  L220fe9:
    if(u8(m,g[4]+0x58u)!=0u){
        g[0]=u32(m,g[1]+0x10u);
        store(m,c.o,0x220ff4u,g[5]+0x10u,g[0]);
        store8(m,c.o,0x220ff7u,g[5]+0xeu,0u);
        goto L221002;
    }
    if(u32(m,g[5])!=g[6])goto L221019;
  L221002:
    {const bool z=(u8(m,g[4]+0x60u)==0u);
     g[0]=lo8(g[0],z?1u:0u);
     g[0]=lo8(g[0],(g[0]-1u)&0xffu);
     store(m,c.o,0x22100du,g[5],g[6]);
     g[0]&=3u;
     store8(m,c.o,0x221013u,g[5]+0xcu,g[0]);
     store8(m,c.o,0x221016u,g[5]+0xdu,g[0]);}
  L221019:
    if(g[3]!=0u)goto L22110f;
    g[7]=u32(m,g[2]+0x18u);
    if(g[7]==g[6])goto L221118;
    g[2]=u32(m,g[4]+0x54u);
    g[1]=u32(m,g[4]+0x50u);
    g[3]=u32(m,g[2]+0x28u);
    g[0]=u32(m,g[4]+0x14u);
    g[4]-=4;store(m,c.o,0x22103bu,g[4],0xffffffffu);
    g[4]-=4;store(m,c.o,0x22103du,g[4],0x221042u);
    object_loop_call(c,a,0x222280u);                 // call 222280 (ret 4)
    g[2]=u32(m,g[4]+0x50u);
    g[6]=g[0];
    g[0]=u32(m,g[4]+0x54u);
    g[1]=lo8(g[1],u8(m,g[0]+4u));
    g[0]=u32(m,g[4]+0x14u);
    g[3]=1u;
    {const bool zf=((g[3]&g[1])&1u)==0u;
     g[1]=u32(m,g[4]+0x5cu);
     g[4]-=4;store(m,c.o,0x22105eu,g[4],g[1]);         // push ecx
     if(!zf)goto L221074;}                             // if bl&[4+eax] goto L221074
    g[4]-=4;store(m,c.o,0x221061u,g[4],g[3]);          // L221061: push ebx
    g[4]-=4;store(m,c.o,0x221062u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x221063u,g[4],0xffffffffu);
    g[4]-=4;store(m,c.o,0x221065u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x221066u,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x221067u,g[4],0x22106cu);
    object_loop_call(c,a,0x222350u);                  // call 222350 (ret 0x18)
    store(m,c.o,0x22106cu,g[5],g[0]);
    goto L2210f8;
  L221074:
    g[4]-=4;store(m,c.o,0x221074u,g[4],0u);
    g[4]-=4;store(m,c.o,0x221076u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x221077u,g[4],0xffffffffu);
    g[4]-=4;store(m,c.o,0x221079u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x22107au,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x22107bu,g[4],0x221080u);
    object_loop_call(c,a,0x222350u);                 // call 222350 (ret 0x18)
    if(g[6]==0xffffffffu)goto L2210e7;
    g[1]=u32(m,g[4]+0x54u);
    set_xmm(a,0,ss(u32(m,g[1]+0xcu)));
    g[2]=u32(m,0x4f2c4cu);
    g[1]=u32(m,g[2]+0x44u);
    g[7]=(g[6]&0xffffu)*0xbcu;
    g[0]=g[6];
    store(m,c.o,0x2210a7u,g[4]+0x2cu,ssb(xmm(a,0)));
    g[7]=g[7]+g[1];
    g[4]-=4;store(m,c.o,0x2210afu,g[4],0x2210b4u);
    object_loop_call(c,a,0x12d320u);                 // call 12d320 (ret 0)
    set_xmm(a,0,ss(u32(m,g[4]+0x2cu)));
    set_xmm(a,0,mul_fp(c,a,xmm(a,0),ss(u32(m,0x46a1dcu))));
    store(m,c.o,0x2210c2u,g[7]+0xb0u,g[0]);
    g[0]=std::uint32_t(cvttss_fp(c,a,xmm(a,0)));
    store16(m,c.o,0x2210ccu,g[7]+0xaeu,g[3]);
    store(m,c.o,0x2210d3u,g[7]+0xb4u,g[0]);
    store(m,c.o,0x2210d9u,g[7]+0xb8u,0xffffffffu);
    store8(m,c.o,0x2210e3u,g[7]+4u,u8(m,g[7]+4u)|0x80u);
  L2210e7:
    g[1]=u32(m,g[4]+0x54u);
    g[2]=u32(m,g[1]+0xcu);
    g[4]-=4;store(m,c.o,0x2210eeu,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x2210efu,g[4],g[3]);
    g[3]=u32(m,g[5]);
    g[4]-=4;store(m,c.o,0x2210f3u,g[4],0x2210f8u);
    object_loop_call(c,a,0x222830u);                 // call 222830 (ret 8)
  L2210f8:
    g[2]=u32(m,g[4]+0x54u);
    g[1]=u32(m,g[4]+0x20u);
    store8(m,c.o,0x221100u,g[5]+0xcu,0u);
    g[0]=lo8(g[0],u8(m,g[5]+0xcu));
    store8(m,c.o,0x221107u,g[5]+0xdu,g[0]);
    g[6]=0xffffffffu;
    goto L221118;
  L22110f:
    if(g[3]==2u)goto L221412;
  L221118:
    if((u8(m,g[1]+0x50u)&8u)!=0u)goto L221412;
    g[1]=u32(m,g[2]+4u);
    g[7]=u8(m,g[2]+4u);
    g[0]=g[1];
    g[0]=g[0]>>1;
    g[3]=g[6];
    g[7]&=1u;
    g[0]=lo8(g[0],g[0]&1u);
    store(m,c.o,0x221134u,g[4]+0x1cu,g[3]);
    store(m,c.o,0x221138u,g[4]+0x24u,g[6]);
    if((g[0]&1u)!=0u)g[7]|=4u; else g[7]&=0xfffffffbu;
    if((g[0]&0xffu)!=0u)g[7]|=0x40u; else g[7]&=0xffffffbfu;
    g[1]=g[1]>>2;
    g[1]=lo8(g[1],g[1]&1u);
    if((g[1]&1u)!=0u)g[7]|=8u; else g[7]&=0xfffffff7u;
    if((g[1]&0xffu)!=0u)g[7]|=0x20u; else g[7]&=0xffffffdfu;
    g[1]=lo8(g[1],u8(m,g[5]+0xcu));
    g[0]=lo8(g[0],u8(m,g[5]+0xdu));
    if((g[1]&0xffu)==(g[0]&0xffu))goto L2211a4;
    g[0]=sx8(g[0]);
    g[0]=u32(m,0x455be0u+g[0]*4u);
    if(u32(m,g[0]+g[2]+4u)==g[6])goto L2211a4;
    g[3]=lo8(g[3],u8(m,g[4]+0x60u));               // bl = [0x60+esp]
    g[0]=1u<<((g[1]&0xffu)&31u);
    {const bool nz=(g[0]&0x58u)!=0u;
     g[0]=lo8(g[0],nz?1u:0u);                       // al = setne
     const bool eq=((g[3]&0xffu)==(g[0]&0xffu));    // cmp bl,al
     g[3]=u32(m,g[4]+0x1cu);                         // ebx = [0x1c+esp]
     if(eq)goto L22123b;}
  L2211a4:
    g[0]=sx8(g[1]);
    g[1]=lo8(g[1],u8(m,g[4]+0x60u));
    if((g[1]&0xffu)!=0u)g[1]=u32(m,0x455bc4u+g[0]*4u);
    else g[1]=u32(m,0x455ba8u+g[0]*4u);
    if(g[1]==g[0])goto L221211;
  L2211c3:
    g[0]=u32(m,0x455be0u+g[1]*4u);
    if(g[0]==0xffffffffu)goto L2211ee;
    g[0]=g[0]+g[2];
    if(g[0]==0u)goto L2211ee;
    g[6]=u32(m,g[0]+4u);
    if(g[6]==0xffffffffu)goto L2211ee;
    g[3]=1u<<((g[1]&0xffu)&31u);
    if((g[7]&g[3])==0u)goto L22120e;
    g[3]=u32(m,g[4]+0x1cu);
    store(m,c.o,0x2211eau,g[4]+0x24u,g[6]);
  L2211ee:
    g[6]=g[1];
    if(u8(m,g[4]+0x60u)!=0u)g[1]=u32(m,0x455bc4u+g[1]*4u);
    else g[1]=u32(m,0x455ba8u+g[1]*4u);
    if(g[1]!=g[6])goto L2211c3;
    goto L221211;
  L22120e:
    g[3]=u32(m,g[0]+4u);
  L221211:
    g[0]=u32(m,g[4]+0x24u);
    store8(m,c.o,0x221218u,g[5]+0xdu,g[1]);
    if(g[0]==0xffffffffu)goto L221236;
    g[1]=u32(m,g[4]+0x5cu);
    g[2]=u32(m,g[4]+0x50u);
    g[4]-=4;store(m,c.o,0x221225u,g[4],g[1]);
    g[4]-=4;store(m,c.o,0x221226u,g[4],0u);
    g[4]-=4;store(m,c.o,0x221228u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x221229u,g[4],0xffffffffu);
    g[4]-=4;store(m,c.o,0x22122bu,g[4],g[0]);
    g[0]=u32(m,g[4]+0x28u);                           // eax = [0x28+esp]
    g[4]-=4;store(m,c.o,0x221230u,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x221231u,g[4],0x221236u);
    object_loop_call(c,a,0x222350u);                 // call 222350 (ret 0x18)
  L221236:
    if(g[3]!=0xffffffffu)goto L221257;
  L22123b:
    g[1]=lo8(g[1],u8(m,g[5]+0xcu));
    g[2]=1u<<((g[1]&0xffu)&31u);
    g[1]=lo8(g[1],u8(m,g[4]+0x60u));
    {const bool nz=(g[2]&0x58u)!=0u;
     g[0]=lo8(g[0],nz?1u:0u);
     if((g[1]&0xffu)==(g[0]&0xffu))goto L221539;}
  L221257:
    g[0]=u32(m,g[4]+0x64u);
    if(g[0]!=0u)goto L22126d;
    g[1]=u32(m,g[4]+0x54u);
    if((u8(m,g[1]+4u)&1u)==0u)goto L22126d;
    g[1]=lo8(g[1],1u);
    goto L22126f;
  L22126d:
    g[1]=lo8(g[1],0u);
  L22126f:
    g[0]=u32(m,g[5]);
    if(g[0]==0xffffffffu)goto L221330;
    if((g[1]&0xffu)!=0u)goto L221330;
    g[2]=u32(m,0x4f2c4cu);
    g[1]=u32(m,g[2]+0x44u);
    g[0]=(g[0]&0xffffu)*0xbcu;
    if(u32(m,g[0]+g[1]+0xcu)==g[3])goto L221539;      // cmp [0xc+eax+ecx],ebx; jz
    g[1]=lo8(g[1],u8(m,g[5]+0xcu));
    g[2]=1u<<((g[1]&0xffu)&31u);
    g[1]=lo8(g[1],u8(m,g[4]+0x60u));
    {const bool nz=(g[2]&0x58u)!=0u;
     g[0]=lo8(g[0],nz?1u:0u);
     if((g[1]&0xffu)==(g[0]&0xffu))goto L221539;}
    g[1]=u32(m,g[4]+0x54u);
    if((u8(m,g[1]+4u)&4u)==0u)goto L221539;
    if(g[3]==0xffffffffu)goto L22132b;
    g[2]=u32(m,g[4]+0x5cu);
    g[0]=u32(m,g[4]+0x14u);
    g[6]=u32(m,g[4]+0x50u);
    g[4]-=4;store(m,c.o,0x2212dcu,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x2212ddu,g[4],2u);
    g[4]-=4;store(m,c.o,0x2212dfu,g[4],g[0]);
    g[7]=0xffffffffu;
    g[0]=g[3];
    g[4]-=4;store(m,c.o,0x2212e5u,g[4],0x2212eau);
    object_loop_call(c,a,0x222320u);                 // call 222320 (ret 0xc)
    g[6]=g[0];
    if(g[6]==0xffffffffu)goto L221539;
  L2212f5:
    g[1]=u32(m,g[4]+0x54u);
    g[2]=u32(m,g[1]+0x50u);
    g[7]=u32(m,g[5]);
    g[4]-=4;store(m,c.o,0x2212ffu,g[4],g[2]);
    g[0]=g[6];
    g[3]=1u;
    g[4]-=4;store(m,c.o,0x221307u,g[4],0x22130cu);
    object_loop_call(c,a,0x129900u);                 // call 129900 (ret 4)
    g[3]=lo8(g[3],u8(m,g[4]+0x60u));
    {const bool z=((g[3]&0xffu)==0u);
     g[0]=lo8(g[0],z?1u:0u);
     g[0]=lo8(g[0],(g[0]-1u)&0xffu);
     g[0]=lo8(g[0],g[0]&3u);
     g[0]=lo8(g[0],(g[0]+1u)&0xffu);
     store(m,c.o,0x22131bu,g[5],g[6]);
     g[1]=lo8(g[1],g[0]&0xffu);
     store8(m,c.o,0x221320u,g[5]+0xcu,g[0]);
     store8(m,c.o,0x221323u,g[5]+0xdu,g[1]);}
    goto L221539;
  L22132b:
    g[6]=0xffffffffu;
    goto L2212f5;
  L221330:
    if(g[3]==0xffffffffu)goto L221539;
    g[0]=u32(m,tag_instances_g);
    g[2]=(g[3]&0xffffu)<<4;
    g[1]=u32(m,g[2]+g[0]+8u);
    g[0]=sx8(u8(m,g[1]+0xbu));
    g[2]=u32(m,sound_gestalt);
    g[6]=u32(m,g[2]+0xcu);
    g[2]=u32(m,g[2]+4u);
    g[0]=g[0]+g[0]*4u;
    g[0]=0x8u+g[6]+g[0]*4u;
    g[6]=sx16(u16(m,g[0]));
    g[0]=sx16(u16(m,g[0]+2u));
    set_xmm(a,1,cvtsi_fp(c,a,xmm(a,1),std::int32_t(g[0])));
    g[0]=sx16(u16(m,g[1]+6u));
    g[0]=g[0]*0x38u;
    g[0]=sx16(u16(m,g[0]+g[2]+0x18u));
    set_xmm(a,0,cvtsi_fp(c,a,xmm(a,0),std::int32_t(g[6])));
    set_xmm(a,1,sub_fp(c,a,xmm(a,1),xmm(a,0)));
    set_xmm(a,1,mul_fp(c,a,xmm(a,1),ss(u32(m,g[5]+0x10u))));
    set_xmm(a,2,cvtsi_fp(c,a,xmm(a,2),std::int32_t(g[0])));
    set_xmm(a,1,add_fp(c,a,xmm(a,1),xmm(a,2)));
    g[0]=0xffffffffu;
    set_xmm(a,1,add_fp(c,a,xmm(a,1),xmm(a,0)));
    g[4]-=4;store(m,c.o,0x221397u,g[4],0x22139cu);
    object_loop_call(c,a,0x21f960u);                 // call 21f960 (ret 0)
    g[1]=u32(m,g[4]+0x50u);
    g[7]=sx16(g[0]&0xffffu);
    g[0]=u32(m,g[4]+0x14u);
    g[4]-=4;store(m,c.o,0x2213a7u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x2213a8u,g[4],0x2213adu);
    object_loop_call(c,a,0x222280u);                 // call 222280 (ret 4)
    g[6]=g[0];
    if(g[6]!=0xffffffffu)goto L2213d7;
    g[1]=u32(m,g[4]+0x5cu);
    g[2]=u32(m,g[4]+0x50u);
    g[0]=u32(m,g[4]+0x14u);
    g[4]-=4;store(m,c.o,0x2213c0u,g[4],g[1]);
    g[4]-=4;store(m,c.o,0x2213c1u,g[4],2u);
    g[4]-=4;store(m,c.o,0x2213c3u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x2213c4u,g[4],g[7]);
    g[4]-=4;store(m,c.o,0x2213c5u,g[4],g[3]);
    g[4]-=4;store(m,c.o,0x2213c6u,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x2213c7u,g[4],0x2213ccu);
    object_loop_call(c,a,0x222350u);                 // call 222350 (ret 0x18)
    g[6]=g[0];
    if(g[6]==0xffffffffu)goto L221539;
  L2213d7:
    g[0]=u32(m,g[4]+0x64u);
    g[3]=1u;
    if(g[0]!=0u)goto L2213f5;
    g[1]=u32(m,g[4]+0x54u);
    if((u8(m,g[1]+4u)&1u)==0u)goto L221404;
    g[2]=g[1];
    g[0]=u32(m,g[2]+0xcu);
    g[4]-=4;store(m,c.o,0x2213f2u,g[4],g[0]);
    goto L2213fa;
  L2213f5:
    g[4]-=4;store(m,c.o,0x2213f5u,g[4],c_half);
  L2213fa:
    g[0]=g[6];
    g[7]=0xffffffffu;
    g[4]-=4;store(m,c.o,0x2213ffu,g[4],0x221404u);
    object_loop_call(c,a,0x129900u);                 // call 129900 (ret 4)
  L221404:
    g[1]=lo8(g[1],u8(m,g[5]+0xdu));
    store(m,c.o,0x221407u,g[5],g[6]);
    store8(m,c.o,0x22140au,g[5]+0xcu,g[1]);
    goto L221539;
  L221412:
    if(u8(m,g[1]+0x52u)==2u)goto L221539;            // cmp [0x52+ecx],2; jz
    set_xmm(a,0,ss(u32(m,g[4]+0x6cu)));
    {const auto f=ucomiss_fp(c,a,xmm(a,0),ss(u32(m,0x46a0e8u)));
     const auto ah=std::uint32_t(f.cf)|2u|(std::uint32_t(f.pf)<<2)|(std::uint32_t(f.zf)<<6);
     g[0]=(g[0]&0xffff00ffu)|(ah<<8);
     const bool pf_even=(std::popcount(ah&0x44u)&1u)==0u;   // PF of test ah,0x44
     if(!pf_even)goto L22144b;}
    g[5]=u32(m,g[5]);                                 // ebp = [ebp]
    if(g[5]==0xffffffffu)goto L221539;
    g[2]=u32(m,g[4]+0x6cu);
    g[4]-=4;store(m,c.o,0x22143fu,g[4],g[5]);
    g[4]-=4;store(m,c.o,0x221440u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x221441u,g[4],0x221446u);
    object_loop_call(c,a,0x222710u);                 // call 222710 (ret 8)
    goto L221539;
  L22144b:
    g[1]=lo8(g[1],u8(m,g[5]+0xcu));                   // cl = [0xc+ebp]
    g[7]=1u<<((g[1]&0xffu)&31u);                      // edi = 1 << cl
    g[1]=u32(m,g[4]+0x54u);                           // ecx = [0x54+esp]
    g[7]&=0x58u;
    if(g[7]==0u)goto L22147d;
    g[0]=u32(m,g[1]+4u);                              // eax = [4+ecx]
    if((g[0]&0x10u)!=0u)goto L22146b;
    if(u32(m,g[1]+0x38u)==0xffffffffu)goto L22147d;
  L22146b:
    g[6]=u32(m,g[1]+0x38u);
    g[3]=g[0];
    g[3]=g[3]>>4;
    store8(m,c.o,0x221473u,g[4]+0x60u,1u);
    g[3]=g[3]&1u;
    goto L22148f;
  L22147d:
    g[0]=u32(m,g[1]+4u);
    g[6]=u32(m,g[1]+0x28u);
    g[3]=g[0];
    g[3]=g[3]>>1;
    store8(m,c.o,0x221487u,g[4]+0x60u,0u);
    g[3]=g[3]&1u;
  L22148f:
    g[2]=u32(m,g[5]);
    if(g[2]==0xffffffffu)goto L2214d6;
    if((g[3]&0xffu)!=0u)goto L2214b1;
    if(g[6]!=0xffffffffu)goto L2214fa;
    g[1]=u32(m,g[4]+0x18u);
    if((u8(m,g[1])&2u)!=0u)goto L221539;
    g[1]=u32(m,g[4]+0x54u);
  L2214b1:
    if(g[7]!=0u && (g[0]&0x10u)!=0u)set_xmm(a,0,ss(u32(m,g[1]+0x54u)));
    else set_xmm(a,0,ss(u32(m,g[1]+0x10u)));
    store(m,c.o,0x2214c5u,g[4]+0x28u,ssb(xmm(a,0)));
    g[4]-=4;store(m,c.o,0x2214cbu,g[4],g[2]);          // push edx
    g[2]=u32(m,g[4]+0x2cu);                            // edx = [0x2c+esp]
    g[4]-=4;store(m,c.o,0x2214d0u,g[4],g[2]);          // push edx
    g[4]-=4;store(m,c.o,0x2214d1u,g[4],0x2214d6u);
    object_loop_call(c,a,0x222710u);                  // call 222710 (ret 8)
  L2214d6:
    if(g[6]==0xffffffffu)goto L221539;
    if((g[3]&0xffu)==0u)goto L2214fa;
    g[0]=u32(m,g[4]+0x5cu);
    g[1]=u32(m,g[4]+0x50u);
    g[2]=u32(m,g[4]+0x14u);
    g[4]-=4;store(m,c.o,0x2214ebu,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x2214ecu,g[4],0u);
    g[4]-=4;store(m,c.o,0x2214eeu,g[4],g[1]);
    g[4]-=4;store(m,c.o,0x2214efu,g[4],0xffffffffu);
    g[4]-=4;store(m,c.o,0x2214f1u,g[4],g[6]);
    g[4]-=4;store(m,c.o,0x2214f2u,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x2214f3u,g[4],0x2214f8u);
    object_loop_call(c,a,0x222350u);                  // call 222350 (ret 0x18)
    goto L221539;
  L2214fa:
    g[0]=u32(m,g[5]);
    if(g[0]==0xffffffffu)goto L221539;
    g[1]=u32(m,0x4f2c4cu);
    g[2]=u32(m,g[1]+0x44u);
    g[0]=(g[0]&0xffffu)*0xbcu;
    g[0]=g[0]+g[2];
    if(u16(m,g[0]+0xacu)==0xffffu)goto L221539;
    g[1]=lo8(g[1],u8(m,g[4]+0x60u));
    {const bool nz=((g[1]&0xffu)!=0u);
     g[2]=lo8(g[2],nz?1u:0u);
     g[2]=0x2u+g[2]*4u;
     store8(m,c.o,0x221532u,g[5]+0xdu,g[2]);
     store8(m,c.o,0x221535u,g[0]+2u,3u);}
    goto L221539;
  L221539:
    g[0]=u32(m,g[4]+0x50u);
    g[1]=u32(m,g[4]+0x18u);
    g[2]=u32(m,g[1]+0x1cu);
    g[3]=u32(m,g[4]+0x64u);
    g[0]+=1u;
    store(m,c.o,0x22154bu,g[4]+0x50u,g[0]);
    if(std::int32_t(g[0])<std::int32_t(g[2]))goto L220f80;
  L221555:
    g[6]=u32(m,g[4]+0x20u);
    if(u8(m,g[6]+0x51u)!=0u)goto L2215ac;
    g[2]=lo8(g[2],u8(m,g[6]+0xcu));
    g[0]=g[6]+0xcu;
    g[2]=lo8(g[2],(g[2]&0xffu)>>3);
    if((g[2]&1u)!=0u)set_xmm(a,0,ss(u32(m,g[0]+0x38u)));
    else{g[1]=u32(m,g[6]+4u);g[2]=u32(m,tag_instances_g);
        g[1]=(g[1]&0xffffu)<<4;g[1]=u32(m,g[1]+g[2]+8u);set_xmm(a,0,ss(u32(m,g[1]+0x10u)));}
  L221590:
    store(m,c.o,0x221590u,g[4]+0x64u,ssb(xmm(a,0)));
    g[2]=u32(m,g[4]+0x64u);
    g[4]-=4;store(m,c.o,0x22159au,g[4],0u);
    g[4]-=4;store(m,c.o,0x22159cu,g[4],g[2]);
    g[4]-=4;store(m,c.o,0x22159du,g[4],g[0]);
    g[4]-=4;store(m,c.o,0x22159eu,g[4],0x2215a3u);
    object_loop_call(c,a,0x12a810u);                  // call 12a810 (ret 0xc)
    if(g[0]==0xffffffffu)goto L220f46;
  L2215ac:
    g[0]=lo8(g[0],u8(m,g[4]+0x13u));
    {const bool z=((g[0]&0xffu)==0u);
     g[0]=lo8(g[0],u8(m,g[6]+0x50u));
     if(!z)g[0]=lo8(g[0],(g[0]|1u)); else g[0]=lo8(g[0],(g[0]&0xfeu));
     store8(m,c.o,0x2215bdu,g[6]+0x50u,g[0]);}
    g[0]=u32(m,g[4]+0x68u);
    g[1]=g[0];
    g[1]=g[1]>>1;
    {const bool z=((g[1]&1u)==0u);
     g[1]=lo8(g[1],u8(m,g[6]+0x50u));
     if(!z)g[1]=lo8(g[1],(g[1]|2u)); else g[1]=lo8(g[1],(g[1]&0xfdu));
     g[0]=g[0]>>2;
     const bool z2=((g[0]&1u)==0u);
     store8(m,c.o,0x2215ddu,g[6]+0x50u,g[1]);
     g[0]=lo8(g[0],g[1]&0xffu);
     if(z2)goto L2215fa;}
    g[0]=lo8(g[0],(g[0]|4u));
    store8(m,c.o,0x2215e6u,g[6]+0x50u,g[0]);
    g[0]=lo8(g[0],u8(m,g[4]+0x11u));
    g[7]=u32(m,g[4]);g[4]+=4;
    store8(m,c.o,0x2215eeu,g[6]+0x52u,g[3]);
    g[6]=u32(m,g[4]);g[4]+=4;g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x3cu;pop_return(m,a,32);return;
  L2215fa:
    g[0]=lo8(g[0],(g[0]&0xfbu));
    store8(m,c.o,0x2215fcu,g[6]+0x50u,g[0]);
    g[0]=lo8(g[0],u8(m,g[4]+0x11u));
    g[7]=u32(m,g[4]);g[4]+=4;
    store8(m,c.o,0x221604u,g[6]+0x52u,g[3]);
    g[6]=u32(m,g[4]);g[4]+=4;g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x3cu;pop_return(m,a,32);return;
  L221610:
    g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
    store8(m,c.o,0x221612u,g[4]+0x9u,0u);
    g[0]=lo8(g[0],u8(m,g[4]+0x9u));
    g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x3cu;pop_return(m,a,32);return;
  L221623:
    store8(m,c.o,0x221623u,g[4]+0x11u,1u);
  L221628:
    g[0]=lo8(g[0],u8(m,g[4]+0x11u));
    g[7]=u32(m,g[4]);g[4]+=4;g[6]=u32(m,g[4]);g[4]+=4;
    g[5]=u32(m,g[4]);g[4]+=4;g[3]=u32(m,g[4]);g[4]+=4;
    g[4]+=0x3cu;pop_return(m,a,32);return;
}

constexpr LoopRoutineEntry table[]{
    {0x18b580u,run_18b580},
    {0x1904d0u,run_1904d0},
    {0x220640u,run_220640},
    {0x190460u,run_190460},
    {0x18ffb0u,run_18ffb0},
    {0x18f9b0u,run_18f9b0},
    {0x18d5a0u,run_18d5a0},
    {0x220c60u,run_220c60},
};
} // namespace

std::span<const LoopRoutineEntry> object_loop_source_routines(){return table;}
} // namespace halo2::native_tu15_sound
