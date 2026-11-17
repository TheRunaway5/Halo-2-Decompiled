#pragma once
// Native TU1.5 PVS update closure, entered at 1532A0 from 13B610 (13B6F0):
//   1532A0  OR the portal-flooded visibility of every flagged scenario
//           cluster (row bit set in [[4F54F4]+0x48]) into the caller vector
//   170C80  per-cluster portal fixed point: start from the cluster's PVS row,
//           repeatedly add (row(a)|row(b))&row(c) through every open portal
//           (flag 4, global portal bit) of every currently visible cluster
//   170FB0  select the cluster's row in the second (or first) PVS matrix
//   44680   next-set-bit iterator over a cluster bit vector
// Ordinary native control flow and arithmetic. The guest bank keeps its
// original ABI at this boundary, so every original memory access still
// happens in program order, at the same address and width, including the
// stack frame (pushes, spills, the 0x40-byte local vectors and return
// addresses) and every store goes through the unit's store helper. The
// instruction cursor is published before each access, so a fault reports the
// original instruction. No translated instruction runs.
// HALO2_REFERENCE_PVS_FLOOD=1 restores the translated bodies.
// HALO2_VERIFY_PVS_FLOOD=1 runs, for every admitted call, the native closure
// and the unmodified translated closure on separate copy-on-write overlays,
// requires the identical ordered store sequence (pc, address, width, bytes),
// registers and flags, then runs the native closure on the live bank. A
// summary (calls, stores, branch coverage) is printed at exit.
#include "physics/runtime/physics_step_internal.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace halo2::native_pvs_flood_detail {
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PVS_FLOOD");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PVS_FLOOD");return p && *p;}();
    return value;
}
inline bool &bypass(){thread_local bool value=false;return value;}
// HALO2_CENSUS_NATIVE_BRIDGES=1 (with HALO2_INSTRUCTION_CENSUS and
// HALO2_REFERENCE_INSTRUCTION_CALLBACKS) admits the native closure under full
// instruction callbacks, so a census counts only what still runs translated.
inline bool census_admission() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_CENSUS_NATIVE_BRIDGES");return p && *p;}();
    return value;
}

// Branch outcomes reached by the native closure (verification coverage).
enum Branch : unsigned {
    scan_skip, scan_hit, scan_end, scan_negative, row_zero, flag_skip, cluster_skip, call_flood,
    or_empty, or_words, flood_plain, flood_flagged, select_second, select_first, portals_none,
    portals_some, portal_visited, portal_closed_flag, portal_closed_global, portal_open,
    fixed_point_equal, fixed_point_repeat, bit_reload, bit_reload_word, bit_end, bit_found,
    copy_bytes, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}

struct Registers { std::uint32_t eax{},ecx{},edx{},ebx{},ebp{},esi{},edi{}; };

inline std::uint32_t sx16(std::uint32_t x){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(x))));}
inline std::int32_t si(std::uint32_t x){return std::int32_t(x);}
inline std::uint32_t sar(std::uint32_t x,unsigned n){return std::uint32_t(std::int32_t(x)>>n);}

