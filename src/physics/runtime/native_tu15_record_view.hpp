#pragma once
// Boundary adapter for the record view, entered at 304C50
// (native_tu15_record_view_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: four arguments (cdecl); a 0x10-byte
// frame; 2F4990 gets the target, the view, the third and the fourth pushed
// (cdecl), every register it returns coming back; the flags of the closing
// ADD ESP,20. Each access publishes its original instruction.
// HALO2_REFERENCE_RECORD_VIEW=1 restores the translated body.
// HALO2_VERIFY_RECORD_VIEW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_record_view_systems.hpp"

namespace halo2::native_record_view_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_record_view;
using Access=logic::record_view::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RECORD_VIEW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RECORD_VIEW");return v;}
enum Branch : unsigned {forwarded, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Record view"};return v;}
template<class State,class Forward> struct Adapter : Port<State> {
    Forward &forward_child;
    std::uint32_t pushed{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Forward &f):Port<State>(s,c,t,{}),forward_child(f){}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);pushed+=4u;}
    std::uint32_t local(std::uint32_t k)const{return this->gp[esp]+pushed+4u*k;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+pushed+0x10u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t sites[]{0x304c53u,0x304c57u,0x304c5au,0x304c5cu,0x304c60u,0x304c64u,0x304c67u,0x304c6au,
            0x304c6eu,0x304c71u,0x304c76u,0x304c7au};
        if(a==Access::target)put(0x304c75u,this->gp[ecx]);                                         // PUSH ECX (the fourth)
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case record:case second_word:gp[eax]=v;break;
        case third_word:case source_word:case fourth_argument:case target:gp[ecx]=v;break;
        case source:case third_argument:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EDX; LEA EAX,[ESP+8]; PUSH EAX; PUSH ECX; CALL 2F4990.
    void forward(std::uint32_t target_value,std::uint32_t,std::uint32_t third_value,std::uint32_t) {
        auto &gp=this->gp;
        put(0x304c7eu,third_value);gp[eax]=gp[esp]+8u;put(0x304c83u,gp[eax]);put(0x304c84u,target_value);
        this->call(0x304c85u,0x304c8au,0x2f4990u,forward_child);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x10u,4);                                                          // SUB ESP,10
        Memory<Adapter> memory{*this};
        logic::record_view::forward(memory,*this);
        this->reached(Branch::forwarded);
        gp[esp]=fl.plus(gp[esp],0x20u,4);                                                           // ADD ESP,20
        this->ret(0x304c8du,0u);this->publish(0x304c8du);
    }
};
template<class State,class Reference,class Forward> bool bridge(State &s,Reference reference,Forward forward) {
    return run_owner(s,0x304c50u,0x304c8du,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Forward>(v,c,t,forward);});
}
} // namespace halo2::native_record_view_detail
