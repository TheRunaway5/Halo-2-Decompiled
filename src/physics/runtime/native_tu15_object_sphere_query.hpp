#pragma once
// Native TU1.5 object sphere query closure, entered at BD080:
//   BD080  objects of a type mask whose bounding sphere meets a query sphere:
//          flood the query location's cluster through 14D0E0 (radius > 0)
//          or use the location cluster alone, then walk every cluster's
//          object reference list(s), dedupe by the query stamp [4EABCC] in
//          [4EABD0+i*4] and keep (center-object)^2 sum < (radius+object)^2
//   BA970  first reference of a cluster in the first table ([4EABB0], pool [4EABB4])
//   BAA40  the same over the second table ([4EABA4], pool [4EABA8])
// Arguments (RET 0x1C): [E+4] list flags (bit0 first table, bit1 second;
// 0 means all), [E+8] type mask (0 means all), [E+0xC] location (cluster
// word at +4), [E+0x10] center, [E+0x14] radius, [E+0x18] output, [E+0x1C]
// output capacity (word); AX returns the count.
// Ordinary native control flow and arithmetic. The guest bank keeps its
// original ABI at this boundary: every original memory access happens in
// program order, at the same address and width (pushes, spills, the
// 0x400-byte cluster frame, return addresses), and every store goes through
// the unit's store helper, so guards and observers see the identical store
// stream. SSE arithmetic is the original scalar sequence (same operand
// order, so NaN payloads, rounding and MXCSR sticky flags are identical);
// XMM0-7, general registers and the final ADD ESP flags are published at
// exit. 14D0E0 stays a genuine child: it is entered through the unit's own
// original route (direct body or dispatch/s.required) with the exact
// register, flag, XMM and stack ABI, and its results are read back.
// The instruction cursor is published before each access and set to the
// original final instruction at exit. No translated BD080/BA970/BAA40
// instruction runs.
// HALO2_REFERENCE_OBJECT_SPHERE_QUERY=1 restores the translated bodies.
// HALO2_VERIFY_OBJECT_SPHERE_QUERY=1 runs, for every admitted call, the
// native closure and the unmodified translated body on separate copy-on-write
// overlays of the live bank (each with its own genuine 14D0E0 child), requires
// identical ordered stores (pc, address, preimage, bytes), registers, flags,
// XMM and x87 state, then runs the native closure on the live bank. A summary
// (calls, stores, branch coverage) is printed at exit.
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
#include <type_traits>
#include <vector>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2::native_object_sphere_query_detail {
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_SPHERE_QUERY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_SPHERE_QUERY");return p && *p;}();
    return value;
}
// HALO2_CENSUS_NATIVE_BRIDGES=1 (diagnostic census runs only) admits the
// closure under full instruction callbacks without a literal cursor, so a
// census counts only what still runs translated (14D0E0 counts itself).
inline bool census_admission() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_CENSUS_NATIVE_BRIDGES");return p && *p;}();
    return value;
}
inline bool &bypass(){thread_local bool value=false;return value;}

// Branch outcomes reached by the native closure (verification coverage).
// The walk outcomes come in first-table/second-table pairs.
enum Branch : unsigned {
    types_default, types_given, lists_default, lists_given, no_cluster, radius_single, radius_flood,
    flood_clamped, flood_kept, no_clusters, cluster_repeat, cluster_end,
    first_skipped, second_skipped, list_none, list_none_2, list_last, list_last_2, list_more, list_more_2,
    type_reject, type_reject_2, seen_reject, seen_reject_2, sphere_reject, sphere_reject_2,
    sphere_accept, sphere_accept_2, output_full, output_full_2, walk_next, walk_next_2,
    walk_end, walk_end_2, next_more, next_more_2, next_last, next_last_2, exit_count,
    branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}

