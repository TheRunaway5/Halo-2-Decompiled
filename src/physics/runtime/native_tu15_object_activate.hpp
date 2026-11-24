#pragma once
// Boundary adapter for the object activation, entered at B92C0
// (native_tu15_object_activate_systems.hpp). Its callers and 1CC3F0 keep the
// original ABI, which the adapter owns: EDI the object handle; ESI saved;
// 1CC3F0 (through the unit's dispatch) gets ECX the handle, and every register it returns comes back;
// EAX (the header, or the attachment flags shifted, or the counter record),
// ECX (CL the header flags or type) and EDX (NONE, or 1 shifted by the
// type) as the original leaves them; the flags of the last instruction that
// sets them (the INC of the count included); RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_OBJECT_ACTIVATE=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ACTIVATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_activate_systems.hpp"

namespace halo2::native_object_activate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_activate;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_ACTIVATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_ACTIVATE");return p && *p;}();
    return value;
}
enum Branch : unsigned {already_active, no_word, parented, activated, notified, counted, branch_count};
static_assert(unsigned(logic::object_activate::Outcome::counted)==Branch::counted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object activate verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Notify> struct ObjectActivate : Port<State> {
    using Access=logic::object_activate::Access;
    using Kept=logic::object_activate::Kept;
    Notify &notify_child;
    ObjectActivate(State &s,std::uint32_t *c,Trace<State> *t,Notify &n):Port<State>(s,c,t,{}),notify_child(n){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xb92c0u;
        case header_datums:return 0xb92c6u;
        case header_flags:return 0xb92d3u;
        case word:this->gp[edx]=this->flags.logic(this->gp[edx]|logic::none,4);return 0xb92e2u;     // OR EDX,-1
        case object_address:this->push(0xb92e8u,this->gp[esi]);return 0xb92e9u;                     // PUSH ESI
        case parent:return 0xb92ecu;
        // OR CL,1 before MOV [EAX+2],CL.
        case header_store:set_low_byte(this->gp[ecx],this->flags.logic((this->gp[ecx]|logic::active)&0xffu,1));return 0xb92f4u;
        case type:return 0xb92f7u;
        case attachment:return 0xb9310u;
        case counter_record:return 0xb931au;
        case count:case count_store:return 0xb931fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[edi]&0xffffu,4)*3u;break;
        // MOV CL,[EDX+EAX*4+2]; TEST CL,1; LEA EAX,[EDX+EAX*4].
        case header_flags:set_low_byte(gp[ecx],v);this->test8(v&logic::active);gp[eax]=gp[edx]+4u*gp[eax];break;
        case word:fl.minus(v&0xffffu,gp[edx]&0xffffu,2);break;                                      // CMP word [EAX+4],DX
        case object_address:gp[esi]=v;break;
        case parent:fl.minus(v,gp[edx],4);break;                                                    // CMP [ESI+14],EDX
        // MOV CL,[EAX+3]; MOV EDX,1; SHL EDX,CL; TEST EDX,1883.
        case type:set_low_byte(gp[ecx],v);gp[edx]=fl.shift(1u,v&0xffu,4);this->test32(gp[edx]&logic::notified_types);break;
        // MOV EAX,[ESI+4]; SHR EAX,E; TEST AL,1.
        case attachment:gp[eax]=fl.right(v,logic::counted_bit,4,false);this->test8(gp[eax]&1u);break;
        case counter_record:gp[eax]=v;break;
        case count:fl.increment(v&0xffffu,2,false);break;                                           // INC word [EAX+4]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV ECX,EDI; CALL 1CC3F0.
    void notify(Kept &kept,std::uint32_t handle) {
        auto &gp=this->gp;
        gp[ecx]=handle;this->call_routed(notify_child,0xb930bu,0xb9310u,0x1cc3f0u);
        kept.object=gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::object_activate::Outcome;
        Memory<ObjectActivate> memory{*this};
        const auto result=logic::object_activate::activate(memory,*this,gp[edi]);
        this->reached(unsigned(result.outcome));
        if(result.notified)this->reached(Branch::notified);
        if(result.outcome!=Outcome::already_active && result.outcome!=Outcome::no_word)gp[esi]=this->pop(0xb9323u);
        this->ret(0xb9324u,0);this->publish(0xb9324u);
    }
};
template<class State,class Reference,class Notify> bool bridge(State &s,Reference reference,Notify notify) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xb92c0u,0xb9324u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ObjectActivate<State,Notify>(v,c,t,notify);});
    return true;
}
} // namespace halo2::native_object_activate_detail
