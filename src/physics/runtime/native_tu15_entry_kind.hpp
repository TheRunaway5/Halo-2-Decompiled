#pragma once
// Boundary adapter for the entry kind check, entered at 1D9F50
// (native_tu15_entry_kind_systems.hpp). Its callers and the virtual call
// keep the original ABI, which the adapter owns: EDI the list; AL the
// result; ESI saved once there is an entry; the virtual function (through
// the unit's route, at the address its table names) gets ECX the
// interface, EAX the object and EDX the table, and every register it
// returns comes back; the flags of the last instruction that sets them;
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_ENTRY_KIND=1 restores the translated body.
// HALO2_VERIFY_ENTRY_KIND=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_entry_kind_systems.hpp"

namespace halo2::native_entry_kind_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_entry_kind;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ENTRY_KIND");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ENTRY_KIND");return p && *p;}();
    return value;
}
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Entry kind verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
enum Branch : unsigned {negative_index, past_count, indexed, kind_six, busy, available, branch_count};
inline constexpr Edge kind_edges[]{
    {0x1d9f87u,0x1d9f8fu,0},{0x1d9f87u,0x1d9fa3u,kind_six},{0x1d9f96u,0x1d9fa3u,busy},{0x1d9f96u,0x1d9f9fu,available},
};
template<class State,class Route> struct Kind : Port<State> {
    using Access=logic::entry_kind::Access;
    using Kept=logic::entry_kind::Kept;
    Route &route;
    Kind(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,kind_edges),route(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case index:return 0x1d9f50u;
        case count:return 0x1d9f57u;
        case entries:return 0x1d9f74u;
        case object:return 0x1d9f7eu;
        case interface:return 0x1d9f82u;
        case table:return 0x1d9f85u;
        case kind_entry:return 0x1d9f87u;
        case entries_again:return 0x1d9f8fu;
        case object_again:return 0x1d9f92u;
        case busy:return 0x1d9f96u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index:set_low_byte(gp[eax],v);this->test8(v);if(v&0x80u)this->reached(Branch::negative_index);break;
        // MOVSX ECX,AL; CMP ECX,EDX.
        case count:gp[edx]=v;gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(gp[eax])));this->compare32(gp[ecx],v);
            if(!(std::int32_t(gp[ecx])<std::int32_t(v)))this->reached(Branch::past_count);
            break;
        // PUSH ESI; LEA ESI,[ECX+ECX*2]; SHL ESI,5.
        case entries:gp[edx]=v;this->push(0x1d9f77u,gp[esi]);gp[esi]=this->flags.shift(gp[ecx]*3u,5u,4);break;
        case object:gp[eax]=v;break;
        case interface:case object_again:gp[ecx]=v;break;
        case table:gp[edx]=v;break;
        case entries_again:gp[eax]=v;break;
        case busy:set_low_byte(gp[eax],v);this->test8(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // (MOVSX AX,AL or OR EAX,-1;) MOVSX ECX,AX; XOR AL,AL; CMP ECX,-1.
    void indexed(std::uint32_t entry_index) {
        auto &gp=this->gp;
        if(entry_index==logic::none)gp[eax]=this->flags.logic(gp[eax]|logic::none,4);
        else{gp[eax]=(gp[eax]&0xffff0000u)|(entry_index&0xffffu);this->reached(Branch::indexed);}
        gp[ecx]=halo2::native_object_update::widen16(gp[eax]);set_low_byte(gp[eax],this->flags.logic(0u,1));
        this->compare32(gp[ecx],logic::none);
    }
    std::uint32_t kind(std::uint32_t entry,Kept &kept) {
        this->call_routed(route,0x1d9f87u,0x1d9f8au,entry);
        kept={this->gp[edi],this->gp[esi]};this->compare32(this->gp[eax],logic::unavailable_kind);   // CMP EAX,6
        return this->gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        using logic::entry_kind::Exit;
        Memory<Kind> memory{*this};
        const auto exit=logic::entry_kind::available(memory,*this,gp[edi]);
        if(exit==Exit::no_entry){this->ret(0x1d9fa4u,0);this->publish(0x1d9fa4u);return;}
        if(exit==Exit::available){set_low_byte(gp[eax],1u);gp[esi]=this->pop(0x1d9f9fu);this->ret(0x1d9fa0u,0);this->publish(0x1d9fa0u);return;}
        set_low_byte(gp[eax],this->flags.logic(0u,1));gp[esi]=this->pop(0x1d9fa3u);this->ret(0x1d9fa4u,0);this->publish(0x1d9fa4u);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d9f50u,0x1d9fa4u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Kind<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_entry_kind_detail
