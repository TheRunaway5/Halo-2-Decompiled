#pragma once
// Boundary adapter for the cluster pairs, 118120
// (native_tu15_cluster_pairs_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: three arguments on the stack (RET C),
// the answer in AL; ECX pushed as the local for the references' end; ESI,
// EDI, EBX and EBP saved as the original saves them. Each access publishes its
// original instruction.
// HALO2_REFERENCE_CLUSTER_PAIRS=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_PAIRS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/native_tu15_cluster_pairs_systems.hpp"

namespace halo2::native_cluster_pairs_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_cluster_pairs;
using Access=logic::cluster_pairs::Access;
using Outcome=logic::cluster_pairs::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CLUSTER_PAIRS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CLUSTER_PAIRS");return v;}
enum Branch : unsigned {no_references, unseen, seen, empty_range, branch_count};
static_assert(unsigned(Outcome::empty_range)==Branch::empty_range);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Cluster pairs"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x118121u,0x118125u,0x11812bu,0x118137u,0x11813bu,0x11814eu,0x11815du,0x118163u,0x118169u,
    0x11816fu,0x118172u,0x118177u,0x11817au,0x118180u,0x118187u,0x118191u,0x1181a3u,0x1181a8u,0x1181bbu,0x1181c0u,0x1181d3u,0x1181d8u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::seen_store)+1u);
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t local()const{return this->gp[esp]+depth-4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    // A cluster's bit: MOV ECX,EAX; AND ECX,1F; MOV reg,1; SHL reg,CL; SAR EAX,5.
    void bit_of(std::uint32_t cluster,unsigned reg) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=cluster;gp[ecx]=fl.logic(cluster&0x1fu,4);gp[reg]=fl.shift(1u,gp[ecx]&0xffu,4);gp[eax]=fl.right(cluster,5u,4,true);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case object_argument:case object_address:case pairs:case pairs_load:gp[ecx]=v;break;
        case headers:gp[edx]=v;break;
        case datums:gp[edx]=v;gp[ecx]=3u*fl.logic(gp[ecx]&0xffffu,4);break;                          // AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        case references:                                                                              // MOV DX; XOR AL,AL; TEST DX,DX
            set_low_word(gp[edx],v);set_low_byte(gp[eax],fl.logic(0u,1));fl.logic(v&0xffffu,2);break;
        case first_reference:                                                                         // MOVSX EAX,DX; ADD EAX,ESI; CMP ESI,EAX
            gp[esi]=logic::widen16(v);gp[eax]=fl.plus(logic::widen16(gp[edx]&0xffffu),gp[esi],4);fl.minus(gp[esi],gp[eax],4);break;
        case structure_address:gp[edi]=v;break;
        case indices_block:gp[eax]=v;break;
        case bitmap_argument:gp[edi]=v;put(0x118176u,gp[ebx]);break;                                 // PUSH EBX
        case index_list_address:gp[ebx]=v;break;
        case reference_index:gp[edx]=gp[esi]&0xffffu;gp[eax]=logic::widen16(v);break;               // MOVZX EDX,SI
        case first_cluster:bit_of(logic::widen16(v),ebp);break;
        case first_bit:fl.logic(v&gp[ebp],4);break;
        case second_cluster:bit_of(logic::widen16(v),edx);break;
        case second_bit:fl.logic(v&gp[edx],4);break;
        case end_load:gp[eax]=v;gp[esi]=fl.increment(gp[esi],4,false);fl.minus(gp[esi],v,4);break;   // INC ESI; CMP ESI,EAX
        case seen_argument:gp[eax]=v;break;
        default:break;
        }
        if(a==reference_index)gp[edx]=gp[ecx]+4u*9u*gp[eax];                                          // LEA EAX,[EAX+EAX*8]; LEA EDX,[ECX+EAX*4]
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving_esi(){put(0x11814du,this->gp[esi]);}
    void saving_edi(){put(0x11815cu,this->gp[edi]);}
    void pairing(){put(0x11817eu,this->gp[ebp]);}
    void restoring(bool seen_path){if(seen_path)this->gp[ebp]=take(0x1181d7u);}
    void run() {
        auto &gp=this->gp;
        put(0x118120u,gp[ecx]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::cluster_pairs::test(memory,*this);
        this->reached(unsigned(outcome));
        switch(outcome) {
        case Outcome::unseen:
            gp[ebp]=take(0x1181c9u);gp[ebx]=take(0x1181cau);gp[edi]=take(0x1181cbu);set_low_byte(gp[eax],1u);gp[esi]=take(0x1181ceu);
            gp[ecx]=take(0x1181cfu);this->ret(0x1181d0u,0xcu);this->publish(0x1181d0u);return;
        case Outcome::seen:gp[ebx]=take(0x1181dbu);[[fallthrough]];
        case Outcome::empty_range:gp[edi]=take(0x1181dcu);set_low_byte(gp[eax],1u);gp[esi]=take(0x1181dfu);break;
        default:break;
        }
        gp[ecx]=take(0x1181e0u);this->ret(0x1181e1u,0xcu);this->publish(0x1181e1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x118120u,0x1181e1u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_cluster_pairs_detail