template<class Io> struct Closure {
    Io &io;
    std::uint64_t hit{};
    void mark(Branch b){hit|=std::uint64_t(1)<<b;}
    // REP MOVSD/MOVSB with DF clear (admission refuses DF): ordered per element.
    void movsd(std::uint32_t pc,std::uint32_t &src,std::uint32_t &dst,std::uint32_t count) {
        for(;count;--count,src+=4u,dst+=4u){const auto x=io.rd(pc,src,4);io.st(pc,dst,4,x);}
    }
    void movsb(std::uint32_t pc,std::uint32_t &src,std::uint32_t &dst,std::uint32_t count) {
        if(count)mark(copy_bytes);
        for(;count;--count,++src,++dst){const auto x=io.rd(pc,src,1);io.st(pc,dst,1,x);}
    }
    void stos(std::uint32_t pc,std::uint32_t &dst,std::uint32_t count,unsigned width) {
        for(;count;--count,dst+=width)io.st(pc,dst,width,0);
    }

    // 44680: EAX=iterator {int16 count, int16 word, u32 pending, u32 words*},
    // [ESP+4]=index out. `entry` is ESP at entry. Returns AL; ESI is restored.
    bool next_bit(std::uint32_t entry,std::uint32_t it,std::uint32_t esi) {
        auto ecx=io.rd(0x44680u,it+4u,4);
        io.st(0x44688u,entry-12u,4,esi);
        if(!ecx) {
            mark(bit_reload);
            const auto count=io.rd(0x4468bu,it,2);
            for(;;) {
                const auto word=(io.rd(0x44690u,it+2u,2)+1u)&0xffffu;
                io.st(0x44690u,it+2u,2,word);
                const auto index=io.rd(0x44694u,it+2u,2);
                if(std::int16_t(index)>=std::int16_t(count))break;
                const auto words=io.rd(0x4469du,it+8u,4);
                ecx=io.rd(0x446a3u,words+sx16(index)*4u,4);
                io.st(0x446a8u,it+4u,4,ecx);
                mark(bit_reload_word);
                if(ecx)break;
            }
        }
        const auto index=io.rd(0x446adu,it+2u,2);
        if(std::int16_t(index)>=std::int16_t(io.rd(0x446b1u,it,2))) {
            mark(bit_end);
            io.rd(0x446fau,entry-12u,4);io.rd(0x446feu,entry,4);
            return false;
        }
        mark(bit_found);
        io.st(0x446b9u,entry-8u,4,io.rd(0x446b6u,it+4u,4));
        std::uint32_t bit=0xffffffffu;
        if(const auto pending=io.rd(0x446c2u,entry-8u,4))bit=std::uint32_t(std::countr_zero(pending));
        io.st(0x446c7u,entry-4u,4,bit);
        const auto word=sx16(io.rd(0x446cbu,it+2u,2));
        const auto low=io.rd(0x446cfu,entry-4u,4);
        const auto out=io.rd(0x446d3u,entry+4u,4);
        io.st(0x446dcu,out,4,(word<<5)+low);
        const auto keep=~(1u<<(low&31u));
        const auto pending=io.rd(0x446e5u,it+4u,4);
        io.rd(0x446e8u,entry-12u,4);
        io.st(0x446edu,it+4u,4,pending&keep);
        io.rd(0x446f5u,entry,4);
        return true;
    }

    // 170FB0: EDX=bsp, AX=cluster. `entry` is ESP at entry. Returns EAX and
    // the popped EBX (the only results 170C80 consumes).
    std::uint32_t select_row(std::uint32_t entry,std::uint32_t bsp,std::uint32_t cluster,
        std::uint32_t &ebx,std::uint32_t esi,std::uint32_t edi) {
        io.st(0x170fb0u,entry-4u,4,ebx);
        const auto size=io.rd(0x170fb1u,bsp+0x54u,4);
        io.st(0x170fb4u,entry-8u,4,esi);
        const auto count=io.rd(0x170fb8u,bsp+0x9cu,4);
        const auto words=sar(count+0x1fu,5);
        const auto first=count*words;
        io.st(0x170fc7u,entry-12u,4,edi);
        std::uint32_t row;
        if(first*4u<size) {
            mark(select_second);
            const auto index=sx16(cluster)*words+first;
            const auto base=io.rd(0x170fdbu,bsp+0x58u,4);
            io.rd(0x170fdeu,entry-12u,4);
            row=base+index*4u;
            io.rd(0x170fe2u,entry-8u,4);ebx=io.rd(0x170fe3u,entry-4u,4);io.rd(0x170fe4u,entry,4);
        } else {
            mark(select_first);
            const auto again=io.rd(0x170fe5u,bsp+0x9cu,4);
            const auto base=io.rd(0x170febu,bsp+0x58u,4);
            const auto index=sar(again+0x1fu,5)*sx16(cluster);
            io.rd(0x170ffau,entry-12u,4);io.rd(0x170ffbu,entry-8u,4);
            row=base+index*4u;
            ebx=io.rd(0x170fffu,entry-4u,4);io.rd(0x171000u,entry,4);
        }
        return row;
    }

    // 170C80: AX=cluster, [ESP+4]=bsp, [ESP+8]=output vector; RET 8.
    // `entry` is ESP at entry; `r` holds the caller's EBX/EBP/ESI/EDI and
    // receives the popped values and the exit EAX/ECX/EDX.
    void flood(std::uint32_t entry,Registers &r) {
        const auto cluster=r.eax;
        io.st(0x170c86u,entry-0xf4u,4,r.ebx);
        io.st(0x170c87u,entry-0xf8u,4,r.ebp);
        const auto bsp=io.rd(0x170c88u,entry+4u,4);
        auto ebx=io.rd(0x170c8fu,bsp+0x9cu,4)+0x1fu;
        const auto c=sx16(cluster);
        io.st(0x170c9bu,entry-0xfcu,4,r.esi);
        const auto words=sar(ebx,5);
        const auto record=c*0xb0u;
        const auto offset=c*words;
        io.st(0x170cacu,entry-0x100u,4,r.edi);
        const auto frame=entry-0x100u;
        const auto bytes=words*4u;
        const auto base=io.rd(0x170cb4u,bsp+0x58u,4);
        const auto own=base+offset*4u;
        const auto clusters=io.rd(0x170cbau,bsp+0xa0u,4);
        const bool flagged=io.rd(0x170cc0u,record+clusters+0x80u,1)&2u;
        io.st(0x170cc8u,frame+0x10u,4,bytes);
        io.st(0x170cccu,frame+0x30u,4,own);
        if(!flagged) {
            mark(flood_plain);
            auto dst=io.rd(0x170f88u,frame+0x108u,4),src=own;
            movsd(0x170f94u,src,dst,bytes>>2);movsb(0x170f9bu,src,dst,bytes&3u);
            r.ecx=0;r.edx=bytes;
        } else {
            mark(flood_flagged);
            io.st(0x170cd8u,frame-4u,4,0x170cddu);
            const auto second=select_row(frame-4u,bsp,cluster,ebx,own,bytes);
            auto length=io.rd(0x170cddu,frame+0x10u,4);
            auto out=io.rd(0x170ce1u,frame+0x108u,4);
            {auto src=second,dst=out;movsd(0x170cf1u,src,dst,length>>2);movsb(0x170cf8u,src,dst,length&3u);}
            {auto src=out,dst=frame+0x80u;movsd(0x170d08u,src,dst,length>>2);movsb(0x170d0fu,src,dst,length&3u);}
            const auto visit_bytes=sar(io.rd(0x170d11u,bsp+0x5cu,4)+0x1fu,5)<<2;
            {auto dst=frame+0xc0u;stos(0x170d2bu,dst,visit_bytes>>2,4);stos(0x170d35u,dst,visit_bytes&3u,1);}
            ebx=sar(ebx,5);
            io.st(0x170d37u,frame+0x14u,4,ebx);
            auto esi=visit_bytes;
            const auto it=frame+0x34u;
            for(;;) {
                // L170D40: iterate the current output vector.
                const auto first=io.rd(0x170d40u,out,4);
                io.st(0x170d42u,frame+0x3cu,4,out);
                io.st(0x170d4au,frame-4u,4,frame+0x20u);
                io.st(0x170d4fu,frame+0x34u,2,ebx&0xffffu);
                io.st(0x170d54u,frame+0x36u,2,0);
                io.st(0x170d5bu,frame+0x38u,4,first);
                io.st(0x170d5fu,frame-8u,4,0x170d64u);
                bool found=next_bit(frame-8u,it,esi);
                while(found) {
                    const auto k=io.rd(0x170d70u,frame+0x20u,4);
                    esi=io.rd(0x170d74u,bsp+0xa0u,4);
                    const auto portals=io.rd(0x170d80u,k*0xb0u+esi+0x8cu,4);
                    const auto entry_record=k*0xb0u+esi;
                    if(si(portals)>0) {
                        mark(portals_some);
                        auto list=io.rd(0x170d91u,entry_record+0x90u,4);
                        io.st(0x170d97u,frame+0x24u,4,list);
                        io.st(0x170d9bu,frame+0x18u,4,portals);
                        for(;;) {
                            const auto p=sx16(io.rd(0x170da0u,list,2));
                            const auto byte_offset=sar(p,5)<<2;
                            const auto mask=1u<<(p&31u);
                            const auto visited=io.rd(0x170db7u,frame+0xc0u+byte_offset,4);
                            auto word=frame+0xc0u+byte_offset;
                            io.st(0x170dc7u,frame+0x28u,4,mask);
                            io.st(0x170dcbu,frame+0x1cu,4,word);
                            esi=mask;
                            if(visited&mask)mark(portal_visited);
                            else {
                                const auto table=io.rd(0x170dd5u,bsp+0x60u,4);
                                const auto portal=table+p*36u;
                                const bool open=io.rd(0x170ddbu,portal+0x18u,1)&4u;
                                io.st(0x170de5u,frame+0x2cu,4,portal);
                                bool join=false;
                                if(!open)mark(portal_closed_flag);
                                else if(!(io.rd(0x170df5u,byte_offset+io.rd(0x170defu,0x505e7cu,4),4)&mask))mark(portal_closed_global);
                                else join=true;
                                if(join) {
                                    mark(portal_open);
                                    const auto count=io.rd(0x170dfeu,bsp+0x9cu,4);
                                    const auto a=io.rd(0x170e04u,portal,2);
                                    const auto size=io.rd(0x170e07u,bsp+0x54u,4);
                                    const auto count_words=sar(count+0x1fu,5);
                                    const auto plane=count*count_words;
                                    const auto rows=io.rd(0x170e1cu,bsp+0x58u,4);
                                    std::uint32_t source;
                                    if(plane*4u<size)source=rows+(sx16(a)*count_words+plane)*4u;
                                    else source=rows+(sar(io.rd(0x170e2eu,bsp+0x9cu,4)+0x1fu,5)*sx16(a))*4u;
                                    const auto copy=io.rd(0x170e43u,frame+0x10u,4);
                                    auto dst=frame+0x40u;
                                    movsd(0x170e50u,source,dst,copy>>2);
                                    const auto again=io.rd(0x170e54u,bsp+0x9cu,4);
                                    movsb(0x170e5du,source,dst,copy&3u);
                                    const auto b=io.rd(0x170e63u,io.rd(0x170e5fu,frame+0x2cu,4)+2u,2);
                                    const auto again_words=sar(again+0x1fu,5);
                                    const auto again_plane=again*again_words;
                                    std::uint32_t other;
                                    if(again_plane*4u<size)other=rows+(sx16(b)*again_words+again_plane)*4u;
                                    else other=rows+(sar(io.rd(0x170e88u,bsp+0x9cu,4)+0x1fu,5)*sx16(b))*4u;
                                    const auto last=io.rd(0x170e9du,frame+0x14u,4)-1u;
                                    for(auto i=last;si(i)>=0;--i) {
                                        const auto mine=io.rd(0x170eb0u,frame+0x40u+i*4u,4);
                                        io.st(0x170ebeu,frame+0x40u+i*4u,4,mine|io.rd(0x170eb7u,other+i*4u,4));
                                    }
                                    if(si(last)>=0) {
                                        const auto limit=io.rd(0x170ecau,frame+0x30u,4);
                                        for(auto i=last;si(i)>=0;--i) {
                                            const auto mine=io.rd(0x170ed4u,frame+0x40u+i*4u,4);
                                            io.st(0x170ee2u,frame+0x40u+i*4u,4,mine&io.rd(0x170edbu,limit+i*4u,4));
                                        }
                                        for(auto i=last;si(i)>=0;--i) {
                                            const auto mine=io.rd(0x170ef0u,frame+0x80u+i*4u,4);
                                            io.st(0x170efeu,frame+0x80u+i*4u,4,mine|io.rd(0x170ef7u,frame+0x40u+i*4u,4));
                                        }
                                    }
                                    esi=io.rd(0x170f07u,frame+0x28u,4);
                                    word=io.rd(0x170f0bu,frame+0x1cu,4);
                                }
                                io.st(0x170f0fu,word,4,io.rd(0x170f0fu,word,4)|esi);
                            }
                            list=io.rd(0x170f11u,frame+0x24u,4)+2u;
                            const auto remaining=io.rd(0x170f15u,frame+0x18u,4)-1u;
                            io.st(0x170f1du,frame+0x24u,4,list);
                            io.st(0x170f21u,frame+0x18u,4,remaining);
                            if(!remaining)break;
                        }
                    } else mark(portals_none);
                    io.st(0x170f2fu,frame-4u,4,frame+0x20u);
                    io.st(0x170f34u,frame-8u,4,0x170f39u);
                    found=next_bit(frame-8u,it,esi);
                }
                // L170F41: REPE CMPSB of the flood against the output vector.
                length=io.rd(0x170f41u,frame+0x10u,4);
                out=io.rd(0x170f45u,frame+0x108u,4);
                auto remaining=length;bool equal=true;
                for(std::uint32_t a=frame+0x80u,b=out;remaining;) {
                    const auto x=io.rd(0x170f59u,a,1),y=io.rd(0x170f59u,b,1);
                    ++a;++b;--remaining;
                    if(x!=y){equal=false;break;}
                }
                if(equal) {
                    mark(fixed_point_equal);
                    r.eax=out;r.ecx=remaining;r.edx=length;break;
                }
                mark(fixed_point_repeat);
                ebx=io.rd(0x170f5du,frame+0x14u,4);
                const auto next=io.rd(0x170f63u,frame+0x108u,4);
                auto src=frame+0x80u,dst=out;
                movsd(0x170f78u,src,dst,length>>2);movsb(0x170f7fu,src,dst,length&3u);
                esi=src;out=next;
            }
        }
        r.edi=io.rd(0x170f9du,frame,4);r.esi=io.rd(0x170f9eu,frame+4u,4);
        r.ebp=io.rd(0x170f9fu,frame+8u,4);r.ebx=io.rd(0x170fa0u,frame+12u,4);
        io.rd(0x170fa7u,entry,4);
    }

    // 1532A0: [ESP+4]=destination vector, [ESP+8] low byte=require portal
    // word; RET 8. `entry` is ESP at entry. Returns every exit register.
    Registers run(std::uint32_t entry,Registers r) {
        io.st(0x1532a3u,entry-0x44u,4,r.ebx);
        io.st(0x1532a4u,entry-0x48u,4,r.ebp);
        r.ebp=io.rd(0x1532a5u,0x4ecc18u,4);
        io.st(0x1532abu,entry-0x4cu,4,r.esi);
        io.st(0x1532acu,entry-0x50u,4,r.edi);
        const auto frame=entry-0x50u;
        r.edi=io.rd(0x1532adu,0x4f54f4u,4);
        r.esi=0xffffffffu;
        for(;;) {
            // L1532B6
            r.eax=r.esi+1u;
            if(si(r.eax)<0){mark(scan_negative);break;}
            r.esi=io.rd(0x1532bdu,r.edi+0x38u,4);
            if(si(r.eax)>=si(r.esi)){mark(scan_end);break;}
            r.edx=io.rd(0x1532c4u,r.edi+0x48u,4);
            bool hit_bit=false;
            do {
                r.ebx=1u<<(r.eax&31u);r.ecx=sar(r.eax,5);
                if(io.rd(0x1532e1u,r.edx+r.ecx*4u,4)&r.ebx){hit_bit=true;break;}
                mark(scan_skip);++r.eax;
            } while(si(r.eax)<si(r.esi));
            if(!hit_bit){mark(scan_end);break;}
            mark(scan_hit);
            if(r.eax==0xffffffffu)break;
            r.ecx=io.rd(0x1532fau,r.edi+0x24u,4);
            r.edx=io.rd(0x1532fdu,r.edi+0x44u,4);
            r.ecx=r.ecx*r.eax+r.edx;
            r.esi=r.eax;
            if(!r.ecx){mark(row_zero);break;}
            r.eax=(r.eax&0xffffff00u)|io.rd(0x153309u,frame+0x58u,1);
            if((r.eax&0xffu) && io.rd(0x153311u,r.ecx+0x28u,2)==0xffffu){mark(flag_skip);continue;}
            r.eax=io.rd(0x15331au,r.ecx+0x2au,2);
            if(r.eax==0xffffu){mark(cluster_skip);continue;}
            mark(call_flood);
            r.edx=frame+0x10u;
            io.st(0x153328u,frame-4u,4,r.edx);
            io.st(0x153329u,frame-8u,4,r.ebp);
            io.st(0x15332au,frame-12u,4,0x15332fu);
            flood(frame-12u,r);
            r.ecx=sar(io.rd(0x15332fu,r.ebp+0x9cu,4)+0x1fu,5)-1u;
            if(si(r.ecx)<0){mark(or_empty);continue;}
            mark(or_words);
            r.ebx=io.rd(0x153342u,frame+0x54u,4);
            r.edx=frame+0x10u;
            r.eax=r.ebx+r.ecx*4u;
            r.edx-=r.ebx;
            ++r.ecx;
            do {
                r.ebx=io.rd(0x153350u,r.edx+r.eax,4);
                io.st(0x153353u,r.eax,4,io.rd(0x153353u,r.eax,4)|r.ebx);
                r.eax-=4u;
            } while(--r.ecx);
        }
        r.edi=io.rd(0x1532ebu,frame,4);r.esi=io.rd(0x1532ecu,frame+4u,4);
        r.ebp=io.rd(0x1532edu,frame+8u,4);r.ebx=io.rd(0x1532eeu,frame+12u,4);
        io.rd(0x1532f2u,entry,4);
        return r;
    }
};

