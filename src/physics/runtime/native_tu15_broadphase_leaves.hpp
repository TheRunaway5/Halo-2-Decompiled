#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <emmintrin.h>

namespace halo2::native_tu15_leaves {
// 2E9CF0: repair the existing packed indices after inserting two new ends.
// The thresholds deliberately straddle the two insertion positions; signed
// PCMPGTW and modulo-word subtraction are retained using native SSE2 lanes.
template<class Io,class Flags> void kernel_2e9cf0(Io &io,Machine<Flags> &m) {
    auto &g=m.gp;push(io,m,0x2e9cf0u,g[5]);g[5]=g[4];g[4]=(g[4]&~15u)-48u;
    const auto low=[&](unsigned reg,std::uint32_t value){g[reg]=(g[reg]&0xffff0000u)|(value&65535u);};
    g[0]=io.rd(0x2e9cf9u,g[5]+16u,4);low(1,io.rd(0x2e9cfcu,g[0]+8u,2));low(1,g[1]-1u);
    io.st(0x2e9d02u,g[4]+24u,2,g[1]);io.st(0x2e9d07u,g[4]+26u,2,g[1]);
    low(1,io.rd(0x2e9d0cu,g[0],2));low(1,g[1]-1u);
    io.st(0x2e9d11u,g[4]+16u,2,g[1]);io.st(0x2e9d16u,g[4]+20u,2,g[1]);
    low(1,io.rd(0x2e9d1bu,g[0]+2u,2));low(1,g[1]-1u);
    io.st(0x2e9d21u,g[4]+18u,2,g[1]);io.st(0x2e9d26u,g[4]+22u,2,g[1]);
    low(1,io.rd(0x2e9d2bu,g[0]+10u,2));low(1,g[1]-2u);
    io.st(0x2e9d33u,g[4]+40u,2,g[1]);io.st(0x2e9d38u,g[4]+42u,2,g[1]);
    low(1,io.rd(0x2e9d3du,g[0]+4u,2));low(0,io.rd(0x2e9d41u,g[0]+6u,2));low(0,g[0]-2u);low(1,g[1]-2u);
    io.st(0x2e9d4du,g[4]+34u,2,g[0]);io.st(0x2e9d52u,g[4]+38u,2,g[0]);
    g[0]=io.rd(0x2e9d57u,g[5]+12u,4);io.st(0x2e9d5au,g[4]+32u,2,g[1]);io.st(0x2e9d5fu,g[4]+36u,2,g[1]);
    g[1]=io.rd(0x2e9d64u,g[5]+8u,4);g[0]=g[1]+g[0]*16u;m.flags.minus(g[1],g[0],4);
    const auto half=[&](std::uint32_t pc,std::uint32_t p){return _mm_castps_si128(io.half(pc,p,_mm_setzero_ps(),0));};
    const auto low64=[](__m128i value){return std::uint64_t(_mm_cvtsi128_si64(value));};
    if(m.flags.cf) {
        m.reset_x87_top=true;
        const auto low_x=half(0x2e9d74u,g[4]+24u),low_yz=half(0x2e9d79u,g[4]+16u);
        const auto high_x=half(0x2e9d7eu,g[4]+40u),high_yz=half(0x2e9d83u,g[4]+32u);
        g[2]=65535u;io.st(0x2e9d89u,g[4],2,g[2]);io.st(0x2e9d8du,g[4]+2u,2,g[2]);g[2]=0;
        io.st(0x2e9d94u,g[4]+4u,2,0);io.st(0x2e9d99u,g[4]+6u,2,0);
        do {
            const auto high=half(0x2e9da0u,g[1]+8u);
            auto x_delta=_mm_add_epi16(_mm_cmpgt_epi16(high,low_x),_mm_cmpgt_epi16(high,high_x));
            x_delta=_mm_and_si128(x_delta,half(0x2e9db3u,g[4]));const auto low_words=half(0x2e9db7u,g[1]);
            const auto yz_delta=_mm_add_epi16(_mm_cmpgt_epi16(low_words,high_yz),_mm_cmpgt_epi16(low_words,low_yz));
            io.st(0x2e9dccu,g[1],8,low64(_mm_sub_epi16(low_words,yz_delta)));
            const auto high_reloaded=half(0x2e9dcfu,g[1]+8u);io.st(0x2e9dd6u,g[1]+8u,8,low64(_mm_sub_epi16(high_reloaded,x_delta)));
            g[1]+=16u;m.flags.minus(g[1],g[0],4);
        }while(m.flags.cf);
    }
    m.reset_x87=true;io.at(0x2e9de1u);g[4]=g[5];g[5]=pop(io,m,0x2e9de5u);pop(io,m,0x2e9de6u);g[4]+=12u;
}

// 2E9DF0: decrement packed per-node edge indices after removing both ends of
// one bounds record. Signed word comparisons and modulo-word adds reproduce
// the original MMX algorithm using native SSE2; no MMX register file executes.
template<class Io,class Flags> void kernel_2e9df0(Io &io,Machine<Flags> &m) {
    auto &gp=m.gp;
    push(io,m,0x2e9df0u,gp[5]);gp[5]=gp[4];
    gp[4]=(gp[4]&~15u)-48u;
    gp[0]=io.rd(0x2e9df9u,gp[5]+16u,4);
    const auto low_word=[&](unsigned reg,std::uint32_t value){gp[reg]=(gp[reg]&0xffff0000u)|(value&65535u);};
    low_word(1,io.rd(0x2e9dfcu,gp[0]+8u,2));
    io.st(0x2e9e00u,gp[4]+24u,2,gp[1]);io.st(0x2e9e05u,gp[4]+26u,2,gp[1]);
    low_word(1,io.rd(0x2e9e0au,gp[0],2));
    io.st(0x2e9e0du,gp[4]+16u,2,gp[1]);io.st(0x2e9e12u,gp[4]+20u,2,gp[1]);
    low_word(1,io.rd(0x2e9e17u,gp[0]+2u,2));
    io.st(0x2e9e1bu,gp[4]+18u,2,gp[1]);io.st(0x2e9e20u,gp[4]+22u,2,gp[1]);
    low_word(1,io.rd(0x2e9e25u,gp[0]+10u,2));
    io.st(0x2e9e29u,gp[4]+40u,2,gp[1]);io.st(0x2e9e2eu,gp[4]+42u,2,gp[1]);
    low_word(1,io.rd(0x2e9e33u,gp[0]+4u,2));
    low_word(0,io.rd(0x2e9e37u,gp[0]+6u,2));
    io.st(0x2e9e3bu,gp[4]+34u,2,gp[0]);io.st(0x2e9e40u,gp[4]+38u,2,gp[0]);
    gp[0]=io.rd(0x2e9e45u,gp[5]+12u,4);
    io.st(0x2e9e48u,gp[4]+32u,2,gp[1]);io.st(0x2e9e4du,gp[4]+36u,2,gp[1]);
    gp[1]=io.rd(0x2e9e52u,gp[5]+8u,4);
    gp[0]=gp[1]+gp[0]*16u;gp[2]=0xffffffffu;
    m.flags.minus(gp[1],gp[0],4);
    io.st(0x2e9e5fu,gp[4]+28u,4,gp[2]);io.st(0x2e9e63u,gp[4]+44u,4,gp[2]);
    const auto half=[&](std::uint32_t pc,std::uint32_t p) {
        return _mm_castps_si128(io.half(pc,p,_mm_setzero_ps(),0));
    };
    const auto low64=[](__m128i value){return std::uint64_t(_mm_cvtsi128_si64(value));};
    if(m.flags.cf) {
        m.reset_x87_top=true;
        const auto low_x=half(0x2e9e69u,gp[4]+24u),low_yz=half(0x2e9e6eu,gp[4]+16u);
        const auto high_x=half(0x2e9e73u,gp[4]+40u),high_yz=half(0x2e9e78u,gp[4]+32u);
        gp[2]=65535u;
        io.st(0x2e9e82u,gp[4],2,gp[2]);io.st(0x2e9e86u,gp[4]+2u,2,gp[2]);
        gp[2]=0;
        io.st(0x2e9e8du,gp[4]+4u,2,0);io.st(0x2e9e92u,gp[4]+6u,2,0);
        do {
            const auto high=half(0x2e9ea0u,gp[1]+8u);
            auto x_delta=_mm_add_epi16(_mm_cmpgt_epi16(high,low_x),_mm_cmpgt_epi16(high,high_x));
            x_delta=_mm_and_si128(x_delta,half(0x2e9eb3u,gp[4]));
            const auto low=half(0x2e9eb7u,gp[1]);
            const auto yz_delta=_mm_add_epi16(_mm_cmpgt_epi16(low,high_yz),_mm_cmpgt_epi16(low,low_yz));
            io.st(0x2e9eccu,gp[1],8,low64(_mm_add_epi16(low,yz_delta)));
            const auto reloaded_high=half(0x2e9ecfu,gp[1]+8u);
            io.st(0x2e9ed6u,gp[1]+8u,8,low64(_mm_add_epi16(reloaded_high,x_delta)));
            gp[1]+=16u;m.flags.minus(gp[1],gp[0],4);
        } while(m.flags.cf);
    }
    m.reset_x87=true;io.at(0x2e9ee1u);
    gp[4]=gp[5];gp[5]=pop(io,m,0x2e9ee5u);
    pop(io,m,0x2e9ee6u);gp[4]+=12u;
}

// 2E9FD0: remove two entries from one sweep-edge array. Each move remains an
// ordered scalar delivery so overlap, guards and the caller's direction flag
// retain their exact original behavior.
template<class Io,class Flags> void kernel_2e9fd0(Io &io,Machine<Flags> &m) {
    auto &gp=m.gp;
    gp[0]=io.rd(0x2e9fd0u,gp[4]+4u,4);gp[2]=gp[1];
    gp[1]=io.rd(0x2e9fd6u,gp[2],4);
    push(io,m,0x2e9fd8u,gp[6]);gp[6]=io.rd(0x2e9fd9u,gp[4]+12u,4);
    gp[0]=gp[1]+gp[0]*4u;gp[1]=gp[1]+gp[6]*4u-4u;
    m.flags.minus(gp[0],gp[1],4);push(io,m,0x2e9fe6u,gp[7]);
    const auto move=[&](std::uint32_t pc) {
        const auto delta=(m.other_flags&1024u)?0xfffffffcu:4u;
        while(gp[1]) {
            const auto value=io.rd(pc,gp[6],4);io.st(pc,gp[7],4,value);
            gp[6]+=delta;gp[7]+=delta;--gp[1];
        }
    };
    const auto distance=[&](std::uint32_t end,std::uint32_t begin) {
        auto value=m.flags.minus(end,begin,4);
        value=m.flags.increment(value,4,true);
        value=m.flags.right(value,2u,4,false);
        return m.flags.increment(value,4,false);
    };
    if(m.flags.cf) {
        gp[1]=distance(gp[1],gp[0]);push(io,m,0x2e9ff0u,gp[3]);
        gp[3]=gp[1];gp[6]=gp[0]+4u;gp[7]=gp[0];gp[0]+=gp[3]*4u;
        move(0x2e9ffbu);gp[3]=pop(io,m,0x2e9ffdu);
    }
    gp[1]=io.rd(0x2e9ffeu,gp[2]+4u,4);gp[6]=gp[1]-2u;
    gp[1]=io.rd(0x2ea004u,gp[2],4);gp[1]+=gp[6]*4u;
    m.flags.minus(gp[0],gp[1],4);io.st(0x2ea00bu,gp[2]+4u,4,gp[6]);
    if(m.flags.cf) {
        gp[1]=distance(gp[1],gp[0]);
        gp[6]=gp[0]+8u;gp[7]=gp[0];move(0x2ea01cu);
    }
    gp[7]=pop(io,m,0x2ea01eu);gp[6]=pop(io,m,0x2ea01fu);
    pop(io,m,0x2ea020u);gp[4]+=8u;
}
} // namespace halo2::native_tu15_leaves
