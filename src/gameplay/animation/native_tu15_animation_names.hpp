#pragma once
// Boundary adapter for the animation names, 1D45A0
// (native_tu15_animation_names_systems.hpp). Its callers and 1E5810 keep the
// original ABI, which the adapter owns: EAX, ECX and EDX the first three
// names, ESI the output, the record and the fourth name pushed (RET 8); EBX
// and EBP saved (EDI around the lookup); 1E5810 (the names, the third and 8
// pushed, RET 0xC) direct, every register it returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_NAMES=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_NAMES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_animation_names_systems.hpp"

namespace halo2::native_animation_names_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_animation_names;
using Access=logic::animation_names::Access;
using Kept=logic::animation_names::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ANIMATION_NAMES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ANIMATION_NAMES");return v;}
enum Branch : unsigned {resolved, found, replaced, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Animation names"};return v;}
inline constexpr std::uint32_t sites[]{0x1d45a6u,0x1d45abu,0u,0x1d45bcu,0x1d45c2u,0x1d45c6u,0x1d45ceu,0x1d45dcu,0x1d45dfu,0x1d45e2u,
    0x1d45f1u,0x1d4600u,0x1d4603u,0x1d4606u,0x1d4615u,0x1d461cu,0x1d461fu,0x1d4622u,0x1d4629u,0x1d4637u,0x1d4648u,0x1d4667u,0x1d4672u,
    0x1d4678u,0x1d467du,0x1d4680u,0x1d468bu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::fourth_default)+1u);
inline constexpr std::uint32_t given_sites[4]{0x1d45afu,0x1d45b1u,0x1d45b4u,0x1d45b7u};
template<class State,class Lookup> struct Adapter : Port<State> {
    Lookup &lookup_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned givens{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Lookup &l):Port<State>(s,c,t,{}),lookup_child(l){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::given?given_sites[givens++]:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case record_argument:gp[ebx]=v;break;
        case fourth_argument:gp[ebp]=v;break;
        case record_first:case record_second:case out_second:case out_third:case record_tag:case found_name:case record_fourth:gp[eax]=v;break;
        case record_third:case instances:gp[ecx]=v;break;
        case third_now:case definition:gp[edx]=v;break;
        case second_now:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void saving(unsigned n) {
        auto &gp=this->gp;
        static constexpr std::uint32_t at[3]{0x1d45a5u,0x1d45aau,0x1d4628u};
        static constexpr unsigned saved[3]{ebx,ebp,edi};
        put(at[n],gp[saved[n]]);
    }
    void pushing(std::uint32_t third,std::uint32_t tag) {                                              // PUSH 8; AND EAX,FFFF; PUSH EDX; SHL EAX,4
        auto &gp=this->gp;auto &fl=this->flags;
        put(0x1d462cu,logic::lookup_kind);
        const auto index=fl.logic(tag&0xffffu,4);
        put(0x1d4633u,third);
        gp[eax]=fl.shift(index,4u,4);
    }
    std::uint32_t lookup(std::uint32_t definition_value,std::uint32_t offset,Kept &kept) {             // ADD EDX,54; PUSH EDX; CALL 1E5810
        auto &gp=this->gp;
        gp[edx]=this->flags.plus(definition_value,offset,4);
        put(0x1d463eu,gp[edx]);
        this->call(0x1d463fu,0x1d4644u,0x1e5810u,lookup_child);depth-=12u;
        refresh(kept);
        this->test32(gp[eax]);
        if(gp[eax])this->reached(Branch::found);
        return gp[eax];
    }
    void second_replaced(const Kept &kept){this->gp[edi]=kept.second;this->reached(Branch::replaced);}   // MOV EDI,EAX
    void settled(const Kept &){auto &gp=this->gp;gp[ecx]=logic::names::fourth_default;gp[edi]=take(0x1d466fu);}   // MOV ECX,400000C; POP EDI
    void restoring(unsigned){auto &gp=this->gp;gp[ebp]=take(0x1d4687u);gp[ebx]=take(0x1d4688u);}       // POP EBP; POP EBX
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::animation_names::resolve(memory,*this,gp[eax],gp[ecx],gp[edx],Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(Branch::resolved);
        this->ret(0x1d468eu,8u);this->publish(0x1d468eu);
    }
};
template<class State,class Reference,class Lookup> bool bridge(State &s,Reference reference,Lookup lookup) {
    return run_owner(s,0x1d45a0u,0x1d468eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Lookup>(v,c,t,lookup);});
}
} // namespace halo2::native_animation_names_detail
