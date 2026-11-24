#pragma once
// Boundary adapter for the framed stamp advance, entered at 12F0C0
// (native_tu15_stamps_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns (RET): ESI and EDI saved; 12F040 (on a wrapped frame count) and 13FD10 get EAX as
// the original leaves it and the record,
// every register each returns coming back; EAX the stamp (one more where it
// advanced); the flags of the stamp's CMP or INC (or the renumbering's).
// Each access publishes its original instruction.
// HALO2_REFERENCE_STAMP_FRAMED=1 restores the translated body.
// HALO2_VERIFY_STAMP_FRAMED=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/native_tu15_stamps_systems.hpp"

namespace halo2::native_stamps_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace stamp_framed {
namespace logic=halo2::native_stamps;
using Access=logic::stamp::Access;
using Kept=logic::stamp::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_STAMP_FRAMED");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_STAMP_FRAMED");return v;}
enum Branch : unsigned {advanced, renumbered, wrapped, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Stamp framed"};return v;}
struct Sites {std::uint32_t frames,wrap_call,push_esi,record,stamp,push_edi,renumber_call,stamp_store,slot,pop,exit;};
inline constexpr Sites sites{0x12f0c0u,0x12f0c8u,0x12f0cdu,0x12f0ceu,0x12f0d4u,0x12f0d7u,0x12f0e3u,0x12f0ebu,0x12f0eeu,0x12f106u,0x12f108u};
template<class State,class Renumber,class Wrapped> struct Adapter : Port<State> {
    Renumber &renumber_child;
    Wrapped &wrapped_child;
    unsigned slot{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Renumber &r,Wrapped &w):Port<State>(s,c,t,{}),renumber_child(r),wrapped_child(w){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case frames:case frames_store:return sites.frames;
        case record:this->push(sites.push_esi,gp[esi]);return sites.record;                       // PUSH ESI
        case stamp:return sites.stamp;
        case stamp_store:return sites.stamp_store;
        case slot_store:return sites.slot+3u*slot++;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case record:gp[esi]=v;break;
        // MOV EAX,[ESI+38]; PUSH EDI; MOV EDI,7FFFFFFF; CMP EAX,EDI.
        case stamp:gp[eax]=v;this->push(sites.push_edi,gp[edi]);gp[edi]=logic::limit;this->compare32(v,logic::limit);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::frames_store)this->flags.increment(v-1u,4,false);                             // INC [4F2D58]
        if(a==Access::stamp_store){this->flags.increment(v-1u,4,false);this->gp[eax]=v;}         // INC EAX
    }
    // MOV EAX,ESI; CALL 13FD10.
    void renumber(Kept &kept) {
        this->gp[eax]=kept.record;
        this->call(sites.renumber_call,sites.renumber_call+5u,0x13fd10u,renumber_child);
        kept={this->gp[esi],this->gp[edi]};
    }
    void wrapped(){this->call(sites.wrap_call,sites.wrap_call+5u,0x12f040u,wrapped_child);}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        bool wrapped=false;
        this->reached(logic::stamp::advance_framed(memory,*this,wrapped)?Branch::renumbered:Branch::advanced);
        if(wrapped)this->reached(Branch::wrapped);
        gp[edi]=this->pop(sites.pop);gp[esi]=this->pop(sites.pop+1u);
        this->ret(sites.exit,0u);this->publish(sites.exit);
    }
};
template<class State,class Reference,class Renumber,class Wrapped>
bool bridge(State &s,Reference reference,Renumber renumber,Wrapped wrapped) {
    return run_owner(s,0x12f0c0u,sites.exit,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Renumber,Wrapped>(v,c,t,renumber,wrapped);});
}
} // namespace stamp_framed
} // namespace halo2::native_stamps_detail
