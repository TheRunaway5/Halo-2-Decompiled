#pragma once
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_broadphase_remove_detail {
struct Activity {std::uint64_t removals{},pairs{},cell_removals{},dense_moves{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode() {
    static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_BROADPHASE_REMOVE");return p&&*p;}();
    return enabled;
}
// The sweep uses native bitsets and packed integer comparisons. The retained
// frame below is the shared memory ABI: allocator owners and observers may
// access it, so reads, publications and dense-node backlink changes stay in
// their original order. Call supplies the temporary/array service closure;
// EraseEdges and Reindex supply the two native packed-edge kernels.
template<class State,class Call,class EraseEdges,class Reindex>
bool run(State &s,Call call,EraseEdges erase_edges,Reindex reindex) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().removals;
    auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t p,unsigned n=4){at(pc);return physics_step_detail::r(s.memory,p,n);};
    const auto store=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){at(pc);physics_step_detail::w(s.memory,s.observe,pc,p,n,value);};
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4;store(pc,g[4],value);};
    const auto pop=[&](std::uint32_t pc){const auto value=read(pc,g[4]);g[4]+=4;return value;};
    const auto low=[&](unsigned reg,std::uint32_t value,unsigned width=2){const auto mask=width==2?0xffffu:0xffu;g[reg]=(g[reg]&~mask)|(value&mask);};
    const auto signed32=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};

    // Aligned exception frame and temporary parity-bitset allocation.
    push(0x2ec9b0u,g[5]);g[5]=g[4];g[4]&=~15u;
    push(0x2ec9b6u,~0u);push(0x2ec9b8u,0x37e188u);
    g[0]=read(0x2ec9bdu,s.fs_base);push(0x2ec9c3u,g[0]);store(0x2ec9c4u,s.fs_base,g[4]);
    g[4]-=0x48u;g[0]=read(0x2ec9ceu,g[5]+8u);
    push(0x2ec9d1u,g[3]);push(0x2ec9d2u,g[6]);g[6]=g[1];
    g[1]=read(0x2ec9d5u,g[6]+0x44u);g[2]=read(0x2ec9d8u,g[6]+0x40u);
    push(0x2ec9dbu,g[7]);g[7]=read(0x2ec9dcu,g[0]);store(0x2ec9deu,g[4]+0x20u,g[1]);
    g[1]=std::uint32_t(signed32(g[1])>>5u);g[0]=g[6]+0x40u;g[1]+=4u;
    store(0x2ec9ebu,g[4]+0x24u,g[7]);push(0x2ec9efu,g[1]);g[7]*=16u;g[1]=g[4]+0x40u;
    store(0x2ec9f7u,g[4]+0x2cu,g[6]);store(0x2ec9fbu,g[4]+0x1cu,g[0]);
    g[7]=s.flags.plus(g[7],g[2],4);push(0x2eca01u,0x2eca06u);call(s,0x2eba20u,0x2eca01u);
    g[1]=read(0x2eca06u,g[7]+10u,2);g[3]=read(0x2eca0au,g[7]+8u,2);
    g[2]=read(0x2eca0eu,g[6]+0x4cu);g[0]=read(0x2eca11u,g[4]+0x3cu);g[3]*=4u;
    store(0x2eca18u,g[4]+0x34u,g[1]);g[1]=read(0x2eca1cu,g[3]+g[2],2);
    store(0x2eca20u,g[4]+0x1cu,g[1]);g[1]=read(0x2eca24u,g[4]+0x20u);
    g[1]=std::uint32_t(signed32(g[1])>>7u);store(0x2eca2du,g[4]+0x5cu,0);
    store(0x2eca35u,g[4]+0x30u,g[3]);s.xmm[0]=_mm_setzero_ps();g[2]=g[0];
    if(signed32(g[1])>=0) {
        ++g[1];store(0x2eca41u,g[4]+0x2cu,g[1]);
        do {
            g[1]=g[2];at(0x2eca47u);native_tu15_leaves::aligned_vw(s.memory,s.observe,0x2eca47u,g[1],s.xmm[0]);
            g[1]=read(0x2eca4au,g[4]+0x2cu);g[2]+=16u;--g[1];store(0x2eca52u,g[4]+0x2cu,g[1]);
        }while(g[1]);
    }

    // Seed parity with the preceding partition cell, then sweep the X edges.
    g[2]=read(0x2eca58u,g[6]+0x4cu);g[1]=read(0x2eca5bu,g[6]+0x70u);g[2]+=4u;
    if(g[1]) {
        g[3]=read(0x2eca69u,g[6]+0x74u);g[1]=16u-g[3];g[3]=read(0x2eca73u,g[4]+0x1cu);
        g[3]=std::uint32_t(signed32(g[3])>>(g[1]&31u));g[1]=g[3];
        if(signed32(g[1])>0) {
            g[2]=read(0x2eca83u,g[6]+0x78u);g[1]=g[1]*16u+g[2]-16u;
            store(0x2eca8du,g[4]+0x14u,g[1]);g[1]=read(0x2eca91u,g[1],2);
            g[2]=g[1]>>5u;g[1]&=31u;g[3]=1u<<g[1];
            store(0x2ecaa3u,g[0]+g[2]*4u,read(0x2ecaa3u,g[0]+g[2]*4u)^g[3]);
            g[1]=read(0x2ecaa6u,g[4]+0x14u);g[2]=read(0x2ecaaau,g[1]+4u);
            store(0x2ecaadu,g[4]+0x1cu,g[2]);g[2]=read(0x2ecab1u,g[1]+8u);--g[2];
            if(signed32(g[2])>=0) {
                ++g[2];store(0x2ecab8u,g[4]+0x2cu,g[2]);
                do {
                    g[1]=read(0x2ecac0u,g[4]+0x1cu);g[1]=read(0x2ecac4u,g[1],2);
                    g[2]=g[1]>>5u;g[1]&=31u;g[3]=1u<<g[1];
                    store(0x2ecad6u,g[0]+g[2]*4u,read(0x2ecad6u,g[0]+g[2]*4u)^g[3]);
                    g[2]=read(0x2ecad9u,g[4]+0x1cu);g[1]=read(0x2ecaddu,g[4]+0x2cu);g[2]+=2u;--g[1];
                    store(0x2ecae5u,g[4]+0x1cu,g[2]);store(0x2ecae9u,g[4]+0x2cu,g[1]);
                }while(g[1]);
                g[1]=read(0x2ecaefu,g[4]+0x14u);
            }
            g[1]=read(0x2ecaf3u,g[1],2);g[2]=read(0x2ecaf6u,g[4]+0x18u);g[3]=read(0x2ecafau,g[2]);
            g[2]=read(0x2ecafcu,g[6]+0x4cu);g[1]=g[1]*16u+g[3];g[3]=read(0x2ecb04u,g[1]+8u,2);
            g[3]=g[2]+g[3]*4u+4u;store(0x2ecb0cu,g[4]+0x1cu,g[3]);g[3]=read(0x2ecb10u,g[1]+10u,2);
            g[3]=g[2]+g[3]*4u;g[2]=read(0x2ecb17u,g[4]+0x1cu);
            store(0x2ecb1du,g[4]+0x14u,g[1]);store(0x2ecb21u,g[4]+0x2cu,g[3]);
            while(g[2]<g[3]) {
                if(!(read(0x2ecb27u,g[2],1)&1u)) {
                    g[1]=read(0x2ecb2cu,g[2]+2u,2);g[3]=g[1]>>5u;g[1]&=31u;g[2]=~(1u<<g[1]);
                    g[1]=read(0x2ecb3fu,g[0]+g[3]*4u)&g[2];g[2]=read(0x2ecb46u,g[4]+0x1cu);
                    store(0x2ecb4au,g[0]+g[3]*4u,g[1]);g[3]=read(0x2ecb4du,g[4]+0x2cu);g[1]=read(0x2ecb51u,g[4]+0x14u);
                }
                g[2]+=4u;store(0x2ecb5au,g[4]+0x1cu,g[2]);
            }
            g[1]=read(0x2ecb60u,g[1]+8u,2);g[2]=read(0x2ecb64u,g[6]+0x4cu);g[2]+=g[1]*4u+4u;
        }
        g[3]=read(0x2ecb6bu,g[4]+0x30u);
    }
    g[1]=read(0x2ecb6fu,g[6]+0x4cu)+g[3];store(0x2ecb76u,g[4]+0x1cu,g[1]);
    if(g[2]<g[1]) {
        do {
            g[1]=read(0x2ecb80u,g[2]+2u,2);g[6]=g[1]>>5u;g[1]&=31u;g[3]=1u<<g[1];
            g[1]=read(0x2ecb93u,g[0]+g[6]*4u);g[2]+=4u;g[1]^=g[3];store(0x2ecb9bu,g[0]+g[6]*4u,g[1]);
        }while(g[2]<read(0x2ecb9eu,g[4]+0x1cu));
        g[6]=read(0x2ecba4u,g[4]+0x28u);
    }
    g[1]=read(0x2ecba8u,g[6]+0x4cu);g[3]=read(0x2ecbabu,g[4]+0x34u);g[1]+=g[3]*4u;g[2]+=4u;
    store(0x2ecbb7u,g[4]+0x1cu,g[1]);
    while(g[2]<g[1]) {
        if(!(read(0x2ecbc0u,g[2],1)&1u)) {
            g[1]=read(0x2ecbc5u,g[2]+2u,2);g[3]=g[1]>>5u;store(0x2ecbceu,g[4]+0x34u,g[3]);
            g[1]&=31u;g[3]=1u<<g[1];g[1]=read(0x2ecbdcu,g[4]+0x34u);
            store(0x2ecbe0u,g[0]+g[1]*4u,read(0x2ecbe0u,g[0]+g[1]*4u)^g[3]);g[1]=read(0x2ecbe3u,g[4]+0x1cu);
        }
        g[2]+=4u;
    }

    // Enumerate candidate words, test Y/Z overlap, and publish removals.
    g[2]=read(0x2ecbeeu,g[4]+0x18u);g[1]=read(0x2ecbf2u,g[2]);store(0x2ecbf4u,g[4]+0x2cu,g[1]);
    g[1]=read(0x2ecbf8u,g[6]+0x44u);g[1]=g[0]+4u+std::uint32_t(signed32(g[1])>>5u)*4u;g[3]=g[0];
    store(0x2ecc06u,g[4]+0x30u,g[3]);store(0x2ecc0au,g[4]+0x34u,g[1]);
    while(g[3]<g[1]) {
        g[2]=read(0x2ecc14u,g[4]+0x2cu);store(0x2ecc18u,g[4]+0x14u,g[2]);
        g[2]=read(0x2ecc1cu,g[3]);store(0x2ecc20u,g[4]+0x1cu,g[2]);
        const bool nonempty_word=g[2]!=0;
        while(g[2]) {
            if(!(g[2]&255u)) {g[1]=read(0x2ecc34u,g[4]+0x14u)+128u;g[2]>>=8u;}
            else {
                if(g[2]&1u) {
                    g[0]=read(0x2ecc4fu,g[4]+0x14u);g[1]=read(0x2ecc53u,g[0]);
                    g[0]=read(0x2ecc55u,g[7]+4u);g[3]=read(0x2ecc58u,g[7]);g[0]-=g[1];
                    g[1]=read(0x2ecc5cu,g[4]+0x14u);g[1]=read(0x2ecc60u,g[1]+4u);g[1]=(g[1]-g[3])|g[0];g[1]&=0x80008000u;
                    store(0x2ecc6du,g[4]+0x38u,g[1]);
                    if(!g[1]) {
                        g[0]=read(0x2ecc77u,g[4]+0x14u);g[3]=read(0x2ecc7bu,g[0]+12u);
                        if(!(g[3]&1u)) {
                            g[0]=read(0x2ecc83u,g[5]+12u);g[1]=read(0x2ecc86u,g[0]+8u);g[2]=read(0x2ecc89u,g[0]+4u);g[1]&=0x7fffffffu;
                            s.flags.minus(g[2],g[1],4);
                            if(g[2]==g[1]) {
                                push(0x2ecc96u,8);push(0x2ecc98u,g[0]);push(0x2ecc99u,0x2ecc9eu);call(s,0x2e3250u,0x2ecc99u);
                                g[0]=read(0x2ecc9eu,g[5]+12u);g[4]=s.flags.plus(g[4],8u,4);
                            }
                            g[2]=read(0x2ecca4u,g[0]+4u);g[0]=read(0x2ecca7u,g[0]);g[1]=read(0x2ecca9u,g[5]+8u);
                            store(0x2eccacu,g[0]+g[2]*8u,g[1]);store(0x2eccafu,g[0]+g[2]*8u+4u,g[3]);
                            g[0]=read(0x2eccb3u,g[5]+12u);g[1]=read(0x2eccb6u,g[0]+4u);g[2]=read(0x2eccb9u,g[4]+0x1cu);
                            ++g[1];store(0x2eccbeu,g[0]+4u,g[1]);++activity().pairs;
                        }else {
                            g[0]=read(0x2eccc3u,g[6]+0x78u);g[3]&=~1u;g[1]=read(0x2eccc9u,g[3]+g[0]+8u);g[3]+=g[0];g[0]=0;
                            bool found=false;
                            if(signed32(g[1])>0) {
                                g[2]=read(0x2eccd5u,g[3]+4u);
                                do {
                                    low(6,read(0x2ecce0u,g[4]+0x24u,2));
                                    if(read(0x2ecce5u,g[2],2)==std::uint16_t(g[6])) {found=true;break;}
                                    ++g[0];g[2]+=2u;
                                }while(signed32(g[0])<signed32(g[1]));
                                g[2]=read(found?0x2ece44u:0x2eccf6u,g[4]+0x1cu);g[6]=read(found?0x2ece48u:0x2eccfau,g[4]+0x28u);
                            }
                            if(!found)g[0]=~0u;
                            g[1]=read(0x2ecd01u,g[3]+8u);--g[1];store(0x2ecd05u,g[3]+8u,g[1]);g[3]=read(0x2ecd08u,g[3]+4u);
                            low(1,read(0x2ecd0bu,g[3]+g[1]*2u,2));store(0x2ecd0fu,g[3]+g[0]*2u,g[1],2);++activity().cell_removals;
                        }
                    }
                }
                g[1]=read(0x2ecd13u,g[4]+0x14u)+16u;g[2]>>=1u;
            }
            store(0x2ecd1eu,g[4]+0x14u,g[1]);store(0x2ecd22u,g[4]+0x1cu,g[2]);
        }
        if(nonempty_word){g[1]=read(0x2ecd2cu,g[4]+0x34u);g[3]=read(0x2ecd30u,g[4]+0x30u);}
        g[0]=read(0x2ecd34u,g[4]+0x2cu);g[3]+=4u;g[0]+=512u;s.flags.minus(g[3],g[1],4);
        store(0x2ecd42u,g[4]+0x2cu,g[0]);store(0x2ecd46u,g[4]+0x30u,g[3]);
    }
    // Empty candidate words skip the inner loop but share the same cursor.
    // The loop's terminating comparison is the destructor's incoming flags.
    s.flags.minus(g[3],g[1],4);g[1]=g[4]+0x3cu;store(0x2ecd54u,g[4]+0x5cu,~0u);
    push(0x2ecd5cu,0x2ecd61u);call(s,0x2eb850u,0x2ecd5cu);

    // Erase the six edges and decrement the packed indices of surviving nodes.
    g[2]=read(0x2ecd61u,g[4]+0x18u);g[1]=read(0x2ecd65u,g[7]+10u,2);g[0]=read(0x2ecd69u,g[2]);g[2]=read(0x2ecd6bu,g[7]+8u,2);
    push(0x2ecd6fu,g[1]);g[3]=g[6]+0x4cu;push(0x2ecd73u,g[2]);g[1]=g[3];store(0x2ecd76u,g[4]+0x3cu,g[0]);
    push(0x2ecd7au,0x2ecd7fu);erase_edges(s);
    g[0]=read(0x2ecd7fu,g[7]+4u,2);g[2]=read(0x2ecd83u,g[7],2);push(0x2ecd86u,g[0]);g[1]=g[6]+0x58u;
    push(0x2ecd8au,g[2]);push(0x2ecd8bu,0x2ecd90u);erase_edges(s);
    g[0]=read(0x2ecd90u,g[7]+6u,2);g[2]=read(0x2ecd94u,g[7]+2u,2);push(0x2ecd98u,g[0]);g[1]=g[6]+0x64u;
    push(0x2ecd9cu,g[2]);push(0x2ecd9du,0x2ecda2u);erase_edges(s);
    g[0]=read(0x2ecda2u,g[4]+0x20u);g[1]=read(0x2ecda6u,g[4]+0x34u);push(0x2ecdaau,g[7]);push(0x2ecdabu,g[0]);push(0x2ecdacu,g[1]);g[1]=g[6];
    push(0x2ecdafu,0x2ecdb4u);reindex(s);

    // Move the final dense node into the hole, then repair edge and owner links.
    g[0]=read(0x2ecdb4u,g[4]+0x20u);g[1]=read(0x2ecdb8u,g[4]+0x24u);g[2]=g[0]-1u;
    if(g[1]<g[2]) {
        ++activity().dense_moves;g[1]=read(0x2ecdc7u,g[4]+0x18u);g[2]=read(0x2ecdcbu,g[1]);g[0]=g[0]*16u+g[2]-16u;
        g[2]=read(0x2ecdd4u,g[0]);g[1]=g[7];store(0x2ecdd8u,g[1],g[2]);g[2]=read(0x2ecddau,g[0]+4u);store(0x2ecdddu,g[1]+4u,g[2]);
        g[2]=read(0x2ecde0u,g[0]+8u);store(0x2ecde3u,g[1]+8u,g[2]);g[0]=read(0x2ecde6u,g[0]+12u);store(0x2ecde9u,g[1]+12u,g[0]);
        g[1]=read(0x2ecdecu,g[7]+8u,2);g[2]=read(0x2ecdf0u,g[3]);g[0]=read(0x2ecdf2u,g[4]+0x24u);store(0x2ecdf6u,g[2]+g[1]*4u+2u,g[0],2);
        g[1]=read(0x2ecdfbu,g[7]+10u,2);g[2]=read(0x2ecdffu,g[3]);store(0x2ece01u,g[2]+g[1]*4u+2u,g[0],2);g[1]=read(0x2ece06u,g[7]+12u);
        if(!(g[1]&1u)) {
            g[1]=read(0x2ece0eu,g[7],2);g[2]=read(0x2ece11u,g[6]+0x58u);store(0x2ece14u,g[2]+g[1]*4u+2u,g[0],2);
            g[1]=read(0x2ece19u,g[7]+4u,2);g[2]=read(0x2ece1du,g[6]+0x58u);store(0x2ece20u,g[2]+g[1]*4u+2u,g[0],2);
            g[1]=read(0x2ece25u,g[7]+2u,2);g[2]=read(0x2ece29u,g[6]+0x64u);store(0x2ece2cu,g[2]+g[1]*4u+2u,g[0],2);
            g[1]=read(0x2ece31u,g[7]+6u,2);g[2]=read(0x2ece35u,g[6]+0x64u);store(0x2ece38u,g[2]+g[1]*4u+2u,g[0],2);
            g[1]=read(0x2ece3du,g[7]+12u);store(0x2ece40u,g[1],g[0]);
        }else {g[2]=read(0x2ece51u,g[6]+0x78u);g[1]&=~1u;store(0x2ece57u,g[1]+g[2],g[0],2);}
        g[0]=read(0x2ece5bu,g[6]+0x70u);
        if(g[0] && !(read(0x2ece66u,g[7]+12u,1)&1u)) {
            low(2,read(0x2ece70u,g[7]+8u,2));g[3]=read(0x2ece74u,g[3]);
            low(0,16u);low(0,std::uint16_t(g[0])-read(0x2ece7au,g[6]+0x74u,2));store(0x2ece7eu,g[4]+0x34u,g[3]);
            g[1]=std::uint16_t(g[0]);g[0]=std::uint16_t(g[2]);g[0]=read(0x2ece88u,g[3]+g[0]*4u,2)>>(g[1]&31u);
            store(0x2ece8eu,g[4]+0x30u,g[1]);
            if(signed32(g[0])>0) {
                g[3]=read(0x2ece96u,g[6]+0x78u);g[1]=read(0x2ece9eu,g[0]*16u+g[3]-16u,2);
                g[3]=read(0x2ecea3u,g[4]+0x18u);g[3]=read(0x2ecea7u,g[3]);g[1]*=16u;
                if(read(0x2eceacu,g[1]+g[3]+10u,2)>std::uint16_t(g[2]))--g[0];
            }
            g[2]=read(0x2eceb4u,g[7]+10u,2);g[1]=read(0x2eceb8u,g[4]+0x34u);g[2]=read(0x2ecebcu,g[1]+g[2]*4u,2);
            low(1,read(0x2ecec0u,g[4]+0x30u,1),1);g[2]=(g[2]>>(g[1]&31u))-1u;
            if(signed32(g[0])<=signed32(g[2])) {
                g[3]=read(0x2ececbu,g[4]+0x20u);g[1]=g[0]*16u;g[2]=g[2]-g[0]+1u;--g[3];
                store(0x2eced8u,g[4]+0x2cu,g[1]);store(0x2ecedcu,g[4]+0x30u,g[2]);
                for(bool first=true;;first=false) {
                    if(!first)g[1]=read(0x2ecee2u,g[4]+0x2cu);
                    g[0]=read(0x2ecee6u,g[6]+0x78u);g[7]=read(0x2ecee9u,g[0]+g[1]+8u);g[0]+=g[1];g[1]=0;
                    bool found=false;
                    if(signed32(g[7])>0) {
                        g[2]=read(0x2ecef5u,g[0]+4u);
                        do {if(read(0x2ecef8u,g[2],2)==std::uint16_t(g[3])){found=true;break;}++g[1];g[2]+=2u;}while(signed32(g[1])<signed32(g[7]));
                    }
                    if(!found)g[1]=~0u;g[2]=read(0x2ecf08u,g[0]+4u);low(0,read(0x2ecf0bu,g[4]+0x24u,2));
                    store(0x2ecf10u,g[2]+g[1]*2u,g[0],2);g[1]=read(0x2ecf14u,g[4]+0x2cu);g[0]=read(0x2ecf18u,g[4]+0x30u);
                    g[1]+=16u;--g[0];store(0x2ecf20u,g[4]+0x2cu,g[1]);store(0x2ecf24u,g[4]+0x30u,g[0]);if(!g[0])break;
                }
            }
        }
    }

    // Retain capacity policy and caller SEH/nonvolatile publication.
    g[7]=read(0x2ecf2au,g[4]+0x18u);g[6]=read(0x2ecf2eu,g[4]+0x20u);g[0]=read(0x2ecf32u,g[7]+8u);--g[6];g[0]&=0x7fffffffu;
    s.flags.minus(g[0],g[6],4);
    if(signed32(g[0])<signed32(g[6])) {
        g[0]+=g[0];s.flags.minus(g[6],g[0],4);if(signed32(g[6])>=signed32(g[0]))g[0]=g[6];
        push(0x2ecf47u,16);push(0x2ecf49u,g[0]);push(0x2ecf4au,g[7]);push(0x2ecf4bu,0x2ecf50u);call(s,0x2e31e0u,0x2ecf4bu);
        g[4]=s.flags.plus(g[4],12u,4);
    }
    g[1]=read(0x2ecf53u,g[4]+0x54u);store(0x2ecf57u,g[7]+4u,g[6]);g[7]=pop(0x2ecf5au);g[6]=pop(0x2ecf5bu);
    store(0x2ecf5cu,s.fs_base,g[1]);g[3]=pop(0x2ecf63u);g[4]=g[5];g[5]=pop(0x2ecf66u);pop(0x2ecf67u);g[4]+=8u;at(0x2ecf67u);
    return true;
}
} // namespace halo2::native_broadphase_remove_detail
