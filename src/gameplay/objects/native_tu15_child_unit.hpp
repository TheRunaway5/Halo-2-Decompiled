#pragma once
// Boundary adapter for the child unit search, entered at CB440
// (native_tu15_child_unit_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the object handle, the word on the stack
// (RET 4); EBP (BP the word meanwhile), EDI and, along the children, ESI
// saved; the child's handle or NONE in EAX; ECX (its low byte the last
// type) and EDX (1 shifted by it) as the original leaves them; the flags of
// the last instruction that sets them (the SHL's included). Each access
// publishes its original instruction.
// HALO2_REFERENCE_CHILD_UNIT=1 restores the translated body.
// HALO2_VERIFY_CHILD_UNIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_child_unit_systems.hpp"

namespace halo2::native_child_unit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_child_unit;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CHILD_UNIT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CHILD_UNIT");return p && *p;}();
    return value;
}
enum Branch : unsigned {childless, found, none_matched, branch_count};
static_assert(unsigned(logic::child_unit::Outcome::none_matched)==Branch::none_matched);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Child unit verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct ChildUnit : Port<State> {
    using Access=logic::child_unit::Access;
    bool walked{};
    ChildUnit(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // [ESP+8] after PUSH EBP.
    std::uint32_t word_slot()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:return 0xcb440u;
        case word:return 0xcb44cu;
        case header_datums:this->push(0xcb451u,gp[edi]);return 0xcb452u;                           // PUSH EDI
        case object_address:return 0xcb458u;
        case first_child:return 0xcb45cu;
        // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2] before MOV ESI,[EDI+ECX*4+8].
        case child_address:gp[ecx]=fl.logic(gp[eax]&0xffffu,4)*3u;return 0xcb470u;
        case type:return 0xcb474u;
        case child_word:return 0xcb486u;
        case sibling:return 0xcb48fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV ECX,[4ECBD0]; AND EAX,FFFF; PUSH EBP.
        case headers:gp[ecx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4);this->push(0xcb44bu,gp[ebp]);break;
        case word:gp[ebp]=(gp[ebp]&0xffff0000u)|(v&0xffffu);break;                                 // MOV BP,[ESP+8]
        case header_datums:gp[edi]=v;gp[edx]=gp[eax]*3u;break;                                     // MOV EDI,[ECX+44]; LEA EDX
        case object_address:gp[eax]=v;break;
        case first_child:gp[eax]=v;fl.minus(v,logic::none,4);break;                                 // MOV EAX,[EAX+10]; CMP EAX,-1
        case child_address:gp[esi]=v;break;
        // MOV CL,[ESI+AA]; MOV EDX,1; SHL EDX,CL; TEST DL,3.
        case type:set_low_byte(gp[ecx],v);gp[edx]=fl.shift(1u,v&0xffu,4);this->test8(gp[edx]&logic::unit_types);break;
        case child_word:fl.minus(v&0xffffu,gp[ebp]&0xffffu,2);break;                                // CMP word [ESI+1FC],BP
        case sibling:gp[eax]=v;fl.minus(v,logic::none,4);break;                                     // MOV EAX,[ESI+C]; CMP EAX,-1
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void walking(){this->push(0xcb464u,this->gp[esi]);walked=true;}                                // PUSH ESI
    void run() {
        auto &gp=this->gp;
        Memory<ChildUnit> memory{*this};
        const auto result=logic::child_unit::find(memory,*this,gp[eax]);
        this->reached(unsigned(result.outcome));
        gp[eax]=result.handle;
        if(walked)gp[esi]=this->pop(0xcb497u);
        gp[edi]=this->pop(0xcb498u);gp[ebp]=this->pop(0xcb499u);this->ret(0xcb49au,4u);this->publish(0xcb49au);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xcb440u,0xcb49au,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ChildUnit<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_child_unit_detail
