#pragma once
// Boundary adapter for the aligned query, entered at 2E9750
// (native_tu15_aligned_query_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the source, three arguments
// (RET C); EBP the frame (ESP aligned down to 16, then 0x7C below), ESI
// saved; 2E96A0 gets ECX the record, 30B890 ECX the record and five values
// pushed (RET 14), every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_ALIGNED_QUERY=1 restores the translated body.
// HALO2_VERIFY_ALIGNED_QUERY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_aligned_query_systems.hpp"

namespace halo2::native_aligned_query_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_aligned_query;
using Access=logic::aligned_query::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ALIGNED_QUERY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ALIGNED_QUERY");return v;}
enum Branch : unsigned {ran, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Aligned query"};return v;}
template<class State,class SetUp,class Run> struct Adapter : Port<State> {
    SetUp &set_up_child;
    Run &run_child;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,SetUp &u,Run &r):Port<State>(s,c,t,{}),set_up_child(u),run_child(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[ebp]+4u+4u*k;}                     // [EBP+8..10]
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case third:return 0x2e9765u;
        case second:return 0x2e9768u;
        case first:return 0x2e976bu;
        case source_second:this->push(0x2e976eu,this->gp[eax]);return 0x2e976fu;                     // PUSH EAX (the third)
        case source_first:this->push(0x2e9772u,this->gp[ecx]);return 0x2e9773u;                      // PUSH ECX (the second)
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case third:case source_second:gp[eax]=v;break;
        case second:case source_first:gp[ecx]=v;break;
        case first:gp[edx]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI; MOV ESI,ECX; LEA ECX,[ESP+10]; CALL 2E96A0.
    std::uint32_t set_up() {
        auto &gp=this->gp;
        this->push(0x2e9759u,gp[esi]);gp[esi]=gp[ecx];gp[ecx]=gp[esp]+0x10u;
        this->call(0x2e9760u,0x2e9765u,0x2e96a0u,set_up_child);
        return gp[esi];
    }
    // PUSH EDX; PUSH EAX; PUSH ECX; LEA ECX,[ESP+24]; CALL 30B890.
    void run(std::uint32_t first_value,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        this->push(0x2e9776u,first_value);this->push(0x2e9777u,gp[eax]);this->push(0x2e9778u,gp[ecx]);
        gp[ecx]=gp[esp]+0x24u;
        this->call(0x2e977du,0x2e9782u,0x30b890u,run_child);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x2e9750u,gp[ebp]);gp[ebp]=gp[esp];                                               // PUSH EBP; MOV EBP,ESP
        gp[esp]=fl.logic(gp[esp]&0xfffffff0u,4);gp[esp]=fl.minus(gp[esp],0x7cu,4);                   // AND ESP,-10; SUB ESP,7C
        Memory<Adapter> memory{*this};
        logic::aligned_query::run(memory,*this);
        this->reached(Branch::ran);
        gp[esi]=this->pop(0x2e9782u);gp[esp]=gp[ebp];gp[ebp]=this->pop(0x2e9785u);                   // POP ESI; MOV ESP,EBP; POP EBP
        this->ret(0x2e9786u,0xcu);this->publish(0x2e9786u);
    }
};
template<class State,class Reference,class SetUp,class Run> bool bridge(State &s,Reference reference,SetUp set_up,Run run) {
    return run_owner(s,0x2e9750u,0x2e9786u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,SetUp,Run>(v,c,t,set_up,run);});
}
} // namespace halo2::native_aligned_query_detail
