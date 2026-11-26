#pragma once
// 10DFA0 receives an object handle in EBX and one stack argument (RET 4).
// Its dynamic callback receives the handle and argument on the stack, owns
// its own cleanup, and may change the live state used by the rest of the walk.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_object_type_notification_systems.hpp"

namespace halo2::native_object_type_notification_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic=halo2::native_object_type_notification;
using logic::Access;

inline bool reference_mode(){static const bool value=flag("HALO2_REFERENCE_OBJECT_TYPE_NOTIFICATION");return value;}
inline bool verify_mode(){static const bool value=flag("HALO2_VERIFY_OBJECT_TYPE_NOTIFICATION");return value;}
enum Branch : unsigned {empty_chain,skipped_callback,called_callback,continued,ended,branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
inline VerifySummary &summary(){static VerifySummary value{"Object type notification"};return value;}

template<class State,class Route>
struct Adapter : Port<State> {
    Route &route;
    Adapter(State &state,std::uint32_t *cursor,Trace<State> *trace,Route &handler)
        :Port<State>(state,cursor,trace,{}),route(handler){}
    std::uint32_t instruction(Access access,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(access) {
        case object_table:return 0x10dfa0u;
        case object_datums:return 0x10dfa6u;
        case object_address:gp[eax]=3u*this->flags.logic(gp[ebx]&0xffffu,4);return 0x10dfb3u;
        case type:return 0x10dfb7u;
        case argument:return 0x10dfbfu;
        case definition:return 0x10dfc5u;
        case first_type:return 0x10dfccu;
        case chain_type:return 0x10dfe0u;
        case callback:return 0x10dfe2u;
        case next_type:
            gp[esi]=this->flags.increment(gp[esi],4,false);
            gp[edx]=logic::widen16(gp[esi]&0xffffu);
            return 0x10dff1u;
        }
        return 0u;
    }
    void loaded(Access access,std::uint32_t address,std::uint32_t value) {
        auto &gp=this->gp;
        using enum Access;
        switch(access) {
        case object_table:gp[ecx]=value;break;
        case object_datums:gp[edx]=value;break;
        case object_address:case chain_type:gp[eax]=value;break;
        case type:gp[ecx]=logic::widen8(value);break;
        case argument:gp[ebp]=value;break;
        case definition:gp[edi]=value;break;
        case first_type:
            gp[ecx]=value;
            gp[eax]=address;
            gp[esi]=this->flags.logic(0u,4);
            this->test32(value);
            if(value==0u)this->reached(Branch::empty_chain);
            break;
        case callback:
            gp[eax]=value;
            this->test32(value);
            if(value==0u)this->reached(Branch::skipped_callback);
            break;
        case next_type:
            gp[ecx]=value;
            this->test32(value);
            gp[eax]=address;
            this->reached(value?Branch::continued:Branch::ended);
            break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t save_and_locate_argument() {
        this->push(0x10dfbeu,this->gp[ebp]);
        return this->gp[esp]+8u;
    }
    void save_chain_registers() {
        this->push(0x10dfc3u,this->gp[esi]);
        this->push(0x10dfc4u,this->gp[edi]);
    }
    void call(logic::Chain &current,std::uint32_t target) {
        auto &gp=this->gp;
        this->reached(Branch::called_callback);
        this->push(0x10dfe9u,current.argument);
        this->push(0x10dfeau,current.handle);
        this->call_routed(route,0x10dfebu,0x10dfedu,target);
        current={gp[ebx],gp[ebp],gp[edi],gp[esi]};
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::notify(memory,*this,gp[ebx]);
        gp[edi]=this->pop(0x10e003u);
        gp[esi]=this->pop(0x10e004u);
        gp[ebp]=this->pop(0x10e005u);
        this->ret(0x10e006u,4u);
        this->publish(0x10e006u);
    }
};

template<class State,class Reference,class Route>
bool bridge(State &state,Reference reference,Route route) {
    return run_owner(state,0x10dfa0u,0x10e006u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace){return Adapter<State,Route>{value,cursor,trace,route};});
}
} // namespace halo2::native_object_type_notification_detail