// Live bank access through the unit's primitives.
template<class State> struct LiveIo {
    State &s;std::uint32_t *cursor{};
    std::uint32_t rd(std::uint32_t pc,std::uint32_t a,unsigned n){if(cursor)*cursor=pc;return physics_step_detail::r(s.memory,a,n);}
    void st(std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t x){if(cursor)*cursor=pc;physics_step_detail::w(s.memory,s.observe,pc,a,n,x);}
};
struct Store {
    std::uint32_t pc,address;unsigned width;std::uint32_t value;
    bool operator==(const Store&)const=default;
};
// Copy-on-write view of the live bank for verification; never writes it.
struct Overlay : SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory &base;
    std::map<std::uint32_t,std::byte> bytes;
    explicit Overlay(const SourcePhysicsContactMemory &memory):base(memory){}
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        base.read(address,output);
        for(std::size_t n=0;n<output.size();++n)
            if(const auto at=bytes.find(address+std::uint32_t(n));at!=bytes.end())output[n]=at->second;
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        for(std::size_t n=0;n<input.size();++n)bytes[address+std::uint32_t(n)]=input[n];
    }
};
struct OverlayIo {
    Overlay memory;std::vector<Store> stores;
    std::uint32_t rd(std::uint32_t,std::uint32_t a,unsigned n){return physics_step_detail::r(memory,a,n);}
    void st(std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t x) {
        std::array<std::byte,4> b{};for(unsigned i=0;i<n;++i)b[i]=std::byte((x>>(8*i))&255u);
        memory.write(a,std::span<const std::byte>(b).first(n));stores.push_back({pc,a,n,x});
    }
};