inline std::uint32_t sx16(std::uint32_t x){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(x))));}
inline std::int16_t s16(std::uint32_t x){return std::int16_t(std::uint16_t(x));}
// MOVSS xmm,m32: the exact bits in lane 0, upper lanes zero.
inline __m128 load_ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// The original scalar instructions with explicit destinations: the first
// operand owns the NaN payload, and the memory clobber keeps program order
// relative to the bank accesses.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline __m128 sub_ss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 mul_ss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 add_ss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
// COMISS a,b; JB: below or unordered.
inline bool comiss_below(__m128 a,__m128 b) {
    unsigned char below;asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(below):"x"(a),"x"(b):"cc","memory");return below;
}
inline constexpr bool backend=true;
#else
inline __m128 sub_ss(__m128 a,__m128 b){return _mm_sub_ss(a,b);}
inline __m128 mul_ss(__m128 a,__m128 b){return _mm_mul_ss(a,b);}
inline __m128 add_ss(__m128 a,__m128 b){return _mm_add_ss(a,b);}
inline bool comiss_below(__m128 a,__m128 b){return _mm_comilt_ss(a,b) || _mm_comieq_ss(a,a)==0 || _mm_comieq_ss(b,b)==0;}
inline constexpr bool backend=false;
#endif

template<class State,class Flood> struct Closure {
    State &s;Flood &flood;std::uint32_t *cursor{};
    std::uint64_t hit{};
    std::uint32_t eax{},ecx{},edx{},ebx{},ebp{},esi{},edi{};
    std::uint32_t E{},F{},final_pc{};
    __m128 x[8];
    void mark(Branch b){hit|=std::uint64_t(1)<<b;}
    void mark(Branch b,bool second){mark(Branch(b+unsigned(second)));}
    std::uint32_t rd(std::uint32_t pc,std::uint32_t a,unsigned n=4){if(cursor)*cursor=pc;return physics_step_detail::r(s.memory,a,n);}
    void st(std::uint32_t pc,std::uint32_t a,unsigned n,std::uint32_t v){if(cursor)*cursor=pc;physics_step_detail::w(s.memory,s.observe,pc,a,n,v);}

    void load() {
        eax=s.gp[0];ecx=s.gp[1];edx=s.gp[2];ebx=s.gp[3];ebp=s.gp[5];esi=s.gp[6];edi=s.gp[7];
        E=s.gp[4];F=E-0x430u;
        for(unsigned i=0;i<8;++i)x[i]=s.xmm[i];
    }
    void publish() {
        s.gp={eax,ecx,edx,ebx,E+0x20u,ebp,esi,edi};
        s.flags.plus(E-0x420u,0x420u,4);
        for(unsigned i=0;i<8;++i)s.xmm[i]=x[i];
        if(cursor)*cursor=final_pc;
    }

    // 14D0E0 with the exact caller ABI: ESP at the pushed return address,
    // the INC ECX flags (CF from the radius COMISS) and the live XMM file.
    template<class Flags> void child(const Flags &flags) {
        s.gp={eax,ecx,edx,ebx,F-0x18u,ebp,esi,edi};
        s.flags=flags;
        for(unsigned i=0;i<8;++i)s.xmm[i]=x[i];
        flood(s);
        if(s.gp[4]!=F)throw std::runtime_error("Native BD080 child 14D0E0 returned an unbalanced stack");
        eax=s.gp[0];ecx=s.gp[1];edx=s.gp[2];ebx=s.gp[3];ebp=s.gp[5];esi=s.gp[6];edi=s.gp[7];
        for(unsigned i=0;i<8;++i)x[i]=s.xmm[i];
    }

    // BA970 (second: BAA40, every PC +0xD0). Entered at ESP=F-0xC with the
    // cluster word at [F-8], the payload slot pointer at [F-4] and EBX the
    // iterator slot; RET 8.
    void begin(bool second) {
        const std::uint32_t o=second?0xd0u:0u,table=second?0x4eaba4u:0x4eabb0u;
        eax=sx16(rd(0xba970u+o,F-8u,2));
        ecx=rd(0xba975u+o,table);
        eax=rd(0xba97bu+o,ecx+eax*4u);
        st(0xba981u+o,ebx,4,eax);
        if(eax==0xffffffffu) {
            mark(list_none,second);
            rd(0xba9d5u+o,F-0xcu);return;
        }
        ecx=rd(0xba985u+o,table+4u);
        edx=rd(0xba98bu+o,ecx+0x44u);
        st(0xba98eu+o,F-0x10u,4,ebp);
        ebp=rd(0xba98fu+o,F-4u);
        st(0xba993u+o,F-0x14u,4,esi);
        eax=(eax&0xffffu)*rd(0xba999u+o,ecx+0x24u);
        esi=ecx+0x24u;
        st(0xba9a0u+o,F-0x18u,4,edi);
        edi=ecx+0x44u;
        eax+=edx;
        edx=rd(0xba9a6u+o,eax+8u);
        ecx=eax+0xcu;
        st(0xba9afu+o,ebp,4,ecx);
        if(edx!=0xffffffffu) {
            mark(list_more,second);
            ecx=(edx&0xffffu)*rd(0xba9bcu+o,esi);
            esi=rd(0xba9bfu+o,edi);
            ecx+=esi;
        } else mark(list_last,second);
        edi=rd(0xba9c7u+o,F-0x18u);
        esi=rd(0xba9c8u+o,F-0x14u);
        st(0xba9c9u+o,ebx,4,edx);
        eax=rd(0xba9cbu+o,eax+4u);
        ebp=rd(0xba9ceu+o,F-0x10u);
        rd(0xba9cfu+o,F-0xcu);
    }

    // Sphere test of the reference payload EBX against the center EDI.
    bool outside(bool second) {
        if(!second) {
            x[3]=load_ss(rd(0xbd1ffu,ebx+0xcu));x[3]=sub_ss(x[3],load_ss(rd(0xbd204u,edi+8u)));
            x[2]=load_ss(rd(0xbd209u,ebx+8u));x[2]=sub_ss(x[2],load_ss(rd(0xbd20eu,edi+4u)));
            x[1]=load_ss(rd(0xbd213u,ebx+4u));x[1]=sub_ss(x[1],load_ss(rd(0xbd218u,edi)));
            x[0]=load_ss(rd(0xbd21cu,ebx+0x10u));
            x[5]=mul_ss(x[3],x[3]);
            x[3]=mul_ss(x[2],x[2]);
            x[2]=mul_ss(x[1],x[1]);
            x[0]=add_ss(x[0],x[4]);
            x[5]=add_ss(x[5],x[3]);
            x[1]=x[0];
            x[5]=add_ss(x[5],x[2]);
            x[1]=mul_ss(x[1],x[0]);
        } else {
            x[3]=load_ss(rd(0xbd34au,ebx+0xcu));x[3]=sub_ss(x[3],load_ss(rd(0xbd34fu,edi+8u)));
            x[1]=load_ss(rd(0xbd354u,ebx+4u));x[1]=sub_ss(x[1],load_ss(rd(0xbd359u,edi)));
            x[2]=load_ss(rd(0xbd35du,ebx+8u));x[2]=sub_ss(x[2],load_ss(rd(0xbd362u,edi+4u)));
            x[0]=load_ss(rd(0xbd367u,ebx+0x10u));
            x[5]=mul_ss(x[3],x[3]);
            x[3]=mul_ss(x[1],x[1]);
            x[1]=mul_ss(x[2],x[2]);
            x[0]=add_ss(x[0],x[4]);
            x[5]=add_ss(x[5],x[3]);
            x[5]=add_ss(x[5],x[1]);
            x[1]=x[0];
            x[1]=mul_ss(x[1],x[0]);
        }
        return comiss_below(x[1],x[5]);
    }

    // BD1C5 (second: BD310, PCs +0x14B outside the sphere test): walk one
    // reference list from payload EBX, object EAX, next reference EDX.
    // Returns true at a full output (the BD443 exit).
    bool walk(bool second) {
        const std::uint32_t o=second?0x14bu:0u,pool=second?0x4eaba8u:0x4eabb4u,slot=second?F+0x1cu:F+0x20u;
        for(;;) {
            ecx=(ecx&0xffffff00u)|rd(0xbd1c5u+o,ebx,1);
            esi=1u<<(ecx&31u);
            ecx=rd(0xbd1ceu+o,E+8u);
            if(!(ecx&esi))mark(type_reject,second);
            else {
                esi=rd(0xbd1ddu+o,0x4eabccu);
                ecx=eax&0xffffu;
                if(rd(0xbd1ebu+o,0x4eabd0u+ecx*4u)==esi)mark(seen_reject,second);
                else {
                    st(0xbd1f8u+o,0x4eabd0u+ecx*4u,4,esi);
                    if(outside(second))mark(sphere_reject,second);
                    else {
                        ecx=rd(0xbd24eu+o,F+0x10u);
                        if(s16(ecx)>=s16(rd(0xbd252u+o,E+0x1cu,2))){mark(output_full,second);return true;}
                        mark(sphere_accept,second);
                        ebp=rd(0xbd260u+o,E+0x18u);
                        esi=sx16(ecx);
                        ++ecx;
                        st(0xbd26bu+o,ebp+esi*4u,4,eax);
                        ebp=rd(0xbd26fu+o,pool);
                        st(0xbd275u+o,F+0x10u,4,ecx);
                    }
                }
            }
            if(edx==0xffffffffu){mark(walk_end,second);ecx=0xffffffffu;}
            else {
                mark(walk_next,second);
                esi=rd(0xbd27eu+o,ebp+0x24u);
                edi=rd(0xbd281u+o,ebp+0x44u);
                edx=(edx&0xffffu)*esi+edi;
                ecx=edx;
                edx=rd(0xbd291u+o,ecx+8u);
                ebx=ecx+0xcu;
                // The prefetch address (EAX) is overwritten below at BD2C2.
                if(edx!=0xffffffffu){mark(next_more,second);eax=(edx&0xffffu)*esi+edi;}
                else mark(next_last,second);
                ecx=rd(0xbd2acu+o,ecx+4u);
                edi=rd(0xbd2afu+o,E+0x10u);
                st(0xbd2b6u+o,slot,4,edx);
            }
            eax=ecx;
            if(ecx==0xffffffffu)return false;
        }
    }

    void exit_count() {
        mark(Branch::exit_count);
        eax=(eax&0xffff0000u)|rd(0xbd42bu,F+0x10u,2);
        edi=rd(0xbd430u,F);esi=rd(0xbd431u,F+4u);ebp=rd(0xbd432u,F+8u);
        st(0xbd433u,0x4eabc8u,1,ebx&0xffu);
        ebx=rd(0xbd439u,F+12u);
        rd(0xbd440u,E);final_pc=0xbd440u;
    }
    void exit_full() {
        edi=rd(0xbd443u,F);esi=rd(0xbd444u,F+4u);ebp=rd(0xbd445u,F+8u);
        st(0xbd446u,0x4eabc8u,1,0);
        eax=(eax&0xffff0000u)|(ecx&0xffffu);
        ebx=rd(0xbd450u,F+12u);
        rd(0xbd457u,E);final_pc=0xbd457u;
    }

    void run() {
        eax=rd(0xbd086u,E+8u);
        st(0xbd08du,E-0x424u,4,ebx);
        st(0xbd08eu,E-0x428u,4,ebp);
        ebx=0;
        const bool all_types=!eax;
        st(0xbd093u,E-0x42cu,4,esi);
        st(0xbd094u,E-0x430u,4,edi);
        st(0xbd095u,F+0x10u,4,0);
        if(all_types){mark(types_default);st(0xbd09bu,E+8u,4,0xffffffffu);} else mark(types_given);
        if(!rd(0xbd0a6u,E+4u)){mark(lists_default);st(0xbd0afu,E+4u,4,0xffffffffu);} else mark(lists_given);
        eax=rd(0xbd0bau,E+0xcu);
        eax=(eax&0xffff0000u)|rd(0xbd0c1u,eax+4u,2);
        edi=rd(0xbd0c9u,E+0x10u);
        st(0xbd0d0u,F+0x14u,4,0);
        bool reload=true;
        if((eax&0xffffu)==0xffffu)mark(no_cluster);
        else {
            x[4]=load_ss(rd(0xbd0d6u,E+0x14u));
            auto flags=s.flags;
            flags.scalar_compare(x[4],load_ss(rd(0xbd0dfu,0x46a0e8u)),false);
            if(flags.cf || flags.zf) {
                mark(radius_single);
                st(0xbd16fu,F+0x14u,4,1);
                st(0xbd177u,F+0x30u,2,eax&0xffffu);
                reload=false;
            } else {
                mark(radius_flood);
                ecx=rd(0xbd0ecu,0x4f3ce4u);
                edx=rd(0xbd0f2u,E+0x14u);
                ecx=flags.increment(ecx,4,false);
                st(0xbd0fau,0x4f3ce4u,4,ecx);
                ecx=F+0x30u;
                st(0xbd104u,F-4u,4,ecx);
                st(0xbd105u,F-8u,4,0x200u);
                st(0xbd10au,F-0xcu,4,edx);
                st(0xbd10bu,F-0x10u,4,edi);
                st(0xbd10cu,F-0x14u,4,eax);
                st(0xbd10du,0x4f3ce1u,1,1);
                st(0xbd114u,F-0x18u,4,0xbd119u);
                child(flags);
                st(0xbd11du,0x4f3ce1u,1,ebx&0xffu);
                st(0xbd123u,F+0x14u,4,eax);
                if(s16(eax)>0x200){mark(flood_clamped);st(0xbd129u,F+0x14u,4,0x200u);} else mark(flood_kept);
            }
        }
        if(reload)x[4]=load_ss(rd(0xbd131u,E+0x14u));
        // BD13A: new query stamp, then the cluster loop.
        edx=rd(0xbd13au,0x4eabccu)+1u;
        eax=0;
        const auto clusters=rd(0xbd143u,F+0x14u,2);
        st(0xbd148u,0x4eabccu,4,edx);
        st(0xbd14eu,0x4eabc8u,1,1);
        st(0xbd155u,F+0x24u,4,eax);
        if(s16(clusters)<=s16(ebx)){mark(no_clusters);exit_count();return;}
        ecx=rd(0xbd15fu,E+4u)&1u;
        st(0xbd169u,F+0x28u,4,ecx);
        for(bool first=true;;first=false) {
            if(!first)ecx=rd(0xbd17eu,F+0x28u);
            eax=sx16(eax);
            const bool skip=ecx==ebx;
            esi=rd(0xbd189u,F+0x30u+eax*2u,2);
            st(0xbd18eu,F+0x2cu,4,esi);
            if(skip)mark(first_skipped);
            else {
                ecx=F+0x18u;edx=sx16(esi);
                st(0xbd19fu,F-4u,4,ecx);
                st(0xbd1a0u,F+0x18u,4,ebx);
                st(0xbd1a4u,F-8u,4,edx);
                ebx=F+0x20u;
                st(0xbd1a9u,F-0xcu,4,0xbd1aeu);
                begin(false);
                if(eax!=0xffffffffu) {
                    ebx=rd(0xbd1b7u,F+0x18u);edx=rd(0xbd1bbu,F+0x20u);ebp=rd(0xbd1bfu,0x4eabb4u);
                    if(walk(false)){exit_full();return;}
                    esi=rd(0xbd2cau,F+0x2cu);
                }
                ebx=0;
            }
            if(!(rd(0xbd2d0u,E+4u,1)&2u))mark(second_skipped);
            else {
                edx=F+0x18u;eax=sx16(esi);
                st(0xbd2e5u,F-4u,4,edx);
                st(0xbd2e6u,F+0x18u,4,ebx);
                st(0xbd2eau,F-8u,4,eax);
                ebx=F+0x1cu;
                st(0xbd2efu,F-0xcu,4,0xbd2f4u);
                begin(true);
                if(eax!=0xffffffffu) {
                    ebx=rd(0xbd2fdu,F+0x18u);edx=rd(0xbd301u,F+0x1cu);ebp=rd(0xbd305u,0x4eaba8u);
                    if(walk(true)){exit_full();return;}
                }
                ebx=0;
            }
            eax=rd(0xbd417u,F+0x24u)+1u;
            const auto limit=rd(0xbd41cu,F+0x14u,2);
            st(0xbd421u,F+0x24u,4,eax);
            if(!(s16(eax)<s16(limit))){mark(cluster_end);break;}
            mark(cluster_repeat);
        }
        exit_count();
    }
};

