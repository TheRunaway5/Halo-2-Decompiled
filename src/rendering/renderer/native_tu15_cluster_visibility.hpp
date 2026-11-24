#pragma once
// Boundary adapter for the cluster visibility, 13B610
// (native_tu15_cluster_visibility_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: ESI the set, the view and a
// second argument pushed (RET 8); EBX and EDI saved; the set cleared as REP
// STOSD clears it; 1532A0 (the set and view pushed, RET 8) and 140EC0 (EAX
// the row, EDX the set; the set pushed, RET 4) direct and 16FD40 through the
// unit's dispatch, every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_CLUSTER_VISIBILITY=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_VISIBILITY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_cluster_visibility_systems.hpp"

namespace halo2::native_cluster_visibility_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_cluster_visibility;
using Access=logic::cluster_visibility::Access;
using Kept=logic::cluster_visibility::Kept;
using Outcome=logic::cluster_visibility::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CLUSTER_VISIBILITY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CLUSTER_VISIBILITY");return v;}
enum Branch : unsigned {flooded, seated, unseated, second, backward, branch_count};
static_assert(unsigned(Outcome::unseated)==Branch::unseated);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Cluster visibility"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x13b611u,0x13b621u,0x13b623u,0x13b629u,0x13b62fu,0x13b63cu,0x13b650u,0x13b669u,0x13b681u,0x13b68bu,
    0x13b69du,0x13b6abu,0x13b6b5u,0x13b6bbu,0x13b6cbu,0x13b6ddu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::bits_again)+1u);
template<class State,class Flood,class Union,class Dispatch> struct Adapter : Port<State> {
    Flood &flood_child;Union &union_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Flood &f,Union &u,Dispatch &d):Port<State>(s,c,t,{}),flood_child(f),union_child(u),dispatch(d){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case bsp:gp[ebx]=v;break;
        case game:gp[edi]=v;break;
        case view:case bits:case bits_again:gp[eax]=v;break;
        case seats:case count:case count_again:case second_global:gp[ecx]=v;break;
        case cluster:set_low_word(gp[eax],v);break;
        case second_flag:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::clear){auto &gp=this->gp;gp[edi]+=4u*step();--gp[ecx];}                          // each STOSD step
    }
    void saving(unsigned n){if(n==0u)put(0x13b610u,this->gp[ebx]);else put(0x13b617u,this->gp[edi]);}   // PUSH EBX; PUSH EDI
    std::uint32_t clearing(std::uint32_t set) {                                                         // XOR EAX,EAX; MOV ECX,10; MOV EDI,ESI
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::set_words;gp[edi]=set;
        if(step()!=1u)this->reached(Branch::backward);
        return set;
    }
    void kind_tested(std::uint32_t kind){this->compare32(kind,logic::seated_kind);}                    // CMP [EDI+8],3
    void flood(std::uint32_t view_value,Kept &kept) {                                                    // PUSH EAX; PUSH ESI; CALL 1532A0
        put(0x13b633u,view_value);put(0x13b634u,kept.set);
        this->call(0x13b635u,0x13b63au,0x1532a0u,flood_child);depth-=8u;
        refresh(kept);
    }
    void seat_walk_start(std::uint32_t slot) {                                                          // OR EDX,-1; XOR EAX,EAX; ADD ECX,C
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.logic(logic::none,4);gp[eax]=fl.logic(0u,4);gp[ecx]=fl.plus(slot-logic::seats::players,logic::seats::players,4);
    }
    void seat_tested(std::uint32_t player){this->compare32(player,logic::none);}                       // CMP [ECX],-1
    void seat_found(std::uint32_t k){this->gp[edx]=k;}                                                  // MOV EDX,EAX
    void seat_next(std::uint32_t k,std::uint32_t slot) {                                                // INC EAX; ADD ECX,4; CMP EAX,4
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.increment(k-1u,4,false);gp[ecx]=fl.plus(slot-4u,4u,4);this->compare32(k,logic::seat_count);
    }
    void seat_chosen(std::uint32_t index){this->gp[eax]=this->flags.logic(0u,4);this->compare32(index,logic::none);}   // XOR EAX,EAX; CMP EDX,-1
    void used_tested(std::uint32_t used){this->flags.minus(used&0xffffu,0xffffu,2);}                   // CMP word [474C24],-1
    std::uint32_t player(std::uint32_t index) {                                                         // IMUL EDX,358; ADD EDX,4F655C; MOV EAX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.plus(fl.product(index,logic::player_size),logic::globals::players,4);
        return gp[eax]=gp[edx];
    }
    void cluster_tested(std::uint32_t cluster_value){this->flags.minus(cluster_value&0xffffu,0xffffu,2);}   // CMP AX,FFFF
    std::uint32_t row_words(std::uint32_t cluster_value,std::uint32_t count_value) {                    // MOVSX EAX,AX; LEA EDX,[ECX+1F]; SAR EDX,5; IMUL EDX,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=cluster_value;gp[edx]=fl.product(fl.right(count_value+0x1fu,5u,4,true),cluster_value);
        return gp[edx];
    }
    void row(unsigned n,std::uint32_t address,Kept &kept) {                                             // LEA EAX; PUSH ESI; MOV EDX,ESI; CALL 140EC0
        auto &gp=this->gp;
        gp[eax]=address;put(n?0x13b6e3u:0x13b6a3u,kept.set);gp[edx]=kept.set;
        const auto site=n?0x13b6e6u:0x13b6a6u;
        this->call(site,site+5u,0x140ec0u,union_child);depth-=4u;
        refresh(kept);
    }
    void tested8(std::uint32_t v){this->test8(v);}
    void byte_compared(std::uint32_t v){this->flags.minus(v&0xffu,0u,1);}                              // CMP byte [ECX],0
    std::uint32_t second_cluster(Kept &kept) {                                                          // CALL 16FD40
        auto child=[&](State &v){dispatch(v,0x16fd40u,0x13b6c0u);};
        this->call(0x13b6c0u,0x13b6c5u,0x16fd40u,child);
        refresh(kept);
        this->reached(Branch::second);
        return this->gp[eax];
    }
    void restoring(){auto &gp=this->gp;gp[edi]=take(0x13b6ebu);gp[ebx]=take(0x13b6ecu);}             // POP EDI; POP EBX
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::cluster_visibility::gather(memory,*this,Kept{gp[ebx],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(0x13b6edu,8u);this->publish(0x13b6edu);
    }
};
template<class State,class Reference,class Flood,class Union,class Dispatch>
bool bridge(State &s,Reference reference,Flood flood,Union union_bits,Dispatch dispatch) {
    return run_owner(s,0x13b610u,0x13b6edu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Flood,Union,Dispatch>(v,c,t,flood,union_bits,dispatch);});
}
} // namespace halo2::native_cluster_visibility_detail
