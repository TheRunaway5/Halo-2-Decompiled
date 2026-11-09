#pragma once
#include "physics/runtime/native_tu15_broadphase_add.hpp"

namespace halo2::native_broadphase_add_detail {
struct ObjectActivity {std::uint64_t additions{},pairs{},cell_pairs{};};
inline ObjectActivity &object_activity(){thread_local ObjectActivity value;return value;}
// Quantize the bounds, insert the three endpoint pairs, repair packed indices,
// then enumerate overlap parity. Service callbacks enter the existing native
// DATA owners; their actual Havok allocation owners remain explicit residuals.
template<class State,class Grow,class Insert,class Reindex,class Construct,class Destroy>
bool add_object(State &s,Grow grow,Insert insert,Reindex reindex,Construct construct,Destroy destroy) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++object_activity().additions;auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p,unsigned n=4){at(pc);return physics_step_detail::r(s.memory,p,n);};
    const auto vector=[&](std::uint32_t pc,std::uint32_t p){at(pc);return native_tu15_leaves::aligned_v(s.memory,p);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){at(pc);physics_step_detail::w(s.memory,s.observe,pc,p,n,value);};
    const auto store_vector=[&](std::uint32_t pc,std::uint32_t p,__m128 value){at(pc);native_tu15_leaves::aligned_vw(s.memory,s.observe,pc,p,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4;store(pc,g[4],value);};
    const auto pop=[&](std::uint32_t pc){const auto value=read(pc,g[4]);g[4]+=4;return value;};
    const auto low=[&](unsigned reg,std::uint32_t value){g[reg]=(g[reg]&0xffff0000u)|(value&65535u);};
    const auto signed32=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    using native_tu15_leaves::ordered_add_ps;using native_tu15_leaves::ordered_mul_ps;

    // Source SSE quantization keeps all six observable vector registers and
    // the scalar scratch words, including aliases with the retained frame.
    push(0x2ec4b0u,g[5]);g[5]=g[4];g[4]&=~15u;push(0x2ec4b6u,~0u);push(0x2ec4b8u,0x37e168u);
    g[0]=read(0x2ec4bdu,s.fs_base);push(0x2ec4c3u,g[0]);store(0x2ec4c4u,s.fs_base,g[4]);g[4]-=0x68u;
    g[0]=read(0x2ec4ceu,g[5]+12u);s.xmm[4]=vector(0x2ec4d1u,g[0]);s.xmm[5]=vector(0x2ec4d4u,g[0]+16u);
    s.xmm[3]=vector(0x2ec4d8u,0x41c6a0u);s.xmm[2]=vector(0x2ec4dfu,0x41c690u);
    push(0x2ec4e6u,g[3]);push(0x2ec4e7u,g[6]);push(0x2ec4e8u,g[7]);g[7]=g[1];
    s.xmm[0]=vector(0x2ec4ebu,g[7]+16u);s.xmm[1]=vector(0x2ec4efu,g[7]+48u);
    s.xmm[4]=ordered_mul_ps(ordered_add_ps(s.xmm[4],s.xmm[0]),s.xmm[1]);s.xmm[0]=s.xmm[4];s.xmm[4]=vector(0x2ec4fcu,0x41c680u);
    s.xmm[0]=ordered_add_ps(_mm_max_ps(_mm_min_ps(s.xmm[0],s.xmm[3]),s.xmm[2]),s.xmm[4]);store_vector(0x2ec50cu,g[4]+48u,s.xmm[0]);
    s.xmm[0]=vector(0x2ec511u,g[7]+32u);g[1]=read(0x2ec515u,g[4]+48u);g[2]=read(0x2ec519u,g[4]+52u);g[1]>>=7u;
    s.xmm[5]=ordered_add_ps(s.xmm[5],s.xmm[0]);store(0x2ec523u,g[4]+40u,g[1],2);g[1]=read(0x2ec528u,g[4]+56u);
    s.xmm[5]=ordered_mul_ps(s.xmm[5],s.xmm[1]);s.xmm[0]=s.xmm[5];s.xmm[0]=ordered_add_ps(_mm_max_ps(_mm_min_ps(s.xmm[0],s.xmm[3]),s.xmm[2]),s.xmm[4]);
    store_vector(0x2ec53bu,g[4]+48u,s.xmm[0]);g[0]=read(0x2ec540u,g[4]+48u);g[0]=(g[0]>>7u)&65535u;g[1]>>=7u;g[0]|=1u;
    store(0x2ec550u,g[4]+44u,g[1],2);g[1]=read(0x2ec555u,g[4]+52u);store(0x2ec559u,g[4]+96u,g[0]);g[2]>>=7u;g[1]>>=7u;
    store(0x2ec563u,g[4]+42u,g[2],2);g[2]=read(0x2ec568u,g[4]+56u);g[6]=read(0x2ec56cu,g[4]+40u);g[0]=std::uint16_t(g[1]);
    g[1]=read(0x2ec573u,g[7]+68u);g[0]|=1u;g[6]&=65534u;g[2]>>=7u;store(0x2ec582u,g[4]+100u,g[0]);store(0x2ec586u,g[4]+80u,g[6]);
    g[6]=read(0x2ec58au,g[4]+42u);g[0]=std::uint16_t(g[2]);g[2]=read(0x2ec591u,g[7]+68u);g[0]|=1u;g[6]&=65534u;g[3]=g[7]+64u;
    store(0x2ec5a0u,g[4]+104u,g[0]);g[0]=read(0x2ec5a4u,g[3]+8u);store(0x2ec5a7u,g[4]+84u,g[6]);g[6]=read(0x2ec5abu,g[4]+44u);g[6]&=65534u;g[0]&=0x7fffffffu;
    s.flags.minus(g[1],g[0],4);store(0x2ec5bcu,g[4]+88u,g[6]);store(0x2ec5c0u,g[4]+16u,g[2]);
    if(g[1]==g[0]) {push(0x2ec5c6u,16);push(0x2ec5c8u,g[3]);push(0x2ec5c9u,0x2ec5ceu);grow(s);g[4]=s.flags.plus(g[4],8u,4);}

    // Append the node and insert each axis. The old node-bank address is an
    // intentional unused first edge argument retained by the original ABI.
    g[0]=read(0x2ec5d1u,g[3]+4u);g[1]=read(0x2ec5d4u,g[3]);g[6]=g[0]*16u+g[1];++g[0];store(0x2ec5deu,g[3]+4u,g[0]);
    g[0]=g[6]+10u;push(0x2ec5e4u,g[0]);g[2]=g[6]+8u;push(0x2ec5e8u,g[2]);store(0x2ec5e9u,g[4]+36u,g[2]);
    g[2]=read(0x2ec5edu,g[4]+104u);push(0x2ec5f1u,g[2]);g[2]=read(0x2ec5f2u,g[4]+28u);store(0x2ec5f6u,g[4]+44u,g[0]);
    g[0]=read(0x2ec5fau,g[4]+92u);push(0x2ec5feu,g[0]);g[3]=g[1];push(0x2ec601u,g[2]);g[1]=g[7]+76u;push(0x2ec605u,g[3]);push(0x2ec606u,0x2ec60bu);insert(s);
    g[1]=read(0x2ec60bu,g[4]+84u);g[2]=read(0x2ec60fu,g[4]+16u);g[0]=g[6]+4u;push(0x2ec616u,g[0]);g[0]=read(0x2ec617u,g[4]+104u);
    push(0x2ec61bu,g[6]);push(0x2ec61cu,g[0]);push(0x2ec61du,g[1]);push(0x2ec61eu,g[2]);push(0x2ec61fu,g[3]);g[1]=g[7]+88u;push(0x2ec623u,0x2ec628u);insert(s);
    g[2]=read(0x2ec628u,g[4]+104u);g[0]=g[6]+6u;push(0x2ec62fu,g[0]);g[0]=read(0x2ec630u,g[4]+92u);g[1]=g[6]+2u;push(0x2ec637u,g[1]);
    g[1]=read(0x2ec638u,g[4]+24u);push(0x2ec63cu,g[2]);push(0x2ec63du,g[0]);push(0x2ec63eu,g[1]);push(0x2ec63fu,g[3]);g[1]=g[7]+100u;push(0x2ec643u,0x2ec648u);insert(s);
    g[2]=read(0x2ec648u,g[4]+16u);push(0x2ec64cu,g[6]);push(0x2ec64du,g[2]);push(0x2ec64eu,g[3]);g[1]=g[7];push(0x2ec651u,0x2ec656u);reindex(s);
    g[0]=read(0x2ec656u,g[5]+8u);g[1]=read(0x2ec659u,g[4]+16u);store(0x2ec65du,g[6]+12u,g[0]);store(0x2ec660u,g[0],g[1]);
    g[3]=read(0x2ec662u,g[7]+68u);g[2]=std::uint32_t(signed32(g[3])>>5u);g[2]=s.flags.plus(g[2],4u,4);push(0x2ec66du,g[2]);g[1]=g[4]+52u;
    push(0x2ec672u,0x2ec677u);construct(s);
    g[1]=read(0x2ec677u,g[4]+32u);g[2]=read(0x2ec67bu,g[1],2);g[1]=read(0x2ec67eu,g[4]+28u);g[0]=read(0x2ec682u,g[4]+48u);
    store(0x2ec686u,g[4]+40u,g[2]);g[2]=read(0x2ec68au,g[1],2);g[3]=std::uint32_t(signed32(g[3])>>7u);
    store(0x2ec692u,g[4]+124u,0);store(0x2ec69au,g[4]+36u,g[2]);s.xmm[0]=_mm_setzero_ps();g[1]=g[0];
    if(signed32(g[3])>=0) {++g[3];do {g[2]=g[1];g[1]+=16u;--g[3];store_vector(0x2ec6acu,g[2],s.xmm[0]);}while(g[3]);}

    // Seed parity from the preceding spatial cell, if present.
    g[2]=read(0x2ec6b1u,g[7]+76u);g[1]=read(0x2ec6b4u,g[7]+112u);store(0x2ec6b9u,g[4]+32u,g[2]);g[3]=g[2]+4u;
    if(g[1]) {
        g[2]=read(0x2ec6c6u,g[7]+116u);g[1]=16u-g[2];g[2]=read(0x2ec6d0u,g[4]+80u);g[2]=std::uint32_t(signed32(g[2])>>(g[1]&31u));g[1]=g[2];
        if(signed32(g[1])>0) {
            g[2]=read(0x2ec6e0u,g[7]+120u);g[1]=g[1]*16u+g[2]-16u;store(0x2ec6eau,g[4]+20u,g[1]);g[1]=read(0x2ec6eeu,g[1],2);
            g[2]=g[1]>>5u;g[1]&=31u;g[3]=1u<<g[1];store(0x2ec700u,g[0]+g[2]*4u,read(0x2ec700u,g[0]+g[2]*4u)^g[3]);
            g[1]=read(0x2ec703u,g[4]+20u);g[2]=read(0x2ec707u,g[1]+4u);store(0x2ec70au,g[4]+24u,g[2]);g[2]=read(0x2ec70eu,g[1]+8u);--g[2];
            if(signed32(g[2])>=0) {
                ++g[2];store(0x2ec715u,g[4]+28u,g[2]);
                do {g[1]=read(0x2ec720u,g[4]+24u);g[1]=read(0x2ec724u,g[1],2);g[2]=g[1]>>5u;g[1]&=31u;g[3]=1u<<g[1];
                    store(0x2ec736u,g[0]+g[2]*4u,read(0x2ec736u,g[0]+g[2]*4u)^g[3]);g[2]=read(0x2ec739u,g[4]+24u);g[1]=read(0x2ec73du,g[4]+28u);g[2]+=2u;--g[1];
                    store(0x2ec745u,g[4]+24u,g[2]);store(0x2ec749u,g[4]+28u,g[1]);}while(g[1]);g[1]=read(0x2ec74fu,g[4]+20u);
            }
            g[1]=read(0x2ec753u,g[1],2);g[2]=read(0x2ec756u,g[7]+64u);g[1]*=16u;g[3]=read(0x2ec75cu,g[1]+g[2]+8u,2);g[1]+=g[2];g[2]=read(0x2ec763u,g[7]+76u);
            g[3]=g[2]+g[3]*4u+4u;store(0x2ec76au,g[4]+24u,g[3]);g[3]=read(0x2ec76eu,g[1]+10u,2);g[3]=g[2]+g[3]*4u;g[2]=read(0x2ec775u,g[4]+24u);
            store(0x2ec77bu,g[4]+28u,g[1]);store(0x2ec77fu,g[4]+32u,g[3]);
            while(g[2]<g[3]) {
                if(!(read(0x2ec785u,g[2],1)&1u)) {
                    g[1]=read(0x2ec78au,g[2]+2u,2);g[3]=g[1]>>5u;g[1]&=31u;g[2]=~(1u<<g[1]);g[1]=read(0x2ec79du,g[0]+g[3]*4u)&g[2];g[2]=read(0x2ec7a4u,g[4]+24u);
                    store(0x2ec7a8u,g[0]+g[3]*4u,g[1]);g[3]=read(0x2ec7abu,g[4]+32u);g[1]=read(0x2ec7afu,g[4]+28u);
                }
                g[2]+=4u;store(0x2ec7b8u,g[4]+24u,g[2]);
            }
            g[1]=read(0x2ec7beu,g[1]+8u,2);g[2]=read(0x2ec7c2u,g[7]+76u);g[3]=g[2]+g[1]*4u+4u;
        }else g[2]=read(0x2ec7cbu,g[4]+32u);
    }
    g[1]=read(0x2ec7cfu,g[4]+36u);g[1]=g[2]+g[1]*4u;store(0x2ec7d8u,g[4]+24u,g[1]);
    while(g[3]<g[1]) {
        g[1]=read(0x2ec7e0u,g[3]+2u,2);g[2]=g[1]>>5u;store(0x2ec7e9u,g[4]+36u,g[2]);g[1]&=31u;g[2]=1u<<g[1];g[1]=read(0x2ec7f7u,g[4]+36u);g[3]+=4u;
        store(0x2ec7feu,g[0]+g[1]*4u,read(0x2ec7feu,g[0]+g[1]*4u)^g[2]);g[1]=read(0x2ec801u,g[4]+24u);
    }
    g[2]=read(0x2ec807u,g[7]+76u);g[1]=read(0x2ec80au,g[4]+40u);g[1]=g[2]+g[1]*4u;g[3]+=4u;store(0x2ec816u,g[4]+24u,g[1]);
    while(g[3]<g[1]) {
        if(!(read(0x2ec820u,g[3],1)&1u)) {g[1]=read(0x2ec825u,g[3]+2u,2);g[2]=g[1]>>5u;store(0x2ec82eu,g[4]+40u,g[2]);g[1]&=31u;g[2]=1u<<g[1];g[1]=read(0x2ec83cu,g[4]+40u);
            store(0x2ec840u,g[0]+g[1]*4u,read(0x2ec840u,g[0]+g[1]*4u)^g[2]);g[1]=read(0x2ec843u,g[4]+24u);}
        g[3]+=4u;
    }

    // Resolve candidate Y/Z overlaps and publish either a body pair or the
    // new node index in a special cell's dense list.
    g[1]=read(0x2ec84eu,g[7]+64u);store(0x2ec851u,g[4]+28u,g[1]);g[1]=read(0x2ec855u,g[7]+68u);g[1]=g[0]+4u+std::uint32_t(signed32(g[1])>>5u)*4u;g[2]=g[0];
    store(0x2ec863u,g[4]+32u,g[2]);store(0x2ec867u,g[4]+36u,g[1]);
    while(g[2]<g[1]) {
        g[3]=read(0x2ec871u,g[2]);g[1]=read(0x2ec875u,g[4]+28u);store(0x2ec879u,g[4]+20u,g[1]);store(0x2ec87du,g[4]+24u,g[3]);
        const bool nonempty=g[3]!=0;
        while(g[3]) {
            if(!(g[3]&255u)){g[1]+=128u;g[3]>>=8u;}
            else {
                if(g[3]&1u) {
                    g[0]=read(0x2ec8a2u,g[6]);g[2]=read(0x2ec8a4u,g[1]);store(0x2ec8a6u,g[4]+40u,g[0]);g[0]=read(0x2ec8aau,g[6]+4u);g[0]-=g[2];g[2]=read(0x2ec8afu,g[1]+4u);
                    g[2]=(g[2]-read(0x2ec8b2u,g[4]+40u))|g[0];g[2]&=0x80008000u;store(0x2ec8beu,g[4]+40u,g[2]);
                    if(!g[2]) {
                        g[3]=read(0x2ec8c4u,g[1]+12u);
                        if(!(g[3]&1u)) {
                            g[0]=read(0x2ec8ccu,g[5]+16u);g[1]=read(0x2ec8cfu,g[0]+8u);g[2]=read(0x2ec8d2u,g[0]+4u);g[1]&=0x7fffffffu;s.flags.minus(g[2],g[1],4);
                            if(g[2]==g[1]) {push(0x2ec8dfu,8);push(0x2ec8e1u,g[0]);push(0x2ec8e2u,0x2ec8e7u);grow(s);g[0]=read(0x2ec8e7u,g[5]+16u);g[4]=s.flags.plus(g[4],8u,4);}
                            g[2]=read(0x2ec8edu,g[0]+4u);g[0]=read(0x2ec8f0u,g[0]);g[1]=read(0x2ec8f2u,g[5]+8u);store(0x2ec8f5u,g[0]+g[2]*8u,g[1]);store(0x2ec8f8u,g[0]+g[2]*8u+4u,g[3]);
                            g[0]=read(0x2ec8fcu,g[5]+16u);store(0x2ec8ffu,g[0]+4u,read(0x2ec8ffu,g[0]+4u)+1u);++object_activity().pairs;
                        }else {
                            g[2]=read(0x2ec904u,g[7]+120u);g[3]&=~1u;g[0]=read(0x2ec90au,g[3]+g[2]+12u);g[1]=read(0x2ec90eu,g[3]+g[2]+8u);g[3]+=g[2]+4u;g[0]&=0x7fffffffu;s.flags.minus(g[1],g[0],4);
                            if(g[1]==g[0]) {push(0x2ec91fu,2);push(0x2ec921u,g[3]);push(0x2ec922u,0x2ec927u);grow(s);g[4]=s.flags.plus(g[4],8u,4);}
                            g[1]=read(0x2ec92au,g[3]+4u);low(0,read(0x2ec92du,g[4]+16u,2));g[2]=read(0x2ec932u,g[3]);store(0x2ec934u,g[2]+g[1]*2u,g[0],2);store(0x2ec938u,g[3]+4u,read(0x2ec938u,g[3]+4u)+1u);++object_activity().cell_pairs;
                        }
                        g[3]=read(0x2ec93bu,g[4]+24u);g[1]=read(0x2ec93fu,g[4]+20u);
                    }
                }
                g[1]+=16u;g[3]>>=1u;
            }
            store(0x2ec94au,g[4]+20u,g[1]);store(0x2ec94eu,g[4]+24u,g[3]);
        }
        if(nonempty)g[2]=read(0x2ec958u,g[4]+32u);
        g[3]=read(0x2ec95cu,g[4]+28u);g[0]=read(0x2ec960u,g[4]+36u);g[2]+=4u;g[3]+=512u;s.flags.minus(g[2],g[0],4);
        store(0x2ec96fu,g[4]+28u,g[3]);store(0x2ec973u,g[4]+32u,g[2]);g[1]=g[0];
    }
    s.flags.minus(g[2],g[1],4);g[1]=g[4]+48u;store(0x2ec981u,g[4]+124u,~0u);push(0x2ec989u,0x2ec98eu);destroy(s);
    g[1]=read(0x2ec98eu,g[4]+116u);g[7]=pop(0x2ec992u);g[6]=pop(0x2ec993u);store(0x2ec994u,s.fs_base,g[1]);g[3]=pop(0x2ec99bu);g[4]=g[5];g[5]=pop(0x2ec99eu);pop(0x2ec99fu);g[4]+=12u;at(0x2ec99fu);return true;
}
} // namespace halo2::native_broadphase_add_detail