struct Store {
    std::uint32_t pc,address;std::vector<std::byte> before,written;
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
struct VerifySummary {
    std::atomic<std::uint64_t> calls{},stores{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object sphere query verify: %llu calls, %llu ordered stores identical, coverage %llx\n",
            (unsigned long long)calls.load(),(unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline bool same_bits(__m128 a,__m128 b){return !std::memcmp(&a,&b,16);}
template<class State> void copy_registers(State &to,const State &from) {
    to.gp=from.gp;to.flags=from.flags;to.other_flags=from.other_flags;to.fs_base=from.fs_base;to.fp=from.fp;
    for(unsigned i=0;i<8;++i)to.xmm[i]=from.xmm[i];
}
template<class State> bool same_state(const State &a,const State &b) {
    bool same=a.gp==b.gp && a.other_flags==b.other_flags && a.flags.cf==b.flags.cf && a.flags.zf==b.flags.zf &&
        a.flags.sf==b.flags.sf && a.flags.of==b.flags.of && a.flags.pf==b.flags.pf;
    if constexpr(requires{a.flags.af;})same=same && a.flags.af==b.flags.af;
    for(unsigned i=0;i<8;++i)same=same && same_bits(a.xmm[i],b.xmm[i]);
    same=same && a.fp.count==b.fp.count && a.fp.status==b.fp.status;
    if constexpr(requires{a.fp.top;})same=same && a.fp.top==b.fp.top;
    for(unsigned i=0;i<a.fp.count && i<8;++i)same=same && !std::memcmp(&a.fp.lanes[i],&b.fp.lanes[i],10);
    return same;
}

template<class State,class Reference,class Flood> void verify(State &s,Reference reference,Flood &flood) {
    // Children keep their own native admission without a cursor; the
    // instruction callback is the only service withheld from the overlays.
    auto services=s.services;services.instruction=nullptr;
    const auto record=[](std::vector<Store> &into){
        return SourcePhysicsContactRecordObserver([&into](const SourcePhysicsContactRecordStore &e){into.push_back({e.instruction,e.address,e.before,e.written});});
    };
    std::vector<Store> native_stores,original_stores;
    Overlay native_bank{s.memory},original_bank{s.memory};
    const auto native_observer=record(native_stores),original_observer=record(original_stores);
    // A fault must be the same exception after the same store prefix; the
    // live run then reproduces it on the bank.
    std::string native_fault,original_fault;
    State native{native_bank,native_observer,services,s.image};copy_registers(native,s);
    Closure<State,Flood> closure{native,flood,nullptr};
    try {closure.load();closure.run();closure.publish();}
    catch(const std::exception &e){native_fault=e.what();}
    State original{original_bank,original_observer,services,s.image};copy_registers(original,s);
    try {struct Hold{Hold(){bypass()=true;}~Hold(){bypass()=false;}} hold;reference(original);}
    catch(const std::exception &e){original_fault=e.what();}
    if(native_stores!=original_stores || native_fault!=original_fault ||
       (native_fault.empty() && !same_state(native,original))) {
        std::size_t at=0;while(at<native_stores.size() && at<original_stores.size() && native_stores[at]==original_stores[at])++at;
        std::string detail="Native object sphere query differs from BD080 at store "+std::to_string(at)+" of "+
            std::to_string(native_stores.size())+"/"+std::to_string(original_stores.size());
        if(at<native_stores.size() && at<original_stores.size()) {
            char text[160];const auto &a=native_stores[at],&b=original_stores[at];
            std::snprintf(text,sizeof text," native pc=%x a=%x original pc=%x a=%x",a.pc,a.address,b.pc,b.address);detail+=text;
        } else if(native_fault!=original_fault)detail+=" (fault '"+native_fault+"' vs '"+original_fault+"')";
        else if(native_stores==original_stores)detail+=" (registers/flags/FP)";
        throw std::runtime_error(detail);
    }
    coverage().fetch_or(closure.hit);
    auto &total=summary();total.calls.fetch_add(1);total.stores.fetch_add(original_stores.size());
}

inline bool in_closure(std::uint32_t site) {
    return (site>=0xbd080u && site<=0xbd457u) || (site>=0xba970u && site<=0xba9d5u) || (site>=0xbaa40u && site<=0xbaaa5u);
}
// Admission keeps every case outside the authenticated domain translated:
// another image, an active MMX register file, unmasked SSE exceptions,
// active traces, arbitrary store observers and service sites in the closure.
template<class State,class Reference,class Flood> bool bridge(State &s,Reference reference,Flood flood) {
    if(!backend || reference_mode() || bypass() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
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
        for(const auto site:policy->service_sites)if(in_closure(site))return false;
        cursor=policy->position;
    }
    if(verify_mode())verify(s,reference,flood);
    Closure<State,Flood> closure{s,flood,cursor};
    closure.load();closure.run();closure.publish();
    coverage().fetch_or(closure.hit,std::memory_order_relaxed);
    return true;
}
} // namespace halo2::native_object_sphere_query_detail