struct VerifySummary {
    std::uint64_t calls{},stores{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"PVS flood verify: %llu calls, %llu ordered stores identical, coverage %llx\n",
            (unsigned long long)calls,(unsigned long long)stores,(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

template<class State> Registers entry_registers(const State &s) {
    return {s.gp[0],s.gp[1],s.gp[2],s.gp[3],s.gp[5],s.gp[6],s.gp[7]};
}
template<class State> void publish(State &s,const Registers &r,std::uint32_t entry) {
    s.gp[0]=r.eax;s.gp[1]=r.ecx;s.gp[2]=r.edx;s.gp[3]=r.ebx;s.gp[5]=r.ebp;s.gp[6]=r.esi;s.gp[7]=r.edi;
    s.flags.plus(entry-0x40u,0x40u,4);
    s.gp[4]=entry+12u;
}

template<class State,class Reference> void verify(State &s,Reference reference) {
    const auto entry=s.gp[4];
    OverlayIo native{Overlay{s.memory},{}};
    Closure<OverlayIo> closure{native};
    const auto mine=closure.run(entry,entry_registers(s));
    auto flags=s.flags;flags.plus(entry-0x40u,0x40u,4);
    Overlay overlay{s.memory};std::vector<Store> original;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e) {
        std::uint32_t value=0;for(std::size_t i=0;i<e.written.size();++i)value|=std::to_integer<std::uint32_t>(e.written[i])<<(8*i);
        original.push_back({e.instruction,e.address,unsigned(e.written.size()),value});
    };
    std::remove_cvref_t<decltype(s.services)> services{};
    State copy{overlay,observe,services,s.image};
    copy.gp=s.gp;copy.flags=s.flags;copy.other_flags=s.other_flags;copy.fs_base=s.fs_base;copy.fp=s.fp;
    for(unsigned i=0;i<8;++i)copy.xmm[i]=s.xmm[i];
    {struct Hold{Hold(){bypass()=true;}~Hold(){bypass()=false;}} hold;reference(copy);}
    bool same=native.stores==original && copy.gp[4]==entry+12u &&
        copy.gp[0]==mine.eax && copy.gp[1]==mine.ecx && copy.gp[2]==mine.edx && copy.gp[3]==mine.ebx &&
        copy.gp[5]==mine.ebp && copy.gp[6]==mine.esi && copy.gp[7]==mine.edi &&
        copy.flags.cf==flags.cf && copy.flags.zf==flags.zf && copy.flags.sf==flags.sf &&
        copy.flags.of==flags.of && copy.flags.pf==flags.pf;
    if constexpr(requires{flags.af;})same=same && copy.flags.af==flags.af;
    if(!same) {
        std::size_t at=0;while(at<native.stores.size() && at<original.size() && native.stores[at]==original[at])++at;
        std::string detail="Native PVS flood differs from 1532A0 at store "+std::to_string(at)+" of "+
            std::to_string(native.stores.size())+"/"+std::to_string(original.size());
        if(at<native.stores.size() && at<original.size()) {
            char text[160];const auto &a=native.stores[at],&b=original[at];
            std::snprintf(text,sizeof text," native pc=%x a=%x w=%u v=%x original pc=%x a=%x w=%u v=%x",
                a.pc,a.address,a.width,a.value,b.pc,b.address,b.width,b.value);detail+=text;
        }
        throw std::runtime_error(detail);
    }
    coverage().fetch_or(closure.hit);
    auto &total=summary();++total.calls;total.stores+=original.size();
}

// Admission keeps every case outside the authenticated domain translated:
// another image, direction flag set, active traces/service sites, and
// vectors larger than the original 0x40-byte frame buffers (whose overflow
// would alias the frame itself).
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(reference_mode() || bypass() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor=nullptr;
    if(s.services.instruction && census_admission() && !s.instruction_cursor) {
        // Diagnostic census only: full callbacks without a literal cursor.
        // The closure has no service site; skipping its per-instruction
        // callbacks drops only census counts and the last-PC diagnostic.
    } else if(s.services.instruction) {
        if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
        const auto *policy=s.instruction_cursor;
        if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace) ||
            (s.observe && !policy->observation_only_stores))return false;
        for(const auto site:policy->service_sites)
            if((site>=0x1532a0u && site<=0x15335bu) || (site>=0x170c80u && site<=0x171000u) ||
               (site>=0x44680u && site<=0x446feu))return false;
        cursor=policy->position;
    }
    // Admission reads the scenario header without faulting: committed direct
    // pages, else the ordinary read (a refusal leaves the original route).
    const auto peek=[&](std::uint32_t address,std::uint32_t &value) {
        if(const auto *page=s.memory.direct_page(address,4)) {
            std::memcpy(&value,page->bytes+(address&4095u),4);return true;
        }
        try {value=physics_step_detail::r(s.memory,address,4);return true;}
        catch(const std::exception &) {return false;}
    };
    std::uint32_t bsp{},clusters{},portals{};
    if(!peek(0x4ecc18u,bsp) || !peek(bsp+0x9cu,clusters) || !peek(bsp+0x5cu,portals))return false;
    const auto words=std::int64_t(std::int32_t(clusters))+31,visit=std::int64_t(std::int32_t(portals))+31;
    if(words<0 || words>=17*32 || visit<0 || visit>=17*32)return false;
    if(verify_mode())verify(s,reference);
    const auto entry=s.gp[4];
    LiveIo<State> io{s,cursor};
    Closure<LiveIo<State>> closure{io};
    const auto r=closure.run(entry,entry_registers(s));
    coverage().fetch_or(closure.hit,std::memory_order_relaxed);
    publish(s,r,entry);
    if(cursor)*cursor=0x1532f2u;
    return true;
}
} // namespace halo2::native_pvs_flood_detail
